#include <Arduino.h>
#include "WiFi.h"
#include "PubSubClient.h"
#include "Ticker.h"
#include "initialToken.h"
 
const char *ssid = "leadinno";               //wifi名
const char *password = "lead2801";       //wifi密码
const char *mqtt_server = "183.230.40.96"; //onenet 的 IP地址
const int port = 1883;                     //端口号
 
#define mqtt_devid "desktop_led" //设备ID desktop_led 密钥 SnRmaVBMbUxtN1lQTU43RmF5ZjhmejFZM0RNWlVBRG0=
#define mqtt_pubid "tJZPdrj1bL"        //产品ID tJZPdrj1bL 产品密钥 kfcPMeFvN8Hf31+Heyd9mzc7ueWrR8MLwXPR
//鉴权信息
#define mqtt_password "version=2018-10-31&res=products%2FtJZPdrj1bL%2Fdevices%2Fdesktop_led&et=4092512761&method=md5&sign=MUV%2BKFLzv81a4Bw6BDrChQ%3D%3D" //鉴权信息
 
oneNET_connect_msg_t oneNET_connect_msg;
onenet_connect_msg_init(&oneNET_connect_msg,ONENET_METHOD_MD5);

WiFiClient espClient;           //创建一个WIFI连接客户端
PubSubClient client(espClient); // 创建一个PubSub客户端, 传入创建的WIFI客户端
 
char msgJson[75]; //发送信息缓冲区
//信息模板
char dataTemplate[] = "{\"id\":123,\"dp\":{\"temp\":[{\"v\":%.2f}],\"humi\":[{\"v\":%.2f}]}}";
Ticker tim1; //定时器,用来循环上传数据
 
//连接WIFI相关函数
void setupWifi()
{
  delay(10);
  Serial.println("连接WIFI");
  WiFi.begin(ssid, password);
  while (!WiFi.isConnected())
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("OK");
  Serial.println("Wifi连接成功");
}
 
//收到主题下发的回调, 注意这个回调要实现三个形参 1:topic 主题, 2: payload: 传递过来的信息 3: length: 长度
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.println("message rev:");
  Serial.println(topic);
  for (size_t i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
 
//向主题发送模拟的温湿度数据
void sendTempAndHumi()
{
  if (client.connected())
  {
    snprintf(msgJson, 75, dataTemplate, 22.5, 35.6); //将模拟温湿度数据套入dataTemplate模板中, 生成的字符串传给msgJson
    Serial.print("public the data:");
    Serial.println(msgJson);
    client.publish("$sys/tJZPdrj1bL/desktop_led/dp/post/json", (uint8_t *)msgJson, strlen(msgJson));
    //发送数据到主题
  }
}
 
//重连函数, 如果客户端断线,可以通过此函数重连
void clientReconnect()
{
  while (!client.connected()) //再重连客户端
  {
    Serial.println("reconnect MQTT...");
    if (client.connect(mqtt_devid, mqtt_pubid, mqtt_password))
    {
      Serial.println("connected");
      client.subscribe("$sys/tJZPdrj1bL/desktop_led/cmd/request/#"); //订阅命令下发主题
    }
    else
    {
      Serial.println("failed");
      Serial.println(client.state());
      Serial.println("try again in 5 sec");
      delay(5000);
    }
  }
}
 
void setup()
{
  Serial.begin(115200);                                  //初始化串口
  delay(3000);                                           //这个延时是为了让我打开串口助手
  setupWifi();                                           //调用函数连接WIFI
  client.setServer(mqtt_server, port);                   //设置客户端连接的服务器,连接Onenet服务器, 使用6002端口
  client.connect(mqtt_devid, mqtt_pubid, mqtt_password); //客户端连接到指定的产品的指定设备.同时输入鉴权信息
  if (client.connected())
  {
    Serial.println("OneNet is connected!");//判断以下是不是连好了.
  }
  client.setCallback(callback);                                //设置好客户端收到信息是的回调
  client.subscribe("$sys/tJZPdrj1bL/desktop_led/cmd/request/#"); //订阅命令下发主题
  tim1.attach(10, sendTempAndHumi);                            //定时每10秒调用一次发送数据函数sendTempAndHumi
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