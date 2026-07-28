/** @file bme_manager.h @brief Cached BME280 environmental readings. */
#pragma once

#include <stdint.h>
#include <Adafruit_BME280.h>

struct SensorData
{
    float temperatureC;
    float humidityPercent;
    float pressureHPa;
    bool valid;
};

class BmeManager
{
public:
    static bool init();
    static void update();
    static SensorData getData();
    static float getTemperatureC();
    static float getHumidityPercent();
    static float getPressureHPa();
    static bool isAvailable();

private:
    static Adafruit_BME280 _bme;
    static SensorData _cache;
    static bool _available;
    static bool _usingSimulationFallback;
    BmeManager() = delete;
};
