#ifndef HTTPROUTES_H
#define HTTPROUTES_H

#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

void startHttpServer();
void handleRoot();
void handleSensorStatus();
void handleWifiSettings();
void handleSaveWifi();
void handleRelayState();
void handleSetWaitTime();
void handleRestart();
void handleOTAUpdate();
void handleEnterOTAMode();
void performOTAUpdate(const String& url);
void handleNotFound();
void handleHTTPClients(ESP8266WebServer& server);
void restartESP();
void saveWifiCredentials(const char* ssid, const char* password);
#endif
