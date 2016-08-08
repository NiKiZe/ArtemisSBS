#include <Adafruit_NeoPixel.h>

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

const float refX = 32767 / (float)1600;
const float refY = 32767 / (float)900;

#define NEOPXPIN 6
#define SLIDERS 3
#define POTDETECT 16
#define POTMIN 20
#define POTMAX 920
const int ROWPINS[] = {2, 3, 4};
int oVal[SLIDERS];
// oTouched could be a byte, using it's bits (to save a few bytes of ram)
bool oTouched[SLIDERS];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, NEOPXPIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();

  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  AbsoluteMouse.begin();
  // initialize base slider pos (non touched base value)
  for (int i = 0; i < SLIDERS; i++) {
    pinMode(ROWPINS[i], OUTPUT);
    oVal[i] = POTMIN;
  }
}

void colorWipe(uint32_t c) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

void setPos(int x, int y) {
  //Serial.println("Moving to " + String(x) + ", " + String(y));
  AbsoluteMouse.moveTo(x, y);
}

void setRefPos(int x, int y) {
  //Serial.println("Moving to ref " + String(x) + ", " + String(y));
  setPos(refX * x, refY * y);
}

void loop() {
  int sv[SLIDERS];
  bool hasChange = false;

  // walk each output rows selector ..
  for (int i = 0; i < SLIDERS; i++) {
    digitalWrite(ROWPINS[i], HIGH);
    // ... waiting a litle bit for ADC to settle and read analog value.
    delay(2);
    sv[i] = analogRead(A0);
    digitalWrite(ROWPINS[i], LOW);

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
      setRefPos(250 + oVal[i], 200 + 150 * i);
    }
    oTouched[i] = hasTouch;
  }

  int ledv[SLIDERS];
  if (hasChange) {
    // print a timestamp followed by each sliders (current analog, last analog, and mapped) value
    Serial.print(millis());
    Serial.print("\t");
    for (int i = 0; i < SLIDERS; i++) {
      Serial.print(sv[i]);
      Serial.print("\t: ");
      Serial.print(oVal[i]);
      Serial.print(" = ");
      ledv[i] = map(oVal[i], POTMIN, POTMAX, 0, 255);
      Serial.print(ledv[i]);
      Serial.print("\t");
    }
    Serial.println();
    // change color of neopixel strip
    colorWipe(strip.Color(ledv[0], ledv[1], ledv[2]));
  }
}
