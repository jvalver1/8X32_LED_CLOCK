/**
 * @file    button_manager.cpp
 * @brief   ButtonManager implementation – debounce, short/long press, auto-repeat.
 */

#include "button_manager.h"
#include "../config.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
ButtonManager::BtnState ButtonManager::_state[ButtonManager::NUM_BTNS];
constexpr uint8_t ButtonManager::PIN[ButtonManager::NUM_BTNS];

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
void ButtonManager::init()
{
    for (uint8_t i = 0; i < NUM_BTNS; i++)
    {
        pinMode(PIN[i], INPUT_PULLUP);
        // Inputs idle HIGH. "debounced" stores the pressed state, so it must
        // start false; starting true creates a phantom button release shortly
        // after boot and immediately advances away from the clock screen.
        _state[i] = { true, false, 0, 0, false, 0, ButtonEvent::NONE };
    }

#ifdef ENABLE_DEBUG_SERIAL
    Serial.println(F("[BTN] Button manager initialised."));
#endif
}

// ---------------------------------------------------------------------------
// update() – call every loop tick
// ---------------------------------------------------------------------------
void ButtonManager::update()
{
    uint32_t now = millis();

    for (uint8_t i = 0; i < NUM_BTNS; i++)
    {
        BtnState& s = _state[i];
        bool raw = digitalRead(PIN[i]); // HIGH = released (pull-up + active LOW)

        // ---- Debounce ----
        if (raw != s.lastRaw)
        {
            s.lastRaw      = raw;
            s.lastChangeMs = now;
        }

        if ((now - s.lastChangeMs) < BTN_DEBOUNCE_MS)
            continue; // Within debounce window – skip

        bool pressed = !raw; // Active LOW → pressed == LOW == false raw

        // ---- Transition: released → pressed ----
        if (pressed && !s.debounced)
        {
            s.debounced    = true;
            s.pressStartMs = now;
            s.longFired    = false;
            s.lastRepeatMs = now;
        }

        // ---- Transition: pressed → released ----
        if (!pressed && s.debounced)
        {
            s.debounced = false;
            if (!s.longFired)
            {
                // Short press – only fire if not already consumed by long-press
                if (s.pendingEvent == ButtonEvent::NONE)
                    s.pendingEvent = ButtonEvent::SHORT_PRESS;
            }
        }

        // ---- While held: long press + auto-repeat ----
        if (pressed && s.debounced)
        {
            uint32_t held = now - s.pressStartMs;

            // Long press threshold
            if (!s.longFired && held >= BTN_LONG_PRESS_MS)
            {
                s.longFired    = true;
                s.lastRepeatMs = now;
                if (s.pendingEvent == ButtonEvent::NONE)
                    s.pendingEvent = ButtonEvent::LONG_PRESS;
            }

            // Auto-repeat after long press
            if (s.longFired && (now - s.lastRepeatMs) >= BTN_REPEAT_INTERVAL_MS)
            {
                s.lastRepeatMs = now;
                if (s.pendingEvent == ButtonEvent::NONE)
                    s.pendingEvent = ButtonEvent::REPEAT;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Event polling
// ---------------------------------------------------------------------------
ButtonEvent ButtonManager::getEvent(Button btn)
{
    uint8_t idx = static_cast<uint8_t>(btn);
    if (idx >= NUM_BTNS) return ButtonEvent::NONE;

    ButtonEvent ev = _state[idx].pendingEvent;
    _state[idx].pendingEvent = ButtonEvent::NONE;
    return ev;
}

bool ButtonManager::isHeld(Button btn)
{
    uint8_t idx = static_cast<uint8_t>(btn);
    if (idx >= NUM_BTNS) return false;
    return _state[idx].debounced;
}
