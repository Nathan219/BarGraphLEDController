#include "Perimeter.h"

// Perimeter LED array
CRGB leds_perimeter[PERIMETER_LEDS];

// Perimeter state
PerimeterPattern perimeterPattern = PER_OFF;
CRGB perimeterColor1 = CRGB::Purple;
CRGB perimeterColor2 = CRGB::Blue;
uint8_t perimeterBrightness =   100;  // Default brightness
bool perimeterAltChase = true;  // Chase mode for alternating pattern
uint8_t perimeterChasePos = 0;  // Position for chase animation
uint32_t perimeterChaseDelay = 500;  // Default chase delay: 500ms
uint32_t perimeterChaseLastUpdate = 0;  // Last time chase position was updated

void initPerimeter() {
  // Initialize perimeter colors
  perimeterColor1 = CRGB::Purple;
  perimeterColor2 = CRGB::Blue;
  perimeterBrightness = 100;
  // Set a default pattern for testing (can be changed via UART)
  perimeterPattern = PER_ALTERNATING;
}

void updatePerimeter() {
  // Update chase position for alternating pattern
  if (perimeterPattern == PER_ALTERNATING && perimeterAltChase) {
    uint32_t now = millis();
    if (now - perimeterChaseLastUpdate >= perimeterChaseDelay) {
      perimeterChasePos = (perimeterChasePos + 1) % PERIMETER_LEDS;
      perimeterChaseLastUpdate = now;
    }
  }
  // Perimeter rainbow uses the global hueOffset from LEDController.ino
  // No separate update needed - it's handled in the main loop
}

void drawPerimeter() {
  // Clear all LEDs
  for (int i = 0; i < PERIMETER_LEDS; i++) {
    leds_perimeter[i] = CRGB::Black;
  }
  
  if (perimeterPattern == PER_OFF) {
    // Even when OFF, set first LED to dim red for debugging
    leds_perimeter[0] = CRGB(10, 0, 0);
    return;
  }
  
  switch (perimeterPattern) {
    case PER_SOLID: {
      for (int i = 0; i < PERIMETER_LEDS; i++) {
        leds_perimeter[i] = perimeterColor1;
      }
      break;
    }
    
    case PER_ALTERNATING: {
      if (perimeterAltChase) {
        // Chase mode: colors chase around the perimeter
        for (int i = 0; i < PERIMETER_LEDS; i++) {
          int pos = (i + perimeterChasePos) % PERIMETER_LEDS;
          leds_perimeter[i] = (pos % 2 == 0) ? perimeterColor1 : perimeterColor2;
        }
      } else {
        // Static alternating pattern
        for (int i = 0; i < PERIMETER_LEDS; i++) {
          leds_perimeter[i] = (i % 2 == 0) ? perimeterColor1 : perimeterColor2;
        }
      }
      break;
    }
    
    case PER_RAINBOW: {
      // Use the same hueOffset as floors for consistent speed
      // Rainbow cycles across the strip with spacing similar to floors
      extern uint8_t hueOffset;
      for (int i = 0; i < PERIMETER_LEDS; i++) {
        // Match floor rainbow spacing: floors use (pixelIdx * 40), we'll use (i * 2) for similar effect
        uint8_t hue = ((255 - hueOffset + (i * 2)) & 255);
        leds_perimeter[i] = CHSV(hue, 255, 255);
      }
      break;
    }
    
    default:
      break;
  }
  
  // Apply brightness scaling
  if (perimeterBrightness < 255) {
    for (int i = 0; i < PERIMETER_LEDS; i++) {
      leds_perimeter[i].r = (leds_perimeter[i].r * perimeterBrightness) / 255;
      leds_perimeter[i].g = (leds_perimeter[i].g * perimeterBrightness) / 255;
      leds_perimeter[i].b = (leds_perimeter[i].b * perimeterBrightness) / 255;
    }
  }
}

