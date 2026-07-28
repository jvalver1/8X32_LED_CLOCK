/**
 * @file    display_manager.cpp
 * @brief   DisplayManager implementation.
 */

#include "display_manager.h"
#include "font3x5.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
CRGB DisplayManager::_leds[NUM_LEDS];
uint8_t DisplayManager::_brightness = MIN_BRIGHTNESS;

// Predefined brightness steps (low → high)
const uint8_t DisplayManager::BRIGHTNESS_STEPS[] = { 2, 5, 10, 20, 35, 50, 60 };
const uint8_t DisplayManager::NUM_BRIGHTNESS_STEPS =
    sizeof(BRIGHTNESS_STEPS) / sizeof(BRIGHTNESS_STEPS[0]);

static const uint32_t PROGMEM FONT_PALETTE[16] = {
    0xFFFFFF, 0xFF2020, 0xFF6000, 0xFF9800,
    0xFFE000, 0x90FF00, 0x20FF20, 0x00FF90,
    0x00FFFF, 0x0090FF, 0x2040FF, 0x7020FF,
    0xB020FF, 0xFF20FF, 0xFF2080, 0xFFD0A0
};

// Native RGB888 artwork. The unused columns are transparent black so every
// icon can share one 8x8 table and be copied directly into FastLED CRGB.
static const uint32_t PROGMEM ENVIRONMENTAL_ICONS[3][8][8] = {
    {
        {0x000000, 0xFFFBFF, 0xF7F7FF, 0xEFEFFF, 0x000000, 0, 0, 0},
        {0x000000, 0xEFEFFF, 0x000000, 0xD6DBEF, 0x000000, 0, 0, 0},
        {0x000000, 0xEFEFFF, 0xFFA25A, 0xD6DBEF, 0x000000, 0, 0, 0},
        {0x000000, 0xF7F7FF, 0xFF7942, 0xD6DBEF, 0x000000, 0, 0, 0},
        {0x000000, 0xF7F7FF, 0xFF5539, 0xD6DBEF, 0x000000, 0, 0, 0},
        {0xFFFBFF, 0xFF6D42, 0xFF4931, 0xDE2821, 0xD6DBEF, 0, 0, 0},
        {0xEFEFFF, 0xFF8252, 0xEF3429, 0xFF5D42, 0xEFEFFF, 0, 0, 0},
        {0x000000, 0xEFEFFF, 0xF7F7FF, 0xD6DBEF, 0x000000, 0, 0, 0}
    },
    {
        {0x000000, 0x000000, 0xEFEBFF, 0x000000, 0x000000, 0, 0, 0},
        {0x000000, 0xFFFFFF, 0xE7E7FF, 0xC6C7FF, 0x000000, 0, 0, 0},
        {0xFFFFFF, 0xEFEBFF, 0xE7E7FF, 0xC6C7FF, 0xADAAFF, 0, 0, 0},
        {0xEFEBFF, 0xE7E7FF, 0xE7E7FF, 0xADAAFF, 0xADAAFF, 0, 0, 0},
        {0xEFEBFF, 0xC6C7FF, 0xC6C7FF, 0xADAAFF, 0x7B7DCE, 0, 0, 0},
        {0x000000, 0xADAAFF, 0xADAAFF, 0x7B7DCE, 0x000000, 0, 0, 0},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0, 0, 0},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0, 0, 0}
    },
    {
        {0xFFC06A, 0xFFA552, 0xFF8A3D, 0xE53935, 0xC62828, 0, 0, 0},
        {0xFFC06A, 0xFF9A45, 0xF4511E, 0xC62828, 0x000000, 0, 0, 0},
        {0xFF8A3D, 0xF4511E, 0xC62828, 0x000000, 0x006DCE, 0, 0, 0},
        {0xE53935, 0xB71C1C, 0x000000, 0x006BCC, 0x007EE5, 0, 0, 0},
        {0xC62828, 0x000000, 0x006ECD, 0x0088EC, 0x68E5FC, 0, 0, 0},
        {0x000000, 0x006DCE, 0x0089ED, 0x63E3FC, 0x7BEBFD, 0, 0, 0},
        {0x006ED0, 0x0081E7, 0x59E1FB, 0x73EAFD, 0x7BECFD, 0, 0, 0},
        {0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0, 0, 0}
    }
};

uint8_t DisplayManager::_paletteIndex = 0;
CRGB DisplayManager::_colorFrom = CRGB::White;
uint32_t DisplayManager::_colorTransitionStart = 0;
uint8_t DisplayManager::_pullDownFrame[NUM_LEDS];

static CRGB paletteColor(uint8_t index)
{
    return CRGB(pgm_read_dword(&FONT_PALETTE[index & 0x0f]));
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void DisplayManager::init()
{
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, COLOR_ORDER>(_leds, NUM_LEDS)
           .setCorrection(UncorrectedColor);
    FastLED.setDither(DISABLE_DITHER);
    setBrightnessLevel(DEFAULT_BRIGHTNESS_LEVEL);
#ifndef WOKWI_SIMULATION
    FastLED.setMaxPowerInVoltsAndMilliamps(LED_SUPPLY_VOLTS,
                                           LED_MAX_MILLIAMPS);
#endif
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
    delayMicroseconds(300);
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

void DisplayManager::setBrightnessLevel(uint8_t level)
{
    level = constrain(level, 1, 10);
    const uint16_t range = MAX_BRIGHTNESS - MIN_BRIGHTNESS;
    setBrightness(MIN_BRIGHTNESS +
                  static_cast<uint8_t>(((level - 1) * range + 4) / 9));
}

uint8_t DisplayManager::getBrightnessLevel()
{
    const uint16_t range = MAX_BRIGHTNESS - MIN_BRIGHTNESS;
    if (range == 0)
        return 1;
    return 1 + static_cast<uint8_t>(
        ((_brightness - MIN_BRIGHTNESS) * 9UL + range / 2) / range);
}

CRGB DisplayManager::fontColor()
{
    const uint16_t durationMs = 350;
    uint32_t elapsed = millis() - _colorTransitionStart;
    CRGB target = paletteColor(_paletteIndex);
    if (elapsed >= durationMs)
        return target;

    uint8_t amount = static_cast<uint8_t>((elapsed * 255UL) / durationMs);
    return blend(_colorFrom, target, amount);
}

void DisplayManager::cycleFontColor()
{
    _colorFrom = fontColor();
    _paletteIndex = (_paletteIndex + 1) & 0x0f;
    _colorTransitionStart = millis();
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
 * @details The physical layout starts with LED 0 at bottom-right. The chain
 *          ascends the rightmost column, moves one column left and descends,
 *          then continues right-to-left in a vertical serpentine.
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
    const uint8_t physicalColumn = MATRIX_WIDTH - 1 - x;
    if ((physicalColumn & 1U) == 0U)
        return physicalColumn * MATRIX_HEIGHT +
               (MATRIX_HEIGHT - 1 - y); // bottom → top
    return physicalColumn * MATRIX_HEIGHT + y; // top → bottom
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
    const uint8_t* glyph = Font3x5::getGlyph(c);
    for (uint8_t row = 0; row < FONT_HEIGHT; row++)
    {
        uint8_t rowData = glyph ? pgm_read_byte(&glyph[row]) : 0;
        for (uint8_t col = 0; col < FONT_WIDTH; col++)
        {
            if (rowData & (1 << (FONT_WIDTH - 1 - col)))
                setPixel(x + col, y + row, color);
            else
                setPixel(x + col, y + row, CRGB::Black);
        }
    }
    return FONT_ADVANCE;
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

void DisplayManager::drawEnvironmentalIcon(EnvironmentalIcon icon)
{
    uint8_t iconIndex = static_cast<uint8_t>(icon);
    if (iconIndex > static_cast<uint8_t>(EnvironmentalIcon::PRESSURE))
        iconIndex = 0;
    for (uint8_t y = 0; y < 8; y++)
    {
        for (uint8_t x = 0; x < 8; x++)
        {
            uint32_t rgb888 =
                pgm_read_dword(&ENVIRONMENTAL_ICONS[iconIndex][y][x]);
            setPixel(x, y, CRGB(rgb888));
        }
    }
}

void DisplayManager::scrollText(const char* str, CRGB color, uint16_t delay_ms)
{
    // Calculate total pixel width of the string
    int16_t totalWidth = 0;
    const char* p = str;
    while (*p) { totalWidth += FONT_ADVANCE; p++; }

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
    // Render the complete rainbow on every frame. Hue advances by only two
    // 8-bit steps between frames, while brightness fades in and out smoothly.
    const uint8_t frameCount = 80;
    const uint8_t fadeFrames = 20;
    for (uint8_t frame = 0; frame < frameCount; frame++)
    {
        uint8_t value = 255;
        if (frame < fadeFrames)
            value = static_cast<uint8_t>((frame * 255U) / fadeFrames);
        else if (frame >= frameCount - fadeFrames)
            value = static_cast<uint8_t>(
                ((frameCount - 1U - frame) * 255U) / fadeFrames);

        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            uint8_t hue = static_cast<uint8_t>(
                (static_cast<uint16_t>(x) * 255U) /
                (MATRIX_WIDTH - 1U) + frame * 2U);
            for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
                setPixel(x, y, CHSV(hue, 255, value));
        }
        render();
        delay(18);
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

void DisplayManager::capturePullDownFrame()
{
    // RGB332 keeps the saved frame to 256 bytes instead of the 768 bytes a
    // second CRGB framebuffer would require on the ATmega328P.
    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
        const CRGB& pixel = _leds[i];
        _pullDownFrame[i] = (pixel.r & 0xe0) |
                            ((pixel.g >> 3) & 0x1c) |
                            (pixel.b >> 6);
    }
}

void DisplayManager::composePullDownFrame(uint8_t progress)
{
    if (progress > MATRIX_HEIGHT)
        progress = MATRIX_HEIGHT;

    // The incoming screen is already rendered normally in _leds. Move its
    // bottom 'progress' rows into view from above.
    for (uint8_t y = 0; y < progress; y++)
    {
        uint8_t sourceY = MATRIX_HEIGHT - progress + y;
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
            _leds[xyToIndex(x, y)] = _leds[xyToIndex(x, sourceY)];
    }

    // Place the saved outgoing frame below it, displaced by the same amount.
    for (uint8_t y = progress; y < MATRIX_HEIGHT; y++)
    {
        uint8_t sourceY = y - progress;
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
            uint8_t packed = _pullDownFrame[xyToIndex(x, sourceY)];
            CRGB restored((packed & 0xe0) | ((packed & 0xe0) >> 3) |
                          ((packed & 0xe0) >> 6),
                          ((packed & 0x1c) << 3) | (packed & 0x1c) |
                          ((packed & 0x1c) >> 3),
                          ((packed & 0x03) << 6) | ((packed & 0x03) << 4) |
                          ((packed & 0x03) << 2) | (packed & 0x03));
            _leds[xyToIndex(x, y)] = restored;
        }
    }
}

// ---------------------------------------------------------------------------
// Custom Calendar & Clock UI (matching reference photo)
// ---------------------------------------------------------------------------

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
        // Single digit centered at x=3 (cols 3..5), with 3 empty columns
        // on either side of the 9-column calendar tile.
        uint8_t d = day;
        for (uint8_t r = 0; r < 5; r++)
        {
            const uint8_t* glyph = Font3x5::getGlyph('0' + d);
            uint8_t rowBits = pgm_read_byte(&glyph[r]);
            for (uint8_t c = 0; c < 3; c++)
            {
                if (rowBits & (1 << (2 - c)))
                {
                    setPixel(3 + c, 2 + r, CRGB::Black);
                }
            }
        }
    }
    else
    {
        // Double digit: use the 7 central columns (1..7). Column 4 is the
        // separator and columns 0 and 8 remain empty margins.
        uint8_t d1 = day / 10;
        uint8_t d2 = day % 10;
        for (uint8_t r = 0; r < 5; r++)
        {
            const uint8_t* glyph1 = Font3x5::getGlyph('0' + d1);
            const uint8_t* glyph2 = Font3x5::getGlyph('0' + d2);
            uint8_t b1 = pgm_read_byte(&glyph1[r]);
            uint8_t b2 = pgm_read_byte(&glyph2[r]);
            for (uint8_t c = 0; c < 3; c++)
            {
                if (b1 & (1 << (2 - c))) setPixel(1 + c, 2 + r, CRGB::Black);
                if (b2 & (1 << (2 - c))) setPixel(5 + c, 2 + r, CRGB::Black);
            }
        }
    }
}

void DisplayManager::drawClockScreen(uint8_t day, uint8_t dayOfWeek, uint8_t hour, uint8_t minute, bool colonVisible)
{
    clear();

    // 1. Draw 9x8 calendar tile (cols 0..8)
    drawCalendarPage(day);

    // 2. Draw HH:MM with the shared 3x5 numeric font, vertically centered.
    char buf[6];
    snprintf(buf, sizeof(buf), "%02u%c%02u", hour, colonVisible ? ':' : ' ', minute);

    CRGB textColor = fontColor();
    drawChar(11, 1, buf[0], textColor);
    drawChar(15, 1, buf[1], textColor);
    drawChar(19, 1, buf[2], textColor);
    drawChar(23, 1, buf[3], textColor);
    drawChar(27, 1, buf[4], textColor);

    // 3. Draw 7-segment Day of Week Bar on Row 7.
    // 7 segments of 2 pixels wide with 1-pixel gaps (cols 10..29).
    // RTClib dayOfTheWeek: 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat
    // Mon-first index: 0=Mon, 1=Tue, 2=Wed, 3=Thu, 4=Fri, 5=Sat, 6=Sun
    uint8_t activeSeg = (dayOfWeek == 0) ? 6 : (dayOfWeek - 1);

    for (uint8_t s = 0; s < 7; s++)
    {
        uint8_t startCol = 10 + s * 3;
        CRGB color = (s == activeSeg) ? textColor : CRGB(40, 40, 45);
        setPixel(startCol, 7, color);
        setPixel(startCol + 1, 7, color);
    }
}
