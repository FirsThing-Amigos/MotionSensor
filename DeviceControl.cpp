#include <EEPROM.h>
#include "DeviceControl.h"
#include "Variables.h"

int ldrPin = 5;  // 5 For Digital LDR And 17 For Analog
int microPin = 4;
int relayPin = 13;
int pirPin = 14;  // Uncomment this line if PIR is connected/available

int ldrState = -1;
int microMotion = -1;
int pirMotion = -1;
int motion = -1;
int relayState = -1;
int pval = -1;
int ldrVal = -1;

bool alarm = false;
unsigned long lastMotionTime = 0;
unsigned long lastPrintTime = 0;
unsigned long waitTime = 60000;

void initializeDevices() {
  bool otaMode = (EEPROM.read(64) == 1);  // Read and check if the value in EEPROM indicates OTA mode
  if (!otaMode) {
    pinMode(relayPin, OUTPUT);
    pinMode(microPin, INPUT);
    pinMode(ldrPin, INPUT);
    pinMode(pirPin, INPUT);  // Uncomment this line if PIR is connected/available
    readSensors();
    updateDeviceState();
  }
}

void readSensors() {
  readLDRSensor();
  readMicrowaveSensor();
  readPIRSensor();  // Uncomment this line if PIR is connected/available
}

void readLDRSensor() {
  // ldrState = digitalRead(ldrPin);
  ldrVal = analogRead(ldrPin);
  ldrState = ldrVal > 220 ? LOW : HIGH;
}

void readMicrowaveSensor() {
  microMotion = digitalRead(microPin);
}

// Uncomment this line if PIR is connected/available
void readPIRSensor() {
  pirMotion = digitalRead(pirPin);
}

void updateDeviceState() {
  motion = (microMotion || pirMotion) ? 1 : 0;  // Uncomment this line if PIR is connected/available
  // motion = microMotion;  // Uncomment this line if PIR is not connected/available

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
