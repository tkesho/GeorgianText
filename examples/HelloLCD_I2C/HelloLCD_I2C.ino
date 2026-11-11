// Georgian text on a 16x2 HD44780 LCD with a PCF8574 I2C backpack.
// Wiring: Uno/Nano SDA=A4 SCL=A5 | ESP32 SDA=21 SCL=22. VCC 5V, GND.
// Library: "LiquidCrystal I2C" (Frank de Brabander / Marco Schwartz). Address usually 0x27 or 0x3F.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define GEORGIAN_LCD_MAX_CELLS 32   // 16x2: 32 cells instead of the 20x4 default (saves 48 B RAM)
#include <GeorgianLCD.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
GeorgianLCD<LiquidCrystal_I2C> glcd(lcd, 16, 2);

void setup() {
  lcd.init();
  lcd.backlight();
  glcd.begin();

  glcd.printCentered(0, "გამარჯობა");   // 7 distinct letters (აბგმორჯ): fits in the 8 CGRAM slots
  glcd.printCentered(1, "Hello 2026");
}

void loop() {
  delay(3000);
  glcd.clear();                              // frees all 8 slots
  glcd.print("თბილისი");                     // Tbilisi: 5 distinct letters (თბილს)
  glcd.setCursor(0, 1);
  glcd.print("slots: ");
  glcd.print(glcd.slotsInUse());
  delay(3000);
  glcd.clear();
  glcd.printCentered(0, "გამარჯობა");
  glcd.printCentered(1, "Hello 2026");
}
