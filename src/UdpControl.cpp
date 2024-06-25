#include <EEPROM.h>
#include <Arduino.h>
#include "UdpControl.h"
#include "Variables.h"
#include "MQTT.h"
#include "DeviceControl.h"
#include "MQTT.h"


bool switchStateUdp = false;

// Define the local port for UDP operations
const unsigned int defaultLocalPort = 5000;

// Global variables for UDP instance and state
WiFiUDP udp;        
bool udpActive = false;  
String neighbourDeviceIP = "0.0.0.0";
unsigned long lastUpdateMillis = 0;

void openUdp() {
    if (!udpActive) {
        udp.begin(defaultLocalPort);
        Serial.println("UDP initialized and listening on port " + String(defaultLocalPort));
        udpActive = true;
    }
}

void closeUdp() {
    if (udpActive) {
        udp.stop();
        Serial.println("UDP closed on port " + String(defaultLocalPort));
        udpActive = false;
    }
}

void ipBroadcastByUdp() {
    udp.beginPacket("255.255.255.255", defaultLocalPort);
    // StaticJsonDocument<200> doc;
    // doc["deviceIp"] = serverIP.c_str();
    char jsonBuffer[256];
    // serializeJson(doc, jsonBuffer);
    udp.write(jsonBuffer);
    udp.endPacket();
    delay(100);
}

void processNeighbourDeviceIp() {
    int packetSize = udp.parsePacket(); // Check if a packet has been received

    if (packetSize) {
        char incomingPacket[256]; // Buffer to hold the incoming packet
        int len = udp.read(incomingPacket, 256); // Read the packet into the buffer

        if (len > 0) {
            incomingPacket[len] = 0; // Null-terminate the string
        }

        String remoteIp = udp.remoteIP().toString();

        Serial.print("From IP: ");
        Serial.println(remoteIp);
        neighbourDeviceIP = remoteIp;
        lastUpdateMillis = millis(); // Update the last update timestamp
    }
}

void sendUdpBroadcast(const char* jsonMessage) {
    if (!udpActive) {
        Serial.println("UDP is not active. Please call openUdp() before sending.");
        return;
    }
    
    Serial.println("Sending UDP broadcast packet...");

    // Send UDP broadcast packet
    udp.beginPacket("255.255.255.255", defaultLocalPort);
    udp.write(jsonMessage);
    udp.endPacket();
    Serial.println("Packet sent");
    delay(100);
}


String parseJsonValue(const char* json, const char* key) {
    String jsonString = String(json);
    String searchKey = "\"" + String(key) + "\":";
    int startIndex = jsonString.indexOf(searchKey);
    if (startIndex == -1) return "";

    startIndex += searchKey.length();
    char delimiter = json[startIndex] == '\"' ? '\"' : ',';
    int endIndex = jsonString.indexOf(delimiter, startIndex + 1);

    if (delimiter == '\"') {
        startIndex += 1; // Skip the first quotation mark
        endIndex = jsonString.indexOf('\"', startIndex);
    } else {
        endIndex = jsonString.indexOf(',', startIndex);
        if (endIndex == -1) endIndex = jsonString.indexOf('}', startIndex);
    }

    return jsonString.substring(startIndex, endIndex);
}
void processIncomingUdp() {
    int packetSize = udp.parsePacket();

    if (packetSize) {
        char incomingPacket[256];
        int len = udp.read(incomingPacket, 256);
        incomingPacket[len] = 0; // Null-terminate the string

        String date = parseJsonValue(incomingPacket, "date");
        String deviceID = parseJsonValue(incomingPacket, "deviceID");
        int motionState = parseJsonValue(incomingPacket, "motionState").toInt();
        int lightState = parseJsonValue(incomingPacket, "lightState").toInt();
        int relayState = parseJsonValue(incomingPacket, "relayState").toInt();
        String localIp = parseJsonValue(incomingPacket, "localIp");
        int sbDeviceId = parseJsonValue(incomingPacket, "sbDeviceId").toInt();

        // Process and print the received values
        Serial.print("Date: ");
        Serial.println(date);
        Serial.print("Device ID: ");
        Serial.println(deviceID);
        Serial.print("Motion State: ");
        Serial.println(motionState);
        Serial.print("Light State: ");
        Serial.println(lightState);
        Serial.print("Relay State: ");
        Serial.println(relayState);
        Serial.print("Local IP: ");
        Serial.println(localIp);
        Serial.print("Switch Board Device ID: ");
        Serial.println(sbDeviceId);

        // Example: Publish to MQTT if necessary
        String mqttMessage = "{";
        mqttMessage += "\"date\":\"" + date + "\",";
        mqttMessage += "\"deviceID\":\"" + deviceID + "\",";
        mqttMessage += "\"motionState\":" + String(motionState) + ",";
        mqttMessage += "\"lightState\":" + String(lightState) + ",";
        mqttMessage += "\"relayState\":" + String(relayState) + ",";
        mqttMessage += "\"localIp\":\"" + localIp + "\",";
        mqttMessage += "\"sbDeviceId\":" + String(sbDeviceId);
        mqttMessage += "}";

        // Publish the message using MqttManager
        pubSubClient.publish("sensors/heartbeat", mqttMessage.c_str());

        // For demonstration, print MQTT message to Serial monitor
        Serial.print("MQTT Message: ");
        Serial.println(mqttMessage);
    }
}

bool shouldProcessNeighbourDeviceIp() {
    unsigned long currentMillis = millis();
    return (currentMillis - lastUpdateMillis >= 30 * 1000);
}

String getNeighbourDeviceIP() {
    return neighbourDeviceIP;
}

void broadcastChangedRelayState() {
    const time_t currentTime = relayStateChangesTime;
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
    jsonMessage += R"("localIp":")" + serverIP.toString() + "\",";
    jsonMessage += R"("sbDeviceId":")" + String(sbDeviceId) + "\",";
    jsonMessage += "}";
    sendUdpBroadcast(jsonMessage.c_str());
}
