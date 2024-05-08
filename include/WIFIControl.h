#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <Arduino.h>

void begin();
void initNetwork();
void initWifi();
bool isWifiConnected();
void initHotspot();
void deactivateHotspot();
String readStringFromEEPROM(int start, int end);

#endif
