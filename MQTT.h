// MQTT Header File MQTT.h
#ifndef MQTT_h
#define MQTT_h

#include <Arduino.h>

// Initialize static member variables outside the class definition
static bool shouldRestart = false;

class MQTT {
public:
  static void initialize();
  static void connectAWSIoT();
  static void handleMQTT();
  static void publishSwitchState(int switchNumber, int switchState);
  static void publishDeviceInformation();
  static void publishMessage(const String& message);
  static void sendHeartBeat();
  static String getDeviceID();
  static bool isMqttConnected();

private:
  static bool initializeMqttConfig();
  static void reconnect();
  static void NTPConnect();
  static bool configureTime();
  static void messageReceived(char* topic, byte* payload, unsigned int length);

  // Encapsulated variables
  static bool switchState;
  static String mqttHost;
  static int mqttPort;
  static String thingName;
  static String subTopic;
  static String pubTopic;
  static unsigned long lastReconnectAttempt;
  static unsigned long lastMillis;
  static unsigned long previousMillis;
  static const long interval;
  static unsigned long lastHeartbeatTime;
  static const long heartbeatInterval;
};

#endif
