#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include "DeviceControl.h"
#include "HTTPRoutes.h"
#include "MQTT.h"
#include "OTAControl.h"
#include "Variables.h"
#include "WIFIControl.h"
#include "UdpControl.h"

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
constexpr unsigned long WiFiCheckInterval = 60000;
unsigned long lightOffWaitTime = 120;
int lowLightThreshold = 100;
int heartbeatInterval = 60;
uint8_t wifiDisabled;
unsigned long restartTimerCounter;


void initConfig() {
    EEPROM.begin(FS_SIZE);
    configMode = (EEPROM.read(79) > 0 && EEPROM.read(79) < 4) ? EEPROM.read(79) : 0;
    configMode++;
    EEPROM.write(79, configMode);
    EEPROM.commit();
    Serial.println("Configmode: " + String(configMode));
    isOtaMode = (EEPROM.read(69) == 1);
    disabled = (EEPROM.read(70) == 1);
    lightOffWaitTime = (EEPROM.read(72) > 0) ? EEPROM.read(72) : lightOffWaitTime;
    lowLightThreshold = (EEPROM.read(74) > 0) ? EEPROM.read(74) : lowLightThreshold;
    heartbeatInterval = (EEPROM.read(65) > 0) ? EEPROM.read(65) : heartbeatInterval;
    sbDeviceId = (EEPROM.read(77) > 0) ? EEPROM.read(77) : sbDeviceId;
    wifiDisabled = EEPROM.read(81); 

    

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
            if (isMqttConnected()){
                ipBroadcastByUdp();
                processIncomingUdp();

            }
        }
    }
}

void handleConfigMode() {
    if (configMode == 3) {
        initHotspot();
        configMode = 0;
        EEPROM.write(79, configMode);
        EEPROM.commit();
        Serial.printf("hotspot mode end");
    }
    else{
        if (wifiDisabled == 0){
            initNetwork();
            Serial.println(" WifiDisable Mode = 0");
            openUdp();
        }

    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("");
    initConfig();
    initDevices();
    handleConfigMode();
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
    if (shouldResetCounterTime()){
        configMode = 0;
        EEPROM.write(79, configMode);
        EEPROM.commit();
    }

    if (!isOtaMode) {
        if (millis() - lastWiFiCheckTime >= WiFiCheckInterval) {
            lastWiFiCheckTime = millis();
            if (!isWifiConnected() && !hotspotActive && wifiDisabled == 0) {
#ifdef DEBUG
                Serial.println(F("WiFi disconnected. Attempting to reconnect..."));
#endif
                // initNetwork();
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