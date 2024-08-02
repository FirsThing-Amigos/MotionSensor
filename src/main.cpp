#include <EEPROM.h>
#ifdef ESP8266
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <WiFiClientSecure.h>
#include "DeviceControl.h"
#include "HTTPRoutes.h"
#include "MQTT.h"
#include "OTAControl.h"
#include "Variables.h"
#include "WIFIControl.h"
#include "UdpMeshControl.h"

#ifdef SOCKET
#ifdef ESP8266
#include <ESP8266WebServer.h>
#elif defined(ESP32)
#include <WebServer.h>
#endif
#include "WebSocketHelper.h"
WebSocketsServer webSocketServer(81);
#endif

WiFiClientSecure wifiClientSecureOTA;

bool disabled = false;
bool shouldRestart = false;
bool isOtaMode = false;
String otaUrl;
uint8_t wifiDisabled;
bool MeshNetwork = false;

#if defined(ESP8266)
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif

unsigned long lastWiFiCheckTime = 0;
constexpr unsigned long WiFiCheckInterval = 60000;
unsigned long lightOffWaitTime = 120;  // lightOffWaitTime is stored in second
int lowLightThreshold = 140;
int heartbeatInterval = 60; // heartbeatInterval is stored in second
unsigned long restartTimerCounter;
unsigned long lastUpdateMillis = 0;
const unsigned long MeshHotspotDeactiveTime = 5 * 60 * 1000;

void initRestartCounter(){
    EEPROM.begin(FS_SIZE);
    restartCounter = (EEPROM.read(79) > 0 && EEPROM.read(79) < 4) ? EEPROM.read(79) : 0;
    restartCounter++;
    saveResetCounter(restartCounter);
    Serial.println("Configmode: " + String(restartCounter));

}

void initConfig() {
    isOtaMode = (EEPROM.read(69) == 1);
    disabled = (EEPROM.read(70) == 1);
    lightOffWaitTime = (EEPROM.read(72) > 0) ? EEPROM.read(72) : lightOffWaitTime;
    lowLightThreshold = (EEPROM.read(74) > 0) ? EEPROM.read(74) : lowLightThreshold;
    heartbeatInterval = (EEPROM.read(65) > 0) ? EEPROM.read(65) : heartbeatInterval;
    sbDeviceId = (EEPROM.read(77) > 0) ? EEPROM.read(77) : sbDeviceId;
    wifiDisabled = EEPROM.read(81);
    MeshNetwork = (EEPROM.read(87) == 1) ? true : false;
    if(MeshNetwork){
        Serial.println("Mesh Netwrok Mode is Active!!!");
    }
    if (wifiDisabled != 0){
        Serial.println("Wi-Fi Disabled Mode Active!!!");

    }
    if (disabled){
        Serial.println("Device Disabled  Active!!!");
    }

    String tempOtaUrl = "";
    char otaUrlBuffer[FS_SIZE];
    for (int i = 0; i < FS_SIZE; ++i) {
        otaUrlBuffer[i] = static_cast<char>(EEPROM.read(100 + i));
        if (otaUrlBuffer[i] == '\0')
            break;
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
    if(hotspotActive || wifiDisabled == 0){
        initHttpServer();
    #ifdef SOCKET
        initWebSocketServer();
    #endif

        if (isOtaMode) {
            setupOTA();
        } else {
            if (wifiDisabled == 0  && !hotspotActive && !node) {
                initMQTT();
            }
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

        if (isWifiConnected()&& !node) {
            handleMQTT();
        }
    }
}

void handleConfigMode(WiFiClientSecure& wifiClientSecureOTA) {
    if (restartCounter == 3) {
        initHotspot();
        saveResetCounter(0);
        MeshNetwork = false;
        
    }
    else{
        if (wifiDisabled == 0){
            initWifi();
            Serial.println(" WifiDisable Mode = 0");
        }

    }
    if (otaUrl.length() == 0) {
        initServers();
    } else {
        performOTAUpdate(wifiClientSecureOTA);
    }
    if (MeshNetwork && !hotspotActive){
        initmeshHotspot();
        initmeshUdpListen();
    }
}




void setup() {
    Serial.begin(115200);
    Serial.println("");
    initRestartCounter();
    initConfig();
    initDevices();
    handleConfigMode(wifiClientSecureOTA);
    unsigned long currentMillis = millis();
    lastUpdateMillis = currentMillis;
}

void loop() {

    restartTimerCounter = millis();
    if (shouldRestart) {
        restartESP();
    }
 
    if(nodeHotspot){
        forwardingIncomingPackets();
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdateMillis >= MeshHotspotDeactiveTime && nodeHotspot) {
        lastUpdateMillis = currentMillis;  // Save the last time checkConnectedStations() was executed
        int numStations = WiFi.softAPgetStationNum();
        if (numStations == 0) {
            WiFi.softAPdisconnect();
            nodeHotspot = false;
            Serial.println("No nodes are connected. Deactivating hotspot.");
        }
    }

    readSensors();

    if (!disabled) {
        updateRelay();
    }
    if (shouldResetCounterTime()){
        saveResetCounter(0);
    }

    if (!isOtaMode) {
        if (hotspotActive) {
            deactivateHotspot();
        }  
    }
    handleServers();
#ifdef SOCKET
    publishSensorStatus();
#endif
    delay(200);
}
