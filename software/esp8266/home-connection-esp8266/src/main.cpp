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

// ��Ҫ�޸ĵĵط�
char identifier_name1[] = "led_sta"; //��ģ������1

const char *ssid = "9x";               //wifi��
const char *password = "1984832368";       //wifi����

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
 


//����WIFI��غ���
void setupWifi()
{
  delay(10);
  Serial.println("����WIFI");
  WiFi.begin(ssid, password);
  while (!WiFi.isConnected())
  {
    Serial.print(".");
    //ָʾ����ʾ
    digitalWrite(LED_BUILTIN, false); //true - off
    delay(500);
    digitalWrite(LED_BUILTIN, true); //true - off
  }

  Serial.println("OK");
  Serial.println("Wifi���ӳɹ�");
}

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
  while (!client.connected()) //�������ͻ���
  {
    Serial.println("reconnect MQTT...");
    if (client.connect(oneNET_connect_msg.device_name, oneNET_connect_msg.produt_id, oneNET_connect_msg.token))
    {
      Serial.println("connected");
      client.subscribe(dataPostReplyTopic_str); //�����豸�����ϱ���Ӧ����
      client.subscribe(dataSetTopic_str); //�����豸����������������
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
  Serial.begin(115200);                                  //��ʼ������
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(CTR_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !led_state); //true - off
  digitalWrite(CTR_PIN, !led_state);

  setupWifi();                     //���ú�������WIFI

  onenet_connect_msg_init(&oneNET_connect_msg,ONENET_METHOD_MD5);
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