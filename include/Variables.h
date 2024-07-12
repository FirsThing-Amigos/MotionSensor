#ifndef VARIABLES_H
#define VARIABLES_H

#include <Arduino.h>
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif

#define FS_SIZE 256 // Define filesystem size (256 bytes in this example)
#define OTA_SIZE 786432 // Define OTA size (768KB in this example)

#define DEBUG
// #define SOCKET
// #define PIR;

// Define global variables here
extern const String deviceMacAddress;
extern const String chipId;
extern String deviceID;
extern String otaUrl;
extern IPAddress serverIP;
extern const char *thingName;
extern int sbDeviceId;
extern uint8_t restartCounter; 
extern uint8_t wifiDisabled;
extern unsigned long restartTimerCounter;
extern unsigned long heartbeatIntervalTime;

extern int ldrPin;
extern int microPin;
extern int relayPin;
extern int tempHumiPin;
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
extern int temperature;
extern int humidity;

extern bool disabled;
extern bool relayState;
extern bool shouldRestart;
extern bool isOtaMode;
extern bool hotspotActive;

extern unsigned long lastMotionTime;
extern unsigned long lightOffWaitTime;
extern unsigned long countDownLightOff;
extern unsigned long countDownDayLight;
extern int heartbeatInterval;

#endif
