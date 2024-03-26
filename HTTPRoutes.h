#ifndef HTTPROUTES_H
#define HTTPROUTES_H

#include <ESP8266WebServer.h>

extern ESP8266WebServer server;

void startHttpServer();
void handleRoot();
void handleSensorStatus();
void handleWifiSettings();
void handleSaveWifi();
void handleUpdateVariable();
void handleNotFound();
void handleHTTPClients(ESP8266WebServer& server);
bool isVariableDefined(const String& variableName);
bool updateVariable(const String& variableName, const String& value);
void performOTAUpdate(const String& url);
void sendServerResponse(int statusCode, bool isJsonResponse, const String& content);
void restartESP();
void saveWifiCredentials(const char* ssid, const char* password);

#endif
