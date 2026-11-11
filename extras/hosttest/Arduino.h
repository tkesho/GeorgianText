// Minimal Arduino stand-in for host-side unit tests.
#pragma once
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <string>
inline unsigned long &hostMillisClock() { static unsigned long t = 0; return t; }
inline unsigned long millis() { return hostMillisClock(); }
inline void hostSetMillis(unsigned long t) { hostMillisClock() = t; }
class Print {
public:
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *b, size_t n) { size_t k = 0; while (n--) k += write(*b++); return k; }
  size_t print(const char *s) { return write((const uint8_t *)s, strlen(s)); }
  size_t print(int v) { char b[16]; snprintf(b, 16, "%d", v); return print(b); }
  virtual ~Print() {}
};
class Stream : public Print {
public:
  virtual int available() = 0; virtual int read() = 0;
};
