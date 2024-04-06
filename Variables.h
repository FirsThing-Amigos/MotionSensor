#ifndef VARIABLES_H
#define VARIABLES_H

#include <Arduino.h>  // Include Arduino core library for data types
#include <ESP8266WiFi.h>

// #define DEBUG
// #define SOCKET
// #define PIR;

// Define global variables here
extern const String deviceMacAddress;
extern const String chipId;
extern String deviceID;
const char otaUrl[] PROGMEM = "";
extern IPAddress serverIP;

extern int ldrPin;
extern int microPin;
extern int relayPin;
#ifdef PIR
extern int pirPin;
extern int pirMotion;
#endif

extern int maxAttempts;
extern int light;
extern int microMotion;
extern int pirMotion;
extern int ldrVal;
extern int pval;
extern int lowLightThreshold;

extern bool relayState;
extern bool shouldRestart;
extern bool isOtaMode;
extern bool hotspotActive;

extern unsigned long lastMotionTime;
extern unsigned long lightOffWaitTime;
extern unsigned long countDownLightOff;
extern unsigned long countDownDayLight;

#endif
