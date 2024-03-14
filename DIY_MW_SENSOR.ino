#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

#include "MQTT.h"

const char* defaultSsid = "yugg";
const char* defaultPassword = "98103080";

ESP8266WebServer server(80);

String ssid;
String password;
int relayPin;
int microPin;
int ldrPin;
int pirPin;
int light;

int motion = 0;
int microMotion = 0;
int pirMotion = 0;
int relayState = 0;
int pval = -1;
bool alarm = false;
bool hotspotActive = false;
bool pirActive = true;
unsigned long lastMotionTime = 0;
unsigned long lastPrintTime = 0;
unsigned long waitTime = 1 * 60 * 1000;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  initializeDevices();
  Serial.println("Initial motion state:" + String(motion));

  if (!connectToWifi()) {
    setupHotspot();
  } else {
    // Initialize MQTT connection using MqttManager
    MQTT::initialize();
  }

  startHttpServer();
  setupOTA();
}

void loop() {
  if (shouldRestart) {
    restartESP();
  }
  reconnectToWifi();
  server.handleClient();
  ArduinoOTA.handle();

  if (WiFi.status() == WL_CONNECTED) {
    MQTT::handleMQTT();
  }

  delay(200);
  readLDRSensor();

  if (light == LOW) {
    Serial.println("Light Low");
    readMotion();
    updateRelayState();
  } else {
    Serial.println("Light High");
    if (digitalRead(relayPin) == HIGH) {
      Serial.println("Light On");
      digitalWrite(relayPin, LOW);
      relayState = LOW;
      Serial.println("");
      Serial.println("Light Available: " + String(light) + "\n" + "Enough light available, no need to turn on lights");
    } else {
      Serial.println("Light Off");
    }
  }
}

void initializeDevices() {
  EEPROM.begin(182);

  relayPin = EEPROM.read(0);
  microPin = EEPROM.read(2);
  ldrPin = 5;
  if (pirActive) {
    pirPin = 14;
  }

  // Check if pins are valid
  if (microPin < 0 || microPin > 13 || relayPin < 0 || relayPin > 13) {
    // Use default pins
    microPin = 4;  // Default digital pin D4
    relayPin = 13;
    savePinConfig();
  }

  pinMode(relayPin, OUTPUT);
  pinMode(microPin, INPUT);
  pinMode(ldrPin, INPUT);
  if (pirActive) {
    pinMode(pirPin, INPUT);
  }
  readMotion();
  readLDRSensor();
  updateRelayState();

  // Read SSID and password from EEPROM
  ssid = readStringFromEEPROM(4, 36);
  password = readStringFromEEPROM(36, 68);

  if (ssid.length() == 0 || password.length() == 0) {
    ssid = defaultSsid;
    password = defaultPassword;
    saveWifiCredentials(defaultSsid, defaultPassword);
  }
}

String readStringFromEEPROM(int start, int end) {
  String value = "";
  for (int i = start; i < end; ++i) {
    value += char(EEPROM.read(i));
  }
  return value;
}

void saveWifiCredentials(const char* ssid, const char* password) {
  char ssidCharArray[32];
  char passwordCharArray[32];
  strncpy(ssidCharArray, ssid, sizeof(ssidCharArray));
  strncpy(passwordCharArray, password, sizeof(passwordCharArray));
  EEPROM.put(4, ssidCharArray);
  EEPROM.put(36, passwordCharArray);
  EEPROM.commit();
}

void savePinConfig() {
  EEPROM.write(0, relayPin);
  EEPROM.write(2, microPin);
  EEPROM.commit();
}

bool connectToWifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.print(ssid);
  WiFi.begin(ssid.c_str(), password.c_str());
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 100) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi: ");
    Serial.print(defaultSsid);
    WiFi.begin(defaultSsid, defaultPassword);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi.");
    return false;
  }
}

void reconnectToWifi() {
  // Check if WiFi is disconnected
  if (!hotspotActive && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    connectToWifi();  // Implement your own function to reconnect to WiFi
  }
}

void setupHotspot() {
  Serial.println("Starting hotspot...");
  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP("ESP8266AP-MoSen");
  hotspotActive = true;

  Serial.println("Hotspot Name: " + String("ESP8266AP-MoSen"));
  Serial.println("Hotspot IP address: " + WiFi.softAPIP().toString());
}

void startHttpServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleSensorStatus);
  server.on("/wifi", HTTP_GET, handleWifiSettings);
  server.on("/saveWifi", HTTP_POST, handleSaveWifi);
  server.on("/relay", HTTP_GET, handleRelayState);
  server.on("/setRelayPin", HTTP_GET, handleSetRelayPin);
  server.on("/setSensorPin", HTTP_GET, handleSetSensorPin);
  if (pirActive) {
    server.on("/setPirSensorPin", HTTP_GET, handleSetPIRSensorPin);
  }
  server.on("/setWaitTime", HTTP_GET, handleSetWaitTime);
  server.on("/restart", HTTP_GET, handleRestart);
  // Route for handling OTA update
  server.on("/ota", HTTP_POST, handleOTAUpdate);
  server.onNotFound(handleNotFound);
  server.begin();
}

void handleRoot() {
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
      z-index: 1; /* Ensure light-status div appears on top */
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
            const data = await response.json(); // Parse JSON response
            updateIcons(data); // Call updateIcons function with JSON data
          } catch (error) {
            console.error('There was a problem with your fetch operation:', error);
          }
        }

        // Function to update icons based on sensor pin states
        function updateIcons(data) {
          const lightState = data["alarm_status"];
          const lightStateElement = document.getElementById("light-status");
          if(lightStateElement){
            if(data["last_motion_time"]){
              lightStateElement.textContent = data["last_motion_time"];
            }
            if(lightState){
              lightStateElement.classList.add('on'); // Add 'on' class if pin state is true
              lightStateElement.classList.remove('off'); // Remove 'off' class
            } else {
              lightStateElement.classList.add('off'); // Add 'off' class if pin state is false
              lightStateElement.classList.remove('on'); // Remove 'on' class
            }
          }

          // Iterate over sensor data and update icon classes
          for (const key in data) {
            const sensorElement = document.getElementById(key);
            if (sensorElement) {
              const sensorStateKey = key + '_state';
              const sensorValElement = document.querySelector(`#${key} > div > span`); // Target the span inside the div with sensorId
              if(data[sensorStateKey]){
                sensorElement.classList.add('on'); // Add 'on' class if pin state is true
                sensorElement.classList.remove('off'); // Remove 'off' class
              } else {
                sensorElement.classList.add('off'); // Add 'off' class if pin state is false
                sensorElement.classList.remove('on'); // Remove 'on' class
              }
              if (sensorValElement) {
                sensorValElement.textContent = data[key]; // Set sensor pin value in respective span
              }
            }
          }
        }

        // Call fetchContent function every 0.5 seconds
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
          <div class="sensor-icon" id="pir_sensor_pin">
            <div class="icon pir"></div>
            <div>PIR: <span></span></div>
          </div>
          <div class="sensor-icon" id="ldr_sensor_pin">
            <div class="icon ldr"></div>
            <div>LDR: <span></span></div>
          </div>
          <!--
          -->
        </div>
      </div>
    </body>
    </html>
  )";

  server.send(200, "text/html", html);
}

void handleSensorStatus() {
  // readMotion();
  // readLDRSensor();

  // Create a JSON object to store sensor status
  StaticJsonDocument<200> doc;
  doc["microwave_sensor_pin"] = microPin;
  doc["microwave_sensor_pin_state"] = microMotion;
  if (pirActive) {
    doc["pir_sensor_pin"] = pirPin;
    doc["pir_sensor_pin_state"] = pirMotion;
  }
  doc["ldr_sensor_pin"] = ldrPin;
  doc["ldr_sensor_pin_state"] = light;
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

  server.send(200, "text/plain", "WiFi credentials saved. Restarting device...");
  restartESP();
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

void handleSetRelayPin() {
  if (server.hasArg("pin")) {
    int pin = server.arg("pin").toInt();
    relayPin = pin;
    savePinConfig();
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    relayState = LOW;
    server.send(200, "text/plain", "Relay pin set to: " + String(pin));
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void handleSetSensorPin() {
  if (server.hasArg("pin")) {
    int pin = server.arg("pin").toInt();
    microPin = pin;
    savePinConfig();
    pinMode(microPin, INPUT);
    server.send(200, "text/plain", "Sensor pin set to: " + String(microPin));
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void handleSetPIRSensorPin() {
  if (server.hasArg("pin")) {
    int pin = server.arg("pin").toInt();
    pirPin = pin;
    // savePinConfig();
    pinMode(pirPin, INPUT);
    server.send(200, "text/plain", "PIR Sensor pin set to: " + String(pirPin));
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

void readMotion() {
  readMotionSensor();
  if (pirActive) {
    readPIRSensor();
    motion = (microMotion || pirMotion) ? 1 : 0;
  } else {
    motion = microMotion ? 1 : 0;
  }
}

void readMotionSensor() {
  microMotion = digitalRead(microPin);
}

void readPIRSensor() {
  pirMotion = digitalRead(pirPin);
}

void readLDRSensor() {
  light = digitalRead(ldrPin);
}

void updateRelayState() {
  Serial.println("Motion: " + String(motion));
  if (motion == HIGH) {
    lastMotionTime = millis();
  }
  if (pval != motion) {
    Serial.println("a");
    if (motion == HIGH) {
      Serial.println("b");
      if (millis() - lastPrintTime > 500) {
        Serial.println("");
        Serial.println("Motion Detected!");
        lastPrintTime = millis();  // Update lastPrintTime when printing
      }
      if (!alarm) {
        Serial.println("c");
        Serial.println("");
        Serial.println("Alarm turned on.");
        digitalWrite(relayPin, HIGH);
        relayState = HIGH;
        alarm = true;
      }
    } else {
      Serial.println("d");
      if (millis() - lastPrintTime > 500) {
        Serial.println("");
        Serial.println("No Motion Detected!");
        lastPrintTime = millis();  // Update lastPrintTime when printing
      }
    }
    pval = motion;
  } else {
    Serial.println("e");
    if (millis() - lastMotionTime >= waitTime && motion == LOW && alarm == true) {
      Serial.println("");
      Serial.println("Alarm turned off.");
      digitalWrite(relayPin, LOW);
      relayState = LOW;
      alarm = false;
    }
    if (millis() - lastPrintTime > 5000) {
      Serial.print(".");
      lastPrintTime = millis();  // Update lastPrintTime when printing
    }
  }
}

// Handler function for OTA update route
void handleOTAUpdate() {
  if (server.hasArg("url")) {
    String url = server.arg("url");
    performOTAUpdate(url);
    server.send(200, "text/plain", "OTA update started from URL: " + url);
  } else {
    server.send(400, "text/plain", "OTA URL not provided");
  }
}

void handleRestart() {
  server.send(404, "text/plain", "System Restarting.");
  restartESP();
}

void restartESP() {
  Serial.println("Restarting device!...");
  // Add a small delay for the message to be printed
  delay(2000);
  ESP.restart();
}

void handleNotFound() {
  String message = "HTTP routes are not available on hotspot mode.";
  server.send(404, "text/plain", message);
}

void setupOTA() {
  // Set the OTA hostname (optional)
  ArduinoOTA.setHostname("ESP8266-MoSenOTA");

  // Set the OTA password (optional)
  ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    restartESP();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();
  Serial.println("OTA Initialized");
}

// Function to perform OTA update using HTTP URL
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
