// #include <Arduino.h>

// #define LED_BUILTIN 8   //ESP32模块自身的LED，对应GPIO8，低电平亮

// // put function declarations here:
// int myFunction(int, int);

// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(115200);//串口波特率配置
//   pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
// }

// void loop() {
//   // put your main code here, to run repeatedly:
//   Serial.print("Hello world!\r\n");//串口打印
  
//   digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on
//   delay(20);                      // Wait for a second
//   digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
//   delay(200);                      // Wait for two seconds (to demonstrate the active low LED)

// }


#include <painlessMesh.h>

#define   MESH_PREFIX     "espMesh"
#define   MESH_PASSWORD   "12345678"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
uint8_t ctr_relay1_sta = 1;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND / 5 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  String msg = "esp32 ctr_relay1:";
  msg += ctr_relay1_sta;
  msg = msg + ", Id:" + mesh.getNodeId();
  mesh.sendBroadcast( msg );
  Serial.printf("send = %s\n", msg.c_str());
  //taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));  //线程内延时
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
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
  Serial.begin(115200);
  Serial.print("home-connection-esp32!\r\n");

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();

  //delay(2000); 
  //Serial.print("Hello world!\r\n");
}
