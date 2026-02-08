
#include "menu_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <SPI.h>
#include <Arduino.h>
#include <cstring>
#include "color_utils.h"
#include "eeprom_storage.h"
#include <cmath>
#include "curve_constants.h"

// Forward declaration: curve preview helper used by `renderCurveMenu`
static void drawCurvePreview(Adafruit_ST7796S* tft, CurveType ct);


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
}

void MenuManager::begin(Adafruit_ST7796S* tft) {
    _tft = tft;
    if (!_tft) return;
    _initialized = true;
    // Load persisted curve selection (defaults to linear if not present)
    uint8_t c = eeprom_getCurve();
    if (c >= CURVE_COUNT) c = CURVE_LINEAR;
    _currentCurve = (CurveType)c;
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
        "Curve"
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
    const int count = 5; // number of main options
    _mainSelectedIdx = (_mainSelectedIdx + 1) % count;
    renderMainMenu();
}
void MenuManager::onMain_CCW() {
    const int count = 5;
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
        case 4: _currentMenu = MENU_CURVE; break;
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
    // Leave numbers to updateMonitor when values arrive
}

void MenuManager::updateMonitor(int rawADC, int norm) {
    if (!_initialized || !_tft) return;
    if (_currentMenu != MENU_MONITOR) {
        // cache values but don't draw
        _lastRaw = rawADC;
        _lastNorm = norm;
        return;
    }

    // Only update if value changed
    if (rawADC == _lastRaw && norm == _lastNorm) return;
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
    // Clear area (fixed width) to avoid leftover digits (do not touch triangle area)
    _tft->fillRect(rawX - 6, rawY - 6, rawTextW + 12, rawHeight + 4, COLOR_BLACK);
    _tft->setCursor(rawX, rawY);
    _tft->print(rawbuf);

    // The second parameter is now the already-mapped value (0..1023)
    int mapped = norm;

    // Mapped (curved) value below arrow, same font size, use magenta
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
    // recompute arrowTopY for local use
    int16_t arrowCX = w / 2;
    int16_t arrowSize = 8;
    int16_t arrowCenterY = (rawY + rawHeight + normY) / 2;
    int16_t arrowTopY = arrowCenterY - arrowSize;
    if (normX < 0) normX = 0;
    // Clear area for normalized display
    _tft->fillRect(normX - 6, normY - 6, normTextW + 12, normHeight + 16, COLOR_BLACK);
    _tft->setCursor(normX, normY);
    _tft->print(normbuf);

    // Draw a horizontal progress bar below the mapped value
    int barW = w - 40;
    int barX = 20;
    // place bar near bottom of screen
    int barH = 16;
    int barY = h - barH - 12;
    // background border
    _tft->drawRect(barX, barY, barW, barH, COLOR_WHITE);
    // clear inside first (ensures decreases erase previous fill)
    _tft->fillRect(barX + 1, barY + 1, barW - 2, barH - 2, COLOR_BLACK);
    // fill to new width
    int fillW = (int)((long)mapped * (long)barW / 1023);
    if (fillW > 0) {
        _tft->fillRect(barX + 1, barY + 1, max(0, fillW - 2), barH - 2, COLOR_CYAN);
    }

    // restore default text size
    _tft->setTextSize(1);
}
void MenuManager::renderMidiChannelMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("MIDI CH");
}
void MenuManager::renderMidiCCMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("MIDI CC");
}
void MenuManager::renderCalibrationMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("CALIBRATION");
}
void MenuManager::renderCurveMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("CURVE");
    // draw preview of currently selected curve
    drawCurvePreview(_tft, _currentCurve);
}

// Helper: draw a preview of the given curve type into the screen center area
static void drawCurvePreview(Adafruit_ST7796S* tft, CurveType ct) {
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
    tft->setTextSize(1);
}

// Submenu auxiliary/button handlers: return to main menu
void MenuManager::onMonitor_CW() {}
void MenuManager::onMonitor_CCW() {}
void MenuManager::onMonitor_Btn() {}
void MenuManager::onMonitor_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }

void MenuManager::onMidiChannel_CW() {}
void MenuManager::onMidiChannel_CCW() {}
void MenuManager::onMidiChannel_Btn() {}
void MenuManager::onMidiChannel_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }

void MenuManager::onMidiCC_CW() {}
void MenuManager::onMidiCC_CCW() {}
void MenuManager::onMidiCC_Btn() {}
void MenuManager::onMidiCC_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }

void MenuManager::onCalibration_CW() {}
void MenuManager::onCalibration_CCW() {}
void MenuManager::onCalibration_Btn() {}
void MenuManager::onCalibration_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }


// Curve Menu
void MenuManager::onCurve_CW() {
    // next curve
    int c = (int)_currentCurve;
    c = (c + 1) % CURVE_COUNT;
    _currentCurve = (CurveType)c;
    eeprom_saveCurve((uint8_t)_currentCurve);
    renderCurveMenu();
}
void MenuManager::onCurve_CCW() {
    int c = (int)_currentCurve;
    c = (c - 1 + CURVE_COUNT) % CURVE_COUNT;
    _currentCurve = (CurveType)c;
    eeprom_saveCurve((uint8_t)_currentCurve);
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
    return mapCurve(linear);
}
void MenuManager::onCurve_Btn() {}
void MenuManager::onCurve_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }



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
    // MENU_CURVE
    { &MenuManager::onCurve_CW, &MenuManager::onCurve_CCW, &MenuManager::onCurve_Btn, &MenuManager::onCurve_Aux }
};


