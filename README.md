


## Expression2MIDI — Overview
Small ESP32 firmware that reads an expression pedal (analog TRS), maps the pedal position through selectable curves, and sends MIDI Control Change (CC) messages over a UART TX pin (31250 baud). Targets an SPI TFT for configuration and a clickable rotary encoder for UI.

## Table of contents
- Overview
- Quick Start
- Dependencies
- Hardware & Wiring
- Pin mapping & TFT orientation
- Regenerating CC definitions
- Usage / UI cheat-sheet
- Calibration & units
- Examples & testing
- Troubleshooting
- Persistence / EEPROM
- Contributing

## Quick Start
Build and upload with PlatformIO (project root contains `platformio.ini`):

```bash
platformio run --target upload
```

Open serial monitor for logs:

```bash
platformio device monitor --port COMx --baud 115200
```

Replace `COMx` with your Windows COM port. After flashing, use the encoder and Aux button to navigate the menus and the Monitor view to validate pedal readings.

## Dependencies
This project uses the following Arduino/PlatformIO libraries (see `lib_deps` in `platformio.ini`):
- FortySevenEffects `MIDI` (hardware MIDI)
- `Adafruit_GFX`
- `Adafruit_ST7796S` (display driver used here)
- `ESP32Encoder`
- `SPI` (Arduino core)

Install via PlatformIO `lib_deps` or the Library Manager.

## Hardware & Wiring
- Expression pedal (TRS): Tip → `PEDAL_PIN` (ADC input), Ring → 3.3V, Sleeve → GND.
- MIDI DIN/TX: TTL MIDI is sent on the configured `MIDI_TX_PIN` (see `include/pin_definitions.h`). The board uses `Serial1` at 31250 baud.
- Encoder: standard A/B pins + push switch (pins in `include/pin_definitions.h`).
- TFT: SPI pins plus `TFT_CS`, `TFT_DC`, `TFT_RST` defined in `include/pin_definitions.h`.

See `include/pin_definitions.h` for the specific pin names used in this build: [include/pin_definitions.h](include/pin_definitions.h).

## Pin mapping & TFT orientation
- Default TFT initialization happens in `src/main.cpp` (`tft.init(320,480)` and `tft.setRotation(0)`). Adjust rotation or width/height there if your panel is oriented differently.

## Regenerating CC definitions
CC label mappings are generated from XMLs in `ccSources/`. To regenerate the header used by the firmware:

```bash
python ccSources/process_files.py
```

This writes/updates `include/cc_definitions.h`. See `ccSources/README.md` for details about the XML sources. 

## Usage / UI cheat-sheet
- Turn encoder CW/CCW: move selection or change numeric values.
- Press encoder: select / save current value in many menus (e.g. save CC number).
- Aux/back button: Return to main menu. On the main menu, this is a quick shortcut to enter monitor view.
- Monitor: shows raw (calibrated) ADC (0–4095) and mapped MIDI value (0–127). Use it while moving the pedal.
- Calibration: set MIN then MAX values in the Calibration menu and save to persist to EEPROM.

## Calibration & units
- ADC range on ESP32: 0–4095 (12-bit). The firmware normalizes reads to 0–1023 internally, then maps to MIDI 0–127.
- Calibration stores pedal `min` and `max` to EEPROM and uses those endpoints for normalization. Use the Calibration menu to set and save values.

## Examples & testing
Quick serial debug and platformio monitor commands are above. To verify MIDI output you can use a MIDI DIN→USB interface or a hardware synth that listens on the DIN cable. If using a PC, tools like `MIDI-OX` (Windows) or `Midi Monitor` (macOS) will show incoming CC messages.

## Troubleshooting
- No MIDI output: verify `MIDI_TX_PIN` in `include/pin_definitions.h` and that the DIN wiring matches the MIDI device. `Serial1` is configured at 31250 baud.
- Unexpected ADC range: re-check TRS wiring (Tip=ADC, Ring=3.3V, Sleeve=GND).
- TFT blank/flicker: confirm SPI pins and `tft.init()` parameters and rotation.
- Encoder jitter: ensure encoder pins are set with pull-ups (code enables `INPUT_PULLUP` and uses `ESP32Encoder` internal pull-ups).

## Persistence / EEPROM
Settings persisted to EEPROM include selected curve, invert flag, active instrument index, MIDI CC, MIDI channel, and calibration endpoints. Default values are initialized in `src/main.cpp` (e.g. `CC_NUMBER = 74`, `MIDI_CHANNEL = 2`) and in `MenuManager` defaults; stored values overwrite them at startup.

----

This was a tool meant to accompish a specific want of mine, so I may not develop it much more on my own as it fulfills that goal.  That said, I'm happy to help with it or do more work on it if you have a feature idea, feel free to open one up.  

Do not spend $200 or whatever insane price they're charging to do this.  This project is easy to do, is **way** cheaper, and is more powerful than anything on the market that I've seen.