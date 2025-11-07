#include <FastLED.h>
#include <Preferences.h>

// =====================
// --- CONFIG ---
// =====================
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      128

#define NUM_LEDS_PAIR   288
#define NUM_LEDS_SINGLE 144

#define PIN_F11_12   1
#define PIN_F15_16   2
#define PIN_F17_POOL 3
#define PIN_TEA      4

#define UART_RX 44
#define UART_TX 43

// =====================
// --- ARRAYS ---
// =====================
CRGB leds_f11_12[NUM_LEDS_PAIR];
CRGB leds_f15_16[NUM_LEDS_PAIR];
CRGB leds_f17_pool[NUM_LEDS_PAIR];
CRGB leds_tea[NUM_LEDS_SINGLE];

// =====================
// --- CONSTANTS ---
// =====================
#define TITLE_LEDS 20
#define TITLE_GAP  4
#define GAP_LEDS   4
#define PIXELS_PER_STRIP 6

const int LEDS_PER_PIXEL =
  (NUM_LEDS_SINGLE - TITLE_LEDS - TITLE_GAP - (GAP_LEDS * (PIXELS_PER_STRIP - 1))) / PIXELS_PER_STRIP;

#define HALF_A_START 0
#define HALF_A_END   143
#define HALF_B_START 144
#define HALF_B_END   287

// =====================
// --- STATE ---
// =====================
float currentLeds[7];
int   targetLeds[7];
bool  rainbow[7];
bool  easterEgg = false;
uint8_t hueOffset = 0;

// label mapping
enum Floors {F11, F12, F15, F16, F17, POOL, TEA};
const char* labels[7] = {"FLOOR11","FLOOR12","FLOOR15","FLOOR16","FLOOR17","POOL","TEA_ROOM"};

CRGB titleColor[7] = {CRGB::Purple,CRGB::Cyan,CRGB::Orange,CRGB::Green,CRGB::Blue,CRGB::HotPink,CRGB::Yellow};
CRGB pixelColor[7] = {CRGB::Purple,CRGB::Cyan,CRGB::Orange,CRGB::Green,CRGB::Blue,CRGB::HotPink,CRGB::Yellow};

// =====================
// --- GLOBALS ---
// =====================
Preferences prefs;
HardwareSerial extSerial(1);
float animSpeed = 1.0f;  // LEDs per frame

// =====================
// --- SETUP ---
// =====================
void setup() {
  Serial.begin(115200);
  extSerial.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  prefs.begin("floors", false);

  FastLED.addLeds<LED_TYPE, PIN_F11_12, COLOR_ORDER>(leds_f11_12, NUM_LEDS_PAIR);
  FastLED.addLeds<LED_TYPE, PIN_F15_16, COLOR_ORDER>(leds_f15_16, NUM_LEDS_PAIR);
  FastLED.addLeds<LED_TYPE, PIN_F17_POOL, COLOR_ORDER>(leds_f17_pool, NUM_LEDS_PAIR);
  FastLED.addLeds<LED_TYPE, PIN_TEA, COLOR_ORDER>(leds_tea, NUM_LEDS_SINGLE);
  FastLED.setBrightness(BRIGHTNESS);

  // Restore
  for (int i=0;i<7;i++) {
    String v=String(labels[i])+"_val";
    String r=String(labels[i])+"_r";
    int saved = prefs.getInt(v.c_str(),0);
    bool rb = prefs.getBool(r.c_str(),false);
    targetLeds[i]=saved*LEDS_PER_PIXEL;
    currentLeds[i]=targetLeds[i];
    rainbow[i]=rb;
  }

  Serial.println("✅ Ready with per-LED smooth animation.");
}

// =====================
// --- LOOP ---
// =====================
void loop() {
  if (Serial.available()) handleInput(Serial.readStringUntil('\n'));
  if (extSerial.available()) handleInput(extSerial.readStringUntil('\n'));

  for (int i=0;i<7;i++) animateProgress(currentLeds[i], targetLeds[i], animSpeed);

  drawFloor(leds_f11_12, HALF_A_START, HALF_A_END, currentLeds[F11], titleColor[F11], pixelColor[F11], rainbow[F11], false);
  drawFloor(leds_f11_12, HALF_B_START, HALF_B_END, currentLeds[F12], titleColor[F12], pixelColor[F12], rainbow[F12], true);

  drawFloor(leds_f15_16, HALF_A_START, HALF_A_END, currentLeds[F15], titleColor[F15], pixelColor[F15], rainbow[F15], false);
  drawFloor(leds_f15_16, HALF_B_START, HALF_B_END, currentLeds[F16], titleColor[F16], pixelColor[F16], rainbow[F16], true);

  drawFloor(leds_f17_pool, HALF_A_START, HALF_A_END, currentLeds[F17], titleColor[F17], pixelColor[F17], rainbow[F17], false);
  drawFloor(leds_f17_pool, HALF_B_START, HALF_B_END, currentLeds[POOL], titleColor[POOL], pixelColor[POOL], rainbow[POOL], true);

  drawStrip(leds_tea, currentLeds[TEA], titleColor[TEA], pixelColor[TEA], rainbow[TEA]);

  FastLED.show();
  hueOffset+=2;
  delay(15);
}

// =====================
// --- FUNCTIONS ---
// =====================

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
// --- PARSER ---
// =====================
void handleInput(String msg){
  msg.trim();
  if(msg.isEmpty()) return;

  int sp=msg.indexOf(' ');
  if(sp==-1) sp=msg.length();
  String label=msg.substring(0,sp);
  String rest=(sp<(int)msg.length())?msg.substring(sp+1):"";
  rest.trim();
  bool hasStar=rest.endsWith("*");
  rest.replace("*",""); rest.trim();
  int val=constrain(rest.toInt(),0,PIXELS_PER_STRIP);

  bool known=false;
  for(int i=0;i<7;i++){
    if(label.equalsIgnoreCase(labels[i])){
      targetLeds[i] = val * LEDS_PER_PIXEL;
      rainbow[i]=hasStar;
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

  if(known)
    Serial.printf("CMD: %s %s\n",label.c_str(),rest.c_str());
  else
    Serial.println("Unknown command");
}

// =====================
// --- DRAW FUNCTIONS ---
// =====================
void drawFloor(CRGB* leds,int start,int end,float ledsLit,
               CRGB titleColor,CRGB staticColor,bool rainbow,bool reverse) {
  fill_solid(&leds[start], end - start + 1, CRGB::Black);

  // Title zone
  for (int i=0;i<TITLE_LEDS && start+i<=end;i++)
    leds[reverse?end-i:start+i] = titleColor;

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

  int barStart = start + TITLE_LEDS + TITLE_GAP;

  // Draw by segments
  for (int seg = 0; seg < PIXELS_PER_STRIP; seg++) {
    float segProgress = ledsLit - (seg * LEDS_PER_PIXEL);
    if (segProgress <= 0) break;                     // nothing more to fill
    int ledsToLight = min(LEDS_PER_PIXEL, (int)segProgress);
    if (segProgress > LEDS_PER_PIXEL) ledsToLight = LEDS_PER_PIXEL;

    int segStart = barStart + seg * (LEDS_PER_PIXEL + GAP_LEDS);
    for (int j=0;j<ledsToLight;j++) {
      int idx = reverse ? (end - (segStart - start + j)) : (segStart + j);
      if (idx < start || idx > end) continue;
      if (rainbow)
        leds[idx] = CHSV((255 - hueOffset + (seg * 40) + (j * 2)) & 255, 255, beatBrightness);
      else
        leds[idx] = staticColor;
    }
  }
}

void drawStrip(CRGB* leds,float ledsLit,CRGB titleColor,CRGB staticColor,bool rainbow) {
  fill_solid(leds,NUM_LEDS_SINGLE,CRGB::Black);
  for (int i=0;i<TITLE_LEDS && i<NUM_LEDS_SINGLE;i++)
    leds[i]=titleColor;

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

  int barStart = TITLE_LEDS + TITLE_GAP;

  for (int seg = 0; seg < PIXELS_PER_STRIP; seg++) {
    float segProgress = ledsLit - (seg * LEDS_PER_PIXEL);
    if (segProgress <= 0) break;
    int ledsToLight = min(LEDS_PER_PIXEL, (int)segProgress);
    if (segProgress > LEDS_PER_PIXEL) ledsToLight = LEDS_PER_PIXEL;

    int segStart = barStart + seg * (LEDS_PER_PIXEL + GAP_LEDS);
    for (int j=0;j<ledsToLight;j++) {
      int idx = segStart + j;
      if (idx >= NUM_LEDS_SINGLE) break;
      if (rainbow)
        leds[idx] = CHSV((255 - hueOffset + (seg * 40) + (j * 2)) & 255, 255, beatBrightness);
      else
        leds[idx] = staticColor;
    }
  }
}
