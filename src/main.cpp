/*
 * USB HID Ambient Light Sensor — RP2040 + TCS34725
 *
 * Based on tiny-hid-als by Victor Antonovich (https://github.com/3cky/tiny-hid-als)
 * Ported to RP2040 with Adafruit TinyUSB and TCS34725 color sensor.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */

#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include "TCS34725Lux.h"
#include "HidSensorSpec.h"

#define LED_OUT() pinMode(LED_BUILTIN, OUTPUT)
#define LED_OFF() digitalWrite(LED_BUILTIN, 0)
#define LED_ON() digitalWrite(LED_BUILTIN, 1)
#define LED_TOGGLE() digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN))

// HID Report Descriptor
const uint8_t desc_hid_report[] = {
  HID_USAGE_PAGE_SENSOR,
  HID_USAGE_SENSOR_TYPE_LIGHT_AMBIENTLIGHT,
  HID_COLLECTION(Application),

  //feature reports (xmit/receive)
  HID_USAGE_PAGE_SENSOR,
  // Sensor Connection Type - RO
  HID_USAGE_SENSOR_PROPERTY_SENSOR_CONNECTION_TYPE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_8(2),
  HID_REPORT_SIZE(8),
  HID_REPORT_COUNT(1),
  HID_COLLECTION(Logical),
  HID_USAGE_SENSOR_PROPERTY_CONNECTION_TYPE_PC_INTEGRATED_SEL,
  HID_USAGE_SENSOR_PROPERTY_CONNECTION_TYPE_PC_ATTACHED_SEL,
  HID_USAGE_SENSOR_PROPERTY_CONNECTION_TYPE_PC_EXTERNAL_SEL,
  HID_FEATURE(Data_Arr_Abs),
  HID_END_COLLECTION,
  // Reporting State - RW
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_8(5),
  HID_REPORT_SIZE(8),
  HID_REPORT_COUNT(1),
  HID_COLLECTION(Logical),
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_NO_EVENTS_SEL,
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_ALL_EVENTS_SEL,
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_THRESHOLD_EVENTS_SEL,
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_NO_EVENTS_WAKE_SEL,
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_ALL_EVENTS_WAKE_SEL,
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_THRESHOLD_EVENTS_WAKE_SEL,
  HID_FEATURE(Data_Arr_Abs),
  HID_END_COLLECTION,
  // Power State - RW
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_8(5),
  HID_REPORT_SIZE(8),
  HID_REPORT_COUNT(1),
  HID_COLLECTION(Logical),
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_UNDEFINED_SEL,
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D0_FULL_POWER_SEL,
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D1_LOW_POWER_SEL,
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D2_STANDBY_WITH_WAKE_SEL,
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D3_SLEEP_WITH_WAKE_SEL,
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D4_POWER_OFF_SEL,
  HID_FEATURE(Data_Arr_Abs),
  HID_END_COLLECTION,
  // Sensor State - RW
  HID_USAGE_SENSOR_STATE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_8(6),
  HID_REPORT_SIZE(8),
  HID_REPORT_COUNT(1),
  HID_COLLECTION(Logical),
  HID_USAGE_SENSOR_STATE_UNKNOWN_SEL,
  HID_USAGE_SENSOR_STATE_READY_SEL,
  HID_USAGE_SENSOR_STATE_NOT_AVAILABLE_SEL,
  HID_USAGE_SENSOR_STATE_NO_DATA_SEL,
  HID_USAGE_SENSOR_STATE_INITIALIZING_SEL,
  HID_USAGE_SENSOR_STATE_ACCESS_DENIED_SEL,
  HID_USAGE_SENSOR_STATE_ERROR_SEL,
  HID_FEATURE(Data_Arr_Abs),
  HID_END_COLLECTION,
  // Report Interval - RW
  HID_USAGE_SENSOR_PROPERTY_REPORT_INTERVAL,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_32(0xFF,0xFF,0xFF,0xFF),
  HID_REPORT_SIZE(32),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0),
  HID_FEATURE(Data_Var_Abs),
  // Minimum Report Interval - RO
  HID_USAGE_SENSOR_PROPERTY_MINIMUM_REPORT_INTERVAL,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_32(0xFF, 0xFF, 0xFF, 0xFF),
  HID_REPORT_SIZE(32),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0),
  HID_FEATURE(Data_Var_Abs),
  // Range Maximum - RO
  HID_USAGE_SENSOR_DATA(HID_USAGE_SENSOR_DATA_LIGHT_ILLUMINANCE,
                        HID_USAGE_SENSOR_DATA_MOD_MAX),
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_32(0xFF, 0xFF, 0xFF, 0xFF),
  HID_REPORT_SIZE(32),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0x0F), // exponent -1
  HID_FEATURE(Data_Var_Abs),
  // Range Minimum - RO
  HID_USAGE_SENSOR_DATA(HID_USAGE_SENSOR_DATA_LIGHT_ILLUMINANCE,
                        HID_USAGE_SENSOR_DATA_MOD_MIN),
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0x0F), // exponent -1
  HID_FEATURE(Data_Var_Abs),
  // Change Sensitivity Absolute (illuminance) - RW
  HID_USAGE_SENSOR_DATA(HID_USAGE_SENSOR_DATA_LIGHT_ILLUMINANCE,
                        HID_USAGE_SENSOR_DATA_MOD_CHANGE_SENSITIVITY_ABS),
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0x0D),  // exponent -3
  HID_FEATURE(Data_Var_Abs),
  // Change Sensitivity Absolute (chromaticity x) - RW
  HID_USAGE_SENSOR_DATA(HID_USAGE_SENSOR_DATA_LIGHT_CHROMATICITY_X,
                        HID_USAGE_SENSOR_DATA_MOD_CHANGE_SENSITIVITY_ABS),
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0x0C),  // exponent -4
  HID_FEATURE(Data_Var_Abs),
  // Change Sensitivity Absolute (chromaticity y) - RW
  HID_USAGE_SENSOR_DATA(HID_USAGE_SENSOR_DATA_LIGHT_CHROMATICITY_Y,
                        HID_USAGE_SENSOR_DATA_MOD_CHANGE_SENSITIVITY_ABS),
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0x0C),  // exponent -4
  HID_FEATURE(Data_Var_Abs),

  //input reports (transmit)
  HID_USAGE_PAGE_SENSOR,
  // Sensor states
  HID_USAGE_SENSOR_STATE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_8(6),
  HID_REPORT_SIZE(8),
  HID_REPORT_COUNT(1),
  HID_COLLECTION(Logical),
  HID_USAGE_SENSOR_STATE_UNKNOWN_SEL,
  HID_USAGE_SENSOR_STATE_READY_SEL,
  HID_USAGE_SENSOR_STATE_NOT_AVAILABLE_SEL,
  HID_USAGE_SENSOR_STATE_NO_DATA_SEL,
  HID_USAGE_SENSOR_STATE_INITIALIZING_SEL,
  HID_USAGE_SENSOR_STATE_ACCESS_DENIED_SEL,
  HID_USAGE_SENSOR_STATE_ERROR_SEL,
  HID_INPUT(Data_Arr_Abs),
  HID_END_COLLECTION,
  // Sensor Events
  HID_USAGE_SENSOR_EVENT,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_8(5),
  HID_REPORT_SIZE(8),
  HID_REPORT_COUNT(1),
  HID_COLLECTION(Logical),
  HID_USAGE_SENSOR_EVENT_UNKNOWN_SEL,
  HID_USAGE_SENSOR_EVENT_STATE_CHANGED_SEL,
  HID_USAGE_SENSOR_EVENT_PROPERTY_CHANGED_SEL,
  HID_USAGE_SENSOR_EVENT_DATA_UPDATED_SEL,
  HID_USAGE_SENSOR_EVENT_POLL_RESPONSE_SEL,
  HID_USAGE_SENSOR_EVENT_CHANGE_SENSITIVITY_SEL,
  HID_INPUT(Data_Arr_Abs),
  HID_END_COLLECTION,
  // Sensor data - illuminance
  HID_USAGE_SENSOR_DATA_LIGHT_ILLUMINANCE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_32(0xFF,0xFF,0xFF,0xFF),
  HID_UNIT_EXPONENT(0x0F), // exponent -1: integer 1 = 0.1 lux
  HID_REPORT_SIZE(32),
  HID_REPORT_COUNT(1),
  HID_INPUT(Data_Var_Abs),
  // Sensor data - color temperature
  HID_USAGE_SENSOR_DATA_LIGHT_COLOR_TEMPERATURE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_UNIT_EXPONENT(0),
  HID_USAGE_SENSOR_UNITS_KELVIN,
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_INPUT(Data_Var_Abs),
  // Sensor data - chromaticity x (CIE 1931, dimensionless)
  HID_USAGE_SENSOR_DATA_LIGHT_CHROMATICITY_X,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_USAGE_SENSOR_UNITS_NOT_SPECIFIED,  // reset unit (Kelvin inherited from color temp)
  HID_UNIT_EXPONENT(0x0C), // exponent -4: integer 3320 = 0.3320
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_INPUT(Data_Var_Abs),
  // Sensor data - chromaticity y (CIE 1931, dimensionless)
  HID_USAGE_SENSOR_DATA_LIGHT_CHROMATICITY_Y,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_INPUT(Data_Var_Abs),
  HID_END_COLLECTION
};

// Feature Report buffer
struct __attribute__((packed)) {
  uint8_t connectionType;
  uint8_t reportingState;
  uint8_t powerState;
  uint8_t sensorState;
  uint32_t reportInterval;
  uint32_t minReportInterval;
  uint32_t rangeMax;
  uint16_t rangeMin;
  uint16_t lightChangeSensitivity;
  uint16_t chromXChangeSensitivity;
  uint16_t chromYChangeSensitivity;
} featureReportBuf = {
  HID_USAGE_SENSOR_PROPERTY_CONNECTION_TYPE_PC_ATTACHED_SEL_ENUM,
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_ALL_EVENTS_SEL_ENUM,
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D0_FULL_POWER_ENUM,
  HID_USAGE_SENSOR_STATE_READY_SEL_ENUM,
  1000,   // report interval (ms)
  250,    // min report interval (ms)
  655350, // range max = 65535.0 lux
  0,      // range min = 0.0 lux
  100,    // change sensitivity illuminance = 0.1 lux (exponent -3)
  100,    // change sensitivity chromaticity x = 0.0100 (exponent -4)
  100,    // change sensitivity chromaticity y = 0.0100 (exponent -4)
};

// Input Report buffer
struct __attribute__((packed)) {
  uint8_t sensorState;
  uint8_t eventType;
  uint32_t lightValue;       // lux ×10 (unit exponent -1)
  uint16_t colorTemperature; // Kelvin
  uint16_t chromaticityX;    // CIE 1931 x ×10000 (unit exponent -4)
  uint16_t chromaticityY;    // CIE 1931 y ×10000 (unit exponent -4)
} inputReportBuf = {
  HID_USAGE_SENSOR_STATE_READY_SEL_ENUM,
  HID_USAGE_SENSOR_EVENT_POLL_RESPONSE_SEL_ENUM,
  0, 0, 0, 0,
};

bool getInputRequested = false;
bool timeout = false;
uint8_t timeoutCounter = 0;

Adafruit_USBD_HID usbHid(desc_hid_report, sizeof(desc_hid_report),
                          HID_ITF_PROTOCOL_NONE, 10, false);
TCS34725Lux lightSensor;

// GET_REPORT callback
uint16_t getReportCallback(uint8_t report_id, hid_report_type_t report_type,
                            uint8_t *buffer, uint16_t reqlen) {
  if (report_type == HID_REPORT_TYPE_FEATURE) {
    uint16_t len = reqlen < sizeof(featureReportBuf) ? reqlen : sizeof(featureReportBuf);
    memcpy(buffer, &featureReportBuf, len);
    return len;
  } else if (report_type == HID_REPORT_TYPE_INPUT) {
    getInputRequested = true;
  }
  return 0;
}

// SET_REPORT callback
void setReportCallback(uint8_t report_id, hid_report_type_t report_type,
                        uint8_t const *buffer, uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_FEATURE) {
    uint16_t len = bufsize < sizeof(featureReportBuf) ? bufsize : sizeof(featureReportBuf);
    memcpy(&featureReportBuf, buffer, len);
  }
}

void setup() {
  LED_OUT();
  LED_OFF();

  // Configure USB identity
  TinyUSBDevice.setID(0x16c0, 0x27dc);
  TinyUSBDevice.setManufacturerDescriptor("https://github.com/3cky/tiny-hid-als");
  TinyUSBDevice.setProductDescriptor("Light Sensor");

  usbHid.setReportCallback(getReportCallback, setReportCallback);
  usbHid.begin();

  while (!TinyUSBDevice.mounted()) {
    delay(1);
  }

  Serial.begin(115200);
  for (int i = 0; i < 500 && !Serial; i++) delay(10);
  delay(100);
  Serial.println("ALS Sensor started");
  Serial.print("HID descriptor size: ");
  Serial.println(sizeof(desc_hid_report));

  // Initialize TCS34725 on I2C1 (GP26=SDA, GP27=SCL)
  Wire1.setSDA(26);
  Wire1.setSCL(27);
  Wire1.begin();
  lightSensor.begin(&Wire1);
}

void loop() {
  if (getInputRequested || (timeout && (featureReportBuf.reportingState
      != HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_NO_EVENTS_SEL_ENUM))) {
    if (getInputRequested) {
      getInputRequested = false;
      inputReportBuf.eventType = HID_USAGE_SENSOR_EVENT_POLL_RESPONSE_SEL_ENUM;
    } else {
      timeout = false;
      inputReportBuf.eventType = HID_USAGE_SENSOR_EVENT_DATA_UPDATED_SEL_ENUM;
    }

    int32_t lux = lightSensor.readLightLevel();
    if (lux >= 0) {
      inputReportBuf.lightValue = (uint32_t)lux * 10;
      int32_t cct = lightSensor.getColorTemperature();
      inputReportBuf.colorTemperature = (cct >= 0) ? (uint16_t)cct : 0;
      float cx, cy;
      if (lightSensor.getChromaticity(cx, cy)) {
        inputReportBuf.chromaticityX = (uint16_t)(cx * 10000.0f);
        inputReportBuf.chromaticityY = (uint16_t)(cy * 10000.0f);
      }
      inputReportBuf.sensorState = HID_USAGE_SENSOR_STATE_READY_SEL_ENUM;
    } else {
      inputReportBuf.sensorState = HID_USAGE_SENSOR_STATE_ERROR_SEL_ENUM;
    }

    Serial.print("lux=");
    Serial.print(lux);
    Serial.print(" cct=");
    Serial.print(inputReportBuf.colorTemperature);
    Serial.print(" x=");
    Serial.print(inputReportBuf.chromaticityX);
    Serial.print(" y=");
    Serial.println(inputReportBuf.chromaticityY);

    if (TinyUSBDevice.mounted()) {
      usbHid.sendReport(0, &inputReportBuf, sizeof(inputReportBuf));
    }
  }

  // Periodic debug output every ~2 seconds
  static uint16_t debugCounter = 0;
  if (++debugCounter >= 1000) {
    debugCounter = 0;
    int32_t dbgLux = lightSensor.readLightLevel();
    int32_t dbgCct = lightSensor.getColorTemperature();
    Serial.print("lux=");
    Serial.print(dbgLux);
    Serial.print(" cct=");
    Serial.println(dbgCct);
  }

  delay(2);
  if (!--timeoutCounter) {
    timeout = true;
  }
}
