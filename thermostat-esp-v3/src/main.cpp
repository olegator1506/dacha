#include <Arduino.h>
#include "debug.h"
#include "mynet.h"
#include "malloc.h"
#include "config.h"

// Global variables
time_t sensorLastUpdate = 0; 
String errorMessage;
bool _releState[RELE_COUNT];



#ifdef DHT_PIN
#include <DHTesp.h>
#endif

#ifdef DS20_PIN
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

#ifdef DHT22_PIN
  bool dhtSynced = false; 


#endif

#ifdef DS20_PIN
  OneWire oneWire(DS20_PIN);
  DallasTemperature ds20(&oneWire);
  uint8_t ds20Count;
  float *ds20Temperatures;
  DeviceAddress *ds20Address;

  void ds20PrintAddress(DeviceAddress deviceAddress)  {
    for (uint8_t i = 0; i < 8; i++)   {
    // zero pad the address if necessary
      if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
    }
    Serial.println("");
  }


  void sensorInit(){
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
  }


  bool sensorPoll(void) {
    bool result = false;
    float temperature;
    char strT[10], topic[20];
    for(uint8_t i =0; i<ds20Count;i++){
      DBG("DS20 request sensor # %d",i);
      if(!ds20.requestTemperaturesByAddress(ds20Address[i])) {
        DBG("Error");
        continue;
      }
      temperature = ds20.getTempC(ds20Address[i]);
      if(temperature == DEVICE_DISCONNECTED_C) continue;
      result = true;
      DBG("Temp %d = %f",i,temperature);
      sprintf(topic,"temperature%02d",i + 1);
      if(temperature > 0) strT[0] = '+'; 
      else if(temperature < 0) strT[0] = '-';
      else if( (temperature <0.1) && (temperature > -0.1)) strT[0] = ' ';
      dtostrf(temperature, 4, 1, strT+1);
      mqttPublish(topic,strT,true);
    }  
    return result;
  }


#elif defined DHT_PIN
  DHTesp dht;
  void sensorInit(void) {
    dht.setup(DHT_PIN,DHTesp::AUTO_DETECT);
  }

  bool sensorPoll(void){
    char strT[10],strH[10];
    float temperature = 0. , humidity = 0.;
    temperature = dht.getTemperature();
    humidity = dht.getHumidity();
    if(dht.getStatus() != DHTesp::ERROR_NONE) {
      DBG("Error");
      return false;
    }  
    if(temperature > 0) strT[0] = '+'; 
    else if(temperature < 0) strT[0] = '-';
    else if( (temperature <0.1) && (temperature > -0.1)) strT[0] = ' ';
    dtostrf(temperature, 4, 1, strT+1);
    dtostrf(humidity, 2, 0, strH);
    mqttPublish("temperature",strT,true);
    mqttPublish("humidity",strH,true);
    DBG("Temperature %s Humidity %s",strT,strH);
    return true;
  }
#endif


void setRele(uint8_t num,bool state) {
  int pins[] = RELE_PINS;
  DBG("Set rele #%d %s",num, state ? "ON":"OFF");
  _releState[num] = state;
  digitalWrite(pins[num],state); 
  digitalWrite(LED_BUILTIN,!state); 

}

bool execRequest(String topic, String data) {
  const char* pData = data.c_str();
  const char* pTopic = topic.c_str();
  DBG("Got request %s = %s",pTopic,pData);
  if(topic == "setRele01") {
    setRele(0,(data == "1")); 
  }
  return true;
}


void sensorLoop(){
  float temperature;
  char strT[10], topic[20];
  if(!mqttConnected()) return;
  time_t curT = millis() /1000;
  if((curT - sensorLastUpdate) < UPDATE_PERIOD)  return;
  if(sensorPoll())
  DBG("Sensor Loop");
  if(sensorPoll())  sensorLastUpdate = curT;
}





void setup(void) {
  
#ifdef __DEBUG__  
  debugMode = true;
#ifdef __DEBUG_SERIAL__
  debugSerial = true;
#endif  
  if(debugSerial){
    Serial.begin(115200);  while(!Serial);
  }
#endif
DBG("Start..");
//  os_timer_setfn(&myTimer,timerISR,NULL);
//  os_timer_arm(&myTimer,1,true);
  netInit(WIFI_SSID, WIFI_PASS, MQTT_SERVER, MQTT_CLIENT_ID, MQTT_BASE,MQTT_USER,MQTT_PASSWORD);
//  attachInterrupt(D5,czInterrupt,RISING);
  sensorInit();
  pinMode(LED_BUILTIN,OUTPUT);
  int pins[] = RELE_PINS;
  for(uint8_t i=0; i< RELE_COUNT;i++) {
    pinMode(pins[i],OUTPUT_OPEN_DRAIN);
    setRele(i,false);    
  }
  
}

void loop() {
  static unsigned int seconds =0;
  unsigned int s = millis() / 1000;
  int ms = millis() % 1000;
  if((s - seconds) == 10) {
    seconds = s;
    DBG("CurTime %lu. %lu",s,ms);
  } 
  netLoop();
  sensorLoop();
  delay(100);
}
