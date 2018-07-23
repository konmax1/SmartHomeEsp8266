/*
  ESP8266 CheckFlashConfig by Markus Sattler

  This sketch tests if the EEPROM settings of the IDE match to the Hardware

*/

#include <SparkFunBME280.h>
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

char t1[20];
char t2[20];
char t3[20];

#define BUFFER_SIZE 100


void callbackMQTT(const MQTT::Publish& pub)
{

	//Serial.println(pub.topic());
	uint8_t gpio;
	String str = pub.topic();
	str.replace("/", "\t");
	//Serial.printf("topic is: %s\n", str.c_str());
	sscanf(str.c_str(), "%s%s%s%d", &t1, &t2, &t3, &gpio);
	//Serial.printf("Line is %s|%s|%s|%d\n", t1, t2, t3, gpio);
	if (strcmp(t3, "pinOut") == 0) {
		digitalWrite(gpio, (1 - pub.payload_string().toInt()));
	}
	else
	{
		Serial.println("Not led");
	}
	//digitalWrite(gpio, pub.payload_string().toInt());
}
	


void setup(void) {
	Serial.begin(256000);
	delay(10);

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
	initparam.initInternalGeneral();
}



void otaTask() {
	ArduinoOTA.handle();
}

void bme280Task() {
	char tmp[60];
	for (volatile int i = 0; i < initparam.vect_bme280.size(); i++) {
		sprintf(tmp, "%s/%s/bme280/%d/temp", nameSensor, clientSensor, i);
		client.publish(tmp, String(initparam.vect_bme280[i].readTempC()));
		sprintf(tmp, "%s/%s/bme280/%d/hum", nameSensor, clientSensor, i);
		client.publish(tmp, String(initparam.vect_bme280[i].readFloatHumidity()));
		sprintf(tmp, "%s/%s/bme280/%d/press", nameSensor, clientSensor, i);
		client.publish(tmp, String(initparam.vect_bme280[i].readFloatPressure() * 0.00750063755419211));
	}
	//!client.publish("/esp/temp", String(t));
   //! client.publish("/esp/hum", String(h));
}

void mqttTask() {
	if (!client.connected()) {
		if (client.connect(clientSensor)) {
			Serial.println("MQTT connected");
			client.set_callback(callbackMQTT);
			initparam.initInternalStructure();
		}
		else {
			Serial.println("MQTT not connected");
		}
	}
	if (client.connected()) {
		client.loop();
	}
}




unsigned long otaMillis = 0;
unsigned long bme280Millis = 0;
unsigned long mqttMillis = 0;
unsigned long currentMillis = 0;
const long interval = 1000;
void loop() {
	int32_t mqtt_time = 20;
	currentMillis = millis();
	int32_t inputFunc = 0;
	if (currentMillis - otaMillis >= 1000) {
		inputFunc = 1;
		otaMillis = currentMillis;
		otaTask();
	}
	if (currentMillis - bme280Millis >= 2050) {
		inputFunc = 1;
		bme280Millis = currentMillis;
		bme280Task();
	}
	if (currentMillis - mqttMillis >= mqtt_time) {
		inputFunc = 1;
		mqttMillis = currentMillis;
		mqttTask();
		if (client.connected()) {
			mqtt_time = 20;
		}
		else {
			mqtt_time = 1000;
		}
	}
	if (inputFunc == 0) {
		delay(10);
	}
}



