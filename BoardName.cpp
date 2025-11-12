#include "BoardName.h"

// BoardName LED array
CRGB leds_boardname[BOARDNAME_LEDS];

// BoardName state
BoardNamePattern boardNamePattern = BN_OFF;
CRGB boardNameColors[5];  // Colors for each word: "Where", "is", "Every", "Body", "At?!"
CRGB boardNameTargetColors[5];  // Target colors for fading
uint8_t boardNameHueOffset = 0;
uint32_t boardNameLastColorChange = 0;  // Last time colors changed (for 5-minute fade)
uint32_t boardNameBlinkStartTime = 0;   // When blink sequence started
int boardNameBlinkWordIndex = -1;       // Current word index being lit (-1 = none)

// BoardName word definitions: "Where is Every\nBody At?!"
WordDef boardNameWords[NUM_WORDS] = {
  {0, 9, 0},   // "Where" - approximate, adjust based on actual font
  {10, 11, 0}, // "is"
  {12, 17, 0}, // "Every"
  {0, 4, 1},   // "Body" (second row)
  {5, 8, 1}    // "At?!" (second row)
};

void initBoardName() {
  // Initialize BoardName colors
  for(int i = 0; i < NUM_WORDS; i++) {
    boardNameColors[i] = CRGB::White;
    boardNameTargetColors[i] = CRGB::White;
  }
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

// Map BoardName logical position (row, col) to physical LED index
// Layout: Row 0: LEDs right to left within row0Start-row0End range
//         Row 1: LEDs right to left within row1Start-row1End range
// col is the logical column (0-41), mapped to the physical range defined by row boundaries
int mapBoardNameToPhysical(int row, int col) {
  if (row == 0) {
    // Row 0: reversed (right to left)
    // Map logical col to physical range (BOARDNAME_ROW0_START to BOARDNAME_ROW0_END, but reversed)
    int row0Width = BOARDNAME_ROW0_END - BOARDNAME_ROW0_START + 1;
    if (col < 0 || col >= row0Width) return -1;  // Out of bounds
    
    // Logical col 0 = rightmost of usable area (BOARDNAME_ROW0_END)
    // Logical col (width-1) = leftmost of usable area (BOARDNAME_ROW0_START)
    int physicalCol = BOARDNAME_ROW0_END - col;
    
    // Now map physicalCol to LED index (reversed: col 0 = LED 41, col 41 = LED 0)
    return (BOARDNAME_COLS - 1) - physicalCol;
  } else {
    // Row 1: reversed (right to left), starting at LED 42
    // Map logical col to physical range (BOARDNAME_ROW1_START to BOARDNAME_ROW1_END, but reversed)
    int row1Width = BOARDNAME_ROW1_END - BOARDNAME_ROW1_START + 1;
    if (col < 0 || col >= row1Width) return -1;  // Out of bounds
    
    // Logical col 0 = rightmost of usable area (BOARDNAME_ROW1_END)
    // Logical col (width-1) = leftmost of usable area (BOARDNAME_ROW1_START)
    int physicalCol = BOARDNAME_ROW1_END - col;
    
    // Now map physicalCol to LED index (reversed, starting at LED 42)
    return BOARDNAME_COLS + ((BOARDNAME_COLS - 1) - physicalCol);
  }
}

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
          int ledIdx = mapBoardNameToPhysical(word.row, col);
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
          int ledIdx = mapBoardNameToPhysical(word.row, col);
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
          int ledIdx = mapBoardNameToPhysical(word.row, col);
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
        // Restart sequence
        boardNameBlinkStartTime = now;
        currentWord = -1;
      }
      
      CRGB color = boardNameColors[0];  // Use first color as the blink color
      for (int wordIdx = 0; wordIdx <= currentWord && wordIdx < NUM_WORDS; wordIdx++) {
        WordDef& word = boardNameWords[wordIdx];
        for (int col = word.startCol; col <= word.endCol && col < BOARDNAME_COLS; col++) {
          int ledIdx = mapBoardNameToPhysical(word.row, col);
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
          int ledIdx = mapBoardNameToPhysical(word.row, col);
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

