#include "UdpMeshControl.h"
#ifdef ESP8266
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <WiFi.h>
#endif
#include "DeviceControl.h"
#include "HTTPRoutes.h"
#include "Variables.h"
#include "WIFIControl.h"
#include "MQTT.h"
#include <WiFiUdp.h>
WiFiUDP udp;

const unsigned int localPort = 4210;

void initmeshUdpListen() {
  udp.begin(localPort);
  Serial.printf("Hotspot listening at IP %s, UDP port %d\n", WiFi.softAPIP().toString().c_str(), localPort);
}

void forwardingIncomingPackets() {
    int packetSize = udp.parsePacket();
    if (packetSize!= 0) {
        char incomingPacket[512]; 
        int len = udp.read(incomingPacket, 512);
        if (len > 0) {
        incomingPacket[len] = 0; // Null-terminate the string
        }
        Serial.printf("Received packet: %s\n", incomingPacket);
        Serial.printf("From IP: %s, Port: %d\n", udp.remoteIP().toString().c_str(), udp.remotePort());
        // Forward the received message to the remote server over Wi-Fi
        if(node){
          sendUdpPacket(incomingPacket);
        }
        else{
          publishUdpDataToMqtt(incomingPacket);
        }
    }
}
void sendUdpPacket(const char* message){
  udp.beginPacket(GatwayIP, localPort);
  #ifdef ESP8266
    udp.write(message);
  #elif defined ESP32
    udp.write(reinterpret_cast<const uint8_t*>(message), strlen(message));
  #endif
  udp.endPacket();
  Serial.println("Forwarded packet to remote server");
}

void broadcastDeviceState(){
  String jsonMessage = "{";
  jsonMessage += R"("dID":")" + deviceID + "\",";
  jsonMessage += "\"mS\":" + String(microMotion) + ",";
  jsonMessage += "\"lS\":" + String(ldrVal) + ",";
  jsonMessage += "\"rS\":" + String(digitalRead(relayPin)) + ",";
  jsonMessage += R"("sbId":")" + String(sbDeviceId);
  jsonMessage += "}";
  sendUdpPacket(jsonMessage.c_str());
  Serial.println("Relay Update is broadcasting to UDP ");
}

void broadcastHeartbeat(){
  String MacAddress = getDeviceMacAddress();
  String jsonMessage = "{";
  jsonMessage += R"("dID":")" + deviceID + "\",";
  jsonMessage += "\"mS\":" + String(microMotion) + ",";
  jsonMessage += "\"lS\":" + String(ldrVal) + ",";
  jsonMessage += "\"rS\":" + String(digitalRead(relayPin)) + ",";
  jsonMessage += R"("lIp":")" + serverIP.toString();
  // jsonMessage += R"("deviceMac":")" + String(MacAddress) + "\","; 
  // jsonMessage += R"("temperature":")" + String(temperature) + "\",";
  // jsonMessage += R"("humidity":")" + String(humidity);
  jsonMessage += "}";
  sendUdpPacket(jsonMessage.c_str());
  Serial.println("Heartbeat broadcast to UDP ");
}
