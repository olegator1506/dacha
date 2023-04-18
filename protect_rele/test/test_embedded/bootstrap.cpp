#include <Arduino.h>
#include <unity.h>
#include "debug.h"
#include "rele.h"
#include "_config.h"

#define DEBUG_STR_LEN 100

Rele *rele;

extern bool _emulateMotion;
extern bool _emulateU;


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

// Проверка включения
void _assertOn() {
  TEST_ASSERT_EQUAL(true , rele->ledState());
  TEST_ASSERT_EQUAL(true , rele->outState());
  TEST_ASSERT_EQUAL(false , digitalRead(LED_PIN));
  TEST_ASSERT_EQUAL(true , digitalRead(PIN_OUTPUT));
}

// Проверка выключения
void _assertOff() {
  TEST_ASSERT_EQUAL(false , rele->ledState());
  TEST_ASSERT_EQUAL(false , rele->outState());
  TEST_ASSERT_EQUAL(true , digitalRead(LED_PIN));
  TEST_ASSERT_EQUAL(false , digitalRead(PIN_OUTPUT));
}

void _wait(unsigned long t) {
  DBG("%s %d",__FUNCTION__, t);
  for(unsigned long i =0; i< (t+10); i++) rele->loop();
}

void test_init() {
  DBG(__FUNCTION__);
  rele = new Rele();
  _assertOff();
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
// Ожидание отработки задержки включения  
  _wait(DELAY_ON);
  _assertOn();
    
}

void test_led() {
    DBG(__FUNCTION__);
    Rele *r = new Rele();
    r->ledOn();
    TEST_ASSERT_EQUAL(true , r->ledState());
    TEST_ASSERT_EQUAL(false , digitalRead(LED_PIN));
    r->ledOff();
    TEST_ASSERT_EQUAL(false , r->ledState());
    TEST_ASSERT_EQUAL(true , digitalRead(LED_PIN));
}

void test_motion_on() {
  DBG(__FUNCTION__);
  _emulateMotion = true;
  rele->loop();
  _assertOn();
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOff());
  _wait(DELAY_OFF);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOff();
}

void test_motion_off() {
  DBG(__FUNCTION__);
  _emulateMotion = false;
  rele->loop();
  _assertOff();
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _wait(DELAY_ON);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOn();
}

void test_u_on() {
  DBG(__FUNCTION__);
  _emulateU = true;
  rele->loop();
  _assertOn();
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOff());
  _wait(DELAY_OFF);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOff();
}


void test_u_off() {
  DBG(__FUNCTION__);
  _emulateU = false;
  rele->loop();
  _assertOff();
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _wait(DELAY_ON);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOn();
}

void test_both_on() {
  DBG(__FUNCTION__);
//Включаем датчик движения
  _emulateMotion = true;
  rele->loop();
  _assertOn();
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOff());
  _wait(DELAY_OFF);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOff();
//Включаем датчик движения
  _emulateU = true;
  rele->loop();
// Таймеры не должны запуститься  
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOff();
}

void test_both_off() {
  DBG(__FUNCTION__);
//Включаем датчик движения
  _emulateU = false;
  rele->loop();
  _assertOff();
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _emulateMotion = false;
  rele->loop();
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _wait(DELAY_ON);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOn();
}

void test_bounce_on() {
  DBG(__FUNCTION__);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _emulateMotion = true;
  rele->loop();
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_NOT_EQUAL(0 , rele->timerOff());
  _wait(10);
  _emulateMotion = false;
  rele->loop();
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOn();
}

void test_bounce_off() {
  DBG(__FUNCTION__);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
// Включаем датчик и ждем выключения
  _emulateMotion = true;
  _wait(DELAY_OFF);
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOff();
  _emulateMotion = false;
  _wait(10);
  _emulateMotion = true;
  rele->loop();
  TEST_ASSERT_EQUAL(0 , rele->timerOn());
  TEST_ASSERT_EQUAL(0 , rele->timerOff());
  _assertOff();
}


int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_motion_on);
  RUN_TEST(test_motion_off);
  RUN_TEST(test_u_on);
  RUN_TEST(test_u_off);
  RUN_TEST(test_both_on);
  RUN_TEST(test_both_off);
  RUN_TEST(test_bounce_on);
  RUN_TEST(test_bounce_off);
  return UNITY_END();
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  debugMode = true;
  delay(1000);
  runUnityTests();
  UNITY_END();
}

void loop(void){}

int main(void) {
  return runUnityTests();
}
