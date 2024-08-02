#include "WIFIControl.h"
#include <EEPROM.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <AsyncTCP.h>
    #include <WebServer.h>
#endif
#include "UdpMeshControl.h"
#include "Variables.h"
#include "HTTPRoutes.h"
#include "OTAControl.h"
#include "MQTT.h"

String ssid;
String password;
IPAddress serverIP;
const char *MeshID = "nodeHotspot";
const char *MeshPassword = "1234567890";

bool node = false;

bool hotspotActive = false;
int maxAttempts = 50;
const String deviceMacAddress;
uint8_t restartCounter;

unsigned long hotspotActivationTime = 0;
constexpr unsigned long hotspotDeactivationDelay = 5 * 60 * 1000; // 5 minutes in milliseconds
unsigned long previousMillis123 = 0;
const unsigned long resetCounterTime = 60000;
const int wifiDisconnectDuration = 1800000; // 30 minutes in milliseconds

IPAddress GatwayIP;
bool nodeHotspot = false;

String getDeviceMacAddress() { 
    String deviceMacAddress = WiFi.macAddress();        
    // Serial.println(deviceMacAddress);
    return deviceMacAddress;
}

void initWifi() {
    ssid = readStringFromEEPROM(0, 32);
    password = readStringFromEEPROM(32, 64);
    bool connected = false;

    if (ssid.length() > 0 || password.length() > 0) {
        Serial.print("Connecting to WiFi: ");
        Serial.println(ssid);
        WiFi.begin(ssid.c_str(), password.c_str());
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
        }
        if (MeshNetwork && !connected) {
            WiFi.begin(MeshID, MeshPassword);
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");

            }
            node =true;
        }
    }else {
        Serial.println(F("SSID or Password is Missing"));
    }
     if (isWifiConnected()) {
        serverIP = WiFi.localIP();
        Serial.println(F(""));
        Serial.print("Connected to WiFi. IP address: ");
        Serial.println(serverIP);
    }

}


bool isWifiConnected() {
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0)) {
        return false;
    }
    return true;
}

void initmeshHotspot() {
    IPAddress GatwayIP = WiFi.gatewayIP();
    Serial.print("Gateway IP address: ");
    Serial.println(GatwayIP);

    IPAddress localAPIP;
    IPAddress gateway;
    IPAddress subnet(255, 255, 255, 0);

    if (GatwayIP.toString() == "192.168.4.1") {
        localAPIP = IPAddress(192, 168, 1, 1);  // Desired static IP address
        gateway = IPAddress(192, 168, 1, 1);  // Gateway address (same as localIP for AP)
        Serial.println("Host IP is set to: '192.168.1.1'");
    } else {
        localAPIP = IPAddress(192, 168, 4, 1);  // Desired static IP address
        gateway = IPAddress(192, 168, 4, 1);  // Gateway address (same as localIP for AP)
        Serial.println("Host IP is set to: '192.168.4.1'");
    }
    WiFi.softAPConfig(localAPIP, gateway, subnet);
    // WiFi.softAPsetMaxConnections(5);
    WiFi.softAP(MeshID, MeshPassword);
    Serial.print("Mesh Hosypot name is : ");
    Serial.println(MeshID);
    nodeHotspot = true;
}

void initHotspot() {

    Serial.println(F("Initializing hotspot..."));
    const String hotSpotName = "MS-" + String(deviceID.c_str());
    IPAddress ip(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(ip, ip, subnet);
    WiFi.softAP(hotSpotName);
    serverIP = WiFi.softAPIP();
    hotspotActivationTime = millis();
    hotspotActive = true;
    Serial.println("Hotspot Name: " + hotSpotName);
    disabled = 1;

    Serial.print("Hotspot IP address: ");
    Serial.println(WiFi.softAPIP()); 
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

void saveResetCounter(int value){
    EEPROM.write(79, value);
    EEPROM.commit();
}

void handleWiFiDisconnection() {
    WiFi.disconnect(true);
    delay(wifiDisconnectDuration);
}