#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>

void initializeDevices();
void readSensors();
void readLDRSensor();
void readMicrowaveSensor();
#ifdef PIR
void readPIRSensor();
#endif
void updateDeviceState();
String getDeviceStatus();
#endif