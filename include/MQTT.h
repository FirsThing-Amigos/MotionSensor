// MQTT Header File MQTT.h
#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

extern unsigned long lastBroadcastTime;
extern const unsigned long broadcastInterval;
extern time_t relayStateChangesTime;
extern PubSubClient pubSubClient;
extern WiFiClientSecure wifiClientSecure;


void initMQTT();
void connectToMqtt();
bool configureTime();
void messageReceived(const char *topic, const byte *payload, unsigned int length);
void reconnect();
bool isMqttConnected();
void pushDeviceState(int heartBeat);
void handleMQTT();
#endif
