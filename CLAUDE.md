# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# ALS Sensor — USB HID Ambient Light Sensor

USB HID ambient light sensor, works natively on Windows 8+ and Linux 3.7+ without drivers. Fork of [tiny-hid-als](https://github.com/3cky/tiny-hid-als), ported to RP2040.

## Hardware

- **MCU**: VCC-GND 2040 (RP2040, Raspberry Pi Pico-compatible)
- **Sensor**: TCS34725 (I2C RGB/clear, lux calculated via DN40 algorithm)
- **Wiring**: SDA → GP26, SCL → GP27, VIN → 3.3V, GND → GND
- **I2C bus**: GP26/GP27 are on **I2C1** (`Wire1`), not I2C0 (`Wire`). Using `Wire` with these pins will hang in `Wire.begin()`.

## Build Commands

PlatformIO, two environments:

```bash
pio run -e rp2040          # primary target (VCC-GND 2040 + TCS34725)
pio run -e digispark-tiny  # legacy target (ATtiny85 + BH1750)
pio run -e rp2040 -t upload  # build and flash via picotool
```

**Flashing via UF2**: hold BOOTSEL, plug USB, copy `.pio/build/rp2040/firmware.uf2` to the `RPI-RP2` drive.

**PlatformIO path** (not in global PATH): `~/.platformio/penv/Scripts/pio`

The `rp2040` env uses `platform = https://github.com/maxgerhardt/platform-raspberrypi.git` (required for earlephilhower core), `board = pico`, `board_build.core = earlephilhower`, and `-DUSE_TINYUSB`. Libraries `vusb` and `bh1750` are excluded via `lib_ignore`.

## Architecture

`src/main.cpp` uses `#ifdef ARDUINO_ARCH_RP2040` to select between two complete, incompatible USB stacks in a single file:

| Aspect | RP2040 | AVR (ATtiny85) |
|---|---|---|
| USB stack | Adafruit TinyUSB (`Adafruit_USBD_HID`) | V-USB (`usbdrv`) |
| Sensor | `TCS34725Lux` (I2C, DN40 lux) | `BH1750` (I2C) |
| USB callbacks | `usbHid.setReportCallback()` | `usbFunctionSetup` / `usbFunctionWrite` |

**HID callbacks (RP2040)**: Must use `Adafruit_USBD_HID::setReportCallback(get, set)` — do NOT define `tud_hid_get_report_cb` / `tud_hid_set_report_cb` directly, as Adafruit's library already defines them and dispatches to registered callbacks. Defining them causes multiple-definition linker errors.

**HID report descriptor**: Top-level collection MUST be `HID_COLLECTION(Application)` — `Physical` will compile but Windows HidUsb won't create child TLC devices, so `SensorsHidClassDriver` won't load. The descriptor uses macros from `include/HidSensorSpec.h` which intentionally redefine TinyUSB's HID macros (warnings are expected and safe — both produce identical byte values).

**HID report flow**: The device exposes one Feature report (sensor config: connection type, reporting state, power state, sensor state, report interval, range, sensitivity) and one Input report (sensor state, event type, lux value as `uint32_t`). Lux is scaled ×10 (unit exponent 0x0F = -1) so integer 1 = 0.1 lux.

**Periodic reporting**: `timeoutCounter` (uint8_t, wraps at 256) drives the report interval. Each `loop()` iteration delays 2 ms; at overflow the `timeout` flag triggers a sensor read and HID report, giving ~512 ms default cadence. `featureReportBuf.reportInterval` is stored but not used to control the actual cadence.

**`lib/tcs34725/TCS34725Lux`**: Wraps `Adafruit_TCS34725` with DN40 lux calculation. `readLightLevel()` returns `int32_t` lux (0–65535) or `-1` on error, intentionally matching the `BH1750::readLightLevel()` API. Configured with `TCS34725_INTEGRATIONTIME_50MS` and `TCS34725_GAIN_4X`.

**USB identity**: VID `0x16c0`, PID `0x27da`, manufacturer `https://github.com/3cky/tiny-hid-als`, product `Light Sensor`.

## Troubleshooting

- **Windows driver cache**: If changing HID descriptor structure (e.g. collection type), Windows may cache old driver assignment. Changing PID temporarily forces full re-enumeration. After `SensorsHidClassDriver` loads, the original PID can be restored.
- **`picotool` upload fails on Windows**: Install WinUSB driver via Zadig for the RP2040 BOOTSEL device, or use UF2 copy method.
- **IntelliSense errors in VS Code**: `#include` errors for AVR headers (`util/delay.h`, `WProgram.h`) are expected — IntelliSense doesn't know PlatformIO paths. Build works fine.

## Status

Firmware compiles, runs on hardware, and reads lux from the TCS34725 sensor. Windows 10/11 recognizes the device as "Датчик освещённости HID" (HID Ambient Light Sensor) in the Sensor device class. Adaptive brightness works on laptops with built-in displays.
