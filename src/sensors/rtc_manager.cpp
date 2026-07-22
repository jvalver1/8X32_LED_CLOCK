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
DateValue  RtcManager::_cachedDate = { 1, 1, 2000, 6 };
bool       RtcManager::_valid      = false;

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
        Serial.println(F("[RTC] WARNING: RTC lost power. Setting default 2000-01-01 00:00:00."));
#endif
        // A new/unconfigured RTC always starts from the documented baseline.
        // Subsequent boots retain the value stored by the battery-backed RTC.
        _rtc.adjust(DateTime(2000, 1, 1, 0, 0, 0));
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

    DateTime now = _rtc.now();
    _cachedTime  = { now.hour(), now.minute(), now.second() };
    _cachedDate  = { now.day(), now.month(), now.year(), now.dayOfTheWeek() };
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
TimeOfDay RtcManager::getTime() { return _cachedTime; }
DateValue RtcManager::getDate() { return _cachedDate; }
bool      RtcManager::isValid() { return _valid; }

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------
void RtcManager::setTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    if (!_valid) return;
    DateTime current = _rtc.now();
    _rtc.adjust(DateTime(current.year(), current.month(), current.day(),
                         hour, minute, second));
    update();
}

void RtcManager::setDate(uint8_t day, uint8_t month, uint16_t year)
{
    if (!_valid) return;
    DateTime current = _rtc.now();
    _rtc.adjust(DateTime(year, month, day,
                         current.hour(), current.minute(), current.second()));
    update();
}
