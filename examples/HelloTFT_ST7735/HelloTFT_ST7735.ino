// Georgian text in colour on a 1.8" 128x160 ST7735 SPI TFT.
// Uno/Nano: CS=10 DC=9 RST=8, MOSI=11 SCK=13.  ESP32: CS=5 DC=2 RST=4, MOSI=23 SCK=18.
// Libraries: "Adafruit ST7735 and ST7789 Library" + "Adafruit GFX Library".
// For an ST7789 / ILI9341 just swap the display class: GeorgianGFX takes any Adafruit_GFX.
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <GeorgianGFX.h>
#include <fonts/GeorgianFont9x18.h>

#if defined(ARDUINO_ARCH_ESP32)
#define TFT_CS 5
#define TFT_DC 2
#define TFT_RST 4
#else
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#endif

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
GeorgianGFX geo(tft);

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);                       // landscape 160x128
  tft.fillScreen(ST77XX_BLACK);
  geo.setFont(&GeorgianFont9x18);

  tft.setTextColor(ST77XX_YELLOW);
  geo.printCentered(24, "გამარჯობა");
  tft.setTextColor(ST77XX_CYAN);
  geo.printCentered(50, "საქართველო");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);                       // fonts scale like any GFX font
  geo.printCentered(95, "ქართ", 2);
  tft.setTextSize(1);
}

void loop() {}
