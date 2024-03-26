#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>

void initializeDevices();
void readSensors();
void readLDRSensor();
void readMicrowaveSensor();
// void readPIRSensor();  // Uncomment this line if PIR is connected/available
void updateDeviceState();
String getDeviceStatus();
#endif