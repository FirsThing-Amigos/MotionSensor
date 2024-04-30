#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include "Variables.h"
#include "DeviceControl.h"
#include "WIFIControl.h"
#include "HTTPRoutes.h"
#include "MQTT.h"
#include "OTAControl.h"

#ifdef SOCKET
#include <WebSocketsServer.h>
#include "WebSocketHelper.h"
WebSocketsServer webSocketServer(81);
#endif

WiFiClientSecure wifiClientSecureOTA;

bool disabled = false;
bool shouldRestart = false;
bool isOtaMode = false;
String otaUrl;

ESP8266WebServer server(80);

unsigned long lastWiFiCheckTime = 0;
const unsigned long WiFiCheckInterval = 60000;
unsigned long lightOffWaitTime = 120;
int lowLightThreshold = 140;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  initConfig();
  initDevices();
  initNetwork();
  if (otaUrl.length() == 0) {
    initServers();
  } else {
    performOTAUpdate(wifiClientSecureOTA);
  }
}

void loop() {

  if (shouldRestart) {
    restartESP();
  }

  readSensors();

  if (!disabled) {
    updateRelay();
  }

  if (!isOtaMode) {
    if (millis() - lastWiFiCheckTime >= WiFiCheckInterval) {
      lastWiFiCheckTime = millis();
      if (!isWifiConnected() && !hotspotActive) {
#ifdef DEBUG
        Serial.println(F("WiFi disconnected. Attempting to reconnect..."));
#endif
        initNetwork();
      } else if (hotspotActive) {
        deactivateHotspot();
      }
    }
  }

  handleServers();
#ifdef SOCKET
  publishSensorStatus();
#endif
  delay(200);
}

void initConfig() {
  EEPROM.begin(FS_SIZE);
  isOtaMode = (EEPROM.read(64) == 1);
  disabled = (EEPROM.read(65) == 1);
  lightOffWaitTime = (EEPROM.read(66) > 0) ? EEPROM.read(66) : lightOffWaitTime;
  lowLightThreshold = (EEPROM.read(67) > 0) ? EEPROM.read(67) : lowLightThreshold;

  String tempOtaUrl = "";
  char otaUrlBuffer[FS_SIZE];
  for (int i = 0; i < FS_SIZE; ++i) {
    otaUrlBuffer[i] = EEPROM.read(68 + i);
    if (otaUrlBuffer[i] == '\0') break;
  }

  tempOtaUrl = String(otaUrlBuffer);

  if (isValidUrl(tempOtaUrl)) {
    otaUrl = tempOtaUrl;
    Serial.print("otaUrl found: ");
    Serial.println(otaUrl);

  } else {
    Serial.println("Error: Invalid OTA URL read from EEPROM");
    if (tempOtaUrl.length() == 0) {
      writeOtaUrlToEEPROM("");
    }
  }
}

void initServers() {
  initHttpServer();
#ifdef SOCKET
  initWebSocketServer();
#endif

  if (isOtaMode) {
    setupOTA();
  } else {
    if (isWifiConnected()) {
      initMQTT();
    }
  }
}

void handleServers() {

  handleHTTP(server);

#ifdef SOCKET
  handleWebSocket();
#endif

  if (isOtaMode) {
    handleOTA();

  } else {

    if (isWifiConnected()) {
      handleMQTT();
    }
  }
}
