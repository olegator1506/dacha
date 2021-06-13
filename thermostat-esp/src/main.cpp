#include <Arduino.h>
#include "debug.h"
#include "mynet.h"
#include "malloc.h"
#include "config.h"

#ifdef DHT22_PIN
#include <DHTesp.h>
#endif

#ifdef DS20_PIN
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#ifdef DHT22_PIN
  DHTesp dht;
  bool dhtSynced = false; 
  time_t dhtLastUpdate = 0; 
#endif

#ifdef DS20_PIN
  OneWire oneWire(DS20_PIN);
  DallasTemperature ds20(&oneWire);
  uint8_t ds20Count;
  float *ds20Temperatures;
  DeviceAddress *ds20Address;
  time_t ds20LastUpdate = 0;

#endif

os_timer_t myTimer;
String errorMessage;
typedef struct {
    unsigned long seconds,microseconds;
} CurTime;
volatile CurTime curTime{0,0};

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

/*
void ds20PrintAddress(DeviceAddress deviceAddress)
{
  char s[20];
  for (uint8_t i = 0; i < 8; i++)
  {
    sprintf(s+i*2,"%02X",deviceAddress[i]);
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    DBG("Device Address %s);
  }
}
*/
void ds20PrintAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
    Serial.println("");
}


void ds20Init(){
#ifdef DS20_PIN  
  DBG("Init DS20");
  ds20.begin();
  ds20Count = ds20.getDeviceCount();
  if(ds20Count == 0) {
    DBG("No DS20 sensors found");
    return;
  }
  DBG("Found %d DS sensors",ds20Count);
  ds20Temperatures = (float*)calloc(ds20Count, sizeof(float));
  ds20Address = (DeviceAddress*)calloc(ds20Count,sizeof(DeviceAddress));
  for(uint8_t i = 0; i<ds20Count;i++){
    ds20.getAddress(ds20Address[i],i);
    ds20.setResolution(ds20Address[i], DS20_PRECISION);
  }
  for(uint8_t i = 0; i<ds20Count;i++)  
    ds20PrintAddress( ds20Address[i]);

#endif
}

void ds20Loop(){
#ifdef DS20_PIN  
  float temperature;
  char strT[10], topic[20];
  if(!mqttReady()) return;
  time_t curT = millis() /1000;
  if((curT - ds20LastUpdate) < DS20_UPDATE_PERIOD)  return;
  ds20LastUpdate = curT;
//DBG("DS20 Loop");
  ;
  for(uint8_t i =0; i<ds20Count;i++){
    DBG("DS20 request sensor # %d",i);
    if(!ds20.requestTemperaturesByAddress(ds20Address[i])) {
      DBG("Error");
      continue;
    }
    temperature = ds20.getTempC(ds20Address[i]);
//    if(temperature == DEVICE_DISCONNECTED_C) continue;
    DBG("Temp %d = %f",i,temperature);
    sprintf(topic,"temperature%02d",i + 1);
    if(temperature > 0) strT[0] = '+'; 
    else if(temperature < 0) strT[0] = '-';
    else if( (temperature <0.1) && (temperature > -0.1)) strT[0] = ' ';
    dtostrf(temperature, 4, 1, strT+1);
    mqttPublish(topic,strT,true);
  }  
#endif
}

void  timerISR(void *arg){
    curTime.microseconds += TIMER_PERIOD;
    if(curTime.microseconds == 1000000L) {
      curTime.microseconds = 0;
      curTime.seconds++;
      timer1s();
    }
}

void dhtLoop(void) {
#ifdef DHT22_PIN
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
#endif
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
  ds20Init();
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
  ds20Loop();
}
