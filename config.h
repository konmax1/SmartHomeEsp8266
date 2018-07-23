#ifndef CONFIG_H
#define CONFIG_H

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <WiFiServerSecure.h>
#include <WiFiServer.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266WiFiType.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MQTT.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <SparkFunBME280.h>

extern IPAddress serverIP;
extern WiFiClient wclient;
extern PubSubClient client;
extern char ssid[20];
extern char password[20];
extern char nameSensor[20];
extern char clientSensor[20];




void initOTA();

class IntStructure{
  private:
  public:

	  std::vector<BME280> vect_bme280;

    IntStructure();

	void initInternalStructure();
	void initInternalGeneral();
	void initGPIO(uint8_t _gpio, uint8_t _mode, uint8_t _val, int _modeIrq);
};

extern IntStructure initparam;

#endif
