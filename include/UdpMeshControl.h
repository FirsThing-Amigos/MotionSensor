#ifndef UDPMESHCONTROL_H
#define UDPMESHCONTROL_H


#include <WiFiUdp.h>

extern const unsigned int localPort;



void initmeshUdpListen();
void forwardingIncomingPackets();
void sendUdpPacket(const char* message);
void broadcastDeviceState();
void broadcastHeartbeat();

#endif
