// MQTT File MQTT.cpp
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <PubSubClient.h>
#include <ESPAsyncTCP.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "MQTT.h"
#include "Variables.h"

WiFiClientSecure net;
PubSubClient client(net);

const char* MQTT::mqttHost = "a38blua3zelira-ats.iot.ap-south-1.amazonaws.com";
const int MQTT::mqttPort = 8883;
const char* MQTT::thingName = "ESP-Devices";
const char* MQTT::subTopic = "sensor/state/sub";
const char* MQTT::pubTopic = "sensor/state/pub";
unsigned long MQTT::lastReconnectAttempt = 0;
unsigned long MQTT::lastMillis = 0;
unsigned long MQTT::previousMillis = 0;
unsigned long MQTT::lastHeartbeatTime = 0;
const long MQTT::interval = 5000;
const long MQTT::heartbeatInterval = 60000;  // 1 minute
const char* MQTT::deviceMacAddress = WiFi.macAddress().c_str();
const char* MQTT::chipId = String(ESP.getChipId()).c_str();

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

void MQTT::initialize() {

  Serial.print("mqttServer Host: ");
  Serial.println(MQTT::mqttHost);
  Serial.print("mqttServer Port: ");
  Serial.println(MQTT::mqttPort);
  Serial.print("mqttServer ThingName: ");
  Serial.println(MQTT::thingName);
  Serial.print("mqttServer Sub Topic: ");
  Serial.println(MQTT::subTopic);
  Serial.print("mqttServer Pub Topic: ");
  Serial.println(MQTT::pubTopic);

  NTPConnect();
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
  client.setServer(MQTT::mqttHost, MQTT::mqttPort);
  client.setCallback(messageReceived);
  Serial.println("Connecting to AWS IOT");
  mqttConnectStartTime = millis();

  while (!client.connect(MQTT::thingName)) {
    Serial.print(".");
    if (millis() - mqttConnectStartTime >= mqttConnectTimeout) {
      Serial.println("AWS connection timed out");
      Serial.print("Failed to connect to AWS IoT, rc=");
      Serial.println(client.state());

      break;
    }
    delay(1000);
  }

  if (client.connected()) {
  client.subscribe(MQTT::subTopic);
    Serial.println("AWS IoT Connected!");
  }
}

void MQTT::NTPConnect() {
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

bool MQTT::configureTime() {
  configTime(TIME_ZONE * 3600, 0, "time.nist.gov", "pool.ntp.org");

  // Wait for time configuration for a limited time
  unsigned long configTimeout = millis() + 5000;  // Wait for up to 15 seconds
  while (timeStatus() != timeSet && millis() < configTimeout) {
    delay(1000);
  }

  return timeStatus() == timeSet;
}

void MQTT::reconnect() {
  if (!client.connected() && (lastReconnectAttempt == 0 || millis() - lastReconnectAttempt > 300000)) {
    lastReconnectAttempt = millis();
    Serial.println("Re-initiating Mqtt Connection");
    MQTT::initialize();
  }
}

void MQTT::messageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println((char*)payload);
  Serial.println("...");

  // Create a JSON document to store the configuration
  DynamicJsonDocument jsonDocument(1024);

  // Deserialize JSON data from the request
  DeserializationError error = deserializeJson(jsonDocument, payload);

  if (error) {
    // Error handling: Failed to parse JSON data
    Serial.println("Error: Failed to parse JSON data");
  } else {
    if (jsonDocument.containsKey("ota")) {
      String otaUrl = jsonDocument["ota"].as<String>();
      // Configuration::FirmwareConfig firmwareConfig;
      // firmwareConfig.firmwareUrl = otaUrl.c_str();
      // Configuration configInstance;
      // Configuration::saveFirmwareConfig(firmwareConfig);
      shouldRestart = true;

    } else {
      int switchNumber = jsonDocument["switch_id"].as<int>();
      int switchState = jsonDocument["status"].as<int>();
      String message = jsonDocument["message"].as<String>();
    }
  }
  Serial.println();
}

void MQTT::publishSwitchState(int switchNumber, int switchState) {
  if (client.connected()) {
    // Check if pubTopic is not empty or null
    if (MQTT::pubTopic && MQTT::pubTopic[0] != '\0') {
      StaticJsonDocument<200> doc;
      doc["time"] = millis();
      doc["switchId"] = switchNumber;
      doc["status"] = switchState;
      char jsonBuffer[512];
      serializeJson(doc, jsonBuffer);
      Serial.print("Publish message: ");
      Serial.println(jsonBuffer);

      client.publish(MQTT::pubTopic, jsonBuffer);
    } else {
      Serial.println("pubTopic is not initialized or empty!");
    }
  }
}

void MQTT::publishDeviceInformation() {
  if (client.connected()) {
    String infoPubTopic = "modules/info/publish";
    StaticJsonDocument<200> doc;
    doc["time"] = millis();
    doc["deviceId"] = getDeviceID();
    doc["localMacAddress"] = deviceMacAddress;
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    Serial.print("Publish message: ");
    Serial.println(jsonBuffer);
    client.publish(infoPubTopic.c_str(), jsonBuffer);
    Serial.print("Device Information Published To Queue: ");
    Serial.println(infoPubTopic);
  } else {
    Serial.println("AWS Mqtt Client Not Connected!");
  }
}

void MQTT::publishMessage(const String& message) {
  if (client.connected()) {
    String infoPubTopic = "modules/info/publish";
    Serial.print("Publish message: ");
    Serial.println(message);
    client.publish(infoPubTopic.c_str(), message.c_str());
    Serial.print("Info Published To Queue: ");
    Serial.println(infoPubTopic);
  } else {
    Serial.println("AWS Mqtt Client Not Connected!");
  }
}

void MQTT::sendHeartBeat() {
  // Check if it's time to send the heartbeat
  unsigned long currentMillis = millis();

  if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
    // It's time to send the heartbeat

    StaticJsonDocument<200> doc;

    // Convert millis() to human-readable timestamp
    time_t currentTime = now();  // Use TimeLib.h function to get current time
    // Change the "time" field to human-readable format
    char formattedTime[20];  // Adjust the size based on your needs
    sprintf(formattedTime, "%04d-%02d-%02d %02d:%02d:%02d", year(currentTime), month(currentTime), day(currentTime), hour(currentTime), minute(currentTime), second(currentTime));

    doc["time"] = formattedTime;
    doc["device"] = getDeviceID();
    // doc["ip"] = WiFiManager::getDeviceIpAddress();
    doc["mac"] = deviceMacAddress;
    // doc["wifi"] = WiFiManager::isWifiConnected();
    // doc["int"] = Utilities::checkInternetConnectivity();
    // doc["mqtt"] = Utilities::checkMqttUrlAccessible();

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    Serial.print("Publish message: ");
    Serial.println(jsonBuffer);

    if (client.connected()) {
      String infoPubTopic = "modules/info/publish";
      if (client.publish(infoPubTopic.c_str(), jsonBuffer)) {
        Serial.print("Heartbeat Sent To Queue: ");
      } else {
        Serial.print("Heartbeat Sent To Queue Failed: ");
        Serial.println(client.state());
      }
      Serial.println(infoPubTopic);

    } else {
      Serial.println("AWS Mqtt Client Not Connected!");
    }

    // Update the last heartbeat time
    lastHeartbeatTime = currentMillis;
  }
}

bool MQTT::isMqttConnected() {
  return client.connected();
}

void MQTT::handleMQTT() {
  // Handle the regular MQTT client
  if (!MQTT::isMqttConnected()) {
    reconnect();
  } else {
    client.loop();
  }
}

String MQTT::getDeviceID() {
  // Change this line:
  char deviceID[50];
  strcpy(deviceID, chipId);
  strcat(deviceID, "-");
  strcat(deviceID, deviceMacAddress);
  return String(deviceID);
}