


## Curve Menu:
Turn the encoder to scroll through available curves. Click the encoder to select (and autosave) the selected curve.  Aux button returns to main menu.

## MIDI CC Definition files
An instrument can be selected to have its CC meanings placed under the CC values when scrolling through them. These are generated using xmls derived from what's offered by midi.guide. They can be generated using `process_files.py`.  See the readme in `ccSources` for more information.

## MIDI CC Menu (how it works)
- The top-left of the `MIDI CC` menu shows the currently active instrument in magenta ("None" if no instrument is active).
- The center of the screen displays a large MIDI CC number (0–127). Turn the encoder CW/CCW to change the CC number (it wraps 0↔127).
- Under the number the UI will show a textual label for the CC if the active instrument provides one.
- Changes are only written to persistent storage when you press the encoder button; press the encoder to save the current CC. When saved, the UI draws a small "saved" badge over the CC number.
- Instrument CC mappings are supplied by the generated header `include/cc_definitions.h`. Run `python ccSources/process_files.py` to regenerate `cc_definitions.h` from the XMLs in `ccSources/` whenever you add or update instrument files.

## Monitor Menu
- Shows the current raw ADC reading (big, yellow) and the mapped MIDI value (magenta) produced by the selected curve and mapping. A horizontal progress bar near the bottom visualizes the 0–127 MIDI range.
- The Monitor view is intended for live feedback while moving the pedal; it updates at a short poll interval and avoids redrawing large static elements unnecessarily.
- Shortcut: from the main menu you can immediately jump to the Monitor view by pressing the back/aux button.

## Instrument Definitions Menu
- Lists instruments discovered in `include/cc_definitions.h` (generated from `ccSources/`). The list always starts with `"None"` to indicate no instrument is active.
- Use the encoder to highlight an instrument and press the encoder button to make it the active instrument; the active instrument index is persisted to storage.
- When an instrument is active, its CC label mappings are used by the `MIDI CC` menu to show human-readable names under CC numbers.

## Invert Menu
- Toggle between `Normal` and `Inverted` behavior for the applied curve mapping using the encoder. Press the encoder button to commit the selection; the choice is persisted.
- The menu highlights the staged selection and marks the currently persisted choice with an `[active]` tag.


TODOS:
normalize the formatting for "active" in highlight-selection menuslike invert and instrument definitions.  Magenta font for unhighligted but active selection, green font (on white background) for highlighted and selected option.