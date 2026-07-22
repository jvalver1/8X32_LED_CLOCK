/**
 * @file    clock_app.h
 * @brief   Main application state machine for the LED clock.
 *
 * @details ClockApp orchestrates all sub-systems:
 *          - Cycles through DisplayMode states
 *          - Handles button events (mode switch, config edits)
 *          - Drives periodic sensor & RTC reads
 *          - Formats and sends data to DisplayManager
 */

#pragma once

#include "../config.h"

// ---------------------------------------------------------------------------
// ClockApp (static class)
// ---------------------------------------------------------------------------
class ClockApp
{
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /** Initialise state machine and timers. */
    static void init();

    /**
     * @brief  Main update function – call every loop tick.
     *         Reads inputs, updates sensor/RTC caches, renders display.
     */
    static void update();

    // -----------------------------------------------------------------------
    // State accessors
    // -----------------------------------------------------------------------

    /** Return the current display mode. */
    static DisplayMode currentMode();

    /** Force a mode transition (used in tests or external triggers). */
    static void setMode(DisplayMode mode);

private:
    // Current display mode
    static DisplayMode _mode;

    // Timestamps for periodic tasks
    static uint32_t _lastRtcUpdate;
    static uint32_t _lastSensorUpdate;
    static uint32_t _lastAutoScroll;

    // Temporary configuration storage (used in SET_* modes)
    static uint8_t  _cfgHour;
    static uint8_t  _cfgMinute;
    static uint8_t  _cfgDay;
    static uint8_t  _cfgMonth;
    static uint16_t _cfgYear;

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /** Handle button events and update mode / config values. */
    static void handleButtons();

    /** Render the current mode to the display. */
    static void render();

    // Render helpers for each mode
    static void renderClock();
    static void renderDate();
    static void renderTemperature();
    static void renderPressure();
    static void renderBrightness();
    static void renderConfig();

    // Config mode helpers
    static void enterConfig();
    static void applyConfig();
    static void cancelConfig();

    ClockApp() = delete;
};
