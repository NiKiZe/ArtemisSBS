#include "Adafruit_WS2801.h"
//#include "SPI.h" // Comment out this line if using Trinket or Gemma

/*****************************************************************************
  Engineering Heatbar Testing by Stugo
*****************************************************************************/

// Choose which 2 pins you will use for output.
// Can be any valid output pins.
// The colors of the wires may be totally different so
// BE SURE TO CHECK YOUR PIXELS TO SEE WHICH WIRES TO USE!
// My wires, green = data, blue = clock, yellow = gnd, red = 5V
#define dataPin 20
#define clockPin 21
// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply
// format for strip.setPixelColor(Pixelnuber, r, g, b);

#define numPixels 20  //  20 = 20 pixels in a row

// Set the first variable to the NUMBER of pixels.
Adafruit_WS2801 hstrip = Adafruit_WS2801(numPixels, dataPin, clockPin);

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
  hstrip.begin();

  // Update LED contents, to start they are all 'off'
  hstrip.show();
}

void loop() {
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

  //Read Serial and update leds
  if (Serial.available() > 0 ) {
    // Reset flashing
    overheat = false;
    for (int stationNo = 0; stationNo < numStations && Serial.available() > 0; stationNo++) {
      int cur = Serial.parseInt();
      stations[stationNo] = cur;
      Serial.print(cur);
      if (cur == 1) {     //white
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
}    //End of loop


void flash_rgb(byte r, byte g, byte b) {
  for (int i = numStations; i < numPixels; i++) {
    hstrip.setPixelColor(i, r, g, b);
  }
  hstrip.show();
}

// Clear Serialbuffer
void flushReceive() {
  while (Serial.available() > 0)
    Serial.read();
}

