#include <Arduino.h>

#define LED_BUILTIN 2   //ESP-01S模块自身的LED，对应GPIO2，低电平亮

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  Serial.begin(115200);//串口波特率配置
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Hello world!\r\n");//串口打印
  
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
  delay(1000);                      // Wait for a second
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(2000);                      // Wait for two seconds (to demonstrate the active low LED)

}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}