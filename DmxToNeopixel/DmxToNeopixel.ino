#include <Adafruit_NeoPixel.h>
#include <DMXSerial.h>

#define BASE_CH 4 // 1 based DMX channel, max 512
// but remember that Artemis DMX commands file is 0 based
#define NEOPIN 6
#define RELAYS 5
const uint8_t relaychs[] = {7, 8, 11, 12, 13};
const uint8_t relaypins[] = {4, 5, 7, 8, 16};
const bool relayinverted[] = {false, true, false, false, true};
#define SetRelay(r, nrelay)     \
  if (oldrelay[r] != nrelay) {   \
    Serial.println(String(millis()) + " relay ch " + String(relaychs[r]) + " change to " + String(nrelay ? "on" : "off"));  \
    digitalWrite(relaypins[r], nrelay == relayinverted[r] ? LOW : HIGH);  \
    oldrelay[r] = nrelay;  \
  }
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
    if (0 < maxOn && i >= maxOn)
      strip.setPixelColor(i - maxOn, 0);
    if (wait > 0) {
      strip.show();
      delay(wait);
    }
  }
  if (wait <= 0)
    strip.show();
}

bool oldrelay[RELAYS];

void setup()
{
  DMXSerial.init(DMXReceiver);

  strip.begin();

  for (int r = 0; r < RELAYS; r++) {
    uint8_t pin = relaypins[r];
    pinMode(pin, OUTPUT);
    SetRelay(r, true);
    delay(200);
    SetRelay(r, false);
  }

  strip.setPixelColor(0, 0x000008);
  strip.setPixelColor(PixelCount - 1, 0x000100);
  //strip.setPixelColor(PixelCount+1, 0);
  strip.show();

  Serial.begin(9600);
}

void loop()
{
  // Calculate how long no data backet was received
  unsigned long lastPacket = DMXSerial.noDataSince();

  static int lastOk = 1;
  if (lastPacket < 3000) {
    bool hasChange = lastOk != 0;
    lastOk = 0;
    uint32_t c = 0; //pixel color return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
    for (int i = BASE_CH; i < BASE_CH + 3 && i <= 512; i++) {
      byte b = DMXSerial.read(i);
      c = (c << 8) | b;
    }
    static uint32_t old = 0;
    if (old != c) {
      colorWipe(c, 0, 0);
      old = c;
    }

    for (int r = 0; r < RELAYS; r++) {
      bool nrelay = DMXSerial.read(relaychs[r]) != 0;
      SetRelay(r, nrelay);
    }
    if (hasChange) {
      strip.show();
    }
  } else if (lastOk == 0) {
    lastOk = 1;
  } else {
    if (lastOk == 1) {
      // make sure all relays are reset when DMX signal disappears
      for (int r = 0; r < RELAYS; r++) {
        SetRelay(r, false);
      }
    }
    colorWipe(ColorArray[lastOk - 1], 10);
    if (lastOk++ == 9)
      lastOk = 1;
  }
}

