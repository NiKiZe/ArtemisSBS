#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma

/*****************************************************************************
  Engineering Heatbar Testing by Stugo
*****************************************************************************/

// Choose which 2 pins you will use for output.
// Can be any valid output pins.
// The colors of the wires may be totally different so
// BE SURE TO CHECK YOUR PIXELS TO SEE WHICH WIRES TO USE!
// My wires, green = data, blue = clock, yellow = gnd, red = 5V
#define dataPin 2
#define clockPin 3
// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply
// format for strip.setPixelColor(Pixelnuber, r, g, b);

#define numPixels 20  //  20 = 20 pixels in a row

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 strip = Adafruit_WS2801(numPixels, dataPin, clockPin);

// Variables will change:
bool overheat = false;             // start with flashing off
bool ledState = false;              // leds off

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long previousMillis = 0;        // will store last time LEDs was updated
#define interval 200           // interval at which to blink (milliseconds)

#define numStations 8
int stations[numStations] = { 1, 1, 1, 1, 1, 1, 1, 1 };

void setup() {
  // Open serial connection.
  Serial.begin(9600);
  strip.begin();

  // Update LED contents, to start they are all 'off'
  strip.show();
}

void loop() {
  // Flashing timer
  if (overheat) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;

      // if the LED is off turn it on and vice-versa:
      if (!ledState) {
        flash_on();
      }
      else {
        flash_off();
      }
      ledState = !ledState;
    }
  }
  // Make sure flashing-LEDs are off if no flashing
  else {
    ledState = false;
    flash_off();
  }

  // Reset flashing
  overheat = false;

  //Read Serial and update leds
  if (Serial.available() > 0 ) {
    for (int stationNo = 0; stationNo < numStations && Serial.available() > 0; stationNo++) {
      stations[stationNo] = Serial.parseInt();
      Serial.print(stations[stationNo]);
      if (stations[stationNo] == 1) {     //white
        strip.setPixelColor(stationNo, 128, 128, 128);
      }
      else if (stations[stationNo] == 2) {   //blue
        strip.setPixelColor(stationNo, 0, 0, 128);
      }
      else if (stations[stationNo] == 3) {   //green
        strip.setPixelColor(stationNo, 0, 128, 0);
      }
      else if (stations[stationNo] == 4) {   //orange
        strip.setPixelColor(stationNo, 128, 64, 0);
      }
      else if (stations[stationNo] == 5) {   //red
        strip.setPixelColor(stationNo, 128, 0, 0);
        overheat = true;
      }
      else {           //fail
        strip.setPixelColor(stationNo, 128, 0, 128);     //purple
      }
    }
    strip.show();
  }
  Serial.println();

  //Clear Serial
  flushReceive();
}    //End of loop


//Turn leds 8 < numPixels on (white)
void flash_on() {
  for (int i = numStations; i < numPixels; i++) {
    strip.setPixelColor(i, 128, 0, 0);
  }
  strip.show();
}

//Turn leds 8 < numPixels off
void flash_off() {
  for (int i = numStations; i < numPixels; i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
  strip.show();
}

// Clear Serialbuffer
void flushReceive() {
  while (Serial.available())
    Serial.read();
}

