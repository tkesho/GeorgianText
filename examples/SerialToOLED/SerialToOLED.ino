// Serial Monitor (115200 baud) -> 128x64 SSD1306 OLED, with word wrap.
// Type Georgian UTF-8 directly, or prefix a line with '~' to use Latin keyboard-layout input:
// "~gamarjoba megobaro" -> "გამარჯობა მეგობარო".
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <GeorgianGFX.h>
#include <fonts/GeorgianFont6x13.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
GeorgianGFX geo(display);

char inBuf[48];
char geoBuf[100];
GeorgianSerialReader reader(Serial, inBuf, sizeof(inBuf));

void show(const char *text) {
  display.clearDisplay();
  display.setCursor(0, 11);          // baseline of the first line for the 6x13 font
  geo.print(text);                   // Adafruit_GFX wraps at the screen edge
  display.display();
}

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for (;;) delay(1000); }
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(true);
  geo.setFont(&GeorgianFont6x13);
  show("ჩაწერე ტექსტი Serial Monitor-ში");
  Serial.println(F("Ready. Type a line (prefix ~ for layout transliteration)."));
}

void loop() {
  if (!reader.poll()) return;
  const char *text = reader.line();
  if (text[0] == '~') { Georgian::fromLayout(text + 1, geoBuf, sizeof(geoBuf)); text = geoBuf; }
  show(text);
  Serial.print(F("Shown: ")); Serial.println(text);
}
