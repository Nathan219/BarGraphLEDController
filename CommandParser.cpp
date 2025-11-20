#include "CommandParser.h"
#include "BoardName.h"
#include "Perimeter.h"
#include <Preferences.h>
#include <FastLED.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Forward declarations for state access
extern Preferences prefs;
extern portMUX_TYPE stateMux;
extern float currentLeds[7];
extern int targetLeds[7];
extern bool rainbow[7];
extern int rainbowWavePos[7];
extern uint8_t rainbowWaveMode[7];
extern bool easterEgg;
extern bool multiColor[7];
extern const char* labels[7];
extern HardwareSerial extSerial;
extern bool testMode;
extern int getLitLedsForValue(int floorIndex, int pixelValue);
#define PIXELS_PER_STRIP 6

enum RainbowWaveMode : uint8_t {
  WAVE_IDLE = 0,
  WAVE_ENTERING = 1,
  WAVE_EXITING = 2
};

// PerimeterPattern enum and variables are defined in Perimeter.h
extern BoardNamePattern boardNamePattern;

// Forward declaration
void saveState(const char* keyV, const char* keyR, int val, bool r);

// Helper function to get BoardName pattern name as string
String getBoardNamePatternName(BoardNamePattern pattern) {
  switch(pattern) {
    case BN_OFF: return "OFF";
    case BN_PER_WORD: return "PER WORD";
    case BN_SOLID: return "SOLID";
    case BN_SOLID_WHITE: return "SOLID WHITE";
    case BN_BLINK: return "BLINK";
    case BN_EVERY_BODY_SAME: return "EVERY BODY SAME";
    case BN_EVERY_BODY_DIF: return "EVERY BODY DIF";
    default: return "UNKNOWN";
  }
}

// Helper function to get Perimeter pattern name as string
String getPerimeterPatternName(PerimeterPattern pattern) {
  switch(pattern) {
    case PER_OFF: return "OFF";
    case PER_SOLID: return "SOLID";
    case PER_ALTERNATING: return "ALT";
    case PER_RAINBOW: return "RAINBOW";
    default: return "UNKNOWN";
  }
}

// Color name to CRGBW mapping
struct ColorDef {
  const char* name;
  uint8_t r, g, b, w;
};

// Define available colors (about 20 major colors)
const ColorDef colorMap[] = {
  {"RED", 255, 0, 0, 0},
  {"GREEN", 0, 255, 0, 0},
  {"BLUE", 0, 0, 255, 0},
  {"YELLOW", 255, 255, 0, 0},
  {"PURPLE", 128, 0, 128, 0},
  {"CYAN", 0, 255, 255, 0},
  {"MAGENTA", 255, 0, 255, 0},
  {"ORANGE", 255, 165, 0, 0},
  {"PINK", 255, 192, 203, 0},
  {"WHITE", 255, 255, 255, 0},  // RGB white for CRGB
  {"BLACK", 0, 0, 0, 0},
  {"LIME", 50, 255, 0, 0},  // Bright yellow-green
  {"TEAL", 0, 128, 128, 0},
  {"NAVY", 0, 0, 128, 0},
  {"MAROON", 128, 0, 0, 0},
  {"OLIVE", 128, 128, 0, 0},
  {"GOLD", 255, 215, 0, 0},
  {"SILVER", 192, 192, 192, 0},
  {"CORAL", 255, 127, 80, 0},
  {"TURQUOISE", 64, 224, 208, 0}
};

const int NUM_COLORS = sizeof(colorMap) / sizeof(colorMap[0]);

// Convert color name to CRGB
bool parseColorName(const String& colorName, CRGB& color) {
  String upperName = colorName;
  upperName.toUpperCase();
  upperName.trim();
  
  for(int i = 0; i < NUM_COLORS; i++) {
    if(upperName.equals(colorMap[i].name)) {
      color = CRGB(colorMap[i].r, colorMap[i].g, colorMap[i].b);
      return true;
    }
  }
  return false;
}

// Convert CRGB to color name
String getColorName(const CRGB& color) {
  for(int i = 0; i < NUM_COLORS; i++) {
    if(color.r == colorMap[i].r && 
       color.g == colorMap[i].g && 
       color.b == colorMap[i].b && 
       colorMap[i].w == 0) {  // Only match if white component is 0 (RGB colors)
      return String(colorMap[i].name);
    }
  }
  // If no exact match, return RGB values as fallback
  return String(color.r) + " " + String(color.g) + " " + String(color.b);
}

void handleInput(String msg){
  msg.trim();
  if(msg.isEmpty()) return;

  Serial.println(msg);
  int sp=msg.indexOf(' ');
  if(sp==-1) sp=msg.length();
  String label=msg.substring(0,sp);
  String rest=(sp<(int)msg.length())?msg.substring(sp+1):"";
  rest.trim();
  
  // GET commands - return current state
  if(label.equalsIgnoreCase("GET")){
    if(rest.equalsIgnoreCase("BOARDNAME")){
      String response = "BOARDNAME " + getBoardNamePatternName(boardNamePattern);
      Serial.println(response);
      extSerial.println(response);
      return;
    }
    if(rest.equalsIgnoreCase("PERIMETER") || rest.equalsIgnoreCase("PER")){
      String response = "PER " + getPerimeterPatternName(perimeterPattern);
      if(perimeterPattern == PER_SOLID){
        response += " " + getColorName(perimeterColor1);
      } else if(perimeterPattern == PER_ALTERNATING){
        extern bool perimeterAltChase;
        extern uint32_t perimeterChaseDelay;
        response += " ALT " + getColorName(perimeterColor1) + " " + getColorName(perimeterColor2);
        if(perimeterAltChase){
          response += " CHASE " + String(perimeterChaseDelay);
        }
      }
      Serial.println(response);
      extSerial.println(response);
      return;
    }
    String response = "INVALID COMMAND";
    Serial.println(response);
    extSerial.println(response);
    return;
  }
  
  // SET TESTMODE command - enable/disable test mode
  // SET TESTMODE command - enable/disable test mode
  if(label.equalsIgnoreCase("SET") && rest.startsWith("TESTMODE")){
    // Parse: SET TESTMODE {TRUE/FALSE}
    int sp1 = rest.indexOf(' ', 8);
    if(sp1 > 0){
      String value = rest.substring(sp1+1);
      value.trim();
      
      extern bool testMode;
      
      if(value.equalsIgnoreCase("TRUE")){
        testMode = true;
        String response = "TESTMODE ACCEPTED";
        Serial.println(response);
        extSerial.println(response);
        return;
      } else if(value.equalsIgnoreCase("FALSE")){
        testMode = false;
        String response = "TESTMODE ACCEPTED";
        Serial.println(response);
        extSerial.println(response);
        return;
      }
    }
    // Invalid SET TESTMODE command
    String response = "INVALID COMMAND";
    Serial.println(response);
    extSerial.println(response);
    return;
  }
  
  // SET BRITE command - set brightness for BoardName or Perimeter
  if(label.equalsIgnoreCase("SET") && rest.startsWith("BRITE")){
    // Parse: SET BRITE {PER/PERIMETER/BD/BOARDNAME} <value>
    int sp1 = rest.indexOf(' ', 5);
    if(sp1 > 0){
      int sp2 = rest.indexOf(' ', sp1+1);
      if(sp2 > 0){
        String target = rest.substring(sp1+1, sp2);
        String valueStr = rest.substring(sp2+1);
        valueStr.trim();
        target.trim();
        int brightness = constrain(valueStr.toInt(), 0, 255);
        
        extern uint8_t boardNameBrightness;
        extern uint8_t perimeterBrightness;
        
        if(target.equalsIgnoreCase("PER") || target.equalsIgnoreCase("PERIMETER")){
          perimeterBrightness = brightness;
          Serial.printf("Perimeter brightness set to %d\n", brightness);
          return;
        } else if(target.equalsIgnoreCase("BD") || target.equalsIgnoreCase("BOARDNAME")){
          boardNameBrightness = brightness;
          Serial.printf("BoardName brightness set to %d\n", brightness);
          return;
        }
      }
    }
    // Invalid SET BRITE command
    String response = "INVALID COMMAND";
    Serial.println(response);
    extSerial.println(response);
    return;
  }
  
  // Check for star BEFORE removing it - look for * anywhere in the string
  bool hasStar = (rest.length() > 0 && rest.indexOf('*') >= 0);
  bool hasPlus = (rest.length() > 0 && rest.indexOf('+') >= 0);
  
  // Remove star and plus, then get the numeric value
  rest.replace("*",""); 
  rest.replace("+","");
  rest.trim();
  int val=constrain(rest.toInt(),0,PIXELS_PER_STRIP);

  bool known=false;
  for(int i=0;i<7;i++){
    if(label.equalsIgnoreCase(labels[i])){
      bool changedRainbow = false;
      int wavePosSnapshot = 0;
      float currentSnapshot = 0.0f;

      portENTER_CRITICAL(&stateMux);
      bool wasRainbow = rainbow[i];
      int totalPixels = getLitLedsForValue(i, val);
      targetLeds[i] = totalPixels;
      multiColor[i] = hasPlus;  // Set multi-color mode if '+' is present
      // If multi-color mode is enabled, disable rainbow
      // If rainbow is enabled (hasStar), disable multi-color
      if (hasPlus && hasStar) {
        // Both + and * specified - prioritize rainbow (star)
        multiColor[i] = false;
        rainbow[i] = true;
      } else {
        multiColor[i] = hasPlus;
        rainbow[i] = hasStar;  // Explicitly set based on star presence - MUST be false if no star
      }
      if (rainbow[i] != wasRainbow) {
        rainbowWavePos[i] = 0;
        rainbowWaveMode[i] = rainbow[i] ? WAVE_ENTERING : WAVE_EXITING;
        changedRainbow = true;
        wavePosSnapshot = rainbowWavePos[i];
      }
      currentSnapshot = currentLeds[i];
      portEXIT_CRITICAL(&stateMux);

      Serial.printf("Floor %s: hasStar=%d, rainbow[%d]=%d, wasRainbow=%d\n",
                    labels[i], hasStar, i, rainbow[i], wasRainbow);
      if (changedRainbow) {
        Serial.printf("Wave pos reset: rainbowWavePos[%d]=%d (rainbow=%d, currentLeds=%f)\n",
                      i, wavePosSnapshot, rainbow[i], currentSnapshot);
      }

      String v=String(labels[i])+"_val";
      String r=String(labels[i])+"_r";
      saveState(v.c_str(),r.c_str(),val,hasStar);
      known=true;
      break;
    }
  }

  if(label.equalsIgnoreCase("EASTER_EGG")){
    easterEgg=rest.equalsIgnoreCase("ON");
    Serial.printf("🥁 Easter Egg %s\n",easterEgg?"ON":"OFF");
    extSerial.printf("EASTER_EGG %s\n",easterEgg?"ON":"OFF");
    known=true;
  }

  // BoardName commands: BOARDNAME <pattern>
  if(label.equalsIgnoreCase("BOARDNAME")){
    handleBoardNameCommand(rest);
    known=true;
  }

  // Perimeter commands: PER <pattern> [color1] [color2]
  // Patterns: OFF, SOLID, PER ALT, RAINBOW
  // Also support PERIMETER for backwards compatibility
  if(label.equalsIgnoreCase("PER") || label.equalsIgnoreCase("PERIMETER")){
    if(rest.equalsIgnoreCase("OFF")){
      perimeterPattern = PER_OFF;
      known=true;
    } else if(rest.startsWith("SOLID")){
      perimeterPattern = PER_SOLID;
      // Parse color: SOLID <COLOR_NAME>
      int sp1 = rest.indexOf(' ', 5);
      if(sp1 > 0){
        String colorName = rest.substring(sp1+1);
        colorName.trim();
        CRGB color;
        if(parseColorName(colorName, color)){
          perimeterColor1 = color;
          known=true;
        } else {
          known=false;  // Invalid color
        }
      } else {
        known=true;  // SOLID without color is valid (uses default)
      }
    } else if(rest.startsWith("ALT")){
      perimeterPattern = PER_ALTERNATING;
      // Parse: ALT <COLOR1> <COLOR2> [CHASE [delay_ms]]
      int sp1 = rest.indexOf(' ', 3);
      if(sp1 > 0){
        int sp2 = rest.indexOf(' ', sp1+1);
        if(sp2 > 0){
          String color1Name = rest.substring(sp1+1, sp2);
          String remaining = rest.substring(sp2+1);
          remaining.trim();
          
          // Check for CHASE flag
          bool hasChase = remaining.indexOf("CHASE") >= 0;
          uint32_t chaseDelay = 500;  // Default 500ms
          
          if(hasChase){
            // Find CHASE position
            int chaseIdx = remaining.indexOf("CHASE");
            String beforeChase = remaining.substring(0, chaseIdx);
            beforeChase.trim();
            String afterChase = remaining.substring(chaseIdx + 5);
            afterChase.trim();
            
            // Color2 is before CHASE
            String color2Name = beforeChase;
            
            // Check if there's a delay value after CHASE
            if(afterChase.length() > 0){
              int delayValue = afterChase.toInt();
              if(delayValue > 0){
                chaseDelay = delayValue;
              } else {
                known=false;  // Invalid delay value
              }
            }
            
            color1Name.trim();
            color2Name.trim();
            
            CRGB color1, color2;
            if(parseColorName(color1Name, color1) && parseColorName(color2Name, color2)){
              perimeterColor1 = color1;
              perimeterColor2 = color2;
              extern bool perimeterAltChase;
              extern uint32_t perimeterChaseDelay;
              extern uint32_t perimeterChaseLastUpdate;
              perimeterAltChase = true;
              perimeterChaseDelay = chaseDelay;
              perimeterChaseLastUpdate = millis();
              known=true;
            } else {
              known=false;  // Invalid color(s)
            }
          } else {
            // No CHASE, just two colors
            color1Name.trim();
            remaining.trim();
            
            CRGB color1, color2;
            if(parseColorName(color1Name, color1) && parseColorName(remaining, color2)){
              perimeterColor1 = color1;
              perimeterColor2 = color2;
              extern bool perimeterAltChase;
              perimeterAltChase = false;
              known=true;
            } else {
              known=false;  // Invalid color(s)
            }
          }
        } else {
          known=false;  // Missing second color
        }
      } else {
        known=false;  // Missing colors
      }
    } else if(rest.equalsIgnoreCase("RAINBOW") || rest.equalsIgnoreCase("CHASING_RAINBOW")){
      // Support both RAINBOW and CHASING_RAINBOW for backwards compatibility
      perimeterPattern = PER_RAINBOW;
      known=true;
    } else {
      known=false;  // Unknown perimeter pattern
    }
    if(known) {
      Serial.printf("Perimeter pattern: %d\n", perimeterPattern);
    }
  }

  if(known) {
    Serial.printf("CMD: %s %s\n",label.c_str(),rest.c_str());
  } else {
    String response = "INVALID COMMAND";
    Serial.println(response);
    extSerial.println(response);
  }
}

