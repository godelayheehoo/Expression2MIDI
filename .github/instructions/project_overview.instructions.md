---
description: "When planning out next work"
# applyTo: 'Describe when these instructions should be loaded' # when provided, instructions will automatically be added to the request context when the pattern matches an attached file
---
# Project Overview
## ESP32 MIDI Expression Pedal Project

**Goal**: Build a MIDI expression pedal interface using an ESP32 with a TFT screen and encoder. The initial version reads the pedal position via ADC and sends basic debug output. Future features will add full MIDI CC output, calibration, CC/channel selection, and UI.

## Features (Planned)

- Read analog value from expression pedal (TRS potentiometer)

- TFT screen to display current pedal value

- Calibration routine (set min/max pedal range)

- Clickable encoder to select MIDI CC number and channel

 Immediate send of CC on value changes

- Optional pickup / curve mapping

- EEPROM storage for settings

- Optional panic / reset gesture

## Hardware

- ESP32 dev board (any common variant)

- Expression pedal (TRS, 10k–50k potentiometer)

- TFT display (SPI or parallel, choose library accordingly)

- Clickable endless rotary encoder

- Optional buttons for calibration / reset

## Wiring Notes

Expression pedal:

Tip → ESP32 ADC input

Ring → 3.3V

Sleeve → GND

Encoder: standard 3-pin + switch wiring

TFT: SPI recommended for simplicity and speed

Software

PlatformIO project using Arduino framework

Initial libraries:

Arduino.h

SPI.h / TFT library (TFT_eSPI recommended)

MIDI.h (for eventual MIDI CC output)

Start by reading ADC, normalize to 0–1023, print via Serial for testing

TODO / Development Roadmap

Implement pedal calibration routine

Add TFT interface for CC number, channel, and current value

Implement MIDI CC sending (USB or DIN)

Add encoder handling (switch between CC/channel selection)

Add curve / pickup mode mapping

Store settings in EEPROM

Add optional panic / reset gesture

Optimize TFT updates to avoid flicker / jitter