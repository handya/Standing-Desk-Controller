# Standing Desk Controller (macOS + Arduino)

This repo contains a **macOS test app** and matching **Arduino firmware** to control a custom-built standing desk using serial communication.

[![Watch the video](https://img.youtube.com/vi/RGp0JSYM98Q/hqdefault.jpg)](https://www.youtube.com/watch?v=RGp0JSYM98Q)

[Find out more on YeahNahDIY.com](https://yeahnahdiy.com/blog/this-standing-desk-was-dumb-so-i-upgraded-it)

⚠️ **This project is experimental and not production ready.** The code is rough and intended for testing purposes only.

---

## 🔌 Arduino Firmware

This firmware runs on an Arduino-compatible board to control the physical desk hardware (motor driver, buttons, rotary encoder).

### Features

* Manual buttons for up/down
* Rotary encoder for height tracking
* 3 preset memory buttons
* EEPROM save/restore of height
* Serial interface for remote commands and status
* Homing support
* Movement timeouts

### Pin Mapping

| Function     | Pin |
| ------------ | --- |
| Rotary A     | D3  |
| Rotary B     | D2  |
| Button: Down | D4  |
| Button: Up   | D5  |
| Button: A    | D8  |
| Button: B    | D7  |
| Button: C    | D6  |
| Motor: Down  | D11 |
| Motor: Up    | D12 |

### Serial Protocol (Same as macOS)

* Send `a`, `b`, `c` to go to presets
* Send `s` to stop
* Send a number like `950` to go to that height in mm
* Desk replies with state + height every cycle, e.g.:

  ```
  movingUp
  850
  ```

### Notes

* EEPROM is used to persist height between restarts
* Desk must be **homed** (manually or via `a`/`HOME` button) for presets to work
* Height is calculated using encoder steps with a conversion factor (`heightOffset`)
* All output is in mm

---

## 🛠 Requirements

* Arduino-compatible board (Uno, Nano, etc.)
* Motor driver circuitry
* Rotary encoder (TWO03 latch mode)
* Desk with UP/DOWN inputs
* macOS app built via Xcode
* USB serial connection

---

## 💻 macOS App

### Features

* Control your desk height via a slider
* Three preset position buttons (`a`, `b`, `c`)
* Realtime desk state and height display
* Serial communication via [ORSSerialPort](https://github.com/armadsen/ORSSerialPort)

### Setup

1. Open the project in Xcode.
2. Find your serial port:

   ```swift
   let ports = ORSSerialPortManager.shared().availablePorts
   print(ports)
   ```
3. Update this line in `ViewController.swift` with the correct serial path:

   ```swift
   serialPort = ORSSerialPort(path: "/dev/cu.usbserial-840")!
   ```
4. Build and run.

### Serial Protocol

The macOS app sends the following over serial:

| Action        | Serial Output   |
| ------------- | --------------- |
| Preset 1      | `a\n`           |
| Preset 2      | `b\n`           |
| Preset 3      | `c\n`           |
| Stop Movement | `s\n`           |
| Set Height    | `950\n` (in mm) |

---

## ⚠️ Disclaimer

This is test software. No safety limits or failsafes are implemented. Use at your own risk.
