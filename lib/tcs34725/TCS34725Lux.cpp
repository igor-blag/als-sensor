/*
 * TCS34725 light sensor wrapper with DN40 lux calculation.
 *
 * Uses the AMS/TAOS DN40 application note algorithm to convert
 * raw RGBC values to lux.
 */

#include "TCS34725Lux.h"

// DN40 constants for TCS34725
#define TCS34725_DF      310.0f   // Device factor
#define TCS34725_GA      1.0f     // Glass attenuation (1.0 = no glass)
#define TCS34725_COEF_C  0.136f
#define TCS34725_COEF_R  0.421f
#define TCS34725_COEF_C2 0.235f
#define TCS34725_COEF_B  0.515f

bool TCS34725Lux::begin(TwoWire *wire) {
    initialized = tcs.begin(TCS34725_ADDRESS, wire);
    return initialized;
}

float TCS34725Lux::calculateLux(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    // Compute IR component
    int32_t ir = ((int32_t)r + g + b - c) / 2;
    if (ir < 0) ir = 0;

    // Subtract IR from channels
    float c_ir = c - ir;
    float r_ir = r - ir;
    float b_ir = b - ir;

    // Counts per lux: CPL = (atime_ms * gain) / (GA * DF)
    // TCS34725_INTEGRATIONTIME_50MS = 0xEB → atime_ms = (256 - 0xEB) * 2.4 = 50.4ms
    float atime_ms = 50.4f;
    float gain = 4.0f;
    float cpl = (atime_ms * gain) / (TCS34725_GA * TCS34725_DF);

    // Two lux estimates
    float lux1 = (TCS34725_COEF_C * c_ir - TCS34725_COEF_R * r_ir) / cpl;
    float lux2 = (TCS34725_COEF_C2 * c_ir - TCS34725_COEF_B * b_ir) / cpl;

    float lux = lux1 > lux2 ? lux1 : lux2;
    return lux > 0.0f ? lux : 0.0f;
}

int32_t TCS34725Lux::readLightLevel() {
    if (!initialized) return -1;

    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);

    // Check for saturation or zero (sensor error)
    if (c == 0) return -1;

    // Cache raw values for subsequent getColorTemperature() call
    lastR = r; lastG = g; lastB = b; lastC = c;

    float lux = calculateLux(r, g, b, c);

    // Clamp to 16-bit range (HID report uses 0-65535)
    if (lux > 65535.0f) lux = 65535.0f;

    return (int32_t)lux;
}

float TCS34725Lux::calculateCCT(uint16_t r, uint16_t g, uint16_t b) {
    // McCamy's approximation via CIE chromaticity coordinates
    // Convert RGB to CIE XYZ (simplified matrix for TCS34725)
    float X = -0.14282f * r + 1.54924f * g + -0.95641f * b;
    float Y = -0.32466f * r + 1.57837f * g + -0.73191f * b;
    float Z = -0.68202f * r + 0.77073f * g +  0.56332f * b;

    float sum = X + Y + Z;
    if (sum < 1.0f) return -1.0f;

    // CIE chromaticity
    float x = X / sum;
    float y = Y / sum;

    // McCamy's formula: CCT = 449*n^3 + 3525*n^2 + 6823.3*n + 5520.33
    float n = (x - 0.3320f) / (0.1858f - y);
    float cct = 449.0f * n * n * n + 3525.0f * n * n + 6823.3f * n + 5520.33f;

    return cct;
}

int32_t TCS34725Lux::getColorTemperature() {
    if (!initialized || lastC == 0) return -1;

    float cct = calculateCCT(lastR, lastG, lastB);
    if (cct < 1000.0f || cct > 12000.0f) return -1;

    return (int32_t)cct;
}
