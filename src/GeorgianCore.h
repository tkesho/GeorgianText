// GeorgianCore.h - UTF-8 decoding, Georgian letter lookup, transliteration and Serial
// line input. No display dependencies; works on AVR (Uno/Nano) and ESP32.
#pragma once
#include <Arduino.h>

#if defined(__AVR__)
  #include <avr/pgmspace.h>
#endif
#ifndef PROGMEM
  #define PROGMEM
#endif
#ifndef pgm_read_byte
  #define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif

#define GEO_LETTER_COUNT 33          // ა (U+10D0) .. ჰ (U+10F0)
#define GEO_FIRST_CP     0x10D0UL
#define GEO_MTAVRULI_CP  0x1C90UL    // Mtavruli capitals Ა..Ჰ map onto the same 33 glyphs
#define GEO_FONT_FIRST   0x7F        // GFX font code for ა; ჰ = 0x9F
#define GEO_REPLACEMENT  0xFFFDUL

// 5x8 glyph bitmaps for HD44780 CGRAM (flash on AVR). Row-major, low 5 bits used.
extern const uint8_t GEORGIAN_LCD_GLYPHS[GEO_LETTER_COUNT][8] PROGMEM;

namespace Georgian {

// Returns 0..32 for a modern Georgian letter (Mkhedruli or Mtavruli), else -1.
int8_t letterIndex(uint32_t cp);

// Single ASCII char for a letter, following the standard Georgian keyboard layout
// (ა=a, თ=T, შ=S, ...). Used as the LCD fallback when all 8 CGRAM slots are busy.
char layoutChar(uint8_t idx);

// Romanization (Georgian national system, e.g. "sh", "ts'"), from flash.
// Copies into buf (>= 4 bytes) and returns buf.
const char *romanize(uint8_t idx, char *buf);

// Encode a code point as UTF-8 into out (>= 4 bytes). Returns byte count.
uint8_t encodeUTF8(uint32_t cp, char *out);

// Convert Latin text typed with the Georgian keyboard layout ("gamarjoba") into
// Georgian UTF-8 ("გამარჯობა"). Unmapped characters are copied unchanged.
// Returns bytes written (out is always NUL-terminated when outSize > 0).
size_t fromLayout(const char *in, char *out, size_t outSize);

// Convert UTF-8 into the single-byte encoding used by the bundled GFX fonts
// (ASCII as-is, Georgian -> 0x7F..0x9F, anything else -> '?'). Lets you call
// Adafruit_GFX's own print()/getTextBounds() on the result.
size_t toFontEncoding(const char *utf8, char *out, size_t outSize);

// Number of code points (not bytes) in a UTF-8 string.
size_t utf8Length(const char *utf8);

} // namespace Georgian

// Streaming UTF-8 decoder: feed bytes one at a time.
class GeoUTF8Decoder {
public:
  // Returns true when a full code point is available in cp. Malformed input yields
  // U+FFFD and resynchronises on the next lead byte.
  bool feed(uint8_t b, uint32_t &cp);
  void reset() { _need = 0; }
private:
  uint32_t _cp = 0;
  uint8_t _need = 0;
  uint32_t _min = 0;
};

// Reads lines of UTF-8 text from Serial (or any Stream). Works with the Arduino Serial
// Monitor set to "Newline", "Both NL & CR", or "No line ending" (idle timeout).
class GeorgianSerialReader {
public:
  GeorgianSerialReader(Stream &in, char *buf, size_t bufSize, uint16_t idleTimeoutMs = 60)
    : _in(in), _buf(buf), _size(bufSize), _timeout(idleTimeoutMs) { clear(); }

  // Call often from loop(). Returns true once when a complete line is ready.
  bool poll();
  const char *line() const { return _buf; }   // NUL-terminated UTF-8, no CR/LF
  size_t length() const { return _len; }
  bool overflowed() const { return _overflow; }
  void clear() { _len = 0; _overflow = false; _ready = false; if (_size) _buf[0] = 0; }

private:
  Stream &_in;
  char *_buf;
  size_t _size, _len = 0;
  uint16_t _timeout;
  uint32_t _last = 0;
  bool _overflow = false, _ready = false;
  void finish();
};
