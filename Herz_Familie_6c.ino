/* Witnessmenow's ESP8266 Wemo Emulator library is an awesome way to build an IoT device on
   the cheap, and its direct integration with Alexa / Echo means that voice control can
   be done with natural sounding commands. However, it is limited by its reliance solely on
   voice control, as it doesnt work with the official Wemo app. It also provided no affordance
   for toggling the switch when an internet connection was unavailable.

   With just a bit of additional code, devices can be made controllable by the Blynk app, hardware
   switches, IFTTT event triggers,and of course, Alexa. Toss in OTA updates and Tzapulica's
   WiFiManager for easy provisioning, and you've got a really versatile, easy to use device.


   OTA updates are hardware dependent, but don't seem to cause any problems for devices
   that don't support it.

   Wemo Emulator and WiFi Manager libraries:
   https://github.com/witnessmenow/esp8266-alexa-wemo-emulator
   https://github.com/tzapu/WiFiManager

   In order to control a multitude of devices with a single Blynk dashboard, each ESP8266
   should be programmed with a unique virtual pin assignment, corresponding to a Blynk switch.

   The onboard LED is set to ON when the relay is off. This made sense to me, if you're looking
   for the physical switch in a dark room.

   For IFTTT control, use the Maker Channel with the following settings:
      URL: http://blynk-cloud.com:8080/YOUR_TOKEN/V1       Substitute your own token and vitual pin
      Method: PUT
      Content type: application/json
      Body: ["1"]                                          Use 1 for ON, 0 for OFF
*/

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
//#include <BlynkSimpleEsp8266.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//#include <SimpleTimer.h>
#include <Adafruit_NeoPixel.h>
#include <FastLED.h>

#include "WemoSwitch.h"
#include "WemoManager.h"
#include "CallbackFunction.h"

#define APNAME "Herz_Familie"
//#define VPIN V1  //Use a unique virtual pin for each device using the same token / dashboard
#define LEDPIN D4 //Change pins according to your NodeMCU/ESP pinouts
#define BRIGHTNESS  255
#define FRAMES_PER_SECOND 60
bool gReverseDirection;
#define COLOR_ORDER GRB
#define CHIPSET WS2812B
#define NUM_LEDS 25
#define HOWMANYSTRIPS 5
//#define WARMWHITE 0xD2691E
//#define WARMWHITE 210,105,30
#define WARMWHITE 255,127,36
#define DARKWARMWHITE 14,7,2
#define BLACK 0,0,0
#define MAXEFFECTS 6 // how many effects you have?
#define STARTEFFECT 2 // standard effect after first start
CRGB leds[NUM_LEDS];
CRGBPalette16 gPal;
int lasteffect;
int i;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LEDPIN, NEO_GRB + NEO_KHZ800);
String effect;
int glow_r = 255;
int glow_g = 96;
int glow_b = 12;
int COOLING = 55, SPARKING = 120;
uint16_t rainbow_k = 0, rainbow_j = 0;
int addreeprom = 0; // eeprom
int valueeeprom; // eeprom

//char auth[] = "YOUR_TOKEN"; //Get token from Blynk

//on/off callbacks
//void lightOn();
//void lightOff();
void alexa_fire_on();
void alexa_glow_on();
void alexa_white_on();
void alexa_rainbow_on();
void alexa_glitter_on();
void alexa_ice_on();
void alexa_fire_off();
void alexa_glow_off();
void alexa_white_off();
void alexa_rainbow_off();
void alexa_glitter_off();
void alexa_ice_off();

WemoManager wemoManager;
//WemoSwitch *light = NULL;
WemoSwitch *alexa_fire = NULL;
WemoSwitch *alexa_glow = NULL;
WemoSwitch *alexa_white = NULL;
WemoSwitch *alexa_rainbow = NULL;
WemoSwitch *alexa_glitter = NULL;
WemoSwitch *alexa_ice = NULL;

//boolean LampState = 0;
//boolean SwitchReset = true;   //Flag indicating that the hardware button has been released

//const int TacSwitch = 0;      //Pin for hardware momentary switch. On when grounded. Pin 0 on Sonoff
//const int RelayPin = 12;      //Relay switching pin. Relay is pin 12 on the SonOff
//const int LED = 13;           //On / Off indicator LED. Onboard LED is 13 on Sonoff

//SimpleTimer timer;
bool firststart = true;

void setup() {
  //  pinMode(BUILTIN_LED, OUTPUT);  // initialize onboard LED as output
  EEPROM.begin(512);
  Serial.begin(115200);
  FastLED.addLeds<CHIPSET, LEDPIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  strip.begin();
  FastLED.showColor(CRGB(50, 0, 0)); // all LEDs dark red to show: I'm ready, contact with me
  strip.show();
  strip.setBrightness(BRIGHTNESS);

  Serial.print("WiFiManager begin... I'm ready, connect to me at: ");
  Serial.println(APNAME);
  WiFiManager wifi;   //WiFiManager intialization.
  wifi.autoConnect(APNAME); //Create AP, if necessary

  Serial.println("wemoManager begin...");
  wemoManager.begin();
  // Format: Alexa invocation name, local port no, on callback, off callback
  //  light = new WemoSwitch("SonOfWemo", 80, lightOn, lightOff);
  //  wemoManager.addDevice(*light);

  alexa_fire    = new WemoSwitch("Herz Familie Feuer",       81, alexa_fire_on, alexa_fire_off);
  alexa_glow    = new WemoSwitch("Herz Familie Glühen",      82, alexa_glow_on, alexa_glow_off);
  alexa_white   = new WemoSwitch("Herz Familie Weiß",        83, alexa_white_on, alexa_white_off);
  alexa_rainbow = new WemoSwitch("Herz Familie Regenbogen",  84, alexa_rainbow_on, alexa_rainbow_off);
  alexa_glitter = new WemoSwitch("Herz Familie Glitzer",     85, alexa_glitter_on, alexa_glitter_off);
  alexa_ice     = new WemoSwitch("Herz Familie Eis",         86, alexa_ice_on, alexa_ice_off);
  wemoManager.addDevice(*alexa_fire);
  wemoManager.addDevice(*alexa_glow);
  wemoManager.addDevice(*alexa_white);
  wemoManager.addDevice(*alexa_rainbow);
  wemoManager.addDevice(*alexa_glitter);
  wemoManager.addDevice(*alexa_ice);


  //  pinMode(RelayPin, OUTPUT);
  //  pinMode(LED, OUTPUT);
  //  pinMode(TacSwitch, INPUT_PULLUP);
  delay(10);
  //  digitalWrite(RelayPin, LOW);
  //  digitalWrite(LED, LOW);


  //  Blynk.config(auth);
  Serial.println("ArduinoOTA begin...");
  ArduinoOTA.begin();
  FastLED.showColor(CRGB(0, 50, 0)); // all LEDs dark green to show: I'm connected to your WiFi, let's go...
  strip.show();
  Serial.println("I'm connected to your WiFi, let's go...");
  //  digitalWrite(BUILTIN_LED, LOW);   // turn off LED with voltage LOW
  delay(1000); // 1s green then use alexa...
  //  timer.setInterval(100, ButtonCheck);
}

void loop() {
  wemoManager.serverLoop();
  // Blynk.run();
  ArduinoOTA.handle();
  // timer.run();
  if ( firststart ) {
    valueeeprom = eepromReadInt();
    Serial.print("First start! Stored value in eeprom to use last effect: ");
    Serial.println(valueeeprom);
    if ( ( valueeeprom < 0 ) || ( valueeeprom > MAXEFFECTS ) ) {
      valueeeprom = STARTEFFECT; // Standard wenn nichts gespeichert: glühen
      Serial.print("Changed to: ");
      Serial.println(valueeeprom);
    }
    lasteffect = valueeeprom;
    firststart = false;
  }
  if ( lasteffect != 0 ) {
    switchLED(lasteffect, true);
  }
}

int eepromReadInt() { // http://shelvin.de/eine-integer-zahl-in-das-arduiono-eeprom-schreiben/
  byte low, high;
  low = EEPROM.read(addreeprom);
  high = EEPROM.read(addreeprom + 1);
  int newvalue = low + ((high << 8) & 0xFF00);
//  Serial.print("Read used effect: ");
//  Serial.println(newvalue);
  return newvalue;
} //eepromReadInt

void eepromWriteInt(int wert) { // http://shelvin.de/eine-integer-zahl-in-das-arduiono-eeprom-schreiben/
//  Serial.print("Store used effect: ");
//  Serial.print(wert);
  byte low, high;
  low = wert & 0xFF;
  high = (wert >> 8) & 0xFF;
  EEPROM.write(addreeprom, low); // dauert 3,3ms
  EEPROM.write(addreeprom + 1, high);
//  Serial.print(", reread Value: ");
//  Serial.println(eepromReadInt());
  return;
} //eepromWriteInt

/* void savetoeeprom_old(byte valueeeprom) {
  EEPROM.write(addreeprom, valueeeprom);
  Serial.print("Save used effect to eeprom: ");
  Serial.print(valueeeprom);
  valueeeprom = EEPROM.read(addreeprom);
  Serial.print(", and read it back from eeprom: ");
  Serial.println(valueeeprom, DEC);
}
*/


void savetoeeprom(int valueeeprom) {
  Serial.print("Save used effect to eeprom: ");
  Serial.print(valueeeprom);
  eepromWriteInt(valueeeprom);
  Serial.print(", and read it back from eeprom: ");
  Serial.println(eepromReadInt());
}

void alexa_fire_on() {
  savetoeeprom(1);
  Serial.println("Effekt Feuer");
  gReverseDirection = true;
  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  // Default 55, suggested range 20-100
  COOLING = 40;

  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  // Default 120, suggested range 50-200.
  SPARKING = 150;
  effect = "fire";
  gPal = HeatColors_p;
  lasteffect = 1;
  switchLED(1, true);
}
void alexa_glow_on() {
  savetoeeprom(2);
  Serial.println("Effekt Glühen");
  lasteffect = 2;
  switchLED(2, true);
}
void alexa_white_on() {
  savetoeeprom(3);
  Serial.println("Effekt Weiß");
  lasteffect = 3;
  switchLED(3, true);
}
void alexa_rainbow_on() {
  savetoeeprom(4);
  Serial.println("Effekt Regenbogen");
  lasteffect = 4;
  switchLED(4, true);
}
void alexa_glitter_on() {
  savetoeeprom(5);
  Serial.println("Effekt Glitzer");
  lasteffect = 5;
  switchLED(5, true);
}
void alexa_ice_on() {
  savetoeeprom(6);
  Serial.println("Effekt Eis");
  // These are other ways to set up the color palette for the 'fire'.
  // First, a gradient from black to red to yellow to white -- similar to HeatColors_p
  //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);

  // Second, this palette is like the heat colors, but blue/aqua instead of red/yellow
  //   gPal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);

  // Third, here's a simpler, three-step gradient, from black to red to white
  //   gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);
  gReverseDirection = false;
  // COOLING: How much does the air cool as it rises?
  // Less cooling = taller flames.  More cooling = shorter flames.
  // Default 55, suggested range 20-100
  COOLING = 20;

  // SPARKING: What chance (out of 255) is there that a new spark will be lit?
  // Higher chance = more roaring fire.  Lower chance = more flickery fire.
  // Default 120, suggested range 50-200.
  SPARKING = 300;
  effect = "ice";
  gPal = CRGBPalette16( CRGB::Aquamarine, CRGB::Blue, CRGB::Aqua,  CRGB::White);
  lasteffect = 6;
  switchLED(6, true);
}
void alexa_fire_off() {
  savetoeeprom(0);
  switchLED(1, false);
}
void alexa_glow_off() {
  savetoeeprom(0);
  switchLED(2, false);
}
void alexa_white_off() {
  savetoeeprom(0);
  switchLED(3, false);
}
void alexa_rainbow_off() {
  savetoeeprom(0);
  switchLED(4, false);
}
void alexa_glitter_off() {
  savetoeeprom(0);
  switchLED(5, false);
}
void alexa_ice_off() {
  savetoeeprom(0);
  switchLED(6, false);
}

void switchLED(int ledeffect, bool onoff) {
  if ( onoff == false ) {
    allOff();
    lasteffect = 0;
  } else {
    switch (ledeffect) {
      case 1:
        Fire2012WithPalette();
        break;
      case 2:
        glow();
        break;
      case 3:
        colorWipe(strip.Color(WARMWHITE), 50); // Weiß
        break;
      case 4:
        //      rainbow(20); // Regenbogen
        rainbowCycle(20); // Regenbogen
        //      theaterChaseRainbow(20);
        break;
      case 5:
        SnowSparkle(DARKWARMWHITE , random(10, 20), random(10, 200)); // Glitzer
        break;
      case 6:
        Fire2012WithPalette();
        break;
    }
  }
}

void setAll(byte red, byte green, byte blue) { // Set all LEDs to a given color and apply it (visible)
  for (int j = 0; j < NUM_LEDS; j++ )
    strip.setPixelColor(j, strip.Color(red, green, blue));
  strip.show();
}

void SnowSparkle(byte red, byte green, byte blue, int SparkleDelay, int SpeedDelay) {
  setAll(red, green, blue);
  int num = random(4, 10);
  for ( int j = 0; j <= num; j++ ) {
    int Pixel = random(NUM_LEDS);
    strip.setPixelColor(Pixel, strip.Color(0xff, 0xff, 0xff));
    strip.show();
    delay(SparkleDelay);
    strip.setPixelColor(Pixel, strip.Color(red, green, blue));
    strip.show();
  }
  delay(SpeedDelay);
}

uint32_t Wheel(byte WheelPos) {
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/* void theaterChase(uint32_t c, uint8_t wait) {
  //Theatre-style crawling lights
  for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, c);  //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
  }
*/

/* void theaterChaseRainbow(uint8_t wait) {
  //Theatre-style crawling lights with rainbow effect
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
  }
*/

void colorWipesegment(int startseg, int lastseg, uint32_t c, uint8_t wait) {
  // Fill the dots one after the other with a color
  for (uint16_t i = startseg; i < lastseg; i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void colorWipe(uint32_t c, uint8_t wait) {
  // Fill the dots one after the other with a color
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void allOff() {
  //Turns all pixels OFF
  for ( int i = 0; i < NUM_LEDS; i++ ) {
    strip.setPixelColor(i, 0, 0, 0 );
    leds[i] = CRGB::Black;
  }
  strip.show();
  FastLED.show();
}

/* void fadeall() {
  for ( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i].nscale8(250);
  }
  FastLED.show();
  }
*/

/* void rainbow(uint8_t wait) {
  //Rainbow pattern
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait);
  }
  }
*/

void rainbowCycle(uint8_t wait) {
  if ( rainbow_j > 255 )
    rainbow_j = 0;
  else
    rainbow_j++;

  if ( rainbow_k > strip.numPixels() )
    rainbow_k = 0;
  else
    rainbow_k++;

  strip.setPixelColor(rainbow_k, Wheel(((rainbow_k * 256 / strip.numPixels()) + rainbow_j) & 255));
  strip.show();
  delay(wait);
}

void rainbowCycle_old(uint8_t wait) {
  //Rainbow equally distributed throughout
  uint16_t k, j;

  for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
    for (k = 0; k < strip.numPixels(); k++) {
      strip.setPixelColor(k, Wheel(((k * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void Fire2012WithPalette() {
  /*
    // Fire2012 by Mark Kriegsman, July 2012
    // as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
    ////
    // This basic one-dimensional 'fire' simulation works roughly as follows:
    // There's a underlying array of 'heat' cells, that model the temperature
    // at each point along the line.  Every cycle through the simulation,
    // four steps are performed:
    //  1) All cells cool down a little bit, losing heat to the air
    //  2) The heat from each cell drifts 'up' and diffuses a little
    //  3) Sometimes randomly new 'sparks' of heat are added at the bottom
    //  4) The heat from each cell is rendered as a color into the leds array
    //     The heat-to-color mapping uses a black-body radiation approximation.
    //
    // Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
    //
    // This simulation scales it self a bit depending on NUM_LEDS; it should look
    // "OK" on anywhere from 20 to 100 LEDs without too much tweaking.
    //
    // I recommend running this simulation at anywhere from 30-100 frames per second,
    // meaning an interframe delay of about 10-35 milliseconds.
    //
    // Looks best on a high-density LED setup (60+ pixels/meter).
    //
    //
    // There are two main parameters you can play with to control the look and
    // feel of your fire: COOLING (used in step 1 above), and SPARKING (used
    // in step 3 above).
  */
  // Array of temperature readings at each simulation cell
  const int numleds = NUM_LEDS / HOWMANYSTRIPS;
  static byte heat[numleds];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < numleds; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / numleds) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = numleds - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < numleds; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (numleds - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[ pixelnumber ] = color;
    leds[ pixelnumber + 9 ] = color;
    leds[ pixelnumber + 18 ] = color;
    leds[ pixelnumber + 27 ] = color;
  }
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

void glow() {
  //www.walltech.cc/neopixel-fire-effect/

  for (int x = 0; x < NUM_LEDS; x++) {
    //    int flicker = random(0, 40);
    int flicker = random(40, 90);
    int glow_r1 = glow_r - flicker;
    int glow_g1 = glow_g - flicker;
    int glow_b1 = glow_b - flicker;
    if (glow_g1 < 0) glow_g1 = 0;
    if (glow_r1 < 0) glow_r1 = 0;
    if (glow_b1 < 0) glow_b1 = 0;
    strip.setPixelColor(x, glow_r1, glow_g1, glow_b1);
    //    leds[x] = CRGB(glow_r1, glow_g1, glow_b1); // stürzt ab
    //    leds[x].nscale8(random(50, 150)); // stürzt ab
  }
  //  FastLED.show();
  strip.show();
  delay(random(50, 150));
}

/* void lightOn() { // Toggle the relay on
    Serial.println("Switch 1 turn on ...");
  //    digitalWrite(RelayPin, HIGH);
  //    digitalWrite(LED, HIGH);
    LampState = 1;
  //    Blynk.virtualWrite(VPIN, HIGH);     // Sync the Blynk button widget state
  }

  // Toggle the relay off
  void lightOff() {
    Serial.println("Switch 1 turn off ...");
  //    digitalWrite(RelayPin, LOW);
  //    digitalWrite(LED, LOW);
    LampState = 0;
  //    Blynk.virtualWrite(VPIN, LOW);      // Sync the Blynk button widget state
  }
*/

/* BLYNK_WRITE(VPIN){ // Handle switch changes originating on the Blynk app
  int SwitchStatus = param.asInt();

  Serial.println("Blynk switch activated");

  // For use with IFTTT, toggle the relay by sending a "2"
  if (SwitchStatus == 2){
    ToggleRelay();
  }
  else if (SwitchStatus){
    lightOn();
  }
  else lightOff();
  }
*/

/* void ButtonCheck(){ // Handle hardware switch activation
  // look for new button press
  boolean SwitchState = (digitalRead(TacSwitch));

  // toggle the switch if there's a new button press
  if (!SwitchState && SwitchReset == true){
    Serial.println("Hardware switch activated");
    if (LampState){
      lightOff();
    }
    else{
      lightOn();
    }

    // Flag that indicates the physical button hasn't been released
    SwitchReset = false;
    delay(50);            //debounce
  }
  else if (SwitchState){
    // reset flag the physical button release
    SwitchReset = true;
  }
  }
*/

/* void ToggleRelay(){
  LampState = !LampState;

  if (LampState){
    lightOn();
  }
  else lightOff();
  }
*/
