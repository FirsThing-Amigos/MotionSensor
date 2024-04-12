#include <ESP8266WebServer.h>
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

bool disabled = false;
bool shouldRestart = false;
bool isOtaMode = false;

ESP8266WebServer server(80);

unsigned long lastWiFiCheckTime = 0;
const unsigned long WiFiCheckInterval = 60000;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  EEPROM.begin(128);
  disabled = (EEPROM.read(65) == 1);
  isOtaMode = (EEPROM.read(64) == 1);
  initDevices();
  initNetwork();
  initServers();
}

void loop() {
  if (disabled) {
    if (digitalRead(relayPin) == LOW) {
      digitalWrite(relayPin, HIGH);
    }
  } else {
    readSensors();
  }

  if (shouldRestart) {
    restartESP();
  }

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

  handleServers();
#ifdef SOCKET
  publishSensorStatus();
#endif
  delay(200);
}

void initServers() {
  if (isOtaMode) {
    setupOTA();
  } else {
    initHttpServer();
#ifdef SOCKET
    initWebSocketServer();
#endif
    if (isWifiConnected() && !disabled) {
      initMQTT();
    }
  }
}

void handleServers() {

  if (isOtaMode) {
    handleOTA();

  } else {

    handleHTTP(server);

    if (isWifiConnected() && !disabled) {
      handleMQTT();
    }
  }

#ifdef SOCKET
  handleWebSocket();
#endif
}
