#include <Arduino.h>
#include <DHTesp.h>
#include "debug.h"
#include "mynet.h"
#include "config.h"

DHTesp dht;
os_timer_t myTimer;
String errorMessage;
typedef struct {
    unsigned long seconds,microseconds;
} CurTime;
volatile CurTime curTime{0,0};
bool dhtSynced = false; 
time_t dhtLastUpdate = 0; 

bool execRequest(String topic, String data) {
  const char* pData = data.c_str();
  const char* pTopic = topic.c_str();
  DBG("Got request %s = %s",pTopic,pData);
  return true;
}

void timer1s(void){
}
/**
 * @summary Прерывания от таймера с периодом 1 ms 
**/

void  timerISR(void *arg){
    curTime.microseconds += TIMER_PERIOD;
    if(curTime.microseconds == 1000000L) {
      curTime.microseconds = 0;
      curTime.seconds++;
      timer1s();
    }
}

void dhtLoop(void) {
  char strT[10], strH[10];
  float temperature = 0;
  float humidity = 0;
  time_t curT = millis() /1000;
  if(((curT - dhtLastUpdate) < DHT_UPDATE_PERIOD) && dhtSynced) return;
  DHTesp *dht = new DHTesp;
  dht->setup(DHT22_PIN,DHTesp::DHT22);
  humidity = dht->getHumidity();
  temperature = dht->getTemperature();
  delete(dht);
  dhtLastUpdate = millis() / 1000;
  if(temperature > 0) strT[0] = '+'; 
  else if(temperature < 0) strT[0] = '-';
  else if( (temperature <0.1) && (temperature > -0.1)) strT[0] = ' ';
  dtostrf(temperature, 4, 1, strT+1);
  dtostrf(humidity, 2, 0, strH);
  mqttPublish("temperature",strT,true);
  mqttPublish("humidity",strH,true);
  DBG("Temperature %s Humidity %s",strT,strH);
  dhtSynced = true;
}



void setup(void) {
  
#ifdef __DEBUG__  
  debugMode = true;
#ifdef __DEBUG_SERIAL__
  debugSerial = true;
#endif  
  if(debugSerial){
    Serial.begin(76800);  while(!Serial);
  }
#endif
DBG("Start..");
  os_timer_setfn(&myTimer,timerISR,NULL);
  os_timer_arm(&myTimer,1,true);
  netInit(WIFI_SSID, WIFI_PASS, MQTT_SERVER, MQTT_CLIENT_ID, MQTT_BASE);
//  attachInterrupt(D5,czInterrupt,RISING);
}

void loop() {
  static unsigned int seconds =0;
  unsigned int s = curTime.seconds;
  if((s - seconds) == 10) {
    seconds = s;
    DBG("CurTime %lu. %lu",curTime.seconds,curTime.microseconds);
  } 
  netLoop();
  dhtLoop();
}
