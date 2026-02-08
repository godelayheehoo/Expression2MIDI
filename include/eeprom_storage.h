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

// Save values to storage
void eeprom_save(uint8_t cc, int8_t channel);
