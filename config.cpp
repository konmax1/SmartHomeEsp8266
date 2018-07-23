#include "config.h"
#include "FunctionalInterrupt.h"

WiFiClient wclient;
IPAddress serverIP(192, 168, 0, 35);
PubSubClient client(wclient, serverIP);

DynamicJsonBuffer jsonBuffer;
IntStructure initparam;
char ssid[20] = "KONMAX_WiFi";
char password[20] = "srtoli9k";

char nameSensor[20] = "/esp";
char clientSensor[20] = "genericMod";

typedef void (*voidFuncPtr)(void);
extern "C" void ICACHE_RAM_ATTR __attachInterruptArg(uint8_t pin, voidFuncPtr userFunc, void*fp , int mode);


void interruptFunctionalMy(void* arg)
{
  char ch[60];
  ArgStructure* localArg = (ArgStructure*)arg;
  //localArg->functionInfo->reqFunction();
  uint8_t pin = localArg->interruptInfo->pin;
  uint32_t value = localArg->interruptInfo->value;
  
  sprintf(&ch[0], "%s/%s/button/%d", nameSensor, clientSensor, pin);
 client.publish(String(ch), String(value));
  Serial.printf("%s is %d\n", ch,value);
}

void attachInterruptMy(uint8_t pin, std::function<void(void)> intRoutine, int mode)
{
  // use the local interrupt routine which takes the ArgStructure as argument

  InterruptInfo* ii = new InterruptInfo;

  FunctionInfo* fi = new FunctionInfo;
  fi->reqFunction = intRoutine;

  ArgStructure* as = new ArgStructure;
  as->interruptInfo = ii;
  as->functionInfo = fi;

  __attachInterruptArg (pin, (voidFuncPtr)interruptFunctionalMy, as, mode);
}

IntStructure::IntStructure() {

}

void gpioIRQ() {// Not called, but simple

}

void IntStructure::initGPIO(uint8_t _gpio, uint8_t _mode, uint8_t _val, int _modeIrq) {
  char ch[60];
  pinMode(_gpio, _mode);
  if ((_mode == OUTPUT_OPEN_DRAIN) || (_mode == OUTPUT)) {
	  sprintf(ch, "%s/%s/pinOut/%d", nameSensor, clientSensor, _gpio);
	  Serial.printf("Add led gpio=%d val=%d name =%s\n", _gpio, _val, ch);
	  client.subscribe(ch);
	  //client.publish(String(ch), String(_val));
	  //digitalWrite(_gpio, _val);
  }
  if ((_mode == INPUT) || (_mode == INPUT_PULLUP) || (_mode == INPUT_PULLDOWN_16)) {
	  sprintf(&ch[0], "%s/%s/pinInput/%d", nameSensor, clientSensor, _gpio);
	  Serial.printf("Add gpio input = %s\n", &ch[0]);
	  client.publish(ch, String(digitalRead(_gpio)));
	  if (_modeIrq != 0) {
		  attachInterruptMy(_gpio, gpioIRQ, _modeIrq);
	  }
  }
  client.loop();
}


void initOTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    ESP.reset();
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setHostname(clientSensor);
  //ArduinoOTA.setPassword(clientSensor);
  ArduinoOTA.begin();
}


void IntStructure::initInternalGeneral() {

	Serial.printf("SPIFFS return %d\n", SPIFFS.begin());
	File configFile = SPIFFS.open("/configGeneral.ini", "r");
	if (!configFile)	{
		Serial.println("Failed to open configGeneral file");
		initOTA();
		return;
	}
	JsonObject &root = jsonBuffer.parseObject(configFile); 
	if (!root.success()) {
		Serial.println(F("Failed to parce JSON_Gen"));
		initOTA();
		return;
	}
	strcpy(clientSensor, root["clientSensor"]);

	Serial.println(F("Set OTA"));
	initOTA();




	SPIFFS.end();
}

void IntStructure::initInternalStructure() {
  const char *strJson;
  Serial.printf("SPIFFS return %d\n", SPIFFS.begin());
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  /// Serial.printf("total %d, used %d, block %d,page %d, maxOpen %d, maxPath %d\n", fs_info.totalBytes, fs_info.usedBytes, fs_info.blockSize, fs_info.pageSize, fs_info.maxOpenFiles, fs_info.maxPathLength);
  File configFile = SPIFFS.open("/config.ini", "r");
  if (!configFile)
  {
    Serial.println("Failed to open config file");
    return ;
  }
  JsonObject &root = jsonBuffer.parseObject(configFile);
  if (!root.success()) {
    Serial.println(F("Failed to parce JSON_CNFG"));
    return;
  }
  uint8_t pinmode = INPUT;
  uint8_t gpio = 0;
  int irqmode = 0;
  JsonArray& gpios = root["GPIOS"];
  for (volatile int i = 0; i < gpios.size(); i++) {
	  pinmode = INPUT;
	  irqmode = 0;

	  strJson = gpios[i]["type"];
	  if (strcmp(strJson, "INPUT_PULLUP") == 0) {
		  pinmode = INPUT_PULLUP;
	  }
	  else if (strcmp(strJson, "INPUT") == 0) {
		  pinmode = INPUT;
	  }
	  else if (strcmp(strJson, "OUTPUT_OPEN_DRAIN") == 0) {
		  pinmode = OUTPUT_OPEN_DRAIN;
	  }
	  else if (strcmp(strJson, "OUTPUT") == 0) {
		  pinmode = OUTPUT;
	  }


	  strJson = gpios[i]["irqtype"];
	  if (strcmp(strJson, "CHANGE") == 0) {
		  irqmode = CHANGE;
	  }
	  else if (strcmp(strJson, "RISING") == 0) {
		  irqmode = RISING;
	  }
	  else if (strcmp(strJson, "FALLING") == 0) {
		  irqmode = FALLING;
	  }
	  else if (strcmp(strJson, "NONE") == 0) {
		  irqmode = 0;
	  }
	  gpio = gpios[i]["gpio"];
	  initparam.initGPIO(gpio, pinmode, gpios[i]["initval"], irqmode);
  }
  
  
  JsonArray& sensors = root["SENSORS"];
  for (volatile int i = 0; i < sensors.size(); i++) {
	  strJson = sensors[i]["type"];
	  uint8_t addr = sensors[i]["adrress"];
	  if (strcmp(strJson, "BME280") == 0) {
		  Wire.begin();		  
		  int32_t stat = 0;
		  for (volatile int i = 0; i < vect_bme280.size(); i++) {
			  if (addr == vect_bme280[i].settings.I2CAddress) {
				  stat = 1;
				  break;
			  }
		  }
		  if (stat == 0) {
			  BME280 bme;
			  bme.setI2CAddress(addr);
			  if (bme.begin() == false) {
				  bme.setTempOverSample(2);
				  bme.setPressureOverSample(8);
				  bme.setHumidityOverSample(4);
				  bme.setFilter(3);
				  bme.setStandbyTime(0);
				  bme.setMode(3);
				  client.publish(String("/notBME"), String(0));
				  Serial.printf("BME not start addr %x", addr);
			  }
			  vect_bme280.push_back(bme);
		  }
	  }
  }
  SPIFFS.end();
}












