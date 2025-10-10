#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold12pt7b.h>

#define CS_PIN D1    // Orange
#define DC_PIN D3    // Green
#define RST_PIN D0   // White
#define BUSY_PIN D5  // Purple

// Waveshare 2.13" V4: 250x122, SSD1680
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(GxEPD2_213_B74(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN));
// Alternative: GxEPD2_BW<GxEPD2_213, GxEPD2_213::HEIGHT> display(GxEPD2_213(CS_PIN, DC_PIN, RST_PIN, BUSY_PIN));

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor
  Serial.println("Starting initialization...");
  display.init(115200, true, 20, false); // Baud, reset, duration, pull-up
  Serial.println("Display init called. Send '1' (Hello World), '2' (Come on over), or '3' (Clear screen).");
  display.setRotation(1); // Landscape (250x122)
  display.setFont(&FreeSansBold12pt7b);
  display.setTextColor(GxEPD_BLACK);
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    Serial.print("Received: ");
    Serial.println(input);
    
    display.setPartialWindow(0, 0, 250, 122); // Partial refresh to reduce flicker
    int pageCount = 0;
    display.firstPage();
    do {
      pageCount++;
      display.fillScreen(GxEPD_WHITE); // Clear to white
      if (input == "1") {
        display.setCursor(10, 61); // Center: 122/2 ≈ 61, 10px left margin
        display.print("Hello World");
        Serial.println("Displaying: Hello World");
      } else if (input == "2") {
        display.setCursor(10, 61);
        display.print("Come on over");
        Serial.println("Displaying: Come on over");
      } else if (input == "3") {
        Serial.println("Clearing screen to white");
      } else {
        Serial.println("Invalid input. Send '1', '2', or '3'.");
        return; // Skip display update
      }
    } while (display.nextPage());
    Serial.print("Display update complete. Page cycles: ");
    Serial.println(pageCount);
  }
}