#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h> 
#include <ESP8266WiFi.h> 
#include <WiFiManager.h>
#include "PubSubClient.h"
#include "Ticker.h"
#include "EEPROM.h"
#include <stdio.h>
extern "C"{
#include "initialToken.h"
}



#define LED_BUILTIN 2
#define CTR_PIN     0

// ��Ҫ�޸ĵĵط�
char identifier_name1[] = "led_sta"; //��ģ������1


const char *mqtt_server = "183.230.40.96"; //onenet �� IP��ַ
const int port = 1883;                     //�˿ں�

//�շ���Ϣ������
char msgJson[100]; 
char dataPostTopic_str[100];
char dataPostReplyTopic_str[100];
char dataSetTopic_str[100];
char dataSetReplyTopic_str[100];
char dataSetReply_str[50];
//��Ϣģ��
char dataPostTopic[] = "$sys/%s/%s/thing/property/post"; //�����ϴ�
char dataPostReplyTopic[] = "$sys/%s/%s/thing/property/post/reply"; //�����ϴ��ظ� 
char dataSetTopic[] = "$sys/%s/%s/thing/property/set";  //��������
char dataSetReplyTopic[] = "$sys/%s/%s/thing/property/set_reply"; //�������ûظ� 

char dataPost[] = "{\"id\":\"123\",\"version\": \"1.0\",\"params\":{\"%s\":{\"value\":%s}}}";  //�����ϴ�����
char dataSetReply[] = "{\"id\":\"%s\",\"code\": 200,\"msg\":\"sucess\"}"; //�������óɹ��ظ�����
//��ʱ��,����ѭ���ϴ�����
Ticker tim1; 
//���������ʼ��
DynamicJsonDocument JSON_Buffer(2*1024); /* ����һ����СΪ2K��DynamicJsonDocument����JSON_Buffer,���ڴ洢�����л���ģ������ַ���ת����JSON��ʽ�ģ�JSON���ģ���ʽ�����Ķ��󽫴洢�ڶ��ڴ��У��Ƽ�size����1Kʱʹ�ø÷�ʽ */
oneNET_connect_msg_t oneNET_connect_msg;
WiFiClient espClient;           //����һ��WIFI���ӿͻ���
PubSubClient client(espClient); // ����һ��PubSub�ͻ���, ���봴����WIFI�ͻ���
//ȫ�ֶ���
bool led_state = false; //true-on


char mqtt_product_id[10];
char mqtt_device_name[40];
char mqtt_device_key[44];

unsigned long mtime = 0;

WiFiManager wm;


// TEST OPTION FLAGS
bool TEST_CP         = false; // always start the configportal, even if ap found
int  TESP_CP_TIMEOUT = 90; // test cp timeout

bool ALLOWONDEMAND   = true; // enable on demand
bool WMISBLOCKING    = true; // use blocking or non blocking mode, non global params wont work in non blocking

uint8_t BUTTONFUNC   = 0; // 0 resetsettings, 1 configportal, 2 autoconnect

//flag for saving data
bool shouldSaveConfig = false;
//flag for reset wifimanger
bool shouldRSTWM = false;


//////////////////////// wifi �ȵ�������� ///////////////////

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config"); 
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("[CALLBACK] configModeCallback fired");
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
}

void handleRoute(){
  Serial.println("[HTTP] handle custom route");
  wm.server->send(200, "text/plain", "hello from user code");
}

void handleNotFound(){
  Serial.println("[HTTP] override handle route");
  wm.handleNotFound();
}

void bindServerCallback(){
  wm.server->on("/custom",handleRoute);

  wm.server->on("/erase",handleNotFound); // disable erase
}

void wifiInfo(){
  Serial.println("[WIFI] SAVED: " + (String)(wm.getWiFiIsSaved() ? "YES" : "NO"));
  Serial.println("[WIFI] SSID: " + (String)wm.getWiFiSSID());
  Serial.println("[WIFI] PASS: " + (String)wm.getWiFiPass());

  Serial.println("[MQTT] mqtt_product_id : " + String(mqtt_product_id));
  Serial.println("[MQTT] mqtt_device_name : " + String(mqtt_device_name));
  Serial.println("[MQTT] mqtt_device_key : " + String(mqtt_device_key));
}
//////////////////////// wifi �ȵ�������ؽ��� ///////////////////



//////////////////////// MQTT �շ���� ///////////////////
//������(��ģ��)��������
void send_identifier_data()
{
  if (client.connected())
  {
    if(led_state == true)
      snprintf(msgJson, sizeof(msgJson), dataPost, identifier_name1, "true"); //����������dataTemplateģ����, ���ɵ��ַ�������msgJson
    else 
      snprintf(msgJson, sizeof(msgJson), dataPost, identifier_name1, "false");
    
    Serial.print("public the data:");
    Serial.println(msgJson);
    client.publish(dataPostTopic_str, (uint8_t *)msgJson, strlen(msgJson));//�������ݵ�����
    
  }
}
 
//�յ������·��Ļص�, ע������ص�Ҫʵ�������β� 1:topic ����, 2: payload: ���ݹ�������Ϣ 3: length: ����
void callback(char *topic, byte *payload, unsigned int length)
{
  JsonObject root; 

  Serial.println("message rev:");
  Serial.println(topic);
  // Serial.println(dataPostReplyTopic_str);
  // Serial.println(dataSetTopic_str);
  if(strcmp(topic, dataPostReplyTopic_str) == 0)
  {
    Serial.println("receive onenet PostReply");
  }
  else if(strcmp(topic, dataSetTopic_str) == 0) 
  {
    DeserializationError error = deserializeJson(JSON_Buffer, payload);
    if (!error)
    {
      root = JSON_Buffer.as<JsonObject>(); 
      if(JSON_Buffer["params"][identifier_name1] == true)
        led_state = true;
      else 
        led_state = false;
      digitalWrite(LED_BUILTIN, !led_state); 
      digitalWrite(CTR_PIN, !led_state);
      Serial.println("receive onenet ctr data");

      for (size_t i = 0; i < length; i++)
      {
        Serial.print((char)payload[i]);
      }

      // ���óɹ��ظ�
      const char* ID = root["id"];
      // Serial.println(ID);
      snprintf(dataSetReply_str, sizeof(dataSetReply_str), dataSetReply, ID); 
      // Serial.print("public the data:");
      // Serial.println(dataSetReply_str);
      client.publish(dataSetReplyTopic_str, (uint8_t *)dataSetReply_str, strlen(dataSetReply_str));
      send_identifier_data(); //������������
    }
    else
    {
      Serial.println("receive failed!!!");
    }
  }
  else
  {
    Serial.println("other topic");
    for (size_t i = 0; i < length; i++)
    {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  }

}
 
//��������, ����ͻ��˶���,����ͨ���˺�������
void clientReconnect()
{
  uint8_t recon_num = 5; //����5��

  while (!client.connected() && (recon_num--)) //�������ͻ���
  {
    Serial.println("reconnect MQTT...");
    if (client.connect(oneNET_connect_msg.device_name, oneNET_connect_msg.produt_id, oneNET_connect_msg.token))
    {
      Serial.println("connected");
      client.subscribe(dataPostReplyTopic_str); //�����豸�����ϱ���Ӧ����
      client.subscribe(dataSetTopic_str); //�����豸����������������
      
      shouldRSTWM = false;
    }
    else
    {
      Serial.println("failed");
      Serial.println(client.state());
      Serial.println("try again");
      Serial.println(oneNET_connect_msg.device_name);
      Serial.println(oneNET_connect_msg.produt_id);
      Serial.println(oneNET_connect_msg.token);
      digitalWrite(LED_BUILTIN, false); 
      delay(500);
      digitalWrite(LED_BUILTIN, true); //true - off
      delay(2000);

      shouldRSTWM = true;
    }
  }
}

//////////////////////// MQTT �շ���ؽ��� ///////////////////




void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(CTR_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !led_state); //true - off
  digitalWrite(CTR_PIN, !led_state);
  Serial.begin(115200);
  delay(1000);

  
  //////////////////////// wifi �ȵ�������� ///////////////////
  wm.setDebugOutput(true, WM_DEBUG_DEV);
  wm.debugPlatformInfo();

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if ( ! deserializeError ) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");
          strcpy(mqtt_product_id, json["mqtt_product_id"]);
          strcpy(mqtt_device_key, json["mqtt_device_key"]);
          Serial.println("[MQTT] mqtt_product_id : " + String(mqtt_product_id));
          Serial.println("[MQTT] mqtt_device_key : " + String(mqtt_device_key));

          const char* value = json["mqtt_device_name"]; 
          for (size_t i = 0; value[i] != '\0'; i++) 
          {
              mqtt_device_name[i] = value[i];
          }
          Serial.println("[MQTT] mqtt_device_name : " + String(mqtt_device_name));
          
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read


  // setup some parameters
  WiFiManagerParameter custom_html("<p style=\"color:pink;font-weight:Bold;\">OneNet Setting HTML</p>"); // only custom html
  WiFiManagerParameter custom_mqtt_product_id("product_id", "product_id", mqtt_product_id, 10);
  WiFiManagerParameter custom_mqtt_device_name("device_name", "device_name", mqtt_device_name, 40);
  WiFiManagerParameter custom_mqtt_device_key("device_key", "device_key", mqtt_device_key, 44);

  const char *bufferStr = R"(
  <!-- INPUT CHOICE -->
  <br/>
  <p>Select State</p>
  <input style='display: inline-block;' type='radio' id='on' name='program_selection' value='1'>
  <label for='on'>on</label><br/>
  <input style='display: inline-block;' type='radio' id='off' name='program_selection' value='0'>
  <label for='off'>off</label><br/>

  <!-- INPUT SELECT -->
  <br/>
  <label for='Mode_select'>Label for Mode Select</label>
  <select name="Mode_select" id="Mode_select" class="button">
  <option value="0">Mode 1</option>
  <option value="1" selected>Mode 2</option>
  <option value="2">Mode 3</option>
  <option value="3">Mode 4</option>
  </select>
  )";

  WiFiManagerParameter custom_html_inputs(bufferStr);

  // callbacks
  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setSaveParamsCallback(saveParamCallback);

  // add all your parameters here
  wm.addParameter(&custom_html);
  wm.addParameter(&custom_mqtt_product_id);
  wm.addParameter(&custom_mqtt_device_name);
  wm.addParameter(&custom_mqtt_device_key);

  wm.addParameter(&custom_html_inputs);

  // set custom html menu content , inside menu item "custom", see setMenu()
  const char* menuhtml = "<form action='/custom' method='get'><button>Custom</button></form><br/>\n";
  wm.setCustomMenuHTML(menuhtml);

  // invert theme, dark
  wm.setDarkMode(true);

  std::vector<const char *> menu = {"wifi","wifinoscan","info","param","custom","close","sep","erase","update","restart","exit"};

  
  if(!WMISBLOCKING){
    wm.setConfigPortalBlocking(false);
  }

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
  
  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback

  wifiInfo();

  // to preload autoconnect with credentials
  // wm.preloadWiFi("ssid","password");

  if(!wm.autoConnect("AutoConnectAP","12345678")) {
    Serial.println("failed to connect and hit timeout");
  }
  else if(TEST_CP) {
    // start configportal always
    delay(1000);
    Serial.println("TEST_CP ENABLED");
    wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
    wm.startConfigPortal("ConnectAP","12345678");
  }
  else {
    //if you get here you have connected to the WiFi
     Serial.println("connected...yeey :)");
  }
  
  
  //save the custom parameters to FS
  if (shouldSaveConfig) 
  {
    Serial.println("saving config");
 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["mqtt_product_id"] = custom_mqtt_product_id.getValue();
    json["mqtt_device_name"] = custom_mqtt_device_name.getValue();
    json["mqtt_device_key"] = custom_mqtt_device_key.getValue();

    Serial.println("\nparsed json");
    Serial.println(custom_mqtt_product_id.getValue());
    Serial.println(custom_mqtt_device_name.getValue());
    Serial.println(custom_mqtt_device_key.getValue());

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }
  wifiInfo();

  //////////////////////// wifi �ȵ�������ؽ��� ///////////////////


  //////////////////////// MQTT �շ���� ///////////////////
  Serial.println("OneNet Setting!");
  Serial.println(custom_mqtt_product_id.getValue());
  Serial.println(custom_mqtt_device_name.getValue());
  Serial.println(custom_mqtt_device_key.getValue());
  onenet_connect_msg_init(&oneNET_connect_msg, ONENET_METHOD_MD5, custom_mqtt_product_id.getValue(), custom_mqtt_device_name.getValue(), custom_mqtt_device_key.getValue());
  client.setServer(mqtt_server, port);                   //���ÿͻ������ӵķ�����,����Onenet������, ʹ��6002�˿�
  client.connect(oneNET_connect_msg.device_name, oneNET_connect_msg.produt_id, oneNET_connect_msg.token); //�ͻ������ӵ�ָ���Ĳ�Ʒ��ָ���豸.ͬʱ�����Ȩ��Ϣ
  if (client.connected())
  {
    Serial.println("OneNet is connected!");//�ж������ǲ���������.
  }
  client.setCallback(callback);                                //���úÿͻ����յ���Ϣ�ǵĻص�
  // �����ʽ���
  snprintf(dataPostTopic_str, sizeof(dataPostTopic_str), dataPostTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);
  snprintf(dataPostReplyTopic_str, sizeof(dataPostReplyTopic_str), dataPostReplyTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);
  snprintf(dataSetTopic_str, sizeof(dataSetTopic_str), dataSetTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);  
  snprintf(dataSetReplyTopic_str, sizeof(dataSetReplyTopic_str), dataSetReplyTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);
  //���ⶩ��
  client.subscribe(dataPostReplyTopic_str); //�����豸�����ϱ���Ӧ����
  client.subscribe(dataSetTopic_str); //�����豸����������������

  // tim1.attach(20, send_identifier_data);                            //��ʱÿ10�����һ�η������ݺ���sendTempAndHumi

  //////////////////////// MQTT �շ���ؽ��� ///////////////////
}


void loop() {
  static uint8_t rstWM_num = 0;

  if(!WMISBLOCKING){
    wm.process();
  }
  if (!client.connected()) //����ͻ���û����ONENET, ��������
  {
    clientReconnect();
    delay(100);
  }
  client.loop(); //�ͻ���ѭ�����

  // every 10 seconds
  if(millis()-mtime > 10000 ){
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Wifi connected)");
    }
    else 
    {
      Serial.println("No Wifi");  
      shouldRSTWM = true;
    }
    mtime = millis();
    
  }



  // û�����ӵ�onenet�Ĵ�������wifi����ʧ�ܺ�onenet����ʧ�ܣ� ����3��
  if (ALLOWONDEMAND == true && shouldRSTWM == true && (rstWM_num < 3)) //Ҳ�������������һ�������������õ�����
  {
    delay(100);
    Serial.println("wm resetSettings");
    shouldRSTWM = false;
    rstWM_num = rstWM_num + 1;

    // ���ж���������
    if(BUTTONFUNC == 0){
      wm.resetSettings();
      // wm.erase();
      wm.reboot();
      
      delay(200);
      return;
    }
    
    // start configportal ֻ������������
    if(BUTTONFUNC == 1){
      if (!wm.startConfigPortal("OnDemandAP","12345678")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
      }
      return;
    }

    //�Զ�����
    if(BUTTONFUNC == 2){
      wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
      wm.autoConnect();
      return;
    }
  }


  // put your main code here, to run repeatedly:
  delay(100);
}
