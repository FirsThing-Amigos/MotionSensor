// MQTT Header File MQTT.h
#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>

extern const int initialReconnectDelay ; 
extern int currentReconnectDelay;
extern int reconnectAttemptCount;
extern const int maxReconnectAttempts;
extern String subTopic;

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
void publishUdpDataToMqtt(const char *message);
#endif
