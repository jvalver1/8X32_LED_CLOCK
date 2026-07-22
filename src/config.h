/**
 * @file    config.h
 * @brief   Global compile-time configuration for the 8x32 LED Clock.
 *
 * @details Central place for all hardware pin definitions, timing constants,
 *          display parameters, and feature flags.  Most values are overridable
 *          via platformio.ini build_flags so they do not need to be changed
 *          here for different hardware revisions.
 */

#pragma once

#include <stdint.h>

// ============================================================
// Version
// ============================================================
#define VERSION_MAJOR  0
#define VERSION_MINOR  1
#define VERSION_PATCH  0
#define VERSION_STRING "0.1.0"

// ============================================================
// LED Matrix
// ============================================================
#ifndef LED_DATA_PIN
  #define LED_DATA_PIN   6      ///< Arduino Nano digital pin for WS2812B data
#endif

#ifndef NUM_LEDS
  #define NUM_LEDS       256    ///< 8 rows × 32 columns
#endif

#ifndef MATRIX_WIDTH
  #define MATRIX_WIDTH   32
#endif

#ifndef MATRIX_HEIGHT
  #define MATRIX_HEIGHT  8
#endif

/** LED type selection – change if using a different addressable LED chipset */
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB

/** Default brightness (0-255). Keep low to save power on USB. */
#define DEFAULT_BRIGHTNESS   60
#define MAX_BRIGHTNESS       200
#define MIN_BRIGHTNESS       5

// ============================================================
// Buttons  (active LOW, internal pull-up)
// ============================================================
#ifndef BTN_MODE_PIN
  #define BTN_MODE_PIN   2     ///< Mode/Select – supports hardware INT0
#endif

#ifndef BTN_UP_PIN
  #define BTN_UP_PIN     3     ///< Up/Increment – supports hardware INT1
#endif

#ifndef BTN_DOWN_PIN
  #define BTN_DOWN_PIN   4     ///< Down/Decrement
#endif

/** Debounce time in milliseconds */
#define BTN_DEBOUNCE_MS        50
/** Long-press threshold in milliseconds */
#define BTN_LONG_PRESS_MS      800
/** Auto-repeat interval when held (ms) */
#define BTN_REPEAT_INTERVAL_MS 150

// ============================================================
// I²C Peripherals
// ============================================================
/** DS3231 RTC – fixed address, cannot be changed */
#define DS3231_I2C_ADDR        0x68

/** BMP280 I²C address (0x76 when SDO=GND, 0x77 when SDO=VCC) */
#ifndef BMP280_I2C_ADDR
  #define BMP280_I2C_ADDR      0x76
#endif

// ============================================================
// LED Matrix Layout
// ============================================================
#define MATRIX_LAYOUT_SERPENTINE_COLUMN_MAJOR 0  ///< Vertical serpentine (Column-major)
#define MATRIX_LAYOUT_SERPENTINE_ROW_MAJOR    1  ///< Horizontal serpentine (Row-major)
#define MATRIX_LAYOUT_PROGRESSIVE_ROW_MAJOR   2  ///< Horizontal progressive (Wokwi default)

#ifndef MATRIX_LAYOUT
  #ifdef WOKWI_SIMULATION
    #define MATRIX_LAYOUT MATRIX_LAYOUT_PROGRESSIVE_ROW_MAJOR
  #else
    #define MATRIX_LAYOUT MATRIX_LAYOUT_SERPENTINE_COLUMN_MAJOR
  #endif
#endif

// ============================================================
// Timing
// ============================================================
/** How often to refresh the display (ms)  */
#define DISPLAY_REFRESH_MS     33    ///< ~30 fps

/** How often to read the RTC (ms) */
#define RTC_POLL_MS            1000

/** How often to read the BMP280 sensor (ms) */
#ifndef SENSOR_POLL_MS
  #define SENSOR_POLL_MS       5000
#endif

/** Duration to show each info screen during auto-scroll (ms) */
#ifndef AUTO_SCROLL_DURATION_MS
  #define AUTO_SCROLL_DURATION_MS 4000
#endif

// ============================================================
// Display Modes
// ============================================================
enum class DisplayMode : uint8_t
{
    CLOCK        = 0,   ///< HH:MM or HH:MM:SS
    DATE         = 1,   ///< DD/MM/YYYY
    TEMPERATURE  = 2,   ///< Temperature from BMP280
    PRESSURE     = 3,   ///< Pressure from BMP280
    BRIGHTNESS   = 4,   ///< Brightness adjustment
    SET_HOUR     = 5,   ///< Configuration: set hour
    SET_MINUTE   = 6,   ///< Configuration: set minute
    SET_DAY      = 7,   ///< Configuration: set day
    SET_MONTH    = 8,   ///< Configuration: set month
    SET_YEAR     = 9,   ///< Configuration: set year
    NUM_MODES           ///< Sentinel – keep last
};

// ============================================================
// Colours  (CRGB packed format helpers)
// ============================================================
#define COLOR_CLOCK_DEFAULT   0x00FF80   ///< Cyan-green for digits
#define COLOR_DATE_DEFAULT    0x8080FF   ///< Soft blue for date
#define COLOR_TEMP_DEFAULT    0xFF6000   ///< Orange for temperature
#define COLOR_PRESSURE_DEFAULT 0xFF00A0  ///< Magenta for pressure
#define COLOR_CONFIG_DEFAULT  0xFFFF00   ///< Yellow for config mode

// ============================================================
// Feature Flags
// ============================================================
/** Enable 12-hour clock mode (comment out for 24-hour) */
// #define CLOCK_12H_MODE

/** Enable auto-scroll through all info screens */
// #define FEATURE_AUTO_SCROLL

/** Enable boot animation */
#define FEATURE_BOOT_ANIMATION

/** Enable photoresistor-based auto-brightness (requires LDR on A0) */
// #define FEATURE_AUTO_BRIGHTNESS
// #define LDR_PIN  A0
