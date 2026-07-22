/**
 * @file    display_manager.h
 * @brief   Display Manager – controls the 8x32 WS2812B LED matrix.
 *
 * @details Wraps FastLED and provides:
 *          - Pixel-level set/clear helpers
 *          - 5×7 bitmap font rendering for digits and characters
 *          - Scrolling text support
 *          - Boot animation
 *          - Frame buffering and brightness control
 */

#pragma once

#include <stdint.h>
#include <FastLED.h>
#include "../config.h"

// ---------------------------------------------------------------------------
// Scroll direction
// ---------------------------------------------------------------------------
enum class ScrollDir : uint8_t { LEFT, RIGHT };

// ---------------------------------------------------------------------------
// DisplayManager (static class / namespace-style)
// ---------------------------------------------------------------------------
class DisplayManager
{
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /** Initialise FastLED and clear the display. */
    static void init();

    /** Push the current frame buffer to the LEDs. */
    static void render();

    // -----------------------------------------------------------------------
    // Brightness
    // -----------------------------------------------------------------------

    /** Set global brightness (0–255, clamped to MIN/MAX_BRIGHTNESS). */
    static void setBrightness(uint8_t brightness);

    /** Get current brightness. */
    static uint8_t getBrightness();

    /** Step brightness up by one level (wraps). */
    static void brightnessUp();

    /** Step brightness down by one level (wraps). */
    static void brightnessDown();

    // -----------------------------------------------------------------------
    // Low-level pixel operations
    // -----------------------------------------------------------------------

    /**
     * @brief  Set a single pixel to a colour.
     * @param  x      Column  (0 = left)
     * @param  y      Row     (0 = top)
     * @param  color  FastLED CRGB colour
     */
    static void setPixel(int16_t x, int16_t y, CRGB color);

    /**
     * @brief  Get the LED index for a given (x, y) position.
     *
     * @details Handles the serpentine layout common in WS2812B matrices:
     *          Even columns go top-to-bottom, odd columns go bottom-to-top
     *          (or vice-versa depending on wiring). Adjust xyToIndex() in
     *          the .cpp if your matrix has a different layout.
     */
    static int16_t xyToIndex(int16_t x, int16_t y);

    /** Clear the entire frame buffer (all pixels black). */
    static void clear();

    /** Fill the entire frame buffer with one colour. */
    static void fill(CRGB color);

    // -----------------------------------------------------------------------
    // Font / text rendering
    // -----------------------------------------------------------------------

    /**
     * @brief  Draw a single ASCII character at (x, y).
     * @param  x      Top-left column
     * @param  y      Top-left row
     * @param  c      ASCII character
     * @param  color  Foreground colour
     * @return Width of the character in pixels (including 1px spacing)
     */
    static int8_t drawChar(int16_t x, int16_t y, char c, CRGB color);

    /**
     * @brief  Draw a string starting at (x, y).
     * @return Total pixel width of the string
     */
    static int16_t drawString(int16_t x, int16_t y, const char* str, CRGB color);

    /**
     * @brief  Scroll a string across the display once (blocking).
     * @param  str    Null-terminated string
     * @param  color  Text colour
     * @param  delay_ms  Milliseconds per frame step
     */
    static void scrollText(const char* str, CRGB color, uint16_t delay_ms = 40);

    // -----------------------------------------------------------------------
    // Custom Calendar & Clock UI (matching reference photo)
    // -----------------------------------------------------------------------

    /** Draw the 9x8 calendar page with red banner and day number on cols 0..8. */
    static void drawCalendarPage(uint8_t day);

    /** Draw custom 4x7 bold digit at (x, y). Returns width in pixels. */
    static uint8_t drawCustom4x7Digit(int16_t x, int16_t y, char c, CRGB color);

    /** Draw the full clock screen matching reference photo (Calendar + Time + Day of Week bar). */
    static void drawClockScreen(uint8_t day, uint8_t dayOfWeek, uint8_t hour, uint8_t minute, bool colonVisible);

    // -----------------------------------------------------------------------
    // Animations
    // -----------------------------------------------------------------------

    /** Boot animation – plays once at startup. */
    static void playBootAnimation();

    /** Brief flash/transition between display modes. */
    static void playTransition();

private:
    // Internal frame buffer
    static CRGB _leds[NUM_LEDS];

    // Current brightness level index
    static uint8_t _brightness;

    // Brightness step levels
    static const uint8_t BRIGHTNESS_STEPS[];
    static const uint8_t NUM_BRIGHTNESS_STEPS;

    DisplayManager() = delete; // Static class – do not instantiate
};
