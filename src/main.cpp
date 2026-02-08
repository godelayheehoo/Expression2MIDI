#include <Arduino.h>
#include <MIDI.h>
#include "eeprom_storage.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>
#include "menu_manager.h"
#include "../include/color_utils.h"
#include <cstring>
#include <ESP32Encoder.h>
#include <pin_definitions.h>


// --- MIDI (FortySevenEffects) ---
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);


// --- Pin Definitions ---

const int adcMax = 4095;      // 12-bit ADC on ESP32

// Optional: calibration values (can be loaded from EEPROM later)
int pedalMin = 0;
int pedalMax = adcMax;

//temporary mappings (will be loaded from storage in setup)
uint8_t CC_NUMBER = 74;
int8_t MIDI_CHANNEL = 14; //MIDI channel 15


//// DISPLAY SETUP
// Create the display instance (using hardware SPI)
Adafruit_ST7796S tft = Adafruit_ST7796S(TFT_CS, TFT_DC, TFT_RST);
MenuManager menu;

//// BUTTON SETUP
const unsigned long DEFAULT_BUTTON_DEBOUNCE_MS = 50;

// Button debouncing helper
struct ButtonHelper {
  int pin;
  bool lastState;
  bool currentState;
  bool initialState;
  unsigned long lastChangeTime;
  unsigned long debounceDelay;
  
  ButtonHelper(int buttonPin, unsigned long debounceMs = 50) 
    : pin(buttonPin), debounceDelay(debounceMs), lastChangeTime(0) {
    pinMode(pin, INPUT_PULLUP);
    initialState = digitalRead(pin);  // Assume this is the unpressed state
    lastState = initialState;
    currentState = initialState;
    Serial.println("ButtonHelper initialized for pin " + String(pin));
  }
  
  bool isPressed() {
    bool reading = digitalRead(pin);
    unsigned long currentTime = millis();
    
    if (reading != lastState) {
      lastChangeTime = currentTime;
    }
    
    if ((currentTime - lastChangeTime) > debounceDelay) {
      if (reading != currentState) {
        currentState = reading;
        lastState = reading;
        Serial.print("Button state changed for buttton #");
        Serial.println(pin);
        // Return true when state changes from initial (unpressed) to opposite (pressed)
        return (currentState != initialState);
      }
    }
    
    lastState = reading;
    return false;
  }
};

ButtonHelper auxButtonHelper(AUX_BUTTON_PIN, DEFAULT_BUTTON_DEBOUNCE_MS);

//// ENCODER SETUP
ESP32Encoder enc;
long lastEncoderPos = 0;
ButtonHelper encoderBtnHelper(ENCODER_BTN, DEFAULT_BUTTON_DEBOUNCE_MS);

// Interrupt flags
bool encoderButtonFlag = false;





///// function prototypes
void tftStartupTest();

// ISRs
// void IRAM_ATTR encoderButtonISR() {
//   encoderButtonFlag = true;
// }


void setup() {
    Serial.begin(115200);
    analogReadResolution(12);  // ESP32 default: 12-bit ADC (0-4095)
    analogSetAttenuation(ADC_11db);  // Set ADC attenuation to 11dB for full-scale voltage
    Serial.println("ESP32 Expression Pedal Test");
    // Configure save button pin
    pinMode(AUX_BUTTON_PIN, INPUT_PULLUP);
    ButtonHelper saveButtonHelper(AUX_BUTTON_PIN, DEFAULT_BUTTON_DEBOUNCE_MS);
    ButtonHelper encoderBtnHelper(ENCODER_BTN, DEFAULT_BUTTON_DEBOUNCE_MS);
    // Initialize storage and load settings
    bool existing = eeprom_init();
    if (!existing) {
        Serial.println("No existing settings — defaults written to storage.");
    } else {
        Serial.println("Loaded settings from storage.");
    }
    // Load saved CC and channel
    CC_NUMBER = eeprom_getCC();
    MIDI_CHANNEL = eeprom_getChannel();
    Serial.print("MIDI CC:"); Serial.print(CC_NUMBER); Serial.print("  Channel:"); Serial.println(MIDI_CHANNEL + 1);
    // Initialize UART for MIDI transmission (TX-only)
    // MIDI baud rate is 31250, provide -1 for RX when unused
    Serial1.begin(31250, SERIAL_8N1, -1, MIDI_TX_PIN);
    MIDI.begin(MIDI_CHANNEL_OMNI);

    // Initialize TFT
    SPI.begin();
    tft.init(320,480); // init(width, height) for ST7796 (typical 480x320 panel)
    tft.setRotation(0);
    Serial.print("TFT initialized");
    tft.fillScreen(COLOR_BLACK);
    tft.invertDisplay(true);
// tftStartupTest();
    
    menu.begin(&tft);
    // Render simple main menu text for now
    menu.renderMainMenu();

    // Encoder setup
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    enc.attachSingleEdge(ENCODER_B, ENCODER_A);
    enc.setCount(0);
      // Initialize encoder pins
    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);
    //attach interrupt
    // attachInterrupt(digitalPinToInterrupt(ENCODER_BTN), encoderButtonISR, FALLING);
    
}

void loop() {
    // Read raw ADC
    int raw = analogRead(PEDAL_PIN);

    // Normalize to 0-1023 for convenience
    int norm = map(raw, pedalMin, pedalMax, 0, 1023);
    norm = constrain(norm, 0, 1023);

    // Print values for debugging
    // Serial.print("Raw ADC: ");
    // Serial.print(raw);
    // Serial.print("  | Normalized: ");
    // Serial.println(norm);

    // --- Save button handling (active LOW) ---
    // This will also be what sets MIDI_CHANNEL and MIDI_CC or whatever I called them from "NEW_MIDI_CHANNLEL"
    // and NEW_MIDI_CC, which are what are displayed in the menu as the controls sweep through. 
    if(auxButtonHelper.isPressed()) {
        Serial.println("Back Button Pressed");
        menu.handleInput(AuxBtn);
    }
    if(encoderBtnHelper.isPressed()) {
        Serial.println("Encoder button pressed");
        menu.handleInput(EncoderBtn);
    }

      //check encoder and pass in turns if turns!=0
  int newEncoderPos =  enc.getCount();
  int encoderTurns = newEncoderPos - lastEncoderPos;
  // if(encoderTurns!=0){Serial.println(encoderTurns);};
  if (encoderTurns > 0) {
    lastEncoderPos = newEncoderPos;
    menu.handleInput(EncoderCW);
    } else if (encoderTurns < 0) {
    lastEncoderPos = newEncoderPos;
    menu.handleInput(EncoderCCW);
    }

    // Monitor polling (non-blocking)
    const unsigned long MONITOR_POLL_MS = 100; // poll every 100 ms
    static unsigned long lastMonitorPoll = 0;
    unsigned long now = millis();
    if ((now - lastMonitorPoll) >= MONITOR_POLL_MS) {
      lastMonitorPoll = now;
      int raw = analogRead(PEDAL_PIN);
      int norm = map(raw, pedalMin, pedalMax, 0, 1023);
      norm = constrain(norm, 0, 1023);
      // Debug print whenever polled
      Serial.print("Monitor poll - raw:"); Serial.print(raw); Serial.print(" norm:"); Serial.println(norm);
      menu.updateMonitor(raw, norm);
    }

    // delay(50); // ~20 Hz update, fast enough for ankle motion
}

//// Setup Functions
void tftStartupTest() {
  // Clear
  Serial.println("All black screen");
  tft.fillScreen(ST77XX_BLACK);
  int16_t w = tft.width();
  int16_t h = tft.height();
  delay(200);

  // Magenta square (centered)
  Serial.println("Magenta square");
  int16_t sz = min(w, h) / 3;
  int16_t sx = (w - sz) / 2;
  int16_t sy = (h - sz) / 2;
  tft.fillRect(sx, sy, sz, sz, COLOR_MAGENTA);
  delay(800);

  // Yellow triangle (centered)
  Serial.println("Yellow triangle");
  tft.fillScreen(ST77XX_BLACK);
  int16_t tx0 = w / 2;
  int16_t ty0 = h / 2 - sz / 2;
  int16_t tx1 = w / 2 - sz / 2;
  int16_t ty1 = h / 2 + sz / 2;
  int16_t tx2 = w / 2 + sz / 2;
  int16_t ty2 = h / 2 + sz / 2;
  tft.fillTriangle(tx0, ty0, tx1, ty1, tx2, ty2, COLOR_YELLOW);
  delay(800);

  // Cyan circle (centered)
  tft.fillScreen(COLOR_BLACK);
  Serial.println("Cyan circle");
  int16_t r = sz / 2;
  tft.fillCircle(w / 2, h / 2, r, COLOR_CYAN);
  delay(800);

  // Green "test!" in large text
  Serial.println("Green test!");
  tft.fillScreen(COLOR_BLACK);
  tft.setTextColor(COLOR_GREEN);
  tft.setTextSize(4);
  const char* msg = "test!";
  int len = strlen(msg);
  int charWidth = 6 * 4; // approx width per char at size 4
  int textW = charWidth * len;
  int16_t cx = (w - textW) / 2;
  int16_t cy = (h - (8 * 4)) / 2;
  if (cx < 0) cx = 0;
  if (cy < 0) cy = 0;
  tft.setCursor(cx, cy);
  tft.print(msg);
  delay(1000);
  tft.setTextSize(1);
}