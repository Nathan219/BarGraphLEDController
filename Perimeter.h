#ifndef PERIMETER_H
#define PERIMETER_H

#include <FastLED.h>

// Perimeter strip: RGB strip around the sign
// 44 LEDs top, 44 LEDs bottom, 72 LEDs each side = 232 total
#define PERIMETER_LEDS 232

// Perimeter patterns
enum PerimeterPattern : uint8_t {
  PER_OFF = 0,
  PER_SOLID = 1,
  PER_ALTERNATING = 2,
  PER_RAINBOW = 3
};

// Perimeter state (extern - defined in Perimeter.cpp)
extern CRGB leds_perimeter[];
extern PerimeterPattern perimeterPattern;
extern CRGB perimeterColor1;
extern CRGB perimeterColor2;
extern uint8_t perimeterBrightness;
extern bool perimeterAltChase;  // If true, alternating pattern chases
extern uint8_t perimeterChasePos;  // Position for chase animation
extern uint32_t perimeterChaseDelay;  // Delay in milliseconds between chase updates (default 500ms)
extern uint32_t perimeterChaseLastUpdate;  // Last time chase position was updated

// Function declarations
void initPerimeter();
void drawPerimeter();
void updatePerimeter();

#endif // PERIMETER_H

