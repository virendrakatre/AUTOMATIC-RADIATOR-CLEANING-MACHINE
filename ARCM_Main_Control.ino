/**
 * Project: Automatic Radiator Cleaning Machine (ARCM)
 * Board: ESP32
 * Features: Bluetooth Control, Limit Switch Auto-Reverse, LED Status
 * Event: DIPEX-2026 State Level Finalist
 */

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// -------- MOTOR DRIVER PINS (L298N/L293D) --------
const int IN1 = 26;   // Direction Pin 1
const int IN2 = 27;   // Direction Pin 2

// -------- LIMIT SWITCH PINS (Safety Stops) --------
const int LIMIT_UP = 4;     
const int LIMIT_DOWN = 5;   

// -------- INDICATOR LED --------
const int LED_PIN = 2;

// -------- SYSTEM STATE VARIABLES --------
bool systemRunning = false;
int currentDir = 0;   // 0=Stop, 1=CW (Clockwise), 2=CCW (Anti-Clockwise)

unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool ledState = false;

// -------- MOTOR CONTROL FUNCTIONS --------
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  currentDir = 0;
}

void moveCW() {   // Moving Forward/Clockwise
  stopMotor();
  delay(150);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  currentDir = 1;
}

void moveCCW() {  // Moving Backward/Anti-Clockwise
  stopMotor();
  delay(150);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  currentDir = 2;
}

void setup() {
  Serial.begin(115200);

  // Initialize Bluetooth with Device Name
  if (!SerialBT.begin("P_ARCM")) {
    Serial.println("Bluetooth Initialization Failed!");
  }

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(LIMIT_UP, INPUT_PULLUP);
  pinMode(LIMIT_DOWN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  stopMotor();
  digitalWrite(LED_PIN, LOW);

  Serial.println("ARCM System Ready - Waiting for Bluetooth Commands...");
}

void loop() {
  // ---- 1. Bluetooth Command Processing ----
  if (SerialBT.available()) {
    char cmd = SerialBT.read();

    if (cmd == 'S' || cmd == 's') {
      systemRunning = true;
      SerialBT.println("System: STARTED");
    }
    else if (cmd == 'X' || cmd == 'x') {
      systemRunning = false;
      stopMotor();
      digitalWrite(LED_PIN, LOW);
      SerialBT.println("System: EMERGENCY STOP");
    }
    else if (cmd == 'F' || cmd == 'f') {
      if (systemRunning) {
        moveCCW();
        SerialBT.println("Action: Moving Anti-Clockwise");
      }
    }
    else if (cmd == 'R' || cmd == 'r') {
      if (systemRunning) {
        moveCW();
        SerialBT.println("Action: Moving Clockwise");
      }
    }
  }

  // ---- 2. Status LED Blinking (Active State) ----
  if (systemRunning && currentDir != 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  }

  // ---- 3. Auto-Reverse Logic via Limit Switches ----
  if (systemRunning) {
    bool topHit = digitalRead(LIMIT_UP) == LOW;
    bool bottomHit = digitalRead(LIMIT_DOWN) == LOW;

    if (currentDir == 1 && topHit) {
      stopMotor();
      delay(300);
      moveCCW();
      SerialBT.println("Alert: Top Limit Hit - Auto Reversing");
    }

    if (currentDir == 2 && bottomHit) {
      stopMotor();
      delay(300);
      moveCW();
      SerialBT.println("Alert: Bottom Limit Hit - Auto Reversing");
    }
  }
}
