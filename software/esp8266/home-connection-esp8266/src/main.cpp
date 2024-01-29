#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h> 
#include <WiFiManager.h>
#include "PubSubClient.h"
#include "Ticker.h"
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
//全局定义
bool led_state = false; //true-on
 


//连接WIFI相关函数
void setupWifi()
{
  delay(10);
  Serial.println("连接WIFI");
  WiFi.begin(ssid, password);
  while (!WiFi.isConnected())
  {
    Serial.print(".");
    //指示灯提示
    digitalWrite(LED_BUILTIN, false); //true - off
    delay(500);
    digitalWrite(LED_BUILTIN, true); //true - off
  }

  Serial.println("OK");
  Serial.println("Wifi连接成功");
}

//向主题(物模型)发送数据
void send_identifier_data()
{
  if (client.connected())
  {
    if(led_state == true)
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
  while (!client.connected()) //再重连客户端
  {
    Serial.println("reconnect MQTT...");
    if (client.connect(oneNET_connect_msg.device_name, oneNET_connect_msg.produt_id, oneNET_connect_msg.token))
    {
      Serial.println("connected");
      client.subscribe(dataPostReplyTopic_str); //订阅设备属性上报响应主题
      client.subscribe(dataSetTopic_str); //订阅设备属性设置请求主题
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
      delay(2000);
      digitalWrite(LED_BUILTIN, true); //true - off
    }
  }
}
 
void setup()
{
  Serial.begin(115200);                                  //初始化串口
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(CTR_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !led_state); //true - off
  digitalWrite(CTR_PIN, !led_state);

  setupWifi();                     //调用函数连接WIFI

  onenet_connect_msg_init(&oneNET_connect_msg,ONENET_METHOD_MD5);
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
}
 
void loop()
{
  if (!WiFi.isConnected()) //先看WIFI是否还在连接
  {
    setupWifi();
  }
  if (!client.connected()) //如果客户端没连接ONENET, 重新连接
  {
    clientReconnect();
    delay(100);
  }
  client.loop(); //客户端循环检测
}