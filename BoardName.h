#ifndef BOARDNAME_H
#define BOARDNAME_H

#include <FastLED.h>

// BoardName: 2 rows with different lengths (71 total)
// Layout: Row 1 (bottom): FIRST 30 LEDs (0-29) right to left, then Row 0 (top): NEXT 41 LEDs (30-70) left to right
#define BOARDNAME_LEDS 71
#define BOARDNAME_ROWS 2
#define BOARDNAME_COLS 42  // Max columns (for array size), but Row 0 has 41, Row 1 has 30
#define BOARDNAME_ROW0_LEDS 41
#define BOARDNAME_ROW1_LEDS 30

// BoardName row boundaries - start and end columns for each row
// Row 0 (top): 41 LEDs, columns 0-40
// Row 1 (bottom): 30 LEDs, columns 0-29
#define BOARDNAME_ROW0_START 0   // Start column for row 0 (inclusive)
#define BOARDNAME_ROW0_END 40    // End column for row 0 (inclusive)
#define BOARDNAME_ROW1_START 0   // Start column for row 1 (inclusive)
#define BOARDNAME_ROW1_END 29    // End column for row 1 (inclusive)

// BoardName patterns
enum BoardNamePattern : uint8_t {
  BN_OFF = 0,
  BN_PER_WORD = 1,        // 5 distinct colors, fade to new distinct colors every 5 min
  BN_SOLID = 2,           // Random color, fade to different random color every 5 min
  BN_SOLID_WHITE = 3,     // White, stays white
  BN_BLINK = 4,           // Solid color, light up words sequentially
  BN_EVERY_BODY_SAME = 5, // Random color (purple 1/3), EVERY and BODY cycle rainbow (same)
  BN_EVERY_BODY_DIF = 6   // Random color (purple 1/3), EVERY and BODY cycle rainbow (opposite)
};

// BoardName word definitions: "Where is Every\nBody At?!"
// Each word is defined by its start and end column positions (0-41)
struct WordDef {
  int startCol;
  int endCol;
  int row;  // 0 or 1
};
#define NUM_WORDS 5

// BoardName state (extern - defined in BoardName.cpp)
extern CRGB leds_boardname[];
extern BoardNamePattern boardNamePattern;
extern CRGB boardNameColors[];
extern CRGB boardNameTargetColors[];
extern uint8_t boardNameHueOffset;
extern uint32_t boardNameLastColorChange;
extern uint32_t boardNameBlinkStartTime;
extern int boardNameBlinkWordIndex;
extern WordDef boardNameWords[];
extern uint8_t boardNameBrightness;

#define EVERY_WORD_INDEX 2  // "Every" is word index 2
#define BODY_WORD_INDEX 3   // "Body" is word index 3
#define COLOR_FADE_INTERVAL 300000  // 5 minutes in milliseconds
#define BLINK_INTERVAL 1000         // 1 second between words

// Function declarations
void initBoardName();
void drawBoardName();
void handleBoardNameCommand(String rest);
void generateDistinctColors(CRGB* colors);
CRGB generateRandomColor();
void fadeColorsToTarget(CRGB* current, CRGB* target, float step);

#endif // BOARDNAME_H

