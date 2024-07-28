#include "DeviceControl.h"
#include "WIFIControl.h"
#include "HTTPRoutes.h"
#include "MQTT.h"
#include "Variables.h"
#include <DHT.h>
#include "UdpMeshControl.h"
#ifdef ESP8266
    #include "core_esp8266_features.h"
#elif defined(ESP32)

#endif
#define DHTTYPE DHT11

DHT dht(tempHumiPin, DHTTYPE);

const auto *internetCheckHost = "www.google.com";

#ifdef ESP8266
    const String chipId = String(ESP.getChipId());
    int ldrPin = 17; // 5 For Digital LDR And 17 For Analog
    int microPin = 4;
    int relayPin = 13;
    int tempHumiPin = 5;
#elif defined(ESP32)
    uint64_t chipId64 = ESP.getEfuseMac();
    const String chipId = String((uint16_t)(chipId64 >> 32)); // Use the upper 32 bits for uniqueness
    int ldrPin = 34; // 5 For Digital LDR And 17 For Analog
    int microPin = 21;
    int relayPin = 23;
    int tempHumiPin = 4;

#endif

String deviceID;



int light = -1;
int microMotion = -1;
int temperature = 0;
int humidity = 0;


bool relayState = LOW;

int condition = -1;
int pval = -1;
int ldrVal = -1;
int lightVariable = 20;

unsigned long lastMotionTime = 0;
unsigned long countDownLightOff = 0;
unsigned long countDownDayLight = 0;

String getDeviceID() {
    if (deviceID.length() == 0) {
        String tempDeviceMacAddress = getDeviceMacAddress();
        tempDeviceMacAddress.replace(":", "");
        deviceID = String(chipId) + "-" + tempDeviceMacAddress;
#ifdef DEBUG
        Serial.println("Device ID: " + String(deviceID));
#endif
    }
    return deviceID;
}

void initDevices() {
    getDeviceID();
    pinMode(relayPin, OUTPUT);
    pinMode(microPin, INPUT);
    pinMode(ldrPin, INPUT);
    pinMode(tempHumiPin, INPUT);
    dht.begin();
    if (!isOtaMode && !disabled) {
        setLightVariable();
        readSensors();
        updateRelay();
    } else if (disabled) {
        digitalWrite(relayPin, HIGH);
    }
}

void readSensors() {
    readLDRSensor();
    readMicrowaveSensor();
    readtemperatureHumidity();

}

void readLDRSensor() {
    // light = digitalRead(ldrPin);
    ldrVal = analogRead(ldrPin);
    light = ldrVal > lowLightThreshold ? LOW : HIGH;
}

void readtemperatureHumidity(){
   float humi = dht.readHumidity();
    float temp = dht.readTemperature();
    if (isnan(humi) || isnan(temp)) {
        // Serial.println("Failed to read from DHT sensor!");
        return;
    }
    humidity = (int)humi;
    temperature = (int)temp;
}

void readMicrowaveSensor() { microMotion = digitalRead(microPin); }

void setLightVariable() {
    int lightOffVal = 0;

    digitalWrite(relayPin, HIGH);
    delay(1000);
    readLDRSensor();
    int lightOnVal = ldrVal;

    digitalWrite(relayPin, LOW);
    delay(1000);
    readLDRSensor();
    lightOffVal = max(lightOffVal, ldrVal);

    digitalWrite(relayPin, HIGH);
    delay(1000);
    readLDRSensor();
    lightOnVal = min(lightOnVal, ldrVal);

    digitalWrite(relayPin, LOW);
    delay(1000);
    readLDRSensor();
    lightOffVal = max(lightOffVal, ldrVal);

    digitalWrite(relayPin, HIGH);
    countDownLightOff = millis();
    delay(1000);
    readLDRSensor();
    lightOnVal = min(lightOnVal, ldrVal);

    lightVariable = max(lightVariable, ((lightOffVal - lightOnVal) + 10));
}

void updateRelay() {

    relayState = digitalRead(relayPin);

    if (microMotion == 1) {
        lastMotionTime = millis();
    }

    if ((relayState == LOW && ldrVal > lowLightThreshold) ||
        (relayState == HIGH && ldrVal > (lowLightThreshold - lightVariable))) { // Low light level
        // Scenario 1
        condition = 1;
        countDownDayLight = 0;

        if (microMotion == 1) {
            condition = 2;
            if (relayState == LOW) {
                condition = 3;
                digitalWrite(relayPin, HIGH);
                delay(3000);
            }
            countDownLightOff = millis();
        } else if ((lightOffWaitTime * 1000 + countDownLightOff) < millis() && microMotion == 0 && relayState == HIGH) {
            condition = 4;
            digitalWrite(relayPin, LOW);
        } else {
            condition = 5;
        }
    } else if (relayState == HIGH) { // High light level
        // Scenario 2
        condition = 6;
        if (countDownDayLight == 0) {
            condition = 7;
            countDownDayLight = millis();
        }
        if ((countDownDayLight + 60000) < millis()) {
            condition = 8;
            digitalWrite(relayPin, LOW); // Deactivate relay
        }
    } else {
        condition = 9;
    }

    if (wifiDisabled == 0 && relayState != digitalRead(relayPin)&& !node) {
        if (isWifiConnected()&& !node) {
            pushDeviceState();
        }
        else if (!node) {
            broadcastDeviceState();
        }
    }
}

String getDeviceStatus() {
    String response;

    response += "{";
    response += R"("device_id":")" + String(deviceID) + "\",";
    response += R"("device_disabled":")" + String(disabled) + "\",";
    response += R"("condition":")" + String(condition) + "\",";
    response += R"("thing_name":")" + String(thingName) + "\",";
    response += R"("mac_address":")" + String(getDeviceMacAddress()) + "\",";
    response += "\"mqtt_connected\":" + String(isMqttConnected()) + ",";
    response += "\"microwave_sensor_pin\":" + String(microPin) + ",";
    response += "\"microwave_sensor_pin_state\":" + String(microMotion) + ",";
    response += "\"ldr_sensor_pin\":" + String(ldrPin) + ",";
    if (ldrPin == 17) {
        response += "\"ldr_sensor_pin_val\":" + String(ldrVal) + ",";
    }
    response += "\"DHT_sensor_pin\":" + String(tempHumiPin) + ",";
    response += "\"Temperature_value\":" + String(temperature) + ",";
    response += "\"Humidity_value\":" + String(humidity) + ",";
    response += "\"Mesh_enabled\":" + String(MeshNetwork) + ",";

    response += "\"ldr_sensor_pin_state\":" + String(light) + ",";
    response += "\"relay_pin\":" + String(relayPin) + ",";
    response += "\"relay_pin_state\":" + String(relayState) + ",";
    response += "\"last_motion_time\":" + String(millis() - lastMotionTime) + ",";
    response += "\"millis\":" + String(millis()) + ",";
    response += "\"light_off_wait_time\":" + String(lightOffWaitTime * 1000) + ",";
    response += "\"count_down_light_off\":" + String(millis() - countDownLightOff) + ",";
    response += "\"count_down_light_off_val\":" + String(countDownLightOff) + ",";
    response += "\"count_down_light_off_elapsed\":" + String(lightOffWaitTime + countDownLightOff < millis()) + ",";
    response += "\"count_down_day_light\":" +
                String(countDownDayLight == 0 ? countDownDayLight : (millis() - countDownDayLight)) + ",";
    response += "\"last_motion_state\":" + String(pval) + ",";
    response += "\"current_motion_state\":" + String(microMotion) + ",";
    response += "\"low_light_threshold\":" + String(lowLightThreshold) + ",";
    response += "\"light_variable\":" + String(lightVariable);
    response += "}";

    return response;
}

void restartESP() {
#ifdef DEBUG
    Serial.println(F("Restarting device!..."));
#endif
    delay(5000);
    ESP.restart();
}

bool checkInternetConnectivity() {
    WiFiClient client;

    // Attempt to connect to a well-known server
    if (client.connect(internetCheckHost, 80)) {
        // If connected, return true (internet is accessible)
        client.stop();
        return true;
    }
    // If connection fails, return false (internet is not accessible)
    client.stop();
    return false;
}