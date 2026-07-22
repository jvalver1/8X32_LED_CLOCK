/**
 * @file    bmp_manager.cpp
 * @brief   BmpManager implementation.
 */

#include "bmp_manager.h"
#include "../config.h"
#include <Arduino.h>
#include <Wire.h>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
Adafruit_BMP280 BmpManager::_bmp;
SensorData      BmpManager::_cache     = { 0.0f, 0.0f, false };
bool            BmpManager::_available = false;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
bool BmpManager::init()
{
    // ---------------------------------------------------------------
    // Scan I2C bus to find the BMP280 at 0x76 or 0x77.
    // This avoids calling begin() twice on the same Adafruit_BMP280
    // object which causes heap corruption on AVR (new/delete on 2KB RAM).
    // ---------------------------------------------------------------
    uint8_t foundAddr = 0;
    const uint8_t candidates[2] = { BMP280_I2C_ADDR,
                                     (uint8_t)((BMP280_I2C_ADDR == 0x76) ? 0x77 : 0x76) };

    for (uint8_t i = 0; i < 2; i++)
    {
        Wire.beginTransmission(candidates[i]);
        if (Wire.endTransmission() == 0)   // 0 = ACK → device present
        {
            foundAddr = candidates[i];
            break;
        }
    }

    if (foundAddr == 0)
    {
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[BMP] ERROR: No device on I2C 0x76 or 0x77!"));
        Serial.println(F("[BMP] Check SDA(A4)/SCL(A5) wiring."));
#endif
        _available = false;
        return false;
    }

#ifdef ENABLE_DEBUG_SERIAL
    Serial.print(F("[BMP] Found at 0x"));
    Serial.println(foundAddr, HEX);
#endif

    if (!_bmp.begin(foundAddr))   // called exactly ONCE
    {
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[BMP] ERROR: begin() failed (bad chip ID?)."));
#endif
        _available = false;
        return false;
    }

    // Recommended settings for indoor environment monitoring
    _bmp.setSampling(
        Adafruit_BMP280::MODE_NORMAL,     // Continuous measurement
        Adafruit_BMP280::SAMPLING_X2,     // Temperature oversampling ×2
        Adafruit_BMP280::SAMPLING_X16,    // Pressure oversampling ×16
        Adafruit_BMP280::FILTER_X16,      // IIR filter coefficient
        Adafruit_BMP280::STANDBY_MS_500   // Standby time 500 ms
    );

    _available = true;
    update(); // Populate cache immediately

#ifdef ENABLE_DEBUG_SERIAL
    Serial.print(F("[BMP] Initialised. Temp: "));
    Serial.print(_cache.temperatureC);
    Serial.print(F("°C  Pressure: "));
    Serial.print(_cache.pressureHPa);
    Serial.println(F(" hPa"));
#endif

    return true;
}

void BmpManager::update()
{
    if (!_available) return;

    float t = _bmp.readTemperature();
    float p = _bmp.readPressure() / 100.0f; // Pa → hPa

    // Basic sanity check
    if (isnan(t) || isnan(p) || t < -40.0f || t > 85.0f)
    {
        _cache.valid = false;
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[BMP] Warning: invalid reading."));
#endif
        return;
    }

    _cache = { t, p, true };
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
SensorData BmpManager::getData()           { return _cache; }
float      BmpManager::getTemperatureC()   { return _cache.temperatureC; }
float      BmpManager::getTemperatureF()   { return _cache.temperatureC * 9.0f / 5.0f + 32.0f; }
float      BmpManager::getPressureHPa()    { return _cache.pressureHPa; }
bool       BmpManager::isAvailable()       { return _available && _cache.valid; }
