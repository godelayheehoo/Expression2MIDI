#include "eeprom_storage.h"
#include <Preferences.h>
#include <Arduino.h>

static Preferences prefs;
static const char* PREF_NS = "expr2midi";
static const uint32_t EEPROM_MAGIC = 0x45524D32; // 'ERM2'
static const uint8_t DEFAULT_CC = 74;
static const int8_t DEFAULT_CHANNEL = 14;
static const uint8_t DEFAULT_CURVE = 0; // 0 == linear
static const uint8_t DEFAULT_INVERT = 0; // 0 == normal

bool eeprom_init() {
    prefs.begin(PREF_NS, false);
    uint32_t magic = prefs.getUInt("magic", 0);
    if (magic != EEPROM_MAGIC) {
        // Write defaults and magic
        prefs.putUInt("magic", EEPROM_MAGIC);
        prefs.putUInt("cc", (uint32_t)DEFAULT_CC);
        prefs.putUInt("chan", (uint32_t)DEFAULT_CHANNEL);
        prefs.putUChar("curve", (uint8_t)DEFAULT_CURVE);
        prefs.end();
        return false; // defaults written
    }
    prefs.end();
    return true; // existing settings present
}

uint8_t eeprom_getCC() {
    prefs.begin(PREF_NS, true);
    uint32_t v = prefs.getUInt("cc", (uint32_t)DEFAULT_CC);
    prefs.end();
    return (uint8_t)v;
}

uint8_t eeprom_getCurve() {
    prefs.begin(PREF_NS, true);
    uint8_t v = prefs.getUChar("curve", DEFAULT_CURVE);
    prefs.end();
    return v;
}

int8_t eeprom_getChannel() {
    prefs.begin(PREF_NS, true);
    uint32_t v = prefs.getUInt("chan", (uint32_t)DEFAULT_CHANNEL);
    prefs.end();
    return (int8_t)v;
}

void eeprom_save(uint8_t cc, int8_t channel) {
    prefs.begin(PREF_NS, false);
    prefs.putUInt("cc", (uint32_t)cc);
    prefs.putUInt("chan", (uint32_t)channel);
    prefs.putUInt("magic", EEPROM_MAGIC); // ensure magic persists
    prefs.end();
}

void eeprom_saveCurve(uint8_t curve) {
    prefs.begin(PREF_NS, false);
    prefs.putUChar("curve", (uint8_t)curve);
    prefs.putUInt("magic", EEPROM_MAGIC);
    prefs.end();
}

uint8_t eeprom_getInvert() {
    prefs.begin(PREF_NS, true);
    uint8_t v = prefs.getUChar("invert", DEFAULT_INVERT);
    prefs.end();
    return v;
}

void eeprom_saveInvert(uint8_t inverted) {
    prefs.begin(PREF_NS, false);
    prefs.putUChar("invert", (uint8_t)inverted);
    prefs.putUInt("magic", EEPROM_MAGIC);
    prefs.end();
}
