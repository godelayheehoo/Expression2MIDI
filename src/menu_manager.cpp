
#include "menu_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <SPI.h>
#include <Arduino.h>
#include <cstring>
#include "color_utils.h"


#define TOP_MENU_MARGIN 50
// menuHandlersTable is defined at bottom of this file
// Forward declaration so code above can reference it
extern const MenuHandlers menuHandlersTable[MENU_COUNT];

MenuManager::MenuManager()
    : _tft(nullptr), _initialized(false), _currentMenu(MENU_MAIN) {
    _mainSelectedIdx = 0;
}

void MenuManager::begin(Adafruit_ST7796S* tft) {
    _tft = tft;
    if (!_tft) return;
    _initialized = true;
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

// --- Submenu renders & handlers (minimal: just a title)
void MenuManager::renderMonitorMenu() {
    if (!_initialized || !_tft) return;
    _tft->fillScreen(COLOR_BLACK);
    drawMenuTitle("MONITOR");
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

void MenuManager::onCurve_CW() {}
void MenuManager::onCurve_CCW() {}
void MenuManager::onCurve_Btn() {}
void MenuManager::onCurve_Aux() { _currentMenu = MENU_MAIN; renderMainMenu(); }
void MenuManager::onMain_Aux() {
    //TODO: Implement quick return to Monitor Value
    Serial.println("Quick return to Monitor Value");

}





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


