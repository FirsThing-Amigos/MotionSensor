#ifndef UDPCONTROL_H
#define UDPCONTROL_H

#include <WiFiUdp.h>
void openUdp();
void ipBroadcastByUdp();
void processNeighbourDeviceIp();
void sendUdpBroadcast(const char* jsonMessage);
String parseJsonValue(const char* json, const char* key);
void processIncomingUdp();
bool shouldProcessNeighbourDeviceIp();
String getNeighbourDeviceIP();
void broadcastChangedRelayState();

#endif