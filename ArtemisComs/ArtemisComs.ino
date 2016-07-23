
// https://github.com/NicoHood/HID
#include "HID-Project.h"

const int pins[] = {2, 3, 4, 5, 6, 7, 8, 9};
/*
      7 6 5 4
  3 - 1 2 3 A
  2 - 4 5 6 B
  1 - 7 8 9 C
  0 - * 0 # D
*/
const String kmap[][4] = {
  {"1", "2", "3", "A"},
  {"4", "5", "6", "B"},
  {"7", "8", "9", "C"},
  {"*", "0", "#", "D"},
};
const KeyboardKeycode kkmap[][4] = {
  {KEYPAD_1, KEYPAD_2, KEYPAD_3, KEYPAD_DIVIDE},
  {KEYPAD_4, KEYPAD_5, KEYPAD_6, KEYPAD_MULTIPLY},
  {KEYPAD_7, KEYPAD_8, KEYPAD_9, KEYPAD_SUBTRACT},
  {KEY_PERIOD, KEYPAD_0, KEYPAD_ENTER, KEYPAD_ADD},
}; // we want a . (dot) more often then a KEPAD_DOT which is , (comma) on swedish layout

void setup() {
  // Sends a clean report to the host. This is important on any Arduino type.
  BootKeyboard.begin();

  Serial.begin(9600);
  for (int i = 0; i < 8; i++) {
    pinMode(pins[i], INPUT_PULLUP);
  }
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

uint16_t lpressed;

void loop() {
  //bool hidBoot = (Keyboard.getProtocol() == HID_BOOT_PROTOCOL)

  uint64_t state = 0;
  uint16_t pressed = 0;
  for (int io = 0; io < 8; io++) {
    int sample = 0;
    int mask = 0x1 << (7 - io);
    // set output and force low
    pinMode(pins[io], OUTPUT);
    digitalWrite(pins[io], LOW);

    // check if any other pin is forced low
    for (int ii = 0; ii < 8; ii++) {
      sample = sample << 1;
      if (ii == io) {
        continue;
      }
      if (!digitalRead(pins[ii])) {
        sample |= 0x1;
      }
    }
    // restore input
    pinMode(pins[io], INPUT_PULLUP);
    int smask = sample | mask;
    state = state << 8;
    if (sample) {
      state |= smask;
      for (int x = 0; x < 4; x++) {
        for (int y = 0; y < 4; y++) {
          byte z = (1 << (x + 4)) | (1 << y);
          if ((smask & z) == z) {
            pressed |= 1 << (x * 4 + y);
          }
        }
      }
    }
  }

  if (pressed != lpressed) {
    uint16_t change = lpressed ^ pressed;
    Serial.println(hex_string(state));
    Serial.print(" " + String(pressed, HEX) + "   ");
    Serial.print(" ch: " + String(change, HEX) + " ");
    Serial.println();
    for (int i = 15; i >= 0 ; i--) {
      uint16_t z = 1 << i;
      int x = i / 4;
      int y = i % 4;
      if ((change & z) == z) {
        // this seemed nice to have (but creates a different HID device)
        Keyboard.wakeupHost();
        uint8_t leds = BootKeyboard.getLeds();
        // ensure we always have numlock on
        if (!(leds & LED_NUM_LOCK)) {
          Serial.println("leds: " + String(leds, BIN));
          BootKeyboard.write(KEY_NUM_LOCK);
        }
        Serial.println(" " + kmap[x][y] + " " + (((pressed & z) == z) ? "pressed" : "released"));
        KeyboardKeycode k = kkmap[x][y];
        if ((pressed & z) == z)
          BootKeyboard.press(k);
        else
          BootKeyboard.release(k);
      }
    }
    lpressed = pressed;
  }
}
