// MQTT File MQTT.cpp
// #include <ESP8266WiFi.h>
// #include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <TimeLib.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <EEPROM.h>
#include "Variables.h"
#include "DeviceControl.h"
#include "HTTPRoutes.h"
#include "MQTT.h"

WiFiClientSecure net;
PubSubClient client(net);


const char* mqttHost = "a38blua3zelira-ats.iot.ap-south-1.amazonaws.com";
const int mqttPort = 8883;
const char* thingName = "athea-motion2";

unsigned long lastReconnectAttempt = 0;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
unsigned long heartbeatInterval = 600000;  // 10 minute
unsigned long lastHeartbeatTime = 0;

static const char cacert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// Copy contents from XXXXXXXX-certificate.pem.crt here ▼
static const char client_cert[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIURRZegj1skOoVw2fofNJjNkFmAqEwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIzMTIzMDE3MjIy
OVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALCCBPsxYPAuq0p/Yd3O
auL6HMPKyGnQOxTEKVjHTKFfANE9lZgf8W2J1G7tdoYrEfJfAeKNHEDFeB+NYZj5
GhLwE4JfFDnZed7+K+hqp3O4Pj8LRtuF1tzGtuzFNtjiTUFIlX4YeinuZYz+Z73c
Uv4o2bEW6Ku63BxQfSQ/tGit/YsZzee78nR0J9MKZrTOJ37SwOf8zvBudO+xe3or
8SwfXcxaDKaDhShS7CnDHvm5HcutDRxzGdPc2uPfUkvvgDo5hNuLEzU4KWYMWwO9
mSntLHorIeBnJEIdsB790A4J6SaJq6DR7cqKcbEXqYtcbSkDwo79Vugn6KpFo0+i
SoECAwEAAaNgMF4wHwYDVR0jBBgwFoAUSs+TwmP/DoInQrfT0sQzfWYYR4AwHQYD
VR0OBBYEFMjXv5eRE4JmznFPrr5Xg0YuCHz1MAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQB8ZrW4C383MDDjrQIdx2HbFe+g
lgXLHYVfzWqeu+6funvk724oc56Nd71Q6Jfr9G3LAVyNM3N3T6w9ySTTMMfvJzs2
MlUytmTLIdblewTEMwvR++t6O1dC/5JFsceRV3adh0ZB1YdEExUSQncVOVbMIzd7
IiL5K28lytfO+MdOKNHiwUgLRQy8hsKoyRLN6BsJl+O/vtXHwGwpXpqItp2fEjNH
JaQnhy1rg1bkhKlNagkfBUdJpSvTkrAfcFKKbyoR/qhQ0n/WrOAXgXurCRN/A0Qw
tD4KnGMcWO1tWpNV6XNH3C1HFkMjQIuoQdxP7+e8ERorqE0qVCd2goisRq9a
-----END CERTIFICATE-----
)KEY";

// Copy contents from  XXXXXXXX-private.pem.key here ▼
static const char privkey[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEAsIIE+zFg8C6rSn9h3c5q4vocw8rIadA7FMQpWMdMoV8A0T2V
mB/xbYnUbu12hisR8l8B4o0cQMV4H41hmPkaEvATgl8UOdl53v4r6Gqnc7g+PwtG
24XW3Ma27MU22OJNQUiVfhh6Ke5ljP5nvdxS/ijZsRboq7rcHFB9JD+0aK39ixnN
57vydHQn0wpmtM4nftLA5/zO8G5077F7eivxLB9dzFoMpoOFKFLsKcMe+bkdy60N
HHMZ09za499SS++AOjmE24sTNTgpZgxbA72ZKe0seish4GckQh2wHv3QDgnpJomr
oNHtyopxsRepi1xtKQPCjv1W6CfoqkWjT6JKgQIDAQABAoIBAQCGmzEnS1ns8PpK
PuiombFj1W+4VN4P6/ASfyI1BRBIhEYATCHYOIwKiDEMQ2W/Hft6xb+PQSPJtY5N
KanDWjzrMlD+fHnVQCezykw725JGKry1oWjxIplgFt8Lo76XGcXmwm7XEd2gOqER
LDZ7URmtoAOcDKd1x3Py1+MHkNmJddoJ9wRR8SifjGhzcbCV5RrvR1Y/w7q+fwta
NWPIcIjE07PlgB7PJ8S9KcLrrx+KnWV/LWj5aZ4Vf/A1JIUCWjZFJlRVEt7N/Qop
Hrl2Zo19usYRQoqKdfeD0VDtBKp3LtlE38VznJvCztp+w2+92d9uq8MVOGRHt3V4
xDEPLIjdAoGBAODRgdjAgzghstdiwoyLoAQJxggEu5tpxMpytY3y1fsVaMbwOPpZ
4Wrpmep9xU5YC/17IOVw9DmV5bvcsPa78DkJV9B4E4Gh+h6ygC3CK4YA/t4zs1zO
fnIfo1PHpHdom1BJDBmkQip+AAH8PE61NfFYTJdPr5uUP9pKoIHGK1ITAoGBAMj9
LhoO+yq6+bLoV9qIIuGB74Xz2G9wTO9w+qW42ukol5336BISk2cJYhab4c+eY18u
VP0V0sTk7+U1c2yJt17BkGIFtH39INWj4nYouRSg5Tr4Po5v5IlqKIFCjM+pAXO3
NRR0v1c0P5W1rT9tKd+BQ8syYMX55XARN2UZ9SObAoGBALd9Jz4yOabpkh7K40B0
gQBqva8zta8tj0kwgE+/n7fTDHY6ADcMfreUu2OwjQXZRMf446uAGkSZvCws/l4S
nAjhQEPMjRcjjZHaXFV8gatYSqwuhDi97GPWwKYTbn9q6ECJrg+LidlGh1kXdl1C
9yjoyXJBvnD7eXX/rKreg+LzAoGBAL6s4nW6TLHnHEiMf/xENsM1n+S2x1hBc9uc
lU1vJ88WwXAN5k5u7QRNNI86Z2muW5vKuro3X/OHNcd/g/cuV5Y/kBhOTUNRRzsm
9Qsf6yYU1iOxqrc6k9eBrNekKS5Aqa372xGDCJtUaBZBexC3IYh2e7hkHMb1IqPE
0YbKr3ONAoGAXplE2/yDwNv7ic32FKbdBUVV+gqhuMRNKjiodw4RGdlgTfythdAE
xhVk//J3cp3ZLmGmCGuyIENGhIa4LgC8nTHUbHfemftKXEQEjytknErCmKK5ZRMd
Xy6GnA70LtjFlpc+X8SbZIKDANe6+oCNeEK0nf8jnSNoxUOueF2OyPY=
-----END RSA PRIVATE KEY-----
)KEY";

BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

int8_t TIME_ZONE = +5.5;
#define TIME_ZONE +5.5
unsigned long mqttConnectStartTime;
unsigned long mqttConnectTimeout = 1 * 60 * 1000;

void initMQTT() {
  const String pubTopic = ("sensor/" + String(getDeviceID()) + "/state/pub");
#ifdef DEBUG
  Serial.print(F("mqttServer ThingName: "));
  Serial.println(thingName);
  Serial.print(F("mqttServer Pub Topic: "));
  Serial.println(pubTopic);
  Serial.print(F("mqttServer mqttHost: "));
  Serial.println(mqttHost);
  Serial.print(F("mqttServer mqttPort: "));
  Serial.println(mqttPort);
#endif

  NTPConnect();
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
  client.setServer(mqttHost, mqttPort);
  // client.setCallback(messageReceived);
#ifdef DEBUG
  Serial.println(F("Connecting to AWS IOT"));
#endif
  mqttConnectStartTime = millis();

  while (!client.connect(thingName)) {
    Serial.print(".");
    if (millis() - mqttConnectStartTime >= mqttConnectTimeout) {
      // Serial.println(F("AWS connection timed out"));
#ifdef DEBUG
      Serial.print(F("Failed to connect to AWS IoT, rc="));
      Serial.println(client.state());
#endif

      break;
    }
    delay(1000);
  }

  if (client.connected()) {
    // const String subTopic = ("sensor/" + String(getDeviceID()) + "/state/sub");
    // client.subscribe(subTopic.c_str());
    Serial.println(F("AWS IoT Connected!"));
  }
}

void NTPConnect() {
  Serial.println("Setting time using SNTP");
  // setTime(12, 34, 56, 9, 9, 2023);
  time_t nowish = 1510592825;

  if (configureTime()) {
    time_t currentTime = now();
    Serial.println("done!");
    struct tm timeinfo;
    gmtime_r(&currentTime, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
  } else {
    Serial.println("Time configuration failed");
    // Handle the error condition here, such as retrying or reporting an error
  }
}

bool configureTime() {
  configTime(TIME_ZONE * 3600, 0, "time.nist.gov", "pool.ntp.org");

  // Wait for time configuration for a limited time
  unsigned long configTimeout = millis() + 5000;  // Wait for up to 15 seconds
  while (timeStatus() != timeSet && millis() < configTimeout) {
    delay(1000);
  }

  return timeStatus() == timeSet;
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");

  // Print payload as string
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Convert payload to string
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Extract information from the message
  String command = message.substring(0, message.indexOf(' '));
  String valueStr = message.substring(message.indexOf(' ') + 1);

  if (command.equals("otaUrl")) {
    updateOtaUrl(valueStr.c_str());
    performOTAUpdate();

  } else if (command.equals("disabled")) {
    bool value = valueStr.equalsIgnoreCase("true");
    if (value) {
      disabled = valueStr.toInt();
      EEPROM.write(65, disabled);
      EEPROM.commit();
      shouldRestart = true;
      Serial.println("Motion Sensor disabled");
    } else {
      disabled = 0;
      EEPROM.write(65, disabled);
      EEPROM.commit();
      shouldRestart = true;
      Serial.println("Motion Sensor enabled");
    }
  } else {
    Serial.println("Unknown command");
  }
}

void reconnect() {
  if (!client.connected() && (lastReconnectAttempt == 0 || millis() - lastReconnectAttempt > 300000)) {
    lastReconnectAttempt = millis();
#ifdef DEBUG
    Serial.println(F("Re-initiating Mqtt Connection"));
#endif
    initMQTT();
  }
}

bool isMqttConnected() {
  return client.connected();
}

void pushDeviceState(int heartBeat) {
  // Create a JSON message
  String jsonMessage = "{";
  jsonMessage += "\"date\":\"" + String(year()) + "-" + String(month()) + "-" + String(day()) + "\",";
  jsonMessage += "\"deviceID\":\"" + deviceID + "\",";
  jsonMessage += "\"motionState\":" + String(microMotion) + ",";
  jsonMessage += "\"lightState\":" + String(ldrVal) + ",";
  jsonMessage += "\"relayState\":" + String(digitalRead(relayPin)) + ",";
  jsonMessage += "\"heartBeat\":" + String(heartBeat);
  jsonMessage += "}";

  // Publish the JSON message to the MQTT topic
  client.publish("sensors/heartbeat", jsonMessage.c_str());

#ifdef DEBUG
  Serial.println(F("Device Heartbeat published to MQTT topic: sensors/heartbeat"));
  Serial.println(jsonMessage.c_str());
#endif
}

void handleMQTT() {
  // Handle the regular MQTT client
  if (!isMqttConnected()) {
    reconnect();
  } else {
    client.loop();
    // Check if enough time has passed since the last heartbeat
    if (millis() - lastHeartbeatTime >= heartbeatInterval) {
      pushDeviceState(1);
      // Update the last heartbeat time
      lastHeartbeatTime = millis();
    }
  }
}