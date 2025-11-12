#ifndef BOARDNAME_H
#define BOARDNAME_H

#include <FastLED.h>

// BoardName: 2 rows of 42 LEDs each (84 total)
// Layout: 42 LEDs right to left, then up, then 42 more right to left
#define BOARDNAME_LEDS 84
#define BOARDNAME_ROWS 2
#define BOARDNAME_COLS 42

// BoardName row boundaries - start and end columns for each row
// These account for dead space on the physical strip
#define BOARDNAME_ROW0_START 0   // Start column for row 0 (inclusive)
#define BOARDNAME_ROW0_END 41    // End column for row 0 (inclusive)
#define BOARDNAME_ROW1_START 0   // Start column for row 1 (inclusive)
#define BOARDNAME_ROW1_END 41    // End column for row 1 (inclusive)

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

#define EVERY_WORD_INDEX 2  // "Every" is word index 2
#define BODY_WORD_INDEX 3   // "Body" is word index 3
#define COLOR_FADE_INTERVAL 300000  // 5 minutes in milliseconds
#define BLINK_INTERVAL 5000         // 5 seconds between words

// Function declarations
void initBoardName();
void drawBoardName();
void handleBoardNameCommand(String rest);
int mapBoardNameToPhysical(int row, int col);
void generateDistinctColors(CRGB* colors);
CRGB generateRandomColor();
void fadeColorsToTarget(CRGB* current, CRGB* target, float step);

#endif // BOARDNAME_H

