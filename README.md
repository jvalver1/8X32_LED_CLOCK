# 8x32 WS2812B LED Clock

An Arduino Nano desk clock for a 256-pixel WS2812B matrix. It keeps time with a battery-backed DS3231 RTC, reads room temperature and atmospheric pressure from a BMP280, and provides three-button control for changing screens, brightness, time, and date. A complete Wokwi simulation is included.

## Features

- Custom 8x32 clock face with a calendar tile, `HH:MM`, blinking colon, and weekday indicator
- Date, temperature, pressure, and brightness screens
- Time/date setup without recompiling
- Seven selectable brightness levels
- DS3231 battery-backed timekeeping
- BMP280 automatic address detection at `0x76` or `0x77`
- Debounced short press, long press, and held-button repeat
- Optional 12-hour display and automatic screen cycling
- Boot and screen-transition animations
- PlatformIO debug, release, and Wokwi build environments

## How it works

`setup()` configures the three buttons and initializes the RTC, environmental sensor, FastLED display, and application state machine. The non-blocking main loop polls buttons, refreshes cached RTC and BMP280 data at configured intervals, selects the current screen, draws into a 256-element FastLED buffer, and sends the frame to the matrix.

The exception is scrolling text and short animations, which deliberately use brief delays while their frames play. The clock screen maps logical `(x,y)` pixels to the physical LED order and uses compact custom glyphs so a calendar tile and time fit in only eight rows. Normal hardware defaults to column-major serpentine wiring; Wokwi uses progressive row-major wiring.

## Hardware

### Bill of materials

| Qty | Part | Important details |
|---:|---|---|
| 1 | Arduino Nano | ATmega328P, 5 V logic, 16 MHz; `nanoatmega328new` is selected |
| 1 | WS2812B LED matrix | 8 rows x 32 columns, 256 individually addressable RGB LEDs, GRB order |
| 1 | DS3231 RTC module | I2C address `0x68`; fit a compatible backup coin cell |
| 1 | BMP280 breakout | Temperature and pressure only; I2C address `0x76` or `0x77` |
| 3 | Normally-open momentary buttons | Connected between an input and GND; firmware enables internal pull-ups |
| 1 | Regulated 5 V supply | Size for the selected brightness; 3 A or more is a practical starting point |
| 1 | 1000 uF electrolytic capacitor | Across matrix 5 V and GND, close to the panel; observe polarity |
| 1 | 330-470 ohm resistor | In series between Nano D6 and matrix DIN, close to the matrix |
| optional | 4.7 kohm resistors | SDA/SCL pull-ups if the breakout boards do not already provide them |

### Pinout

| Nano pin | Connects to | Notes |
|---|---|---|
| D2 | MODE button to GND | Active low, internal pull-up; INT0-capable |
| D3 | UP button to GND | Active low, internal pull-up; INT1-capable |
| D4 | DOWN button to GND | Active low, internal pull-up |
| D6 | resistor, then matrix DIN | WS2812B data |
| A4 / SDA | DS3231 SDA and BMP280 SDA | Shared I2C data |
| A5 / SCL | DS3231 SCL and BMP280 SCL | Shared I2C clock |
| GND | All device grounds | The Nano and external LED supply must share ground |

### Power and electrical guidance

Do not power a 256-pixel panel through the Nano's USB connector or onboard regulator. Feed the matrix directly from a regulated 5 V supply and join that supply's ground to Nano GND. Inject power at more than one point if wiring or panel voltage drop becomes visible.

A WS2812B can approach 60 mA at full-brightness white, so an unconstrained 256-pixel panel has a theoretical worst case near 15.4 A. This firmware defaults to brightness 60/255 and normally lights only part of the display, greatly reducing typical demand, but it does not implement a FastLED current limit. Size wiring, connectors, fusing, and supply for the intended use, and keep the 1000 uF capacitor and data resistor near the panel.

Many BMP280 breakouts contain a 3.3 V regulator and level shifting and accept 5 V; a bare BMP280 does not. Verify the markings and datasheet for your particular module. Likewise, confirm whether the RTC and sensor boards already include I2C pull-ups. DS3231 modules vary in their coin-cell charging circuit: never install a non-rechargeable cell in a module that actively charges it.

The BMP280 does **not** measure humidity. A BME280 is not a firmware-level drop-in here; its library and sensor manager would need updating.

### Matrix layout

Physical hardware defaults to a vertical, column-major serpentine arrangement:

- column 0 runs top to bottom;
- column 1 runs bottom to top;
- subsequent columns alternate.

Set `MATRIX_LAYOUT` in `src/config.h` or through a build flag if your panel is progressive row-major or serpentine row-major. A wrong selection produces mirrored, striped, or scrambled graphics without harming the LEDs.

## Controls

| Control | Action |
|---|---|
| MODE short press | Cycle clock, date, temperature, pressure, and brightness screens; advance setup fields |
| MODE long press | Enter time/date setup; while setting, cancel without saving |
| UP / DOWN | Change brightness or the selected setup value; hold to auto-repeat |

Saving occurs after advancing through hour, minute, day, month, and year. The seconds value is reset to zero. Date entry currently permits combinations such as 31 February; select a valid calendar date.

## Software structure

| Path | Purpose |
|---|---|
| `src/main.cpp` | Arduino entry point and subsystem initialization |
| `src/config.h` | Pins, timings, colors, layouts, and feature flags |
| `src/app/clock_app.*` | Screen and setup state machine |
| `src/display/display_manager.*` | FastLED buffer, coordinate mapping, fonts, clock UI, scrolling, animations |
| `src/display/font5x7.h` | Printable ASCII 5x7 bitmap font stored in flash |
| `src/input/button_manager.*` | Debouncing, short/long presses, and repeat events |
| `src/sensors/rtc_manager.*` | RTClib DS3231 wrapper and cached date/time |
| `src/sensors/bmp_manager.*` | BMP280 discovery, configuration, validation, and cached readings |
| `diagram.json`, `wokwi.toml` | Wokwi circuit and firmware configuration |
| `chips/` | Custom Wokwi DS3231 and BMP280 chip models |

The RTC is polled every second and the BMP280 every five seconds on hardware. The BMP280 runs in normal mode with x2 temperature oversampling, x16 pressure oversampling, x16 IIR filtering, and 500 ms standby. If the DS3231 reports loss of power, firmware initializes it to `2000-01-01 00:00:00`; use the buttons to set the correct value.

## Build and upload

Install [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode), or install PlatformIO Core.

```bash
# Debug build with 115200-baud serial diagnostics
pio run -e nano_328p

# Upload the debug build
pio run -e nano_328p -t upload

# Open the serial monitor
pio device monitor -b 115200

# Smaller release build without diagnostics
pio run -e nano_328p_release
```

The configured board uses the newer Nano bootloader. For an older Nano clone, change the board configuration to the appropriate PlatformIO Nano variant and upload speed if needed. PlatformIO downloads FastLED, Adafruit BMP280 Library, RTClib, and Adafruit BusIO automatically.

## Wokwi simulation

The repository includes an Arduino Nano, simulated 8x32 matrix, three buttons, and custom DS3231/BMP280 chips wired in `diagram.json`.

1. Install the Wokwi Simulator extension in VS Code.
2. Build with `pio run -e nano_328p_wokwi`.
3. Run **Wokwi: Start Simulator** from the command palette.

`wokwi.toml` loads `.pio/build/nano_328p_wokwi/firmware.hex` and `.elf`. The simulation build selects progressive row-major matrix mapping, polls the sensor every two seconds, and includes serial diagnostics.

## Configuration

Edit `src/config.h`, or override guarded values with PlatformIO `build_flags`.

| Setting | Default | Purpose |
|---|---:|---|
| `DEFAULT_BRIGHTNESS` | 60 | Startup FastLED brightness, 0-255 scale |
| `MIN_BRIGHTNESS` / `MAX_BRIGHTNESS` | 5 / 200 | Manual bounds |
| `DISPLAY_REFRESH_MS` | 33 | Intended display refresh interval |
| `RTC_POLL_MS` | 1000 | RTC cache refresh |
| `SENSOR_POLL_MS` | 5000 | BMP280 cache refresh |
| `AUTO_SCROLL_DURATION_MS` | 4000 | Optional screen dwell time |
| `BTN_DEBOUNCE_MS` | 50 | Button debounce |
| `BTN_LONG_PRESS_MS` | 800 | Long-press threshold |
| `BTN_REPEAT_INTERVAL_MS` | 150 | Held-button repeat interval |

Optional compile-time features include `CLOCK_12H_MODE`, `FEATURE_AUTO_SCROLL`, `FEATURE_BOOT_ANIMATION`, and the currently unimplemented/disabled `FEATURE_AUTO_BRIGHTNESS` hook.

## Troubleshooting

- **No display:** verify external 5 V, common ground, DIN rather than DOUT, D6, data resistor, and matrix layout.
- **Random colors or resets:** improve the power supply, ground connection, decoupling, and data wiring; reduce brightness.
- **RTC not found:** check A4/A5, address `0x68`, breakout power, and pull-ups.
- **Temperature shows N/A:** check A4/A5 and power; firmware probes both `0x76` and `0x77`.
- **Upload fails:** confirm the serial port and whether the Nano uses the old or new bootloader.
- **Garbled geometry:** select the matrix wiring layout matching the physical panel.

## License

No license has been selected. Until one is added, copyright law reserves reuse and redistribution rights to the author even though the repository is public.
