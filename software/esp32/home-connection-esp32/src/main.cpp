#include <Arduino.h>
#include <ArduinoJson.h>
// #include <WiFi.h>
#include <WiFiManager.h>
#include "PubSubClient.h"
#include "Ticker.h"
#include "EEPROM.h"
#include "FS.h"
#include <SPIFFS.h>
#include <stdio.h>
extern "C"{
#include "initialToken.h"
}

#define DEEPSLEEPTIME      60000  // 无操作多少毫秒后进入深度休眠 10min - 600000

#define   KEY_PIN          1  //按键引脚
#define   LED_BUILTIN      8  //LED引脚
#define OPEN        false
#define CLOSE       true
#define OFFLINE    "T"  //离线模式判断字符
#define ONFLAG     "O"  //开关判断字符

// 需要修改的地方
char identifier_name1[] = "ir_sta"; //物模型名称1

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
WiFiManager wm;
//全局定义
bool ir_state = false; //true-on

char offline_sta[3], on_off_sta[3]; //离线模式判断 开关状态

unsigned long sleep_count_millis = 0;



char mqtt_product_id[10];
char mqtt_device_name[40];
char mqtt_device_key[44];

unsigned long mtime = 0;


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


//////////////////////// wifi 热点配网相关 ///////////////////

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
  static uint32_t print_num = 0;

  Serial.println("Print num: " + (String)(print_num++));

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("[WIFI] STATE: Wifi connected");
  }
  else 
  {
    Serial.println("[WIFI] STATE: No Wifi");
  }

  Serial.println("[WIFI] SAVED: " + (String)(wm.getWiFiIsSaved() ? "YES" : "NO"));
  Serial.println("[WIFI] SSID: " + (String)wm.getWiFiSSID());
  Serial.println("[WIFI] PASS: " + (String)wm.getWiFiPass());
  
  // Serial.println("[MQTT] mqtt_product_id : " + String(custom_mqtt_product_id.getValue()));
  // Serial.println("[MQTT] mqtt_device_key : " + String(custom_mqtt_device_key.getValue()));
  // Serial.println("[MQTT] mqtt_device_name : " + String(custom_mqtt_device_name.getValue()));

  if(client.connected()){
    Serial.println("[MQTT] STATE: Connected");
  }
  else 
  {
    Serial.println("[MQTT] STATE: Disconnected");
  }

  Serial.println("[MQTT] mqtt_product_id : " + String(mqtt_product_id));
  Serial.println("[MQTT] mqtt_device_key : " + String(mqtt_device_key));
  Serial.println("[MQTT] mqtt_device_name : " + String(mqtt_device_name));

  Serial.println("[BUTTON] button mode offline sta : " + String(((String)offline_sta == OFFLINE)  ? "OFFLINE" : "ONLIE"));
  Serial.println("[BUTTON] button onoff sta : " + String(((String)on_off_sta == ONFLAG)  ? "ON" : "OFF"));
}
//////////////////////// wifi 热点配网相关结束 ///////////////////



//////////////////////// MQTT 收发相关 ///////////////////
//向主题(物模型)发送数据
void send_identifier_data()
{
  if (client.connected())
  {
    if(ir_state == true)
      snprintf(msgJson, sizeof(msgJson), dataPost, identifier_name1, "true"); //将数据套入dataTemplate模板中, 生成的字符串传给msgJson
    else 
      snprintf(msgJson, sizeof(msgJson), dataPost, identifier_name1, "false");
    
    Serial.print("public the data:");
    Serial.println(msgJson);
    client.publish(dataPostTopic_str, (uint8_t *)msgJson, strlen(msgJson));//发送数据到主题
    
  }
}
 
//收到主题下发的回调, 注意这个回调要实现三个形参 1:topic 主题, 2: payload: 传递过来的信息 3: length: 长度
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
      // root = JSON_Buffer.as<JsonObject>(); 
      // if(JSON_Buffer["params"][identifier_name1] == true)
      //   ir_state = true;
      // else 
      //   ir_state = false;
      // digitalWrite(LED_BUILTIN, !ir_state); 
      Serial.println("receive onenet ctr data");

      for (size_t i = 0; i < length; i++)
      {
        Serial.print((char)payload[i]);
      }

      // 设置成功回复
      const char* ID = root["id"];
      // Serial.println(ID);
      snprintf(dataSetReply_str, sizeof(dataSetReply_str), dataSetReply, ID); 
      // Serial.print("public the data:");
      // Serial.println(dataSetReply_str);
      client.publish(dataSetReplyTopic_str, (uint8_t *)dataSetReply_str, strlen(dataSetReply_str));
      send_identifier_data(); //更新线上数据
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
 

 
//重连函数, 如果客户端断线,可以通过此函数重连
void clientReconnect()
{
  uint8_t recon_num = 5; //重连5次

  while (!client.connected() && (recon_num--)) //再重连客户端
  {
    Serial.println("reconnect MQTT...");
    if (client.connect(oneNET_connect_msg.device_name, oneNET_connect_msg.produt_id, oneNET_connect_msg.token))
    {
      Serial.println("connected");
      client.subscribe(dataPostReplyTopic_str); //订阅设备属性上报响应主题
      client.subscribe(dataSetTopic_str); //订阅设备属性设置请求主题

      shouldRSTWM = false;
    }
    else
    {
      Serial.println("failed");
      Serial.println(client.state());
      Serial.println("try again in 2 sec");
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

//////////////////////// MQTT 收发相关结束 ///////////////////

/////////////////////////////////IRQ//////////////////////////////////////////////// 
void KEY_PIN_IRQ() // 检测到上升沿则继续延时 
{
  Serial.println("irq check!");
  sleep_count_millis = millis();
  ir_state = true;
  // send_identifier_data();
}

/////////////////////////////////MAIN//////////////////////////////////////////////// 
void setup() {
  Serial.begin(115200);                                  //初始化串口
  Serial.print("home-connection-esp32!\r\n");
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(KEY_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(KEY_PIN), KEY_PIN_IRQ, RISING); //中断的方式最快
  digitalWrite(LED_BUILTIN, CLOSE); //true - off
  esp_deep_sleep_enable_gpio_wakeup(1ULL << 1 , ESP_GPIO_WAKEUP_GPIO_HIGH); //ESP_GPIO_WAKEUP_GPIO_HIGH 高电平唤醒

   //////////////////////// wifi 热点配网相关 ///////////////////
  wm.setDebugOutput(true, WM_DEBUG_DEV);
  wm.debugPlatformInfo();

  if (SPIFFS.begin(true)) {
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

          strcpy(offline_sta, json["OFFLINE"]);
          strcpy(on_off_sta, json["ONFLAG"]);

          wifiInfo();
          if((String)offline_sta == OFFLINE)
          {
            if((String)on_off_sta == ONFLAG)
            {
              digitalWrite(LED_BUILTIN, OPEN); //true - off
            }
            else
            {
              digitalWrite(LED_BUILTIN, CLOSE); 
            }
          }
          else
          {
              digitalWrite(LED_BUILTIN, CLOSE); //true - off
          }
          
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
  WiFiManagerParameter custom_mqtt_product_id("product_id", "product_id", mqtt_product_id, 10);
  WiFiManagerParameter custom_mqtt_device_name("device_name", "device_name", mqtt_device_name, 40);
  WiFiManagerParameter custom_mqtt_device_key("device_key", "device_key", mqtt_device_key, 44);
  const char _customHtml_checkbox[] = "type=\"checkbox\""; 
  WiFiManagerParameter custom_offline_checkbox("offline_checkbox", "Mode offline", OFFLINE, 2, _customHtml_checkbox, WFM_LABEL_AFTER);
  WiFiManagerParameter custom_onoff_checkbox("onoff_checkbox", "open", ONFLAG, 2, _customHtml_checkbox, WFM_LABEL_AFTER);
  WiFiManagerParameter custom_html("<p style=\"color:pink;font-weight:Bold;\">OneNet Setting HTML</p>"); // only custom html

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

  wm.addParameter(&custom_offline_checkbox);
  wm.addParameter(&custom_onoff_checkbox);
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
  
  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback

  if((String)offline_sta != OFFLINE)
  {
    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep in seconds
    Serial.println("cfg TESP_CP_TIMEOUT");
    wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
  }

  wifiInfo();

  // to preload autoconnect with credentials
  // wm.preloadWiFi("ssid","password");

  if(!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
  }
  else if(TEST_CP) {
    // start configportal always
    delay(1000);
    Serial.println("TEST_CP ENABLED");
    wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
    wm.startConfigPortal();
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
    json["OFFLINE"] = custom_offline_checkbox.getValue();
    json["ONFLAG"] = custom_onoff_checkbox.getValue();
    strcpy(offline_sta, json["OFFLINE"]);
    strcpy(on_off_sta, json["ONFLAG"]);

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

  if((String)offline_sta == OFFLINE)
  {
    wm.reboot();
  }

  //////////////////////// wifi 热点配网相关结束 ///////////////////


  //////////////////////// MQTT 收发相关 ///////////////////
  Serial.println("OneNet Setting!");
  Serial.println(custom_mqtt_product_id.getValue());
  Serial.println(custom_mqtt_device_name.getValue());
  Serial.println(custom_mqtt_device_key.getValue());
  onenet_connect_msg_init(&oneNET_connect_msg, ONENET_METHOD_MD5, custom_mqtt_product_id.getValue(), custom_mqtt_device_name.getValue(), custom_mqtt_device_key.getValue());
  client.setServer(mqtt_server, port);                   //设置客户端连接的服务器,连接Onenet服务器, 使用6002端口
  client.connect(oneNET_connect_msg.device_name, oneNET_connect_msg.produt_id, oneNET_connect_msg.token); //客户端连接到指定的产品的指定设备.同时输入鉴权信息
  if (client.connected())
  {
    Serial.println("OneNet is connected!");//判断以下是不是连好了.
  }
  client.setCallback(callback);                                //设置好客户端收到信息是的回调
  // 主题格式组包
  snprintf(dataPostTopic_str, sizeof(dataPostTopic_str), dataPostTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);
  snprintf(dataPostReplyTopic_str, sizeof(dataPostReplyTopic_str), dataPostReplyTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);
  snprintf(dataSetTopic_str, sizeof(dataSetTopic_str), dataSetTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);  
  snprintf(dataSetReplyTopic_str, sizeof(dataSetReplyTopic_str), dataSetReplyTopic, oneNET_connect_msg.produt_id, oneNET_connect_msg.device_name);
  //主题订阅
  client.subscribe(dataPostReplyTopic_str); //订阅设备属性上报响应主题
  client.subscribe(dataSetTopic_str); //订阅设备属性设置请求主题

  // tim1.attach(20, send_identifier_data);                            //定时每10秒调用一次发送数据函数sendTempAndHumi

  //////////////////////// MQTT 收发相关结束 ///////////////////  
}




void loop() {
  
  static uint8_t rstWM_num = 0;

  if(!WMISBLOCKING){
    wm.process();
  }
  if (!client.connected()) //如果客户端没连接ONENET, 重新连接
  {
    clientReconnect();
    delay(100);
  }
  client.loop(); //客户端循环检测

  // every 10 seconds
  if(millis()-mtime > 10000 ){
    wifiInfo();
    mtime = millis();
    if(WiFi.status() != WL_CONNECTED){
      shouldRSTWM = true;
    }
    
  }


  // 没有连接到onenet的处理（包括wifi连接失败和onenet连接失败） 尝试3次
  if (ALLOWONDEMAND == true && shouldRSTWM == true && (rstWM_num < 3)) //也可以在这里添加一个按键触发配置的条件
  {
    delay(100);
    Serial.println("wm resetSettings");
    shouldRSTWM = false;
    rstWM_num = rstWM_num + 1;

    // 所有都重新配置
    if(BUTTONFUNC == 0){
      // wm.resetSettings();
      // wm.erase();
      wm.reboot();
      
      delay(200);
      return;
    }
    
    // start configportal 只进行重新配网
    if(BUTTONFUNC == 1){
      if (!wm.startConfigPortal()) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
      }
      return;
    }

    //自动重连
    if(BUTTONFUNC == 2){
      wm.setConfigPortalTimeout(TESP_CP_TIMEOUT);
      wm.autoConnect();
      return;
    }
  }


  // put your main code here, to run repeatedly:
  delay(100);

  // 延时关灯和进入低功耗
  if(millis()-sleep_count_millis > DEEPSLEEPTIME)  
  {
    ir_state = false;
    send_identifier_data();

    Serial.print("deep_sleep!\r\n");
    delay(1000);

    gpio_deep_sleep_hold_dis();
    esp_deep_sleep_enable_gpio_wakeup(1ULL << 1 , ESP_GPIO_WAKEUP_GPIO_HIGH); //ESP_GPIO_WAKEUP_GPIO_HIGH 高电平唤醒
    pinMode(KEY_PIN, INPUT_PULLUP);
    esp_deep_sleep_start();
  }
  else if(ir_state == true)
  {
    send_identifier_data();
    delay(100);
    ir_state = false;
  }

}



