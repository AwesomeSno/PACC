/*
 * Palma Advanced Cursor Control (PACC) - Firmware
 * ESP32-S3 Bluetooth HID Mouse with BNO085 IMU & Gesture Flex Tracking
 *
 * Dependencies (install via Arduino Library Manager or PlatformIO):
 *   - ESP32-BLE-Mouse (by T-vK: https://github.com/T-vK/ESP32-BLE-Mouse)
 *   - Adafruit BNO08x (by Adafruit)
 *   - Adafruit BusIO & Adafruit Unified Sensor
 */

#include <Arduino.h>
#include <Wire.h>
#include <BleMouse.h>
#include <Adafruit_BNO08x.h>

// Pin Definitions matching PACC Schematic (kc01 / ESP32-S3-WROOM-1)
#define PIN_I2C_SDA     9
#define PIN_I2C_SCL     8
#define PIN_IMU_INT     10
#define PIN_IMU_RST     11
#define PIN_HAPTIC_MTR  12
#define PIN_FLEX_THUMB  4
#define PIN_FLEX_INDEX  5
#define PIN_STATUS_LED  2

// IMU Cursor Movement & Deadzone Calibration
#define GYRO_DEADZONE      0.035f   // Angular velocity deadzone (rad/s) to filter hand tremor
#define SENSITIVITY_X      18.0f    // Cursor speed factor along X (Yaw)
#define SENSITIVITY_Y      18.0f    // Cursor speed factor along Y (Pitch)

// Flex Sensor Gesture Thresholds
#define PINCH_THRESHOLD    1800     // ADC reading threshold for Thumb + Index pinch
#define TAP_THRESHOLD      2200     // ADC reading threshold for rapid finger tap
#define TAP_COOLDOWN_MS    300      // Minimum debounce delay between tap triggers

// BLE HID Mouse instance
BleMouse bleMouse("PACC Glove", "AwesomeSno", 100);

// BNO085 IMU instance & sensor event container
Adafruit_BNO08x bno08x(PIN_IMU_RST);
sh2_SensorValue_t sensorValue;
bool bnoInitialized = false;

// Gesture tracking states
bool isPinching = false;
bool wasPinching = false;
unsigned long lastTapTime = 0;

void triggerHaptic(int durationMs = 30) {
  digitalWrite(PIN_HAPTIC_MTR, HIGH);
  delay(durationMs);
  digitalWrite(PIN_HAPTIC_MTR, LOW);
}

void configureBnoReports() {
  Serial.println("[PACC] Configuring BNO085 report rates...");
  // Calibrated gyroscope for real-time, low-latency cursor tracking (100 Hz / 10ms report interval)
  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED, 10000)) {
    Serial.println("[PACC] Warning: Could not enable calibrated gyroscope report");
  }
  // Game rotation vector for 3D orientation tracking (50 Hz / 20ms report interval)
  if (!bno08x.enableReport(SH2_GAME_ROTATION_VECTOR, 20000)) {
    Serial.println("[PACC] Warning: Could not enable game rotation vector report");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n==========================================");
  Serial.println("  Palma Advanced Cursor Control (PACC)   ");
  Serial.println("  ESP32-S3 BLE HID Mouse Firmware v1.1   ");
  Serial.println("==========================================");

  pinMode(PIN_HAPTIC_MTR, OUTPUT);
  pinMode(PIN_STATUS_LED, OUTPUT);
  pinMode(PIN_IMU_RST, OUTPUT);
  pinMode(PIN_FLEX_THUMB, INPUT);
  pinMode(PIN_FLEX_INDEX, INPUT);

  digitalWrite(PIN_HAPTIC_MTR, LOW);
  digitalWrite(PIN_STATUS_LED, HIGH);

  // Initialize I2C bus for BNO085
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

  // Hardware reset pulse to guarantee BNO085 clean state
  digitalWrite(PIN_IMU_RST, LOW);
  delay(15);
  digitalWrite(PIN_IMU_RST, HIGH);
  delay(60);

  // Initialize BNO085 IMU (try primary address 0x4A, fallback to 0x4B)
  if (bno08x.begin_I2C(0x4A, &Wire, PIN_IMU_INT)) {
    Serial.println("[PACC] BNO085 detected at I2C address 0x4A");
    bnoInitialized = true;
  } else if (bno08x.begin_I2C(0x4B, &Wire, PIN_IMU_INT)) {
    Serial.println("[PACC] BNO085 detected at alternate I2C address 0x4B");
    bnoInitialized = true;
  } else {
    Serial.println("[PACC] ERROR: Failed to detect BNO085 over I2C. Check wiring and pull-ups!");
  }

  if (bnoInitialized) {
    configureBnoReports();
  }

  Serial.println("[PACC] Initializing Bluetooth HID Mouse...");
  bleMouse.begin();

  // Startup haptic confirmation chirp
  triggerHaptic(50);
}

void loop() {
  if (bleMouse.isConnected()) {
    digitalWrite(PIN_STATUS_LED, HIGH);

    // 1. Process BNO085 IMU data for cursor motion
    int16_t deltaX = 0;
    int16_t deltaY = 0;

    if (bnoInitialized) {
      // Re-enable reports if BNO085 experienced an internal reset
      if (bno08x.wasReset()) {
        Serial.println("[PACC] BNO085 reset detected. Re-enabling reports...");
        configureBnoReports();
      }

      while (bno08x.getSensorEvent(&sensorValue)) {
        if (sensorValue.sensorId == SH2_GYROSCOPE_CALIBRATED) {
          float gyroX = sensorValue.un.gyroscope.x; // pitch angular velocity (rad/s)
          float gyroZ = sensorValue.un.gyroscope.z; // yaw angular velocity (rad/s)

          // Deadzone filtering to prevent cursor drift when holding hand steady
          if (fabs(gyroZ) > GYRO_DEADZONE) {
            deltaX = (int16_t)(-gyroZ * SENSITIVITY_X);
          }
          if (fabs(gyroX) > GYRO_DEADZONE) {
            deltaY = (int16_t)(-gyroX * SENSITIVITY_Y);
          }
        }
      }

      // Move BLE mouse cursor using calculated deltas
      if (deltaX != 0 || deltaY != 0) {
        bleMouse.move(deltaX, deltaY);
      }
    }

    // 2. Read Flex Sensors for Gesture Detection
    int thumbVal = analogRead(PIN_FLEX_THUMB);
    int indexVal = analogRead(PIN_FLEX_INDEX);

    // Gesture 1: Pinch (Thumb + Index pressed together = Left Click)
    isPinching = (thumbVal > PINCH_THRESHOLD && indexVal > PINCH_THRESHOLD);
    if (isPinching && !wasPinching) {
      Serial.println("[PACC] Pinch gesture detected -> Left Click Press");
      bleMouse.press(MOUSE_LEFT);
      triggerHaptic(20);
    } else if (!isPinching && wasPinching) {
      Serial.println("[PACC] Pinch gesture ended -> Left Click Release");
      bleMouse.release(MOUSE_LEFT);
    }
    wasPinching = isPinching;

    // Gesture 2: Tap (Quick Index finger flexion = Right Click)
    if (indexVal > TAP_THRESHOLD && (millis() - lastTapTime > TAP_COOLDOWN_MS)) {
      Serial.println("[PACC] Tap gesture detected -> Right Click");
      bleMouse.click(MOUSE_RIGHT);
      triggerHaptic(30);
      lastTapTime = millis();
    }
  } else {
    // Blink status LED while awaiting Bluetooth pairing
    digitalWrite(PIN_STATUS_LED, (millis() / 500) % 2);
  }

  delay(10);
}
