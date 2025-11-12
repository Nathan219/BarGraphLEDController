#include "UART.h"
#include "BoardName.h"
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
extern const char* labels[7];

// Forward declarations for types and constants
struct PixelDef {
  int leftCol;
  int rightCol;
};
extern PixelDef floorPixels[];
#define PIXELS_PER_STRIP 6

enum RainbowWaveMode : uint8_t {
  WAVE_IDLE = 0,
  WAVE_ENTERING = 1,
  WAVE_EXITING = 2
};

enum PerimeterPattern : uint8_t {
  PER_OFF = 0,
  PER_SOLID = 1,
  PER_ALTERNATING = 2,
  PER_CHASING_RAINBOW = 3
};

// CRGBW struct is defined in LEDController.ino
// Forward declare it here
struct CRGBW {
  uint8_t r, g, b, w;
  CRGBW();
  CRGBW(uint8_t ir, uint8_t ig, uint8_t ib, uint8_t iw);
};

extern PerimeterPattern perimeterPattern;
extern CRGBW perimeterColor1;
extern CRGBW perimeterColor2;

// UART hardware
#define UART_RX 8
#define UART_TX 7
#define UART_BAUD 38400
HardwareSerial extSerial(1);

// Forward declaration
void saveState(const char* keyV, const char* keyR, int val, bool r);

void initUART() {
  extSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
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
  
  // Check for star BEFORE removing it - look for * anywhere in the string
  bool hasStar = (rest.length() > 0 && rest.indexOf('*') >= 0);
  
  // Remove star and get the numeric value
  rest.replace("*",""); 
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
      // Calculate total pixels based on val (0-6)
      // Each pixel's width is calculated from its boundaries
      int totalPixels = 0;
      for (int p = 0; p < val && p < PIXELS_PER_STRIP; p++) {
        totalPixels += (floorPixels[p].rightCol - floorPixels[p].leftCol + 1);
      }
      targetLeds[i] = totalPixels;
      rainbow[i] = hasStar;  // Explicitly set based on star presence - MUST be false if no star
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

  // Perimeter commands: PERIMETER <pattern> [color1] [color2]
  // Patterns: OFF, SOLID, ALTERNATING, CHASING_RAINBOW
  if(label.equalsIgnoreCase("PERIMETER")){
    if(rest.equalsIgnoreCase("OFF")){
      perimeterPattern = PER_OFF;
    } else if(rest.startsWith("SOLID")){
      perimeterPattern = PER_SOLID;
      // Parse color: SOLID R G B W
      int sp1 = rest.indexOf(' ', 5);
      if(sp1 > 0){
        int sp2 = rest.indexOf(' ', sp1+1);
        int sp3 = rest.indexOf(' ', sp2+1);
        int sp4 = rest.indexOf(' ', sp3+1);
        if(sp2 > 0 && sp3 > 0){
          int r = rest.substring(sp1+1, sp2).toInt();
          int g = rest.substring(sp2+1, sp3).toInt();
          int b = (sp4 > 0) ? rest.substring(sp3+1, sp4).toInt() : rest.substring(sp3+1).toInt();
          int w = (sp4 > 0) ? rest.substring(sp4+1).toInt() : 0;
          perimeterColor1 = CRGBW(r, g, b, w);
        }
      }
    } else if(rest.startsWith("ALTERNATING")){
      perimeterPattern = PER_ALTERNATING;
      // Parse colors: ALTERNATING R1 G1 B1 W1 R2 G2 B2 W2
      int sp1 = rest.indexOf(' ', 11);
      if(sp1 > 0){
        int sp2 = rest.indexOf(' ', sp1+1);
        int sp3 = rest.indexOf(' ', sp2+1);
        int sp4 = rest.indexOf(' ', sp3+1);
        int sp5 = rest.indexOf(' ', sp4+1);
        int sp6 = rest.indexOf(' ', sp5+1);
        int sp7 = rest.indexOf(' ', sp6+1);
        if(sp2 > 0 && sp3 > 0 && sp4 > 0 && sp5 > 0 && sp6 > 0 && sp7 > 0){
          int r1 = rest.substring(sp1+1, sp2).toInt();
          int g1 = rest.substring(sp2+1, sp3).toInt();
          int b1 = rest.substring(sp3+1, sp4).toInt();
          int w1 = rest.substring(sp4+1, sp5).toInt();
          int r2 = rest.substring(sp5+1, sp6).toInt();
          int g2 = rest.substring(sp6+1, sp7).toInt();
          int b2 = rest.substring(sp7+1).toInt();
          int w2 = 0;
          int sp8 = rest.indexOf(' ', sp7+1);
          if(sp8 > 0) w2 = rest.substring(sp8+1).toInt();
          perimeterColor1 = CRGBW(r1, g1, b1, w1);
          perimeterColor2 = CRGBW(r2, g2, b2, w2);
        }
      }
    } else if(rest.equalsIgnoreCase("CHASING_RAINBOW")){
      perimeterPattern = PER_CHASING_RAINBOW;
    }
    Serial.printf("Perimeter pattern: %d\n", perimeterPattern);
    known=true;
  }

  if(known)
    Serial.printf("CMD: %s %s\n",label.c_str(),rest.c_str());
  else
    Serial.println("Unknown command");
}

void uartTask(void* parameter){
  while(true){
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      if (line.length() > 0 && line.length() < 200) {  // Safety check: limit line length
        handleInput(line);
      }
    }
    if (extSerial.available()) {
      String line = extSerial.readStringUntil('\n');
      if (line.length() > 0 && line.length() < 200) {  // Safety check: limit line length
        handleInput(line);
      }
    }
    // Give the idle task a chance to run and reset the watchdog
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

