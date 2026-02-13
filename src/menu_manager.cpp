
#include "menu_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <SPI.h>
#include <Arduino.h>
#include <cstring>
#include "color_utils.h"
#include "eeprom_storage.h"
#include <cmath>
#include <climits>
#include "curve_constants.h"
#include "cc_definitions.h"
#include "pin_definitions.h"

// Forward declaration: curve preview helper used by `renderCurveMenu`
// Added `active` so the preview can indicate which curve is currently active
static void drawCurvePreview(Adafruit_ST7796S* tft, CurveType ct, CurveType active);


#define TOP_MENU_MARGIN 50
// menuHandlersTable is defined at bottom of this file
// Forward declaration so code above can reference it
extern const MenuHandlers menuHandlersTable[MENU_COUNT];

MenuManager::MenuManager()
    : _tft(nullptr), _initialized(false), _currentMenu(MENU_MAIN) {
    _mainSelectedIdx = 0;
    _lastRaw = -1;
    _lastNorm = -1;
    _monitorTriangleDrawn = false;
    _monitorArrowTopY = -1;
    _monitorArrowCX = -1;
    _currentCurve = CURVE_LINEAR;
    _stagedCurve = CURVE_LINEAR;
    _inverted = false;
    _invertSelectedIdx = 0;
    _activeInstrumentIdx = 0;
    _instrumentSelectedIdx = 0;
    _instrumentTopIdx = 0;
    _activeCC = 0;
    _selectedCC = 0;
    _ccSaved = false;
    _activeChannel = 15; // store channels 1..16 internally (1-indexed)
    _channelSaved = false;
    _pedalMin = 0;
    _pedalMax = 4095;
    _calibSettingMax = false;
    _calibSaved = false;
    _calibSavedIsMax = false;
}

void MenuManager::begin(Adafruit_ST7796S* tft) {
    _tft = tft;
    if (!_tft) return;
    _initialized = true;
    // Load persisted curve selection (defaults to linear if not present)
    uint8_t c = eeprom_getCurve();
    if (c >= CURVE_COUNT) c = CURVE_LINEAR;
    _currentCurve = (CurveType)c;
    _stagedCurve = _currentCurve;
    // Load persisted invert flag
    uint8_t inv = eeprom_getInvert();
    _inverted = (inv != 0);
    _invertSelectedIdx = _inverted ? 1 : 0;
    // Load persisted active instrument (0 == None) and clamp against current list
    uint8_t ai = eeprom_getActiveInstrument();
    // Count instruments using sentinel
    int total = 0;
    for (const InstrumentCCs* p = allInstruments; p && p->name; ++p) ++total;
    if (total <= 0) {
        _activeInstrumentIdx = 0;
        _instrumentSelectedIdx = 0;
        _instrumentTopIdx = 0;
    } else {
        if ((int)ai >= total) ai = 0; // reset to None if stored index out-of-range
        _activeInstrumentIdx = (int)ai;
        _instrumentSelectedIdx = _activeInstrumentIdx;
        _instrumentTopIdx = 0;
    }
    // Load cached CC once to avoid repeated EEPROM reads
    uint8_t savedCC = eeprom_getCC();
    if (savedCC > 127) savedCC = 127;
    _activeCC = savedCC;
    _selectedCC = savedCC;
    _ccSaved = false;
    // Load persisted MIDI channel if present
    int8_t savedCh = (int8_t)eeprom_getChannel();
    if (savedCh < 1) savedCh = 1;
    if (savedCh > 16) savedCh = 16;
    _activeChannel = savedCh;
    _channelSaved = false;
    // Load persisted pedal calibration (if present). We don't auto-save on change here.
    _pedalMin = (int)eeprom_getPedalMin();
    _pedalMax = (int)eeprom_getPedalMax();
    if (_pedalMin < 0) _pedalMin = 0;
    if (_pedalMax < 0) _pedalMax = 4095;
    if (_pedalMin > 4095) _pedalMin = 0;
    if (_pedalMax > 4095) _pedalMax = 4095;
    // calibration UI flags
    _calibSettingMax = false; // default: first action is to set MIN
    _calibSaved = false;
    _calibSavedIsMax = false;
}

void MenuManager::render() {
    if (!_initialized) return;
    switch (_currentMenu) {
        case MENU_MAIN:
            renderMainMenu();
            break;
        case MENU_MONITOR:
            renderMonitorMenu();
            break;
        case MENU_MIDI_CHANNEL:
            renderMidiChannelMenu();
            break;
        case MENU_MIDI_CC_NUMBER:
            renderMidiCCMenu();
            break;
        case MENU_CALIBRATION:
            renderCalibrationMenu();
            break;
        case MENU_INVERT:
            renderInvertMenu();
            break;
        case MENU_INSTRUMENTS:
            renderInstrumentMenu();
            break;
        case MENU_CURVE:
            renderCurveMenu();
            break;
        default:
            renderMainMenu();
            break;
    }
}

void MenuManager::handleInput(InputEvent ev) {
    if (!_initialized) return;
    const MenuHandlers& h = menuHandlersTable[_currentMenu];
    switch (ev) {
        case EncoderCW:
            if (h.onCW) (this->*(h.onCW))();
            break;
        case EncoderCCW:
            if (h.onCCW) (this->*(h.onCCW))();
            break;
        case EncoderBtn:
            if (h.onBtn) (this->*(h.onBtn))();
            break;
        case AuxBtn:
            if (h.onAux) (this->*(h.onAux))();
            break;
    }
}

// Shared menu functions
void MenuManager::drawMenuTitle(const char* title) {
    if (!_initialized || !_tft || !title) return;
    _tft->setTextSize(1);
    // Use cyan text on black background
    _tft->setTextColor(COLOR_CYAN, COLOR_BLACK);
    int len = strlen(title);
    int charWidth = 6 * 1;
    int textW = charWidth * len;
    int16_t x = _tft->width() - textW - 4;
    int16_t y = 2;
    if (x < 0) x = 0;
    _tft->setCursor(x, y);
    _tft->print(title);
    // restore text color without background to avoid affecting later prints
    _tft->setTextColor(COLOR_WHITE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////Specific Menu Stuff/////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

// --- Main menu functions (handlers currently minimal/no-op) ---
void MenuManager::renderMainMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    _tft->setRotation(0);
    // Draw the menu title in the top-right
    drawMenuTitle("MAIN");

    // Menu options
    const char* options[] = {
        "Monitor Value",
        "MIDI Channel",
        "MIDI CC Number",
        "Calibration",
        "Invert",
        "Curve",
        "Instrument Definitions"
    };
    const int optionCount = sizeof(options) / sizeof(options[0]);

    _tft->setTextColor(COLOR_WHITE);
    _tft->setTextSize(2);

    int16_t x = 8;
    int16_t y = TOP_MENU_MARGIN; // start a bit below top
    int16_t lineH = 8 * 2 + 6; // approx line height (font height * size + padding)
    int16_t w = _tft->width();
    for (int i = 0; i < optionCount; ++i) {
        int16_t oy = y + i * lineH;
        if (i == _mainSelectedIdx) {
            // highlight full-width background for this line
            int16_t rectW = w - x - 8;
            _tft->fillRect(x - 4, oy - 2, rectW, lineH - 2, COLOR_WHITE);
            _tft->setTextColor(COLOR_BLACK);
        } else {
            _tft->setTextColor(COLOR_WHITE);
        }
        _tft->setCursor(x, oy);
        _tft->print(options[i]);
    }
}


///// input handlers

void MenuManager::onMain_CW() {
    // move selection down
    const int count = 7; // number of main options
    _mainSelectedIdx = (_mainSelectedIdx + 1) % count;
    renderMainMenu();
}
void MenuManager::onMain_CCW() {
    const int count = 7;
    _mainSelectedIdx = (_mainSelectedIdx - 1 + count) % count;
    renderMainMenu();
}
void MenuManager::onMain_Btn() {
    // Switch to the selected submenu
    switch (_mainSelectedIdx) {
        case 0: _currentMenu = MENU_MONITOR; break;
        case 1: _currentMenu = MENU_MIDI_CHANNEL; break;
        case 2: _currentMenu = MENU_MIDI_CC_NUMBER; break;
        case 3: _currentMenu = MENU_CALIBRATION; break;
        case 4: _currentMenu = MENU_INVERT; break;
        case 5: _currentMenu = MENU_CURVE; _stagedCurve = _currentCurve; break;
        case 6: _currentMenu = MENU_INSTRUMENTS; break;
        default: _currentMenu = MENU_MAIN; break;
    }
    // Render the selected menu (each submenu will show its title)
    render();
}
void MenuManager::onMain_Aux() {
    // Jump directly to Monitor menu and render it
    _currentMenu = MENU_MONITOR;
    render();

}
    
// --- Submenu renders & handlers (minimal: just a title)



//////////// Monitor Menu
void MenuManager::renderMonitorMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("MONITOR");
    // Pre-draw the static triangle/arrow at the planned position so updateMonitor
    // doesn't need to redraw it on every value change.
    const int valueTextSize = 6;
    const int rawHeight = 8 * valueTextSize;
    int16_t w = _tft->width();
    int16_t h = _tft->height();
    int16_t rawY = h / 4 - rawHeight / 2; // place input higher up
    // place output symmetrically: same distance from bottom as input is from top
    int16_t normY = h - rawHeight - rawY;
    int16_t arrowCX = w / 2;
    int16_t arrowSize = 8;
    int16_t arrowCenterY = (rawY + rawHeight + normY) / 2;
    int16_t arrowTopY = arrowCenterY - arrowSize;
    // draw triangle once
    int16_t ax0 = arrowCX;
    int16_t ay0 = arrowTopY + arrowSize;
    int16_t ax1 = arrowCX - arrowSize;
    int16_t ay1 = arrowTopY;
    int16_t ax2 = arrowCX + arrowSize;
    int16_t ay2 = arrowTopY;
    _tft->fillTriangle(ax0, ay0, ax1, ay1, ax2, ay2, COLOR_WHITE);
    _monitorTriangleDrawn = true;
    _monitorArrowTopY = arrowTopY;
    _monitorArrowCX = arrowCX;
    // Force an initial draw of the numbers using any cached values. We
    // temporarily clear the cached values so updateMonitor treats the
    // cached readings as changes and draws them immediately.
    int cachedRaw = _lastRaw;
    int cachedNorm = _lastNorm;
    _lastRaw = INT_MIN;
    _lastNorm = INT_MIN;
    updateMonitor(cachedRaw, cachedNorm);
}

void MenuManager::updateMonitor(int rawADC, int norm) {
    if (!_initialized || !_tft) return;
    if (_currentMenu != MENU_MONITOR) {
        // cache values but don't draw
        _lastRaw = rawADC;
        _lastNorm = norm;
        return;
    }

    // Determine which parts need redrawing separately so MIDI jitter doesn't
    // force redrawing of the large MIDI number/progress bar every poll.
    bool redrawRaw = (rawADC != _lastRaw);
    bool redrawMidi = (norm != _lastNorm);
    if (!redrawRaw && !redrawMidi) return;
    // update cached values
    _lastRaw = rawADC;
    _lastNorm = norm;

    int16_t w = _tft->width();
    int16_t h = _tft->height();

    // Layout: Big raw (4 digits) centered, down arrow, mapped (norm) centered
    // Big raw (input) - use yellow
    char rawbuf[16];
    snprintf(rawbuf, sizeof(rawbuf), "%4d", rawADC); // fixed width 4
    const int valueTextSize = 6; // larger for 3.5" display
    _tft->setTextSize(valueTextSize);
    _tft->setTextColor(COLOR_YELLOW, COLOR_BLACK);
    const int rawCharW = 6 * valueTextSize;
    const int rawDigits = 4;
    const int rawTextW = rawCharW * rawDigits; // fixed area
    const int rawHeight = 8 * valueTextSize; // approximate pixel height for font
    // Use same layout as renderMonitorMenu so the pre-drawn triangle stays centered
    int16_t rawX = (w - rawTextW) / 2;
    int16_t rawY = h / 4 - rawHeight / 2; // place input higher up
    // place output symmetrically: same distance from bottom as input is from top
    int16_t normY = h - rawHeight - rawY; // place output lower
    if (rawX < 0) rawX = 0;
    if (rawY < 0) rawY = 0;
    if (redrawRaw) {
        // Clear area (fixed width) to avoid leftover digits (do not touch triangle area)
        _tft->fillRect(rawX - 6, rawY - 6, rawTextW + 12, rawHeight + 4, COLOR_BLACK);
        _tft->setCursor(rawX, rawY);
        _tft->print(rawbuf);
    }

    // The second parameter is now the already-mapped value (0..1023)
    int mapped = norm;

    // Mapped (curved) value below arrow, same font size, use magenta
    if (redrawMidi) {
        char normbuf[16];
        snprintf(normbuf, sizeof(normbuf), "%4d", mapped);
        _tft->setTextSize(valueTextSize);
        _tft->setTextColor(COLOR_MAGENTA, COLOR_BLACK);
        const int normCharW = 6 * valueTextSize;
        const int normDigits = 4;
        const int normTextW = normCharW * normDigits;
        const int normHeight = rawHeight;
        int16_t normX = (w - normTextW) / 2;
        // Use the same computed normY as layout above so we don't need to recompute arrow
        // (arrow was pre-drawn in renderMonitorMenu)
        if (normX < 0) normX = 0;
        // Clear area for normalized display
        _tft->fillRect(normX - 6, normY - 6, normTextW + 12, normHeight + 16, COLOR_BLACK);
        _tft->setCursor(normX, normY);
        _tft->print(normbuf);

        // Draw a horizontal progress bar below the mapped value (0..127 MIDI)
        int barW = w - 40;
        int barX = 20;
        // place bar near bottom of screen
        int barH = 16;
        int barY = h - barH - 12;
        // background border
        _tft->drawRect(barX, barY, barW, barH, COLOR_WHITE);
        // clear inside first (ensures decreases erase previous fill)
        _tft->fillRect(barX + 1, barY + 1, barW - 2, barH - 2, COLOR_BLACK);
        // fill to new width (scale mapped 0..127)
        int fillW = 0;
        if (mapped > 0) fillW = (int)((long)mapped * (long)barW / 127);
        if (fillW > 0) {
            _tft->fillRect(barX + 1, barY + 1, max(0, fillW - 2), barH - 2, COLOR_CYAN);
        }
    }

    // restore default text size
    _tft->setTextSize(1);
}
void MenuManager::renderMidiChannelMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("MIDI CH");

    int16_t w = _tft->width();
    int16_t h = _tft->height();
    // show large number 1..16 centered
    int displayNum = (int)_activeChannel; // user-facing 1..16 (stored 1..16)
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", displayNum);
    const int valueTextSize = 8; // bigger number per request
    _tft->setTextSize(valueTextSize);
    _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
    int charW = 6 * valueTextSize;
    int textW = (int)strlen(buf) * charW;
    int textH = 8 * valueTextSize;
    int16_t cx = (w - textW) / 2;
    int16_t cy = (h - textH) / 2;
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    _tft->setCursor(cx, cy);
    _tft->print(buf);

    // small saved badge like CC menu
    if (_channelSaved) {
        const char* tag = "saved";
        _tft->setTextSize(1);
        int tagW = (int)strlen(tag) * 6;
        int badgeW = tagW + 12;
        int badgeH = 12;
        int16_t bx = (w - badgeW) / 2;
        // position badge slightly below center for larger numbers
        int16_t by = cy + textH + 6;
        if (bx < 0) bx = 0;
        if (by < 0) by = 0;
        _tft->fillRect(bx, by, badgeW, badgeH, COLOR_BLACK);
        _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
        _tft->setCursor(bx + (badgeW - tagW) / 2, by + 2);
        _tft->print(tag);
        _tft->setTextSize(valueTextSize);
        _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
    }
}
void MenuManager::renderMidiCCMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("MIDI CC");
    
        int16_t w = _tft->width();
        int16_t h = _tft->height();
    
        // Left: active instrument name at same height/font as title
        const char* instName = "";
        if (_activeInstrumentIdx >= 0) {
            const InstrumentCCs* inst = &allInstruments[_activeInstrumentIdx];
            if (inst && inst->name) instName = inst->name;
        }
        _tft->setTextSize(1);
        _tft->setTextColor(COLOR_MAGENTA, COLOR_BLACK);
        _tft->setCursor(8, 2);
        _tft->print(instName);
        _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
    
        // Center: large CC number (0..127) using selected value
        uint8_t ccVal = _selectedCC;
        if (ccVal > 127) ccVal = 127;
        char ccbuf[8];
        snprintf(ccbuf, sizeof(ccbuf), "%u", (unsigned)ccVal);
        const int valueTextSize = 6; // large display
        _tft->setTextSize(valueTextSize);
        _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
        int charW = 6 * valueTextSize;
        int textW = (int)strlen(ccbuf) * charW;
        int textH = 8 * valueTextSize;
        int16_t cx = (w - textW) / 2;
        int16_t cy = (h - textH) / 2 - 10; // leave room for label below
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        _tft->setCursor(cx, cy);
        _tft->print(ccbuf);

        // If recently saved, draw a small badge over the number that says "saved"
        if (_ccSaved) {
            const char* tag = "saved";
            _tft->setTextSize(1);
            int tagW = (int)strlen(tag) * 6;
            int badgeW = tagW + 12;
            int badgeH = 12;
            int16_t bx = (w - badgeW) / 2;
            int16_t by = cy + (textH / 2) - (badgeH / 2);
            if (bx < 0) bx = 0;
            if (by < 0) by = 0;
            // draw black rectangle (text will be white)
            _tft->fillRect(bx, by, badgeW, badgeH, COLOR_BLACK);
            _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
            _tft->setCursor(bx + (badgeW - tagW) / 2, by + 2);
            _tft->print(tag);
            _tft->setTextSize(valueTextSize);
            _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
        }
    
        // If the selected CC is the same as the active CC, show [active] above the number
        if (_selectedCC == _activeCC) {
            _tft->setTextSize(1);
            _tft->setTextColor(COLOR_YELLOW, COLOR_BLACK);
            int16_t ax = cx;
            int16_t ay = cy - 14;
            if (ay < 0) ay = 0;
            _tft->setCursor(ax, ay);
            _tft->print("[active]");
            _tft->setTextSize(valueTextSize);
            _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
        }

        // Under the number: optional CC label from the active instrument mapping (for selected CC)
        const char* label = resolveCCLabel(_selectedCC);
        if (label) {
            Serial.print("Trying to print label: ");
            Serial.println(label);
            _tft->setTextSize(2);
            _tft->setTextColor(COLOR_CYAN, COLOR_BLACK);
            int labW = strlen(label) * 6 * 2;
            int16_t lx = (w - labW) / 2;
            int16_t ly = cy + textH + 6;
            if (lx < 0) lx = 0;
            if (ly + 8 > h) ly = h - 8;
            _tft->setCursor(lx, ly);
            _tft->print(label);
            _tft->setTextSize(1);
            _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
        }
}
void MenuManager::renderCalibrationMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("CALIBRATION");

    int16_t w = _tft->width();
    int16_t h = _tft->height();
    // Instruction text in center
    const char* instr = _calibSettingMax ? "press encoder to set max" : "press encoder to set min";
    _tft->setTextSize(3);
    _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
    int instrW = (int)strlen(instr) * 6 * 3;
    int instrH = 8 * 3;
    int16_t ix = (w - instrW) / 2;
    int16_t iy = (h - instrH) / 2;
    if (ix < 0) ix = 0;
    if (iy < 0) iy = 0;
    _tft->setCursor(ix, iy);
    _tft->print(instr);

    // If a value was just saved, draw a black rectangle directly over the instruction
    if (_calibSaved) {
        // TODO: make this time-limited (auto-clear) rather than persistent
        const char* msg = _calibSavedIsMax ? "Max Value Saved" : "Min Value Saved";
        _tft->setTextSize(2);
        int msgW = (int)strlen(msg) * 6 * 2;
        int msgH = 8 * 2 + 4;
        int16_t mx = (w - msgW) / 2;
        int16_t my = iy + (instrH - msgH) / 2;
        if (mx < 0) mx = 0;
        if (my < 0) my = 0;
        // draw black rect over instruction (same color as background) then draw message in white
        _tft->fillRect(mx - 4, my - 2, msgW + 8, msgH + 4, COLOR_BLACK);
        _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
        _tft->setCursor(mx, my + 2);
        _tft->print(msg);
        _tft->setTextSize(1);
    }
}
void MenuManager::renderInstrumentMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("INSTRUMENTS");

    // Count instruments using sentinel
    int total = 0;
    for (const InstrumentCCs* p = allInstruments; p && p->name; ++p) ++total;

    int16_t w = _tft->width();
    int16_t h = _tft->height();
    const int textSize = 2;
    const int lineH = 8 * textSize + 6;
    int visible = (h - TOP_MENU_MARGIN) / lineH;
    if (visible < 1) visible = 1;

    // clamp top index
    if (_instrumentTopIdx < 0) _instrumentTopIdx = 0;
    if (_instrumentTopIdx > max(0, total - visible)) _instrumentTopIdx = max(0, total - visible);

    int16_t x = 8;
    int16_t y = TOP_MENU_MARGIN;
    _tft->setTextSize(textSize);

    // Render visible window
    int row = 0;
    for (int i = _instrumentTopIdx; i < total && row < visible; ++i, ++row) {
        const InstrumentCCs* p = &allInstruments[i];
        const char* name = p->name ? p->name : "";
        int16_t oy = y + row * lineH;

        bool isSelected = (i == _instrumentSelectedIdx);
        if (isSelected) {
            int16_t rectW = w - x - 8;
            _tft->fillRect(x - 4, oy - 2, rectW, lineH - 2, COLOR_WHITE);
            // If the selected row is the active instrument, show green text on white
            if (i == _activeInstrumentIdx) {
                _tft->setTextColor(COLOR_GREEN, COLOR_WHITE);
            } else {
                _tft->setTextColor(COLOR_BLACK);
            }
        } else {
            // non-selected rows: active instrument indicated in magenta, others white
            if (i == _activeInstrumentIdx) _tft->setTextColor(COLOR_MAGENTA, COLOR_BLACK);
            else _tft->setTextColor(COLOR_WHITE, COLOR_BLACK);
        }

        // checkbox
        _tft->setCursor(x, oy);
        _tft->setCursor(x + 20, oy);
        _tft->print(name);
    }
}
void MenuManager::renderInvertMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("INVERT");

    const char* options[] = { "Normal", "Inverted" };
    const int optionCount = sizeof(options) / sizeof(options[0]);

    _tft->setTextColor(COLOR_WHITE);
    _tft->setTextSize(2);

    int16_t x = 8;
    int16_t y = TOP_MENU_MARGIN;
    int16_t lineH = 8 * 2 + 6;
    int16_t w = _tft->width();
    for (int i = 0; i < optionCount; ++i) {
        int16_t oy = y + i * lineH;
        if (i == _invertSelectedIdx) {
            int16_t rectW = w - x - 8;
            _tft->fillRect(x - 4, oy - 2, rectW, lineH - 2, COLOR_WHITE);
            _tft->setTextColor(COLOR_BLACK);
        } else {
            _tft->setTextColor(COLOR_WHITE);
        }
        _tft->setCursor(x, oy);
        _tft->print(options[i]);
        // append [active] if this option matches the active persisted state
        if ((i == 0 && !_inverted) || (i == 1 && _inverted)) {
            // place tag to the right of the text
            int16_t tagX = x + 140; // rough offset to avoid measuring widths
            _tft->setTextSize(1);
            _tft->setTextColor(COLOR_CYAN, COLOR_BLACK);
            _tft->setCursor(tagX, oy + 6);
            _tft->print("[active]");
            _tft->setTextSize(2);
            _tft->setTextColor(COLOR_WHITE);
        }
    }
}
void MenuManager::renderCurveMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("CURVE");
    // draw preview of currently selected (staged) curve and show which is active
    drawCurvePreview(_tft, _stagedCurve, _currentCurve);
}

//////////// Curve Menu

// Helper: draw a preview of the given curve type into the screen center area
static void drawCurvePreview(Adafruit_ST7796S* tft, CurveType ct, CurveType active) {
    if (!tft) return;
    int16_t w = tft->width();
    int16_t h = tft->height();
    int16_t margin = 20;
    int16_t gx = margin;
    int16_t gy = margin + 20;
    int16_t gw = w - margin * 2;
    int16_t gh = h - (margin * 2) - 40;
    // draw axes
    // tft->drawRect(gx, gy, gw, gh, COLOR_WHITE);
    // sample curve across width
    const int samples = gw;
    int lastX = gx;
    int lastY = gy + gh - 1;
    for (int i = 0; i < samples; ++i) {
        float x = (float)i / (float)(samples - 1);
        float y = 0.0f;
        switch (ct) {
            case CURVE_LINEAR:
                y = x;
                break;
            case CURVE_LOGARITHMIC:
                y = log10f(1.0f + 9.0f * x); // maps 0..1 -> 0..1 (approx)
                break;
            case CURVE_EXPONENTIAL:
                y = (expf(3.0f * x) - 1.0f) / (expf(3.0f) - 1.0f);
                break;
            case CURVE_SIGMOIDAL: {
                float k = 12.0f; // steepness
                y = 1.0f / (1.0f + expf(-k * (x - 0.5f)));
                break;
            }
            case CURVE_TANGENT: {
                // Use tangent mapping normalized to 0..1 across the sampling range.
                float t = tanf(3.0f * (x - 0.5f));
                // Normalize using expected min/max at endpoints (-1.5, +1.5 multiplier)
                float tmin = tanf(-1.5f);
                float tmax = tanf(1.5f);
                if (tmax - tmin == 0.0f) y = 0.5f; else y = (t - tmin) / (tmax - tmin);
                // Clamp to [0,1]
                if (y < 0.0f) y = 0.0f;
                if (y > 1.0f) y = 1.0f;
                break;
            }
            default:
                y = x;
        }
        int px = gx + i;
        int py = gy + gh - 1 - (int)(y * (gh - 2));
        // draw thicker line by drawing multiple offset lines and a small filled circle
        int thickness = 2; // increased thickness for bolder curve
        for (int t = -thickness; t <= thickness; ++t) {
            tft->drawLine(lastX, lastY + t, px, py + t, COLOR_MAGENTA);
        }
        // smooth end cap
        tft->fillCircle(px, py, thickness, COLOR_MAGENTA);
        lastX = px;
        lastY = py;
    }
    // label current curve name in top-left of the graph
    const char* name = "";
    switch (ct) {
        case CURVE_LINEAR: name = "Linear"; break;
        case CURVE_LOGARITHMIC: name = "Logarithmic"; break;
        case CURVE_EXPONENTIAL: name = "Exponential"; break;
        case CURVE_SIGMOIDAL: name = "Sigmoidal"; break;
        case CURVE_TANGENT: name = "Tangent"; break;
        default: name = "Unknown"; break;
    }
    tft->setTextSize(2);
    tft->setTextColor(COLOR_YELLOW, COLOR_BLACK);
    tft->setCursor(gx + 6, gy + 6);
    tft->print(name);
    // If this previewed curve is the currently active one, mark it
    if (ct == active) {
        tft->setTextSize(1);
        tft->setTextColor(COLOR_CYAN, COLOR_BLACK);
        // place below the name (approx font height for size 2 is 16px)
        int16_t labelY = gy + 6 + 18;
        tft->setCursor(gx + 6, labelY);
        tft->print("[active]");
    }
    tft->setTextSize(1);
}

// Submenu auxiliary/button handlers: return to main menu
//////////// Monitor Menu Controls
void MenuManager::onMonitor_CW() {}
void MenuManager::onMonitor_CCW() {}
void MenuManager::onMonitor_Btn() {}
void MenuManager::onMonitor_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }

//////////// MIDI Channel Menu Controls

void MenuManager::onMidiChannel_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }
void MenuManager::onMidiChannel_CW() {
    // advance channel 1..16
    _activeChannel = (int8_t)(_activeChannel + 1);
    if (_activeChannel > 16) _activeChannel = 1;
    _channelSaved = false;
    renderMidiChannelMenu();
    Serial.print("MIDI channel -> "); Serial.println((int)_activeChannel + 1);
}
void MenuManager::onMidiChannel_CCW() {
    int c = (int)_activeChannel - 1;
    if (c < 1) c = 16;
    _activeChannel = (int8_t)c;
    _channelSaved = false;
    renderMidiChannelMenu();
    Serial.print("MIDI channel -> "); Serial.println((int)_activeChannel);
}
void MenuManager::onMidiChannel_Btn() {
    // persist channel to EEPROM and show saved badge
    eeprom_saveChannel(_activeChannel);
    _channelSaved = true;
    renderMidiChannelMenu();
}


//////////// MIDI CC Menu Controls
void MenuManager::onMidiCC_CW() {
    // advance selected CC (0..127); do not persist until button pressed
    _selectedCC = (uint8_t)((_selectedCC + 1) % 128);
    _ccSaved = false;
    renderMidiCCMenu();
    // Debug: print selected CC and resolved label
    const char* dbgLabel = resolveCCLabel(_selectedCC);
    Serial.print("CC turn CW -> "); Serial.print(_selectedCC); Serial.print(" : "); Serial.println(dbgLabel ? dbgLabel : "<none>");
}
void MenuManager::onMidiCC_CCW() {
    // decrease selected CC, wrap at 0 -> 127; do NOT persist until button pressed
    int next = (int)_selectedCC - 1;
    if (next < 0) next += 128;
    _selectedCC = (uint8_t)next;
    _ccSaved = false;
    renderMidiCCMenu();
    // Debug: print selected CC and resolved label
    const char* dbgLabel2 = resolveCCLabel(_selectedCC);
    Serial.print("CC turn CCW -> "); Serial.print(_selectedCC); Serial.print(" : "); Serial.println(dbgLabel2 ? dbgLabel2 : "<none>");
}
void MenuManager::onMidiCC_Btn() {
    // Commit/save current selected CC to EEPROM and show saved badge
    _activeCC = _selectedCC;
    eeprom_saveCC(_activeCC);
    _ccSaved = true;
    renderMidiCCMenu();
    //todo: this is dumb. We don't need to redraw the whole menu, we're just adding the label
    //on top of what's already there.
}
void MenuManager::onMidiCC_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }

uint8_t MenuManager::getActiveCC() {
    return _activeCC;
}

int8_t MenuManager::getMidiChannel() {
    return _activeChannel;
}

const char* MenuManager::resolveCCLabel(uint8_t cc) {
    if (_activeInstrumentIdx < 0) return nullptr;
    const InstrumentCCs* inst = &allInstruments[_activeInstrumentIdx];
    if (!inst) return nullptr;
    const CCLabel* arr = inst->ccArray;
    if (!arr) return nullptr;
    bool seenAny = false;
    for (const CCLabel* p = arr; p; ++p) {
        if (p->cc == 0 && p->label == nullptr) {
            if (seenAny) break; // second sentinel -> end
            seenAny = true;
            continue;
        }
        seenAny = true;
        if (p->cc == cc) return p->label;
    }
    return nullptr;
}


int MenuManager::getPedalMin() { return _pedalMin; }
int MenuManager::getPedalMax() { return _pedalMax; }
void MenuManager::setPedalMin(int v) { _pedalMin = v; }
void MenuManager::setPedalMax(int v) { _pedalMax = v; }



//////////// Calibration Menu Controls
void MenuManager::onCalibration_CW() {
    // Toggle between setting min and max
    _calibSettingMax = !_calibSettingMax;
    // clear any previous saved badge when toggling
    _calibSaved = false;
    renderCalibrationMenu();
}
void MenuManager::onCalibration_CCW() {
    // Same behavior as CW: toggle between min and max
    _calibSettingMax = !_calibSettingMax;
    _calibSaved = false;
    renderCalibrationMenu();
}
void MenuManager::onCalibration_Btn() {
    // Read raw analog value from pedal and set as min or max depending on stage
    int raw = analogRead(PEDAL_PIN);
    if (raw < 0) raw = 0;
    if (raw > 4095) raw = 4095;
    if (_calibSettingMax) {
        setPedalMax(raw);
        _calibSavedIsMax = true;
    } else {
        setPedalMin(raw);
        _calibSavedIsMax = false;
    }
    _calibSaved = true;
    eeprom_saveCalibration(getPedalMin(), getPedalMax());
    renderCalibrationMenu();
}
void MenuManager::onCalibration_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }


//////////// Curve Menu Controls
//////////// Invert Menu Controls
void MenuManager::onInvert_CW() {
    const int count = 2;
    _invertSelectedIdx = (_invertSelectedIdx + 1) % count;
    renderInvertMenu();
}
void MenuManager::onInvert_CCW() {
    const int count = 2;
    _invertSelectedIdx = (_invertSelectedIdx - 1 + count) % count;
    renderInvertMenu();
}
void MenuManager::onInvert_Btn() {
    // commit staged selection
    _inverted = (_invertSelectedIdx == 1);
    eeprom_saveInvert((uint8_t)(_inverted ? 1 : 0));
    _currentMenu = MENU_MAIN;
    renderMainMenu();
}
void MenuManager::onInvert_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }

//////////// Curve Menu Controls
void MenuManager::onCurve_CW() {
    // advance staged selection (preview only)
    int c = (int)_stagedCurve;
    c = (c + 1) % CURVE_COUNT;
    _stagedCurve = (CurveType)c;
    renderCurveMenu();
}
void MenuManager::onCurve_CCW() {
    int c = (int)_stagedCurve;
    c = (c - 1 + CURVE_COUNT) % CURVE_COUNT;
    _stagedCurve = (CurveType)c;
    renderCurveMenu();
}

int MenuManager::mapCurve(int linear) {
    // linear: 0..1023
    float x = 0.0f;
    if (linear <= 0) x = 0.0f; else x = (float)linear / 1023.0f;
    float y = x;
    switch (_currentCurve) {
        case CURVE_LINEAR:
            y = x;
            break;
        case CURVE_LOGARITHMIC: {
            float base = CURVE_LOG_BASE; // >1
            // y = log(1 + (base-1)*x) / log(base)
            float denom = logf(base);
            if (denom == 0.0f) y = x; else y = logf(1.0f + (base - 1.0f) * x) / denom;
            break;
        }
        case CURVE_EXPONENTIAL: {
            float k = CURVE_EXP_K;
            float num = expf(k * x) - 1.0f;
            float den = expf(k) - 1.0f;
            if (den == 0.0f) y = x; else y = num / den;
            break;
        }
        case CURVE_SIGMOIDAL: {
            float s = CURVE_SIGMOID_STEEPNESS;
            y = 1.0f / (1.0f + expf(-s * (x - 0.5f)));
            break;
        }
        case CURVE_TANGENT: {
            float f = CURVE_TAN_SCALE; // scale
            float t = tanf(f * (x - 0.5f));
            float tmin = tanf(-f * 0.5f);
            float tmax = tanf(f * 0.5f);
            if (tmax - tmin == 0.0f) y = x; else y = (t - tmin) / (tmax - tmin);
            break;
        }
        default:
            y = x;
    }
    if (y < 0.0f) y = 0.0f;
    if (y > 1.0f) y = 1.0f;
    int mapped = (int)roundf(y * 1023.0f);
    return mapped;
}

int MenuManager::applyCurve(int linear) {
    int m = mapCurve(linear);
    if (_inverted) {
        // invert within 0..1023
        m = 1023 - m;
    }
    return m;
}
void MenuManager::onCurve_Btn() {
    // Commit staged selection as active, persist it, and return to main menu
    _currentCurve = _stagedCurve;
    eeprom_saveCurve((uint8_t)_currentCurve);
    _currentMenu = MENU_MAIN;
    renderMainMenu();
}
void MenuManager::onCurve_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }

//////////// Instrument Menu Definitions Menu Controls
void MenuManager::onInstrument_CW() {
    // move selection down, update scroll window
    // compute total
    int total = 0;
    for (const InstrumentCCs* p = allInstruments; p && p->name; ++p) ++total;
    if (total <= 0) return;
    _instrumentSelectedIdx = (_instrumentSelectedIdx + 1) % total;
    int16_t h = _tft->height();
    const int textSize = 2;
    const int lineH = 8 * textSize + 6;
    int visible = (h - TOP_MENU_MARGIN) / lineH;
    if (_instrumentSelectedIdx >= _instrumentTopIdx + visible) {
        _instrumentTopIdx = _instrumentSelectedIdx - visible + 1;
    }
    renderInstrumentMenu();
}
void MenuManager::onInstrument_CCW() {
    int total = 0;
    for (const InstrumentCCs* p = allInstruments; p && p->name; ++p) ++total;
    if (total <= 0) return;
    _instrumentSelectedIdx = (_instrumentSelectedIdx - 1 + total) % total;
    int16_t h = _tft->height();
    const int textSize = 2;
    const int lineH = 8 * textSize + 6;
    int visible = (h - TOP_MENU_MARGIN) / lineH;
    if (_instrumentSelectedIdx < _instrumentTopIdx) {
        _instrumentTopIdx = _instrumentSelectedIdx;
    }
    renderInstrumentMenu();
}
void MenuManager::onInstrument_Btn() {
    // commit active instrument
    _activeInstrumentIdx = _instrumentSelectedIdx;
    eeprom_saveActiveInstrument((uint8_t)_activeInstrumentIdx);
    renderInstrumentMenu();
}
void MenuManager::onInstrument_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }




// Define the handler table for available menus
const MenuHandlers menuHandlersTable[MENU_COUNT] = {
    // MENU_MAIN
    { &MenuManager::onMain_CW, &MenuManager::onMain_CCW, &MenuManager::onMain_Btn, &MenuManager::onMain_Aux },
    // MENU_MONITOR
    { &MenuManager::onMonitor_CW, &MenuManager::onMonitor_CCW, &MenuManager::onMonitor_Btn, &MenuManager::onMonitor_Aux },
    // MENU_MIDI_CHANNEL
    { &MenuManager::onMidiChannel_CW, &MenuManager::onMidiChannel_CCW, &MenuManager::onMidiChannel_Btn, &MenuManager::onMidiChannel_Aux },
    // MENU_MIDI_CC_NUMBER
    { &MenuManager::onMidiCC_CW, &MenuManager::onMidiCC_CCW, &MenuManager::onMidiCC_Btn, &MenuManager::onMidiCC_Aux },
    // MENU_CALIBRATION
    { &MenuManager::onCalibration_CW, &MenuManager::onCalibration_CCW, &MenuManager::onCalibration_Btn, &MenuManager::onCalibration_Aux },
    // MENU_INVERT
    { &MenuManager::onInvert_CW, &MenuManager::onInvert_CCW, &MenuManager::onInvert_Btn, &MenuManager::onInvert_Aux },
    // MENU_CURVE
    { &MenuManager::onCurve_CW, &MenuManager::onCurve_CCW, &MenuManager::onCurve_Btn, &MenuManager::onCurve_Aux }
    ,
    // MENU_INSTRUMENTS
    { &MenuManager::onInstrument_CW, &MenuManager::onInstrument_CCW, &MenuManager::onInstrument_Btn, &MenuManager::onInstrument_Aux }
};


