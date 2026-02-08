
#include "menu_manager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include <SPI.h>
#include <Arduino.h>
#include <cstring>

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

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////Specific Menu Stuff/////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

// --- Main menu functions (handlers currently minimal/no-op) ---
void MenuManager::renderMainMenu() {
    if (!_initialized || !_tft) return;

    _tft->fillScreen(ST77XX_BLACK);
    _tft->setTextColor(ST77XX_WHITE);
    _tft->setTextSize(2);
    _tft->setRotation(0);
    const char* msg = "main menu";
    int len = strlen(msg);
    int charWidth = 6 * 2; // approx
    int textWidth = charWidth * len;
    int16_t x = (_tft->width() - textWidth) / 2;
    int16_t y = (_tft->height() - (8 * 2)) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    _tft->setCursor(x, y);
    _tft->print(msg);
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


