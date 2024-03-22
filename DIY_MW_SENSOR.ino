#include <ESP8266WebServer.h>
#include "Variables.h"
#include "DeviceControl.h"
#include "WIFIControl.h"
#include "HTTPRoutes.h"
#include "MQTT.h"
#include "OTAControl.h"

bool shouldRestart = false;
bool otaMode = false;

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  initializeDevices();
  initializeConnections();
  initializeServers();
}

void loop() {
  if (shouldRestart) {
    restartESP();
  }
  handleServers();
  readSensors();
  updateDeviceState();
  delay(500);
}

void initializeConnections() {
  connectToWifi();
  if (isWifiConnected()) {
    if (!otaMode) {
      MQTT::initialize();
    }
  } else {
    setupHotspot();
  }
}

void initializeServers() {
  if (!otaMode) {
    startHttpServer();
  } else {
    setupOTA();
  }
}

void handleServers() {
  if (!isWifiConnected() && !hotspotActive) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    connectToWifi();
  }

  if (otaMode) {
    handleOTA();

  } else {
    handleHTTPClients(server);

    if (isWifiConnected()) {
      MQTT::handleMQTT();
    }
  }
}
