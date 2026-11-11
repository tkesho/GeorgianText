// GeorgianLCD.h - print Georgian (Mkhedruli) UTF-8 text on HD44780 character LCDs.
//
// Works with any LCD class that has setCursor(col,row), clear(), write(uint8_t) and
// createChar(uint8_t, uint8_t*): LiquidCrystal, LiquidCrystal_I2C, hd44780, ...
//
//   LiquidCrystal_I2C lcd(0x27, 16, 2);
//   GeorgianLCD<LiquidCrystal_I2C> glcd(lcd, 16, 2);
//   glcd.print("გამარჯობა");
//
// HD44780 controllers have only 8 user-definable characters (CGRAM). GeorgianLCD loads
// letter glyphs on demand and tracks what is on screen, so up to 8 *different*
// Georgian letters can be visible at once (any number of repeats). When a 9th distinct
// letter is needed and every slot is still on screen, the fallback is used: the letter's
// Latin keyboard-layout key (default) or '?'.
#pragma once
#include "GeorgianCore.h"

#ifndef GEORGIAN_LCD_MAX_CELLS
#define GEORGIAN_LCD_MAX_CELLS 80   // 20x4; lower it (e.g. 32 for 16x2) to save RAM
#endif

enum GeoLcdFallback : uint8_t { GEO_FALLBACK_LAYOUT = 0, GEO_FALLBACK_QUESTION = 1 };

template <class LCD>
class GeorgianLCD : public Print {
public:
  GeorgianLCD(LCD &lcd, uint8_t cols, uint8_t rows) : _lcd(lcd), _cols(cols), _rows(rows) {
    if (_cols == 0) _cols = 1;
    if (_cols > GEORGIAN_LCD_MAX_CELLS) _cols = GEORGIAN_LCD_MAX_CELLS;
    uint8_t maxRows = (uint8_t)(GEORGIAN_LCD_MAX_CELLS / _cols);
    if (_rows > maxRows) _rows = maxRows;
    if (_rows == 0) _rows = 1;
    resetState();
  }

  // Call after the LCD itself is initialised (lcd.begin()/lcd.init()). Clears the screen.
  void begin() { clear(); }

  void clear() { _lcd.clear(); resetState(); }
  void home() { setCursor(0, 0); }
  void setCursor(uint8_t col, uint8_t row) {
    _col = col < _cols ? col : _cols - 1; _row = row < _rows ? row : _rows - 1;
    _lcd.setCursor(_col, _row); _dec.reset();
  }
  void setWrap(bool on) { _wrap = on; }                 // wrap to next row at the edge (default on)
  void setFallback(GeoLcdFallback f) { _fallback = f; }

  // Clear one row (fills with spaces) and put the cursor at its start.
  void clearRow(uint8_t row) {
    setCursor(0, row);
    for (uint8_t i = 0; i < _cols; i++) putRaw(' ', 0xFF);
    setCursor(0, row);
  }

  // Print a UTF-8 string centred on a row (clears that row first).
  void printCentered(uint8_t row, const char *utf8) {
    clearRow(row);
    size_t n = Georgian::utf8Length(utf8);
    setCursor(n < _cols ? (uint8_t)((_cols - n) / 2) : 0, row);
    print(utf8);
  }

  uint8_t slotsInUse() const { uint8_t n = 0; for (uint8_t i = 0; i < 8; i++) if (_slot[i] != 0xFF) n++; return n; }
  uint8_t col() const { return _col; }
  uint8_t row() const { return _row; }

  // Print interface: accepts a UTF-8 byte stream.
  size_t write(uint8_t b) override {
    uint32_t cp;
    if (!_dec.feed(b, cp)) return 1;
    writeCodepoint(cp);
    return 1;
  }
  using Print::write;

  void writeCodepoint(uint32_t cp) {
    if (cp == '\r') return;
    if (cp == '\n') { newline(); return; }
    if (_row >= _rows) return;                           // ran off the bottom without wrap
    int8_t idx = Georgian::letterIndex(cp);
    if (idx >= 0) { putLetter((uint8_t)idx); return; }
    putRaw((cp >= 0x20 && cp < 0x7F) ? (uint8_t)cp : '?', 0xFF);
  }

private:
  LCD &_lcd;
  uint8_t _cols, _rows, _col = 0, _row = 0;
  bool _wrap = true;
  GeoLcdFallback _fallback = GEO_FALLBACK_LAYOUT;
  GeoUTF8Decoder _dec;
  uint8_t _slot[8];                                      // letter index loaded in CGRAM slot, 0xFF = free
  uint8_t _screen[GEORGIAN_LCD_MAX_CELLS];               // per cell: letter index, or 0xFF for ROM char
  uint8_t _clock = 0, _age[8];

  void resetState() {
    _col = _row = 0; _dec.reset();
    for (uint8_t i = 0; i < 8; i++) { _slot[i] = 0xFF; _age[i] = 0; }
    memset(_screen, 0xFF, sizeof(_screen));
  }

  void newline() {
    _col = 0; _row++;
    if (_row < _rows) _lcd.setCursor(_col, _row);
  }

  void putRaw(uint8_t ch, uint8_t letter) {
    if (_col >= _cols) {
      if (!_wrap) return;
      newline();
      if (_row >= _rows) return;
    }
    _lcd.write(ch);
    _screen[_row * _cols + _col] = letter;
    _col++;
  }

  bool onScreen(uint8_t letter, uint16_t except) const {
    uint16_t n = (uint16_t)_cols * _rows;
    for (uint16_t i = 0; i < n; i++) if (i != except && _screen[i] == letter) return true;
    return false;
  }

  void putLetter(uint8_t idx) {
    if (_col >= _cols) { if (!_wrap) return; newline(); if (_row >= _rows) return; }
    uint16_t here = _row * _cols + _col;
    int8_t s = -1;
    for (uint8_t i = 0; i < 8; i++) if (_slot[i] == idx) { s = (int8_t)i; break; }
    if (s < 0) {
      // Prefer a free slot, then the least-recently-used slot whose letter is no longer visible.
      for (uint8_t i = 0; i < 8 && s < 0; i++) if (_slot[i] == 0xFF) s = (int8_t)i;
      if (s < 0) {
        uint8_t bestAge = 0;
        for (uint8_t i = 0; i < 8; i++) {
          if (onScreen(_slot[i], here)) continue;
          uint8_t age = (uint8_t)(_clock - _age[i]);
          if (s < 0 || age > bestAge) { s = (int8_t)i; bestAge = age; }
        }
      }
      if (s < 0) {                                       // all 8 slots visible: fall back
        putRaw(_fallback == GEO_FALLBACK_LAYOUT ? (uint8_t)Georgian::layoutChar(idx) : '?', 0xFF);
        return;
      }
      uint8_t glyph[8];
      for (uint8_t r = 0; r < 8; r++) glyph[r] = pgm_read_byte(&GEORGIAN_LCD_GLYPHS[idx][r]);
      _lcd.createChar((uint8_t)s, glyph);
      _slot[s] = idx;
      _lcd.setCursor(_col, _row);                        // createChar moves the address counter
    }
    _age[s] = ++_clock;
    putRaw((uint8_t)s, idx);
  }
};
