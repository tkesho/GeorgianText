#include "Arduino.h"
#include "../../src/GeorgianCore.h"
#include "../../src/GeorgianLCD.h"
#include <cassert>
#include <vector>
#include <iostream>

struct MockLCD {
  int cols = 16, rows = 2, c = 0, r = 0; uint8_t cg[8][8]; uint8_t ddram[2][16];
  int creates = 0;
  MockLCD() { memset(ddram, ' ', sizeof ddram); }
  void clear() { memset(ddram, ' ', sizeof ddram); c = r = 0; }
  void setCursor(uint8_t cc, uint8_t rr) { c = cc; r = rr; }
  void createChar(uint8_t s, uint8_t *g) { memcpy(cg[s & 7], g, 8); creates++; c = -100; } // address lost
  size_t write(uint8_t ch) { assert(c >= 0 && c < 16); ddram[r][c++] = ch; return 1; }
};
struct StrStream : Stream {
  std::string s; size_t i = 0;
  int available() override { return (int)(s.size() - i); }
  int read() override { return i < s.size() ? (uint8_t)s[i++] : -1; }
  size_t write(uint8_t) override { return 1; }
};
int main() {
  // UTF-8 decode
  GeoUTF8Decoder d; uint32_t cp; std::vector<uint32_t> v;
  for (const char *p = "აჰA\xE1\xB2\x90"; *p; p++) if (d.feed((uint8_t)*p, cp)) v.push_back(cp);
  assert(v.size() == 4 && v[0] == 0x10D0 && v[1] == 0x10F0 && v[2] == 'A' && v[3] == 0x1C90);
  assert(Georgian::letterIndex(0x1C90) == 0 && Georgian::letterIndex(0x10F1) == -1);
  // layout transliteration
  char out[64]; Georgian::fromLayout("gamarjoba, saqarTvelo!", out, sizeof out);
  assert(std::string(out) == "გამარჯობა, საქართველო!");
  // font encoding
  char fe[16]; Georgian::toFontEncoding("აb ჰ€", fe, sizeof fe);
  assert((uint8_t)fe[0] == 0x7F && fe[1] == 'b' && fe[2] == ' ' && (uint8_t)fe[3] == 0x9F && fe[4] == '?' && fe[5] == 0);
  assert(Georgian::utf8Length("გამარჯობა") == 9);
  char rb[4]; assert(std::string(Georgian::romanize(24, rb)) == "sh");
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
  // serial reader with partial UTF-8 at overflow
  StrStream ss; ss.s = "ააააა\nშ"; char buf[8]; GeorgianSerialReader rd(ss, buf, sizeof buf, 0);
  assert(rd.poll()); assert(std::string(rd.line()) == "აა" && rd.overflowed());  // 7 bytes max -> 2 whole letters
  assert(!rd.poll());   // "შ" waits for newline (timeout disabled)
  std::cout << "all host tests passed\n";
}
