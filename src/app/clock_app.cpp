/**
 * @file    clock_app.cpp
 * @brief   ClockApp state machine implementation.
 */

#include "clock_app.h"
#include "../config.h"
#include "../display/display_manager.h"
#include "../sensors/rtc_manager.h"
#include "../sensors/bmp_manager.h"
#include "../input/button_manager.h"
#include <Arduino.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
DisplayMode ClockApp::_mode           = DisplayMode::CLOCK;
uint32_t    ClockApp::_lastRtcUpdate  = 0;
uint32_t    ClockApp::_lastSensorUpdate = 0;
uint32_t    ClockApp::_lastAutoScroll = 0;
uint8_t     ClockApp::_cfgHour        = 0;
uint8_t     ClockApp::_cfgMinute      = 0;
uint8_t     ClockApp::_cfgDay         = 1;
uint8_t     ClockApp::_cfgMonth       = 1;
uint16_t    ClockApp::_cfgYear        = 2026;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void ClockApp::init()
{
    _mode             = DisplayMode::CLOCK;
    _lastRtcUpdate    = 0;
    _lastSensorUpdate = 0;
    _lastAutoScroll   = millis();
}

// ---------------------------------------------------------------------------
// update() – main state machine tick
// ---------------------------------------------------------------------------
void ClockApp::update()
{
    uint32_t now = millis();

    // -- Periodic RTC poll --
    if (now - _lastRtcUpdate >= RTC_POLL_MS)
    {
        _lastRtcUpdate = now;
        RtcManager::update();
    }

    // -- Periodic sensor poll --
    if (now - _lastSensorUpdate >= SENSOR_POLL_MS)
    {
        _lastSensorUpdate = now;
        BmpManager::update();
    }

#ifdef FEATURE_AUTO_SCROLL
    // Auto-scroll in normal display modes (not config modes)
    if (_mode <= DisplayMode::PRESSURE &&
        (now - _lastAutoScroll) >= AUTO_SCROLL_DURATION_MS)
    {
        _lastAutoScroll = now;
        uint8_t next = (static_cast<uint8_t>(_mode) + 1) %
                       (static_cast<uint8_t>(DisplayMode::PRESSURE) + 1);
        _mode = static_cast<DisplayMode>(next);
        DisplayManager::playTransition();
    }
#endif

    handleButtons();
    render();
}

// ---------------------------------------------------------------------------
// Button handling
// ---------------------------------------------------------------------------
void ClockApp::handleButtons()
{
    ButtonEvent evMode = ButtonManager::getEvent(Button::MODE);
    ButtonEvent evUp   = ButtonManager::getEvent(Button::UP);
    ButtonEvent evDown = ButtonManager::getEvent(Button::DOWN);

    // ---- MODE button ----
    if (evMode == ButtonEvent::SHORT_PRESS)
    {
        // Cycle through display modes (normal modes only)
        if (_mode < DisplayMode::BRIGHTNESS)
        {
            uint8_t next = (static_cast<uint8_t>(_mode) + 1) %
                           (static_cast<uint8_t>(DisplayMode::BRIGHTNESS) + 1);
            _mode = static_cast<DisplayMode>(next);
            DisplayManager::playTransition();
            _lastAutoScroll = millis();
        }
        else if (_mode == DisplayMode::BRIGHTNESS)
        {
            _mode = DisplayMode::CLOCK;
            DisplayManager::playTransition();
        }
        else
        {
            // In config mode: advance to next config field or apply
            uint8_t next = static_cast<uint8_t>(_mode) + 1;
            if (next >= static_cast<uint8_t>(DisplayMode::NUM_MODES))
                applyConfig();
            else
                _mode = static_cast<DisplayMode>(next);
        }
    }

    if (evMode == ButtonEvent::LONG_PRESS)
    {
        if (_mode < DisplayMode::SET_HOUR)
            enterConfig();      // Enter config mode
        else
            cancelConfig();     // Cancel and return to clock
    }

    // ---- UP / DOWN buttons ----
    if (evUp == ButtonEvent::SHORT_PRESS || evUp == ButtonEvent::REPEAT)
    {
        switch (_mode)
        {
            case DisplayMode::BRIGHTNESS: DisplayManager::brightnessUp();  break;
            case DisplayMode::SET_HOUR:   _cfgHour   = (_cfgHour   + 1) % 24;    break;
            case DisplayMode::SET_MINUTE: _cfgMinute = (_cfgMinute + 1) % 60;    break;
            case DisplayMode::SET_DAY:    _cfgDay    = (_cfgDay    % 31) + 1;    break;
            case DisplayMode::SET_MONTH:  _cfgMonth  = (_cfgMonth  % 12) + 1;    break;
            case DisplayMode::SET_YEAR:   _cfgYear++;                             break;
            default: break;
        }
    }

    if (evDown == ButtonEvent::SHORT_PRESS || evDown == ButtonEvent::REPEAT)
    {
        switch (_mode)
        {
            case DisplayMode::BRIGHTNESS: DisplayManager::brightnessDown(); break;
            case DisplayMode::SET_HOUR:   _cfgHour   = (_cfgHour   + 23) % 24;  break;
            case DisplayMode::SET_MINUTE: _cfgMinute = (_cfgMinute + 59) % 60;  break;
            case DisplayMode::SET_DAY:    _cfgDay    = (_cfgDay > 1) ? _cfgDay - 1 : 31; break;
            case DisplayMode::SET_MONTH:  _cfgMonth  = (_cfgMonth > 1) ? _cfgMonth - 1 : 12; break;
            case DisplayMode::SET_YEAR:   if (_cfgYear > 2000) _cfgYear--;       break;
            default: break;
        }
    }
}

// ---------------------------------------------------------------------------
// Config helpers
// ---------------------------------------------------------------------------
void ClockApp::enterConfig()
{
    TimeOfDay t = RtcManager::getTime();
    DateValue d = RtcManager::getDate();
    _cfgHour   = t.hour;
    _cfgMinute = t.minute;
    _cfgDay    = d.day;
    _cfgMonth  = d.month;
    _cfgYear   = d.year;
    _mode      = DisplayMode::SET_HOUR;
    DisplayManager::playTransition();
}

void ClockApp::applyConfig()
{
    RtcManager::setTime(_cfgHour, _cfgMinute, 0);
    RtcManager::setDate(_cfgDay, _cfgMonth, _cfgYear);
    _mode = DisplayMode::CLOCK;
    DisplayManager::playTransition();
}

void ClockApp::cancelConfig()
{
    _mode = DisplayMode::CLOCK;
    DisplayManager::playTransition();
}

// ---------------------------------------------------------------------------
// Render dispatch
// ---------------------------------------------------------------------------
void ClockApp::render()
{
    DisplayManager::clear();

    switch (_mode)
    {
        case DisplayMode::CLOCK:       renderClock();       break;
        case DisplayMode::DATE:        renderDate();        break;
        case DisplayMode::TEMPERATURE: renderTemperature(); break;
        case DisplayMode::PRESSURE:    renderPressure();    break;
        case DisplayMode::BRIGHTNESS:  renderBrightness();  break;
        default:                       renderConfig();      break;
    }
}

// ---------------------------------------------------------------------------
// Mode renderers
// ---------------------------------------------------------------------------
void ClockApp::renderClock()
{
    TimeOfDay t = RtcManager::getTime();
    DateValue d = RtcManager::getDate();
    // Synchronise the separator directly to the RTC: on for even seconds,
    // off for odd seconds. This prevents drift from the displayed time.
    DisplayManager::drawClockScreen(d.day, d.weekday, t.hour, t.minute,
                                    (t.second & 1) == 0);
}

void ClockApp::renderDate()
{
    DateValue d = RtcManager::getDate();
    char buf[9]; // "DD/MM/YY\0"
    snprintf(buf, sizeof(buf), "%02u/%02u/%02u", d.day, d.month, d.year % 100);
    DisplayManager::scrollText(buf, CRGB(COLOR_DATE_DEFAULT), 40);
}

void ClockApp::renderTemperature()
{
    if (!BmpManager::isAvailable())
    {
        DisplayManager::drawString(0, 0, "N/A", CRGB::Red);
        return;
    }
    char buf[8]; // "-XX.XC\0"
    float t = BmpManager::getTemperatureC();
    snprintf(buf, sizeof(buf), "%d.%dC", (int)t, abs((int)(t * 10) % 10));
    DisplayManager::scrollText(buf, CRGB(COLOR_TEMP_DEFAULT), 40);
}

void ClockApp::renderPressure()
{
    if (!BmpManager::isAvailable())
    {
        DisplayManager::drawString(0, 0, "N/A", CRGB::Red);
        return;
    }
    char buf[10]; // "XXXX hPa\0"
    snprintf(buf, sizeof(buf), "%dhPa", (int)BmpManager::getPressureHPa());
    DisplayManager::scrollText(buf, CRGB(COLOR_PRESSURE_DEFAULT), 40);
}

void ClockApp::renderBrightness()
{
    char buf[5];
    snprintf(buf, sizeof(buf), "B%3u", DisplayManager::getBrightness());
    DisplayManager::drawString(0, 0, buf, CRGB::Yellow);
}

void ClockApp::renderConfig()
{
    char label[3];
    char value[6];
    CRGB color = CRGB(COLOR_CONFIG_DEFAULT);

    switch (_mode)
    {
        case DisplayMode::SET_HOUR:
            snprintf(label, sizeof(label), "H");
            snprintf(value, sizeof(value), "%02u", _cfgHour);
            break;
        case DisplayMode::SET_MINUTE:
            snprintf(label, sizeof(label), "M");
            snprintf(value, sizeof(value), "%02u", _cfgMinute);
            break;
        case DisplayMode::SET_DAY:
            snprintf(label, sizeof(label), "D");
            snprintf(value, sizeof(value), "%02u", _cfgDay);
            break;
        case DisplayMode::SET_MONTH:
            snprintf(label, sizeof(label), "Mo");
            snprintf(value, sizeof(value), "%02u", _cfgMonth);
            break;
        case DisplayMode::SET_YEAR:
            snprintf(label, sizeof(label), "Y");
            snprintf(value, sizeof(value), "%u", _cfgYear % 100);
            break;
        default:
            return;
    }

    DisplayManager::drawString(0, 0, label, CRGB::White);
    DisplayManager::drawString(16, 0, value, color);
}

// ---------------------------------------------------------------------------
// State accessors
// ---------------------------------------------------------------------------
DisplayMode ClockApp::currentMode() { return _mode; }
void ClockApp::setMode(DisplayMode mode)
{
    _mode = mode;
    DisplayManager::playTransition();
}
