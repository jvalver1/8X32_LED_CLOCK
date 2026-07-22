/**
 * @file    bmp_manager.h
 * @brief   BMP280 Manager – temperature and pressure sensor.
 *
 * @details Uses the Adafruit BMP280 library over I²C.
 *
 * @note    The BMP280 does NOT have a humidity sensor. If humidity is
 *          required, the hardware must be upgraded to a BME280 and this
 *          file updated accordingly (the pin-out and I²C address are
 *          identical, only the library differs).
 */

#pragma once

#include <stdint.h>
#include <Adafruit_BMP280.h>

// ---------------------------------------------------------------------------
// Sensor reading struct
// ---------------------------------------------------------------------------
struct SensorData
{
    float    temperatureC;  ///< Temperature in degrees Celsius
    float    pressureHPa;   ///< Pressure in hectopascals (hPa / mbar)
    bool     valid;         ///< false if sensor read failed
};

// ---------------------------------------------------------------------------
// BmpManager (static class)
// ---------------------------------------------------------------------------
class BmpManager
{
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief  Initialise the BMP280 over I²C.
     * @return true on success, false if sensor not found.
     */
    static bool init();

    /**
     * @brief  Read a new sample from the sensor and cache it.
     *         Call periodically (e.g., every 5 s) – not in every loop tick.
     */
    static void update();

    // -----------------------------------------------------------------------
    // Getters (from cached values)
    // -----------------------------------------------------------------------

    /** @return Latest cached sensor reading. */
    static SensorData getData();

    /** @return Temperature in °C (cached). */
    static float getTemperatureC();

    /** @return Temperature in °F (converted from cached °C). */
    static float getTemperatureF();

    /** @return Pressure in hPa (cached). */
    static float getPressureHPa();

    /** @return true if the sensor was found and last read was successful. */
    static bool isAvailable();

private:
    static Adafruit_BMP280 _bmp;
    static SensorData      _cache;
    static bool            _available;

    BmpManager() = delete;
};
