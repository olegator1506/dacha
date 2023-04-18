 #include <Arduino.h>
 #include "debug.h"
 #include <stdarg.h>
 #include "mynet.h"
 #include "config.h"

#define DEBUG_STR_LEN 100

extern void mqttPublish(const char *path, const char *data,bool wait);
bool debugMode = false;
bool debugSerial = false;

void _debugOut(char *buf){
  char buf2[DEBUG_STR_LEN + 20];
  unsigned long t = millis();
  sprintf(buf2,"%lu.%lu> %s",t / 1000, t % 1000, buf);
  if(debugSerial)
    Serial.println(buf2);
/*    
  else {
    if(buf[strlen(buf2)-1] == '\n') buf[strlen(buf)-1] ='\0';
    mqttPublish(MQTT_DEBUG_TOPIC, buf2);
  }  
*/  
}


void DBGT(const char *fmt, ... ){
  char buf[DEBUG_STR_LEN]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, DEBUG_STR_LEN-1, (const char *)fmt, args);
  va_end (args);
  _debugOut(buf);
 }


void DBG(const char *fmt, ... ){
  if(!debugMode) return;
  char buf[DEBUG_STR_LEN]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, DEBUG_STR_LEN-1, (const char *)fmt, args);
  va_end (args);
  _debugOut(buf);
 }
#ifdef F 
void DBG(const __FlashStringHelper *fmt, ... ){
  if(!debugMode) return;
  char buf[DEBUG_STR_LEN]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf_P(buf, DEBUG_STR_LEN, (const char*)fmt, args);
  va_end (args);
  _debugOut(buf);
}
#endif

/*
void checkFreeRam(){
#ifdef __DEBUG__
  extern int __heap_start, *__brkval; 
  static int freeRam;
  int curFree =  (int) &curFree - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
  if(curFree != freeRam) {
    freeRam = curFree;
    DBG(printf("Free RAM = %d\n",freeRam));
    if(freeRam < 500){
      Serial.print("Free RAM "); Serial.println(freeRam);
    }  
  } 
#endif
}
*/