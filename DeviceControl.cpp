#include "core_esp8266_features.h"
#include "DeviceControl.h"
#include "HTTPRoutes.h"
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

int condition = -1;
int pval = -1;
int ldrVal = -1;
int lowLightThreshold = 230;
int lightVariable = 20;

unsigned long lastMotionTime = 0;
unsigned long lightOffWaitTime = 120;  // 60 Seconds
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
  int lightOffVal = 0;
  int lightOnVal = 0;

  digitalWrite(relayPin, HIGH);
  delay(1000);
  readLDRSensor();
  lightOnVal = ldrVal;

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

  lightVariable = max(lightVariable, lightOffVal - lightOnVal) + 10;
}

void updateRelay() {
#ifdef PIR
  microMotion = (microMotion || pirMotion) ? 1 : 0;
#endif

  relayState = digitalRead(relayPin);

  if (microMotion == 1) {
    lastMotionTime = millis();
  }

  if ((relayState == LOW && ldrVal > lowLightThreshold) || (relayState == HIGH && ldrVal > (lowLightThreshold - lightVariable))) {  // Low light level
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
  } else if (relayState == HIGH) {  // High light level
    // Scenario 2
    condition = 6;
    if (countDownDayLight == 0) {
      condition = 7;
      countDownDayLight = millis();
    }
    if ((countDownDayLight + 60000) < millis()) {
      condition = 8;
      digitalWrite(relayPin, LOW);  // Deactivate relay
    }
  } else {
    condition = 9;
  }

  if (relayState != digitalRead(relayPin)) {
    pushDeviceState(0);
  }
}

String getDeviceStatus() {
  String response;

  response += "{";
  response += "\"device_id\":\"" + String(deviceID) + "\",";
  response += "\"condition\":\"" + String(condition) + "\",";
  response += "\"thing_name\":\"" + String(thingName) + "\",";
  response += "\"mac_address\":\"" + String(deviceMacAddress) + "\",";
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
  response += "\"relay_pin_state\":" + String(relayState) + ",";
  response += "\"last_motion_time\":" + String(millis() - lastMotionTime) + ",";
  response += "\"millis\":" + String(millis()) + ",";
  response += "\"light_off_wait_time\":" + String(lightOffWaitTime) + ",";
  response += "\"count_down_light_off\":" + String(millis() - countDownLightOff) + ",";
  response += "\"count_down_light_off_val\":" + String(countDownLightOff) + ",";
  response += "\"count_down_light_off_elapsed\":" + String(lightOffWaitTime + countDownLightOff < millis()) + ",";
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