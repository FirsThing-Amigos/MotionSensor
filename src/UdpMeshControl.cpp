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

const char *MeshID = "nodeHotspot";
const char *MeshPassword = "1234567890";
const unsigned int localPort = 4210;
IPAddress STA_Host;
IPAddress localIP;
IPAddress gateway;
IPAddress subnet(255, 255, 255, 0);
bool nodeHotspot = false;

void ConnectTOMeshWifi(){
    WiFi.begin(MeshID, MeshPassword);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Wi-Fi connected to : ");
      Serial.println(MeshID);
    }
}

void configureIPAddress() {
  IPAddress STA_Host = WiFi.gatewayIP();
  
  Serial.print("Gateway IP address: ");
  Serial.println(STA_Host);
  
  if (STA_Host.toString() == "192.168.4.1") {
    localIP = IPAddress(192, 168, 1, 1);  // Desired static IP address
    gateway = IPAddress(192, 168, 1, 1);  // Gateway address (same as localIP for AP)
    Serial.println("Host IP is set to: '192.168.1.1'");
  } else {
    localIP = IPAddress(192, 168, 4, 1);  // Desired static IP address
    gateway = IPAddress(192, 168, 4, 1);  // Gateway address (same as localIP for AP)
    Serial.println("Host IP is set to: '192.168.4.1'");
  }
}

void meshHotspot() {
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(MeshID, MeshPassword);
  
  Serial.print("Hotspot name of second device is: ");
  Serial.println(MeshID);
  
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("Hotspot IP address: ");
  Serial.println(apIP);
  nodeHotspot = true;
  
  udp.begin(localPort);
  Serial.printf("Hotspot listening at IP %s, UDP port %d\n", apIP.toString().c_str(), localPort);
}

void forwardingIncomingPackets() {
    int packetSize = udp.parsePacket();
    if (packetSize!= 0) {
        char incomingPacket[255]; 
        int len = udp.read(incomingPacket, 255);
        if (len > 0) {
        incomingPacket[len] = 0; // Null-terminate the string
        }
        Serial.printf("Received packet: %s\n", incomingPacket);
        // Serial.printf("From IP: %s, Port: %d\n", udp.remoteIP().toString().c_str(), udp.remotePort());
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
  udp.beginPacket(STA_Host, localPort);
  #ifdef ESP8266
    udp.write(message);
  #elif defined ESP32
    udp.write(reinterpret_cast<const uint8_t*>(message), strlen(message));
  #endif
  udp.endPacket();
  Serial.println("Forwarded packet to remote server");
}

void checkConnectedStations() {
  int numStations = WiFi.softAPgetStationNum();
  if (numStations == 0) {
    WiFi.softAPdisconnect(true);
    nodeHotspot = false;
    Serial.println("No nodes are connected. Deactivating hotspot.");
  }
}
void broadcastDeviceState(){
  String jsonMessage = "{";
  jsonMessage += R"("deviceID":")" + deviceID + "\",";
  jsonMessage += "\"motionState\":" + String(microMotion) + ",";
  jsonMessage += "\"lightState\":" + String(ldrVal) + ",";
  jsonMessage += "\"relayState\":" + String(digitalRead(relayPin)) + ",";
  jsonMessage += R"("sbDeviceId":")" + String(sbDeviceId) + "\",";
  jsonMessage += "}";
  sendUdpPacket(jsonMessage.c_str());
}

void broadcastHeartbeat(){
  String MacAddress = getDeviceMacAddress();
  String jsonMessage = "{";
  jsonMessage += R"("deviceID":")" + deviceID + "\",";
  jsonMessage += "\"motionState\":" + String(microMotion) + ",";
  jsonMessage += "\"lightState\":" + String(ldrVal) + ",";
  jsonMessage += "\"relayState\":" + String(digitalRead(relayPin)) + ",";
  // jsonMessage += R"("deviceMac":")" + String(MacAddress) + "\","; 
  // jsonMessage += R"("temperature":")" + String(temperature) + "\",";
  // jsonMessage += R"("humidity":")" + String(humidity) + "\",";
  jsonMessage += "}";
  sendUdpPacket(jsonMessage.c_str());
}
