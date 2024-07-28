// MQTT Header File MQTT.h
#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
extern WiFiClientSecure wifiClientSecure;
extern PubSubClient pubSubClient;

void initMQTT();
void connectToMqtt();
bool configureTime();
void messageReceived(const char *topic, const byte *payload, unsigned int length);
void reconnect();
bool isMqttConnected();
void pushDeviceState();
void publishDeviceHeartbeat();
void handleMQTT();
bool updateNtpTimeWithRetries();
void handleHeartbeat();
#endif
