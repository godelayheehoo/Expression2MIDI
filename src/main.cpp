#include <Arduino.h>

// --- Pin Definitions ---
const int pedalPin = 34;      // ADC input connected to Tip of TRS
const int adcMax = 4095;      // 12-bit ADC on ESP32

// Optional: calibration values (can be loaded from EEPROM later)
int pedalMin = 0;
int pedalMax = adcMax;

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);  // ESP32 default: 12-bit ADC (0-4095)
    Serial.println("ESP32 Expression Pedal Test");
}

void loop() {
    // Read raw ADC
    int raw = analogRead(pedalPin);

    // Normalize to 0-1023 for convenience
    int norm = map(raw, pedalMin, pedalMax, 0, 1023);
    norm = constrain(norm, 0, 1023);

    // Print values for debugging
    Serial.print("Raw ADC: ");
    Serial.print(raw);
    Serial.print("  | Normalized: ");
    Serial.println(norm);

    delay(50); // ~20 Hz update, fast enough for ankle motion
}

/*
TODO:
- Add TFT display for visual feedback
- Implement calibration routine (MIN/MAX)
- Add clickable encoder for CC number & MIDI channel selection
- Send MIDI CC messages via USB or DIN
- Implement pickup / curve mapping
- Store settings in EEPROM
- Optional panic/reset gesture
*/
