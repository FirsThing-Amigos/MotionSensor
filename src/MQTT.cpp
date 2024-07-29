// MQTT File MQTT.cpp
#include "MQTT.h"
#include <EEPROM.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
#include "DeviceControl.h"
#include "HTTPRoutes.h"
#include "Variables.h"
#include "WIFIControl.h"
#include "UdpMeshControl.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "in.pool.ntp.org", 19800);

WiFiClientSecure wifiClientSecure;
PubSubClient pubSubClient(wifiClientSecure);

const char *mqttHost = "a38blua3zelira-ats.iot.ap-south-1.amazonaws.com";
constexpr int mqttPort = 8883;
const char *thingName = "ESP-Devices";
// const char* thingName = "YogeshHouseMotionSensor_3";

unsigned long lastReconnectAttempt = 0;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long heartbeatIntervalTime = heartbeatInterval *1000;

static constexpr char cacert[] PROGMEM = R"EOF(
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

static constexpr char client_cert[] PROGMEM = R"KEY(
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

static constexpr char privkey[] PROGMEM = R"KEY(
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

#ifdef ESP8266
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
#elif defined(ESP32)
#endif

int8_t TIME_ZONE = 5; // Adjusted to use integer value
#define TIME_ZONE (+5.5)  // Enclosed in parentheses
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
    if(checkInternetConnectivity() ){
        Serial.print("Internet is connected. Fetching current time form Time Server");
        updateNtpTimeWithRetries();
    }
     #ifdef ESP8266
        wifiClientSecure.setTrustAnchors(&cert);
        wifiClientSecure.setClientRSACert(&client_crt, &key);
    #elif defined(ESP32)
        wifiClientSecure.setCACert(cacert);
        wifiClientSecure.setCertificate(client_cert);
        wifiClientSecure.setPrivateKey(privkey);
    #endif
    pubSubClient.setServer(mqttHost, mqttPort);
    pubSubClient.setCallback(messageReceived);
#ifdef DEBUG
    Serial.print(F("Connecting to AWS IOT"));
#endif
    connectToMqtt();

    if (pubSubClient.connected()) {
        Serial.println(F("AWS IoT Connected!"));
        const String subTopic = ("sensor/" + String(getDeviceID()) + "/state/sub");
        pubSubClient.subscribe(subTopic.c_str());
        Serial.print(subTopic);
        Serial.println(F(": Subscribed!"));
    }
}

void connectToMqtt() {
    mqttConnectStartTime = millis();

    while (!pubSubClient.connect(thingName)) {
        Serial.print(".");
        if (millis() - mqttConnectStartTime >= mqttConnectTimeout) {
#ifdef DEBUG
            Serial.print(F("Failed to connect to AWS IoT, rc="));
            Serial.println(pubSubClient.state());
#endif
            break;
        }
        delay(1000);
    }
}

bool configureTime() {
    configTime(TIME_ZONE * 3600, 0, "in.pool.ntp.org", "time.nist.gov", "pool.ntp.org");
    timeClient.begin();
    delay(500);
    bool timeUpdate = timeClient.update();
    if (!timeUpdate){
        return false;
    }
    timeClient.update();
    Serial.print("Current time: ");
    Serial.println(timeClient.getFormattedTime());
    const time_t currentTime = timeClient.getEpochTime();
    Serial.println("done!");
    tm timeinfo{};
    gmtime_r(&currentTime, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
    return true;
}

void messageReceived(const char *topic, const byte *payload, const unsigned int length) {
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");

    for (unsigned int i = 0; i < length; i++) {
        Serial.print(static_cast<char>(payload[i]));
    }
    Serial.println();

    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += static_cast<char>(payload[i]);
    }

    String command = message.substring(0, message.indexOf(' '));
    String valueStr = message.substring(message.indexOf(' ') + 1);

    if (command.equals("otaUrl")) {
        Serial.print("OTA URL Received: ");
        Serial.println(valueStr);
        writeOtaUrlToEEPROM(valueStr.c_str());
        shouldRestart = true;

    } else if (command.equals("disabled")) {
        if (valueStr.equalsIgnoreCase("true")) {
            disabled = valueStr.toInt();
            EEPROM.write(70, disabled);
            EEPROM.commit();
            shouldRestart = true;
            Serial.println("Motion Sensor disabled");
        } else {
            disabled = false;
            EEPROM.write(70, disabled);
            EEPROM.commit();
            shouldRestart = true;
            Serial.println("Motion Sensor enabled");
        }
    }  else if (command.equals("sbDeviceId")) {
        sbDeviceId = valueStr.toInt();
        EEPROM.write(77, sbDeviceId);
        EEPROM.commit();
    } else if(command.equals( "MeshNetwork")){
      bool MeshMode = valueStr.equals("1");
      EEPROM.write(77, MeshMode);
      EEPROM.commit();
      Serial.print("MeshNetwork is set to be : ");
      Serial.println(EEPROM.read(77));
    } else {
        Serial.println("Unknown command");
    }
}

void reconnect() {
    if (!pubSubClient.connected() && (lastReconnectAttempt == 0 || millis() - lastReconnectAttempt > 300000)) {
        lastReconnectAttempt = millis();
#ifdef DEBUG
        Serial.print(F("Re-initiating Mqtt Connection"));
#endif
        connectToMqtt();
    }
}

bool isMqttConnected() { return pubSubClient.connected(); }

void pushDeviceState() {
    const time_t currentTime = timeClient.getEpochTime();
    tm timeinfo{};
    gmtime_r(&currentTime, &timeinfo);
    String dateTimeString(asctime(&timeinfo));
    dateTimeString.trim();
    
    String jsonMessage = "{";
    jsonMessage += R"("date":")" + dateTimeString + "\",";
    jsonMessage += R"("deviceID":")" + deviceID + "\",";
    jsonMessage += "\"motionState\":" + String(microMotion) + ",";
    jsonMessage += "\"lightState\":" + String(ldrVal) + ",";
    jsonMessage += "\"relayState\":" + String(digitalRead(relayPin)) + ",";
    jsonMessage += R"("sbDeviceId":")" + String(sbDeviceId) + "\",";
    jsonMessage += "}";

    pubSubClient.publish("sensors/heartbeat/stage", jsonMessage.c_str());

#ifdef DEBUG
    Serial.println(F("Device Heartbeat published to MQTT topic: sensors/heartbeat"));
    Serial.println(jsonMessage.c_str());
#endif
}
void publishDeviceHeartbeat(){
    const time_t currentTime = timeClient.getEpochTime();
    tm timeinfo{};
    gmtime_r(&currentTime, &timeinfo);
    String dateTimeString(asctime(&timeinfo));
    dateTimeString.trim();
    String MacAddress = getDeviceMacAddress();

    String jsonMessage = "{";
    jsonMessage += R"("date":")" + dateTimeString + "\",";
    jsonMessage += R"("deviceID":")" + deviceID + "\",";
    jsonMessage += "\"motionState\":" + String(microMotion) + ",";
    jsonMessage += "\"lightState\":" + String(ldrVal) + ",";
    jsonMessage += "\"relayState\":" + String(digitalRead(relayPin)) + ",";
    jsonMessage += R"("localIp":")" + serverIP.toString() + "\",";
    jsonMessage += R"("deviceMac":")" + String(MacAddress) + "\","; 
    jsonMessage += R"("temperature":")" + String(temperature) + "\",";
    jsonMessage += R"("humidity":")" + String(humidity) + "\",";
    jsonMessage += "\"heartBeat\":" + String(1);
    jsonMessage += "}";

    pubSubClient.publish("sensors/heartbeat/stage", jsonMessage.c_str());

}

void handleMQTT() {

    if (!pubSubClient.connected() && isWifiConnected()) {
        reconnect();
    } else {
        pubSubClient.loop();
        handleHeartbeat();
    }
}

bool updateNtpTimeWithRetries() {
    int retries = 1;
    while (retries <= 5) {
        if (configureTime()) {
            Serial.println(retries);
            return true;  // Exit the loop if time synchronization succeeds
        }
        retries++;
        delay(1000);  // Wait for 1 second before retrying
        if(retries==5){
            saveResetCounter(0);
            ESP.restart();
        }
    }
    return false;
}

void handleHeartbeat() {
    if (lastHeartbeatTime == 0 || millis() - lastHeartbeatTime >= heartbeatIntervalTime) {
        if(!node){
         publishDeviceHeartbeat(); 
        }else if(node) {
            broadcastHeartbeat();
        }
        lastHeartbeatTime = millis();
    }
}

void publishUdpDataToMqtt(const char *message){
    const time_t currentTime = timeClient.getEpochTime();
    tm timeinfo{};
    gmtime_r(&currentTime, &timeinfo);
    String dateTimeString(asctime(&timeinfo));
    dateTimeString.trim();

    String UdpMessage = String(message);
    int insertPos = UdpMessage.lastIndexOf('}');
    String newKeyValue = "\"date\": \"" + dateTimeString + "\"";
    if (insertPos != -1) {
        if (UdpMessage.charAt(insertPos - 1) != '{') {
            UdpMessage = UdpMessage.substring(0, insertPos) + "," + newKeyValue + UdpMessage.substring(insertPos);
        } else {
            UdpMessage = UdpMessage.substring(0, insertPos) + newKeyValue + UdpMessage.substring(insertPos);
        }
    }
    pubSubClient.publish("sensors/heartbeat/stage", UdpMessage.c_str());
}