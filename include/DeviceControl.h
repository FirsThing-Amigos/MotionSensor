#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>

String getDeviceID();
void initDevices();
void readSensors();
void readLDRSensor();
void readMicrowaveSensor();
#ifdef PIR
void readPIRSensor();
#endif
void setLightVariable();
void updateRelay();
String getDeviceStatus();
void restartESP();
#endif