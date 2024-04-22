#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <EEPROM.h>
#include "HTTPRoutes.h"
#include "Variables.h"
#include "DeviceControl.h"

String otaUrl;

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
  String css2 = R"(
    .pir {
      background-color: orange;
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
#ifdef PIR
  css += css2;
#endif
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
              const socket = new WebSocket('ws://)"
                 + serverIP + R"(:81');

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
         )" + fetch
                 + R"(
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
  String html2 = R"(
    <div class="sensor-icon" id="pir_sensor_pin">
            <div class="icon pir"></div>
            <div>PIR: <span></span></div>
          </div>
  )";
  String html3 = R"(
    </div>
      </div>
    </body>
    </html>
  )";

  String html = html1;
#ifdef PIR
  html += html2;
#endif
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

void saveWifiCredentials(const char* ssid, const char* password) {
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

void handleNotFound() {
  sendServerResponse(404, false, "HTTP routes are not available on hotspot mode.");
}

void sendServerResponse(int statusCode, bool isJsonResponse, const String& content) {
  String contentType = isJsonResponse ? "application/json" : "text/plain";
  server.send(statusCode, contentType, content);
}

void performOTAUpdate() {
  Serial.print("Checking for updates: ");
  Serial.println(otaUrl);
  WiFiClient client;  // Create a WiFiClient object
  HTTPClient http;

  // Begin the HTTPClient with the WiFiClient object and the OTA URL
  if (!http.begin(client, otaUrl)) {
    Serial.println("Failed to begin HTTP connection");
    return;
  }

  int httpCode = http.GET();  // Perform the HTTP GET request

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();
    ESPhttpUpdate.update(*stream, "", 80, otaUrl);  // Perform OTA update with URL parameter and port 80
  } else {
    Serial.println("Failed to check for updates");
  }

  http.end();  // End the HTTPClient
}

bool isVariableDefined(const String& variableName) {
  static const String variableList[] = {
    "disabled", "shouldRestart", "otaMode", "otaUrl", "ldrPin", "microPin", "relayPin", "lightOffWaitTime", "lowLightThreshold"
  };

  for (const auto& var : variableList) {
    if (variableName.equals(var)) {
      return true;
    }
  }

  return false;
}

bool updateVariable(const String& variableName, const String& value) {
  if (variableName == "disabled") {
    disabled = value.toInt();
    EEPROM.write(65, disabled);
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
      lightOffWaitTime = value.toInt();
      EEPROM.write(66, lightOffWaitTime);
      EEPROM.commit();
    }

  } else if (variableName == "lowLightThreshold") {
    if (value.toInt() > 0) {
      lowLightThreshold = value.toInt();
      EEPROM.write(67, lowLightThreshold);
      EEPROM.commit();
    }

  } else if (variableName == "otaMode") {
#ifdef DEBUG
    Serial.println(EEPROM.read(64));
#endif
    EEPROM.write(64, true);
    EEPROM.commit();
#ifdef DEBUG
    Serial.println(EEPROM.read(64));
#endif
    shouldRestart = true;
  } else if (variableName == "shouldRestart") {
    shouldRestart = true;
  } else if (variableName == "otaUrl") {
    otaUrl = value;
    performOTAUpdate();
  } else {
    return false;
  }

  return true;
}

void handleHTTP(ESP8266WebServer& server) {
  server.handleClient();
}
