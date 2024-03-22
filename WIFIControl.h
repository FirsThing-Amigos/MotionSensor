#ifndef WIFI_CONTROL_H
#define WIFI_CONTROL_H

#include <Arduino.h>

void connectToWifi();
bool isWifiConnected();
void setupHotspot();
String readStringFromEEPROM(int start, int end);

#endif
