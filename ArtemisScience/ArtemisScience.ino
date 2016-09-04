#define BPM_BRIGHTNESS   7 // 0=min, 15=max
#define BPM_I2C_ADDR   0x70 // Edit if backpack A0/A1 jumpers set

#include <Wire.h>
#include "anim.h"       // Animation data is located here

// https://github.com/NicoHood/HID
#include "HID-Project.h"

#include <Adafruit_NeoPixel.h>
#include <DMXSerial.h>

#define BASE_CH 4 // 1 based DMX channel, max 512
#define RELAY_BASE_CH 9
#define NEOPIN 10
#define RELAYS 2
const uint8_t relaypins[] = {16, 255};
const bool relayinverted[] = {true};
#define PixelCount 10

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PixelCount, NEOPIN, NEO_GRB + NEO_KHZ800);

#define KPROWS 3
#define KPCOLS 3
#define KPPINS KPROWS + KPCOLS
const uint8_t kp_pins[KPPINS] = {4, 5, 6, 7, 8, 9};

const KeyboardKeycode kkmap[KPROWS][KPCOLS] = {
  {KEY_1, KEY_2, KEY_3},
  {KEY_4, KEY_5, KEY_6},
  {KEY_7, KEY_8, KEY_9},
};

#define readAnim(anim) pgm_read_byte(&anim[i++])
#define animFrame(anim)                                      \
  Wire.beginTransmission(BPM_I2C_ADDR);                       \
  Wire.write(0);             /* Start address */               \
  for(uint8_t j=0; j<8; j++) {    /* 8 rows... */               \
    Wire.write(pgm_read_byte(&reorder[readAnim(animscan)]));     \
    Wire.write(0);                                                \
  }                                                                \
  Wire.endTransmission();


void ledCmd(uint8_t x) { // Issue command to LED backback driver
  Wire.beginTransmission(BPM_I2C_ADDR);
  Wire.write(x);
  Wire.endTransmission();
}

void ledClear() { // Clear display buffer
  Wire.beginTransmission(BPM_I2C_ADDR);
  for (uint8_t i = 0; i < 17; i++) Wire.write(0);
  Wire.endTransmission();
}

void ledSetup() {
  ledClear();                   // Blank display
  ledCmd(0x21);              // Turn on oscillator
  ledCmd(0xE0 | BPM_BRIGHTNESS); // Set brightness
  ledCmd(0x81);              // Display on, no blink

}

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

bool oldrelay[RELAYS];

void setup() {
  // Sends a clean report to the host. This is important on any Arduino type.
  BootKeyboard.begin();

  DMXSerial.init(DMXReceiver);

  strip.begin();

  for (int r = 0; r < RELAYS; r++) {
    uint8_t pin = relaypins[r];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, true == relayinverted[r] ? LOW : HIGH);
    delay(200);
    digitalWrite(pin, false == relayinverted[r] ? LOW : HIGH);
    oldrelay[r] = false;
  }

  strip.setPixelColor(0, 0x000008);
  strip.setPixelColor(PixelCount - 1, 0x000100);
  //strip.setPixelColor(PixelCount+1, 0);
  strip.show();

  Serial.begin(9600);

  for (int i = 0; i < KPPINS; i++) {
    pinMode(kp_pins[i], INPUT_PULLUP);
  }

  Wire.begin();         // I2C init
  ledSetup();
}


String hex_string(uint64_t v) {
  // this is less then suboptimal code
  String ret = "";
  for (int i = 15; i >= 0; i--) {
    ret += String((int)((v >> (i * 4)) & 0xf), HEX);
    if (i > 0 && i % 2 == 0) ret += " ";
  }
  return ret;
}

void handleKeys() {
  //bool hidBoot = (Keyboard.getProtocol() == HID_BOOT_PROTOCOL)

  uint64_t state = 0;
  uint16_t pressed = 0;
  for (int io = 0; io < KPPINS; io++) {
    int sample = 0;
    int mask = 0x1 << (KPPINS - 1 - io);
    // set output and force low
    pinMode(kp_pins[io], OUTPUT);
    digitalWrite(kp_pins[io], LOW);

    // check if any other pin is forced low
    for (int ii = 0; ii < KPPINS; ii++) {
      sample = sample << 1;
      if (ii == io) {
        continue;
      }
      if (!digitalRead(kp_pins[ii])) {
        sample |= 0x1;
      }
    }
    // restore input
    pinMode(kp_pins[io], INPUT_PULLUP);
    int smask = sample | mask;
    state = state << KPPINS;
    if (sample) {
      state |= smask;
      for (int x = 0; x < KPCOLS; x++) {
        for (int y = 0; y < KPROWS; y++) {
          byte z = (1 << (x + KPCOLS)) | (1 << y);
          if ((smask & z) == z) {
            pressed |= 1 << (x * KPCOLS + y);
          }
        }
      }
    }
  }

  static uint16_t lpressed;
  if (pressed != lpressed) {
    uint16_t change = lpressed ^ pressed;
    Serial.println(hex_string(state));
    Serial.print(" " + String(pressed, HEX) + "   ");
    Serial.print(" ch: " + String(change, HEX) + " ");
    Serial.println();
    for (int i = KPCOLS * KPROWS - 1 ; i >= 0 ; i--) {
      uint16_t z = 1 << i;
      int x = i / KPCOLS;
      int y = i % KPROWS;
      if ((change & z) == z) {
        // this seemed nice to have (but creates a different HID device)
        Keyboard.wakeupHost();
        // Serial.println(" " + kmap[x][y] + " " + (((pressed & z) == z) ? "pressed" : "released"));
        KeyboardKeycode k = kkmap[x][y];
        if ((pressed & z) == z)
          BootKeyboard.press(k);
        else
          BootKeyboard.release(k);
      }
    }
    lpressed = pressed;
    // delay(20);
  }
}

void readDMX() {
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
      bool nrelay = DMXSerial.read(RELAY_BASE_CH + r) != 0;
      if (oldrelay[r] != nrelay) {
        Serial.println("relay change to " + String(nrelay ? "on" : "off"));
        if (relaypins[r] <= 127)
          digitalWrite(relaypins[r], nrelay == relayinverted[r] ? LOW : HIGH);
        // TODO handle special case calls that we want to react to with non relays
        oldrelay[r] = nrelay;
      }
    }
    if (hasChange) {
      strip.show();
    }
  } else if (lastOk == 0) {
    lastOk = 1;
  } else {
    colorWipe(strip.Color(0, 64, 0), 10, 3);
  }
}

uint8_t rep = REPS;

void loop() {
  readDMX();
  for (int i = 0; i < sizeof(animscan); i) { // For each frame...
    animFrame(animscan);
    // todo use millis for delay instead since we need it to be responsive
    delay(readAnim(animscan) * 10);
  }

  if (!--rep) {            // If last cycle...
    delay(5000);
    ledClear();            // Blank display
    //ledCmd(0x21);          // Re-enable matrix
  }
}

