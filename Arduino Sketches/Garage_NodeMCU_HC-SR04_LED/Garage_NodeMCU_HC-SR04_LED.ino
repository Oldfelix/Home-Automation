#include <ArduinoOTA.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define wifi_ssid "redacted" //enter your WIFI SSID
#define wifi_password "redacted" //enter your WIFI Password
#define mqtt_server "redacted" // Enter your MQTT server adderss or IP. I use my DuckDNS adddress (yourname.duckdns.org) in this field
#define mqtt_user "redacted" //enter your MQTT username
#define mqtt_password "redacted8" //enter your password
const char* ssid = (wifi_ssid); //enter your WIFI SSID for OTA
const char* password = (wifi_password); //enter your WIFI Password for OTA



/************ FastLED Defintions ******************/

#define DATA_PIN    5 //on the NodeMCU 1.0, FastLED will default to the D5 pin after throwing an error during compiling. Leave as is. 
#define LED_TYPE    WS2811 //change to match your LED type
#define COLOR_ORDER BRG //change to match your LED configuration
#define NUM_LEDS    5 //change to match your setup


/****************************** MQTT TOPICS (change these topics as you wish)  ***************************************/
#define colorstatuspub "/house/external/garage/ledstrip/colorstatus"
#define setcolorsub "/house/external/garage/ledstrip/setcolor"
#define setpowersub "/house/external/garage/ledstrip/setpower"
#define seteffectsub "/house/external/garage/ledstrip/seteffect"
#define setbrightness "/house/external/garage/ledstrip/setbrightness"
#define setcolorpub "/house/external/garage/ledstrip/setcolorpub"
#define setpowerpub "/house/external/garage/ledstrip/setpowerpub"
#define seteffectpub "/house/external/garage/ledstrip/seteffectpub"
#define setbrightnesspub "/house/external/garage/ledstrip/setbrightnesspub"
#define setanimationspeed "/house/external/garage/ledstrip/setanimationspeed"
#define ledstatus "/house/external/garage/ledstrip"
//No Changes Required After This Point

/*******************UltraSonic sensor HC-SR04****************/
const int trigPin = 12;
const int echoPin = 13;

/*************************** EFFECT CONTROL VARIABLES AND INITIALIZATIONS ************************************/

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

String setColor ="0,0,150";
String setPower;
String setEffect = "Solid";
String setBrightness = "150";
int brightness = 150;
String setAnimationSpeed;
int animationspeed = 240;
String setColorTemp;
int Rcolor = 0;
int Gcolor = 0;
int Bcolor = 0;
CRGB leds[NUM_LEDS];

/****************FOR CANDY CANE***************/
CRGBPalette16 currentPalettestriped; //for Candy Cane
CRGBPalette16 gPal; //for fire

/****************FOR NOISE***************/
static uint16_t dist;         // A random number for our noise generator.
uint16_t scale = 30;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 48;      // Value for blending between palettes.
CRGBPalette16 targetPalette(OceanColors_p);
CRGBPalette16 currentPalette(CRGB::Black);

/*****************For TWINKER********/
#define DENSITY     80
int twinklecounter = 0;

/*********FOR RIPPLE***********/
uint8_t colour;                                               // Ripple colour is randomized.
int center = 0;                                               // Center of the current ripple.
int step = -1;                                                // -1 is the initializing step.
uint8_t myfade = 255;                                         // Starting brightness.
#define maxsteps 16                                           // Case statement wouldn't allow a variable.
uint8_t bgcol = 0;                                            // Background colour rotates.
int thisdelay = 20;                                           // Standard delay value.

/**************FOR RAINBOW***********/
uint8_t thishue = 0;                                          // Starting hue value.
uint8_t deltahue = 10;

/**************FOR DOTS**************/
uint8_t   count =   0;                                        // Count up to 255 and then reverts to 0
uint8_t fadeval = 224;                                        // Trail behind the LED's. Lower => faster fade.
uint8_t bpm = 30;

/**************FOR LIGHTNING**************/
uint8_t frequency = 50;                                       // controls the interval between strikes
uint8_t flashes = 8;                                          //the upper limit of flashes per strike
unsigned int dimmer = 1;
uint8_t ledstart;                                             // Starting location of a flash
uint8_t ledlen;
int lightningcounter = 0;

/********FOR FUNKBOX EFFECTS**********/
int idex = 0;                //-LED INDEX (0 to NUM_LEDS-1
int TOP_INDEX = int(NUM_LEDS / 2);
int thissat = 255;           //-FX LOOPS DELAY VAR
uint8_t thishuepolice = 0;     
int antipodal_index(int i) {
  int iN = i + TOP_INDEX;
  if (i >= TOP_INDEX) {
    iN = ( i + TOP_INDEX ) % NUM_LEDS;
  }
  return iN;
}

/********FIRE**********/
#define COOLING  55
#define SPARKING 120
bool gReverseDirection = false;

/********BPM**********/
uint8_t gHue = 0;
char message_buff[100];


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(12, 10000); //experimental for power management. Feel free to try in your own setup.
  FastLED.setBrightness(brightness);

  setupStripedPalette( CRGB::Red, CRGB::Red, CRGB::White, CRGB::White); //for CANDY CANE

    gPal = HeatColors_p; //for FIRE

  fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0)); //Startup LED Lights
  FastLED.show();

  setup_wifi();

  client.setServer(mqtt_server, 1883); //CHANGE PORT HERE IF NEEDED
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;

  if (String(topic) == setpowersub) {
    for (i = 0; i < length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
    setPower = String(message_buff);
    Serial.println("Set Power: " + setPower);
    if (setPower == "OFF") {
      client.publish(setpowerpub, "OFF");
    }

    if (setPower == "ON") {
      client.publish(setpowerpub, "ON");
    }
  }


  if (String(topic) == seteffectsub) {
    for (i = 0; i < length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
    setEffect = String(message_buff);
    Serial.println("Set Effect: " + setEffect);
    setPower = "ON";
    client.publish(setpowerpub, "ON");
    if (setEffect == "Twinkle") {
      twinklecounter = 0;
    }
    if (setEffect == "Lightning") {
      twinklecounter = 0;
    }
  }



  if (String(topic) == setbrightness) {
    for (i = 0; i < length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
    setBrightness = String(message_buff);
    Serial.println("Set Brightness: " + setBrightness);
    brightness = setBrightness.toInt();
    setPower = "ON";
    client.publish(setpowerpub, "ON");
  }


//  if (String(topic) == setcolortemp) {    //colortemp setup for future update
//    for (i = 0; i < length; i++) {
//      message_buff[i] = payload[i];
//    }
//    message_buff[i] = '\0';
//    setColorTemp = String(message_buff);
//    Serial.println("Set Color Temperature: " + setColorTemp);
//    setPower = "ON";
//    client.publish(setpowerpub, "ON");
//  }

  if (String(topic) == setcolorsub) {
    for (i = 0; i < length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
    client.publish(setcolorpub, message_buff);
    setColor = String(message_buff);
    Serial.println("Set Color: " + setColor);
    setPower = "ON";
    client.publish(setpowerpub, "ON");
    }


  if (String(topic) == setanimationspeed) {
    for (i = 0; i < length; i++) {
      message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
    setAnimationSpeed = String(message_buff);
    animationspeed = setAnimationSpeed.toInt();
  }
}
  /*****************************OTA Setup*********************/

  void setup_OTA(){
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
    ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
 

 // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  long duration, inches, cm;
 
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
 
  // convert the time into a distance
  inches = microsecondsToInches(duration);
  cm = microsecondsToCentimeters(duration);
 
  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
 
  
  if (inches >= 0 && inches <= 1)  
  {fill_solid( &(leds[0]), NUM_LEDS, CRGB::Red);
  FastLED.show();
  }
  
  if (inches >= 1 && inches <= 2)  
  {fill_solid( &(leds[0]), NUM_LEDS, CRGB::Red);
  FastLED.show();
  }
  
  if (inches >= 3 && inches <= 4)  
  //{fill_solid( &(leds[0]), NUM_LEDS, CRGB::(255,191,0));
  {fill_solid( &(leds[0]), NUM_LEDS, CRGB( 255, 191, 0) );
  FastLED.show();
  }
 
  if (inches >= 4 && inches <= 10)  
  {fill_solid( &(leds[0]), NUM_LEDS, CRGB::Green);
  FastLED.show();
  }

  if (inches >= 11 && inches <= 20)  
  {fill_solid( &(leds[0]), NUM_LEDS, CRGB::Purple);
  FastLED.show();
  }
  
  if (inches >= 21 && inches <= 30)  {fill_solid( &(leds[0]), NUM_LEDS, CRGB::Blue);
  FastLED.show();
  }

  else if (inches >= 31) {fill_solid( &(leds[0]), NUM_LEDS, CRGB::Black);
  FastLED.show();
  }
delay(100);

 


  int Rcolor = setColor.substring(0, setColor.indexOf(',')).toInt();
  int Gcolor = setColor.substring(setColor.indexOf(',') + 1, setColor.lastIndexOf(',')).toInt();
  int Bcolor = setColor.substring(setColor.lastIndexOf(',') + 1).toInt();

  if (setPower == "OFF") {
    setEffect = "Solid";
    for ( int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy( 8 );   //FADE OFF LEDS
    }
  }

  if (setEffect == "Sinelon") {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    int pos = beatsin16(13, 0, NUM_LEDS);
    leds[pos] += CRGB(Rcolor, Gcolor, Bcolor);
  }

  if (setEffect == "Juggle" ) {                           // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy( leds, NUM_LEDS, 20);
    byte dothue = 0;
    for ( int i = 0; i < 8; i++) {
      leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CRGB(Rcolor, Gcolor, Bcolor);
      dothue += 32;
    }
  }

  if (setEffect == "Confetti" ) {                       // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] += CRGB(Rcolor + random8(64), Gcolor, Bcolor);
  }


  if (setEffect == "Rainbow") {
    // FastLED's built-in rainbow generator
    static uint8_t starthue = 0;    thishue++;
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
  }


  if (setEffect == "Rainbow with Glitter") {               // FastLED's built-in rainbow generator with Glitter
    static uint8_t starthue = 0;
    thishue++;
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue);
    addGlitter(80);
  }


  if (setEffect == "Glitter") {
    fadeToBlackBy( leds, NUM_LEDS, 20);
    addGlitterColor(80, Rcolor, Gcolor, Bcolor);
  }


  if (setEffect == "BPM") {                                  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

  if (setEffect == "Solid" & setPower == "ON" ) {          //Fill entire strand with solid color
    fill_solid(leds, NUM_LEDS, CRGB(Rcolor, Gcolor, Bcolor));
  }

  

  if (setEffect == "Twinkle") {
    twinklecounter = twinklecounter + 1;
    if (twinklecounter < 2) {                               //Resets strip if previous animation was running
      FastLED.clear();
      FastLED.show();
    }
    const CRGB lightcolor(8, 7, 1);
    for ( int i = 0; i < NUM_LEDS; i++) {
      if ( !leds[i]) continue; // skip black pixels
      if ( leds[i].r & 1) { // is red odd?
        leds[i] -= lightcolor; // darken if red is odd
      } else {
        leds[i] += lightcolor; // brighten if red is even
      }
    }
    if ( random8() < DENSITY) {
      int j = random16(NUM_LEDS);
      if ( !leds[j] ) leds[j] = lightcolor;
    }
  }

  if (setEffect == "Dots") {
    uint8_t inner = beatsin8(bpm, NUM_LEDS / 4, NUM_LEDS / 4 * 3);
    uint8_t outer = beatsin8(bpm, 0, NUM_LEDS - 1);
    uint8_t middle = beatsin8(bpm, NUM_LEDS / 3, NUM_LEDS / 3 * 2);
    leds[middle] = CRGB::Purple;
    leds[inner] = CRGB::Blue;
    leds[outer] = CRGB::Aqua;
    nscale8(leds, NUM_LEDS, fadeval);
  }

  if (setEffect == "Lightning") {
    twinklecounter = twinklecounter + 1;                     //Resets strip if previous animation was running
    Serial.println(twinklecounter);
    if (twinklecounter < 2) {
      FastLED.clear();
      FastLED.show();
    }
    ledstart = random8(NUM_LEDS);           // Determine starting location of flash
    ledlen = random8(NUM_LEDS - ledstart);  // Determine length of flash (not to go beyond NUM_LEDS-1)
    for (int flashCounter = 0; flashCounter < random8(3, flashes); flashCounter++) {
      if (flashCounter == 0) dimmer = 5;    // the brightness of the leader is scaled down by a factor of 5
      else dimmer = random8(1, 3);          // return strokes are brighter than the leader
      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 255 / dimmer));
      FastLED.show();                       // Show a section of LED's
      delay(random8(4, 10));                // each flash only lasts 4-10 milliseconds
      fill_solid(leds + ledstart, ledlen, CHSV(255, 0, 0)); // Clear the section of LED's
      FastLED.show();
      if (flashCounter == 0) delay (150);   // longer delay until next flash after the leader
      delay(50 + random8(100));             // shorter delay between strokes
    }
    delay(random8(frequency) * 100);        // delay between strikes
  }



  if (setEffect == "Police One") {                    //POLICE LIGHTS (TWO COLOR SINGLE LED)
    idex++;
    if (idex >= NUM_LEDS) {
      idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    int thathue = (thishuepolice + 160) % 255;
    for (int i = 0; i < NUM_LEDS; i++ ) {
      if (i == idexR) {
        leds[i] = CHSV(thishuepolice, thissat, 255);
      }
      else if (i == idexB) {
        leds[i] = CHSV(thathue, thissat, 255);
      }
      else {
        leds[i] = CHSV(0, 0, 0);
      }
    }

  }

  if (setEffect == "Police All") {                 //POLICE LIGHTS (TWO COLOR SOLID)
    idex++;
    if (idex >= NUM_LEDS) {
      idex = 0;
    }
    int idexR = idex;
    int idexB = antipodal_index(idexR);
    int thathue = (thishuepolice + 160) % 255;
    leds[idexR] = CHSV(thishuepolice, thissat, 255);
    leds[idexB] = CHSV(thathue, thissat, 255);
  }


  if (setEffect == "Candy Cane") {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* higher = faster motion */

    fill_palette( leds, NUM_LEDS,
                  startIndex, 16, /* higher = narrower stripes */
                  currentPalettestriped, 255, LINEARBLEND);
  }


  if (setEffect == "Cyclon Rainbow") {                    //Single Dot Down
  static uint8_t hue = 0;
  Serial.print("x");
  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  for(int i = (NUM_LEDS)-1; i >= 0; i--) {
    // Set the i'th led to red 
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}

  if (setEffect == "Fire") { 
      Fire2012WithPalette();
  }
     random16_add_entropy( random8());




  EVERY_N_MILLISECONDS(10) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);  // FOR NOISE ANIMATION
    { gHue++; }


    if (setEffect == "Noise") {
      setPower = "ON";
      for (int i = 0; i < NUM_LEDS; i++) {                                     // Just ONE loop to fill up the LED array as all of the pixels change.
        uint8_t index = inoise8(i * scale, dist + i * scale) % 255;            // Get a value from the noise function. I'm using both x and y axis.
        leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);   // With that value, look up the 8 bit colour palette value and assign it to the current LED.
      }
      dist += beatsin8(10, 1, 4);                                              // Moving along the distance (that random number we started out with). Vary it a bit with a sine wave.
      // In some sketches, I've used millis() instead of an incremented counter. Works a treat.
    }


    if (setEffect == "Ripple") {
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(bgcol++, 255, 15);  // Rotate background colour.
      switch (step) {
        case -1:                                                          // Initialize ripple variables.
          center = random(NUM_LEDS);
          colour = random8();
          step = 0;
          break;
        case 0:
          leds[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
          step ++;
          break;
        case maxsteps:                                                    // At the end of the ripples.
          step = -1;
          break;
        default:                                                             // Middle of the ripples.
          leds[(center + step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade / step * 2);   // Simple wrap from Marc Miller
          leds[(center - step + NUM_LEDS) % NUM_LEDS] += CHSV(colour, 255, myfade / step * 2);
          step ++;                                                         // Next step.
          break;
      }
    }

    
  }

  EVERY_N_SECONDS(5) {
    targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 192, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)));
  }

  FastLED.setBrightness(brightness);  //EXECUTE EFFECT COLOR
  FastLED.show();

  if (animationspeed > 0 && animationspeed < 150) {  //Sets animation speed based on receieved value
    FastLED.delay(1000 / animationspeed);
  }
   ArduinoOTA.handle();
}

long microsecondsToInches(long microseconds)
{
// According to Parallax's datasheet for the PING))), there are
// 73.746 microseconds per inch (i.e. sound travels at 
//1130 feet per second).  This gives the distance travelled 
//by the ping, outbound and return, so we divide by 2 to get the 
//distance of the obstacle. 
//See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}
 
long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per
  // centimeter. The ping travels out and back, so to find 
  //the distance of the object we take half of the distance 
  //travelled.
  return microseconds / 29 / 2;
}

void setupStripedPalette( CRGB A, CRGB AB, CRGB B, CRGB BA)
{
  currentPalettestriped = CRGBPalette16(
                            A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                            //    A, A, A, A, A, A, A, A, B, B, B, B, B, B, B, B
                          );
}


void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } } //for CYCLON


void Fire2012WithPalette()
{
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}


void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void addGlitterColor( fract8 chanceOfGlitter, int Rcolor, int Gcolor, int Bcolor) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB(Rcolor, Gcolor, Bcolor);
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
  if (client.connect("PorchLEDs", mqtt_user, mqtt_password)) {
      Serial.println("connected");

      FastLED.clear (); //Turns off startup LEDs after connection is made
      FastLED.show();

      client.subscribe(setcolorsub);
      client.subscribe(setbrightness);
      //client.subscribe(setcolortemp);
      client.subscribe(setpowersub);
      client.subscribe(seteffectsub);
      client.subscribe(setanimationspeed);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


