// MQTT Header File MQTT.h
#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>

void initMQTT();
void NTPConnect();
bool configureTime();
void messageReceived(char* topic, byte* payload, unsigned int length);
void reconnect();
bool isMqttConnected();
void pushDeviceState(int heartBeat);
void handleMQTT();
#endif
