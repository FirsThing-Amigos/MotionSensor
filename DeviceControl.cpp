#include "core_esp8266_features.h"
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
int pval = -1;
int ldrVal = -1;
int lowLightThreshold = 230;
int lightVariable = 20;

unsigned long lastMotionTime = 0;
unsigned long waitTime = 180;  // 60 Seconds
unsigned long countDownLightOff = 0;
unsigned long countDownDayLight = 0;

void initializeDevices() {
  if (!isOtaMode) {
    pinMode(relayPin, OUTPUT);
    pinMode(microPin, INPUT);
    pinMode(ldrPin, INPUT);
#ifdef PIR
    pinMode(pirPin, INPUT);
#endif
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
  light = ldrVal > lowLightThreshold ? LOW : HIGH;
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

void setLightVariable() {
  bool relayState = digitalRead(relayPin);
  int lightOffVal = 0;
  int lightOnVal = 0;
  readLDRSensor();

  if (relayState == HIGH) {
    lightOnVal = ldrVal;
    digitalWrite(relayPin, LOW);
    delay(1000);
    readLDRSensor();
  }

  lightOffVal = ldrVal;
  digitalWrite(relayPin, HIGH);
  delay(1000);
  readLDRSensor();
  if (lightOnVal == 0 || ldrVal < lightOnVal) {
    lightOnVal = ldrVal;
  }
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

  lightVariable = max(lightVariable, lightOffVal - lightOnVal) + 10;
}

void updateDeviceState() {
#ifdef PIR
  microMotion = (microMotion || pirMotion) ? 1 : 0;
#endif

  bool relayState = digitalRead(relayPin);

  if (microMotion == 1) {
    lastMotionTime = millis();
  }

  if ((relayState == LOW && ldrVal > lowLightThreshold) || (relayState == HIGH && ldrVal > (lowLightThreshold - lightVariable))) {  // Low light level
    // Scenario 1
    countDownDayLight = 0;

    if (microMotion == 1) {
      if (relayState == LOW) {
        digitalWrite(relayPin, HIGH);
        delay(3000);
      }
      countDownLightOff = millis();
    } else if ((countDownLightOff + 1000 * waitTime) < millis() && microMotion == 0) {
      digitalWrite(relayPin, LOW);
    }
  } else if (relayState == HIGH) {  // High light level
    // Scenario 2
    if (countDownDayLight == 0) {
      countDownDayLight = millis();
    }
    if ((countDownDayLight + 60000) < millis()) {
      digitalWrite(relayPin, LOW);  // Deactivate relay
    }
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
  doc["last_motion_time"] = millis() - lastMotionTime;
  doc["count_down_light_off"] = millis() - countDownLightOff;
  doc["count_down_day_light"] = countDownDayLight == 0 ? countDownDayLight : (millis() - countDownDayLight);
  doc["last_motion_state"] = pval;
  doc["current_motion_state"] = microMotion;
  doc["low_light_threshold"] = lowLightThreshold;
  doc["light_variable"] = lightVariable;

  String response;
  serializeJson(doc, response);
  return response;
}