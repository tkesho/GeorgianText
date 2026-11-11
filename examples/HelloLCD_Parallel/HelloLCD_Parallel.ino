// Georgian text on a 16x2 HD44780 LCD wired in 4-bit parallel mode (Arduino LiquidCrystal).
// Uno/Nano: RS=12 E=11 D4=5 D5=4 D6=3 D7=2.  ESP32: RS=19 E=23 D4=18 D5=17 D6=16 D7=15
// (5V LCDs on ESP32: the LCD only reads these pins, RW tied to GND, so 3.3V logic is fine).
#include <LiquidCrystal.h>
#define GEORGIAN_LCD_MAX_CELLS 32   // 16x2: 32 cells instead of the 20x4 default (saves 48 B RAM)
#include <GeorgianLCD.h>

#if defined(ARDUINO_ARCH_ESP32)
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);
#else
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
#endif
GeorgianLCD<LiquidCrystal> glcd(lcd, 16, 2);

const char *const words[] = { "მზე", "მთვარე", "ვარსკვლავი", "ცა", "ზღვა" };   // sun moon star sky sea

void setup() {
  lcd.begin(16, 2);
  glcd.begin();
  glcd.print("Georgian:");      // ASCII banner: uses no CGRAM slots
}

void loop() {
  for (uint8_t i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
    glcd.clearRow(1);     // slots used only by the old word become free again
    glcd.print(words[i]);
    delay(1500);
  }
}
