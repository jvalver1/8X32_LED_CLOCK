# Clock Setup Routine - AI Implementation Specification

## 1. System Overview
This document outlines the operational logic and state machine for a digital clock setup routine. The specification is structured to be directly translatable into microcontroller code (e.g., C/C++, Python) by an AI assistant.

### 1.1 Hardware Components
*   **Microcontroller (MCU):** The core processor handling state logic and GPIO.
*   **LED Screen:** Display unit capable of rendering time/date and flashing specific digit fields.
*   **RTC Module:** Real-Time Clock maintaining the time, date, and Daylight Saving Time (DST) flag.
*   **Buttons (Active Low or High, to be debounced in software):**
    *   `BTN_SETUP`: Used for entering, navigating, and exiting the setup mode.
    *   `BTN_UP`: Increments the currently selected field.
    *   `BTN_DOWN`: Decrements the currently selected field.

---

## 2. Input/Event Definitions

### 2.1 Button Events
The system requires a button handling library capable of distinguishing between:
*   `EVENT_BTN_SETUP_LONG_PRESS`: Triggered when `BTN_SETUP` is held for a predefined duration (e.g., > 1000ms).
*   `EVENT_BTN_SETUP_SHORT_PRESS`: Triggered on release of `BTN_SETUP` if held for less than the long-press threshold.
*   `EVENT_BTN_UP_PRESS`: Triggered on a standard press of the Up button.
*   `EVENT_BTN_DOWN_PRESS`: Triggered on a standard press of the Down button.

### 2.2 System Events
*   `EVENT_TIMEOUT`: Triggered when an inactivity timer reaches **7 seconds** with no button events registered.

---

## 3. State Machine Architecture

The setup process is modeled as a Finite State Machine (FSM).

### 3.1 States
*   `STATE_IDLE`: Normal clock operation. Display updates from RTC.
*   `STATE_SETUP_HOUR`: Adjusting hours. Hour digits flash.
*   `STATE_SETUP_MINUTE`: Adjusting minutes. Minute digits flash.
*   `STATE_SETUP_DAY`: Adjusting day of the month. Day digits flash.
*   `STATE_SETUP_MONTH`: Adjusting month. Month digits flash.
*   `STATE_SETUP_YEAR`: Adjusting year. Year digits flash.
*   `STATE_SETUP_MODE`: Adjusting time format (12h/24h). Format indicator flashes.
*   `STATE_SETUP_BRIGHTNESS`: Adjusting the LED brightness on a 1-10 scale.

### 3.2 State Transitions
The system initializes in `STATE_IDLE`.

| Current State | Event | Next State | Action/Effect |
| :--- | :--- | :--- | :--- |
| `STATE_IDLE` | `EVENT_BTN_SETUP_LONG_PRESS`| `STATE_SETUP_HOUR` | Cache current RTC time to temporary buffer. Start 7s timer. Flash hours. |
| `STATE_SETUP_HOUR` | `EVENT_BTN_SETUP_SHORT_PRESS`| `STATE_SETUP_MINUTE` | Reset 7s timer. Flash minutes. |
| `STATE_SETUP_MINUTE` | `EVENT_BTN_SETUP_SHORT_PRESS`| `STATE_SETUP_DAY` | Reset 7s timer. Flash days. |
| `STATE_SETUP_DAY` | `EVENT_BTN_SETUP_SHORT_PRESS`| `STATE_SETUP_MONTH` | Reset 7s timer. Flash months. |
| `STATE_SETUP_MONTH` | `EVENT_BTN_SETUP_SHORT_PRESS`| `STATE_SETUP_YEAR` | Reset 7s timer. Flash years. |
| `STATE_SETUP_YEAR` | `EVENT_BTN_SETUP_SHORT_PRESS`| `STATE_SETUP_MODE` | Reset 7s timer. Flash 12h/24h indicator. |
| `STATE_SETUP_MODE` | `EVENT_BTN_SETUP_SHORT_PRESS`| `STATE_SETUP_BRIGHTNESS` | Reset 7s timer. Flash brightness level. |
| `STATE_SETUP_BRIGHTNESS` | `EVENT_BTN_SETUP_SHORT_PRESS`| `STATE_IDLE` | Trigger **Save Routine** (See Section 5). |
| *ANY SETUP STATE*| `EVENT_BTN_SETUP_LONG_PRESS`| `STATE_IDLE` | Trigger **Save Routine** (See Section 5). |
| *ANY SETUP STATE*| `EVENT_TIMEOUT` | `STATE_IDLE` | Trigger **Save Routine** (See Section 5). |

---

## 4. Field Adjustment Logic

While in any setup state (excluding `STATE_IDLE`), pressing `BTN_UP` or `BTN_DOWN` triggers the following:

1.  **Reset Timeout:** The 7-second inactivity timer is immediately reset.
2.  **Modify Buffer:** The temporary buffer holding the time/date variable is updated.
3.  **Boundary Wrapping:**
    *   **Hour:** Wraps 0-23 (if in 24h internal logic).
    *   **Minute:** Wraps 0-59.
    *   **Month:** Wraps 1-12.
    *   **Year:** Wraps logically (e.g., 2000-2099).
    *   **Day:** Wraps 1 - `MAX_DAYS_IN_MONTH`. *Must dynamically calculate maximum days based on the currently selected month and year (Leap year logic required).*
    *   **Mode:** Toggles between `MODE_12H` and `MODE_24H`.
    *   **Brightness:** Wraps 1-10 and maps across the configured safe LED brightness range.

---

## 5. Save and Exit Routine

Whenever the FSM transitions back to `STATE_IDLE` (via completion, long press, or timeout), the system must execute the `Exit_And_Save()` function:

### 5.1 Exit_And_Save() Implementation Steps:
1.  **Stop Flashing:** Send command to LED screen to stop flashing digits.
2.  **Calculate DST:** Call the DST calculation function based on the newly set date (See Section 6).
3.  **Update RTC:** Write the temporary time/date buffer and the calculated DST flag to the RTC module hardware registers.
4.  **Update Display:** Force an immediate refresh of the LED screen to show the new time/date in non-flashing mode.

---

## 6. Business Logic: European DST Calculation

The DST flag must be dynamically calculated upon exiting the setup mode. The European Union DST rules are:
*   **Start:** Last Sunday of March at 01:00 UTC (Clocks go forward).
*   **End:** Last Sunday of October at 01:00 UTC (Clocks go back).

### 6.1 Algorithmic Approach for AI Implementation:
To determine if a given `Date (Day, Month, Year)` falls within the DST period:
1.  If `Month < 3` or `Month > 10`: **DST is OFF**.
2.  If `Month > 3` and `Month < 10`: **DST is ON**.
3.  If `Month == 3` (March):
    *   Calculate the date of the last Sunday in March for the given `Year`.
    *   If `Day >= Last Sunday`: **DST is ON**. Else: **DST is OFF**.
4.  If `Month == 10` (October):
    *   Calculate the date of the last Sunday in October for the given `Year`.
    *   If `Day < Last Sunday`: **DST is ON**. Else: **DST is OFF**.

*(Note: If precision down to the exact hour of the switch is required, include the temporary buffer's `Hour` field in the edge-case day comparison).*

### 6.2 Display considerations for DST
If the UI requires an indicator for DST, ensure the display rendering logic queries the newly updated RTC DST flag to toggle the UI element accordingly.
