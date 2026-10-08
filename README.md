# Palma Advanced Cursor Control (PACC)

PACC is a glove based controller inspired by sci-fi movies that lets you control any device using your palm without ever touching a mouse. The idea is to have something similar to how you control things on an Apple Vision Pro, but for any regular computer or device.

## Features

- **Palm cursor control**: Move your hand and palm to control the cursor naturally without needing a desk or a mousepad.
- **Connects over Bluetooth and USB-C**: Works wired over USB-C or wirelessly through Bluetooth.
- **Multi-device support**: Stays connected to multiple devices at once so you can switch between them seamlessly without having to disconnect and reconnect every time.
- **ESP32 powered**: Uses an ESP32 to handle the sensor inputs, tracking, and HID signals.

## How it works

1. Sensors on the glove track hand motion, palm orientation, and gestures.
2. The onboard ESP32 takes that data and translates it into standard mouse movements and clicks.
3. It sends standard HID commands to your computer, phone, or tablet over Bluetooth or USB.
4. You get full control over your screen straight from your hand.

## Author

Designed and built by Harinandan J V for Hack Club's Half Life.
