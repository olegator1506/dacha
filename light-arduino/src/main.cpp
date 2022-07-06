#include <Arduino.h>

#define PIR_PIN 2
#define LIGHT_SENSOR_PIN A0
#define OUT_PIN 4
#define MOTION_PAUSE 120
//#define LIGHT_LEVEL 1000
#define LIGHT_LEVEL 100

bool motionDetected = false;
int lightLevel = 0;

void light(bool state){
    digitalWrite(OUT_PIN, state);
    digitalWrite(LED_BUILTIN, state);
    Serial.print("Light "); Serial.println(state ? "ON":"OFF");
}

void pirLoop(){
  static bool state = false;
  static unsigned long pulseTime=0;    
  unsigned long t = millis() /1000;
  if(motionDetected && ((t - pulseTime) > MOTION_PAUSE)){
    pulseTime = 0;
    Serial.println("Motion OFF");
    motionDetected = false;
    light(false);
  } 

  bool s = digitalRead(PIR_PIN);
  if(s != state)  {
   state = s;    
   if(state) {
    Serial.println("PIR pulse");
     if(lightLevel <= LIGHT_LEVEL){
        light(true);
    }
    state = true;
    pulseTime = t;
    if(!motionDetected) {
      motionDetected = true;
      Serial.println("Motion ON");
   }
   }
  }    
}





void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN,INPUT);
  pinMode(OUT_PIN,OUTPUT);
  digitalWrite(OUT_PIN,LOW);
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);
  pinMode(LIGHT_SENSOR_PIN,INPUT);
  
}

void loop() {
  static unsigned long curTime = 0; 
  unsigned long t = millis() / 1000;
  if(t > curTime) {
    curTime = t;
    lightLevel = 1024 - analogRead(LIGHT_SENSOR_PIN);
    Serial.print("Light level ");Serial.println(lightLevel);
  }
  pirLoop();
}