// --- Encoder pins ---
#define ENCODER_BTN 25
#define ENCODER_A  33
#define ENCODER_B  35

// --- ST7789 TFT pins (adjust as needed) ---
// These defaults are common for many ESP32 setups; change to match your wiring.
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  4   // moved off 23

#define TFT_MOSI 23 //SDA
#define TFT_SCLK 18 

// --- MIDI pins ---
#define MIDI_TX_PIN 17 // change to your desired TX pin

// --- Other pins ---
#define PEDAL_PIN  32     // ADC input connected to Tip of TRS
#define AUX_BUTTON_PIN  0     // GPIO pin connected to save button. NOTE: uses external pullup as well, as this is meant for 5V