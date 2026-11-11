# GeorgianLCD

Georgian (Mkhedruli, ა–ჰ) UTF-8 text for Arduino displays:

* **HD44780 character LCDs** (16x2, 20x4; parallel `LiquidCrystal` or I2C `LiquidCrystal_I2C`)
* **Graphic displays** through Adafruit_GFX: SSD1306/SH1106 OLED, ST7735/ST7789/ILI9341 TFT, …
* **Georgian input from the Serial Monitor** (UTF-8), plus Latin keyboard-layout input (`gamarjoba` → `გამარჯობა`)

Boards: **Arduino Uno, Arduino Nano (ATmega328P) and ESP32** — every example compiles for all three.
All fonts and tables are `PROGMEM` (flash) on AVR; nothing font-related is copied into the Uno's 2 KB RAM.

## Install
After the library is in the Arduino Library Manager, install **GeorgianLCD** from *Sketch → Include Library → Manage Libraries*. Until then, download the repository ZIP from https://github.com/tkesho/GeorgianLCD (*Code → Download ZIP*) and use *Sketch → Include Library → Add .ZIP Library*.
`library.properties` lists the display libraries the examples compile against: `LiquidCrystal`, `LiquidCrystal I2C`, `Adafruit GFX Library`, `Adafruit SSD1306`, and `Adafruit ST7735 and ST7789 Library`. The Library Manager installs those with GeorgianLCD. A character-LCD sketch only needs the LCD library you actually wire up; an OLED or TFT sketch needs Adafruit GFX plus that display's library.
Save your sketches as UTF-8 (the Arduino IDE does this by default).

## Character LCD (HD44780)
```cpp
#include <LiquidCrystal_I2C.h>
#include <GeorgianLCD.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
GeorgianLCD<LiquidCrystal_I2C> glcd(lcd, 16, 2);

void setup() { lcd.init(); lcd.backlight(); glcd.begin(); glcd.print("გამარჯობა"); }
```
HD44780 chips have only **8 custom-character slots**. GeorgianLCD loads glyphs on demand and tracks the
screen, so **up to 8 different Georgian letters can be visible at once** (repeats are free: "გამარჯობა" uses 7).
When a 9th distinct letter is needed it reuses a slot whose letter is no longer on screen; if all 8 are still
visible it prints a fallback — the Latin keyboard key (`თ`→`T`, default) or `?` (`setFallback(GEO_FALLBACK_QUESTION)`).
Latin letters, digits and punctuation come from the LCD ROM and never use slots.

API: `begin() clear() home() setCursor(c,r) print(...) clearRow(r) printCentered(r, s) setWrap(bool) setFallback(f) slotsInUse()`.
`slotsInUse()` counts loaded CGRAM slots, not letters currently visible — a slot stays occupied until a later letter reuses it.
`printCentered` stays on its own row (extra characters are dropped instead of wrapping).
Always print through `glcd` (not `lcd`) so the screen tracking stays correct. RAM: ~100 bytes per instance
(set `#define GEORGIAN_LCD_MAX_CELLS 32` before the include to save 48 bytes on a 16x2; the 16x2 examples do this).

## OLED / TFT (Adafruit_GFX)
```cpp
#include <Adafruit_SSD1306.h>
#include <GeorgianGFX.h>
#include <fonts/GeorgianFont9x18.h>      // or GeorgianFont6x13.h
Adafruit_SSD1306 display(128, 64, &Wire, -1);
GeorgianGFX geo(display);
// setup(): display.begin(...); geo.setFont(&GeorgianFont9x18);
display.setCursor(0, 16);                // y = baseline
geo.print("გამარჯობა, Hello!");
display.display();
```
Fonts contain ASCII + all 33 letters (Mtavruli capitals Ა–Ჰ map to the same glyphs), so mixed text works.
Colour, `setTextSize`, wrapping and cursor come from the display. Helpers: `geo.textWidth(s)`, `geo.printCentered(y, s, textSize)`
(applies that size for the draw, then restores the previous size), `geo.resetDecoder()`,
and `Georgian::toFontEncoding()` if you want to call Adafruit_GFX's own `print()`/`getTextBounds()`. Carriage return is ignored.

| Font | Line height | Flash (bitmaps + glyph table) |
|---|---|---|
| GeorgianFont6x13 | 13 px | 648 + 896 B |
| GeorgianFont9x18 | 18 px | 974 + 896 B |

## Serial input
```cpp
char buf[64];
GeorgianSerialReader reader(Serial, buf, sizeof(buf));
void loop() { if (reader.poll()) glcd.print(reader.line()); }
```
Works with any Serial Monitor line-ending setting; never splits a multi-byte letter when the buffer is full.
`Georgian::fromLayout("saqarTvelo", out, n)` converts Georgian-keyboard-layout Latin to UTF-8 (`საქართველო`).
Other helpers: `Georgian::letterIndex(cp)`, `romanize(idx, buf)` (national system: `შ`→`sh`), `layoutChar(idx)`, `utf8Length(s)`, `GeoUTF8Decoder`.

## Examples and measured size (arduino-cli 1.6.0, AVR core 1.8.8, ESP32 core 3.3.12)
| Example | Uno / Nano flash | Uno / Nano RAM (static) | ESP32 flash | ESP32 RAM |
|---|---|---|---|---|
| HelloLCD_I2C | 5,614 B | 414 B | 286,904 B | 23,604 B |
| HelloLCD_Parallel | 3,946 B | 232 B | 260,932 B | 22,220 B |
| SerialToLCD | 7,546 B | 741 B | 297,736 B | 23,796 B |
| HelloOLED_SSD1306 | 18,824 B | 717 B ¹ | 310,620 B | 23,708 B |
| SerialToOLED | 17,268 B | 781 B ¹ | 308,864 B | 23,884 B |
| HelloTFT_ST7735 | 15,120 B | 530 B | 300,736 B | 23,764 B |
| Utf8SerialEcho | 4,666 B | 276 B | 270,468 B | 22,204 B |

Uno: 32,256 B flash / 2,048 B RAM. Nano: 30,720 B flash (old bootloader) / 2,048 B RAM. ESP32: 1,310,720 B / 327,680 B.
¹ Adafruit_SSD1306 additionally `malloc`s a 1,024-byte frame buffer at `begin()` on 128x64 — not in the static figure.
Most of the ESP32 numbers are the ESP32 core itself (an empty sketch is ~250 KB).

Re-run with `tools/compile_all.sh` (needs `arduino-cli` with `arduino:avr` and `esp32:esp32`).
Host-side unit tests: `extras/hosttest/run.sh`.

## Wiring notes
* I2C LCD/OLED: Uno/Nano SDA=A4, SCL=A5 · ESP32 SDA=21, SCL=22. Common LCD addresses 0x27 / 0x3F, OLED 0x3C.
* ESP32 is 3.3 V: 5 V I2C LCD backpacks usually work but add a level shifter if the backpack pulls SDA/SCL to 5 V.
* Pins for parallel LCD and ST7735 per board are in the examples (`#if defined(ARDUINO_ARCH_ESP32)`).

## Editing glyphs
`python3 tools/genfonts.py` regenerates `src/GeorgianLCDGlyphs.cpp` and `src/fonts/*.h` from any working directory.
The 5x8 LCD shapes are hand-tuned in `tools/lcdfinal.py`. GFX bitmaps come from the public-domain Markus Kuhn subsets in `tools/fonts/` (U+0020–U+007E and U+10D0–U+10F0 of ucs-fonts 6x13 and 9x18).
Previews: `extras/lcd_preview.png`, `extras/gfx_6x13_preview.png`, `extras/gfx_9x18_preview.png`.
