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
  #define LED_DATA_PIN   6      ///< Arduino Pro Mini digital pin for WS2812B data
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

/**
 * Brightness limits on FastLED's 0-255 scale.
 *
 * At the conservative WS2812B worst case of 60 mA per pixel, 256 pixels can
 * demand 15.36 A. A ceiling of 58 limits the LEDs to about 3.5 A, leaving
 * 0.5 A headroom on a regulated 5 V / 4 A source.
 */
#ifndef DEFAULT_BRIGHTNESS_LEVEL
  #define DEFAULT_BRIGHTNESS_LEVEL 5
#endif
#ifndef MAX_BRIGHTNESS
  #define MAX_BRIGHTNESS       58
#endif
#ifndef MIN_BRIGHTNESS
  #define MIN_BRIGHTNESS       2
#endif

/**
 * FastLED estimates each frame's demand and scales it to this ceiling.
 * This is a software safeguard, not a replacement for a regulated supply,
 * fuse, suitable wiring, or checking the battery/boost converter rating.
 */
#ifndef LED_SUPPLY_VOLTS
  #define LED_SUPPLY_VOLTS       5
#endif
#ifndef LED_MAX_MILLIAMPS
  // Leaves 0.5 A for control electronics, conversion losses, and margin.
  #define LED_MAX_MILLIAMPS    3500
#endif

// ============================================================
// Buttons  (active LOW, internal pull-up)
// ============================================================
#ifndef BTN_MODE_PIN
  #define BTN_MODE_PIN   3     ///< Mode/Select – supports hardware INT1
#endif

#ifndef BTN_UP_PIN
  #define BTN_UP_PIN     4     ///< Up/Increment
#endif

#ifndef BTN_DOWN_PIN
  #define BTN_DOWN_PIN   5     ///< Down/Decrement
#endif

/** Debounce time in milliseconds */
#define BTN_DEBOUNCE_MS        50
/** Long-press threshold in milliseconds */
#define BTN_LONG_PRESS_MS      800
/** Auto-repeat interval when held (ms) */
#define BTN_REPEAT_INTERVAL_MS 150
#define SETUP_TIMEOUT_MS       7000
#define SETUP_FLASH_MS          500

// ============================================================
// I²C Peripherals
// ============================================================
/** DS3231 RTC – fixed address, cannot be changed */
#define DS3231_I2C_ADDR        0x68

/** BME280 I²C address (0x76 when SDO=GND, 0x77 when SDO=VCC) */
#ifndef BME280_I2C_ADDR
  #define BME280_I2C_ADDR      0x76
#endif

// ============================================================
// LED Matrix Layout
// ============================================================
#define MATRIX_LAYOUT_SERPENTINE_COLUMN_MAJOR 0  ///< Verified chain: bottom-right to bottom-left
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

/** How often to read the BME280 sensor (ms) */
#ifndef SENSOR_POLL_MS
  #define SENSOR_POLL_MS       5000
#endif

/** Carousel dwell times selected by successive UP/DOWN long presses. */
#define CAROUSEL_SHORT_DELAY_MS  5000UL
#define CAROUSEL_LONG_DELAY_MS   7000UL

/** Delay between rows in the pull-down screen transition. */
#define PULL_DOWN_ROW_DELAY_MS     35

/** Flash period for the bottom-right carousel mode indicator. */
#define CAROUSEL_INDICATOR_FLASH_MS 1000UL

// ============================================================
// Display Modes
// ============================================================
enum class DisplayMode : uint8_t
{
    CLOCK        = 0,   ///< HH:MM or HH:MM:SS
    DATE         = 1,   ///< DD/MM/YYYY
    TEMPERATURE  = 2,   ///< Temperature from BME280
    HUMIDITY     = 3,   ///< Relative humidity from BME280
    PRESSURE     = 4,   ///< Pressure from BME280
    SET_HOUR       = 5, ///< Configuration: set hour
    SET_MINUTE     = 6, ///< Configuration: set minute
    SET_DAY        = 7, ///< Configuration: set day
    SET_MONTH      = 8, ///< Configuration: set month
    SET_YEAR       = 9, ///< Configuration: set year
    SET_FORMAT    = 10, ///< Configuration: 12/24-hour display
    SET_BRIGHTNESS= 11, ///< Configuration: LED brightness level 1..10
    NUM_MODES           ///< Sentinel – keep last
};

// ============================================================
// Feature Flags
// ============================================================
/**
 * Display the RTC time with a one-hour daylight-saving offset.
 * Set to 1 while DST is active, or 0 for standard time.
 */
#ifndef DST_ACTIVE
  #define DST_ACTIVE 0
#endif

/**
 * Clock display format: 0 = 24-hour (default), 1 = 12-hour.
 * This changes presentation only; the RTC always stores 24-hour time.
 */
#ifndef CLOCK_12H_FORMAT
  #define CLOCK_12H_FORMAT 0
#endif

/** Enable boot animation */
#define FEATURE_BOOT_ANIMATION

/** Enable photoresistor-based auto-brightness (requires LDR on A0) */
// #define FEATURE_AUTO_BRIGHTNESS
// #define LDR_PIN  A0
