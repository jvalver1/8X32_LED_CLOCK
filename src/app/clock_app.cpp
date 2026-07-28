/**
 * @file    clock_app.cpp
 * @brief   ClockApp state machine implementation.
 */

#include "clock_app.h"
#include "../config.h"
#include "../display/display_manager.h"
#include "../sensors/rtc_manager.h"
#include "../sensors/bme_manager.h"
#include "../input/button_manager.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace
{
void drawCenteredNumeric(const char* value, CRGB color)
{
    uint8_t count = strlen(value);
    uint8_t width = count ? count * 4 - 1 : 0; // 3px glyph + 1px spacing
    int16_t x = (MATRIX_WIDTH - width) / 2;
    DisplayManager::drawString(x, 1, value, color);
}

void drawEnvironmentalReading(const char* value, EnvironmentalIcon icon)
{
    const uint8_t textStart = 9; // Columns 0..7 icon, column 8 spacer.
    uint8_t count = strlen(value);
    uint8_t width = count ? count * 4 - 1 : 0;
    int16_t x = textStart + (MATRIX_WIDTH - textStart - width) / 2;
    DisplayManager::drawEnvironmentalIcon(icon);
    DisplayManager::drawString(x, 1, value, DisplayManager::fontColor());
}

// Temporary 3x5 pixel icon used only by the 12/24-hour setup screen.
// Kept out of the shared font because it is a UI symbol, not text.
void drawHourFormatIcon(int16_t x, int16_t y, CRGB color)
{
    DisplayManager::setPixel(x,     y,     color);
    DisplayManager::setPixel(x,     y + 1, color);
    DisplayManager::setPixel(x,     y + 2, color);
    DisplayManager::setPixel(x,     y + 3, color);
    DisplayManager::setPixel(x,     y + 4, color);
    DisplayManager::setPixel(x + 1, y + 2, color);
    DisplayManager::setPixel(x + 2, y + 2, color);
    DisplayManager::setPixel(x + 2, y + 3, color);
    DisplayManager::setPixel(x + 2, y + 4, color);
}
}

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
DisplayMode ClockApp::_mode           = DisplayMode::CLOCK;
uint32_t    ClockApp::_lastRtcUpdate  = 0;
uint32_t    ClockApp::_lastSensorUpdate = 0;
uint32_t    ClockApp::_lastAutoScroll = 0;
uint32_t    ClockApp::_carouselDelay   = 0;
uint8_t     ClockApp::_cfgHour        = 0;
uint8_t     ClockApp::_cfgMinute      = 0;
uint8_t     ClockApp::_cfgDay         = 1;
uint8_t     ClockApp::_cfgMonth       = 1;
uint16_t    ClockApp::_cfgYear        = 2026;
bool        ClockApp::_cfg12Hour      = CLOCK_12H_FORMAT != 0;
uint8_t     ClockApp::_cfgBrightness  = 1;
bool        ClockApp::_use12Hour      = CLOCK_12H_FORMAT != 0;
uint32_t    ClockApp::_lastSetupActivity = 0;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void ClockApp::init()
{
    _mode             = DisplayMode::CLOCK;
    _lastRtcUpdate    = 0;
    _lastSensorUpdate = 0;
    _lastAutoScroll   = millis();
    _carouselDelay    = 0;
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
        BmeManager::update();
    }

    handleButtons();
    if (isSetupMode() && (now - _lastSetupActivity) >= SETUP_TIMEOUT_MS)
        applyConfig();

    // Runtime carousel. Handle buttons first so entering setup or stopping
    // the carousel takes effect immediately when a dwell timer also expires.
    if (_carouselDelay != 0 && !isSetupMode() &&
        (now - _lastAutoScroll) >= _carouselDelay)
    {
        _lastAutoScroll = now;
        DisplayManager::capturePullDownFrame();
        advanceCarousel();
        for (uint8_t progress = 1; progress <= MATRIX_HEIGHT; progress++)
        {
            render();
            DisplayManager::composePullDownFrame(progress);
            DisplayManager::render();
            delay(PULL_DOWN_ROW_DELAY_MS);
        }
    }

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
    if (isSetupMode() && (evMode != ButtonEvent::NONE ||
                          evUp != ButtonEvent::NONE ||
                          evDown != ButtonEvent::NONE))
        _lastSetupActivity = millis();

    // ---- Left MODE button: enter/advance configuration ----
    if (evMode == ButtonEvent::SHORT_PRESS)
    {
        if (_mode < DisplayMode::SET_HOUR)
        {
            DisplayManager::cycleFontColor();
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
        if (!isSetupMode())
        {
            _carouselDelay = 0;
            enterConfig();      // Enter config mode
        }
        else
            applyConfig();      // Save and return to clock
    }

    // ---- UP / DOWN buttons ----
    if (!isSetupMode() &&
        (evUp == ButtonEvent::LONG_PRESS || evDown == ButtonEvent::LONG_PRESS))
    {
        cycleCarousel();
        return;
    }

    if (evUp == ButtonEvent::SHORT_PRESS || evUp == ButtonEvent::REPEAT)
    {
        if (_mode <= DisplayMode::PRESSURE)
        {
            // REPEAT follows a long press; in normal display mode the long
            // press belongs exclusively to carousel control.
            if (evUp == ButtonEvent::REPEAT)
                return;

            // Centre button: reverse through the normal display cycle.
            uint8_t current = static_cast<uint8_t>(_mode);
            uint8_t last = static_cast<uint8_t>(DisplayMode::PRESSURE);
            _mode = static_cast<DisplayMode>(current == 0 ? last : current - 1);
            DisplayManager::playTransition();
            _lastAutoScroll = millis();
            return;
        }
        switch (_mode)
        {
            case DisplayMode::SET_HOUR:   _cfgHour = (_cfgHour + 1) % 24; break;
            case DisplayMode::SET_MINUTE: _cfgMinute = (_cfgMinute + 1) % 60; break;
            case DisplayMode::SET_DAY:
                _cfgDay = (_cfgDay % daysInMonth(_cfgMonth, _cfgYear)) + 1; break;
            case DisplayMode::SET_MONTH:
                _cfgMonth = (_cfgMonth % 12) + 1; clampConfigDay(); break;
            case DisplayMode::SET_YEAR:
                _cfgYear = (_cfgYear >= 2099) ? 2000 : _cfgYear + 1;
                clampConfigDay(); break;
            case DisplayMode::SET_FORMAT: _cfg12Hour = !_cfg12Hour; break;
            case DisplayMode::SET_BRIGHTNESS:
                _cfgBrightness = (_cfgBrightness % 10) + 1;
                DisplayManager::setBrightnessLevel(_cfgBrightness);
                break;
            default: break;
        }
    }

    if (evDown == ButtonEvent::SHORT_PRESS || evDown == ButtonEvent::REPEAT)
    {
        if (_mode <= DisplayMode::PRESSURE)
        {
            if (evDown == ButtonEvent::REPEAT)
                return;

            // Rightmost button: advance through the normal display cycle.
            uint8_t next = (static_cast<uint8_t>(_mode) + 1) %
                           (static_cast<uint8_t>(DisplayMode::PRESSURE) + 1);
            _mode = static_cast<DisplayMode>(next);
            DisplayManager::playTransition();
            _lastAutoScroll = millis();
            return;
        }
        switch (_mode)
        {
            case DisplayMode::SET_HOUR:   _cfgHour = (_cfgHour + 23) % 24; break;
            case DisplayMode::SET_MINUTE: _cfgMinute = (_cfgMinute + 59) % 60; break;
            case DisplayMode::SET_DAY:
                _cfgDay = (_cfgDay > 1) ? _cfgDay - 1 :
                          daysInMonth(_cfgMonth, _cfgYear); break;
            case DisplayMode::SET_MONTH:
                _cfgMonth = (_cfgMonth > 1) ? _cfgMonth - 1 : 12;
                clampConfigDay(); break;
            case DisplayMode::SET_YEAR:
                _cfgYear = (_cfgYear <= 2000) ? 2099 : _cfgYear - 1;
                clampConfigDay(); break;
            case DisplayMode::SET_FORMAT: _cfg12Hour = !_cfg12Hour; break;
            case DisplayMode::SET_BRIGHTNESS:
                _cfgBrightness = (_cfgBrightness > 1) ? _cfgBrightness - 1 : 10;
                DisplayManager::setBrightnessLevel(_cfgBrightness);
                break;
            default: break;
        }
    }
}

void ClockApp::cycleCarousel()
{
    if (_carouselDelay == 0)
        _carouselDelay = CAROUSEL_SHORT_DELAY_MS;
    else if (_carouselDelay == CAROUSEL_SHORT_DELAY_MS)
        _carouselDelay = CAROUSEL_LONG_DELAY_MS;
    else
        _carouselDelay = 0;

    // A newly selected delay always starts from the screen currently visible.
    _lastAutoScroll = millis();
}

void ClockApp::advanceCarousel()
{
    switch (_mode)
    {
        case DisplayMode::CLOCK:       _mode = DisplayMode::TEMPERATURE; break;
        case DisplayMode::TEMPERATURE: _mode = DisplayMode::HUMIDITY;    break;
        case DisplayMode::HUMIDITY:    _mode = DisplayMode::PRESSURE;    break;
        default:                       _mode = DisplayMode::CLOCK;       break;
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
    _cfg12Hour = _use12Hour;
    _cfgBrightness = DisplayManager::getBrightnessLevel();
    _mode      = DisplayMode::SET_HOUR;
    _lastSetupActivity = millis();
    DisplayManager::playTransition();
}

void ClockApp::applyConfig()
{
    clampConfigDay();
    _use12Hour = _cfg12Hour;
    DisplayManager::setBrightnessLevel(_cfgBrightness);
    RtcManager::setDateTime(_cfgYear, _cfgMonth, _cfgDay,
                            _cfgHour, _cfgMinute, 0);
    _mode = DisplayMode::CLOCK;
    DisplayManager::playTransition();
}

bool ClockApp::isSetupMode()
{
    return _mode >= DisplayMode::SET_HOUR && _mode < DisplayMode::NUM_MODES;
}

uint8_t ClockApp::daysInMonth(uint8_t month, uint16_t year)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30,
                                   31, 31, 30, 31, 30, 31};
    if (month == 2)
    {
        bool leap = (year % 4 == 0) &&
                    ((year % 100 != 0) || (year % 400 == 0));
        return leap ? 29 : 28;
    }
    return days[month - 1];
}

void ClockApp::clampConfigDay()
{
    uint8_t maximum = daysInMonth(_cfgMonth, _cfgYear);
    if (_cfgDay > maximum) _cfgDay = maximum;
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
        case DisplayMode::HUMIDITY:    renderHumidity();    break;
        case DisplayMode::PRESSURE:    renderPressure();    break;
        default:                       renderConfig();      break;
    }

    // A blinking bottom-right pixel indicates that automatic carousel mode
    // is active. clear() above ensures it disappears immediately when stopped.
    if (_carouselDelay != 0 &&
        ((millis() / CAROUSEL_INDICATOR_FLASH_MS) & 1U) == 0)
    {
        CRGB indicator = (_carouselDelay == CAROUSEL_SHORT_DELAY_MS)
                             ? CRGB(255, 96, 0)
                             : CRGB::Red;
        DisplayManager::setPixel(MATRIX_WIDTH - 1, MATRIX_HEIGHT - 1,
                                 indicator);
    }
}

// ---------------------------------------------------------------------------
// Mode renderers
// ---------------------------------------------------------------------------
void ClockApp::renderClock()
{
    TimeOfDay t = RtcManager::getTime();
    DateValue d = RtcManager::getDate();

    uint8_t displayHour = t.hour;
    if (RtcManager::isDstActive())
        displayHour = (displayHour + 1) % 24;
    if (_use12Hour)
    {
        displayHour %= 12;
        if (displayHour == 0) displayHour = 12;
    }

    // Synchronise the separator directly to the RTC: on for even seconds,
    // off for odd seconds. This prevents drift from the displayed time.
    DisplayManager::drawClockScreen(d.day, d.weekday, displayHour, t.minute,
                                    (t.second & 1) == 0);
}

void ClockApp::renderDate()
{
    DateValue d = RtcManager::getDate();
    char buf[9]; // "DD/MM/YY\0"
    snprintf(buf, sizeof(buf), "%02u/%02u/%02u", d.day, d.month, d.year % 100);
    drawCenteredNumeric(buf, DisplayManager::fontColor());
}

void ClockApp::renderTemperature()
{
    if (!BmeManager::isAvailable())
    {
        return;
    }
    char buf[7];
    float t = BmeManager::getTemperatureC();
    snprintf(buf, sizeof(buf), "%doC", (int)t);
    drawEnvironmentalReading(buf, EnvironmentalIcon::TEMPERATURE);
}

void ClockApp::renderHumidity()
{
    if (!BmeManager::isAvailable())
        return;

    char buf[5];
    snprintf(buf, sizeof(buf), "%u%%", (unsigned)BmeManager::getHumidityPercent());
    drawEnvironmentalReading(buf, EnvironmentalIcon::HUMIDITY);
}

void ClockApp::renderPressure()
{
    if (!BmeManager::isAvailable())
    {
        return;
    }
    char buf[6];
    snprintf(buf, sizeof(buf), "%d", (int)BmeManager::getPressureHPa());
    drawEnvironmentalReading(buf, EnvironmentalIcon::PRESSURE);
}

void ClockApp::renderConfig()
{
    const bool hideSelected = ((millis() / SETUP_FLASH_MS) & 1U) != 0;
    char value[9];
    CRGB color = DisplayManager::fontColor();

    switch (_mode)
    {
        case DisplayMode::SET_HOUR:
            snprintf(value, sizeof(value), "%02u:%02u", _cfgHour, _cfgMinute);
            if (hideSelected) value[0] = value[1] = ' ';
            break;
        case DisplayMode::SET_MINUTE:
            snprintf(value, sizeof(value), "%02u:%02u", _cfgHour, _cfgMinute);
            if (hideSelected) value[3] = value[4] = ' ';
            break;
        case DisplayMode::SET_DAY:
            snprintf(value, sizeof(value), "%02u/%02u/%02u",
                     _cfgDay, _cfgMonth, _cfgYear % 100);
            if (hideSelected) value[0] = value[1] = ' ';
            break;
        case DisplayMode::SET_MONTH:
            snprintf(value, sizeof(value), "%02u/%02u/%02u",
                     _cfgDay, _cfgMonth, _cfgYear % 100);
            if (hideSelected) value[3] = value[4] = ' ';
            break;
        case DisplayMode::SET_YEAR:
            snprintf(value, sizeof(value), "%02u/%02u/%02u",
                     _cfgDay, _cfgMonth, _cfgYear % 100);
            if (hideSelected) value[6] = value[7] = ' ';
            break;
        case DisplayMode::SET_FORMAT:
            drawHourFormatIcon(20, 1, color);
            if (hideSelected)
                return;
            snprintf(value, sizeof(value), "%s", _cfg12Hour ? "12" : "24");
            DisplayManager::drawString(10, 1, value, color);
            return;
        case DisplayMode::SET_BRIGHTNESS:
            if (hideSelected)
                return;
            snprintf(value, sizeof(value), "%u", _cfgBrightness);
            break;
        default:
            return;
    }

    drawCenteredNumeric(value, color);
}

// ---------------------------------------------------------------------------
// State accessors
// ---------------------------------------------------------------------------
DisplayMode ClockApp::currentMode() { return _mode; }
void ClockApp::setMode(DisplayMode mode)
{
    _mode = mode;
    if (isSetupMode())
        _carouselDelay = 0;
    DisplayManager::playTransition();
}
