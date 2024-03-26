#ifndef WEBSOCKETHELPER_H
#define WEBSOCKETHELPER_H

#include <WebSocketsServer.h>

extern WebSocketsServer webSocketServer;

void initWebSocketServer();
void handleWebSocket();
void publishSensorStatus();

#endif
