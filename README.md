# 8x32 WS2812B LED Clock

An Arduino Pro Mini desk clock for a 256-pixel WS2812B matrix. It keeps time with a battery-backed DS3231 RTC, reads temperature, humidity, and atmospheric pressure from a BME280, and provides three-button control for changing screens, font colour, time, date, clock format, brightness, and automatic carousel timing. A complete Arduino Nano-based Wokwi simulation is included.

## Features

- Custom 8x32 clock face with a calendar tile, `HH:MM`, blinking colon, and weekday indicator
- Date, temperature, humidity, and pressure screens
- Setup for time, date, 12/24-hour format, and brightness without recompiling
- DS3231 battery-backed timekeeping
- BME280 automatic address detection at `0x76` or `0x77`
- Smooth 16-colour font palette selected with the leftmost button
- Debounced short press, long press, and held-button repeat
- Runtime carousel with 5/7-second timing, orange/red mode indicators, and a pull-down transition
- Boot and screen-transition animations
- PlatformIO release and Wokwi build environments
- Release-only upload workflow that cannot accidentally upload the Wokwi target

## How it works

`setup()` configures the three buttons and initializes the RTC, environmental sensor, FastLED display, and application state machine. The non-blocking main loop polls buttons, refreshes cached RTC and BME280 data at configured intervals, selects the current screen, draws into a 256-element FastLED buffer, and sends the frame to the matrix.

The exception is scrolling text and short animations, which deliberately use brief delays while their frames play. The clock screen maps logical `(x,y)` pixels to the physical LED order and uses compact custom glyphs so a calendar tile and time fit in only eight rows. Normal hardware defaults to column-major serpentine wiring; Wokwi uses progressive row-major wiring.

## Hardware

### Bill of materials

| Qty | Part | Important details |
|---:|---|---|
| 1 | Arduino Pro Mini | ATmega328P, 5 V logic, 16 MHz; PlatformIO board `pro16MHzatmega328` |
| 1 | WS2812B LED matrix | 8 rows x 32 columns, 256 individually addressable RGB LEDs, GRB order |
| 1 | DS3231 RTC module | I2C address `0x68`; fit a compatible backup coin cell |
| 1 | BME280 breakout | Temperature, humidity, and pressure; I2C address `0x76` or `0x77` |
| 3 | Normally-open momentary buttons | Connected between an input and GND; firmware enables internal pull-ups |
| 1 | Regulated 5 V / 4 A supply | Firmware reserves 0.5 A and budgets up to 3.5 A for the LEDs |
| 1 | 1000 uF electrolytic capacitor | Across matrix 5 V and GND, close to the panel; observe polarity |
| 1 | 330-470 ohm resistor | In series between Pro Mini D6 and matrix DIN, close to the matrix |
| optional | 4.7 kohm resistors | SDA/SCL pull-ups if the breakout boards do not already provide them |

### Pinout

| Pro Mini pin | Connects to | Notes |
|---|---|---|
| D3 | MODE button to GND | Active low, internal pull-up; INT1-capable |
| D4 | UP button to GND | Active low, internal pull-up |
| D5 | DOWN button to GND | Active low, internal pull-up |
| D6 | resistor, then matrix DIN | WS2812B data |
| A4 / SDA | DS3231 SDA and BME280 SDA | Shared I2C data |
| A5 / SCL | DS3231 SCL and BME280 SCL | Shared I2C clock |
| GND | All device grounds | The Pro Mini and external LED supply must share ground |

### Power and electrical guidance

Do not power a 256-pixel panel through the Pro Mini regulator or USB-to-serial adapter. Feed the matrix directly from a regulated 5 V supply and join that supply's ground to Pro Mini GND. Inject power at more than one point if wiring or panel voltage drop becomes visible.

A WS2812B can approach 60 mA at full-brightness white, so an unconstrained 256-pixel panel has a theoretical worst case near 15.4 A. For a regulated 5 V / 4 A source, this firmware reserves 0.5 A for the controller, sensors, losses, and margin, then gives the LEDs a 3.5 A budget. The physical 1-10 scale maps to FastLED values 2-58 and starts at level 5 (value 27). FastLED also enforces an estimated 3500 mA LED ceiling. These are software safeguards, not a replacement for suitable wiring, power injection, connectors, fusing, decoupling, and verification with the actual panel and supply.

Many BME280 breakouts contain a 3.3 V regulator and level shifting and accept 5 V; a bare BME280 does not. Verify the markings and datasheet for your particular module. Likewise, confirm whether the RTC and sensor boards already include I2C pull-ups. DS3231 modules vary in their coin-cell charging circuit: never install a non-rechargeable cell in a module that actively charges it.

### Matrix layout

Physical hardware defaults to a vertical, column-major serpentine arrangement:

- column 0 runs top to bottom;
- column 1 runs bottom to top;
- subsequent columns alternate.

Set `MATRIX_LAYOUT` in `src/config.h` or through a build flag if your panel is progressive row-major or serpentine row-major. A wrong selection produces mirrored, striped, or scrambled graphics without harming the LEDs.

## Controls

| Control | Action |
|---|---|
| Leftmost short press | Advance the font through the smooth 16-colour palette |
| Leftmost long press | Enter setup; while setting, save immediately and return to the clock |
| Centre short press | Cycle screens in reverse |
| Rightmost short press | Cycle screens forward |
| Centre / rightmost long press | Carousel off -> 5 seconds -> 7 seconds -> off |
| Centre / rightmost in setup | Decrease / increase the selected value; hold to auto-repeat |

The carousel cycles through clock, temperature, humidity, and pressure; it deliberately skips the date. A bottom-right dot flashes orange for the 5-second mode and red for the 7-second mode. Entering setup disables it. Stopping the carousel leaves the currently displayed screen selected and removes the indicator.

Setup fields appear in this order:

1. Hour
2. Minute
3. Day
4. Month
5. Year
6. 12/24-hour format
7. Brightness level from 1 to 10

The leftmost short press advances to the next field. The selected field flashes, and any button activity restarts the seven-second inactivity timer. Completing the final field, holding the leftmost button, or allowing the setup timer to expire saves the buffered date/time, display format, and brightness. The seconds value is reset to zero. Day values are constrained to the selected month and year, including leap years. Brightness changes are previewed immediately while editing.

The 1-10 brightness value is the stable user-facing setting; the firmware maps it to the target-specific FastLED range. Physical hardware uses values 2-58, while Wokwi uses 26-255 so the simulator can show colour gradients clearly. Both start at level 5.

## Software structure

| Path | Purpose |
|---|---|
| `src/main.cpp` | Arduino entry point and subsystem initialization |
| `src/config.h` | Pins, timings, colors, layouts, and feature flags |
| `src/app/clock_app.*` | Screen and setup state machine |
| `src/display/display_manager.*` | FastLED buffer, coordinate mapping, fonts, clock UI, scrolling, animations |
| `src/display/font3x5.h` | Compact numeric 3x5 bitmap font stored in flash |
| `src/input/button_manager.*` | Debouncing, short/long presses, and repeat events |
| `src/sensors/rtc_manager.*` | RTClib DS3231 wrapper and cached date/time |
| `src/sensors/bme_manager.*` | BME280 discovery, configuration, validation, and cached readings |
| `diagram.json`, `wokwi.toml` | Wokwi circuit and firmware configuration |
| `chips/` | Custom Wokwi DS3231 and BME280 chip models |
| `scripts/build_wokwi.py` | Builds Wokwi after a normal release build, but skips it for uploads |
| `wokwi-icons.test.yaml` | Automated simulator navigation and environmental-icon screenshots |

The RTC is polled every second and the BME280 every five seconds on hardware. The BME280 runs in normal mode with x2 temperature, x16 pressure, and x1 humidity oversampling, x16 IIR filtering, and 500 ms standby. If the DS3231 reports loss of power, firmware initializes it to `2000-01-01 00:00:00`; use the buttons to set the correct value.

### Display and colour implementation

FastLED owns a 256-element `CRGB` framebuffer. Environmental artwork is stored in flash as native 24-bit `0xRRGGBB` RGB888 values and copied directly into `CRGB`; there is no RGB565 conversion or reduced icon palette. Temperature and humidity use 5x8 artwork, and the pressure screen uses the current 5x8 orange/red-to-blue RGB888 icon. Unused columns in the common 8x8 icon table are transparent black.

The selectable font palette contains 16 RGB888 colours. Changing font colour blends from the current colour to the next over 350 ms. Wokwi uses an uncorrected FastLED colour profile and a matrix brightness multiplier of `1`, avoiding channel saturation and preserving the RGB888 gradients.

The pull-down carousel transition saves the outgoing frame in a compact RGB332 buffer to stay within the ATmega328P's 2 KB SRAM. This temporary reduction applies only while reconstructing the outgoing portion of the transition; normal screens and icons remain RGB888.

## Build and upload

Install [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode), or install PlatformIO Core.

```powershell
# Build the Pro Mini release and then the Wokwi firmware
C:\Users\juanjov\.platformio\penv\Scripts\platformio.exe run

# Build and upload only the Pro Mini release firmware
C:\Users\juanjov\.platformio\penv\Scripts\platformio.exe run --target upload

# Build only the Wokwi target when required
C:\Users\juanjov\.platformio\penv\Scripts\platformio.exe run -e nano_328p_wokwi
```

`pro_mini_328p_release` is the sole default PlatformIO environment. Its post-build hook launches `nano_328p_wokwi` only for a normal build. The hook does not run for the `upload` target, so the upload command programs only the physical Pro Mini firmware. PlatformIO downloads FastLED, Adafruit BME280 Library, RTClib, and Adafruit BusIO automatically.

## Wokwi simulation

The repository includes an Arduino Nano, simulated 8x32 matrix, three buttons, and custom DS3231/BME280 chips wired in `diagram.json`.

1. Install the Wokwi Simulator extension in VS Code.
2. Run a normal build (which builds both targets), or explicitly build `nano_328p_wokwi`.
3. Run **Wokwi: Start Simulator** from the command palette.

Stop and restart Wokwi after rebuilding; a running simulation does not hot-reload a changed firmware image. `wokwi.toml` loads `.pio/build/nano_328p_wokwi/firmware.hex` and `.elf`. The simulation build selects progressive row-major matrix mapping, polls the sensor every two seconds, includes serial diagnostics, and uses a wider 26-255 brightness range for display fidelity.

The matrix layout is explicit in `platformio.ini`: the physical release build
uses `MATRIX_LAYOUT=0` (serpentine column-major), while Wokwi uses
`MATRIX_LAYOUT=2` (progressive row-major). `MATRIX_LAYOUT=1` is available for a
physical serpentine row-major panel.

### Simulation screenshots

The environmental screens below are generated by `wokwi-icons.test.yaml` from
the same `nano_328p_wokwi` firmware loaded by `wokwi.toml`.

| Temperature | Humidity | Pressure |
|---|---|---|
| ![Wokwi temperature screen](wokwi-temperature.png) | ![Wokwi humidity screen](wokwi-humidity.png) | ![Wokwi pressure screen](wokwi-pressure.png) |

The following full-matrix RGB test frames show the simulator retaining colour
gradations at low and high intensity after correcting the Wokwi brightness
multiplier and moving the icon artwork to RGB888.

| Low-intensity RGB gradient | High-intensity RGB gradient |
|---|---|
| ![Wokwi low-intensity RGB gradient](wokwi-rainbow-early.png) | ![Wokwi high-intensity RGB gradient](wokwi-rainbow-late.png) |

## Configuration

Edit `src/config.h`, or override guarded values with PlatformIO `build_flags`.

| Setting | Default | Purpose |
|---|---:|---|
| `DEFAULT_BRIGHTNESS_LEVEL` | 5 | Startup value on the user-facing 1-10 scale |
| `MIN_BRIGHTNESS` / `MAX_BRIGHTNESS` | 2 / 58 | Manual bounds sized for a regulated 5 V / 4 A source |
| `LED_MAX_MILLIAMPS` | 3500 | FastLED LED-current ceiling, leaving 0.5 A system headroom |
| Wokwi `MIN_BRIGHTNESS` / `MAX_BRIGHTNESS` | 26 / 255 | Simulation-only range preserving visible RGB gradients |
| `DISPLAY_REFRESH_MS` | 33 | Intended display refresh interval |
| `RTC_POLL_MS` | 1000 | RTC cache refresh |
| `SENSOR_POLL_MS` | 5000 | BME280 cache refresh |
| `CAROUSEL_SHORT_DELAY_MS` / `CAROUSEL_LONG_DELAY_MS` | 5000 / 7000 | Runtime carousel dwell times |
| `DST_ACTIVE` | 0 | Add the one-hour DST display offset when set to 1 |
| `CLOCK_12H_FORMAT` | 0 | Use 12-hour display when set to 1; default is 24-hour |
| `BTN_DEBOUNCE_MS` | 50 | Button debounce |
| `BTN_LONG_PRESS_MS` | 800 | Long-press threshold |
| `BTN_REPEAT_INTERVAL_MS` | 150 | Held-button repeat interval |
| `SETUP_TIMEOUT_MS` | 7000 | Save and leave setup after inactivity |
| `SETUP_FLASH_MS` | 500 | Selected-field flash interval |

Optional compile-time features include `FEATURE_BOOT_ANIMATION` and the currently unimplemented/disabled `FEATURE_AUTO_BRIGHTNESS` hook.

## Troubleshooting

- **No display:** verify external 5 V, common ground, DIN rather than DOUT, D6, data resistor, and matrix layout.
- **Random colors or resets:** improve the power supply, ground connection, decoupling, and data wiring; reduce brightness.
- **RTC not found:** check A4/A5, address `0x68`, breakout power, and pull-ups.
- **Temperature shows N/A:** check A4/A5 and power; firmware probes both `0x76` and `0x77`.
- **Upload fails:** confirm the serial port, USB-to-serial adapter wiring, shared ground, and automatic-reset/DTR connection for the Pro Mini.
- **Garbled geometry:** select the matrix wiring layout matching the physical panel.

## License

No license has been selected. Until one is added, copyright law reserves reuse and redistribution rights to the author even though the repository is public.
