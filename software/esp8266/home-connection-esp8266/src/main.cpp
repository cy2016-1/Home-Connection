#include <Arduino.h>
#include "WiFi.h"
#include "PubSubClient.h"
#include "Ticker.h"
#include "initialToken.h"
 
const char *ssid = "leadinno";               //wifi��
const char *password = "lead2801";       //wifi����
const char *mqtt_server = "183.230.40.96"; //onenet �� IP��ַ
const int port = 1883;                     //�˿ں�
 
#define mqtt_devid "desktop_led" //�豸ID desktop_led ��Կ SnRmaVBMbUxtN1lQTU43RmF5ZjhmejFZM0RNWlVBRG0=
#define mqtt_pubid "tJZPdrj1bL"        //��ƷID tJZPdrj1bL ��Ʒ��Կ kfcPMeFvN8Hf31+Heyd9mzc7ueWrR8MLwXPR
//��Ȩ��Ϣ
#define mqtt_password "version=2018-10-31&res=products%2FtJZPdrj1bL%2Fdevices%2Fdesktop_led&et=4092512761&method=md5&sign=MUV%2BKFLzv81a4Bw6BDrChQ%3D%3D" //��Ȩ��Ϣ
 
oneNET_connect_msg_t oneNET_connect_msg;
onenet_connect_msg_init(&oneNET_connect_msg,ONENET_METHOD_MD5);

WiFiClient espClient;           //����һ��WIFI���ӿͻ���
PubSubClient client(espClient); // ����һ��PubSub�ͻ���, ���봴����WIFI�ͻ���
 
char msgJson[75]; //������Ϣ������
//��Ϣģ��
char dataTemplate[] = "{\"id\":123,\"dp\":{\"temp\":[{\"v\":%.2f}],\"humi\":[{\"v\":%.2f}]}}";
Ticker tim1; //��ʱ��,����ѭ���ϴ�����
 
//����WIFI��غ���
void setupWifi()
{
  delay(10);
  Serial.println("����WIFI");
  WiFi.begin(ssid, password);
  while (!WiFi.isConnected())
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("OK");
  Serial.println("Wifi���ӳɹ�");
}
 
//�յ������·��Ļص�, ע������ص�Ҫʵ�������β� 1:topic ����, 2: payload: ���ݹ�������Ϣ 3: length: ����
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
 
//�����ⷢ��ģ�����ʪ������
void sendTempAndHumi()
{
  if (client.connected())
  {
    snprintf(msgJson, 75, dataTemplate, 22.5, 35.6); //��ģ����ʪ����������dataTemplateģ����, ���ɵ��ַ�������msgJson
    Serial.print("public the data:");
    Serial.println(msgJson);
    client.publish("$sys/tJZPdrj1bL/desktop_led/dp/post/json", (uint8_t *)msgJson, strlen(msgJson));
    //�������ݵ�����
  }
}
 
//��������, ����ͻ��˶���,����ͨ���˺�������
void clientReconnect()
{
  while (!client.connected()) //�������ͻ���
  {
    Serial.println("reconnect MQTT...");
    if (client.connect(mqtt_devid, mqtt_pubid, mqtt_password))
    {
      Serial.println("connected");
      client.subscribe("$sys/tJZPdrj1bL/desktop_led/cmd/request/#"); //���������·�����
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
  Serial.begin(115200);                                  //��ʼ������
  delay(3000);                                           //�����ʱ��Ϊ�����Ҵ򿪴�������
  setupWifi();                                           //���ú�������WIFI
  client.setServer(mqtt_server, port);                   //���ÿͻ������ӵķ�����,����Onenet������, ʹ��6002�˿�
  client.connect(mqtt_devid, mqtt_pubid, mqtt_password); //�ͻ������ӵ�ָ���Ĳ�Ʒ��ָ���豸.ͬʱ�����Ȩ��Ϣ
  if (client.connected())
  {
    Serial.println("OneNet is connected!");//�ж������ǲ���������.
  }
  client.setCallback(callback);                                //���úÿͻ����յ���Ϣ�ǵĻص�
  client.subscribe("$sys/tJZPdrj1bL/desktop_led/cmd/request/#"); //���������·�����
  tim1.attach(10, sendTempAndHumi);                            //��ʱÿ10�����һ�η������ݺ���sendTempAndHumi
}
 
void loop()
{
  if (!WiFi.isConnected()) //�ȿ�WIFI�Ƿ�������
  {
    setupWifi();
  }
  if (!client.connected()) //����ͻ���û����ONENET, ��������
  {
    clientReconnect();
    delay(100);
  }
  client.loop(); //�ͻ���ѭ�����
}