/*
 * This ESP8266 NodeMCU code was developed by newbiely.com
 *
 * This ESP8266 NodeMCU code is made available for public use without any restriction
 *
 * For comprehensive instructions and wiring diagrams, please visit:
 * https://newbiely.com/tutorials/esp8266/esp8266-rfid
 */
#include <Arduino.h>
#include "debug.h"
#include "mynet.h"
#include <SPI.h>
#include <MFRC522.h>
#include <Ticker.h>
#include "config.h"

enum TState {
  OHRANA, // Основное состояние - слежение за датчиком движения, при срабатывании датчика - переход в ATTENTION
  ATTENTION, // ожидание датчика RFID в течении ATTENTION_DELAY, по истечении таймаута включается ALARM, иначе - IDLE 
  IDLE, // Бездействие
  DELAY, // задержка перед вкючением охраны OHRANA_DELAY
  ALARM // Тревога, выключается сигналом от RFID или MQTT
};

enum TLedState {
  OFF, 
  BLINK_SLOW,
  BLINK_FAST,
  ON
};
TLedState _ledState;
TState _state, _nextState;
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
bool irState = false;
Ticker _ledTimer, _mainTimer;


void setState(TState state);
void ledTimerISR(void) {
//    DBG("Таймер сработал!");
    int ledState = digitalRead(LED_PIN);
    digitalWrite(LED_PIN,!ledState);
}

void mainTimerISR(void) {
  setState(_nextState);
  _mainTimer.detach();
}
void rfidLoop(void) {
  unsigned char _rfids[RFIDS_TOTAL][4]  = RFIDS;
  bool flag,success;
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    Serial.print("RFID/NFC Tag Type: ");
    Serial.println(rfid.PICC_GetTypeName(piccType));
  // print UID in Serial Monitor in the hex format
    Serial.print("UID:");
    for (int i = 0; i < rfid.uid.size; i++) {
      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(rfid.uid.uidByte[i], HEX);
    }
    rfid.PICC_HaltA(); // halt PICC
    rfid.PCD_StopCrypto1(); // stop encryption on PCD
    int success = -1;
    for(int i=0;i< RFIDS_TOTAL;i++) {
      flag = true ;
      for(int j=0;j< 4;j++) {
        if(_rfids[i][j] != rfid.uid.uidByte[j])
          flag = false;
      }
      if(flag) {
        DBG("Found good RFID # %d",i);
        success = true;
        break;
      }
    }
    if(success >= 0){
      char *names[RFIDS_TOTAL] = RFID_NAMES;
      mqttPublish("rfid",names[success], false);
      TState nextState;
      switch(_state) {
        case OHRANA:
        case ATTENTION:
        case DELAY:
        case ALARM:
          nextState = IDLE;
          mqttPublish("alarm","0",false);
          break;
        case IDLE:
          nextState = DELAY;
          break;
      }
      setState(nextState);
      return;
    } else DBG("Bad RFID!!!");
  }
}


void ledLoop(void) {
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
                                    // but actually the LED is on; this is because 
                                    // it is acive low on the ESP-01)
  delay(1000);                      // Wait for a second
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(2000);                      // Wait for two seconds (to demonstrate the active low LED)

}

bool execRequest(String topic, String data) {
  const char* pData = data.c_str();
  const char* pTopic = topic.c_str();
  DBG("Got request %s = %s",pTopic,pData);
  if (topic == "setAlarm") {
    if(data == "1") 
      setState(ALARM);
    else if(data == "0")
      setState(OHRANA);  
  }
  if (topic == "setGuard") {
    if(data == "1") 
      setState(OHRANA);
    else if(data == "0")
      setState(IDLE);  
  }

  return true;
}

void setLed(TLedState state) {
  _ledState = state;
  switch(_ledState) {
    case ON: 
      digitalWrite(LED_PIN, LOW );
      _ledTimer.detach();
      break;
    case OFF:
      digitalWrite(LED_PIN, HIGH );
      _ledTimer.detach();
      break;
    case BLINK_FAST:
      _ledTimer.attach(LED_BLINK_FAST_DELAY,ledTimerISR);
      break;
    case BLINK_SLOW:
      _ledTimer.attach(LED_BLINK_SLOW_DELAY,ledTimerISR);
      break;
  } 
  digitalWrite(BUILTIN_LED, state ? LOW : HIGH);
}

void setState(TState state) {
  switch(state) {
    case DELAY:
      DBG("Set state DELAY before arming");
      setLed(BLINK_SLOW);
      _nextState = OHRANA;
      _mainTimer.attach(OHRANA_DELAY, mainTimerISR);
      mqttPublish("motion","0",false);
      break;
    case OHRANA:
      DBG("Set state OHRANA");
      setLed(ON);
      mqttPublish("motion","0",false);
      mqttPublish("guard","1",false);
      break;
    case ATTENTION:
      DBG("Set state ATTENTION (delay beore alarm)");
      setLed(BLINK_FAST);
      _nextState = ALARM;
      _mainTimer.attach(ATTENTION_DELAY, mainTimerISR);
      mqttPublish("motion","1",false);
      break;
    case ALARM:
      DBG("!!!!!! ALARM !!!!!! ALARM !!!!!!");
      setLed(OFF);
      _mainTimer.detach();
      mqttPublish("alarm","1",false);
      break;
    case IDLE:
      DBG("Set state IDLE");
      _mainTimer.detach();
      setLed(OFF);
      mqttPublish("motion","0",false);
      break;  
  }
  _state = state;
}


void irLoop(){
  if(_state != OHRANA) return;
  int st = digitalRead(IR_PIN);
//  DBG("IR = %s", st ? "ON":"OF"); 
  if(st) {
    setState(ATTENTION);
  }  
}

void setup() {
  debugMode = true;
  Serial.begin(115200);  while(!Serial);

  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(IR_PIN, INPUT);
  netInit(WIFI_SSID, WIFI_PASS, MQTT_SERVER, MQTT_PORT, MQTT_CLIENT_ID, MQTT_BASE,MQTT_USER,MQTT_PASSWORD);
  Serial.println("Program started");
  setState(DELAY);
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
  rfidLoop();
  irLoop();
}

