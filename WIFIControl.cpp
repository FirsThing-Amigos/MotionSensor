#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "WIFIControl.h"
#include "Variables.h"

String ssid;
String password;
IPAddress serverIP;

bool hotspotActive = false;
int maxAttempts = 50;
const String deviceMacAddress = WiFi.macAddress();

unsigned long hotspotActivationTime = 0;
const unsigned long hotspotDeactivationDelay = 5 * 60 * 1000;  // 5 minutes in milliseconds

void initNetwork() {
  initWifi();
  if (!isWifiConnected()) {
    initHotspot();
  }
}

void initWifi() {
  ssid = readStringFromEEPROM(0, 32);
  password = readStringFromEEPROM(32, 64);

  if (ssid.length() == 0 || password.length() == 0) {
    Serial.println(F("SSID or Password is Missing"));
  } else {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }

  if (isWifiConnected()) {
    serverIP = WiFi.localIP();
    Serial.println(F(""));
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(serverIP);
  }
}

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void initHotspot() {
  Serial.println(F("Initializing hotspot..."));
  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP("ESP8266AP-MoSen");
  serverIP = WiFi.softAPIP();
  hotspotActivationTime = millis();
  hotspotActive = true;
  Serial.println(F("Hotspot Name: ESP8266AP-MoSen"));
  Serial.print(F("Hotspot IP address: "));
  Serial.println(serverIP);
}

void deactivateHotspot() {
  if (hotspotActive && millis() - hotspotActivationTime >= hotspotDeactivationDelay) {
    // Deactivate hotspot if it has been active for the specified duration
    Serial.println(F("Deactivating hotspot..."));
    WiFi.softAPdisconnect();
    hotspotActive = false;
  }
}


String readStringFromEEPROM(int start, int end) {
  String value = "";
  for (int i = start; i < end; ++i) {
    value += char(EEPROM.read(i));
  }
  return value;
}
