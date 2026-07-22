/**
 * @file    button_manager.h
 * @brief   Button Manager – debounced button input with short/long-press detection.
 *
 * @details Handles three buttons (MODE, UP, DOWN) with:
 *          - Software debouncing (BTN_DEBOUNCE_MS)
 *          - Short press detection (released before BTN_LONG_PRESS_MS)
 *          - Long press detection (held beyond BTN_LONG_PRESS_MS)
 *          - Auto-repeat while held (BTN_REPEAT_INTERVAL_MS)
 *
 *          All buttons are active LOW with internal pull-ups enabled.
 */

#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Button identifiers
// ---------------------------------------------------------------------------
enum class Button : uint8_t
{
    MODE = 0,
    UP   = 1,
    DOWN = 2,
    NUM_BUTTONS
};

// ---------------------------------------------------------------------------
// Button event types
// ---------------------------------------------------------------------------
enum class ButtonEvent : uint8_t
{
    NONE        = 0,
    SHORT_PRESS = 1,   ///< Pressed and released quickly
    LONG_PRESS  = 2,   ///< Held beyond BTN_LONG_PRESS_MS
    REPEAT      = 3,   ///< Auto-repeat while held
};

// ---------------------------------------------------------------------------
// ButtonManager (static class)
// ---------------------------------------------------------------------------
class ButtonManager
{
public:
    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /** Configure GPIO pins and reset internal state. */
    static void init();

    /**
     * @brief  Must be called every loop tick to sample button state.
     *         Internally tracks debounce, press duration, and auto-repeat.
     */
    static void update();

    // -----------------------------------------------------------------------
    // Event polling
    // -----------------------------------------------------------------------

    /**
     * @brief  Consume the next event for a specific button.
     * @return ButtonEvent::NONE if no event pending, otherwise the event type.
     *         Consuming an event clears it from the queue.
     */
    static ButtonEvent getEvent(Button btn);

    /**
     * @brief  Check if a button is currently held down (raw state).
     * @return true if the button is currently pressed.
     */
    static bool isHeld(Button btn);

private:
    static constexpr uint8_t NUM_BTNS = static_cast<uint8_t>(Button::NUM_BUTTONS);

    struct BtnState
    {
        bool          lastRaw;          ///< Raw GPIO reading (last sample)
        bool          debounced;        ///< Debounced state
        uint32_t      lastChangeMs;     ///< millis() at last raw change
        uint32_t      pressStartMs;     ///< millis() when press began
        bool          longFired;        ///< Long-press event already sent
        uint32_t      lastRepeatMs;     ///< millis() of last auto-repeat
        ButtonEvent   pendingEvent;     ///< Event waiting to be consumed
    };

    static BtnState _state[NUM_BTNS];

    static constexpr uint8_t PIN[NUM_BTNS] = {
        BTN_MODE_PIN,
        BTN_UP_PIN,
        BTN_DOWN_PIN
    };

    ButtonManager() = delete;
};
