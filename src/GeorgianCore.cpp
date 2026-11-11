#include "GeorgianCore.h"

// Keyboard-layout Latin key for each letter ა..ჰ (standard Georgian layout).
static const char LAYOUT[GEO_LETTER_COUNT] PROGMEM = {
  'a','b','g','d','e','v','z','T','i','k','l','m','n','o','p','J','r',
  's','t','u','f','q','R','y','S','C','c','Z','w','W','x','j','h'
};

// Romanization, national system (2002), 4 bytes per entry incl. NUL.
static const char ROMAN[GEO_LETTER_COUNT][4] PROGMEM = {
  "a","b","g","d","e","v","z","t","i","k'","l","m","n","o","p'","zh","r",
  "s","t'","u","p","k","gh","q'","sh","ch","ts","dz","ts'","ch'","kh","j","h"
};

namespace Georgian {

int8_t letterIndex(uint32_t cp) {
  if (cp >= GEO_FIRST_CP && cp < GEO_FIRST_CP + GEO_LETTER_COUNT) return (int8_t)(cp - GEO_FIRST_CP);
  if (cp >= GEO_MTAVRULI_CP && cp < GEO_MTAVRULI_CP + GEO_LETTER_COUNT) return (int8_t)(cp - GEO_MTAVRULI_CP);
  return -1;
}

char layoutChar(uint8_t idx) {
  return idx < GEO_LETTER_COUNT ? (char)pgm_read_byte(&LAYOUT[idx]) : '?';
}

const char *romanize(uint8_t idx, char *buf) {
  if (idx >= GEO_LETTER_COUNT) { buf[0] = '?'; buf[1] = 0; return buf; }
  for (uint8_t i = 0; i < 4; i++) buf[i] = (char)pgm_read_byte(&ROMAN[idx][i]);
  return buf;
}

uint8_t encodeUTF8(uint32_t cp, char *o) {
  if (cp < 0x80) { o[0] = (char)cp; return 1; }
  if (cp < 0x800) { o[0] = (char)(0xC0 | (cp >> 6)); o[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
  if (cp < 0x10000) {
    o[0] = (char)(0xE0 | (cp >> 12)); o[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    o[2] = (char)(0x80 | (cp & 0x3F)); return 3;
  }
  o[0] = (char)(0xF0 | (cp >> 18)); o[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  o[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); o[3] = (char)(0x80 | (cp & 0x3F)); return 4;
}

size_t fromLayout(const char *in, char *out, size_t outSize) {
  if (!outSize) return 0;
  size_t n = 0;
  for (; *in; in++) {
    int8_t idx = -1;
    for (uint8_t i = 0; i < GEO_LETTER_COUNT; i++)
      if ((char)pgm_read_byte(&LAYOUT[i]) == *in) { idx = (int8_t)i; break; }
    char tmp[4]; uint8_t len;
    if (idx >= 0) len = encodeUTF8(GEO_FIRST_CP + idx, tmp);
    else { tmp[0] = *in; len = 1; }
    if (n + len >= outSize) break;
    memcpy(out + n, tmp, len); n += len;
  }
  out[n] = 0;
  return n;
}

size_t toFontEncoding(const char *utf8, char *out, size_t outSize) {
  if (!outSize) return 0;
  GeoUTF8Decoder dec; size_t n = 0; uint32_t cp;
  for (; *utf8 && n + 1 < outSize; utf8++) {
    if (!dec.feed((uint8_t)*utf8, cp)) continue;
    for (;;) {
      int8_t idx = letterIndex(cp);
      if (idx >= 0) out[n++] = (char)(GEO_FONT_FIRST + idx);
      else if (cp == '\n' || (cp >= 0x20 && cp < 0x7F)) out[n++] = (char)cp;
      else if (cp != '\r') out[n++] = '?';
      if (n + 1 >= outSize || !dec.pending(cp)) break;
    }
  }
  out[n] = 0;
  return n;
}

size_t utf8Length(const char *s) {
  size_t n = 0;
  for (; *s; s++) if (((uint8_t)*s & 0xC0) != 0x80) n++;
  return n;
}

} // namespace Georgian

bool GeoUTF8Decoder::pending(uint32_t &cp) {
  if (!_pending) return false;
  _pending = false;
  cp = _pendingCp;
  return true;
}

bool GeoUTF8Decoder::feed(uint8_t b, uint32_t &cp) {
  if (_need) {
    if ((b & 0xC0) == 0x80) {
      _cp = (_cp << 6) | (b & 0x3F);
      if (--_need) return false;
      cp = (_cp < _min || _cp > 0x10FFFF || (_cp >= 0xD800 && _cp <= 0xDFFF)) ? GEO_REPLACEMENT : _cp;
      return true;
    }
    _need = 0;   // truncated sequence: emit U+FFFD, then decode b
    uint32_t next = 0;
    if (feed(b, next)) { _pending = true; _pendingCp = next; }
    cp = GEO_REPLACEMENT;
    return true;
  }
  if (b < 0x80) { cp = b; return true; }
  if ((b & 0xE0) == 0xC0) { _cp = b & 0x1F; _need = 1; _min = 0x80; return false; }
  if ((b & 0xF0) == 0xE0) { _cp = b & 0x0F; _need = 2; _min = 0x800; return false; }
  if ((b & 0xF8) == 0xF0) { _cp = b & 0x07; _need = 3; _min = 0x10000; return false; }
  cp = GEO_REPLACEMENT;      // stray continuation byte or invalid lead
  return true;
}

void GeorgianSerialReader::finish() {
  // Never leave a partial multi-byte sequence at the end of the line.
  size_t i = _len;
  while (i > 0 && ((uint8_t)_buf[i - 1] & 0xC0) == 0x80) i--;
  if (i > 0) {
    uint8_t lead = (uint8_t)_buf[i - 1];
    uint8_t want = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
    if (_len - (i - 1) < want) _len = i - 1;
  }
  _buf[_len] = 0;
  _ready = true;
}

bool GeorgianSerialReader::poll() {
  if (_ready) { _len = 0; _overflow = false; _ready = false; _buf[0] = 0; }
  while (_in.available() > 0) {
    int c = _in.read();
    if (c < 0) break;
    _last = millis();
    if (c == '\r') continue;
    if (c == '\n') { finish(); return true; }
    if (_len + 1 < _size) _buf[_len++] = (char)c;
    else _overflow = true;
  }
  if (_len > 0 && _timeout && (uint32_t)(millis() - _last) >= _timeout) { finish(); return true; }
  return false;
}
