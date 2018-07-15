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

void IntStructure::addLed(uint8_t _gpio, uint8_t _val) {
  LED led;
  led.gpio = _gpio;
  sprintf(led.name, "%s/%s/led/%d", nameSensor, clientSensor, _gpio);

 client.subscribe(led.name);
 client.publish(String(led.name), String(_val));

  Serial.printf("Add led gpio=%d val=%d name =%s\n", _gpio, _val, led.name);
  leds.insert(leds.end(), led);
  pinMode(_gpio, OUTPUT);
  digitalWrite(_gpio, _val);
}

void IntStructure::setLedVal(uint8_t _id, uint8_t _val) {
  if ( _id >= leds.size())
    return;
}


void IntStructure::initGPIO(uint8_t _gpio, uint8_t _mode, int _modeIrq) {
  char ch[60];
  //Serial.printf("Add gpio input = %d mode = %d modeIrq = %d\n", _gpio, _mode, _modeIrq);
  sprintf(&ch[0], "%s/%s/button/%d", nameSensor, clientSensor, _gpio);
  Serial.printf("Add gpio input = %s\n", &ch[0]);
  client.publish(ch, String(digitalRead(_gpio)));
  pinMode(_gpio, _mode);
  attachInterruptMy(_gpio, gpioIRQ, _modeIrq);
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


void initInternal() {
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
    Serial.println(F("Failed to parce JSON"));
    return;
  }
  JsonArray& leds = root["leds"];
  JsonArray& buttons = root["buttons"];
  Serial.printf("leds = %d\n", leds.size());
  Serial.printf("buttons = %d\n", buttons.size());
  strcpy(clientSensor, root["clientSensor"]);

  initOTA();

  for (volatile int i = 0; i < leds.size(); i++) {
    initparam.addLed(leds[i]["gpio"], leds[i]["initval"]);
  }
  for (volatile int i = 0; i < buttons.size(); i++) {
    uint8_t pinmode;
    uint8_t gpio;
    int irqmode;
    strJson = buttons[i]["pull"];
    if (strcmp(strJson, "INPUT_PULLUP") == 0) {
      pinmode = INPUT_PULLUP;
    } else if (strcmp(strJson, "INPUT") == 0) {
      pinmode = INPUT;
    }
    strJson = buttons[i]["irqtype"];
    if (strcmp(strJson, "CHANGE") == 0)    {
      irqmode = CHANGE;
    } else if (strcmp(strJson, "RISING") == 0)    {
      irqmode = RISING;
    } else if (strcmp(strJson, "FALLING") == 0)    {
      irqmode = FALLING;
    }
    gpio =  buttons[i]["gpio"];
    initparam.initGPIO(gpio, pinmode, irqmode);
  }
  SPIFFS.end();
}












