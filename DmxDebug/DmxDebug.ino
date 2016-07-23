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
#if false
  Serial.begin(250000);
#endif

  strip.begin();
#if false
  strip.setPixelColor(0, strip.Color(0xff, 0x00, 0x00));
  strip.show(); // Initialize all pixels to 'off'
  delay(5000);

  strip.setPixelColor(0, strip.Color(0x00, 0xff, 0x00));
  strip.show(); // Initialize all pixels to 'off'
  delay(5000);

  strip.setPixelColor(0, strip.Color(0xff, 0xff, 0x00));
  strip.show(); // Initialize all pixels to 'off'
  delay(5000);

  strip.setPixelColor(0, strip.Color(0x00, 0x00, 0xff));
  strip.show(); // Initialize all pixels to 'off'
  delay(5000);

  strip.setPixelColor(0, strip.Color(0x00, 0xff, 0xff));
  strip.show(); // Initialize all pixels to 'off'
  delay(5000);

  strip.setPixelColor(0, strip.Color(0xff, 0x00, 0xff));
  strip.show(); // Initialize all pixels to 'off'
  delay(5000);
#endif

  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);

  strip.show(); // Initialize all pixels to 'off'
  strip.setPixelColor(0, 0x000008);
  strip.setPixelColor(PixelCount - 1, 0x000100);
  //strip.setPixelColor(PixelCount+1, 0);
  strip.show();
  delay(200);
#if false
  Serial.println(String(PixelCount) + " pixel test started");
#endif

  colorWipe(ColorArray[0], 10); // Red
  colorWipe(ColorArray[8], 10);

#if false
  Serial.println("dmx deserializer ready " + String(PixelCount));
  Serial.println();
#endif
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
#if false
    if (Serial.available()) {
      hasChange = true;
      // clean buffer
      Serial.read();
    }
    String out = "\n01: ";
    // read recent DMX values
    for (int i = 1; i <= PixelCount * 3 && i <= 512; i++) {
      byte b = DMXSerial.read(i);
      out += String((b & 0xf0) >> 8, HEX) + String((b & 0xf), HEX);
      if ((i % 0x10) == 0)
        out += ' ';
      out += ' ';
      if ((i % 0x20) == 0) {
        if (hasChange) {
          Serial.print(out);
        }
        out = "\n" + String(i, HEX) + ": ";
      }

      c = (c << 8) | b;
      if ((i % 3) == 0) {
        uint32_t old = strip.getPixelColor(i / 3 - 1);
        if ((hasChange && c) || old != c) {
          out += "[" + String(i - 2) + "/" + String(i / 3) + ":" + String(old, HEX) + ":" + String(c, HEX) + "] ";
          strip.setPixelColor(i / 3 - 1, c);
          hasChange = true;
        }
        c = 0;
      }
    }
#else
    for (int i = BASE_CH; i < BASE_CH + 3 && i <= 512; i++) {
      byte b = DMXSerial.read(i);
      c = (c << 8) | b;
    }
    if (old != c) {
      //Serial.println("[" + String(BASE_CH) + ":" + String(old, HEX) + ":" + String(c, HEX) + "] ");
      colorWipe(c, 0, 512);
      old = c;
    }
    c = 0;
    bool nrelay = DMXSerial.read(BASE_CH + 3) != 0;
    if (relay != nrelay) {
      digitalWrite(RELAYPIN, nrelay ? HIGH : LOW);
      relay = nrelay;
    }
#endif
    if (hasChange) {
      strip.show();
#if false
      Serial.print(out);
#endif
    }
  } else if (lastOk == 0) {
    lastOk = 1;
#if false
    Serial.print(" last DMX " + String(lastPacket) + " ms ago.\n");
#endif
  } else {
    colorWipe(ColorArray[lastOk - 1], 10);
    if (lastOk++ == 9)
      lastOk = 1;
  }
}

