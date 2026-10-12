/*
 * Palma Advanced Cursor Control (PACC) - Firmware
 * ESP32-S3 Bluetooth HID Mouse with BNO085 IMU & Gesture Flex Tracking
 */

#include <Wire.h>
#include <BleMouse.h>

// Pin Definitions matching PACC Schematic (kc01)
#define PIN_I2C_SDA     9
#define PIN_I2C_SCL     8
#define PIN_IMU_INT     10
#define PIN_IMU_RST     11
#define PIN_HAPTIC_MTR  12
#define PIN_FLEX_THUMB  4
#define PIN_FLEX_INDEX  5
#define PIN_STATUS_LED  2

// Gesture Thresholds
#define PINCH_THRESHOLD 1800
#define TAP_THRESHOLD   2200
#define SENSITIVITY     0.15f

BleMouse bleMouse("PACC Glove", "AwesomeSno", 100);

// Tracking states
bool isPinching = false;
bool wasPinching = false;
unsigned long lastTapTime = 0;

void triggerHaptic(int durationMs = 30) {
  digitalWrite(PIN_HAPTIC_MTR, HIGH);
  delay(durationMs);
  digitalWrite(PIN_HAPTIC_MTR, LOW);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_HAPTIC_MTR, OUTPUT);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_IMU_RST, OUTPUT);
  
  digitalWrite(PIN_HAPTIC_MTR, LOW);
  digitalWrite(PIN_STATUS_LED, HIGH);

  // Initialize I2C for BNO085
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  
  // Hardware reset pulse for BNO085
  digitalWrite(PIN_IMU_RST, LOW);
  delay(10);
  digitalWrite(PIN_IMU_RST, HIGH);
  delay(50);

  Serial.println("[PACC] Starting Bluetooth HID Mouse...");
  bleMouse.begin();
  
  // Quick startup haptic chirp
  triggerHaptic(50);
}

void loop() {
  if (bleMouse.isConnected()) {
    digitalWrite(PIN_STATUS_LED, HIGH);

    // Read Flex Sensor analog inputs
    int thumbVal = analogRead(PIN_FLEX_THUMB);
    int indexVal = analogRead(PIN_FLEX_INDEX);

    // Dummy/Sample IMU orientation delta calculation (Pitch/Roll to cursor X/Y)
    // Replace with BNO085 rotation vector reading
    int16_t deltaX = 0;
    int16_t deltaY = 0;

    // Read simulated or I2C sensor data here
    // bleMouse.move(deltaX, deltaY);

    // 1. Pinch Detection (Thumb + Index finger pinch = Left Click)
    isPinching = (thumbVal > PINCH_THRESHOLD && indexVal > PINCH_THRESHOLD);
    if (isPinching && !wasPinching) {
      Serial.println("[PACC] Pinch detected -> Left Click");
      bleMouse.press(MOUSE_LEFT);
      triggerHaptic(20);
    } else if (!isPinching && wasPinching) {
      bleMouse.release(MOUSE_LEFT);
    }
    wasPinching = isPinching;

    // 2. Tap Detection (Quick index finger flex = Right Click)
    if (indexVal > TAP_THRESHOLD && (millis() - lastTapTime > 300)) {
      Serial.println("[PACC] Tap detected -> Right Click");
      bleMouse.click(MOUSE_RIGHT);
      triggerHaptic(30);
      lastTapTime = millis();
    }
  } else {
    // Blink status LED while waiting for BLE connection
    digitalWrite(PIN_STATUS_LED, (millis() / 500) % 2);
  }

  delay(10);
}
