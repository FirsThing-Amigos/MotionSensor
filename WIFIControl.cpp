#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "WIFIControl.h"
#include "Variables.h"

const char* defaultSsid = "yugg";
const char* defaultPassword = "98103080";

String ssid;
String password;

bool hotspotActive = false;
int maxAttempts = 100;

void connectToWifi() {
  EEPROM.begin(128);
  ssid = readStringFromEEPROM(0, 32);
  password = readStringFromEEPROM(32, 64);

  if (ssid.length() == 0 || password.length() == 0) {
    Serial.println("SSID or Password is Missing");
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

  if (!isWifiConnected()) {
    Serial.print("Fallback to default WiFi: ");
    Serial.println(defaultSsid);
    WiFi.begin(defaultSsid, defaultPassword);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }

  if (isWifiConnected()) {
    Serial.println("");
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi.");
  }
}

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void setupHotspot() {
  Serial.println("Starting hotspot...");
  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP("ESP8266AP-MoSen");
  hotspotActive = true;
  Serial.println("Hotspot Name: ESP8266AP-MoSen");
  Serial.println("Hotspot IP address: " + WiFi.softAPIP().toString());
}


String readStringFromEEPROM(int start, int end) {
  String value = "";
  for (int i = start; i < end; ++i) {
    value += char(EEPROM.read(i));
  }
  return value;
}

