#include <Arduino.h>
#include "config.h"

bool motionDetected = false;
int lightLevel = 0;
bool _lightState = false;
int _outLevel = 0;

void adjustLight(void) {
  int l;
  for(l = 0; l <=255;l++) {
    analogWrite(OUT_PIN,l);
    delay(10);
    int ll = 1024 - analogRead(LIGHT_SENSOR_PIN);
    if(ll > LIGHT_LEVEL) break;
      
  }
    Serial.print("Out level ");Serial.println(l);
}

void light(bool state){
    if(state == _lightState) return;
    _lightState = state;
    if(!state) {
      analogWrite(OUT_PIN, 0);
      _outLevel = 0;
    }
    else {
      adjustLight();
    }
    digitalWrite(LED_BUILTIN, state);
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
        light(true);
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
  analogWrite(OUT_PIN,0);
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,LOW);
  pinMode(LIGHT_SENSOR_PIN,INPUT);
  Serial.println("Program started");
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