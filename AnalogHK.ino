// AnalogHotKey
// https://github.com/SilentDreamcast/AnalogHotKey
// Discord: CheckEm #9384
// This code turns an Arduino with native USB into a Human Interface Device (HID) and MIDI
// by default the device is in MIDI mode. pressing buttons 2+4 sets the buttons to keyboard mode (light will flash purple twice)
// pressing buttons 1+4 in MIDI mode makes the buttons toggle (default) or momentary. (light will flash twice with current color)

// USER CONFIGURABLE
//#define DEBUG //enable this to print debug messages to serial port
const static uint8_t MIDICHANNEL = 0; //MIDI instrument channel. CHANGE THIS IF IT CONFLICTS WITH OTHER MIDI DEVICES
//MIDI control channel. You can add more with more buttons/knobs or virtual layers
const static uint8_t BUTTON1CONTROL = 0;
const static uint8_t BUTTON2CONTROL = 1;
const static uint8_t BUTTON3CONTROL = 2;
const static uint8_t BUTTON4CONTROL = 3;
const static uint8_t POT1CONTROL = 10; //if fast feedback enabled for this channel, value maps to red LED value
const static uint8_t POT2CONTROL = 11; //if fast feedback enabled for this channel, value maps to green LED value
const static uint8_t POT3CONTROL = 12; //if fast feedback enabled for this channel, value maps to blue LED value
const static uint8_t POT4CONTROL = 13; //if fast feedback enabled for this channel, value maps to 0 to BRIGHTNESS (defined below) (intended as like the master volume)
const static uint8_t BUTTON1KEY = 'I'; //set your own keys here or even media shortcuts. maybe create a virtual layer 
const static uint8_t BUTTON2KEY = 'J';
const static uint8_t BUTTON3KEY = 'K';
const static uint8_t BUTTON4KEY = 'L';
const static uint8_t BRIGHTNESS = 255; //0-255 max brightness of RGB LED //absolute max brightness
const static uint8_t hystBand = 2; // +/- how much the potentiometer should change before registering as an actual change. 
const static long interval = 10; //polling rate in mS

#include "MIDIUSB.h"
#include "Keyboard.h"
#include <Adafruit_NeoPixel.h>

// EVERYTHING BELOW THIS IS ALSO CONFIGURABLE IF YOU UNDERSTAND THE SPAGHETTI:P
// PINOUTS
#define POT1 A0
#define POT2 A1
#define POT3 A2
#define POT4 A3
#define BUTTON1 7 //put the buttons on hardware interrupt pins in case
#define BUTTON2 3
#define BUTTON3 2
#define BUTTON4 0
#define PIXELPIN 6 //neopixel pin (470ohm series resistor)

// NEOPIXEL (OPTIONAL)
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);

// GENERAL STUFF
unsigned long previousMillis = 0;
int16_t pot1Prev, pot2Prev, pot3Prev, pot4Prev; //stores previous values for hysteresis
boolean button1State, button2State, button3State, button4State; //stores wibbly wobbly latching flags so it doesn't send unnecessary MIDI updates
uint8_t red, green, blue;
boolean buttonToggle = true; //enable momentary(false) or toggle(true) button action
boolean MIDImode = true;

void setup() {
  // put your setup code here, to run once:
  pinMode(POT1,INPUT); //ATMEGA32u4 has 8 ADC channels but only 4 are broken out on the standard Pro Micro. Get a Teensy lol
  pinMode(POT2,INPUT);
  pinMode(POT3,INPUT);
  pinMode(POT4,INPUT);
  //analogReadResolution(8); //set ADC to 8 bit mode. no need to do mapping anymore (only available for certain boards)
  pinMode(BUTTON1,INPUT_PULLUP); //lets be cheap and super accessable since everyone always forgets the pull-up/down resistor
  pinMode(BUTTON2,INPUT_PULLUP);
  pinMode(BUTTON3,INPUT_PULLUP);
  pinMode(BUTTON4,INPUT_PULLUP);
  
  // WS2812B addressable RGB led just because. gonna use it as a MIDI output device just to look cool.
  pixels.begin();
  pixels.clear();
  pixels.show();
  pixels.setBrightness(BRIGHTNESS);
  
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

//it's not fancy since i wanted to make it obvious for beginners to modify
void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) { //EZ debounce
    previousMillis = currentMillis;

    // POTS =======================================================================================
    potHandler(POT1, pot1Prev, MIDICHANNEL, POT1CONTROL);
    potHandler(POT2, pot2Prev, MIDICHANNEL, POT2CONTROL);
    potHandler(POT3, pot3Prev, MIDICHANNEL, POT3CONTROL);
    potHandler(POT4, pot4Prev, MIDICHANNEL, POT4CONTROL);
  
    // BUTTON1 ====================================================================================
    buttonHandler(BUTTON1,button1State,MIDICHANNEL,BUTTON1CONTROL,BUTTON1KEY);
    buttonHandler(BUTTON2,button2State,MIDICHANNEL,BUTTON2CONTROL,BUTTON2KEY);
    buttonHandler(BUTTON3,button3State,MIDICHANNEL,BUTTON3CONTROL,BUTTON3KEY);
    buttonHandler(BUTTON4,button4State,MIDICHANNEL,BUTTON4CONTROL,BUTTON4KEY);

    //special key combo to do certain things buttons 1-3 + 4 ======================================
    comboHandler();
  }

  MIDI_feedback(); //process feedback all the time as needed
}

void MIDI_feedback(){
  //use the remaining processing time to update the LED
  midiEventPacket_t rx;
  do {
    rx = MidiUSB.read();
    if (rx.header != 0) {
      #ifdef DEBUG
        Serial.print("Received: ");
        Serial.print(rx.header, HEX);
        Serial.print("-");
        Serial.print(rx.byte1, HEX);
        Serial.print("-");
        Serial.print(rx.byte2, HEX);
        Serial.print("-");
        Serial.println(rx.byte3, HEX);
      #endif
      if(rx.byte2 == POT1CONTROL){ //if(rx.byte2 == 20){
        red = map(rx.byte3,0,127,0,255);
      }
      if(rx.byte2 == POT2CONTROL){ //if(rx.byte2 == 21){
        green = map(rx.byte3,0,127,0,255);
      }
      if(rx.byte2 == POT3CONTROL){ //if(rx.byte2 == 22){
        blue = map(rx.byte3,0,127,0,255);
      }
      if(rx.byte2 == POT4CONTROL){ //if(rx.byte2 == 22){
        pixels.setBrightness(map(rx.byte3,0,127,0,BRIGHTNESS)); //remap between 0 and BRIGHTNESS so it doesn't exceed the maximum defined.
      }
      pixels.setPixelColor(0, pixels.Color(red,green,blue));
      pixels.show();
    }
  } while (rx.header != 0);
}

void potHandler(uint8_t pot, int16_t & prev, uint8_t midiChannel, uint8_t potControl){
  uint16_t potCurr = analogRead(pot)/8; //divide 10bits by 8 turns value into 7 bit value
  if(potCurr > prev+hystBand || potCurr < prev-hystBand){
    #ifdef DEBUG
      Serial.print("update: ");
      Serial.print(potCurr);
      Serial.print(", ");
      Serial.println(prev);
    #endif
    prev = potCurr;
    controlChange(midiChannel, potControl, potCurr);
    MidiUSB.flush();
  }
}

void buttonHandler(uint8_t button, boolean & state, uint8_t midiChannel, uint8_t buttonControl, char key){
  if(!digitalRead(button)){ //since buttons are configured as INPUT_PULLUP detect if LOW
    if(state == false){ //doing some boolean latching
      #ifdef DEBUG
        Serial.print(button);
        Serial.println(" PRESSED");
      #endif
      if(MIDImode){
        noteOn(midiChannel,buttonControl,127);
        MidiUSB.flush();
      }
      else{
        Keyboard.press(key);
      }
      state = true;
    }
  }
  else{
    if(state == true){
      #ifdef DEBUG
        Serial.print(button);
        Serial.println(" RELEASED");
      #endif
      if(buttonToggle){
        if(MIDImode){
          noteOff(midiChannel,buttonControl,127);
        }
        else{
          Keyboard.release(key);
        }
      }
      else{
        if(MIDImode){
          noteOn(midiChannel,buttonControl,127);
        }
      }
      if(MIDImode){
        MidiUSB.flush();
      }
      state = false;
    }
  }
}

// MIDI FUNCTIONS
// channel = midi device
// control/pitch = device's subcontrol (button, dial, indicator)
// value/velocity = actual value. 0-127
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void comboHandler(){
  // MIDI button toggle or momentary
  if(!digitalRead(BUTTON1) && !digitalRead(BUTTON4)){
    buttonToggle = !buttonToggle;
    for(uint8_t i = 0; i < 2; i++){ //kinda like a debounce
      pixels.setPixelColor(0, pixels.Color(0,0,0));
      pixels.show();
      delay(250);
      pixels.setPixelColor(0, pixels.Color(red,green,blue));
      pixels.show();
      delay(250);
    }
  }

  // keyboard mode
  if(!digitalRead(BUTTON2) && !digitalRead(BUTTON4)){
    MIDImode = !MIDImode;
    if(MIDImode == false){
      buttonToggle = true; //while in keyboard mode it works opposite. honestly this is spaghetti now
    }
    else{
      Keyboard.releaseAll(); //release all keys just in case
    }
    for(uint8_t i = 0; i < 2; i++){ //kinda like a debounce
      pixels.setPixelColor(0, pixels.Color(0,0,0));
      pixels.show();
      delay(250);
      pixels.setPixelColor(0, pixels.Color(128,0,128)); //purple at half power
      pixels.show();
      delay(250);
    }
  }
  
  if(!digitalRead(BUTTON3) && !digitalRead(BUTTON4)){
    //does nothing yet
    delay(1000);
  }
}
