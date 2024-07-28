#include "HTTPRoutes.h"
#include <EEPROM.h>
#ifdef ESP8266
  #include <ESP8266HTTPClient.h>
  #include <ESP8266httpUpdate.h>
#elif defined(ESP32)
  #include <Update.h>
  #include <HTTPClient.h>
#endif
#include <algorithm>
#include "DeviceControl.h"
#include "Variables.h"
#include "WIFIControl.h"

int sbDeviceId;

void initHttpServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleSensorStatus);
    server.on("/wifi", HTTP_GET, handleWifiSettings);
    server.on("/saveWifi", HTTP_POST, handleSaveWifi);
    server.on("/updateVariable", HTTP_POST, handleUpdateVariable);
    server.onNotFound(handleNotFound);
    server.begin();
#ifdef DEBUG
    Serial.println(F("HTTP Server Started!"));
#endif
}

void handleRoot() {
    // Serial.println(F("Entered handle root route:"));
    // Define CSS for styling the icons based on their state
    String css1 = R"(
    <style>
      .container {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      text-align: center;
    }
    .light-status {
      width: 300px;
      height: 300px;
      margin: 10px;
      display: flex;
      flex-direction: column;
      align-items: center;
      text-align: center;
      z-index: 1;
    }
    .sensor-icons {
      display: flex;
      justify-content: center;
      margin-top: 20px;
    }
    .sensor-icon {
      width: 100px;
      height: 100px;
      margin: 10px;
      display: flex;
      flex-direction: column;
      align-items: center;
      text-align: center;
    }
    .on { background-color: green; }
    .off { background-color: red; }
    .icon {
      width: 60px;
      height: 60px;
      border-radius: 50%;
    }
    .microwave {
      background-color: blue;
    }
    )";

    String css3 = R"(
    .ldr {
      background-color: yellow;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    </style>
  )";

    String css = css1;
    css += css3;

#ifdef SOCKET
    String fetch = R"(
        let isFetching = false;
      // Function to fetch content from server and update the response div
        async function fetchContent() {
          if (!isFetching) {
            isFetching = true;
            try {
              // Create a WebSocket connection to the ESP device
              const socket = new WebSocket('ws://)" +
                   serverIP + R"(:81');

              // Event listener for when the WebSocket connection is opened
              socket.onopen = function(event) {
                console.log('WebSocket connection opened.');
              };

              // Event listener for incoming messages from the WebSocket server
              socket.onmessage = function(event) {
                const sensorData = JSON.parse(event.data);
                console.log('Received sensor data:', sensorData);
                updateIcons(sensorData);
              };

              // Event listener for WebSocket connection errors
              socket.onerror = function(error) {
                console.error('WebSocket error:', error);
              };
            } catch (error) {
              console.error('There was a problem with your WebSocket operation:', error);
            } finally {
              isFetching = false;
            }
          }
        }
  )";
#else
    String fetch = R"(
        let isFetching = false;
      // Function to fetch content from server and update the response div
    async function fetchContent() {
          if (!isFetching) {
            isFetching = true;
            try {
              const response = await fetch('/status');
              const data = await response.json();
              updateIcons(data);
            } catch (error) {
              console.error('There was a problem with your fetch operation:', error);
            } finally {
              isFetching = false;
            }
          }
        }
  )";
#endif

    // Define the HTML content with icons and pin numbers
    String html1 = R"(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Continuous Server Response</title>
      )" + css + R"(
    </head>
    <body>
      <div id="response"></div>
      <script>
         )" + fetch +
                   R"(
        function updateIcons(data) {
          const lightState = data["relay_pin_state"];
          const lightStateElement = document.getElementById("light-status");
          if(lightStateElement){
            if(data["last_motion_time"]){
              lightStateElement.textContent = data["last_motion_time"] + "<br>" + data["count_down_light_off"] + "</br>" + data["count_down_day_light"];
            }
            if(lightState){
              lightStateElement.classList.add('on');
              lightStateElement.classList.remove('off');
            } else {
              lightStateElement.classList.add('off');
              lightStateElement.classList.remove('on');
            }
          }

          for (const key in data) {
            const sensorElement = document.getElementById(key);
            if (sensorElement) {
              const sensorStateKey = key + '_state';
              const sensorValKey = key + '_val';
              const sensorValElement = document.querySelector(`#${key} > div > span`);

              if (data[sensorStateKey]){
                sensorElement.classList.add('on');
                sensorElement.classList.remove('off');
              } else {
                sensorElement.classList.add('off');
                sensorElement.classList.remove('on');
              }

              if (sensorValElement) {
                sensorValElement.textContent = data[key];
                const iconLdrDiv = document.querySelector('.ldr');
                const sensorVal = data[sensorValKey];
                if (iconLdrDiv && sensorVal) {
                  iconLdrDiv.textContent = sensorVal;
                }
              }
            }
          }
        }

        setInterval(fetchContent, 500);
      </script>
      <div class="container">
        <div class="light-status" id="light-status">
          <div>Light Status: <span></span></div>
        </div>
        <div class="sensor-icons">
          <div class="sensor-icon" id="microwave_sensor_pin">
            <div class="icon microwave"></div>
            <div>Microwave: <span></span></div>
          </div>
          <div class="sensor-icon" id="ldr_sensor_pin">
            <div class="icon ldr"></div>
            <div>LDR: <span></span></br><span></span></div>
          </div>
  )";
    String html3 = R"(
    </div>
      </div>
    </body>
    </html>
  )";

    String html = html1;
    html += html3;

    server.send(200, "text/html", html);
}

void handleSensorStatus() {
    String response = getDeviceStatus();
    sendServerResponse(200, true, response);
}

void handleWifiSettings() {
    Serial.println("Handeling handleWifiSettings route...");
    String form = "<form action='/saveWifi' method='post'>";
    form += "SSID: <input type='text' name='ssid'><br>";
    form += "Password: <input type='password' name='password'><br>";
    form += "<input type='submit' value='Save'>";
    form += "</form>";

    server.send(200, "text/html", form);
}

void handleSaveWifi() {
    String ssidString = server.arg("ssid");
    String passwordString = server.arg("password");

    char ssidCharArray[32];
    char passwordCharArray[32];
    ssidString.toCharArray(ssidCharArray, sizeof(ssidCharArray));
    passwordString.toCharArray(passwordCharArray, sizeof(passwordCharArray));

    // Save WiFi credentials to EEPROM
    saveWifiCredentials(ssidCharArray, passwordCharArray);
    shouldRestart = true;
    server.send(200, "text/plain", "WiFi credentials saved. Restarting device...");
}

void saveWifiCredentials(const char *ssid, const char *password) {
    char ssidCharArray[32];
    char passwordCharArray[32];
    strncpy(ssidCharArray, ssid, sizeof(ssidCharArray));
    strncpy(passwordCharArray, password, sizeof(passwordCharArray));
    EEPROM.put(0, ssidCharArray);
    EEPROM.put(32, passwordCharArray);
    EEPROM.commit();
}

void handleUpdateVariable() {
    String key = server.arg("key");
    String value = server.arg("value");
    Serial.print("key: ");
    Serial.print(key);
    Serial.println("value: ");
    Serial.println(value);

    if (isVariableDefined(key)) {
        if (updateVariable(key, value)) {
            sendServerResponse(200, false, "Variable updated successfully.");
        } else {
            sendServerResponse(400, false, "Failed to update variable.");
        }
    } else {
        sendServerResponse(400, false, "Variable does not exist.");
    }
}

void handleNotFound() { sendServerResponse(404, false, "HTTP routes are not available on hotspot mode."); }

void sendServerResponse(int statusCode, bool isJsonResponse, const String &content) {
    String contentType = isJsonResponse ? "application/json" : "text/plain";
    server.send(statusCode, contentType, content);
}

void performOTAUpdate(WiFiClientSecure &wifiClientSecureOTA) {
    if (wifiDisabled == 0 && isWifiConnected()) {

        Serial.print("Checking for updates: ");
        Serial.println(otaUrl);
        writeOtaUrlToEEPROM("#");
        shouldRestart = true;
        wifiClientSecureOTA.setInsecure();
        wifiClientSecureOTA.setTimeout(10000);
        Serial.println("OTA Update Started");

        #ifdef ESP8266
            switch (ESPhttpUpdate.update(wifiClientSecureOTA, otaUrl)) {
                case HTTP_UPDATE_FAILED:
                    Serial.println("OTA Update failed. Error: " + ESPhttpUpdate.getLastErrorString());
                    break;
                case HTTP_UPDATE_NO_UPDATES:
                    Serial.println("No OTA updates available");
                    break;
                case HTTP_UPDATE_OK:
                    Serial.println("OTA Update successful");
                    break;
            }
        #elif defined(ESP32)
            HTTPClient http;
            http.begin(wifiClientSecureOTA, otaUrl);
            int httpCode = http.GET();
            if (httpCode == HTTP_CODE_OK) {
                int contentLength = http.getSize();
                if (contentLength > 0) {
                    bool canBegin = Update.begin(contentLength);
                    if (canBegin) {
                        Serial.println("Begin OTA. This may take a few minutes...");
                        WiFiClient *stream = http.getStreamPtr();
                        size_t written = Update.writeStream(*stream);
                        if (written == contentLength) {
                            Serial.println("Written : " + String(written) + " successfully");
                        } else {
                            Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
                        }
                        if (Update.end()) {
                            Serial.println("OTA done!");
                            if (Update.isFinished()) {
                                Serial.println("Update successfully completed. Rebooting.");
                                shouldRestart = true;
                            } else {
                                Serial.println("Update not finished? Something went wrong!");
                            }
                        } else {
                            Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                        }
                    } else {
                        Serial.println("Not enough space to begin OTA");
                    }
                } else {
                    Serial.println("Content length not valid");
                }
            } else {
                Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
            }
            http.end();  // Close HTTP connection
        #endif
        saveResetCounter(0);
    } else {
        Serial.println("Unable to perform OTA, Wifi not connected");
    }
}

bool isVariableDefined(const String &variableName) {
    static const String variableList[] = {"disabled", "shouldRestart", "otaMode",          "otaUrl",           "ldrPin",
                                          "microPin", "relayPin",      "lightOffWaitTime", "lowLightThreshold", "heartbeatInterval", "sbDeviceId","wifiDisabled","MeshNetwork"};


    return std::any_of(std::begin(variableList), std::end(variableList),
                       [&variableName](const String &var) { return variableName.equals(var); });
}

  bool updateVariable(const String &variableName, const String &value) {
      if (variableName == "disabled") {
          disabled = value.toInt();
          EEPROM.write(70, disabled);
          EEPROM.commit();
          shouldRestart = true;

      } else if (variableName == "ldrPin") {
          ldrPin = value.toInt();
          pinMode(ldrPin, INPUT);
      } else if (variableName == "microPin") {
          microPin = value.toInt();
          pinMode(microPin, INPUT);
      } else if (variableName == "relayPin") {
        relayPin = value.toInt();
        pinMode(relayPin, OUTPUT);
    } else if (variableName == "lightOffWaitTime") {
        if (value.toInt() > 0) {
          int lightOffWaitTimeInMillis = value.toInt();
            lightOffWaitTime = lightOffWaitTimeInMillis/1000;
            EEPROM.write(72, lightOffWaitTime);
            EEPROM.commit();
        }

    } else if (variableName == "wifiDisabled") {
      wifiDisabled = value.toInt();
      EEPROM.write(81, wifiDisabled);
      EEPROM.commit();
      Serial.print("wifiDisabled value saved : ");
      Serial.println(EEPROM.read(81));
      shouldRestart = true;
        
    } else if (variableName == "sbDeviceId") {
        sbDeviceId = value.toInt();
        EEPROM.write(77, sbDeviceId);
        EEPROM.commit();

    } else if (variableName == "lowLightThreshold") {
        if (value.toInt() > 0) {
            lowLightThreshold = value.toInt();
            EEPROM.write(74, lowLightThreshold);
            EEPROM.commit();
        }

    } else if (variableName == "otaMode") {
      bool otaModeValue = value.equals("1");
#ifdef DEBUG
        Serial.println(EEPROM.read(69));
#endif
        EEPROM.write(69, otaModeValue);
        EEPROM.commit();
#ifdef DEBUG
        Serial.println(EEPROM.read(69));
#endif
        shouldRestart = true;
    } else if (variableName == "shouldRestart") {
        shouldRestart = true;
    } else if (variableName == "otaUrl") {
        writeOtaUrlToEEPROM(value.c_str());
        shouldRestart = true;
    } else if(variableName == "heartbeatInterval"){
      if (value.toInt() > 0) {
          int heartbeatIntervalInMillis = value.toInt();
            heartbeatInterval = heartbeatIntervalInMillis/1000;
            EEPROM.write(65, heartbeatInterval);
            EEPROM.commit();
        }
    } else if(variableName == "MeshNetwork"){
      bool MeshMode = value.equals("1");
      EEPROM.write(77, MeshMode);
      EEPROM.commit();
      Serial.print("MeshNetwork is set to be : ");
      Serial.println(EEPROM.read(77));
    } else {
        return false;
    }

    return true;
}

#ifdef ESP8266
  void handleHTTP(ESP8266WebServer &server) { server.handleClient(); }
#elif defined(ESP32)
  void handleHTTP(WebServer &server) { server.handleClient();}
#endif


void writeOtaUrlToEEPROM(const char *url) {
    EEPROM.begin(256);
    for (int i = 0; i < static_cast<int>(strlen(url)) && i < 256; ++i) {
        EEPROM.write(88 + i, url[i]);
    }
    EEPROM.write(88 + static_cast<int>(strlen(url)), '\0');
    EEPROM.commit();
}

bool isValidUrl(const String &url) {
    if (url.length() == 0) {
        return false;
    }

    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        return false;
    }

    // Check if URL contains a valid hostname (You can implement your own validation logic here) For simplicity, we'll
    // just check if there's at least one '.' character
    if (url.indexOf('.') == -1) {
        return false;
    }

    return true;
}
