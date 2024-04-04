// MQTT Header File MQTT.h
#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>

void initialize();
void handleMQTT();
String getDeviceID();
void reconnect();
bool isMqttConnected();

#endif
