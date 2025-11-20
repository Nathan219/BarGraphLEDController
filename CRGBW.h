#ifndef CRGBW_H
#define CRGBW_H

#include <FastLED.h>

// Simple CRGBW struct for RGBW strips
struct CRGBW {
  union {
    struct {
      union {
        uint8_t r;
        uint8_t red;
      };
      union {
        uint8_t g;
        uint8_t green;
      };
      union {
        uint8_t b;
        uint8_t blue;
      };
      union {
        uint8_t w;
        uint8_t white;
      };
    };
    uint8_t raw[4];
  };
  
  CRGBW() : r(0), g(0), b(0), w(0) {}
  CRGBW(uint8_t ir, uint8_t ig, uint8_t ib, uint8_t iw) : r(ir), g(ig), b(ib), w(iw) {}
  CRGBW(const CRGB& rgb) : r(rgb.r), g(rgb.g), b(rgb.b), w(0) {}
  
  static CRGBW Black;
  static CRGBW White;
  static CRGBW Yellow;
};

#endif // CRGBW_H

