/**
 * @file    rtc_manager.cpp
 * @brief   RtcManager implementation.
 */

#include "rtc_manager.h"
#include "../config.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
RTC_DS3231 RtcManager::_rtc;
TimeOfDay  RtcManager::_cachedTime = { 0, 0, 0 };
DateValue  RtcManager::_cachedDate = { 1, 1, 2026, 4 };
bool       RtcManager::_valid      = false;
bool       RtcManager::_dstActive  = false;

namespace
{
uint8_t lastSunday(uint16_t year, uint8_t month)
{
    DateTime lastDay(year, month, 31);
    return 31 - lastDay.dayOfTheWeek();
}
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
bool RtcManager::init()
{
    if (!_rtc.begin())
    {
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[RTC] ERROR: DS3231 not found on I2C bus!"));
        Serial.println(F("[RTC] Check SDA(A4)/SCL(A5) connections and pull-up resistors."));
#endif
        _valid = false;
        return false;
    }

    if (_rtc.lostPower())
    {
#ifdef ENABLE_DEBUG_SERIAL
        Serial.println(F("[RTC] WARNING: RTC lost power. Setting default 2026-01-01 00:00:00."));
#endif
        // A new/unconfigured RTC always starts from the documented baseline.
        // Subsequent boots retain the value stored by the battery-backed RTC.
        _rtc.adjust(DateTime(2026, 1, 1, 0, 0, 0));
    }

    _valid = true;
    update(); // Populate cache immediately

#ifdef ENABLE_DEBUG_SERIAL
    Serial.print(F("[RTC] Initialised. Time: "));
    Serial.print(_cachedTime.hour);
    Serial.print(':');
    Serial.print(_cachedTime.minute);
    Serial.print(':');
    Serial.println(_cachedTime.second);
#endif

    return true;
}

void RtcManager::update()
{
    if (!_valid) return;

    // The DS3231 is authoritative and always stores standard time. DST is
    // recalculated on every poll, so a new day and its transition are applied
    // as soon as the RTC date changes.
    DateTime standardTime = _rtc.now();
    _dstActive = calculateEuropeanDst(standardTime.year(),
                                      standardTime.month(),
                                      standardTime.day());
    DateTime localTime = _dstActive
                             ? standardTime + TimeSpan(0, 1, 0, 0)
                             : standardTime;
    _cachedTime = { localTime.hour(), localTime.minute(), localTime.second() };
    _cachedDate = { localTime.day(), localTime.month(), localTime.year(),
                    localTime.dayOfTheWeek() };
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
TimeOfDay RtcManager::getTime() { return _cachedTime; }
DateValue RtcManager::getDate() { return _cachedDate; }
bool      RtcManager::isValid() { return _valid; }
bool      RtcManager::isDstActive() { return _dstActive; }

bool RtcManager::calculateEuropeanDst(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 3 || month > 10) return false;
    if (month > 3 && month < 10) return true;
    if (month == 3) return day >= lastSunday(year, 3);
    return day < lastSunday(year, 10);
}

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
bool RtcManager::setTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    if (!_valid) return false;
    return setDateTime(_cachedDate.year, _cachedDate.month, _cachedDate.day,
                       hour, minute, second);
}

bool RtcManager::setDate(uint8_t day, uint8_t month, uint16_t year)
{
    if (!_valid) return false;
    return setDateTime(year, month, day, _cachedTime.hour,
                       _cachedTime.minute, _cachedTime.second);
}

bool RtcManager::setDateTime(uint16_t year, uint8_t month, uint8_t day,
                             uint8_t hour, uint8_t minute, uint8_t second)
{
    if (!_valid) return false;

    DateTime localTime(year, month, day, hour, minute, second);
    bool enteredTimeIsDst = calculateEuropeanDst(year, month, day);
    DateTime standardTime = enteredTimeIsDst
                                ? localTime - TimeSpan(0, 1, 0, 0)
                                : localTime;
    _rtc.adjust(standardTime);

    // RTClib adjust() has no status result, so verify the hardware registers
    // immediately by reading the stored fields back.
    DateTime stored = _rtc.now();
    uint32_t storedSeconds = stored.unixtime();
    uint32_t expectedSeconds = standardTime.unixtime();
    bool written = storedSeconds == expectedSeconds ||
                   storedSeconds == expectedSeconds + 1;
#ifdef ENABLE_DEBUG_SERIAL
    Serial.println(written ? F("[RTC] Date/time write verified.")
                           : F("[RTC] ERROR: Date/time write verification failed."));
#endif
    update();
    return written;
}
