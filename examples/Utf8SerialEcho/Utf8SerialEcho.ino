// No display needed: check what the board receives from the Serial Monitor (115200 baud).
// For every code point it prints U+XXXX, whether it is a Georgian letter, its romanization
// and its keyboard-layout key. Useful to verify your Serial Monitor really sends UTF-8.
#include <GeorgianCore.h>

char buf[64];
GeorgianSerialReader reader(Serial, buf, sizeof(buf));

void setup() {
  Serial.begin(115200);
  Serial.println(F("Type Georgian text, e.g. გამარჯობა"));
}

void loop() {
  if (!reader.poll()) return;
  Serial.print(F("Line: ")); Serial.println(reader.line());
  GeoUTF8Decoder dec; uint32_t cp; char rom[4];
  for (const char *p = reader.line(); *p; p++) {
    if (!dec.feed((uint8_t)*p, cp)) continue;
    do {
      char hex[8]; snprintf(hex, sizeof(hex), "U+%04lX", (unsigned long)cp);
      Serial.print(hex);
      int8_t idx = Georgian::letterIndex(cp);
      if (idx >= 0) {
        Serial.print(F("  letter #")); Serial.print(idx);
        Serial.print(F("  ")); Serial.print(Georgian::romanize(idx, rom));
        Serial.print(F("  key '")); Serial.print(Georgian::layoutChar(idx)); Serial.print('\'');
      }
      Serial.println();
    } while (dec.pending(cp));
  }
}
