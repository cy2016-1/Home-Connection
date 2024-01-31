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

// 需要修改的地方
char identifier_name1[] = "led_sta"; //物模型名称1

const char *ssid = "9x";               //wifi名
const char *password = "1984832368";       //wifi密码

const char *mqtt_server = "183.230.40.96"; //onenet 的 IP地址
const int port = 1883;                     //端口号

//收发信息缓冲区
char msgJson[100]; 
char dataPostTopic_str[100];
char dataPostReplyTopic_str[100];
char dataSetTopic_str[100];
char dataSetReplyTopic_str[100];
char dataSetReply_str[50];
//信息模板
char dataPostTopic[] = "$sys/%s/%s/thing/property/post"; //数据上传
char dataPostReplyTopic[] = "$sys/%s/%s/thing/property/post/reply"; //数据上传回复 
char dataSetTopic[] = "$sys/%s/%s/thing/property/set";  //数据设置
char dataSetReplyTopic[] = "$sys/%s/%s/thing/property/set_reply"; //数据设置回复 

char dataPost[] = "{\"id\":\"123\",\"version\": \"1.0\",\"params\":{\"%s\":{\"value\":%s}}}";  //数据上传内容
char dataSetReply[] = "{\"id\":\"%s\",\"code\": 200,\"msg\":\"sucess\"}"; //数据设置成功回复内容
//定时器,用来循环上传数据
Ticker tim1; 
//第三方库初始化
DynamicJsonDocument JSON_Buffer(2*1024); /* 申明一个大小为2K的DynamicJsonDocument对象JSON_Buffer,用于存储反序列化后的（即由字符串转换成JSON格式的）JSON报文，方式声明的对象将存储在堆内存中，推荐size大于1K时使用该方式 */
oneNET_connect_msg_t oneNET_connect_msg;
WiFiClient espClient;           //创建一个WIFI连接客户端
PubSubClient client(espClient); // 创建一个PubSub客户端, 传入创建的WIFI客户端
WiFiManager wifiManager;
//全局定义
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

uint8_t BUTTONFUNC   = 1; // 0 resetsettings, 1 configportal, 2 autoconnect

//flag for saving data
bool shouldSaveConfig = false;

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
  // can contain gargbage on esp32 if wifi is not ready yet
  Serial.println("[WIFI] WIFI_INFO DEBUG");
  // WiFi.printDiag(Serial);
  Serial.println("[WIFI] SAVED: " + (String)(wm.getWiFiIsSaved() ? "YES" : "NO"));
  Serial.println("[WIFI] SSID: " + (String)wm.getWiFiSSID());
  Serial.println("[WIFI] PASS: " + (String)wm.getWiFiPass());

  Serial.println("[MQTT] mqtt_product_id : " + String(mqtt_product_id));
  Serial.println("[MQTT] mqtt_device_name : " + String(mqtt_device_name));
  Serial.println("[MQTT] mqtt_device_key : " + String(mqtt_device_key));
}

void setup() {
  Serial.begin(115200);
  delay(1000);


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

  //reset settings - for testing
  // wm.resetSettings();
  // wm.erase();

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
    //read updated parameters
    strcpy(mqtt_product_id, custom_mqtt_product_id.getValue());
    strcpy(mqtt_device_name, custom_mqtt_device_name.getValue());
    strcpy(mqtt_device_key, custom_mqtt_device_key.getValue());

    Serial.println("saving config");
 #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["mqtt_product_id"] = mqtt_product_id;
    json["mqtt_device_name"] = mqtt_device_name;
    json["mqtt_device_key"] = mqtt_device_key;


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
}


void loop() {

  if(!WMISBLOCKING){
    wm.process();
  }


  // is configuration portal requested?
  if (ALLOWONDEMAND == LOW ) {
    delay(100);
    if (BUTTONFUNC == 2){
      Serial.println("BUTTON PRESSED");

      // button reset/reboot
      if(BUTTONFUNC == 0){
        wm.resetSettings();
        wm.reboot();
        delay(200);
        return;
      }
      
      // start configportal
      if(BUTTONFUNC == 1){
        if (!wm.startConfigPortal("OnDemandAP","12345678")) {
          Serial.println("failed to connect and hit timeout");
          delay(3000);
        }
        return;
      }

      //test autoconnect as reconnect etc.
      if(BUTTONFUNC == 2){
        wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
        wm.autoConnect();
        return;
      }
    
    }
    else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
  }

  // every 10 seconds
  if(millis()-mtime > 10000 ){
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Wifi connected)");
    }
    else Serial.println("No Wifi");  
    mtime = millis();
  }
  // put your main code here, to run repeatedly:
  delay(100);
}
