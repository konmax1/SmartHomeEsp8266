/*
  ESP8266 CheckFlashConfig by Markus Sattler

  This sketch tests if the EEPROM settings of the IDE match to the Hardware

*/

#include <DHT_U.h>
#include <DHT.h>
#include <ESP8266mDNS.h>
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
#include <ArduinoOTA.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <MQTT.h>
#include "config.h"
#define DHTTYPE DHT22


const int DHTPin = 5;
DHT dht(DHTPin, DHTTYPE);

char t1[20];
char t2[20];
char t3[20];

#define BUFFER_SIZE 100

void callback(const MQTT::Publish& pub)
{

	Serial.println(pub.topic());
	Serial.println("stream");
	uint8_t gpio;
	String str = pub.topic();
	str.replace("/", "\t");
	Serial.printf("topic is: %s\n", str.c_str());
	sscanf(str.c_str(), "%s%s%s%d", &t1, &t2, &t3, &gpio);
	Serial.printf("Line is %s|%s|%s|%d\n", t1, t2, t3, gpio);
	if (strcmp(t3, "led") == 0) {
		digitalWrite(gpio, pub.payload_string().toInt());
	}
	else
	{
		Serial.println("Not led");
	}
	//digitalWrite(gpio, pub.payload_string().toInt());
}
	


void setup(void) {
	Serial.begin(115200);
	delay(10);
	dht.begin();

	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");
	delay(1000);
	initInternalGeneral();
}



void otaTask() {
	ArduinoOTA.handle();
}

void tempTask() {
	float t = dht.readTemperature();
	float h = dht.readHumidity();
	//!client.publish("/esp/temp", String(t));
   //! client.publish("/esp/hum", String(h));
}

void mqttTask() {
	if (!client.connected()) {
		if (client.connect(clientSensor)) {
			Serial.println("MQTT connected");
		}
		else {
			Serial.println("MQTT not connected");
		}
		initInternalStructure();
		client.set_callback(callback);
		//client.subscribe("/esp/led1");
		//client.publish("/esp/led1", "0");
	}
	if (client.connected()) {
		client.loop();
	}
}




unsigned long otaMillis = 0;
unsigned long tempMillis = 0;
unsigned long mqttMillis = 0;
unsigned long currentMillis = 0;
const long interval = 1000;
void loop() {
	currentMillis = millis();
	int32_t inputFunc = 0;
	if (currentMillis - otaMillis >= 1000) {
		inputFunc = 1;
		otaMillis = currentMillis;
		otaTask();
	}
	if (currentMillis - tempMillis >= 2000) {
		inputFunc = 1;
		tempMillis = currentMillis;
		tempTask();
	}
	if (currentMillis - mqttMillis >= 20) {
		inputFunc = 1;
		mqttMillis = currentMillis;
		mqttTask();
	}
	if (inputFunc == 0) {
		delay(10);
	}
}



