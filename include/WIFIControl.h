#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <Arduino.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#include <Arduino.h>

extern const char *MeshID;
extern const char *MeshPassword;
extern unsigned long previousMillis123;
extern const unsigned long resetCounterTime;
extern IPAddress GatwayIP;

String getDeviceMacAddress();
void initWifi();
bool isWifiConnected();
void initHotspot();
void deactivateHotspot();
String readStringFromEEPROM(int start, int end);
bool shouldResetCounterTime();
void saveResetCounter(int value);
void initmeshHotspot();
void handleWiFiDisconnection();

#endif
