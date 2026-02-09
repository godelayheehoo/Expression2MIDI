#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <stdint.h>
// Forward declaration to avoid including display headers in the public API
class Adafruit_ST7796S;

// Menu identifiers
enum MenuState : uint8_t {
    MENU_MAIN = 0,
    MENU_MONITOR,
    MENU_MIDI_CHANNEL,
    MENU_MIDI_CC_NUMBER,
    MENU_CALIBRATION,
    MENU_INVERT,
    MENU_CURVE,
    MENU_INSTRUMENTS,
    MENU_COUNT
};

// Curve types available for mapping
enum CurveType : uint8_t {
    CURVE_LINEAR = 0,
    CURVE_LOGARITHMIC,
    CURVE_EXPONENTIAL,
    CURVE_SIGMOIDAL,
    CURVE_TANGENT,
    CURVE_COUNT
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
    void renderMonitorMenu();
    void renderMidiChannelMenu();
    void renderMidiCCMenu();
    void renderCalibrationMenu();
    void renderInvertMenu();
    void renderInstrumentMenu();
    void renderCurveMenu();
    void render();
    void handleInput(InputEvent ev);
    void drawMenuTitle(const char* title);
    // Monitor update called from main loop when polling
    void updateMonitor(int rawADC, int norm);
    // Public wrapper to map a linear 0..1023 value through the selected curve
    int applyCurve(int linear);
    // Per-menu handlers
    void onMain_CW();
    void onMain_CCW();
    void onMain_Btn();
    void onMain_Aux();
    // Monitor menu handlers
    void onMonitor_CW();
    void onMonitor_CCW();
    void onMonitor_Btn();
    void onMonitor_Aux();
    // MIDI Channel menu handlers
    void onMidiChannel_CW();
    void onMidiChannel_CCW();
    void onMidiChannel_Btn();
    void onMidiChannel_Aux();
    // MIDI CC menu handlers
    void onMidiCC_CW();
    void onMidiCC_CCW();
    void onMidiCC_Btn();
    void onMidiCC_Aux();
    // Calibration menu handlers
    void onCalibration_CW();
    void onCalibration_CCW();
    void onCalibration_Btn();
    void onCalibration_Aux();
    // Curve menu handlers
    void onCurve_CW();
    void onCurve_CCW();
    void onCurve_Btn();
    void onCurve_Aux();
    // Instrument Definitions menu handlers
    void onInstrument_CW();
    void onInstrument_CCW();
    void onInstrument_Btn();
    void onInstrument_Aux();
    // Invert menu handlers
    void onInvert_CW();
    void onInvert_CCW();
    void onInvert_Btn();
    void onInvert_Aux();

    // Read cached MIDI CC (0..127)
    uint8_t getActiveCC();
    // Pedal calibration values accessors
    int getPedalMin();
    int getPedalMax();
    void setPedalMin(int v);
    void setPedalMax(int v);
    
private:
    // Resolve a CC label for the active instrument, or return nullptr
    const char* resolveCCLabel(uint8_t cc);

private:
    // Apply currently selected curve to a linear 0..1023 input
    int mapCurve(int linear);
    Adafruit_ST7796S* _tft;
    bool _initialized;
    MenuState _currentMenu;
    int _mainSelectedIdx;
    // Monitor cached values
    int _lastRaw;
    int _lastNorm;
    // Monitor rendering state (avoid redrawing the arrow/triangle repeatedly)
    bool _monitorTriangleDrawn;
    int _monitorArrowTopY;
    int _monitorArrowCX;
    // Active curve selection (persisted in EEPROM)
    CurveType _currentCurve;
    // Staged curve used while previewing in the Curve menu (committed on Btn)
    CurveType _stagedCurve;
    // Invert state (persisted) and staged selection index for the Invert menu
    bool _inverted;
    int _invertSelectedIdx;
    // Instrument Definitions state
    int _activeInstrumentIdx; // index into allInstruments array, 0 == None
    int _instrumentSelectedIdx; // currently highlighted index in the menu
    int _instrumentTopIdx; // top visible index for scrolling
    // Cached MIDI CC number (0..127) to avoid reading EEPROM frequently
    uint8_t _activeCC;
    // Flag set when CC value was saved by encoder button (controls saved badge)
    bool _ccSaved;
    // Pedal calibration values (raw ADC range)
    int _pedalMin;
    int _pedalMax;
    // Calibration UI state
    bool _calibSettingMax; // false == showing "set min", true == showing "set max"
    bool _calibSaved; // true when a min/max was recently saved (UI badge)
    bool _calibSavedIsMax; // which value was saved when _calibSaved is true
};

// (No standalone getter; use MenuManager::getActiveCC())

// Table-driven handlers struct
struct MenuHandlers {
    void (MenuManager::*onCW)();
    void (MenuManager::*onCCW)();
    void (MenuManager::*onBtn)();
    void (MenuManager::*onAux)();
};


#endif // MENU_MANAGER_H
