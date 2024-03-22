// MQTT Header File MQTT.h
#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>

class MQTT {
public:
  static void initialize();
  static void handleMQTT();
  static void sendHeartBeat();
  static void publishSwitchState(int switchNumber, int switchState);
  static void publishDeviceInformation();
  static void publishMessage(const String& message);
  static String getDeviceID();

private:
  static void reconnect();
  static bool isMqttConnected();
  static void NTPConnect();
  static bool configureTime();
  static void messageReceived(char* topic, byte* payload, unsigned int length);

  static unsigned long lastReconnectAttempt;
  static unsigned long lastMillis;
  static unsigned long previousMillis;
  static unsigned long lastHeartbeatTime;
  static const long interval;
  static const long heartbeatInterval;
  static const char* mqttHost;
  static const int mqttPort;
  static const char* thingName;
  static const char* subTopic;
  static const char* pubTopic;
  static const char* deviceMacAddress;
  static const char* chipId;
};

#endif
