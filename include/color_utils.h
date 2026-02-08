#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include <stdint.h>

static inline uint16_t color565_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// For panels wired as BGR, swap red and blue when converting
static inline uint16_t color565_bgr(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3));
}

// Convenience constants (use BGR converter for panels that require it)
static const uint16_t COLOR_BLACK   = color565_bgr(0, 0, 0);
static const uint16_t COLOR_WHITE   = color565_bgr(255, 255, 255);
static const uint16_t COLOR_MAGENTA = color565_bgr(255, 0, 255);
static const uint16_t COLOR_YELLOW  = color565_bgr(255, 255, 0);
static const uint16_t COLOR_CYAN    = color565_bgr(0, 255, 255);
static const uint16_t COLOR_GREEN   = color565_bgr(0, 255, 0);
static const uint16_t COLOR_RED     = color565_bgr(255, 0, 0);
static const uint16_t COLOR_BLUE    = color565_bgr(0, 0, 255);

#endif // COLOR_UTILS_H
