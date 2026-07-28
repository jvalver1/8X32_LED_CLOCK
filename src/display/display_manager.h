/**
 * @file    display_manager.h
 * @brief   Display Manager – controls the 8x32 WS2812B LED matrix.
 *
 * @details Wraps FastLED and provides:
 *          - Pixel-level set/clear helpers
 *          - Compact 3x5 bitmap rendering for digits and separators
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

/** Environmental icon stored as native 24-bit RGB888 artwork. */
enum class EnvironmentalIcon : uint8_t { TEMPERATURE, HUMIDITY, PRESSURE };

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

    /** Set/get the user-facing brightness level (1..10). */
    static void setBrightnessLevel(uint8_t level);
    static uint8_t getBrightnessLevel();

    /** Advance to the next of 16 font colours with a smooth transition. */
    static void cycleFontColor();

    /** Return the current interpolated font colour. */
    static CRGB fontColor();

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
     * @param  c      Digit or ':' / ';' separator
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
     * @brief Draw an environmental icon in columns 0..7 and rows 0..7.
     *
     * RGB888 values map directly to FastLED's 8-bit RGB channels.
     */
    static void drawEnvironmentalIcon(EnvironmentalIcon icon);

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

    /** Draw the full clock screen matching reference photo (Calendar + Time + Day of Week bar). */
    static void drawClockScreen(uint8_t day, uint8_t dayOfWeek, uint8_t hour, uint8_t minute, bool colonVisible);

    // -----------------------------------------------------------------------
    // Animations
    // -----------------------------------------------------------------------

    /** Boot animation – plays once at startup. */
    static void playBootAnimation();

    /** Brief flash/transition between display modes. */
    static void playTransition();

    /** Save the outgoing screen before a pull-down transition begins. */
    static void capturePullDownFrame();

    /**
     * Composite one pull-down step over the newly rendered incoming screen.
     * Progress ranges from 1 to MATRIX_HEIGHT.
     */
    static void composePullDownFrame(uint8_t progress);

private:
    // Internal frame buffer
    static CRGB _leds[NUM_LEDS];

    // Current brightness level index
    static uint8_t _brightness;

    static const uint8_t BRIGHTNESS_STEPS[];
    static const uint8_t NUM_BRIGHTNESS_STEPS;

    static uint8_t _paletteIndex;
    static CRGB _colorFrom;
    static uint32_t _colorTransitionStart;
    static uint8_t _pullDownFrame[NUM_LEDS];

    DisplayManager() = delete; // Static class – do not instantiate
};
