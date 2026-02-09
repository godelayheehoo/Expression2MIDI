// EEPROM-like storage using ESP32 Preferences
#pragma once

#include <stdint.h>

// Initialize storage. Returns true if existing settings were found, false if defaults were written.
bool eeprom_init();

// Get stored values (after init)
uint8_t eeprom_getCC();
int8_t eeprom_getChannel();

// Curve storage: stores an index representing the chosen curve
uint8_t eeprom_getCurve();
void eeprom_saveCurve(uint8_t curve);

// Invert flag storage: 0 == normal, 1 == inverted
uint8_t eeprom_getInvert();
void eeprom_saveInvert(uint8_t inverted);

// Save values to storage
void eeprom_save(uint8_t cc, int8_t channel);

// Save CC only (does not touch saved channel)
void eeprom_saveCC(uint8_t cc);

// Active instrument index (0 == None)
uint8_t eeprom_getActiveInstrument();
void eeprom_saveActiveInstrument(uint8_t idx);
