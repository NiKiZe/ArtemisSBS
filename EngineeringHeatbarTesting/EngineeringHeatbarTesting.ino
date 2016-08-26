#include "SPI.h"
#include "WS2801.h"

/*****************************************************************************
Engineering Heatbar Testing by Stugo
*****************************************************************************/

// Choose which 2 pins you will use for output.
// Can be any valid output pins.
// The colors of the wires may be totally different so
// BE SURE TO CHECK YOUR PIXELS TO SEE WHICH WIRES TO USE!
// My wires, green = data, blue = clock, yellow = gnd, red = 5V
int dataPin = 2;
int clockPin = 3;
// Don't forget to connect the ground wire to Arduino ground,
// and the +5V wire to a +5V supply
// format for strip.setPixelColor(Pixelnuber, r, g, b);

const int numPixels = 20;  //  20 = 20 pixels in a row

// Set the first variable to the NUMBER of pixels.
WS2801 strip = WS2801(numPixels, dataPin, clockPin);

// Variables will change:
int flashing = false;             // start with flashing off
int ledState = 0;              // leds off

// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long previousMillis = 0;        // will store last time LEDs was updated
long interval = 200;           // interval at which to blink (milliseconds)

int stations[] = {
  1, 1, 1, 1, 1, 1, 1, 1 };
int numStations = 8;
int nbr = 0;


void setup() {
  // Open serial connection.
  Serial.begin(9600);
  strip.begin();

  // Update LED contents, to start they are all 'off'
  strip.show();

}

void loop() {
  // Flashing timer
  if(flashing == true){
    unsigned long currentMillis = millis();
 
    if(currentMillis - previousMillis > interval) {
      // save the last time you blinked the LED 
       previousMillis = currentMillis;   

      // if the LED is off turn it on and vice-versa:
      if (ledState == 0){
        ledState = 1;
        flash_on();
        }
      else{
        ledState = 0;
        flash_off();
        }
      }
  }
  // Make sure flashing-LEDs are off if no flashing
  else if(flashing == false){
    ledState = 0;
    flash_off();
  }
  
  // Reset overheat
  int overheat[] = {
  0, 0, 0, 0, 0, 0, 0, 0,};
  
  // Reset flashing
  flashing = false;
  
  //Read Serial
  if(Serial.available() > 0 ){
    for (int i=0; i < 8; i++){
    stations[i] = Serial.parseInt();
    }
  }
  //Clear Serial
  flushReceive();
  
  // Update the leds  
  for (int thisStation = 0; thisStation < numStations; thisStation++) {
    nbr = thisStation;
    if (stations[thisStation] == 1){      //white
      strip.setPixelColor(nbr, 128, 128, 128);
      strip.show();
      }
    else if (stations[thisStation] == 2){    //blue
      strip.setPixelColor(nbr, 0, 0, 128);
      strip.show();
    }
    else if (stations[thisStation] == 3){    //green
      strip.setPixelColor(nbr, 0, 128, 0);
      strip.show();
    }
    else if (stations[thisStation] == 4){    //orange
      strip.setPixelColor(nbr, 128, 64, 0);
      strip.show();
    }
    else if (stations[thisStation] == 5){    //red
      strip.setPixelColor(nbr, 128, 0, 0);
      strip.show();
      overheat[nbr] = 1;
    }
    else {           //fail
      strip.setPixelColor(nbr, 128, 0, 128);     //purple
      strip.show(); 
    } 
  }
  
  //Check for overheat
  for (int thisLED = 0; thisLED < numStations; thisLED++){
    if (overheat[thisLED] == 1)
      flashing = true;
  }
  /*
  // Print stations[] debug
  for (int a=0; a < 8; a++){
    if (a <= 6){
      Serial.print(stations[a]);
      }
    else if (a == 7){
      Serial.println(stations[a]);
      }
  }
  delay(500);  */
}    //End of loop


//Turn leds 8 < numPixels on (white)
void flash_on() {
  
  for (int i=8; i<numPixels; i++){
    strip.setPixelColor(i, 128, 0, 0);
    strip.show();
  }
}

//Turn leds 8 < numPixels off
void flash_off(){
  for (int i=8; i<numPixels; i++){
    strip.setPixelColor(i, 0, 0, 0);
    strip.show();
  }
}

// Clear Serialbuffer
void flushReceive(){
  while(Serial.available())
    Serial.read();
}

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}
