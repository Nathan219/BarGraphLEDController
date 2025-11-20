#include <FastLED.h>
#include <Preferences.h>
#include <WiFi.h>
#include <stdint.h>
#include "esp_wifi.h"
#include "esp_bt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "BoardName.h"
#include "Perimeter.h"
#include "UART.h"
#include "CRGBW.h"

// CRGBW static member definitions
CRGBW CRGBW::Black(0, 0, 0, 0);
CRGBW CRGBW::White(0, 0, 0, 255);
CRGBW CRGBW::Yellow(255, 255, 0, 0);

// =====================
// --- CONFIG ---
// =====================
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      150

#define PIN_STRIP1   2
#define PIN_STRIP2   3
#define PIN_BOARDNAME 4  // BoardName pin (defined in BoardName.h)
#define PIN_PERIMETER 5

// UART constants are defined in UART.cpp

// =====================
// --- CONSTANTS ---
// =====================
#define TITLE_LEDS 12
#define TITLE_GAP  0
#define PIXELS_PER_STRIP 6
#define MAX_PIXEL_LEDS   10
#define TOTAL_FLOORS     7

// New row-based weaving layout
#define LEDS_PER_ROW 42
#define ROWS_PER_FLOOR 2
#define LEDS_PER_FLOOR (LEDS_PER_ROW * 2)  // Row 1: 42 going left, Row 2: 42 going right = 84 LEDs per floor

// Each pixel is 2 LEDs tall (one in each row), floors are 42 pixels wide
#define PIXELS_PER_FLOOR 42
#define LEDS_PER_PIXEL_HEIGHT 2  // Each pixel is 2 LEDs tall (one per row)

// Physical LED indices used by each pixel on every floor (constant)
const int16_t floorPixelLedMap[TOTAL_FLOORS][PIXELS_PER_STRIP][MAX_PIXEL_LEDS] = {
  { // F17
    {  28,  55,  27,  56,  26,  57,  25,  58,  24,  59 },
    {  23,  60,  22,  61,  21,  62,  20,  63,  19,  64 },
    {  18,  65,  17,  66,  16,  67,  15,  68,  14,  69 },
    {  13,  70,  12,  71,  11,  72,  10,  73,   9,  74 },
    {   8,  75,   7,  76,   6,  77,   5,  78,   4,  79 },
    {   3,  80,   2,  81,   1,  82,   0,  83,  -1,  -1 },
  },
  { // F16
    { 112, 139, 111, 140, 110, 141, 109, 142, 108, 143 },
    { 107, 144, 106, 145, 105, 146, 104, 147, 103, 148 },
    { 102, 149, 101, 150, 100, 151,  99, 152,  98, 153 },
    {  97, 154,  96, 155,  95, 156,  94, 157,  93, 158 },
    {  92, 159,  91, 160,  90, 161,  89, 162,  88, 163 },
    {  87, 164,  86, 165,  85, 166,  84,  -1,  -1,  -1 },
  },
  { // F15
    { 195, 222, 194, 223, 193, 224, 192, 225, 191, 226 },
    { 190, 227, 189, 228, 188, 229, 187, 230, 186, 231 },
    { 185, 232, 184, 233, 183, 234, 182, 235, 181, 236 },
    { 180, 237, 179, 238, 178, 239, 177, 240, 176, 241 },
    { 175, 242, 174, 243, 173, 244, 172, 245, 171, 246 },
    { 170, 247, 169, 248, 168, 249, 167, 250,  -1,  -1 },
  },
  { // F12
    {  28,  55,  27,  56,  26,  57,  25,  58,  24,  59 },
    {  23,  60,  22,  61,  21,  62,  20,  63,  19,  64 },
    {  18,  65,  17,  66,  16,  67,  15,  68,  14,  69 },
    {  13,  70,  12,  71,  11,  72,  10,  73,   9,  74 },
    {   8,  75,   7,  76,   6,  77,   5,  78,   4,  79 },
    {   3,  80,   2,  81,   1,  82,   0,  83,  -1,  -1 },
  },
  { // F11
    { 112, 139, 111, 140, 110, 141, 109, 142, 108, 143 },
    { 107, 144, 106, 145, 105, 146, 104, 147, 103, 148 },
    { 102, 149, 101, 150, 100, 151,  99, 152,  98, 153 },
    {  97, 154,  96, 155,  95, 156,  94, 157,  93, 158 },
    {  92, 159,  91, 160,  90, 161,  89, 162,  88, 163 },
    {  87, 164,  86, 165,  85, 166,  84, 167,  -1,  -1 },
  },
  { // TEA
    { 195, -1, 194, -1, 193, 224, 192, 225, 227, 226 },
    { 190, 232, 189, 228, 188, 229, 187, 230, 191, 231 },
    { 185, 237, 184, 233, 183, 234, 182, 235, 186, 236 },
    { 180, 242, 179, 238, 178, 239, 177, 240, 181, 241 },
    { 175, -1, 174, 243, 173, 244, 172, 245, 176, 246 },
    { 170, -1, 169, 248, 168, 249, 167, 250, 171, -1 },
  },
  { // POOL
    { 280, 307, 279, 308, 278, 309, 277, 310, 276, 311 },
    { 275, 312, 274, 313, 273, 314, 272, 315, 271, 316 },
    { 270, 317, 269, 318, 268, 319, 267, 320, 266, 321 },
    { 265, 322, 264, 323, 263, 324, 262, 325, 261, 326 },
    { 260, 327, 259, 328, 258, 329, 257, 330, 256, 331 },
    { 255, 332, 254, 333, 253, 334, 252, 335,  -1,  -1 },
  },
};

const uint8_t floorPixelLedCount[TOTAL_FLOORS][PIXELS_PER_STRIP] = {
  {10, 10, 10, 10, 10,  8},  // F17
  {10, 10, 10, 10, 10,  6},  // F16
  {10, 10, 10, 10, 10,  8},  // F15
  {10, 10, 10, 10, 10,  8},  // F12
  {10, 10, 10, 10, 10,  8},  // F11
  {10, 10, 10, 10, 10,  8},  // TEA
  {10, 10, 10, 10, 10,  8},  // POOL
};

const uint16_t floorTotalLedCount[TOTAL_FLOORS] = {
  58, // F17
  56, // F16
  58, // F15
  58, // F12
  58, // F11
  58, // TEA
  58  // POOL
};

// Floor groupings on strips
// Order from top to bottom: F17, F16, F15, F12, F11, TEA, POOL
// Strip 1: F17, F16, F15 (3 floors, top)
// Strip 2: F12, F11, TEA, POOL (4 floors, bottom)
#define STRIP1_LEDS (LEDS_PER_FLOOR * 3)  // 252 LEDs (84 * 3)
#define STRIP2_LEDS (LEDS_PER_FLOOR * 4)  // 336 LEDs (84 * 4)

// Explicit LED start positions for each floor (0-indexed)
// Strip 1 (leds_strip1):
#define FLOOR17_START 0                    // LEDs 0-83
#define FLOOR16_START LEDS_PER_FLOOR       // LEDs 84-167
#define FLOOR15_START (LEDS_PER_FLOOR * 2) // LEDs 168-251

// Strip 2 (leds_strip2):
#define FLOOR12_START 0                    // LEDs 0-83
#define FLOOR11_START LEDS_PER_FLOOR       // LEDs 84-167
#define TEAROOM_START (LEDS_PER_FLOOR * 2) // LEDs 168-251
#define POOL_START (LEDS_PER_FLOOR * 3)    // LEDs 252-335

// Logical to physical LED mapping for each floor
// Each array is [row][column] where row is 0 or 1, column is 0-41
const int floor17_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
     41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26,
     25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,
      9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
  },
  {  // Row 1
     42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
     74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
  }
};

const int floor16_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
    125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,
    109,108,107,106,105,104,103,102,101,100, 99, 98, 97, 96, 95, 94,
     93, 92, 91, 90, 89, 88, 87, 86, 85, 84,
  },
  {  // Row 1
    126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,
    142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,
    158,159,160,161,162,163,164,165, 166
  }
};

const int floor15_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
    208,207,206,205,204,203,202,201,200,199,198,197,196,195,194,
    193,192,191,190,189,188,187,186,185,184,183,182,181,180,179,178,
    177,176,175,174,173,172,171,170,169,168,167
  },
  {  // Row 1
    209, 210, 211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250
  }
};

const int floor12_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
     41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26,
     25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10,
      9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
  },
  {  // Row 1
     42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
     74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
  }
};

const int floor11_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
    125,124,123,122,121,120,119,118,117,116,115,114,113,112,111,110,
    109,108,107,106,105,104,103,102,101,100, 99, 98, 97, 96, 95, 94,
     93, 92, 91, 90, 89, 88, 87, 86, 85, 84,
  },
  {  // Row 1
    126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,
    142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,
    158,159,160,161,162,163,164,165,166,167,
  }
};

const int tearoom_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
    208,207,206,205,204,203,202,201,200,199,198,197,196,195,194,
    193,192,191,190,189,188,187,186,185,184,183,182,181,180,179,178,
    177,176,175,174,173,172,171,170,169,168,167,
  },
  {  // Row 1
    209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,
  }
};

const int pool_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
    293,292,291,290,289,288,287,286,285,284,283,282,281,280,279,278,
    277,276,275,274,273,272,271,270,269,268,267,266,265,264,263,262,
    261,260,259,258,257,256,255,254,253,
  },
  {  // Row 1
    294,295,296,297,298,299,300,301,302,303,304,305,306,307,308,309,
    310,311,312,313,314,315,316,317,318,319,320,321,322,323,324,325,
    326,327,328,329,330,331,332,333,334,335,
  }
};

// BoardName constants and definitions are in BoardName.h
// Perimeter constants and definitions are in Perimeter.h


// =====================
// --- ARRAYS ---
// =====================
CRGB leds_strip1[STRIP1_LEDS];  // Floor 11, 12, 15
CRGB leds_strip2[STRIP2_LEDS];  // Floor 16, 17, Tea Room, Pool
// leds_boardname is defined in BoardName.cpp
// leds_perimeter is defined in Perimeter.cpp


// =====================
// --- STATE ---
// =====================
float currentLeds[7];
int   targetLeds[7];
bool  rainbow[7];
bool  multiColor[7];  // Multi-color mode: each pixel is a different static color
int   rainbowWavePos[7];  // Pixel position of transition wave (0 to totalLitPixels, in pixel units)
bool  easterEgg = true;
uint8_t hueOffset = 0;
bool  testMode = false;  // Test mode: lights all floors with multi-color pixels

// label mapping
enum Floors {F17, F16, F15, F12, F11, TEA, POOL};
const char* labels[7] = {"FLOOR17","FLOOR16","FLOOR15","FLOOR12","FLOOR11","TEAROOM","POOL"};

// Get the LED mapping array for a given floor
// Returns a pointer to a 2D array [2][LEDS_PER_ROW]
const int (*getFloorLedMap(int floorIndex))[LEDS_PER_ROW] {
  switch (floorIndex) {
    case F17: return floor17_led_map;
    case F16: return floor16_led_map;
    case F15: return floor15_led_map;
    case F12: return floor12_led_map;
    case F11: return floor11_led_map;
    case TEA: return tearoom_led_map;
    case POOL: return pool_led_map;
    default: return nullptr;
  }
}

const int16_t* getFloorPixelLedIndices(int floorIndex, int pixelIndex) {
  if (floorIndex < 0 || floorIndex >= TOTAL_FLOORS) return nullptr;
  if (pixelIndex < 0 || pixelIndex >= PIXELS_PER_STRIP) return nullptr;
  return floorPixelLedMap[floorIndex][pixelIndex];
}

int getFloorPixelLedCount(int floorIndex, int pixelIndex) {
  if (floorIndex < 0 || floorIndex >= TOTAL_FLOORS) return 0;
  if (pixelIndex < 0 || pixelIndex >= PIXELS_PER_STRIP) return 0;
  return floorPixelLedCount[floorIndex][pixelIndex];
}

int getTotalLedsForFloor(int floorIndex) {
  if (floorIndex < 0 || floorIndex >= TOTAL_FLOORS) return 0;
  return floorTotalLedCount[floorIndex];
}

int getLitLedsForValue(int floorIndex, int pixelValue) {
  if (floorIndex < 0 || floorIndex >= TOTAL_FLOORS) return 0;
  int total = 0;
  for (int pixelIdx = 0; pixelIdx < pixelValue && pixelIdx < PIXELS_PER_STRIP; pixelIdx++) {
    total += getFloorPixelLedCount(floorIndex, pixelIdx);
  }
  return total;
}

enum RainbowWaveMode : uint8_t {
  WAVE_IDLE = 0,
  WAVE_ENTERING = 1,
  WAVE_EXITING = 2
};

// BoardName patterns are defined in BoardName.h
// Perimeter patterns are defined in Perimeter.h

CRGB titleColor[7] = {CRGB::Blue,CRGB::Green,CRGB::Orange,CRGB::Cyan,CRGB::Purple,CRGB::Yellow,CRGB::HotPink};
CRGB pixelColor[7] = {CRGB::Blue,CRGB::Green,CRGB::Orange,CRGB::Cyan,CRGB::Purple,CRGB::Yellow,CRGB::HotPink};

// Multi-color palette: different color for each pixel (0-5)
CRGB multiColorPalette[6] = {
  CRGB::Red,
  CRGB::Green,
  CRGB::Blue,
  CRGB::Yellow,
  CRGB::Purple,
  CRGB::Cyan
};

// BoardName state is defined in BoardName.cpp
// Perimeter state is defined in Perimeter.cpp

// BoardName word definitions are in BoardName.cpp

// =====================
// --- GLOBALS ---
// =====================
Preferences prefs;
// extSerial is defined in UART.cpp
float animSpeed = 1.0f;  // LEDs per frame
TaskHandle_t uartTaskHandle = nullptr;
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
uint8_t rainbowWaveMode[7]; // 0=idle,1=entering,2=exiting

// UART functions are in UART.cpp
void drawFloorWeave(CRGB* leds, int floorIndex, float ledsLit,
                    CRGB titleColor, CRGB staticColor, int wavePos, bool targetRainbow, uint8_t waveMode, bool useMultiColor);

// =====================
// --- SETUP ---
// =====================
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_stop();
  btStop();
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

  initUART();  // Initialize UART hardware
  prefs.begin("floors", false);

  FastLED.addLeds<LED_TYPE, PIN_STRIP1, COLOR_ORDER>(leds_strip1, STRIP1_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_STRIP2, COLOR_ORDER>(leds_strip2, STRIP2_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_BOARDNAME, COLOR_ORDER>(leds_boardname, BOARDNAME_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_PERIMETER, COLOR_ORDER>(leds_perimeter, PERIMETER_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  randomSeed(analogRead(0));  // Initialize random seed
  initBoardName();  // Initialize BoardName colors
  initPerimeter();  // Initialize Perimeter colors

  // Restore
  for (int i=0;i<7;i++) {
    String v=String(labels[i])+"_val";
    String r=String(labels[i])+"_r";
    int saved = prefs.getInt(v.c_str(),0);
    bool rb = prefs.getBool(r.c_str(),false);
    // Calculate total pixels based on saved value (0-6)
    // Each pixel's width is calculated from its boundaries
    int totalPixels = getLitLedsForValue(i, saved);
    targetLeds[i] = totalPixels;
    currentLeds[i]=targetLeds[i];
    rainbow[i]=rb;
    multiColor[i] = false;  // Multi-color mode not saved/restored (default off)
    // Initialize wave position: if rainbow, wave is at the end; if not, wave is at start
  rainbowWavePos[i] = rb ? (int)(currentLeds[i] + 0.5f) : 0;
  rainbowWaveMode[i] = WAVE_IDLE;
  }

  Serial.println("✅ Ready with per-LED smooth animation.");

  xTaskCreatePinnedToCore(uartTask, "UARTTask", 8192, nullptr, 1, &uartTaskHandle, 1);
}

// =====================
// --- LOOP ---
// =====================
void loop() {
  // Test mode: light all floors fully with multi-color pixels
  if (testMode) {
    portENTER_CRITICAL(&stateMux);
    for (int j = 0; j < 7; j++) {
      targetLeds[j] = getTotalLedsForFloor(j);
      rainbow[j] = false;  // Disable rainbow in test mode
    }
    portEXIT_CRITICAL(&stateMux);
  }

  for (int i=0;i<7;i++) {
    animateProgress(currentLeds[i], targetLeds[i], animSpeed);

    portENTER_CRITICAL(&stateMux);
    float totalLit = currentLeds[i];
    int wave = rainbowWavePos[i];
    uint8_t mode = rainbowWaveMode[i];
    bool isRainbow = rainbow[i];
    int totalLitInt = (int)(totalLit + 0.5f);  // Round to nearest integer

    switch (mode) {
      case WAVE_ENTERING:
        // Only animate if rainbow is actually enabled
        if (!isRainbow) {
          wave = 0;
          rainbowWaveMode[i] = WAVE_IDLE;
        } else if (wave < totalLitInt) {
          int animSpeedInt = (int)(animSpeed + 0.5f);
          wave = min(totalLitInt, wave + max(1, animSpeedInt));
        }
        if (wave >= totalLitInt) {
          wave = totalLitInt;
          rainbowWaveMode[i] = WAVE_IDLE;
        }
        break;
      case WAVE_EXITING:
        // Only animate if rainbow is actually disabled
        if (isRainbow) {
          wave = totalLitInt;
          rainbowWaveMode[i] = WAVE_IDLE;
        } else if (wave < totalLitInt) {
          int animSpeedInt = (int)(animSpeed + 0.5f);
          wave = min(totalLitInt, wave + max(1, animSpeedInt));
        }
        if (wave >= totalLitInt) {
          wave = 0;
          rainbowWaveMode[i] = WAVE_IDLE;
        }
        break;
      default:
        wave = isRainbow ? totalLitInt : 0;
        break;
    }

    // Clamp to valid range relative to current lit amount
    if (wave > totalLitInt) wave = totalLitInt;
    if (wave < 0) wave = 0;

    rainbowWavePos[i] = wave;
    mode = rainbowWaveMode[i];
    bool state = rainbow[i];
    portEXIT_CRITICAL(&stateMux);

  // Update draw data arrays after leaving critical section
  // Order from top to bottom: F17, F16, F15, F12, F11, TEA, POOL
  // Strip 1: F17 (0-83), F16 (84-167), F15 (168-251)
  // Strip 2: F12 (0-83), F11 (84-167), TEA (168-251), POOL (252-335)
  switch (i) {
      case F17:
        drawFloorWeave(leds_strip1, F17, totalLit, titleColor[F17], pixelColor[F17], (int)wave, state, mode, multiColor[F17] || testMode);
        break;
      case F16:
        drawFloorWeave(leds_strip1, F16, totalLit, titleColor[F16], pixelColor[F16], (int)wave, state, mode, multiColor[F16] || testMode);
        break;
      case F15:
        drawFloorWeave(leds_strip1, F15, totalLit, titleColor[F15], pixelColor[F15], (int)wave, state, mode, multiColor[F15] || testMode);
        break;
      case F12:
        drawFloorWeave(leds_strip2, F12, totalLit, titleColor[F12], pixelColor[F12], (int)wave, state, mode, multiColor[F12] || testMode);
        break;
      case F11:
        drawFloorWeave(leds_strip2, F11, totalLit, titleColor[F11], pixelColor[F11], (int)wave, state, mode, multiColor[F11] || testMode);
        break;
      case TEA:
        drawFloorWeave(leds_strip2, TEA, totalLit, titleColor[TEA], pixelColor[TEA], (int)wave, state, mode, multiColor[TEA] || testMode);
        break;
      case POOL:
        drawFloorWeave(leds_strip2, POOL, totalLit, titleColor[POOL], pixelColor[POOL], (int)wave, state, mode, multiColor[POOL] || testMode);
        break;
    }
  }

  // Update BoardName animations
  if (boardNamePattern == BN_EVERY_BODY_SAME || boardNamePattern == BN_EVERY_BODY_DIF) {
    boardNameHueOffset += 2;
  }
  drawBoardName();

  // Update Perimeter animations
  updatePerimeter();
  drawPerimeter();

  FastLED.show();
  hueOffset+=2;
  delay(15);
}

// =====================
// --- FUNCTIONS ---
// =====================

// Map logical LED position within a floor to physical position in the strip
// 42x2 matrix layout (0-indexed physical positions):
// Row 0 (logical 0-41): maps to physical 41-0 (reversed, right to left)
//   - logical 0 → physical 41, logical 41 → physical 0 (first LED)
// Row 1 (logical 42-83): maps to physical 42-83 (normal, left to right)
//   - logical 42 → physical 42, logical 83 → physical 83 (last LED)
// LED mapping is now done via constant arrays (floor17_led_map, etc.)
// See getFloorLedMap() function to get the appropriate mapping array

void animateProgress(float &current, int target, float step) {
  if (fabs(current - target) > 0.01f) {
    if (current < target) current = fminf((float)target, current + step);
    else current = fmaxf((float)target, current - step);
  }
}

void saveState(const char* keyV,const char* keyR,int val,bool r){
  prefs.putInt(keyV,val);
  prefs.putBool(keyR,r);
}

// =====================
// BoardName functions are in BoardName.cpp
// Perimeter functions are in Perimeter.cpp

// =====================
// --- PARSER ---
// =====================
// UART functions are in UART.cpp

// =====================
// --- DRAW FUNCTIONS ---
// =====================
void drawFloorWeave(CRGB* leds, int floorIndex, float ledsLit,
                    CRGB titleColor, CRGB staticColor, int wavePos, bool targetRainbow, uint8_t waveMode, bool useMultiColor) {
  // Get the LED mapping array for this floor
  const int (*ledMap)[LEDS_PER_ROW] = getFloorLedMap(floorIndex);
  if (ledMap == nullptr) return;
  
  // Determine which strip this is and its max LEDs
  int floorStart = (floorIndex == F17 || floorIndex == F16 || floorIndex == F15) ? 
                   ((floorIndex == F17) ? FLOOR17_START : (floorIndex == F16) ? FLOOR16_START : FLOOR15_START) :
                   ((floorIndex == F12) ? FLOOR12_START : (floorIndex == F11) ? FLOOR11_START : (floorIndex == TEA) ? TEAROOM_START : POOL_START);
  int maxLeds = (floorIndex == F17 || floorIndex == F16 || floorIndex == F15) ? STRIP1_LEDS : STRIP2_LEDS;
  
  // Clear this floor's LEDs
  int floorEnd = floorStart + LEDS_PER_FLOOR;  // Exclusive end of this floor's range
  for (int row = 0; row < 2; row++) {
    for (int col = 0; col < LEDS_PER_ROW; col++) {
      int physIdx = ledMap[row][col];
      if (physIdx >= 0 && physIdx >= floorStart && physIdx < floorEnd && physIdx < maxLeds) {
        leds[physIdx] = CRGB::Black;
      }
    }
  }

  // Title zone: covering both rows (2 LEDs tall) - includes column 0
  // All floors need columns 0-12 for title area
  int titleStartCol = 0;
  int titleEndCol = titleStartCol + TITLE_LEDS - 1;
  for (int col = titleStartCol; col <= titleEndCol; col++) {
    // Row 0
    int physIdx0 = ledMap[0][col];
    if (physIdx0 >= 0 && physIdx0 >= floorStart && physIdx0 < floorEnd && physIdx0 < maxLeds) {
      leds[physIdx0] = titleColor;
    }
    // Row 1
    int physIdx1 = ledMap[1][col];
    if (physIdx1 >= 0 && physIdx1 >= floorStart && physIdx1 < floorEnd && physIdx1 < maxLeds) {
      leds[physIdx1] = titleColor;
    }
  }

  if (ledsLit <= 0.05f) return;

  uint8_t beatBrightness = 200;
  // Disable easter egg flashing in multi-color mode
  if (easterEgg && !useMultiColor) {
    const float bpm = 174.0, beatMs = 60000.0/bpm, barMs = beatMs*4.0;
    static uint32_t syncStart = millis();
    float t = fmod((float)(millis()-syncStart), barMs);
    float b = t/beatMs;
    if (b<0.20) beatBrightness+=40;
    else if (b>=2.0&&b<2.20) beatBrightness+=40;
    else if (b>=2.35&&b<2.50) beatBrightness+=35;
    else if (b>=3.0&&b<3.25) beatBrightness+=30;
  }

  // Draw pixels using explicit LED lists
  float totalLit = ledsLit;
  int totalLitInt = (int)(totalLit + 0.5f);  // Round to nearest integer for wave calculations
  int cumulativePos = 0;  // Track cumulative position for rainbow wave calculations (in LED units)
  
  for (int pixelIdx = 0; pixelIdx < PIXELS_PER_STRIP; pixelIdx++) {
    int pixelLedCount = getFloorPixelLedCount(floorIndex, pixelIdx);
    if (pixelLedCount <= 0) continue;
    
    if (cumulativePos >= totalLitInt) break;  // No more LEDs to light
    
    float litInPixel = fminf((float)pixelLedCount, totalLit - cumulativePos);
    if (litInPixel <= 0.0f) {
      cumulativePos += pixelLedCount;
      continue;
    }
    
    int pixelStartPos = cumulativePos;
    int pixelEndPos = cumulativePos + pixelLedCount;
    
    // Determine if this pixel should be rainbow based on wave position and direction
    bool useRainbow = false;
    
    if (!targetRainbow && waveMode == WAVE_IDLE) {
      useRainbow = false;
    } else if (waveMode == WAVE_ENTERING) {
      if (targetRainbow) {
        useRainbow = pixelEndPos >= (totalLitInt - wavePos);
      } else {
        useRainbow = false;
      }
    } else if (waveMode == WAVE_EXITING) {
      if (!targetRainbow) {
        useRainbow = pixelStartPos >= wavePos;
      } else {
        useRainbow = false;
      }
    } else {
      useRainbow = targetRainbow;
    }
    
    CRGB pixelColorVal;
    if (useMultiColor) {
      pixelColorVal = multiColorPalette[pixelIdx % 6];
    } else if (useRainbow) {
      pixelColorVal = CHSV((255 - hueOffset + (pixelIdx * 40)) & 255, 255, beatBrightness);
    } else {
      pixelColorVal = staticColor;
    }
    
    const int16_t* ledIndices = getFloorPixelLedIndices(floorIndex, pixelIdx);
    if (ledIndices != nullptr) {
      for (int ledOffset = 0; ledOffset < pixelLedCount && ledOffset < MAX_PIXEL_LEDS; ledOffset++) {
        if (ledOffset >= litInPixel) break;
        int physIdx = ledIndices[ledOffset];
        if (physIdx < 0) continue;
        if (physIdx >= floorStart && physIdx < floorEnd && physIdx < maxLeds) {
          leds[physIdx] = pixelColorVal;
        }
      }
    }
    
    cumulativePos += pixelLedCount;
  }
}

