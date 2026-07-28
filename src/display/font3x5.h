/**
 * @file    font3x5.h
 * @brief   Compact 3x5 numeric bitmap font for the LED matrix.
 *
 * @details Each glyph contains five rows. The three least-significant bits
 *          describe the pixels from left to right. Digits, separators, and
 *          the environmental-unit symbols "o", "C", and "%" are supported.
 */

#pragma once

#include <stdint.h>
#include <avr/pgmspace.h>

#define FONT_WIDTH   3
#define FONT_HEIGHT  5
#define FONT_ADVANCE 4

namespace Font3x5
{
    static const uint8_t PROGMEM GLYPHS[15][FONT_HEIGHT] = {
        { 0b111, 0b101, 0b101, 0b101, 0b111 }, // 0
        { 0b010, 0b110, 0b010, 0b010, 0b111 }, // 1
        { 0b111, 0b001, 0b111, 0b100, 0b111 }, // 2
        { 0b111, 0b001, 0b111, 0b001, 0b111 }, // 3
        { 0b101, 0b101, 0b111, 0b001, 0b001 }, // 4
        { 0b111, 0b100, 0b111, 0b001, 0b111 }, // 5
        { 0b111, 0b100, 0b111, 0b101, 0b111 }, // 6
        { 0b111, 0b001, 0b010, 0b010, 0b010 }, // 7
        { 0b111, 0b101, 0b111, 0b101, 0b111 }, // 8
        { 0b111, 0b101, 0b111, 0b001, 0b111 }, // 9
        { 0b000, 0b010, 0b000, 0b010, 0b000 }, // :
        { 0b001, 0b001, 0b010, 0b100, 0b100 }, // /
        { 0b110, 0b110, 0b000, 0b000, 0b000 }, // o (2x2 degree symbol)
        { 0b111, 0b100, 0b100, 0b100, 0b111 }, // C
        { 0b101, 0b001, 0b010, 0b100, 0b101 }  // %
    };

    inline const uint8_t* getGlyph(char c)
    {
        if (c >= '0' && c <= '9')
            return GLYPHS[c - '0'];
        if (c == ':' || c == ';')
            return GLYPHS[10];
        if (c == '/')
            return GLYPHS[11];
        if (c == 'o')
            return GLYPHS[12];
        if (c == 'C')
            return GLYPHS[13];
        if (c == '%')
            return GLYPHS[14];
        return nullptr;
    }
}
