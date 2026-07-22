/**
 * @file    display_manager.cpp
 * @brief   DisplayManager implementation.
 */

#include "display_manager.h"
#include "font5x7.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
CRGB DisplayManager::_leds[NUM_LEDS];
uint8_t DisplayManager::_brightness = DEFAULT_BRIGHTNESS;

// Predefined brightness steps (low → high)
const uint8_t DisplayManager::BRIGHTNESS_STEPS[] = { 5, 15, 30, 60, 100, 150, 200 };
const uint8_t DisplayManager::NUM_BRIGHTNESS_STEPS =
    sizeof(BRIGHTNESS_STEPS) / sizeof(BRIGHTNESS_STEPS[0]);

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void DisplayManager::init()
{
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(_leds, NUM_LEDS)
           .setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(_brightness);
    clear();
    FastLED.show();

#ifdef ENABLE_DEBUG_SERIAL
    Serial.println(F("[Display] FastLED initialised."));
#endif
}

void DisplayManager::render()
{
    FastLED.show();
}

// ---------------------------------------------------------------------------
// Brightness
// ---------------------------------------------------------------------------
void DisplayManager::setBrightness(uint8_t brightness)
{
    _brightness = constrain(brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    FastLED.setBrightness(_brightness);
}

uint8_t DisplayManager::getBrightness()
{
    return _brightness;
}

void DisplayManager::brightnessUp()
{
    // Find next step above current brightness
    for (uint8_t i = 0; i < NUM_BRIGHTNESS_STEPS; i++)
    {
        if (BRIGHTNESS_STEPS[i] > _brightness)
        {
            setBrightness(BRIGHTNESS_STEPS[i]);
            return;
        }
    }
    // Already at max – wrap to min
    setBrightness(BRIGHTNESS_STEPS[0]);
}

void DisplayManager::brightnessDown()
{
    for (int8_t i = NUM_BRIGHTNESS_STEPS - 1; i >= 0; i--)
    {
        if (BRIGHTNESS_STEPS[i] < _brightness)
        {
            setBrightness(BRIGHTNESS_STEPS[i]);
            return;
        }
    }
    // Already at min – wrap to max
    setBrightness(BRIGHTNESS_STEPS[NUM_BRIGHTNESS_STEPS - 1]);
}

// ---------------------------------------------------------------------------
// Pixel operations
// ---------------------------------------------------------------------------

/**
 * @brief  Convert (x, y) matrix coordinates to LED strip index.
 *
 * @details Assumes a serpentine layout where:
 *          - Column 0 is the LEFT column.
 *          - Even columns are wired top-to-bottom (y=0 at top).
 *          - Odd  columns are wired bottom-to-top.
 *
 *          Adjust this function if your matrix has a different wiring order.
 *          Common alternatives:
 *            - Row-major (row 0 top, even rows L→R, odd rows R→L)
 *            - Column-major non-serpentine (all columns same direction)
 */
int16_t DisplayManager::xyToIndex(int16_t x, int16_t y)
{
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT)
        return -1; // Out of bounds

#if (MATRIX_LAYOUT == MATRIX_LAYOUT_PROGRESSIVE_ROW_MAJOR)
    return y * MATRIX_WIDTH + x;
#elif (MATRIX_LAYOUT == MATRIX_LAYOUT_SERPENTINE_ROW_MAJOR)
    if (y % 2 == 0)
        return y * MATRIX_WIDTH + x;
    else
        return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
#else // MATRIX_LAYOUT_SERPENTINE_COLUMN_MAJOR
    if (x % 2 == 0)
        return x * MATRIX_HEIGHT + y;                        // top → bottom
    else
        return x * MATRIX_HEIGHT + (MATRIX_HEIGHT - 1 - y); // bottom → top
#endif
}

void DisplayManager::setPixel(int16_t x, int16_t y, CRGB color)
{
    int16_t idx = xyToIndex(x, y);
    if (idx >= 0)
        _leds[idx] = color;
}

void DisplayManager::clear()
{
    fill_solid(_leds, NUM_LEDS, CRGB::Black);
}

void DisplayManager::fill(CRGB color)
{
    fill_solid(_leds, NUM_LEDS, color);
}

// ---------------------------------------------------------------------------
// Font / text rendering
// ---------------------------------------------------------------------------

int8_t DisplayManager::drawChar(int16_t x, int16_t y, char c, CRGB color)
{
    if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR)
        c = '?';

    const uint8_t* glyph = Font5x7::getGlyph(c);

    for (uint8_t col = 0; col < FONT_WIDTH; col++)
    {
        uint8_t colData = glyph[col];
        for (uint8_t row = 0; row < FONT_HEIGHT; row++)
        {
            if (colData & (1 << row))
                setPixel(x + col, y + row, color);
            else
                setPixel(x + col, y + row, CRGB::Black);
        }
    }
    return FONT_WIDTH + 1; // glyph width + 1 pixel spacing
}

int16_t DisplayManager::drawString(int16_t x, int16_t y, const char* str, CRGB color)
{
    int16_t cursor = x;
    while (*str)
    {
        cursor += drawChar(cursor, y, *str++, color);
    }
    return cursor - x;
}

void DisplayManager::scrollText(const char* str, CRGB color, uint16_t delay_ms)
{
    // Calculate total pixel width of the string
    int16_t totalWidth = 0;
    const char* p = str;
    while (*p) { totalWidth += FONT_WIDTH + 1; p++; }

    int16_t startX = MATRIX_WIDTH;
    int16_t endX   = -totalWidth;

    for (int16_t x = startX; x >= endX; x--)
    {
        clear();
        drawString(x, 0, str, color);
        render();
        delay(delay_ms);
    }
}

// ---------------------------------------------------------------------------
// Animations
// ---------------------------------------------------------------------------

void DisplayManager::playBootAnimation()
{
#ifdef FEATURE_BOOT_ANIMATION
    // Simple sweep: fill columns left to right with a rainbow, then fade out
    for (uint8_t col = 0; col < MATRIX_WIDTH; col++)
    {
        CRGB c = CHSV(col * 8, 255, 200);
        for (uint8_t row = 0; row < MATRIX_HEIGHT; row++)
            setPixel(col, row, c);
        render();
        delay(20);
    }
    delay(300);
    // Fade out
    for (uint8_t i = 0; i < 10; i++)
    {
        fadeToBlackBy(_leds, NUM_LEDS, 30);
        render();
        delay(30);
    }
    clear();
    render();
#endif
}

void DisplayManager::playTransition()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        fadeToBlackBy(_leds, NUM_LEDS, 60);
        render();
        delay(20);
    }
    clear();
}

// ---------------------------------------------------------------------------
// Custom Calendar & Clock UI (matching reference photo)
// ---------------------------------------------------------------------------

// 3x5 compact digits for calendar day rendering (0-9)
static const uint8_t PROGMEM DIGITS_3X5[10][5] = {
    { 0b111, 0b101, 0b101, 0b101, 0b111 }, // 0
    { 0b010, 0b110, 0b010, 0b010, 0b111 }, // 1
    { 0b111, 0b001, 0b111, 0b100, 0b111 }, // 2
    { 0b111, 0b001, 0b111, 0b001, 0b111 }, // 3
    { 0b101, 0b101, 0b111, 0b001, 0b001 }, // 4
    { 0b111, 0b100, 0b111, 0b001, 0b111 }, // 5
    { 0b111, 0b100, 0b111, 0b101, 0b111 }, // 6
    { 0b111, 0b001, 0b010, 0b010, 0b010 }, // 7
    { 0b111, 0b101, 0b111, 0b101, 0b111 }, // 8
    { 0b111, 0b101, 0b111, 0b001, 0b111 }  // 9
};

void DisplayManager::drawCalendarPage(uint8_t day)
{
    if (day == 0) day = 1;
    if (day > 31) day = 31;

    // Rows 0 and 1: red header banner across the 9-column calendar tile.
    for (uint8_t r = 0; r < 2; r++)
    {
        for (uint8_t c = 0; c < 9; c++)
        {
            setPixel(c, r, CRGB::Red);
        }
    }

    // Rows 2 to 7: white calendar body.
    for (uint8_t r = 2; r < 8; r++)
    {
        for (uint8_t c = 0; c < 9; c++)
        {
            setPixel(c, r, CRGB::White);
        }
    }

    // Draw day digit in black on the white body (Rows 2..6)
    if (day < 10)
    {
        // Single digit centered at x=2 (cols 2..4)
        uint8_t d = day;
        for (uint8_t r = 0; r < 5; r++)
        {
            uint8_t rowBits = pgm_read_byte(&DIGITS_3X5[d][r]);
            for (uint8_t c = 0; c < 3; c++)
            {
                if (rowBits & (1 << (2 - c)))
                {
                    setPixel(2 + c, 2 + r, CRGB::Black);
                }
            }
        }
    }
    else
    {
        // Double digit: d1 at x=1 (cols 1..3), d2 at x=4 (cols 4..6)
        uint8_t d1 = day / 10;
        uint8_t d2 = day % 10;
        for (uint8_t r = 0; r < 5; r++)
        {
            uint8_t b1 = pgm_read_byte(&DIGITS_3X5[d1][r]);
            uint8_t b2 = pgm_read_byte(&DIGITS_3X5[d2][r]);
            for (uint8_t c = 0; c < 3; c++)
            {
                if (b1 & (1 << (2 - c))) setPixel(1 + c, 2 + r, CRGB::Black);
                if (b2 & (1 << (2 - c))) setPixel(4 + c, 2 + r, CRGB::Black);
            }
        }
    }
}

// 4x7 bold digits (rows 0..6, 4 bits wide MSB left)
static const uint8_t PROGMEM DIGITS_4X7[10][7] = {
    { 0b1111, 0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b1111 }, // 0
    { 0b0111, 0b0010, 0b0010, 0b0010, 0b0010, 0b0010, 0b0111 }, // 1
    { 0b1111, 0b0001, 0b0001, 0b1111, 0b1000, 0b1000, 0b1111 }, // 2
    { 0b1111, 0b0001, 0b0001, 0b1111, 0b0001, 0b0001, 0b1111 }, // 3
    { 0b1001, 0b1001, 0b1001, 0b1111, 0b0001, 0b0001, 0b0001 }, // 4
    { 0b1111, 0b1000, 0b1000, 0b1111, 0b0001, 0b0001, 0b1111 }, // 5
    { 0b1111, 0b1000, 0b1000, 0b1111, 0b1001, 0b1001, 0b1111 }, // 6
    { 0b1111, 0b0001, 0b0001, 0b0001, 0b0001, 0b0001, 0b0001 }, // 7
    { 0b1111, 0b1001, 0b1001, 0b1111, 0b1001, 0b1001, 0b1111 }, // 8
    { 0b1111, 0b1001, 0b1001, 0b1111, 0b0001, 0b0001, 0b1111 }  // 9
};

uint8_t DisplayManager::drawCustom4x7Digit(int16_t x, int16_t y, char c, CRGB color)
{
    if (c == ':')
    {
        // 2x2 dot colon (Row 1..2 and Row 4..5)
        setPixel(x, y + 1, color); setPixel(x + 1, y + 1, color);
        setPixel(x, y + 2, color); setPixel(x + 1, y + 2, color);

        setPixel(x, y + 4, color); setPixel(x + 1, y + 4, color);
        setPixel(x, y + 5, color); setPixel(x + 1, y + 5, color);
        return 2;
    }

    if (c < '0' || c > '9') return 0;
    uint8_t d = c - '0';

    if (d == 1)
    {
        // 3px wide digit 1
        for (uint8_t r = 0; r < 7; r++)
        {
            uint8_t rowBits = pgm_read_byte(&DIGITS_4X7[1][r]);
            for (uint8_t col = 0; col < 3; col++)
            {
                if (rowBits & (1 << (2 - col)))
                {
                    setPixel(x + col, y + r, color);
                }
            }
        }
        return 3;
    }

    // Standard 4px wide digit
    for (uint8_t r = 0; r < 7; r++)
    {
        uint8_t rowBits = pgm_read_byte(&DIGITS_4X7[d][r]);
        for (uint8_t col = 0; col < 4; col++)
        {
            if (rowBits & (1 << (3 - col)))
            {
                setPixel(x + col, y + r, color);
            }
        }
    }
    return 4;
}

void DisplayManager::drawClockScreen(uint8_t day, uint8_t dayOfWeek, uint8_t hour, uint8_t minute, bool colonVisible)
{
    clear();

    // 1. Draw 9x8 calendar tile (cols 0..8)
    drawCalendarPage(day);

    // 2. Draw HH:MM (cols 8..31, rows 0..6)
    char buf[6];
    snprintf(buf, sizeof(buf), "%02u%c%02u", hour, colonVisible ? ':' : ' ', minute);

    int16_t cursorX = 9;
    for (uint8_t i = 0; i < 5; i++)
    {
        char ch = buf[i];
        if (ch == ' ')
        {
            cursorX += 2; // Blank colon gap
        }
        else
        {
            uint8_t w = drawCustom4x7Digit(cursorX, 0, ch, CRGB::White);
            cursorX += w;
        }
        cursorX += 1; // 1px spacing between digits
    }

    // 3. Draw 7-segment Day of Week Bar on Row 7 (cols 8..31)
    // 7 segments of 2 pixels wide with 1-pixel gap (cols 9..28)
    // RTClib dayOfTheWeek: 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat
    // Mon-first index: 0=Mon, 1=Tue, 2=Wed, 3=Thu, 4=Fri, 5=Sat, 6=Sun
    uint8_t activeSeg = (dayOfWeek == 0) ? 6 : (dayOfWeek - 1);

    for (uint8_t s = 0; s < 7; s++)
    {
        uint8_t startCol = 9 + s * 3;
        CRGB color = (s == activeSeg) ? CRGB::White : CRGB(40, 40, 45); // Active = White, Inactive = Dark Gray
        setPixel(startCol, 7, color);
        setPixel(startCol + 1, 7, color);
    }
}
