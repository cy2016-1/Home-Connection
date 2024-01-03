// #include <Arduino.h>

// #define LED_BUILTIN 8   //ESP32ģ�������LED����ӦGPIO8���͵�ƽ��

// // put function declarations here:
// int myFunction(int, int);

// void setup() {
//   // put your setup code here, to run once:
//   Serial.begin(115200);//���ڲ���������
//   pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
// }

// void loop() {
//   // put your main code here, to run repeatedly:
//   Serial.print("Hello world!\r\n");//���ڴ�ӡ
  
//   digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on
//   delay(20);                      // Wait for a second
//   digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
//   delay(200);                      // Wait for two seconds (to demonstrate the active low LED)

// }


#include <painlessMesh.h>
#include "OneButton.h"


#define   KEY_PIN          9  //��������
#define   LED_PIN          8  //LED����
#define   ACTION_DELAY_TIM_S  3  //��ʱ����

#define   MESH_PREFIX     "espMesh"
#define   MESH_PASSWORD   "12345678"
#define   MESH_PORT       5555

OneButton button(KEY_PIN, true);

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
uint8_t ctr_relay1_sta = 1;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
void action_delay() ;

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage );
Task taskAction_delay( TASK_SECOND, TASK_FOREVER, &action_delay );

void ledBlink() {
  digitalWrite(LED_PIN, LOW);
  delay(20); 
  digitalWrite(LED_PIN, HIGH);
  delay(20); 
  digitalWrite(LED_PIN, LOW);
  delay(20); 
  digitalWrite(LED_PIN, HIGH);
}

/////////////////////////////////MESH////////////////////////////////////////////////  
void sendMessage() {
  String msg = "esp32 ctr_relay1:";
  msg += ctr_relay1_sta;
  msg = msg + ", Id:" + mesh.getNodeId();
  mesh.sendBroadcast( msg );
  Serial.printf("send = %s\n", msg.c_str());
  //taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));  //�߳�����ʱ
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


/////////////////////////////////BUTTON//////////////////////////////////////////////// 


void action_delay() {
  static uint8_t time_num = 0;
  // taskAction_delay.setInterval(TASK_SECOND * ACTION_DELAY_TIM_S );  //�߳�����ʱ
  if (time_num ++ > ACTION_DELAY_TIM_S)
  {
    time_num = 0;
    ctr_relay1_sta = !ctr_relay1_sta;
    sendMessage();
    taskAction_delay.disable();
  }
}

void doubleclick()
{
  Serial.print("doubleclick");
}

void click()
{
  ctr_relay1_sta = 0;
  sendMessage();  
  taskAction_delay.enable();
}

void LongPressStart(void *oneButton)
{
  ctr_relay1_sta = 0;
  sendMessage();
  // Serial.print(((OneButton *)oneButton)->getPressedMs());
  Serial.print("\t - LongPressStart()\n");
}

void LongPressStop(void *oneButton)
{
  // Serial.print(((OneButton *)oneButton)->getPressedMs());
  Serial.print("\t - LongPressStop()\n");
  taskAction_delay.enable();
}


/////////////////////////////////MAIN//////////////////////////////////////////////// 
void setup() {
  button.attachClick(click); //����
  button.attachDoubleClick(doubleclick);  //˫��
  button.attachLongPressStart(LongPressStart, &button);  //������ʼ
  button.attachLongPressStop(LongPressStop, &button);  //�����ͷ�
  button.setLongPressIntervalMs(1000);

  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.print("home-connection-esp32!\r\n");
  ledBlink();

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  userScheduler.addTask( taskAction_delay );
  //taskSendMessage.enable();
}

void loop() {
  button.tick();

  mesh.update();
  
  delay(20); 
  //Serial.print("Hello world!\r\n");
}



