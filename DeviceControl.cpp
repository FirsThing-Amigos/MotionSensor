#include "core_esp8266_features.h"
#include "DeviceControl.h"
#include "Variables.h"
#include "MQTT.h"

const String chipId = String(ESP.getChipId());
String deviceID;

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

bool relayState = LOW;

int pval = -1;
int ldrVal = -1;
int lowLightThreshold = 230;
int lightVariable = 20;

unsigned long lastMotionTime = 0;
unsigned long lightOffWaitTime = 180;  // 60 Seconds
unsigned long countDownLightOff = 0;
unsigned long countDownDayLight = 0;

String getDeviceID() {
  if (deviceID.length() == 0) {
    String tempDeviceMacAddress = deviceMacAddress;
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
#ifdef PIR
  pinMode(pirPin, INPUT);
#endif
  if (!isOtaMode) {
    setLightVariable();
    readSensors();
    updateRelay();
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
  relayState = digitalRead(relayPin);
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

void updateRelay() {
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
        pushDeviceState(false);
        delay(3000);
      }
      countDownLightOff = millis();
    } else if ((countDownLightOff + 1000 * lightOffWaitTime) < millis() && microMotion == 0) {
      digitalWrite(relayPin, LOW);
      pushDeviceState(false);
    }
  } else if (relayState == HIGH) {  // High light level
    // Scenario 2
    if (countDownDayLight == 0) {
      countDownDayLight = millis();
    }
    if ((countDownDayLight + 60000) < millis()) {
      digitalWrite(relayPin, LOW);  // Deactivate relay
      pushDeviceState(false);
    }
  }
}

String getDeviceStatus() {
  String response;

  response += "{";
  response += "\"microwave_sensor_pin\":" + String(microPin) + ",";
  response += "\"microwave_sensor_pin_state\":" + String(microMotion) + ",";
#ifdef PIR
  response += "\"pir_sensor_pin\":" + String(pirPin) + ",";
  response += "\"pir_sensor_pin_state\":" + String(pirMotion) + ",";
#endif
  response += "\"ldr_sensor_pin\":" + String(ldrPin) + ",";
  if (ldrPin == 17) {
    response += "\"ldr_sensor_pin_val\":" + String(ldrVal) + ",";
  }
  response += "\"ldr_sensor_pin_state\":" + String(light) + ",";
  response += "\"relay_pin\":" + String(relayPin) + ",";
  response += "\"relay_pin_state\":" + String(digitalRead(relayPin)) + ",";
  response += "\"last_motion_time\":" + String(millis() - lastMotionTime) + ",";
  response += "\"count_down_light_off\":" + String(millis() - countDownLightOff) + ",";
  response += "\"count_down_day_light\":" + String(countDownDayLight == 0 ? countDownDayLight : (millis() - countDownDayLight)) + ",";
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