#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <stdint.h>
// Forward declaration to avoid including display headers in the public API
class Adafruit_ST7796S;

// Menu identifiers
enum MenuState : uint8_t {
    MENU_MAIN = 0,
    MENU_COUNT
};

// Input events exposed to the menu system
enum InputEvent : uint8_t {
    EncoderCW = 0,
    EncoderCCW,
    EncoderBtn,
    AuxBtn
};


class MenuManager {
public:
    MenuManager();
    void begin(Adafruit_ST7796S* tft);
    void renderMainMenu();
    void render();
    void handleInput(InputEvent ev);
    void drawMenuTitle(const char* title);
    // Per-menu handlers
    void onMain_CW();
    void onMain_CCW();
    void onMain_Btn();
    void onMain_Aux();

private:
    Adafruit_ST7796S* _tft;
    bool _initialized;
    MenuState _currentMenu;
};

// Table-driven handlers struct
struct MenuHandlers {
    void (MenuManager::*onCW)();
    void (MenuManager::*onCCW)();
    void (MenuManager::*onBtn)();
    void (MenuManager::*onAux)();
};


#endif // MENU_MANAGER_H
