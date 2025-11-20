#include "BoardName.h"

// BoardName LED array
CRGB leds_boardname[BOARDNAME_LEDS];

// Logical to physical LED mapping for BoardName
// Each array is [row][column] where row is 0 or 1
// Row 1 (bottom): FIRST LEDs 0-29, 30 LEDs, right to left (col 0 = LED 29, col 29 = LED 0)
// Row 0 (top): NEXT LEDs 30-70, 41 LEDs, left to right (col 0 = LED 30, col 40 = LED 70)
// Invalid columns return -1
const int boardname_led_map[2][BOARDNAME_COLS] = {
  {  // Row 0 (top): 41 LEDs, left to right (LEDs 30-70)
     30, 31, 32, 33, 34, 35, 36, 37,
     38, 39, 40, 41, 42, 43, 44, 45,
     46, 47, 48, 49, 50, 51, 52, 53,
     54, 55, 56, 57, 58, 59, 60, 61,
     62, 63, 64, 65, 66, 67, 68, 69,
     70, -1,
  },
  {  // Row 1 (bottom): 30 LEDs, right to left (LEDs 29-0)
     29, 28, 27, 26, 25, 24, 23, 22,
     21, 20, 19, 18, 17, 16, 15, 14,
     13, 12, 11, 10,  9,  8,  7,  6,
      5,  4,  3,  2,  1,  0, -1, -1,
     -1, -1, -1, -1, -1, -1, -1, -1,
     -1, -1,
  }
};

// BoardName state
BoardNamePattern boardNamePattern = BN_PER_WORD;  // Default to PER WORD pattern
CRGB boardNameColors[5];  // Colors for each word: "Where", "is", "Every", "Body", "At?!"
CRGB boardNameTargetColors[5];  // Target colors for fading
uint8_t boardNameHueOffset = 0;
uint32_t boardNameLastColorChange = 0;  // Last time colors changed (for 5-minute fade)
uint32_t boardNameBlinkStartTime = 0;   // When blink sequence started
int boardNameBlinkWordIndex = -1;       // Current word index being lit (-1 = none)
uint8_t boardNameBrightness = 255;      // 100% brightness

// BoardName word definitions: "Where is Every\nBody At?!"
WordDef boardNameWords[NUM_WORDS] = {
  {0, 18, 0},   // "Where" - approximate, adjust based on actual font
  {19, 25, 0}, // "is"
  {26, 40, 0}, // "Every"
  {0, 14, 1},   // "Body" (second row)
  {15, 29, 1}    // "At?!" (second row)
};

void initBoardName() {
  // Initialize BoardName with PER WORD pattern
  boardNamePattern = BN_PER_WORD;
  generateDistinctColors(boardNameColors);
  generateDistinctColors(boardNameTargetColors);
  boardNameLastColorChange = millis();
}

// Generate 5 distinct colors
void generateDistinctColors(CRGB* colors) {
  // Use evenly spaced hues for distinct colors
  for (int i = 0; i < 5; i++) {
    uint8_t hue = (i * 51) & 255;  // 51 = 255/5
    colors[i] = CHSV(hue, 255, 255);
  }
}

// Generate a random color (with 1/3 chance of purple)
CRGB generateRandomColor() {
  if (random(3) == 0) {
    // 1/3 chance of purple
    return CRGB::Purple;
  } else {
    // Random hue
    return CHSV(random(256), 255, 255);
  }
}

// Fade colors towards target colors
void fadeColorsToTarget(CRGB* current, CRGB* target, float step) {
  for (int i = 0; i < 5; i++) {
    float r = (float)current[i].r;
    float g = (float)current[i].g;
    float b = (float)current[i].b;
    
    if (r < target[i].r) {
      r = fminf(255.0f, r + step);
    } else if (r > target[i].r) {
      r = fmaxf(0.0f, r - step);
    }
    
    if (g < target[i].g) {
      g = fminf(255.0f, g + step);
    } else if (g > target[i].g) {
      g = fmaxf(0.0f, g - step);
    }
    
    if (b < target[i].b) {
      b = fminf(255.0f, b + step);
    } else if (b > target[i].b) {
      b = fmaxf(0.0f, b - step);
    }
    
    current[i].r = (uint8_t)r;
    current[i].g = (uint8_t)g;
    current[i].b = (uint8_t)b;
  }
}

// BoardName LED mapping is now done via boardname_led_map array (same as floors)

void drawBoardName() {
  // Clear all LEDs
  fill_solid(leds_boardname, BOARDNAME_LEDS, CRGB::Black);
  
  if (boardNamePattern == BN_OFF) return;
  
  // Handle color fading for patterns that need it
  uint32_t now = millis();
  if (boardNamePattern == BN_PER_WORD || boardNamePattern == BN_SOLID) {
    if (now - boardNameLastColorChange >= COLOR_FADE_INTERVAL) {
      // Time to change colors
      if (boardNamePattern == BN_PER_WORD) {
        generateDistinctColors(boardNameTargetColors);
      } else {
        // BN_SOLID - single random color for all
        CRGB newColor = generateRandomColor();
        for (int i = 0; i < NUM_WORDS; i++) {
          boardNameTargetColors[i] = newColor;
        }
      }
      boardNameLastColorChange = now;
    }
    // Fade colors towards target (slow fade over 5 minutes)
    fadeColorsToTarget(boardNameColors, boardNameTargetColors, 0.1f);
  }
  
  switch (boardNamePattern) {
    case BN_PER_WORD: {
      // Each word has its own distinct color, fading to new distinct colors
      for (int wordIdx = 0; wordIdx < NUM_WORDS; wordIdx++) {
        WordDef& word = boardNameWords[wordIdx];
        CRGB color = boardNameColors[wordIdx];
        for (int col = word.startCol; col <= word.endCol && col < BOARDNAME_COLS; col++) {
          int ledIdx = boardname_led_map[word.row][col];
          if (ledIdx >= 0 && ledIdx < BOARDNAME_LEDS) {
            leds_boardname[ledIdx] = color;
          }
        }
      }
      break;
    }
    
    case BN_SOLID: {
      // All words same color, fading to new random color
      CRGB color = boardNameColors[0];
      for (int wordIdx = 0; wordIdx < NUM_WORDS; wordIdx++) {
        WordDef& word = boardNameWords[wordIdx];
        for (int col = word.startCol; col <= word.endCol && col < BOARDNAME_COLS; col++) {
          int ledIdx = boardname_led_map[word.row][col];
          if (ledIdx >= 0 && ledIdx < BOARDNAME_LEDS) {
            leds_boardname[ledIdx] = color;
          }
        }
      }
      break;
    }
    
    case BN_SOLID_WHITE: {
      // All words white
      for (int wordIdx = 0; wordIdx < NUM_WORDS; wordIdx++) {
        WordDef& word = boardNameWords[wordIdx];
        for (int col = word.startCol; col <= word.endCol && col < BOARDNAME_COLS; col++) {
          int ledIdx = boardname_led_map[word.row][col];
          if (ledIdx >= 0 && ledIdx < BOARDNAME_LEDS) {
            leds_boardname[ledIdx] = CRGB::White;
          }
        }
      }
      break;
    }
    
    case BN_BLINK: {
      // Light up words sequentially
      uint32_t elapsed = now - boardNameBlinkStartTime;
      int currentWord = (elapsed / BLINK_INTERVAL) - 1;  // -1 = none, 0-4 = words
      if (currentWord < 0) currentWord = -1;
      if (currentWord >= NUM_WORDS) {
        // Restart sequence with a new random color
        boardNameColors[0] = generateRandomColor();
        boardNameBlinkStartTime = now;
        currentWord = -1;
      }
      
      CRGB color = boardNameColors[0];  // Use first color as the blink color
      for (int wordIdx = 0; wordIdx <= currentWord && wordIdx < NUM_WORDS; wordIdx++) {
        WordDef& word = boardNameWords[wordIdx];
        for (int col = word.startCol; col <= word.endCol && col < BOARDNAME_COLS; col++) {
          int ledIdx = boardname_led_map[word.row][col];
          if (ledIdx >= 0 && ledIdx < BOARDNAME_LEDS) {
            leds_boardname[ledIdx] = color;
          }
        }
      }
      break;
    }
    
    case BN_EVERY_BODY_SAME:
    case BN_EVERY_BODY_DIF: {
      // Use stored random color for other words (set when pattern starts)
      CRGB otherColor = boardNameColors[0];  // Store random color in first slot
      
      // EVERY and BODY cycle rainbow
      uint8_t everyHue = boardNameHueOffset;
      uint8_t bodyHue = (boardNamePattern == BN_EVERY_BODY_SAME) ? everyHue : ((everyHue + 128) & 255);
      
      for (int wordIdx = 0; wordIdx < NUM_WORDS; wordIdx++) {
        WordDef& word = boardNameWords[wordIdx];
        CRGB color;
        
        if (wordIdx == EVERY_WORD_INDEX) {
          color = CHSV(everyHue, 255, 255);
        } else if (wordIdx == BODY_WORD_INDEX) {
          color = CHSV(bodyHue, 255, 255);
        } else {
          color = otherColor;
        }
        
        for (int col = word.startCol; col <= word.endCol && col < BOARDNAME_COLS; col++) {
          int ledIdx = boardname_led_map[word.row][col];
          if (ledIdx >= 0 && ledIdx < BOARDNAME_LEDS) {
            leds_boardname[ledIdx] = color;
          }
        }
      }
      break;
    }
    
    default:
      break;
  }
  
  // Apply brightness scaling
  if (boardNameBrightness < 255) {
    for (int i = 0; i < BOARDNAME_LEDS; i++) {
      leds_boardname[i].r = (leds_boardname[i].r * boardNameBrightness) / 255;
      leds_boardname[i].g = (leds_boardname[i].g * boardNameBrightness) / 255;
      leds_boardname[i].b = (leds_boardname[i].b * boardNameBrightness) / 255;
    }
  }
}

void handleBoardNameCommand(String rest) {
  if(rest.equalsIgnoreCase("OFF")){
    boardNamePattern = BN_OFF;
  } else if(rest.equalsIgnoreCase("PER WORD")){
    boardNamePattern = BN_PER_WORD;
    generateDistinctColors(boardNameColors);
    generateDistinctColors(boardNameTargetColors);
    boardNameLastColorChange = millis();
  } else if(rest.equalsIgnoreCase("SOLID")){
    boardNamePattern = BN_SOLID;
    CRGB startColor = generateRandomColor();
    for (int i = 0; i < NUM_WORDS; i++) {
      boardNameColors[i] = startColor;
      boardNameTargetColors[i] = generateRandomColor();
    }
    boardNameLastColorChange = millis();
  } else if(rest.equalsIgnoreCase("SOLID WHITE")){
    boardNamePattern = BN_SOLID_WHITE;
  } else if(rest.equalsIgnoreCase("BLINK")){
    boardNamePattern = BN_BLINK;
    boardNameColors[0] = generateRandomColor();
    boardNameBlinkStartTime = millis();
    boardNameBlinkWordIndex = -1;
  } else if(rest.equalsIgnoreCase("EVERY BODY SAME")){
    boardNamePattern = BN_EVERY_BODY_SAME;
    boardNameHueOffset = 0;
    boardNameColors[0] = generateRandomColor();  // Store random color for other words
  } else if(rest.equalsIgnoreCase("EVERY BODY DIF")){
    boardNamePattern = BN_EVERY_BODY_DIF;
    boardNameHueOffset = 0;
    boardNameColors[0] = generateRandomColor();  // Store random color for other words
  }
  Serial.printf("BoardName pattern: %d\n", boardNamePattern);
}

