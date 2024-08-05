#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>
#include "HLW8012.h"
extern HLW8012 hlw8012;

String getDeviceID();
void initDevices();
void readSensors();
void readLDRSensor();
void readtemperatureHumidity();
void readMicrowaveSensor();
#ifdef PIR
void readPIRSensor();
#endif
void ICACHE_RAM_ATTR hlw8012_cf1_interrupt();
void ICACHE_RAM_ATTR hlw8012_cf_interrupt();
void initEnergyMetering();
void setInterrupts();
void setLightVariable();
void updateRelay();
String getDeviceStatus();
void restartESP();
void readVotalgeCurrentPowerfactor();
bool checkInternetConnectivity();
unsigned int readFromEEPROM(int address);
void saveTOEEPROM(int address,unsigned long value);
#endif
