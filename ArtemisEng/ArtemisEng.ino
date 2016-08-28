#include <Adafruit_NeoPixel.h>
#include "Adafruit_WS2801.h"

// https://github.com/NicoHood/HID
#include "HID-Project.h"

/*
   Copyright Christian Nilsson 2016

   Using pin 2-9 for row select output, goes to high part input of each pot (pin one)
   Each pot select pin is connected via a separate diode to A0, A0 is also pulled low by a 1M ohm resistor
   ASCII art displaying this
                             ______________________
   D2   o>------------------| <
              +---|<|-------| --------#############
              | 1N4184 +----|______________________
              |        |     ______________________
   D3   o>------------------| <
              +---|<|-------| --------#############
              | 1N4184 +----|______________________
   4..6 Con-  !        !
   tinued     !        !
              |   1M   |
   A0   o<----+-/\/\/\-+
                       |
   GND o---------------+

   Diodes will work as a small resistor for each pin making it possible to detect no touch as 0 on ADC
   While a touch will register at about 16 to 940, but leaving a bit of room for actually reaching the full
   by only detecting between POTMIN and POTMAX
*/

const float refX = 32767 / (float)1366;
const float refY = 32767 / (float)768;

#define X_SLIDE 50
#define X_COOL 94
#define X_SPACE 168
#define Y_SLIDETOP 502
#define Y_SLIDEBOT 748
#define Y_COOLUP 535
#define Y_COOLDOWN 745

#define NEOPXPIN 0
#define SLIDERS 8
#define POTDETECT 16
#define POTMIN 20
#define POTMAX 920
const uint8_t COLPINS[SLIDERS] = {2, 3, 4, 5, 6, 7, 8, 9};
#define ROWS 4
const uint8_t ROWPINS[ROWS] = {14, 15, 10, 16};
uint16_t oVal[SLIDERS];
// oTouched could be a byte, using it's bits (to save a few bytes of ram)
bool oTouched[SLIDERS];

uint8_t coolant[SLIDERS];
Adafruit_NeoPixel cstrip = Adafruit_NeoPixel(8, NEOPXPIN, NEO_GRB + NEO_KHZ800);

#define WSDATAPIN 20
#define WSCLOCKPIN 21
#define WSNUMPIXELS 20
Adafruit_WS2801 hstrip = Adafruit_WS2801(WSNUMPIXELS, WSDATAPIN, WSCLOCKPIN);

// Variables will change:
bool overheat = false;             // start with flashing off
bool ledState = false;              // leds off
long previousMillis = 0;        // will store last time LEDs was updated
#define interval 200           // interval at which to blink (milliseconds)
uint8_t stations[SLIDERS] = { 1, 1, 1, 1, 1, 1, 1, 1 };

void reset_coolant() {
  for (int i = 0; i < SLIDERS; i++) {
    coolant[i] = 0;
  }
}

void setup() {
  // Sends a clean report to the host. This is important on any Arduino type.
  BootKeyboard.begin();
  AbsoluteMouse.begin();

  cstrip.begin();
  hstrip.begin();
  hstrip.show();

  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  for (int i = 0; i < ROWS; i++) {
    pinMode(ROWPINS[i], INPUT_PULLUP);
  }

  // initialize base slider pos (non touched base value)
  for (int i = 0; i < SLIDERS; i++) {
    pinMode(COLPINS[i], OUTPUT);
    oVal[i] = POTMIN;
  }
}

void setPos(int x, int y, bool press = false) {
  //Serial.println("Moving to " + String(x) + ", " + String(y) + String(press ? " pressed" : ""));
  AbsoluteMouse.moveTo(x, y);
  if (press)
    AbsoluteMouse.press();
}

void setRefPos(int x, int y, bool press = false) {
  //Serial.println("Moving to ref " + String(x) + ", " + String(y) + String(press ? " pressed" : ""));
  setPos(refX * x, refY * y, press);
}

void key(KeyboardKeycode k, bool pres) {
  if (pres)
    BootKeyboard.press(k);
  else
    BootKeyboard.release(k);
}

String bin_string(uint64_t v) {
  // this is less then suboptimal code
  String ret = "";
  for (int i = 31; i >= 0; i--) {
    ret += String((int)((v >> (i )) & 0x1), HEX);
    if (i > 0 && i % 4 == 0) ret += " ";
  }
  return ret;
}

void flash_rgb(byte r, byte g, byte b) {
  for (int i = SLIDERS; i < WSNUMPIXELS; i++) {
    hstrip.setPixelColor(i, r, g, b);
  }
  hstrip.show();
}

// Clear Serialbuffer
void flushReceive() {
  while (Serial.available() > 0)
    Serial.read();
}

void do_flash() {
  // Flashing timer
  if (overheat) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the LED is off turn it on and vice-versa:
      ledState = !ledState;
      flash_rgb(ledState ? 128 : 0, 0, 0);
    }
  }
  // Make sure flashing-LEDs are off if no flashing
  else if(ledState) {
    ledState = false;
    flash_rgb(0, 0, 0);
  }
}

void read_coolant() {
  //Read Serial and update leds
  if (Serial.available() > 0 ) {
    // Reset flashing
    overheat = false;
    for (int stationNo = 0; stationNo < SLIDERS && Serial.available() > 0; stationNo++) {
      int cur = Serial.parseInt();
      stations[stationNo] = cur;
      Serial.print(cur);
      if (cur == 0) {   //off
        hstrip.setPixelColor(stationNo, 0, 0, 0);
      }
      else if (cur == 1) {     //white
        hstrip.setPixelColor(stationNo, 128, 128, 128);
      }
      else if (cur == 2) {   //blue
        hstrip.setPixelColor(stationNo, 0, 0, 128);
      }
      else if (cur == 3) {   //green
        hstrip.setPixelColor(stationNo, 0, 128, 0);
      }
      else if (cur == 4) {   //orange
        hstrip.setPixelColor(stationNo, 128, 64, 0);
      }
      else if (cur == 5) {   //red
        hstrip.setPixelColor(stationNo, 128, 0, 0);
        overheat = true;
      }
      else {           //fail
        hstrip.setPixelColor(stationNo, 128, 0, 128);     //purple
      }
      Serial.print(",");
    }
    hstrip.show();
    int av = Serial.available();
    if (av > 0) {
      Serial.print("Clear available: ");
      Serial.print(av);
      //Clear Serial
      flushReceive();
    }
    Serial.println();
  }
}

void loop() {
  int sv[SLIDERS];
  bool hasChange = false;

  byte samplemask = 0;
  byte rsamples[SLIDERS];
  // walk each output rows selector ..
  for (int i = 0; i < SLIDERS; i++) {
    digitalWrite(COLPINS[i], HIGH);

    // check if any row pin is forced low
    byte sample = 0;
    for (int ii = 0; ii < ROWS; ii++) {
      sample = sample << 1;
      if (!digitalRead(ROWPINS[ii])) {
        sample |= 0x1;
      }
    }
    samplemask |= sample;
    rsamples[i] = sample;

    // ... waiting a litle bit for ADC to settle and read analog value.
    delay(2);
    sv[i] = analogRead(A0);
    digitalWrite(COLPINS[i], LOW);

    // check if it is within pot detection (is touched) and value is changed from before.
    // save touched state, and only do updates if there was a touch last loop,
    //   it works as a debounce to prevent random 1-time weird data reads.
    bool hasTouch = (sv[i] >= POTDETECT && sv[i] != oVal[i]);
    if (hasTouch && oTouched[i]) {
      oVal[i] = sv[i];
      // clamp the new value within MIN and MAX
      if (oVal[i] < POTMIN) oVal[i] = POTMIN;
      if (oVal[i] > POTMAX) oVal[i] = POTMAX;
      hasChange = true;
      // use absolute mouse to move cursor to a pixel position (based on defined screen resolution)
      setRefPos(X_SLIDE + X_SPACE * i, map(oVal[i], POTMIN, POTMAX, Y_SLIDEBOT, Y_SLIDETOP), true);
    }
    oTouched[i] = hasTouch;
  }
  if (!hasChange && AbsoluteMouse.isPressed())
    AbsoluteMouse.release();

  uint32_t state = 0;
  for (int i = 0; i < SLIDERS; i++) {
    byte sample = rsamples[i] ^ samplemask;
    state = state << 0x4;
    if (sample) {
      state |= sample & 0xf;
    }
  }
  static uint32_t laststate = 0;
  static uint32_t lastpressed = 0;
  // debounce by only using state if it is the same state as last loop
  if (state == laststate && state != lastpressed) {
    Serial.println(bin_string(state));
    uint32_t change = lastpressed ^ state;
    for (int i = 0; i < SLIDERS; i++) {
      byte sample = state >> (0x4 * (SLIDERS - 1 - i)) & 0xf;
      byte changed = change >> (0x4 * (SLIDERS - 1 - i)) & 0xf;
      if (changed) {
        if (changed & B1000) {
          Serial.println(" Cool " + String(i) + " down " + (sample & B1000 ? "pressed" : "released"));
          if (sample & B1000) {
            setRefPos(X_COOL + X_SPACE * i, Y_COOLDOWN);
            AbsoluteMouse.click();
            if (coolant[i] > 0) {
              coolant[i]--;
              hasChange = true;
            }
          }
          changed ^= B1000;
        }
        if (changed & B0100) {
          Serial.println(" Cool " + String(i) + " up " + (sample & B0100 ? "pressed" : "released"));
          if (sample & B0100) {
            setRefPos(X_COOL + X_SPACE * i, Y_COOLUP);
            AbsoluteMouse.click();
            if (coolant[i] < 8) {
              coolant[i]++;
              hasChange = true;
            }
          }
          changed ^= B0100;
        }
        if (changed & B0001) {
          Serial.println(" preset " + String(i + 1) + " " + (sample & B0001 ? "pressed" : "released"));
          key((KeyboardKeycode)(KEY_1 + i), sample & B0001);
          changed ^= B0001;
        }
        if (changed & B0010 && i >= 6 && i <= 7) {
          Serial.println(" preset " + String(i + 2) + " " + (sample & B0010 ? "pressed" : "released"));
          key((KeyboardKeycode)(KEY_1 + i + 2), sample & B0010);
          changed ^= B0010;
        }
        if (changed & B0010 && i == 0) {
          Serial.println(" shift " + String(sample & B0010 ? "pressed" : "released"));
          key(KEY_LEFT_SHIFT, sample & B0010);
          changed ^= B0010;
        }
        if (changed & B0010 && i == 4) {
          Serial.println(" enter " + String(sample & B0010 ? "pressed" : "released"));
          key(KEY_ENTER, sample & B0010);
          changed ^= B0010;
          reset_coolant();
          hasChange = true;
        }
        if (changed & B0010 && i == 5) {
          Serial.println(" space " + String(sample & B0010 ? "pressed" : "released"));
          key(KEY_SPACE, sample & B0010);
          changed ^= B0010;
        }
        if (changed) {
          Serial.print(i);
          Serial.print("   ");
          Serial.print(changed, HEX);
          Serial.print("   ");
          Serial.println(sample, HEX);
        }
      }
    }
    lastpressed = state;
  }
  laststate = state;

  read_coolant();
  do_flash();

  if (hasChange) {
    // print a timestamp followed by each sliders (current analog, last analog, and mapped) value
    Serial.print(millis());
    Serial.print("\t");
    for (int i = 0; i < SLIDERS; i++) {
      Serial.print(sv[i]);
      Serial.print("\t: ");
      Serial.print(oVal[i]);
      Serial.print("\t");
      // update color of neopixel for strip
      cstrip.setPixelColor(i, cstrip.Color(map(oVal[i], POTMIN, POTMAX, 0, 8 * 16), 0, coolant[i] * 16));
    }
    Serial.println();
    cstrip.show();
  }
}
