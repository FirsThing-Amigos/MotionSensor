#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>

String getDeviceID();
void initDevices();
void readSensors();
void readLDRSensor();
void readtemperatureHumidity();
void readMicrowaveSensor();

void setLightVariable();
void updateRelay();
String getDeviceStatus();
void restartESP();
bool checkInternetConnectivity();
#endif
