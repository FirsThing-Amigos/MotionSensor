#include <ArduinoJson.h>
#include "DeviceControl.h"
#include "Variables.h"

int ldrPin = 17;  // 5 For Digital LDR And 17 For Analog
int microPin = 4;
int relayPin = 13;
#ifdef PIR
int pirPin = 5;  // 14 or 5 Pin Number Uncomment this line if PIR is connected/available
#endif

int light = -1;
int microMotion = -1;
#ifdef PIR
int pirMotion = -1;
#endif
int motion = -1;
int pval = -1;
int ldrVal = -1;  // Low Light 116, High Light < 85
int lightOnThreshold = 110;
int lightOffThreshold = lightOnThreshold - 25;

bool alarm = false;
unsigned long lastMotionTime = 0;
unsigned long lastPrintTime = 0;
unsigned long waitTime = 60000;

void initializeDevices() {
  if (!isOtaMode) {
    pinMode(relayPin, OUTPUT);
    pinMode(microPin, INPUT);
    pinMode(ldrPin, INPUT);
#ifdef PIR
    pinMode(pirPin, INPUT);
#endif
    readSensors();
    updateDeviceState();
  }
}

void readSensors() {
  readLDRSensor();
  readMicrowaveSensor();
#ifdef PIR
  readPIRSensor();
#endif
}

void readLDRSensor() {
  // light = digitalRead(ldrPin);
  ldrVal = analogRead(ldrPin);
  light = ldrVal > lightOnThreshold ? LOW : HIGH;
}

void readMicrowaveSensor() {
  microMotion = digitalRead(microPin);
}

// Uncomment this line if PIR is connected/available
#ifdef PIR
void readPIRSensor() {
  pirMotion = digitalRead(pirPin);
}
#endif

void updateDeviceState() {
#ifdef PIR
  motion = (microMotion || pirMotion) ? 1 : 0;  // Uncomment this line if PIR is connected/available
#else
  motion = microMotion;  // Uncomment this line if PIR is not connected/available
#endif

  if (motion == HIGH) {
    lastMotionTime = millis();
  }

  // ldrVal : Low Light 116, High Light < 85
  if (ldrVal > lightOnThreshold) {

    if (motion == HIGH && !alarm) {
#ifndef DEBUG
      Serial.println(F(""));
      Serial.println(F("Alarm turned on."));
#endif
      digitalWrite(relayPin, HIGH);
      alarm = true;
      delay(2000);
      int currentLdrVal = analogRead(ldrPin);
      // Check if the value of analogRead(ldrPin) is within ±10 of ldrVal
      if (abs(currentLdrVal - ldrVal) > 10) {
        lightOffThreshold = currentLdrVal - 30;
      }
    } else {
      if (millis() - lastMotionTime >= waitTime && alarm) {
#ifndef DEBUG
        Serial.println(F(""));
        Serial.println(F("Alarm turned off."));
#endif
        digitalWrite(relayPin, LOW);
        alarm = false;
      }
    }
  } else if (digitalRead(relayPin) == HIGH && ldrVal < lightOffThreshold) {
    digitalWrite(relayPin, LOW);
    alarm = false;
  }
}

String getDeviceStatus() {
  StaticJsonDocument<200> doc;
  doc["microwave_sensor_pin"] = microPin;
  doc["microwave_sensor_pin_state"] = microMotion;
#ifdef PIR
  doc["pir_sensor_pin"] = pirPin;           // Uncomment this line if PIR is connected/available
  doc["pir_sensor_pin_state"] = pirMotion;  // Uncomment this line if PIR is connected/available
#endif
  doc["ldr_sensor_pin"] = ldrPin;
  if (ldrPin == 17) {
    doc["ldr_sensor_pin_val"] = ldrVal;
  }
  doc["ldr_sensor_pin_state"] = light;
  doc["relay_pin"] = relayPin;
  doc["relay_pin_state"] = digitalRead(relayPin);
  doc["alarm_status"] = alarm;
  doc["last_motion_time"] = millis() - lastMotionTime;
  doc["last_motion_state"] = pval;
  doc["current_motion_state"] = motion;
  doc["light_on_threshold"] = lightOnThreshold;
  doc["light_off_threshold"] = lightOffThreshold;

  String response;
  serializeJson(doc, response);
  return response;
}