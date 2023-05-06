#ifndef __CONFIG_H__
#define __CONFIG_H__
#define __DEBUG__ 1
#define __DEBUG_SERIAL__ 1
#define WIFI_SSID "GTS_2BRG11"
#define WIFI_PASS "e7d64500"
//#define WIFI_SSID "SweetHome"
//#define WIFI_PASS "DoYr1941?"
//#define MQTT_SERVER "192.168.1.13"
#define MQTT_SERVER "94.199.70.134"
#define MQTT_CLIENT_ID "Thermostat01"
#define MQTT_USER "majordomo"
#define MQTT_PASSWORD "Recoot7ska"
#define MQTT_BASE "/SweetHome/Dacha/thermostat01/"
#define MQTT_DEBUG_TOPIC "/SweetHome/Dacha/debug"
#define TIMER_PERIOD 1000 
//#define DHT22_PIN D5
#define DHT_UPDATE_PERIOD 60
// DS20 sensors
#define DS20_PIN D3
#define DS20_UPDATE_PERIOD 60
#define DS20_PRECISION 9
#define RELE_COUNT 1
#define RELE_PINS {D5}
#endif
