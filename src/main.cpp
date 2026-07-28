/**
 * @file    main.cpp
 * @brief   8x32 WS2812B LED Clock - Main entry point
 *
 * @details Main application loop for the LED clock. Coordinates:
 *          - Time display (fetched from DS3231 RTC)
 *          - Date display mode
 *          - Temperature, humidity and pressure display (from BME280)
 *          - Button handling for configuration
 *          - Display animations/transitions
 *
 * @hardware
 *   MCU  : Arduino Pro Mini (ATmega328P, 5 V @ 16 MHz)
 *   LEDs : 8x32 WS2812B matrix  → Pin D6
 *   RTC  : DS3231 (I2C)         → SDA=A4, SCL=A5
 *   Sens : BME280 (I2C)         → SDA=A4, SCL=A5  (addr 0x76)
 *   BTN1 : Mode   button        → Pin D3  (INT1, active LOW, pull-up)
 *   BTN2 : Up/Inc button        → Pin D4  (active LOW, pull-up)
 *   BTN3 : Down/Dec button      → Pin D5  (active LOW, pull-up)
 *
 * @note    BME280 provides all three environmental readings over I2C.
 *
 * @author  Your Name
 * @date    2026-07-21
 * @version 0.1.0
 */

#include <Arduino.h>

#include "config.h"
#include "display/display_manager.h"
#include "sensors/rtc_manager.h"
#include "sensors/bme_manager.h"
#include "input/button_manager.h"
#include "app/clock_app.h"

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
void initHardware();

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void setup()
{
#ifdef ENABLE_DEBUG_SERIAL
    Serial.begin(115200);
    Serial.println(F("=== 8x32 LED Clock v" VERSION_STRING " ==="));
    Serial.println(F("Initialising..."));
#endif

    initHardware();

    // Initialise sub-systems
    ButtonManager::init();
    RtcManager::init();
    BmeManager::init();
    DisplayManager::init();

    // Boot animation
    DisplayManager::playBootAnimation();

    ClockApp::init();

#ifdef ENABLE_DEBUG_SERIAL
    Serial.println(F("Ready."));
#endif
}

// ---------------------------------------------------------------------------
// loop()
// ---------------------------------------------------------------------------
void loop()
{
    // Poll hardware
    ButtonManager::update();

    // Run main application state machine
    ClockApp::update();

    // Render current frame to the LED matrix
    DisplayManager::render();
}

// ---------------------------------------------------------------------------
// initHardware()
// ---------------------------------------------------------------------------
void initHardware()
{
    // Button pins – internal pull-ups, active LOW
    pinMode(BTN_MODE_PIN,  INPUT_PULLUP);
    pinMode(BTN_UP_PIN,    INPUT_PULLUP);
    pinMode(BTN_DOWN_PIN,  INPUT_PULLUP);

#ifdef ENABLE_DEBUG_SERIAL
    Serial.println(F("[HW] GPIO configured."));
#endif
}
