#ifndef HTTPROUTES_H
#define HTTPROUTES_H

#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>

extern ESP8266WebServer server;
extern WiFiClientSecure wifiClientSecureOTA;
// extern String SwitchID;
void initHttpServer();
void handleRoot();
void handleSensorStatus();
void handleWifiSettings();
void handleSaveWifi();
void handleUpdateVariable();
void handleNotFound();
void handleHTTP(ESP8266WebServer &server);
void writeSwitchIdToEEPROM(const char *id);
bool isVariableDefined(const String &variableName);
bool updateVariable(const String &variableName, const String &value);
void performOTAUpdate(WiFiClientSecure &wifiClientSecureOTA);
void sendServerResponse(int statusCode, bool isJsonResponse, const String &content);
void saveWifiCredentials(const char *ssid, const char *password);
void writeOtaUrlToEEPROM(const char *url);
bool isValidUrl(const String &url);
bool verifyDataInEEPROM(const char *expectedData);



#endif
