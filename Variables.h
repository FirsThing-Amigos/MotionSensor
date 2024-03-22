#ifndef VARIABLES_H
#define VARIABLES_H

#include <Arduino.h>  // Include Arduino core library for data types

// Define global variables here
extern const char* defaultSsid;
extern const char* defaultPassword;

extern String ssid;
extern String password;

extern int ldrPin;
extern int microPin;
extern int relayPin;
extern int pirPin;  // Uncomment this line if PIR is connected/available

extern int ldrState;
extern int microMotion;
extern int pirMotion;
extern int motion;
extern int relayState;
extern int pval;
extern int ldrVal;

extern bool shouldRestart;
extern bool otaMode;
extern bool alarm;
extern bool hotspotActive;

extern unsigned long lastMotionTime;
extern unsigned long lastPrintTime;
extern unsigned long waitTime;
extern int maxAttempts;

#endif
