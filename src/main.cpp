/*
 * Copyright (c) 2022 Victor Antonovich (victor@antonoivch.me).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#if defined(ARDUINO_ARCH_RP2040)
  #include <Adafruit_TinyUSB.h>
  #include "TCS34725Lux.h"
#else
  #include <util/delay.h>
  #include <BH1750.h>
  #include <usbdrv.h>
#endif

#include <Wire.h>
#include "HidSensorSpec.h"

#define LED_OUT() pinMode(LED_BUILTIN, OUTPUT)
#define LED_OFF() digitalWrite(LED_BUILTIN, 0)
#define LED_ON() digitalWrite(LED_BUILTIN, 1)
#define LED_TOGGLE() digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN))

// HID Report Descriptor
#if defined(ARDUINO_ARCH_RP2040)
const uint8_t desc_hid_report[] = {
#else
PROGMEM const uchar usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
#endif
  HID_USAGE_PAGE_SENSOR,         // USAGE_PAGE (Sensor)
  HID_USAGE_SENSOR_TYPE_LIGHT_AMBIENTLIGHT, // USAGE (AmbientLight)
  HID_COLLECTION(Application),

  //feature reports (xmit/receive)
  HID_USAGE_PAGE_SENSOR,
  // Sensor Connection Type - RO
  HID_USAGE_SENSOR_PROPERTY_SENSOR_CONNECTION_TYPE,  // NAry
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
  // Minimum Report Interval  -  RO
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
  HID_UNIT_EXPONENT(0x0F), // scale default unit to provide 1 digit past decimal point
  HID_FEATURE(Data_Var_Abs),
  // Range Minimum - RO
  HID_USAGE_SENSOR_DATA(HID_USAGE_SENSOR_DATA_LIGHT_ILLUMINANCE,
                        HID_USAGE_SENSOR_DATA_MOD_MIN),
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0x0F), // scale default unit to provide 1 digit past decimal point
  HID_FEATURE(Data_Var_Abs),
  // Change Sensitivity Absolute - RW
  HID_USAGE_SENSOR_DATA(HID_USAGE_SENSOR_DATA_LIGHT_ILLUMINANCE,
                        HID_USAGE_SENSOR_DATA_MOD_CHANGE_SENSITIVITY_ABS),
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_16(0xFF,0xFF),
  HID_REPORT_SIZE(16),
  HID_REPORT_COUNT(1),
  HID_UNIT_EXPONENT(0x0D),  // scale default unit to provide 3 digits past decimal point
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
  // Sensor data
  HID_USAGE_SENSOR_DATA_LIGHT_ILLUMINANCE,
  HID_LOGICAL_MIN_8(0),
  HID_LOGICAL_MAX_32(0xFF,0xFF,0xFF,0xFF),
  HID_UNIT_EXPONENT(0x0F), // scale default unit to provide 1 digit past decimal point
  HID_REPORT_SIZE(32),
  HID_REPORT_COUNT(1),
  HID_INPUT(Data_Var_Abs),

  HID_END_COLLECTION
};

// Feature Report buffer
struct __attribute__((packed)) {
  //common properties
  //uint8_t   reportId;
  uint8_t connectionType;          // HID_USAGE_SENSOR_PROPERTY_SENSOR_CONNECTION_TYPE
  uint8_t reportingState;          // HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE
  uint8_t powerState;              // HID_USAGE_SENSOR_PROPERTY_POWER_STATE
  uint8_t sensorState;             // HID_USAGE_SENSOR_STATE
  uint32_t reportInterval;         // HID_USAGE_SENSOR_PROPERTY_REPORT_INTERVAL
  uint32_t minReportInterval;      // HID_USAGE_SENSOR_PROPERTY_MINIMUM_REPORT_INTERVAL
  uint32_t rangeMax;               // HID_USAGE_SENSOR_PROPERTY_RANGE_MAXIMUM
  uint16_t rangeMin;               // HID_USAGE_SENSOR_PROPERTY_RANGE_MINIMUM
  uint16_t lightChangeSensitivity; // HID_USAGE_SENSOR_PROPERTY_CHANGE_SENSITIVITY_ABS

} featureReportBuf = {
  //1,  // Report ID
  HID_USAGE_SENSOR_PROPERTY_CONNECTION_TYPE_PC_ATTACHED_SEL_ENUM,
  HID_USAGE_SENSOR_PROPERTY_REPORTING_STATE_ALL_EVENTS_SEL_ENUM,
  HID_USAGE_SENSOR_PROPERTY_POWER_STATE_D0_FULL_POWER_ENUM,
  HID_USAGE_SENSOR_STATE_READY_SEL_ENUM,
  1000, /* Reporting interval (ms)*/
  250,  /* Min reporting interval (ms)*/

  655350, /* Range max = 65535.0 lx */
  0,      /* Range min = 0.0 lx */
  100,    /* Change sensitivity = 0.1 lx*/
};

// Input Report buffer
struct __attribute__((packed)) {
  // common values
  // uint8_t   reportId;
  uint8_t sensorState;  // HID_USAGE_SENSOR_STATE
  uint8_t eventType;    // HID_USAGE_SENSOR_EVENT

  // values specific to this sensor
  uint32_t lightValue;  // HID_USAGE_SENSOR_TYPE_LIGHT_AMBIENTLIGHT

} inputReportBuf = {
  //1,  // Report ID
  HID_USAGE_SENSOR_STATE_READY_SEL_ENUM,
  HID_USAGE_SENSOR_EVENT_POLL_RESPONSE_SEL_ENUM,

  0, /* lux */
};

#if !defined(ARDUINO_ARCH_RP2040)
uint8_t *writeBuf;
uint16_t bytesRemaining;
#endif

bool getInputRequested = false;

bool timeout = false;
uint8_t timeoutCounter = 0;

#if defined(ARDUINO_ARCH_RP2040)
Adafruit_USBD_HID usbHid(desc_hid_report, sizeof(desc_hid_report),
                          HID_ITF_PROTOCOL_NONE, 10, false);
TCS34725Lux lightSensor;
#else
BH1750 lightSensor;
#endif

#if defined(ARDUINO_ARCH_RP2040)
// Adafruit_USBD_HID GET_REPORT callback
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

// Adafruit_USBD_HID SET_REPORT callback
void setReportCallback(uint8_t report_id, hid_report_type_t report_type,
                        uint8_t const *buffer, uint16_t bufsize) {
  if (report_type == HID_REPORT_TYPE_FEATURE) {
    uint16_t len = bufsize < sizeof(featureReportBuf) ? bufsize : sizeof(featureReportBuf);
    memcpy(&featureReportBuf, buffer, len);
  }
}
#endif

void setup() {
  // Turn off onboard LED
  LED_OUT();
  LED_OFF();

#if defined(ARDUINO_ARCH_RP2040)
  // Configure USB identity
  TinyUSBDevice.setID(0x16c0, 0x27da); // temporary PID to bypass Windows driver cache
  TinyUSBDevice.setManufacturerDescriptor("https://github.com/3cky/tiny-hid-als");
  TinyUSBDevice.setProductDescriptor("Light Sensor");

  // Register HID callbacks and start USB HID
  usbHid.setReportCallback(getReportCallback, setReportCallback);
  usbHid.begin();

  // Wait for USB mount
  while (!TinyUSBDevice.mounted()) {
    delay(1);
  }

  Serial.begin(115200);
  // Wait up to 2s for Serial CDC connection (don't block if no monitor)
  for (int i = 0; i < 500 && !Serial; i++) delay(10);
  delay(100);
  Serial.println("ALS Sensor started");
  Serial.print("HID descriptor size: ");
  Serial.println(sizeof(desc_hid_report));
#else
  // Initialize USB connection (V-USB)
  cli();
  usbInit();
  usbDeviceDisconnect(); /* enforce re-enumeration */
  while (--timeoutCounter) { /* fake USB disconnect for > 250 ms */
    _delay_ms(1);
  }
  usbDeviceConnect();
  sei();
#endif

  // Initialize light sensor (GP26/GP27 are on I2C1, not I2C0)
#if defined(ARDUINO_ARCH_RP2040)
  Wire1.setSDA(26);
  Wire1.setSCL(27);
  Wire1.begin();
  lightSensor.begin(&Wire1);
#else
  Wire.begin();
  lightSensor.begin();
#endif
}

// the loop routine runs over and over again forever:
void loop() {
#if !defined(ARDUINO_ARCH_RP2040)
  usbPoll();
#endif

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
      inputReportBuf.lightValue = lux;
      inputReportBuf.sensorState = HID_USAGE_SENSOR_STATE_READY_SEL_ENUM;
    } else {
      inputReportBuf.sensorState = HID_USAGE_SENSOR_STATE_ERROR_SEL_ENUM;
    }
#if defined(ARDUINO_ARCH_RP2040)
    Serial.print("lux=");
    Serial.println(lux);
#endif
#if defined(ARDUINO_ARCH_RP2040)
    if (TinyUSBDevice.mounted()) {
      usbHid.sendReport(0, &inputReportBuf, sizeof(inputReportBuf));
    }
#else
    /* called after every poll of the interrupt endpoint */
    usbSetInterrupt((uchar *)&inputReportBuf, sizeof(inputReportBuf));
#endif
  }

#if defined(ARDUINO_ARCH_RP2040)
  // Periodic lux output every ~2 seconds (also used by als-brightness utility)
  static uint16_t debugCounter = 0;
  if (++debugCounter >= 1000) {
    debugCounter = 0;
    int32_t dbgLux = lightSensor.readLightLevel();
    Serial.print("lux=");
    Serial.println(dbgLux);
  }
#endif

  delay(2);
  if (!--timeoutCounter) {
    timeout = true;
  }
}

#if !defined(ARDUINO_ARCH_RP2040)
#ifdef __cplusplus
extern "C" {
#endif
usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  usbRequest_t *rq = (usbRequest_t *)data;
  if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) { /* class request type */
    if (rq->bRequest == USBRQ_HID_GET_REPORT) {
      /* wValue: ReportType (highbyte), ReportID (lowbyte) */
      if (rq->wValue.bytes[1] == 0x03) { /* Get Feature report */
        usbMsgPtr = (uchar *)&featureReportBuf;
        return sizeof(featureReportBuf);
      } else if (rq->wValue.bytes[1] == 0x01) { /* Get Input report */
        getInputRequested = true;
      }
    } else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
      if (rq->wValue.bytes[1] == 0x03) { /* Set Feature report */
        bytesRemaining = rq->wLength.word;
        writeBuf = (uchar *)&featureReportBuf;
        if (bytesRemaining > sizeof(featureReportBuf)) {
          bytesRemaining = sizeof(featureReportBuf);
        }
      }
      return USB_NO_MSG; /* Use usbFunctionWrite() to get data from host */
    }
  } else {
    /* no vendor specific requests implemented */
  }
  return 0; /* default for not implemented requests: return no data back to host */
}

uchar usbFunctionWrite(uchar *data, uchar len) {
  uint8_t timeoutCounter;
  if (len > bytesRemaining)
    len = bytesRemaining;
  for (timeoutCounter = 0; timeoutCounter < len; timeoutCounter++)
    writeBuf[timeoutCounter] = data[timeoutCounter];
  writeBuf += len;
  bytesRemaining -= len;
  return bytesRemaining == 0; /* return 1 if this was the last chunk */
}
#ifdef __cplusplus
}  // extern "C"
#endif
#endif
