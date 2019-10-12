#include <FastLED.h>

#define BUFFER_SIZE 60
uint16_t analog_buffer[BUFFER_SIZE];

#define NUM_LEDS    269
#define BRIGHTNESS  50
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 100

//const byte soundPin = 2;        //Pin for Sound Sensor
const byte soundPin = A0;       //Pin for Sound Sensor
const byte LED_PIN = 10;        //Dataline for LED Strip
const byte modePin = 8;         //Used to determine if in music mode or relax mode
                                //HIGH = Relax, LOW = Music

CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t move_speed = 1;         //How many LEDS the color will move over

//Sound Tracking Variables
unsigned long lastIntTime = 0;  //Last Time Palette was changed
unsigned long endOfBothPats= 0;
int detection_count = 0;        //
bool curr_alt_pal = false;      //Used to Alternate Between Alternative Palette's
bool pattern1_started = false;   //Used to Signal Alternative Palettes Have been started
bool pattern2_started = false;   //Used to Signal Alternative Palettes Have been started
bool pat1_flag = false;
bool pat2_flag = false;

//Pre-Made Palettes That are stored in Flash because it is
//much more plentiful
extern const TProgmemPalette16 alt1_palette1_p PROGMEM;
extern const TProgmemPalette16 alt1_palette2_p PROGMEM;
extern const TProgmemPalette16 alt2_palette1_p PROGMEM;
extern const TProgmemPalette16 alt2_palette2_p PROGMEM;
extern const TProgmemPalette16 bright_colors_p PROGMEM;
extern const TProgmemPalette16 love_fest_p PROGMEM;
extern const TProgmemPalette16 ocean_p PROGMEM;
extern const TProgmemPalette16 lava_p PROGMEM;
extern const TProgmemPalette16 forest_p PROGMEM;


#define NUM_RELAX_TEMPLATES 4
byte index = 0;

enum MODE{
  MUSIC,
  RELAX
};
MODE mode;


////////////////////////////////////////////////////////////
/////                        SETUP                      ////
////////////////////////////////////////////////////////////

void setup() {
  delay( 3000 ); // power-up safety delay
  pinMode(modePin,INPUT);

  for(int i=0;i<BUFFER_SIZE;i++){analog_buffer[i] = analogRead(soundPin);}
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  if(digitalRead(modePin)){
    mode = RELAX;
    currentPalette = love_fest_p;
  }
  else{
    mode = MUSIC;
    currentPalette = RainbowColors_p;
  }
  currentBlending = LINEARBLEND;
  lastIntTime = millis();
//  Serial.begin(115200);
}

////////////////////////////////////////////////////////////
/////                       LOOP                        ////
////////////////////////////////////////////////////////////

void loop() {
  static long fft_debounce = millis();
  static uint8_t start_index = 0;
  start_index = start_index + move_speed; //Move the LEDS
  
  FillLEDsFromPaletteColors(start_index);
  
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);

  if(newPeak())
  {
    if(millis() - fft_debounce > 300){
      soundDetected();
      fft_debounce = millis();
    }
  }
}

bool newPeak()
{
  static int pos = 0;
  int newVal = analogRead(soundPin);
//  Serial.println(newVal);
  double avg = 0;
  for(int i=0;i<BUFFER_SIZE;i++)
  {
    avg += analog_buffer[i];
  }
  avg = avg/BUFFER_SIZE;
  analog_buffer[pos] = newVal;
  pos = (pos+1)%BUFFER_SIZE;
  return( abs((double)newVal-avg) > 3);
}

//Change the LED's colors from palette
void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
  uint8_t brightness = 200;
  for( int i = 0; i < NUM_LEDS; i++) {
      leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
      colorIndex += 3;
  }
}

///////////////////////////////////////////////////
///                  Interrupt                /////
///////////////////////////////////////////////////

// Change Palette when noise is detected
void soundDetected()
{ 
  //If in Relax mode
  if(mode == RELAX)
  {
    index = (index + 1) % NUM_RELAX_TEMPLATES;
    if(index == 0){currentPalette = love_fest_p;}
    else if(index==1){currentPalette = ocean_p;}
    else if(index==2){currentPalette = forest_p;}
    else{currentPalette = lava_p;}
    return;
  }
  
  //Time of Interrupt
  unsigned long timeNow = millis();
  detection_count++;
//  Serial.println(detection_count);
  if((timeNow - lastIntTime > 100 && timeNow - lastIntTime < 500) && !pat1_flag ){
    if(curr_alt_pal){currentPalette = alt2_palette1_p;}
    else{currentPalette = alt2_palette2_p;}
    move_speed = 0; 
    curr_alt_pal=!curr_alt_pal; 
    currentBlending = NOBLEND;
    pattern1_started = true;
    if(detection_count > 20){
      pattern1_started = false;
      pat1_flag = true;
      if(pat2_flag){endOfBothPats = millis();}
//      Serial.println("DONE PAT 1");
    }
//    Serial.println("DOING PAT 1");
  }
  
  else if((timeNow - lastIntTime > 500 && timeNow - lastIntTime < 700) && !pat2_flag){
    if(curr_alt_pal){currentPalette = alt1_palette1_p;}
    else{currentPalette = alt1_palette2_p;}
    move_speed = 0; 
    curr_alt_pal=!curr_alt_pal; 
    currentBlending = NOBLEND;
    pattern2_started = true;
    if(detection_count > 20){
      pattern2_started = false;
      pat2_flag = true;
      if(pat1_flag){endOfBothPats = millis();}
//      Serial.println("DONE PAT 2");
    }
//    Serial.println("DOING PAT 2");
  }
  //((pat1_flag && pat2_flag) && (timeNow - lastIntTime > 300 && timeNow - lastIntTime < 2000))
  else if((timeNow - lastIntTime > 700 && timeNow - lastIntTime < 1500) || ((pat1_flag && pat2_flag) && (timeNow - lastIntTime > 100 && timeNow - lastIntTime < 2000))){
    move_speed = 1;
    randomPalette();
//    Serial.println("RANDO");
  }
  else if((timeNow - lastIntTime > 1500 && timeNow - lastIntTime < 2500) || ((pat1_flag && pat2_flag) && (timeNow - lastIntTime > 2000 && timeNow - lastIntTime < 4000))){
    randomStripes(detection_count % 4);
//    Serial.println("STRIPES");
  }
  else if(timeNow -lastIntTime > 4000 || ((pat1_flag && pat2_flag) && (timeNow - lastIntTime > 4000))){
//    Serial.println("ONE PAL");
    if(random8() > 127){
      currentPalette = bright_colors_p;
      currentBlending = LINEARBLEND;
    }
    else if(random8() > 127){
      currentPalette = love_fest_p;
      currentBlending = LINEARBLEND;
    }
    else{
      oneColorPalette();
    }
    move_speed = 1;
  }

  //End Pattern
  if((pat1_flag && pat2_flag) && timeNow - endOfBothPats > 4000 ){
    detection_count = 0;
    pattern1_started = false;
    pattern2_started = false;
    pat1_flag = false;
    pat2_flag = false;
  }
  lastIntTime = timeNow;
}


/////////////////////////////////////////////////
///                 Palette's               /////
/////////////////////////////////////////////////

//Change entire palette to one random color
void oneColorPalette(){
  int rgbVal1 = random8();
  int rgbVal2 = random8();
  int rgbVal3 = random8();
  for( int i = 0; i < 16; i++) {currentPalette[i] = CHSV(rgbVal1 , rgbVal2, rgbVal3);}
}

//Change entire palette to random colors
void randomPalette(){
  for( int i = 0; i < 16; i++) {
    if(i%4 == 0){currentPalette[i] = CHSV( 255, random8(), random8());}
    else if(i % 3 == 0){currentPalette[i] = CHSV( random8(), random8(), 255);}
    else{currentPalette[i] = CHSV( random8(), 255, random8());}
  }
  if(random8() > 127){currentBlending = LINEARBLEND;}
  else{currentBlending = NOBLEND;}
}

//Change palette random stripes that can move at different intervals
void randomStripes(uint8_t moveSpeed){
  fill_solid( currentPalette, 16, CRGB::Black);
  currentPalette[0] = CHSV( random8(), 255, random8());
  currentPalette[4] = CHSV( random8(), 255, random8());
  currentPalette[8] = CHSV( 255, random8(), random8());
  currentPalette[12] = CHSV( random8(), random8(), 255);
  currentBlending = NOBLEND;
  move_speed = moveSpeed;
}




////Palettes Stored in Flash Becauase RAM is not plentiful
const TProgmemPalette16 bright_colors_p PROGMEM =
{
  CRGB::Red,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Green,
  
  CRGB::Red,
  CRGB::Yellow,
  CRGB::Blue,
  CRGB::Purple,
  
  CRGB::Orange,
  CRGB::Red,
  CRGB::Gray,
  CRGB::Purple,
  
  CRGB::Blue,
  CRGB::Blue,
  CRGB::Yellow,
  CRGB::Green
};

const TProgmemPalette16 alt1_palette2_p PROGMEM =
{
  CRGB::Red,
  CRGB::Yellow, 
  CRGB::Blue,
  CRGB::Green,
  
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  
  CRGB::Orange,
  CRGB::Purple,
  CRGB::Blue,
  CRGB::Red,
  
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black
};

const TProgmemPalette16 alt1_palette1_p PROGMEM =
{
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  
  CRGB::Red,
  CRGB::Yellow,
  CRGB::Blue,
  CRGB::Green,
  
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  
  CRGB::Orange,
  CRGB::Purple,
  CRGB::Blue,
  CRGB::Red
};

const TProgmemPalette16 alt2_palette1_p PROGMEM =
{
  CRGB::Blue,
  CRGB::Purple,
  CRGB::Pink,
  CRGB::Red,
  
  CRGB::Pink, 
  CRGB::Purple,
  CRGB::Blue,
  CRGB::Blue,
  
  CRGB::Purple,
  CRGB::Pink,
  CRGB::Red,
  
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black
};
const TProgmemPalette16 alt2_palette2_p PROGMEM =
{
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  
  CRGB::Blue,
  CRGB::Purple,
  CRGB::Pink,
  CRGB::Red,
  
  CRGB::Pink, 
  CRGB::Purple,
  CRGB::Blue,
  CRGB::Blue,
  
  CRGB::Purple,
  CRGB::Pink,
  CRGB::Red
};

////Palettes Stored in Flash Becauase RAM is not plentiful
const TProgmemPalette16 love_fest_p PROGMEM =
{
    0x640000,
    0x640032,
    0x640064,
    0x003264,
    
    0x006432,
    0x006400,
    0x640032,
    0x000000,
    
    0x000000,
    0x320064,
    0x323264,
    0x000064,
    
    0x641e3c,
    0x226422,
    0x145050,
    0x000000
};


const TProgmemPalette16 ocean_p PROGMEM
{
  CRGB::MidnightBlue,
  CRGB::DarkBlue,
  CRGB::MidnightBlue,
  CRGB::Navy,

  CRGB::DarkBlue,
  CRGB::MediumBlue,
  CRGB::SeaGreen,
  CRGB::Teal,

  CRGB::CadetBlue,
  CRGB::Blue,
  CRGB::DarkCyan,
  CRGB::CornflowerBlue,

  CRGB::Aquamarine,
  CRGB::SeaGreen,
  CRGB::Aqua,
  CRGB::LightSkyBlue
};

const TProgmemPalette16 lava_p PROGMEM
{
  CRGB::Black,
  CRGB::Maroon,
  CRGB::Black,
  CRGB::Maroon,

  CRGB::DarkRed,
  CRGB::Maroon,
  CRGB::DarkRed,

  CRGB::DarkRed,
  CRGB::DarkRed,
  CRGB::Red,
  CRGB::Orange,

  CRGB::White,
  CRGB::Orange,
  CRGB::Red,
  CRGB::DarkRed
};

const TProgmemPalette16 forest_p PROGMEM
{
      0x004b00,
      0x000000,
      0x3c461e,
      0x004b00,
      
      0x006400,
      0x146414,
      0x46641e,
      0x000000,
      
      0x0a5a3c,
      0x145046,
      0x329632,
      0x505028,
      
      0x000000,
      0x226422,
      0x145050,
      0x323232
};

//    CRGB(0,75,0),
//    CRGB(0,0,0),
//    CRGB(60,70,30),
//    CRGB(0,75,0),
//  
//    CRGB(0,100,0),
//    CRGB(20,80,20),
//    CRGB(70,80,30),
//    CRGB(0,0,0),
//  
//    CRGB(10,90,60),
//    CRGB(20,80,70),
//    CRGB(50,150,50),
//    CRGB(80,80,40),
//  
//    CRGB(0,0,0),
//    CRGB(34, 100,34),
//    CRGB(20,80,80),
//    CRGB(50,50,60)
