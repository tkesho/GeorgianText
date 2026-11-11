// Stand-in so GeorgianGFX.h can be compiled by the host tests.
#pragma once
#include "Arduino.h"
#include <stdint.h>

struct GFXglyph {
  uint16_t bitmapOffset;
  uint8_t width, height, xAdvance;
  int8_t xOffset, yOffset;
};

struct GFXfont {
  uint8_t *bitmap;
  GFXglyph *glyph;
  uint16_t first, last;
  uint8_t yAdvance;
};

class Adafruit_GFX : public Print {
public:
  explicit Adafruit_GFX(int16_t w = 128, int16_t h = 64) : _w(w), _h(h) {}
  void setFont(const GFXfont *) {}
  void setCursor(int16_t x, int16_t y) { cx = x; cy = y; }
  void setTextSize(uint8_t s) { setTextSize(s, s); }
  void setTextSize(uint8_t sx, uint8_t sy) {
    textsize_x = sx ? sx : 1;
    textsize_y = sy ? sy : 1;
  }
  int16_t width() const { return _w; }
  size_t write(uint8_t c) override { last = c; sizeAtWrite = textsize_x; n++; return 1; }
  uint8_t sizeX() const { return textsize_x; }
  uint8_t sizeY() const { return textsize_y; }
  int16_t cx = 0, cy = 0;
  uint8_t last = 0, sizeAtWrite = 1;
  int n = 0;
protected:
  uint8_t textsize_x = 1, textsize_y = 1;
  int16_t _w, _h;
};
