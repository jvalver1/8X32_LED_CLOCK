/** @file bme_manager.cpp @brief BME280 temperature/humidity/pressure sensor. */
#include "bme_manager.h"
#include "../config.h"
#include <Arduino.h>
#include <Wire.h>

Adafruit_BME280 BmeManager::_bme;
SensorData BmeManager::_cache = { 0.0f, 0.0f, 0.0f, false };
bool BmeManager::_available = false;
bool BmeManager::_usingSimulationFallback = false;

bool BmeManager::init()
{
    const uint8_t alternate = BME280_I2C_ADDR == 0x76 ? 0x77 : 0x76;
    uint8_t detectedAddress = BME280_I2C_ADDR;
    _available = _bme.begin(detectedAddress);
    if (!_available)
    {
        detectedAddress = alternate;
        _available = _bme.begin(detectedAddress);
    }
    if (!_available)
    {
#ifdef WOKWI_SIMULATION
        // The bundled Wokwi BME280 custom chip currently implements SPI only,
        // while the physical clock uses the sensor's I2C interface. Keep the
        // production path unchanged and provide representative simulation
        // readings so the environmental screens can be exercised.
        _cache = { 22.0f, 55.0f, 1013.0f, true };
        _available = true;
        _usingSimulationFallback = true;
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[BME] Using Wokwi simulated environmental readings."));
#endif
        return true;
#else
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[BME] ERROR: sensor not found at 0x76 or 0x77."));
#endif
        return false;
#endif
    }

#ifdef ENABLE_DEBUG_SERIAL
    Serial.print(F("[BME] Initialised at 0x"));
    Serial.println(detectedAddress, HEX);
#endif

    _bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                     Adafruit_BME280::SAMPLING_X2,
                     Adafruit_BME280::SAMPLING_X16,
                     Adafruit_BME280::SAMPLING_X1,
                     Adafruit_BME280::FILTER_X16,
                     Adafruit_BME280::STANDBY_MS_500);
    update();
    return true;
}

void BmeManager::update()
{
    if (!_available) return;
    if (_usingSimulationFallback) return;

    float temperature = _bme.readTemperature();
    float humidity = _bme.readHumidity();
    float pressure = _bme.readPressure() / 100.0f;
    if (isnan(temperature) || isnan(humidity) || isnan(pressure))
    {
        _cache.valid = false;
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[BME] ERROR: invalid sensor reading."));
#endif
        return;
    }
    _cache = { temperature, humidity, pressure, true };
}

SensorData BmeManager::getData() { return _cache; }
float BmeManager::getTemperatureC() { return _cache.temperatureC; }
float BmeManager::getHumidityPercent() { return _cache.humidityPercent; }
float BmeManager::getPressureHPa() { return _cache.pressureHPa; }
bool BmeManager::isAvailable() { return _available && _cache.valid; }
