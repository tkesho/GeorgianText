// Type Georgian in the Serial Monitor (115200 baud, any line ending) and see it on a 16x2 I2C LCD.
// Real Georgian (UTF-8) works directly. No Georgian keyboard? Start the line with '~' and type
// with the Georgian layout in Latin letters: "~gamarjoba" -> "გამარჯობა", "~saqarTvelo".
// Line 1 shows the text, line 2 shows the romanization of the first letters.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <GeorgianLCD.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
GeorgianLCD<LiquidCrystal_I2C> glcd(lcd, 16, 2);

char inBuf[64];                       // raw line from Serial (UTF-8 bytes)
char geoBuf[100];                     // after optional layout conversion (3 bytes per letter)
GeorgianSerialReader reader(Serial, inBuf, sizeof(inBuf));

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  glcd.begin();
  glcd.print("ჩაწერე ტექსტი");     // "type text"
  Serial.println(F("GeorgianLCD ready. Type a line (prefix ~ for layout transliteration)."));
}

void loop() {
  if (!reader.poll()) return;

  const char *text = reader.line();
  if (text[0] == '~') {
    Georgian::fromLayout(text + 1, geoBuf, sizeof(geoBuf));
    text = geoBuf;
  }

  glcd.clear();
  glcd.setWrap(false);
  glcd.print(text);

  // Second row: romanized text (pure ASCII, no CGRAM needed).
  lcd.setCursor(0, 1);
  GeoUTF8Decoder dec; uint32_t cp; char rom[5]; uint8_t used = 0;
  for (const char *p = text; *p && used < 16; p++) {
    if (!dec.feed((uint8_t)*p, cp)) continue;
    int8_t idx = Georgian::letterIndex(cp);
    const char *s = idx >= 0 ? Georgian::romanize(idx, rom) : (rom[0] = (cp < 0x7F ? (char)cp : '?'), rom[1] = 0, rom);
    for (; *s && used < 16; s++, used++) lcd.write(*s);
  }

  Serial.print(F("Shown: ")); Serial.print(text);
  Serial.print(F("  (")); Serial.print(Georgian::utf8Length(text)); Serial.print(F(" chars, "));
  Serial.print(glcd.slotsInUse()); Serial.println(F(" CGRAM slots)"));
  if (reader.overflowed()) Serial.println(F("(line truncated)"));
}
