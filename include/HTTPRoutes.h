#ifndef HTTPROUTES_H
#define HTTPROUTES_H

#if defined(ESP8266)
  #include <ESP8266WebServer.h>
#elif defined(ESP32)
  #include <WebServer.h>
#endif
#include <WiFiClientSecure.h>

#if defined(ESP8266)
  extern ESP8266WebServer server;
#elif defined(ESP32)
  extern WebServer server;
#endif
extern WiFiClientSecure wifiClientSecureOTA;

void initHttpServer();
void handleRoot();
void handleSensorStatus();
void handleWifiSettings();
void handleSaveWifi();
void handleUpdateVariable();
void handleNotFound();
#ifdef ESP8266
  void handleHTTP(ESP8266WebServer &server);
#elif defined(ESP32)
  void handleHTTP(WebServer &server);
#endif
bool isVariableDefined(const String &variableName);
bool updateVariable(const String &variableName, const String &value);
void performOTAUpdate(WiFiClientSecure &wifiClientSecureOTA);
void sendServerResponse(int statusCode, bool isJsonResponse, const String &content);
void saveWifiCredentials(const char *ssid, const char *password);
void writeOtaUrlToEEPROM(const char *url);
bool isValidUrl(const String &url);

#endif
