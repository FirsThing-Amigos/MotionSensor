#ifndef HTTPROUTES_H
#define HTTPROUTES_H

#ifdef ESP8266
    #include <ESP8266WebServer.h>
    extern ESP8266WebServer server;
    void handleHTTP(ESP8266WebServer &server);
#elif defined(ESP32)
    #include <WebServer.h>
    extern WebServer server;
    void handleHTTP(WebServer &server);
#endif
#include <WiFiClientSecure.h>


extern WiFiClientSecure wifiClientSecureOTA;

void initHttpServer();
void handleRoot();
void handleSensorStatus();
void handleWifiSettings();
void handleSaveWifi();
void handleUpdateVariable();
void handleNotFound();
bool isVariableDefined(const String &variableName);
bool updateVariable(const String &variableName, const String &value);
void performOTAUpdate(WiFiClientSecure &wifiClientSecureOTA);
void sendServerResponse(int statusCode, bool isJsonResponse, const String &content);
void saveWifiCredentials(const char *ssid, const char *password);
void writeOtaUrlToEEPROM(const char *url);
bool isValidUrl(const String &url);

#endif
