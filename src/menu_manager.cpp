
#include "menu_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <SPI.h>
#include <Arduino.h>
#include <cstring>
#include "color_utils.h"

// menuHandlersTable is defined at bottom of this file
// Forward declaration so code above can reference it
extern const MenuHandlers menuHandlersTable[MENU_COUNT];

MenuManager::MenuManager()
    : _tft(nullptr), _initialized(false), _currentMenu(MENU_MAIN) {
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
    int16_t y = 28; // start a bit below top
    int16_t lineH = 8 * 2 + 6; // approx line height (font height * size + padding)
    for (int i = 0; i < optionCount; ++i) {
        _tft->setCursor(x, y + i * lineH);
        _tft->print(options[i]);
    }
}


// This selects the currently highlighted menu.  We move between highlighted items with the encoder,
// we engage this with onMain_Btn()
void mainMenuSelectCurrentMenu() {
    // no-op for now
}
///// input handlers

void MenuManager::onMain_CW() {
    // no-op for now
}
void MenuManager::onMain_CCW() {
    // no-op for now
}
void MenuManager::onMain_Btn() {
    // no-op for now
}
void MenuManager::onMain_Aux() {
    mainMenuSelectCurrentMenu();
}





// Define the handler table for available menus
const MenuHandlers menuHandlersTable[MENU_COUNT] = {
    // MENU_MAIN
    { &MenuManager::onMain_CW, &MenuManager::onMain_CCW, &MenuManager::onMain_Btn, &MenuManager::onMain_Aux }
};


