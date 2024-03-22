#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "HTTPRoutes.h"
#include "Variables.h"

void startHttpServer() {
  Serial.println("Starting HTTP Server...");
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleSensorStatus);
  server.on("/wifi", HTTP_GET, handleWifiSettings);
  server.on("/saveWifi", HTTP_POST, handleSaveWifi);
  server.on("/relay", HTTP_GET, handleRelayState);
  server.on("/setWaitTime", HTTP_GET, handleSetWaitTime);
  server.on("/restart", HTTP_GET, handleRestart);
  server.on("/ota", HTTP_POST, handleOTAUpdate);
  server.on("/enterOTAMode", HTTP_GET, handleEnterOTAMode);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP Server Started!");
}

void handleRoot() {
  Serial.println("Entered handle root route:");
  // Define CSS for styling the icons based on their state
  String css = R"(
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
    /*
    */
    .pir {
      background-color: orange;
    }
    .ldr {
      background-color: yellow;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    </style>
  )";

  // Define the HTML content with icons and pin numbers
  String html = R"(
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
        // Function to fetch content from server and update the response div
        async function fetchContent() {
          try {
            const response = await fetch('/status');
            const data = await response.json();
            updateIcons(data);
          } catch (error) {
            console.error('There was a problem with your fetch operation:', error);
          }
        }

        function updateIcons(data) {
          const lightState = data["alarm_status"];
          const lightStateElement = document.getElementById("light-status");
          if(lightStateElement){
            if(data["last_motion_time"]){
              lightStateElement.textContent = data["last_motion_time"];
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
          <div class="sensor-icon" id="pir_sensor_pin">
            <div class="icon pir"></div>
            <div>PIR: <span></span></div>
          </div>
          <!--
          -->
        </div>
      </div>
    </body>
    </html>
  )";

  // Serial.println(html);
  server.send(200, "text/html", html);
}


void handleSensorStatus() {
  // Create a JSON object to store sensor status
  StaticJsonDocument<200> doc;
  doc["microwave_sensor_pin"] = microPin;
  doc["microwave_sensor_pin_state"] = microMotion;
  doc["pir_sensor_pin"] = pirPin;           // Uncomment this line if PIR is connected/available
  doc["pir_sensor_pin_state"] = pirMotion;  // Uncomment this line if PIR is connected/available
  doc["ldr_sensor_pin"] = ldrPin;
  if (ldrPin == 17) {
    doc["ldr_sensor_pin_val"] = ldrVal;
  }
  doc["ldr_sensor_pin_state"] = ldrState;
  doc["relay_pin"] = relayPin;
  doc["relay_pin_state"] = relayState;
  doc["alarm_status"] = alarm;
  doc["last_motion_time"] = millis() - lastMotionTime;
  doc["last_motion_state"] = pval;
  doc["current_motion_state"] = motion;

  // Serialize the JSON document to a String
  String response;
  serializeJson(doc, response);
  // Serial.println(response);
  server.send(200, "application/json", response);
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


void handleRelayState() {
  if (server.hasArg("state")) {
    int state = server.arg("state").toInt();
    digitalWrite(relayPin, state);
    relayState = state;
    server.send(200, "text/plain", "Relay state set to: " + String(state));
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void handleSetWaitTime() {
  if (server.hasArg("waitTime")) {
    unsigned long newWaitTime = server.arg("waitTime").toInt();
    waitTime = newWaitTime;
    server.send(200, "text/plain", "Wait time set to: " + String(waitTime) + " milliseconds");
  } else {
    server.send(400, "text/plain", "Invalid request: waitTime parameter missing");
  }
}

void handleOTAUpdate() {
  if (server.hasArg("url")) {
    String url = server.arg("url");
    performOTAUpdate(url);
    server.send(200, "text/plain", "OTA update started from URL: " + url);
  } else {
    server.send(400, "text/plain", "OTA URL not provided");
  }
}

void handleEnterOTAMode() {
  EEPROM.write(64, true);  // Write otaMode to EEPROM
  EEPROM.commit();
  shouldRestart = true;
  server.send(200, "text/plain", "Entered OTA mode");
}

void handleRestart() {
  shouldRestart = true;
  server.send(200, "text/plain", "System Restarting.");
}

void handleNotFound() {
  String message = "HTTP routes are not available on hotspot mode.";
  server.send(404, "text/plain", message);
}

void performOTAUpdate(const String& url) {
  // Start the OTA update process
  Serial.println("Starting OTA update from URL: " + url);

  // Create an instance of WiFiClient
  WiFiClient client;

  // Create an instance of HTTPClient
  HTTPClient http;
  http.begin(client, url);  // Specify the URL for the update

  int httpCode = http.GET();  // Start the GET request

  // Check the HTTP response code
  if (httpCode == HTTP_CODE_OK) {
    // Start the update process
    t_httpUpdate_return ret = ESPhttpUpdate.update(http, url);
    // Check the result of the update process
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP update failed, error: %d\n", ESPhttpUpdate.getLastError());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("No updates available");
        break;
      case HTTP_UPDATE_OK:
        Serial.println("OTA update successful");
        break;
    }
  } else {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  // End the HTTPClient
  http.end();
}

void handleHTTPClients(ESP8266WebServer& server) {
  server.handleClient();
}

void restartESP() {
  Serial.println("Restarting device!...");
  // Add a small delay for the message to be printed
  delay(5000);
  ESP.restart();
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