#include <FastLED.h>
#include <Preferences.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_bt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "BoardName.h"
#include "UART.h"

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

CRGBW CRGBW::Black(0, 0, 0, 0);
CRGBW CRGBW::White(0, 0, 0, 255);
CRGBW CRGBW::Yellow(255, 255, 0, 0);

// =====================
// --- CONFIG ---
// =====================
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      150

#define PIN_STRIP1   3
#define PIN_STRIP2   2
#define PIN_BOARDNAME 4  // BoardName pin (defined in BoardName.h)
#define PIN_PERIMETER 5

// UART constants are defined in UART.cpp

// =====================
// --- CONSTANTS ---
// =====================
#define TITLE_LEDS 12
#define TITLE_GAP  0
#define PIXELS_PER_STRIP 6

// New row-based weaving layout
#define LEDS_PER_ROW 42
#define ROWS_PER_FLOOR 2
#define LEDS_PER_FLOOR (LEDS_PER_ROW * 2)  // Row 1: 42 going left, Row 2: 42 going right = 84 LEDs per floor

// Each pixel is 2 LEDs tall (one in each row), floors are 42 pixels wide
#define PIXELS_PER_FLOOR 42
#define LEDS_PER_PIXEL_HEIGHT 2  // Each pixel is 2 LEDs tall (one per row)

// Pixel definition: each pixel has explicit left and right column boundaries
struct PixelDef {
  int leftCol;   // Left column (inclusive)
  int rightCol;  // Right column (inclusive)
};

// Define pixel boundaries for each floor
// Each pixel spans from leftCol to rightCol (inclusive)
// Pixels are numbered 0 to PIXELS_PER_STRIP-1
// Example: Pixel 0 might be columns 12-15, Pixel 1 might be columns 18-21, etc.
PixelDef floorPixels[PIXELS_PER_STRIP] = {
  {13, 17},  // Pixel 0: columns 13-17 (5 columns)
  {18, 22},  // Pixel 1: columns 18-22 (5 columns)
  {23, 27},  // Pixel 2: columns 23-27 (5 columns)
  {28, 32},  // Pixel 3: columns 28-32 (5 columns)
  {33, 37},  // Pixel 4: columns 33-37 (5 columns)
  {38, 41}   // Pixel 5: columns 38-41 (4 columns) - last column (only valid columns 0-41)
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
    177,176,175,174,173,172,171,170,169,168,167
  },
  {  // Row 1
    209, 210, 211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250
  }
};

const int pool_led_map[2][LEDS_PER_ROW] = {
  {  // Row 0
    293,292,291,290,289,288,287,286,285,284,283,282,281,280,279,278,
    277,276,275,274,273,272,271,270,269,268,267,266,265,264,263,262,
    261,260,259,258,257,256,255,254,253,252,
  },
  {  // Row 1
    294,295,296,297,298,299,300,301,302,303,304,305,306,307,308,309,
    310,311,312,313,314,315,316,317,318,319,320,321,322,323,324,325,
    326,327,328,329,330,331,332,333,334,335,
  }
};

// BoardName constants and definitions are in BoardName.h

// Perimeter strip: RGBW strip around the sign
#define PERIMETER_LEDS 250


// =====================
// --- ARRAYS ---
// =====================
CRGB leds_strip1[STRIP1_LEDS];  // Floor 11, 12, 15
CRGB leds_strip2[STRIP2_LEDS];  // Floor 16, 17, Tea Room, Pool
// leds_boardname is defined in BoardName.cpp
CRGBW leds_perimeter[PERIMETER_LEDS];  // Perimeter RGBW strip


// =====================
// --- STATE ---
// =====================
float currentLeds[7];
int   targetLeds[7];
bool  rainbow[7];
int   rainbowWavePos[7];  // Pixel position of transition wave (0 to totalLitPixels, in pixel units)
bool  easterEgg = true;
uint8_t hueOffset = 0;

// label mapping
enum Floors {F11, F12, F15, F16, F17, POOL, TEA};
const char* labels[7] = {"FLOOR11","FLOOR12","FLOOR15","FLOOR16","FLOOR17","POOL","TEAROOM"};

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

enum RainbowWaveMode : uint8_t {
  WAVE_IDLE = 0,
  WAVE_ENTERING = 1,
  WAVE_EXITING = 2
};

// BoardName patterns are defined in BoardName.h

// Perimeter patterns
enum PerimeterPattern : uint8_t {
  PER_OFF = 0,
  PER_SOLID = 1,
  PER_ALTERNATING = 2,
  PER_CHASING_RAINBOW = 3
};

CRGB titleColor[7] = {CRGB::Purple,CRGB::Cyan,CRGB::Orange,CRGB::Green,CRGB::Blue,CRGB::HotPink,CRGB::Yellow};
CRGB pixelColor[7] = {CRGB::Purple,CRGB::Cyan,CRGB::Orange,CRGB::Green,CRGB::Blue,CRGB::HotPink,CRGB::Yellow};

// BoardName state is defined in BoardName.cpp

// Perimeter state
PerimeterPattern perimeterPattern = PER_OFF;
CRGBW perimeterColor1 = CRGBW::White;
CRGBW perimeterColor2 = CRGBW::Yellow;
uint8_t perimeterHueOffset = 0;
uint8_t perimeterChasePos = 0;

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
                    CRGB titleColor, CRGB staticColor, int wavePos, bool targetRainbow, uint8_t waveMode);

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
  // For WS2814 RGBW, cast CRGBW array to CRGB* for FastLED
  // Note: This assumes CRGBW has the same memory layout as CRGB with an extra byte
  FastLED.addLeds<WS2812B, PIN_PERIMETER, RGB>(reinterpret_cast<CRGB*>(leds_perimeter), PERIMETER_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  
  randomSeed(analogRead(0));  // Initialize random seed
  initBoardName();  // Initialize BoardName colors

  // Restore
  for (int i=0;i<7;i++) {
    String v=String(labels[i])+"_val";
    String r=String(labels[i])+"_r";
    int saved = prefs.getInt(v.c_str(),0);
    bool rb = prefs.getBool(r.c_str(),false);
    // Calculate total pixels based on saved value (0-6)
    // Each pixel's width is calculated from its boundaries
    int totalPixels = 0;
    for (int p = 0; p < saved && p < PIXELS_PER_STRIP; p++) {
      totalPixels += (floorPixels[p].rightCol - floorPixels[p].leftCol + 1);
    }
    targetLeds[i] = totalPixels;
    currentLeds[i]=targetLeds[i];
    rainbow[i]=rb;
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
        drawFloorWeave(leds_strip1, F17, totalLit, titleColor[F17], pixelColor[F17], (int)wave, state, mode);
        break;
      case F16:
        drawFloorWeave(leds_strip1, F16, totalLit, titleColor[F16], pixelColor[F16], (int)wave, state, mode);
        break;
      case F15:
        drawFloorWeave(leds_strip1, F15, totalLit, titleColor[F15], pixelColor[F15], (int)wave, state, mode);
        break;
      case F12:
        drawFloorWeave(leds_strip2, F12, totalLit, titleColor[F12], pixelColor[F12], (int)wave, state, mode);
        break;
      case F11:
        drawFloorWeave(leds_strip2, F11, totalLit, titleColor[F11], pixelColor[F11], (int)wave, state, mode);
        break;
      case TEA:
        drawFloorWeave(leds_strip2, TEA, totalLit, titleColor[TEA], pixelColor[TEA], (int)wave, state, mode);
        break;
      case POOL:
        drawFloorWeave(leds_strip2, POOL, totalLit, titleColor[POOL], pixelColor[POOL], (int)wave, state, mode);
        break;
    }
  }

  // Update BoardName animations
  if (boardNamePattern == BN_EVERY_BODY_SAME || boardNamePattern == BN_EVERY_BODY_DIF) {
    boardNameHueOffset += 2;
  }
  drawBoardName();

  // Update Perimeter animations
  if (perimeterPattern == PER_CHASING_RAINBOW) {
    perimeterHueOffset += 2;
    perimeterChasePos = (perimeterChasePos + 1) % PERIMETER_LEDS;
  }
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

// =====================
// --- PERIMETER FUNCTIONS ---
// =====================

void drawPerimeter() {
  // Clear all LEDs
  for (int i = 0; i < PERIMETER_LEDS; i++) {
    leds_perimeter[i] = CRGBW::Black;
  }
  
  if (perimeterPattern == PER_OFF) return;
  
  switch (perimeterPattern) {
    case PER_SOLID: {
      for (int i = 0; i < PERIMETER_LEDS; i++) {
        leds_perimeter[i] = perimeterColor1;
      }
      break;
    }
    
    case PER_ALTERNATING: {
      for (int i = 0; i < PERIMETER_LEDS; i++) {
        leds_perimeter[i] = (i % 2 == 0) ? perimeterColor1 : perimeterColor2;
      }
      break;
    }
    
    case PER_CHASING_RAINBOW: {
      for (int i = 0; i < PERIMETER_LEDS; i++) {
        uint8_t hue = ((perimeterHueOffset + (i * 256 / PERIMETER_LEDS) + (perimeterChasePos * 10)) & 255);
        CRGB rgb = CHSV(hue, 255, 255);
        leds_perimeter[i] = CRGBW(rgb.r, rgb.g, rgb.b, 0);
      }
      break;
    }
    
    default:
      break;
  }
}

// =====================
// --- PARSER ---
// =====================
// UART functions are in UART.cpp

// =====================
// --- DRAW FUNCTIONS ---
// =====================
void drawFloorWeave(CRGB* leds, int floorIndex, float ledsLit,
                    CRGB titleColor, CRGB staticColor, int wavePos, bool targetRainbow, uint8_t waveMode) {
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
  if (easterEgg) {
    const float bpm = 174.0, beatMs = 60000.0/bpm, barMs = beatMs*4.0;
    static uint32_t syncStart = millis();
    float t = fmod((float)(millis()-syncStart), barMs);
    float b = t/beatMs;
    if (b<0.20) beatBrightness+=40;
    else if (b>=2.0&&b<2.20) beatBrightness+=40;
    else if (b>=2.35&&b<2.50) beatBrightness+=35;
    else if (b>=3.0&&b<3.25) beatBrightness+=30;
  }

  // Draw pixels using their explicit boundaries
  float totalLit = ledsLit;
  int totalLitInt = (int)(totalLit + 0.5f);  // Round to nearest integer for wave calculations
  int cumulativePos = 0;  // Track cumulative position for rainbow wave calculations (in pixel units)
  
  // Draw each pixel
  for (int pixelIdx = 0; pixelIdx < PIXELS_PER_STRIP; pixelIdx++) {
    PixelDef& pixel = floorPixels[pixelIdx];
    int pixelWidth = pixel.rightCol - pixel.leftCol + 1;
    
    // Check if this pixel should be lit at all
    if (cumulativePos >= totalLitInt) break;  // No more pixels to light
    
    // Calculate how much of this pixel should be lit
    int pixelStartPos = cumulativePos;
    int pixelEndPos = cumulativePos + pixelWidth;
    float litInPixel = fminf((float)pixelWidth, totalLit - cumulativePos);
    
    if (litInPixel <= 0) {
      cumulativePos += pixelWidth;
      continue;
    }
    
    // Determine if this pixel should be rainbow based on wave position and direction
    bool useRainbow = false;
    
    // Safety: if targetRainbow is false and wave is idle, never show rainbow
    if (!targetRainbow && waveMode == WAVE_IDLE) {
      useRainbow = false;
    } else if (waveMode == WAVE_ENTERING) {
      // Transitioning to rainbow - only if targetRainbow is true
      if (targetRainbow) {
        useRainbow = pixelEndPos >= (totalLitInt - wavePos);
      } else {
        useRainbow = false;
      }
    } else if (waveMode == WAVE_EXITING) {
      // Transitioning away from rainbow - only if targetRainbow is false
      if (!targetRainbow) {
        useRainbow = pixelStartPos >= wavePos;
      } else {
        useRainbow = false;
      }
    } else {
      // WAVE_IDLE: use targetRainbow directly
      useRainbow = targetRainbow;
    }
    
    // Determine color for this pixel
    CRGB pixelColorVal;
    if (useRainbow) {
      pixelColorVal = CHSV((255 - hueOffset + (pixelIdx * 40)) & 255, 255, beatBrightness);
    } else {
      pixelColorVal = staticColor;
    }
    
    // Draw this pixel's columns
    // Determine title boundaries for this floor (matches title drawing above)
    int titleStartCol = 0;
    int titleEndCol = titleStartCol + TITLE_LEDS - 1;
    
    for (int col = pixel.leftCol; col <= pixel.rightCol; col++) {
      if (col < 0 || col >= LEDS_PER_ROW) continue;  // Allow columns 0-41 (42 columns total)
      
      // Skip title area columns - don't overwrite title
      if (col >= titleStartCol && col <= titleEndCol) continue;
      
      // Check if this column should be lit
      float colPosInPixel = col - pixel.leftCol;
      if (colPosInPixel >= litInPixel) break;  // Past the lit portion
      
      // Each pixel is 2 LEDs tall: draw LED in row 0 and row 1
      // Ensure we only draw within this floor's boundaries
      // Row 0
      int physIdx0 = ledMap[0][col];
      if (physIdx0 >= 0 && physIdx0 >= floorStart && physIdx0 < floorEnd && physIdx0 < maxLeds) {
        leds[physIdx0] = pixelColorVal;
      }
      
      // Row 1
      int physIdx1 = ledMap[1][col];
      if (physIdx1 >= 0 && physIdx1 >= floorStart && physIdx1 < floorEnd && physIdx1 < maxLeds) {
        leds[physIdx1] = pixelColorVal;
      }
    }
    
    cumulativePos += pixelWidth;
  }
}

