#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "MQTT.h"

const char* defaultSsid = "yugg";
const char* defaultPassword = "98103080";

String ssid;
String password;

ESP8266WebServer server(80);

int ldrPin = 17;
int microPin = 4;
int relayPin = 13;
// int pirPin = 14;  // Uncomment this line if PIR is connected/available

int ldrState = -1;
int microMotion = -1;
int pirMotion = -1;
int motion = -1;
int relayState = -1;
int pval = -1;
int ldrVal = -1;

bool alarm = false;
bool hotspotActive = false;
unsigned long lastMotionTime = 0;
unsigned long lastPrintTime = 0;
unsigned long waitTime = 60000;
int maxAttempts = 100;

void setup() {
  Serial.begin(115200);
  Serial.println("");
  initializeDevices();
  initializeConnections();
  initializeServers();
}

void loop() {
  if (shouldRestart) {
    restartESP();
  }
  handleServers();
  readSensors();
  updateDeviceState();
  delay(500);
}

void initializeDevices() {
  EEPROM.begin(128);
  ssid = readStringFromEEPROM(0, 32);
  password = readStringFromEEPROM(32, 64);
  pinMode(relayPin, OUTPUT);
  pinMode(microPin, INPUT);
  pinMode(ldrPin, INPUT);
#ifdef pirPin
  pinMode(pirPin, INPUT);
#endif
  readSensors();
  updateDeviceState();
}

String readStringFromEEPROM(int start, int end) {
  String value = "";
  for (int i = start; i < end; ++i) {
    value += char(EEPROM.read(i));
  }
  return value;
}

void readSensors() {
  readLDRSensor();
  readMicrowaveSensor();
#ifdef pirPin
  readPIRSensor();
#endif
}

void readLDRSensor() {
  // ldrState = digitalRead(ldrPin);
  ldrVal = analogRead(ldrPin);
  ldrState = ldrVal > 800 ? HIGH : LOW;
}

void readMicrowaveSensor() {
  microMotion = digitalRead(microPin);
}

#ifdef pirPin
void readPIRSensor() {
  pirMotion = digitalRead(pirPin);
}
#endif

void updateDeviceState() {
#ifdef pirPin
  motion = (microMotion || pirMotion) ? 1 : 0;
#else
  motion = microMotion;
#endif

  if (motion == HIGH) {
    lastMotionTime = millis();
    if (!alarm) {
      Serial.println("");
      Serial.println("Alarm turned on.");
      digitalWrite(relayPin, HIGH);
      relayState = HIGH;
      alarm = true;
    } else if (relayState == LOW) {
      digitalWrite(relayPin, HIGH);
      relayState = HIGH;
    }
  } else {
    if (millis() - lastMotionTime >= waitTime && alarm) {
      Serial.println("");
      Serial.println("Alarm turned off.");
      digitalWrite(relayPin, LOW);
      relayState = LOW;
      alarm = false;
    }
  }
}

void initializeConnections() {
  connectToWifi();
  if (isWifiConnected()) {
    MQTT::initialize();
  } else {
    setupHotspot();
  }
}

void connectToWifi() {
  if (ssid.length() == 0 || password.length() == 0) {
    Serial.println("SSID or Password is Missing");
  } else {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }

  if (!isWifiConnected()) {
    Serial.print("Fallback to default WiFi: ");
    Serial.println(defaultSsid);
    WiFi.begin(defaultSsid, defaultPassword);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }

  if (isWifiConnected()) {
    Serial.println("");
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi.");
  }
}


bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void setupHotspot() {
  Serial.println("Starting hotspot...");
  WiFi.mode(WIFI_AP);
  IPAddress ip(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP("ESP8266AP-MoSen");
  hotspotActive = true;
  Serial.println("Hotspot Name: ESP8266AP-MoSen");
  Serial.println("Hotspot IP address: " + WiFi.softAPIP().toString());
}

void initializeServers() {
  startHttpServer();
  setupOTA();
}

void startHttpServer() {
  Serial.println("Starting HTTP Server...");
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleSensorStatus);
  server.on("/wifi", HTTP_GET, handleWifiSettings);
  server.on("/saveWifi", HTTP_POST, handleSaveWifi);
  server.on("/relay", HTTP_GET, handleRelayState);
  server.on("/setWaitTime", HTTP_GET, handleSetWaitTime);
  server.on("/restart", HTTP_GET, handleRestart);
  // Route for handling OTA update
  server.on("/ota", HTTP_POST, handleOTAUpdate);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP Server Started!");
}

void setupOTA() {
  Serial.println("Initializing OTA...");
  ArduinoOTA.setHostname("ESP8266-MoSenOTA");
  ArduinoOTA.setPassword("admin");
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA Initialized");
}

void handleServers() {
  if (!isWifiConnected() && !hotspotActive) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    connectToWifi();
  }
  server.handleClient();
  ArduinoOTA.handle();

  if (isWifiConnected()) {
    MQTT::handleMQTT();
  }
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

  // Serial.println(html);
  server.send(200, "text/html", html);
}

void handleSensorStatus() {
  // Create a JSON object to store sensor status
  StaticJsonDocument<200> doc;
  doc["microwave_sensor_pin"] = microPin;
  doc["microwave_sensor_pin_state"] = microMotion;
#ifdef pirPin
  doc["pir_sensor_pin"] = pirPin;
  doc["pir_sensor_pin_state"] = pirMotion;
#endif
  doc["ldr_sensor_pin"] = ldrPin;
  if (ldrPin == 17) {
    doc["ldr_sensor_val"] = ldrVal;
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

void handleRestart() {
  server.send(404, "text/plain", "System Restarting.");
  restartESP();
}

void restartESP() {
  Serial.println("Restarting device!...");
  // Add a small delay for the message to be printed
  delay(5000);
  ESP.restart();
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
