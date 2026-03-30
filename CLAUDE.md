# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# ALS Sensor — USB HID Ambient Light Sensor

USB HID ambient light sensor with color temperature support, works natively on Windows 8+ and Linux 3.7+ without drivers. Based on [tiny-hid-als](https://github.com/3cky/tiny-hid-als), ported to RP2040.

Desktop adaptive brightness/color utility: [als-brightness](https://github.com/igor-blag/als-brightness) (separate repo).

## Hardware

- **MCU**: VCC-GND 2040 (RP2040, Raspberry Pi Pico-compatible)
- **Sensor**: TCS34725 (I2C RGB/clear, lux calculated via DN40 algorithm)
- **Wiring**: SDA → GP26, SCL → GP27, VIN → 3.3V, GND → GND
- **I2C bus**: GP26/GP27 are on **I2C1** (`Wire1`), not I2C0 (`Wire`). Using `Wire` with these pins will hang in `Wire.begin()`.

## Build Commands

```bash
pio run -e rp2040             # build
pio run -e rp2040 -t upload   # build and flash via picotool
```

**Flashing via UF2**: hold BOOTSEL, plug USB, copy `.pio/build/rp2040/firmware.uf2` to the `RPI-RP2` drive.

**PlatformIO path** (not in global PATH): `~/.platformio/penv/Scripts/pio`

The `rp2040` env uses `platform = https://github.com/maxgerhardt/platform-raspberrypi.git` (required for earlephilhower core), `board = pico`, `board_build.core = earlephilhower`, and `-DUSE_TINYUSB`.

## Architecture

Single-target firmware: RP2040 + TCS34725 + Adafruit TinyUSB.

**HID callbacks**: Must use `Adafruit_USBD_HID::setReportCallback(get, set)` — do NOT define `tud_hid_get_report_cb` / `tud_hid_set_report_cb` directly, as Adafruit's library already defines them and dispatches to registered callbacks. Defining them causes multiple-definition linker errors.

**HID report descriptor**: Top-level collection MUST be `HID_COLLECTION(Application)` — `Physical` will compile but Windows HidUsb won't create child TLC devices, so `SensorsHidClassDriver` won't load. The descriptor uses macros from `include/HidSensorSpec.h` which intentionally redefine TinyUSB's HID macros (warnings are expected and safe — both produce identical byte values).

**HID report flow**: The device exposes one Feature report (sensor config: connection type, reporting state, power state, sensor state, report interval, range, sensitivity, chromaticity change sensitivity) and one Input report (sensor state, event type, lux as `uint32_t`, color temperature as `uint16_t` in Kelvin, chromaticity x and y as `uint16_t` in CIE 1931 coordinates). Lux is scaled ×10 (unit exponent 0x0F = -1) so integer 1 = 0.1 lux. Chromaticity uses exponent -4 so integer 3320 = 0.3320. Per MS spec, color-capable sensors MUST report chromaticity x,y alongside any color temperature — kelvins alone is invalid and may cause `SensorsHidClassDriver` to reject samples. After `USAGE_SENSOR_UNITS_KELVIN` (color temp field), chromaticity fields MUST reset unit via `USAGE_SENSOR_UNITS_NOT_SPECIFIED` since HID unit is a global item that persists. Adaptive Color (display white point adjustment) only works on Windows 11 with Brightness3 DDI displays (laptops).

**Periodic reporting**: `timeoutCounter` (uint8_t, wraps at 256) drives the report interval. Each `loop()` iteration delays 2 ms; at overflow the `timeout` flag triggers a sensor read and HID report, giving ~512 ms default cadence. `featureReportBuf.reportInterval` is stored but not used to control the actual cadence.

**`lib/tcs34725/TCS34725Lux`**: Wraps `Adafruit_TCS34725` with DN40 lux calculation and CIE 1931 chromaticity. `readLightLevel()` returns `int32_t` lux (0–65535) or `-1` on error. `getChromaticity(cx, cy)` returns CIE 1931 x,y. `getColorTemperature()` returns CCT via McCamy's formula. All color data is computed from cached RGBC values after `readLightLevel()`.

**USB identity**: VID `0x16c0`, PID `0x27dc`, manufacturer `https://github.com/3cky/tiny-hid-als`, product `Light Sensor`.

## Troubleshooting

- **Windows driver cache**: If changing HID descriptor structure, Windows may cache old driver assignment. Changing PID temporarily forces full re-enumeration. After `SensorsHidClassDriver` loads, the original PID can be restored.
- **`picotool` upload fails on Windows**: Install WinUSB driver via Zadig for the RP2040 BOOTSEL device, or use UF2 copy method.

## Status

Firmware compiles, runs on hardware, and reads lux + color temperature + chromaticity from the TCS34725 sensor. Windows 10/11 recognizes the device as "Датчик освещённости HID" (HID Ambient Light Sensor) in the Sensor device class. Adaptive brightness works on laptops with built-in displays. Color temperature data is available via Windows Sensor API for desktop utilities (als-brightness).
