#include <painlessMesh.h>
#include <Arduino.h>

#define LED_BUILTIN 2
#define CTR_PIN     0

#define   MESH_PREFIX     "espMesh"
#define   MESH_PASSWORD   "12345678"
#define   MESH_PORT       5555
#define   ESP8266_NAME    "ctr_relay1"

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

//Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage(uint32_t index, uint32_t sta) {
  String msg = "ACK from esp8266, index: ";
  msg += index;
  msg += ", sta: ";
  msg += sta;
  // msg += ", Id: ";
  // msg += mesh.getNodeId();
  msg += ", name: ";
  msg += ESP8266_NAME;
  mesh.sendBroadcast( msg );
  Serial.printf("send = %s\n", msg.c_str());

  // taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  static uint32_t num = 0, sta = 0;
  char *p;
  char num_str[10];

  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  
  p = strstr(msg.c_str(), ESP8266_NAME);
  if (p != NULL)
  {
    strncpy(num_str, p + strlen(ESP8266_NAME) + 1, 1); // 跳过关键字
    sta = atoi(num_str); // 将剩余的字符串转换为整数
    Serial.printf("rx ok %d\n", sta);;
    digitalWrite(LED_BUILTIN, sta); 
    digitalWrite(CTR_PIN, sta); 
    num++;
    sendMessage(num, sta);
  }
  else
  {
    Serial.printf("deveice %s is not be controlled\n", ESP8266_NAME);
  }

}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(CTR_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, true); 
  digitalWrite(CTR_PIN, true); 

  Serial.begin(115200);
  Serial.printf("home-connection-esp8266, my name is %s\r\n", ESP8266_NAME);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  //userScheduler.addTask( taskSendMessage );
  //taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();

  //delay(200); 
  
}
