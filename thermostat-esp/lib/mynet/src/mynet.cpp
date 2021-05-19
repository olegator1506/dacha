#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <MQTT.h>
#include <ESP8266WebServer.h>
#include "config.h"
#include "debug.h" 

#define KEEP_ALIVE_PERIOD 60


char *_wifiSsid = NULL, *_wifiPass=NULL, *_mqttServer = NULL, *_mqttId = NULL,*_mqttBase=NULL;
bool _mqttConnected = false;
WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
WiFiClient wifiClient;
MQTTClient mqttClient;
static ESP8266WebServer server(80);
unsigned long _lastPubTime =0;
String _lastMqttPubTopic="",_lastMqttPubValue="";
void netLoop(void);


extern bool execRequest(String topic, String data);
extern String errorMessage;
extern void DBG(const char *fmt, ... );

static void httpHandleNotFound() {
  String message = "Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


static void httpHandleRoot() {
  server.send(200, "text/plain", "Ok\r\n");
  String cmd = server.arg("cmd");
  String data = server.arg("data");
  DBG("Got HTTP request. cmd = %s data = %s",cmd.c_str(),data.c_str());
  if((cmd == "") || (data == "")) {
    DBG("Invalid request");
    server.send(400, "text/plain", "Invalid request");
  }
  if(!execRequest(cmd,data))  
    server.send(400, "text/plain", errorMessage);
}

void httpInit(void) {
    server.on("/", httpHandleRoot);
    server.onNotFound(httpHandleNotFound);
    server.begin();
}




/**
 * Проверяем не я вляется ли принятое сообщение последним опубликованым
 * Возвращает true если топик и значение совпадают с последним отправленым и 
 * время с последней публикации < 500 mc
**/
bool _mqttIsLoopBack(String topic, String value) {
  if((millis() - _lastPubTime) > 500 ) return false;
  if(topic != _lastMqttPubTopic) return false;
  if(value != _lastMqttPubValue) return false;
  return true;
}


void _mqttMessage(String &topic, String &payload){
  String s;
  int l = strlen(_mqttBase);
   s = topic.substring(0,l);
  if(s != String(_mqttBase)) return;
  s = topic.substring(l);
  if(s == "debug") return;
  if(s == "upTime") return;
  if(_mqttIsLoopBack(s,payload)) return;
//  Serial.print("Got MQTT message:"); Serial.print(s);Serial.print(" = "); Serial.println(payload);
  execRequest(s,payload);
}

void otaInit(){
//    Serial.println("OTA init");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA start");
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}


void mqttLoop(){
  if(WiFi.status() != WL_CONNECTED) return;
  if(!mqttClient.connected()){
//    Serial.print("MQTT client disconnected. Try connect...");
      if(mqttClient.connect(_mqttId)) {
//        Serial.println("Success");
        mqttClient.subscribe(String(_mqttBase) + String("#"));
      } 
/*      else {
          Serial.println("Fail");
      }    
*/
  }
  mqttClient.loop();
}
bool mqttReady(void){
  if(WiFi.status() != WL_CONNECTED) return false;
  return mqttClient.connected();
}

void mqttPublish(const char *path, const char *data,bool wait=false){
    char bufPath[255];
    if(!mqttClient.connected()) return;
    snprintf(bufPath, sizeof(bufPath),"%s%s",_mqttBase,path);
//    Serial.print("Publish ");Serial.print(bufPath);Serial.print(" ");Serial.println(data);
    mqttClient.publish(bufPath,data);
    if(strcmp(path,"debug") == 0 ) return;
    if(strcmp(path,"upTime") == 0 ) return;
    _lastPubTime = millis();
    _lastMqttPubTopic = path;
    _lastMqttPubValue = data;
    if(!wait) return;
//    Serial.println("Publish wait");
    for(int i =0;i<200;i++){
        netLoop();
        delay(1);
    } 

}


void netLoop(){
  static unsigned long lastTime = 0;
  unsigned long t = millis() / 1000;

  if(WiFi.status() == WL_CONNECTED ){
    ArduinoOTA.handle();
    mqttLoop();
    if((t - lastTime) >= KEEP_ALIVE_PERIOD){
        lastTime = t;
        char buf[10];
        ltoa(t,buf,10);
        mqttPublish("upTime",buf,false);
    }
    server.handleClient();
  }  
}



void netInit(char *wifiSsid, char *wifiPass,char *mqttServer, char *mqttId, char *mqttBase){
  _wifiSsid = strdup(wifiSsid);
  _wifiPass = strdup(wifiPass);
  _mqttServer = strdup(mqttServer);
  _mqttId = strdup(mqttId); 
  _mqttBase = strdup(mqttBase); 
  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event)
  {
    Serial.print("Station connected, IP: ");   Serial.println(WiFi.localIP());
    otaInit();
    mqttClient.begin(_mqttServer,wifiClient);
  });

  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  {
//    Serial.println("Station disconnected");
  });
//  Serial.printf("Connecting to %s ...\n", _wifiSsid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(_wifiSsid, _wifiPass);
  mqttClient.begin(_mqttServer,wifiClient);
  mqttClient.onMessage(_mqttMessage);
  httpInit();
}  
