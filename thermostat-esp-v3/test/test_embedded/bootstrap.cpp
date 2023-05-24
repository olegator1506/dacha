#include <Arduino.h>
#include <unity.h>
#include "mynet.h"
#include "config.h"

#define _DEBUG_MODE_
#define DEBUG_STR_LEN 100

String errorMessage;

#ifdef _DEBUG_MODE_
void DBG(const char *fmt, ... ){

  char buf[DEBUG_STR_LEN-10]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, DEBUG_STR_LEN-11, (const char *)fmt, args);
  va_end (args);
  unsigned long t = millis();	
//  snprintf(buf,DEBUG_STR_LEN-1,"%lu.%lu >%s",t / 1000,t % 1000, s);
  Serial.printf("%lu>%s\n",t, buf);
}
#else 
void DBG(const char *fmt, ... ){}
#endif


bool execRequest(String topic, String data){
  Serial.printf("%s = %s\n", topic.c_str(), data.c_str());
  return true;
}




void test_gpio() {
  int pin;
  int pins[] = {D0,D1,D2,D3,D4,D5,D6,D7,D8,D9,0};
  for( int i =0; pins[i]!= 0; i++) {
    pin = pins[i];
    Serial.printf("Test pin %d\n",pin);  
    pinMode(pin, INPUT);
    TEST_ASSERT_EQUAL(true , digitalRead(pin));
    pinMode(pin, OUTPUT);
    digitalWrite(pin, true);
    TEST_ASSERT_EQUAL(true , digitalRead(pin));
    digitalWrite(pin, false);
    TEST_ASSERT_EQUAL(false , digitalRead(pin));
  }
}

void test_wifi() {
  analogWriteFreq(20000);
  pinMode(D5, OUTPUT_OPEN_DRAIN);
  netInit(WIFI_SSID, WIFI_PASS, MQTT_SERVER,"espTest",MQTT_BASE,"","");
  for(int i=0; i< 5000; i++) {
    netLoop();
    delay(1);
  }
  TEST_ASSERT_EQUAL( WL_CONNECTED, WiFi.status());
  TEST_ASSERT_EQUAL( true, mqttConnected());
  while(1) netLoop();
}

void test_led(void) {
  int _pin = LED_BUILTIN;
  pinMode(_pin,OUTPUT);
  bool state = false;
  for(;;) {
    DBG("Led %s", state ? "On":"Off");
    digitalWrite(_pin,state);
    state = !state;
    delay(1000);
  }
}


int runUnityTests(void) {
  UNITY_BEGIN();
//  RUN_TEST(test_wifi);
  RUN_TEST(test_led);
  return UNITY_END();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  runUnityTests();
}

void loop(void){}

int main(void) {
  return runUnityTests();
}
