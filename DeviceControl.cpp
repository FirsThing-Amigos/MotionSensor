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
int ldrVal = -1;  // Low Light 116, High Light < 85
int lowLightThreshold = 230;
int highLightThreshold = 200;  // Threshold for high light level

unsigned long lastMotionTime = 0;
unsigned long waitTime = 60;       // 60 Seconds
unsigned long coolOffPeriod = 10;  // 10 Seconds
unsigned long countDownLightOff = 0;
unsigned long coolOffCountDown = 0;

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

void updateDeviceState() {
#ifdef PIR
  microMotion = (microMotion || pirMotion) ? 1 : 0;  // Uncomment this line if PIR is connected/available
#endif
  if (microMotion == 1) {
    lastMotionTime = millis();
  }
  // ldrVal : Low Light 116, High Light < 85
  if (ldrVal > lowLightThreshold) {  // Low light level
    // Scenario 1
    if (microMotion == 1) {
      if (countDownLightOff == 0) {
        digitalWrite(relayPin, HIGH);  // Activate relay
        coolOffCountDown = millis();
      }
      countDownLightOff = millis();
    } else if ((countDownLightOff + 1000 * waitTime) < millis() && microMotion == 0) {
      digitalWrite(relayPin, LOW);  // Deactivate relay
      countDownLightOff = 0;
    }

  } else if ((coolOffCountDown + 1000 * coolOffPeriod) < millis()) {
    // Scenario 2
    if ((countDownLightOff + 1000 * waitTime) < millis() && ldrVal < highLightThreshold) {
      digitalWrite(relayPin, LOW);  // Deactivate relay
      countDownLightOff = 0;
    }
  } else if (ldrVal < highLightThreshold) {
    highLightThreshold = ldrVal - 10;
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
  doc["last_motion_state"] = pval;
  doc["current_motion_state"] = microMotion;
  doc["low_light_threshold"] = lowLightThreshold;
  doc["high_light_threshold"] = highLightThreshold;

  String response;
  serializeJson(doc, response);
  return response;
}