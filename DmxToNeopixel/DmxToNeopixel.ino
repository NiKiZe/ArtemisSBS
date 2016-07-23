#include <Adafruit_NeoPixel.h>
#include <DMXSerial.h>

#define BASE_CH 4 // 1 based DMX channel, max 512
#define NEOPIN 6
#define RELAYPIN 4
#define PixelCount 240

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PixelCount, NEOPIN, NEO_GRB + NEO_KHZ800);

uint32_t const ColorArray[] = {
  strip.Color(64, 0, 0), // Red
  strip.Color(128, 0, 0), // Red
  strip.Color(255, 0, 0), // Red
  strip.Color(64, 64, 0),
  strip.Color(0, 64, 0), // Green
  strip.Color(0, 64, 64),
  strip.Color(0, 0, 64), // Blue
  strip.Color(64, 0, 64),
  strip.Color(1, 1, 1),
};

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait, uint16_t maxOn = 8) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    // only keep maxOn pixels turned on at a time
    if (i >= maxOn)
      strip.setPixelColor(i - maxOn, 0);
    if (wait > 0) {
      strip.show();
      delay(wait);
    }
  }
  if (wait <= 0)
    strip.show();
}

void setup()
{
  DMXSerial.init(DMXReceiver);

  strip.begin();

  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);

  strip.show();
  strip.setPixelColor(0, 0x000008);
  strip.setPixelColor(PixelCount - 1, 0x000100);
  //strip.setPixelColor(PixelCount+1, 0);
  strip.show();
  delay(200);

  colorWipe(ColorArray[0], 10); // Red
  colorWipe(ColorArray[8], 10);

  digitalWrite(RELAYPIN, LOW);
}

int lastOk = 1;
uint32_t old = 0;
bool relay = false;

void loop()
{
  // Calculate how long no data backet was received
  unsigned long lastPacket = DMXSerial.noDataSince();

  if (lastPacket < 3000) {
    bool hasChange = lastOk != 0;
    lastOk = 0;
    uint32_t c = 0; //pixel color return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
    for (int i = BASE_CH; i < BASE_CH + 3 && i <= 512; i++) {
      byte b = DMXSerial.read(i);
      c = (c << 8) | b;
    }
    if (old != c) {
      colorWipe(c, 0, 512);
      old = c;
    }
    c = 0;
    bool nrelay = DMXSerial.read(BASE_CH + 3) != 0;
    if (relay != nrelay) {
      digitalWrite(RELAYPIN, nrelay ? HIGH : LOW);
      relay = nrelay;
    }
    if (hasChange) {
      strip.show();
    }
  } else if (lastOk == 0) {
    lastOk = 1;
  } else {
    colorWipe(ColorArray[lastOk - 1], 10);
    if (lastOk++ == 9)
      lastOk = 1;
  }
}

