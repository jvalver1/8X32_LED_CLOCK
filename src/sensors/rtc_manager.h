/**
 * @file    rtc_manager.h
 * @brief   RTC Manager – wrapper around the DS3231 real-time clock.
 *
 * @details Uses the Adafruit RTClib library. Provides:
 *          - Time/date read with caching (avoids hammering I²C every loop)
 *          - Time/date write for configuration
 *          - Lost-power / invalid-time detection
 */

#pragma once

#include <stdint.h>
#include <RTClib.h>

// ---------------------------------------------------------------------------
// Simple time-of-day struct (avoids pulling in DateTime everywhere)
// ---------------------------------------------------------------------------
struct TimeOfDay
{
    uint8_t  hour;    ///< 0–23
    uint8_t  minute;  ///< 0–59
    uint8_t  second;  ///< 0–59
};

struct DateValue
{
    uint8_t  day;     ///< 1–31
    uint8_t  month;   ///< 1–12
    uint16_t year;    ///< e.g. 2026
    uint8_t  weekday; ///< 0=Sunday … 6=Saturday
};

// ---------------------------------------------------------------------------
// RtcManager (static class)
// ---------------------------------------------------------------------------
class RtcManager
{
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief  Initialise the DS3231 over I²C.
     * @return true on success, false if RTC not found or power was lost.
     */
    static bool init();

    /**
     * @brief  Poll the RTC and update the internal cache.
     *         Call periodically (e.g., every second).
     */
    static void update();

    // -----------------------------------------------------------------------
    // Getters (from cached values – no I²C traffic)
    // -----------------------------------------------------------------------

    static TimeOfDay getTime();
    static DateValue getDate();

    /** @return true if the RTC has valid time (no power-loss event). */
    static bool isValid();
    static bool isDstActive();

    // -----------------------------------------------------------------------
    // Setters (write to RTC via I²C)
    // -----------------------------------------------------------------------

    static void setTime(uint8_t hour, uint8_t minute, uint8_t second);
    static void setDate(uint8_t day, uint8_t month, uint16_t year);
    static void setDateTime(uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second);
    static bool calculateEuropeanDst(uint16_t year, uint8_t month, uint8_t day);

private:
    static RTC_DS3231 _rtc;
    static TimeOfDay  _cachedTime;
    static DateValue  _cachedDate;
    static bool       _valid;
    static bool       _dstActive;

    RtcManager() = delete;
};
