#define DEBUG_MODE 1
#include <Arduino.h>
#include "rele.h"
#include "mynet.h"
#include "_config.h"
#include "dbg.h"



Rele *rele;
String errorMessage;

bool execRequest(String topic, String data) {
  return true;
}
void setup() {
  Serial.begin(115200);
  Serial.println("");
  delay(1000);
  debugMode = true;
  debugSerial = true;
  netInit(WIFI_SSID, WIFI_PASS,MQTT_SERVER, MQTT_ID, MQTT_BASE, "","");
//  WiFi.mode(WIFI_STA);
//  WiFi.begin(WIFI_SSID, WIFI_PASS);
  rele = new Rele();
}

void loop() {
  rele->loop();
//  netLoop();
}