#ifndef UDPMESHCONTROL_H
#define UDPMESHCONTROL_H


#include <WiFiUdp.h>

extern const char *MeshID;
extern const char *MeshPassword ;
extern const unsigned int localPort;
extern IPAddress STA_Host;
extern IPAddress localIP;
extern IPAddress gateway;
extern IPAddress subnet;


void ConnectTOMeshWifi();
void configureIPAddress();
void meshHotspot();
void forwardingIncomingPackets();
void checkConnectedStations();
void sendUdpPacket(const char* message);
void broadcastDeviceState();
void broadcastHeartbeat();

#endif
