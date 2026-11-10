// Georgian text on a 128x64 SSD1306 I2C OLED (address 0x3C).
// Wiring: Uno/Nano SDA=A4 SCL=A5 | ESP32 SDA=21 SCL=22.
// Libraries: "Adafruit SSD1306" + "Adafruit GFX Library".
// Note (Uno/Nano): the SSD1306 driver allocates a 1024-byte frame buffer at runtime,
// half of the Uno's 2 KB RAM. Keep other buffers small.
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <GeorgianGFX.h>
#include <fonts/GeorgianFont6x13.h>
#include <fonts/GeorgianFont9x18.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
GeorgianGFX geo(display);

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 not found"));
    for (;;) delay(1000);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  geo.setFont(&GeorgianFont9x18);
  geo.printCentered(16, "გამარჯობა!");          // y is the text baseline

  geo.setFont(&GeorgianFont6x13);
  display.setCursor(0, 36);
  geo.print("ყველა 33 ასო:");                     // all 33 letters
  display.setCursor(0, 49);
  geo.print("აბგდევზთიკლმნოპჟრ");
  display.setCursor(0, 62);
  geo.print("სტუფქღყშჩცძწჭხჯჰ");
  display.display();
}

void loop() {}
