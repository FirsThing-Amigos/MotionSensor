#include <EEPROM.h>
#include "OTAControl.h"
#include "Variables.h"

void setupOTA() {
  Serial.println("Initializing OTA...");
  // Initialize OTA
  ArduinoOTA.setHostname("DIY_MW_SENSOR");
  ArduinoOTA.setPassword("admin");

  // Set OTA update callback functions
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  // Start OTA server
  ArduinoOTA.begin();
  Serial.println("OTA Initialized");
}

void handleOTA() {
  // Handle OTA updates
  ArduinoOTA.handle();

  // Check if OTA update is in progress
  if (ArduinoOTA.getCommand() == U_FLASH) {
    // OTA update completed successfully
    Serial.println("OTA update completed successfully.");
    // Set otaMode to false after OTA update completion
    EEPROM.write(64, false);  // Write otaMode to EEPROM
    EEPROM.commit();
  }
}