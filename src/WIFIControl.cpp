#include "WIFIControl.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "Variables.h"

String ssid;
String password;
IPAddress serverIP;

bool hotspotActive = false;
int maxAttempts = 50;
const String deviceMacAddress;
uint8_t restartCounter;

unsigned long hotspotActivationTime = 0;
constexpr unsigned long hotspotDeactivationDelay = 5 * 60 * 1000; // 5 minutes in milliseconds
unsigned long previousMillis123 = 0;
const unsigned long resetCounterTime = 60000;

String getDeviceMacAddress() { 
    String deviceMacAddress = WiFi.macAddress();        
    // Serial.println(deviceMacAddress);
    return deviceMacAddress;
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

bool isWifiConnected() { return WiFi.status() == WL_CONNECTED; }

void initHotspot() {
    Serial.println(F("Initializing hotspot..."));
    const String hotSpotName = "MS-" + String(deviceID.c_str());
    WiFi.mode(WIFI_AP);
    IPAddress ip(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(ip, ip, subnet);
    WiFi.softAP(hotSpotName);
    serverIP = WiFi.softAPIP();
    hotspotActivationTime = millis();
    hotspotActive = true;
    Serial.println("Hotspot Name: " + hotSpotName);
    Serial.print(F("Hotspot IP address: "));
    Serial.println(serverIP);
    disabled = 1;
    restartCounter = 0;
    EEPROM.write(79, restartCounter);
    EEPROM.commit();
}

void deactivateHotspot() {
    if (hotspotActive && ((millis() - hotspotActivationTime) >= hotspotDeactivationDelay)) {
        Serial.println(F("Deactivating hotspot..."));
        WiFi.softAPdisconnect();
        hotspotActive = false;
    }
}


String readStringFromEEPROM(const int start, const int end) {
    String value = "";
    for (int i = start; i < end; ++i) {
        value += static_cast<char>(EEPROM.read(i));
    }
    return value;
}

bool shouldResetCounterTime() {
    if (restartTimerCounter - previousMillis123 >= resetCounterTime) {
        previousMillis123 =restartTimerCounter; 
        return true;
    }
    return false;
}
