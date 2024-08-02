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

unsigned long energyConsumed = 0;

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
unsigned int wattSec = 0;
unsigned long lastUpdate = 0;
unsigned long previousupdate = 0;
const long interval = 1000;
const long energyConsumedTime = 60000;

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
    if (wifiDisabled != 0){
        Serial.println("Wi-Fi Disabled Mode Active!!!");
    }

    energyConsumed = readFromEEPROM(83);
    Serial.println(energyConsumed);
    if (energyConsumed <= 0){
        saveTOEEPROM(83,0);
    }


    String tempOtaUrl = "";
    char otaUrlBuffer[FS_SIZE];
    for (int i = 0; i < FS_SIZE; ++i) {
        otaUrlBuffer[i] = static_cast<char>(EEPROM.read(88 + i));
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
            if (wifiDisabled == 0 && isWifiConnected()) {
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

        if (isWifiConnected()) {
            handleMQTT();
        }
    }
}

void handleConfigMode() {
    if (restartCounter == 3) {
        initHotspot();
        saveResetCounter(0);
        Serial.printf("hotspot mode end");
    }
    else{
        if (wifiDisabled == 0){
            initWifi();
            Serial.println(" WifiDisable Mode = 0");
        }

    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("");
    initRestartCounter();
    initConfig();
    initDevices();
    handleConfigMode();
    initEnergyMetering();
    if (otaUrl.length() == 0) {
        initServers();
    } else {
        performOTAUpdate(wifiClientSecureOTA);
    }
}

void loop() {

    restartTimerCounter = millis();
    if (shouldRestart) {
        restartESP();
    }

    readSensors();

    if (!disabled) {
        updateRelay();
    }

    unsigned long currentMillis = millis();
    if (currentMillis - lastUpdate >= interval) {
        lastUpdate = currentMillis;
        wattSec = wattSec + hlw8012.getActivePower();
        Serial.print("wattSec: ");
        Serial.println(wattSec);

    }
    unsigned long CURRENTMilliss = millis();
    if (CURRENTMilliss - previousupdate >= energyConsumedTime && wattSec > 0 ) {
        previousupdate = currentMillis;
        energyConsumed = energyConsumed + wattSec;
        Serial.print("energyConsumed in time loop 2 min");
        Serial.println(energyConsumed);
        wattSec = 0;
        saveTOEEPROM(83,energyConsumed);
        Serial.print("energyConsumed: ");
        Serial.println(energyConsumed);
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
