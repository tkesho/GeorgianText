#include "Arduino.h"
#include "../../src/GeorgianCore.h"
#include "../../src/GeorgianLCD.h"
#include "../../src/GeorgianGFX.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

struct MockLCD {
  int cols = 16, rows = 2, c = 0, r = 0; uint8_t cg[8][8]; uint8_t ddram[2][16];
  int creates = 0;
  MockLCD() { memset(ddram, ' ', sizeof ddram); }
  void clear() { memset(ddram, ' ', sizeof ddram); c = r = 0; }
  void setCursor(uint8_t cc, uint8_t rr) { c = cc; r = rr; }
  void createChar(uint8_t s, uint8_t *g) { memcpy(cg[s & 7], g, 8); creates++; c = -100; } // address lost
  size_t write(uint8_t ch) { assert(c >= 0 && c < 16 && r >= 0 && r < 2); ddram[r][c++] = ch; return 1; }
};
struct StrStream : Stream {
  std::string s; size_t i = 0;
  int available() override { return (int)(s.size() - i); }
  int read() override { return i < s.size() ? (uint8_t)s[i++] : -1; }
  size_t write(uint8_t) override { return 1; }
};

static void decodeAll(const char *s, std::vector<uint32_t> &v) {
  GeoUTF8Decoder d; uint32_t cp;
  for (; *s; s++) {
    if (!d.feed((uint8_t)*s, cp)) continue;
    do v.push_back(cp); while (d.pending(cp));
  }
}

int main() {
  // UTF-8 decode
  GeoUTF8Decoder d; uint32_t cp; std::vector<uint32_t> v;
  for (const char *p = "აჰA\xE1\xB2\x90"; *p; p++) if (d.feed((uint8_t)*p, cp)) v.push_back(cp);
  assert(v.size() == 4 && v[0] == 0x10D0 && v[1] == 0x10F0 && v[2] == 'A' && v[3] == 0x1C90);
  assert(Georgian::letterIndex(0x1C90) == 0 && Georgian::letterIndex(0x10F1) == -1);
  // truncated lead, overlong, bad lead, stray continuation, surrogate
  v.clear(); decodeAll("\xE1" "A", v);
  assert(v.size() == 2 && v[0] == GEO_REPLACEMENT && v[1] == 'A');
  v.clear(); decodeAll("\xC0\x80", v);
  assert(v.size() == 1 && v[0] == GEO_REPLACEMENT);
  v.clear(); decodeAll("\xFF", v);
  assert(v.size() == 1 && v[0] == GEO_REPLACEMENT);
  v.clear(); decodeAll("\x80", v);
  assert(v.size() == 1 && v[0] == GEO_REPLACEMENT);
  v.clear(); decodeAll("\xED\xA0\x80", v);
  assert(v.size() == 1 && v[0] == GEO_REPLACEMENT);
  // layout transliteration
  char out[64]; Georgian::fromLayout("gamarjoba, saqarTvelo!", out, sizeof out);
  assert(std::string(out) == "გამარჯობა, საქართველო!");
  // font encoding (CR ignored, truncated sequence -> '?' then the interrupter)
  char fe[16]; Georgian::toFontEncoding("აb ჰ€", fe, sizeof fe);
  assert((uint8_t)fe[0] == 0x7F && fe[1] == 'b' && fe[2] == ' ' && (uint8_t)fe[3] == 0x9F && fe[4] == '?' && fe[5] == 0);
  Georgian::toFontEncoding("\rA", fe, sizeof fe);
  assert(std::string(fe) == "A");
  Georgian::toFontEncoding("\xE1" "A", fe, sizeof fe);
  assert(fe[0] == '?' && fe[1] == 'A' && fe[2] == 0);
  assert(Georgian::utf8Length("გამარჯობა") == 9);
  char rb[4]; assert(std::string(Georgian::romanize(24, rb)) == "sh");
  // GFX map: CR is ignored, Georgian and ASCII pass through, other BMP -> '?'
  assert(GeorgianGFX::map('\r') == 0);
  assert(GeorgianGFX::map('\n') == '\n');
  assert(GeorgianGFX::map('A') == 'A');
  assert(GeorgianGFX::map(0x10D0) == 0x7F);
  assert(GeorgianGFX::map(0x20AC) == '?');
  Adafruit_GFX gfx(128, 64);
  gfx.setTextSize(3, 4);
  GeorgianGFX geo(gfx);
  geo.printCentered(10, "A", 2);
  assert(gfx.cx == (128 - 12) / 2 && gfx.cy == 10);
  assert(gfx.sizeAtWrite == 2 && gfx.last == 'A' && gfx.n == 1);
  assert(gfx.sizeX() == 3 && gfx.sizeY() == 4);   // previous size restored
  geo.resetDecoder();
  geo.print("\rB");
  assert(gfx.last == 'B' && gfx.n == 2);          // CR not drawn
  // LCD slots
  MockLCD m; GeorgianLCD<MockLCD> g(m, 16, 2); g.begin();
  g.print("გამარჯობა");   // გ ა მ რ ჯ ო ბ = 7 distinct
  assert(g.slotsInUse() == 7);
  assert(m.ddram[0][1] == m.ddram[0][3]);           // both ა share one slot
  g.setCursor(0, 1); g.print("ზთ");                 // 8th fits, 9th falls back to layout key 'T'
  assert(g.slotsInUse() == 8 && m.ddram[1][1] == 'T');
  g.clearRow(0);                                    // free the visible slots
  g.setCursor(0, 0); g.print("ქ");
  assert(m.ddram[0][0] < 8);                        // reused a slot no longer on screen
  // the reused slot must not be the one still visible on row 1 (ზ)
  assert(m.ddram[0][0] != m.ddram[1][0]);
  // glyph contents loaded from table
  assert(memcmp(m.cg[m.ddram[0][0]], GEORGIAN_LCD_GLYPHS[21], 8) == 0);
  // wrap
  g.clear(); g.print("12345678901234567X"); assert(m.ddram[1][0] == '7' && m.ddram[1][1] == 'X');
  // truncated UTF-8 on the LCD path: replacement, then the interrupting byte
  g.clear(); g.print("\xE1" "A");
  assert(m.ddram[0][0] == '?' && m.ddram[0][1] == 'A');
  // setWrap(false) drops characters past the edge
  g.clear(); g.setWrap(false); g.print("12345678901234567X");
  assert(m.ddram[0][15] == '6' && m.ddram[1][0] == ' ');
  g.setWrap(true);
  // printCentered never spills onto the next row, and restores wrapping
  g.clear(); memset(m.ddram[1], 'Z', 16);
  g.printCentered(0, "12345678901234567890");
  assert(m.ddram[0][0] == '1' && m.ddram[0][15] == '6');
  for (int i = 0; i < 16; i++) assert(m.ddram[1][i] == 'Z');
  g.setCursor(0, 1); g.print("AB");
  assert(m.ddram[1][0] == 'A' && m.ddram[1][1] == 'B');
  g.clear(); g.printCentered(0, "Hi");
  assert(m.ddram[0][7] == 'H' && m.ddram[0][8] == 'i' && m.ddram[0][6] == ' ');
  // question-mark fallback and a 9-distinct word (საქართველო: ო is the 9th)
  g.clear(); g.setFallback(GEO_FALLBACK_QUESTION);
  g.print("აბგდევზთი");
  assert(g.slotsInUse() == 8 && m.ddram[0][8] == '?');
  g.clear(); g.setFallback(GEO_FALLBACK_LAYOUT);
  g.print("საქართველო");
  assert(g.slotsInUse() == 8 && m.ddram[0][8] < 8 && m.ddram[0][9] == 'o');
  // Mtavruli uses the same glyph slot as Mkhedruli
  g.clear(); g.print("აᲐ");
  assert(g.slotsInUse() == 1 && m.ddram[0][0] == m.ddram[0][1] && m.ddram[0][0] < 8);
  // geometry is clamped into the cell buffer
  MockLCD mw; GeorgianLCD<MockLCD> wide(mw, 200, 4);
  wide.setCursor(250, 3);
  assert(wide.col() == (uint8_t)(GEORGIAN_LCD_MAX_CELLS - 1) && wide.row() == 0);
  MockLCD mz; GeorgianLCD<MockLCD> zero(mz, 0, 0);
  zero.setCursor(5, 5);
  assert(zero.col() == 0 && zero.row() == 0);
  zero.print("A");
  assert(mz.ddram[0][0] == 'A');
  // serial reader with partial UTF-8 at overflow
  StrStream ss; ss.s = "ააააა\nშ"; char buf[8]; GeorgianSerialReader rd(ss, buf, sizeof buf, 0);
  assert(rd.poll()); assert(std::string(rd.line()) == "აა" && rd.overflowed());  // 7 bytes max -> 2 whole letters
  assert(!rd.poll());   // "შ" waits for newline (timeout disabled)
  // CR is ignored; LF ends the line
  hostSetMillis(0);
  StrStream lines; lines.s = "ab\r\ncd\n"; char lbuf[16];
  GeorgianSerialReader rl(lines, lbuf, sizeof lbuf, 0);
  assert(rl.poll() && std::string(rl.line()) == "ab");
  assert(rl.poll() && std::string(rl.line()) == "cd");
  // idle timeout flushes a line that has no newline
  hostSetMillis(1000);
  StrStream idle; idle.s = "hi"; char ibuf[16];
  GeorgianSerialReader ri(idle, ibuf, sizeof ibuf, 60);
  assert(!ri.poll());
  hostSetMillis(1059);
  assert(!ri.poll());
  hostSetMillis(1060);
  assert(ri.poll() && std::string(ri.line()) == "hi");
  std::cout << "all host tests passed\n";
}
