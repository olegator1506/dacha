#ifndef __MYNET_H__
#define __MYNET_H__
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

extern void netInit(char *wifiSsid, char *wifiPass,char *mqttServer, char *mqttId,char *mqttBase);
extern void netLoop(void);
extern void mqttPublish(const char *path, const char *data, bool wait = false);
extern bool mqttReady(void);
#endif 

