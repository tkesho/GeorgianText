// GeorgianGFX.h - print Georgian (Mkhedruli) UTF-8 text on any Adafruit_GFX display
// (SSD1306/SH1106 OLED, ST7735/ST7789/ILI9341 TFT, ...).
//
//   #include <GeorgianGFX.h>
//   #include <fonts/GeorgianFont9x18.h>
//   GeorgianGFX geo(display);
//   geo.setFont(&GeorgianFont9x18);
//   display.setCursor(0, 20);          // y = baseline, as with every Adafruit GFX font
//   geo.print("გამარჯობა");
//
// The bundled fonts contain ASCII plus the 33 Georgian letters, so mixed text works.
// Colour, textSize, wrapping and cursor all come from the underlying display object.
// Header-only on purpose: sketches that only use character LCDs never need Adafruit_GFX.
#pragma once
#include <Adafruit_GFX.h>
#include "GeorgianCore.h"

class GeorgianGFX : public Print {
public:
  explicit GeorgianGFX(Adafruit_GFX &gfx) : _gfx(gfx) {}

  void setFont(const GFXfont *f) { _font = f; _gfx.setFont(f); }
  const GFXfont *font() const { return _font; }
  Adafruit_GFX &gfx() { return _gfx; }

  size_t write(uint8_t b) override {
    uint32_t cp;
    if (!_dec.feed(b, cp)) return 1;
    _gfx.write(map(cp));
    return 1;
  }
  using Print::write;

  // Width in pixels of a UTF-8 string with the current font and text size (no wrapping).
  int16_t textWidth(const char *utf8, uint8_t textSize = 1) const {
    if (!_font) return (int16_t)(Georgian::utf8Length(utf8) * 6 * textSize);
    GeoUTF8Decoder dec; uint32_t cp; int16_t w = 0;
    uint16_t first = pgm_read_word(&_font->first), last = pgm_read_word(&_font->last);
    for (; *utf8; utf8++) {
      if (!dec.feed((uint8_t)*utf8, cp)) continue;
      uint8_t c = map(cp);
      if (c < first || c > last) continue;
      const GFXglyph *glyph = glyphAt(_font, (uint16_t)(c - first));
      w += (int16_t)pgm_read_byte(&glyph->xAdvance) * textSize;
    }
    return w;
  }

  // Print a UTF-8 string horizontally centred on the display at baseline y.
  void printCentered(int16_t y, const char *utf8, uint8_t textSize = 1) {
    int16_t w = textWidth(utf8, textSize);
    _gfx.setCursor((_gfx.width() - w) / 2, y);
    print(utf8);
  }

  static uint8_t map(uint32_t cp) {
    int8_t idx = Georgian::letterIndex(cp);
    if (idx >= 0) return (uint8_t)(GEO_FONT_FIRST + idx);
    if (cp == '\n' || (cp >= 0x20 && cp < 0x7F)) return (uint8_t)cp;
    if (cp == '\r') return '\r';
    return '?';
  }

private:
  // Same rule as Adafruit_GFX's pgm_read_glyph_ptr: 16-bit PROGMEM pointers on
  // AVR, 32-bit pgm_read on ESP8266, direct access where flash is mapped.
  static const GFXglyph *glyphAt(const GFXfont *font, uint16_t index) {
#if defined(__AVR__)
    return &((const GFXglyph *)pgm_read_word(&font->glyph))[index];
#elif defined(ESP8266)
    return &((const GFXglyph *)pgm_read_dword(&font->glyph))[index];
#else
    return font->glyph + index;
#endif
  }

  Adafruit_GFX &_gfx;
  const GFXfont *_font = nullptr;
  GeoUTF8Decoder _dec;
};
