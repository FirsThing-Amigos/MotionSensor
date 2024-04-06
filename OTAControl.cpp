#include <EEPROM.h>
#include "OTAControl.h"
#include "Variables.h"

void setupOTA() {
  Serial.println(F("Initializing OTA..."));
  // Initialize OTA
  ArduinoOTA.setHostname(deviceID.c_str());
  ArduinoOTA.setPassword("admin");

  // Set OTA update callback functions
  ArduinoOTA.onStart([]() {
    Serial.println(F("Start updating..."));
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nEnd"));
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });
  // Start OTA server
  ArduinoOTA.begin();
  Serial.println(F("OTA Initialized"));
}

void handleOTA() {
  // Handle OTA updates
  ArduinoOTA.handle();

  if (ArduinoOTA.getCommand() == U_FLASH) {
    EEPROM.write(64, false);  // Write otaMode to EEPROM
    EEPROM.commit();
  }
}
