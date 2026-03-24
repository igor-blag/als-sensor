/*
 * TCS34725 light sensor wrapper with DN40 lux calculation.
 *
 * Provides readLightLevel() returning int32_t lux (0-65535)
 * or -1 on error, matching the BH1750 API contract.
 */

#ifndef TCS34725_LUX_H
#define TCS34725_LUX_H

#include <Adafruit_TCS34725.h>
#include <Wire.h>

class TCS34725Lux {
public:
    bool begin(TwoWire *wire = &Wire);
    int32_t readLightLevel();
    int32_t getColorTemperature();

private:
    Adafruit_TCS34725 tcs = Adafruit_TCS34725(
        TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
    bool initialized = false;

    uint16_t lastR = 0, lastG = 0, lastB = 0, lastC = 0;

    float calculateLux(uint16_t r, uint16_t g, uint16_t b, uint16_t c);
    float calculateCCT(uint16_t r, uint16_t g, uint16_t b);
};

#endif
