#include <SPI.h> 
#include <TFT_eSPI.h> 
#include <XPT2046_Touchscreen.h> 
#include "pet_images.h"

// Initialize TFT screen (Portrait mode 240x320) 
TFT_eSPI tft = TFT_eSPI();  

// Touch Screen Pin Definitions for Cheap Yellow Display (CYD) 
#define XPT2046_IRQ 36 
#define XPT2046_MOSI 32 
#define XPT2046_MISO 39 
#define XPT2046_CLK 25 
#define XPT2046_CS 33 

SPIClass touchscreenSPI(HSPI); 
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ); 

// Pin Definition 
const int MQ3_PIN = 35; // Analog input for MQ-3 on ESP32 (GPIO 35) 

// Tamagotchi Stats (0 to 100) 
float petHunger = 80.0; 
float petHappiness = 80.0; 
float petHealth = 100.0;  
String petState = "Chilling ~"; 
bool isDead = false; 

unsigned long lastUpdate = 0; 
const long updateInterval = 2000; // Update stats every 2 seconds 

void setup() { 
  Serial.begin(115200); 
  delay(1000); 

  // Initialize Touch Controller on HSPI bus 
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS); 
  ts.begin(touchscreenSPI); 
  ts.setRotation(0); // Match portrait orientation 

  // Initialize the TFT screen 
  tft.init(); 
  tft.setRotation(0); // Portrait mode (240x320 pixels) 
  tft.fillScreen(TFT_BLACK); 

  // Boot Screen 
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(1); 
  tft.setCursor(48, 155); 
  tft.print("Tamagotchi Booting..."); 
  delay(2000); 
  tft.fillScreen(TFT_BLACK); 
   
  // Draw static UI layout frames once 
  drawStaticUI(); 
} 

void loop() { 
  // 1. Read MQ-3 Sensor from GPIO 35 
  int rawValue = analogRead(MQ3_PIN); 
  float voltage = rawValue * (3.3 / 4095.0); 

  // 2. Handle Touch Input for Reset Button if Dead (with Serial debug) 
  if (isDead && ts.touched()) { 
    TS_Point p = ts.getPoint(); 
     
    // Print raw touch points to Serial Monitor
    Serial.print("Touch Raw -> X: "); Serial.print(p.x);
    Serial.print(" | Y: "); Serial.println(p.y);

    // Map raw touch coordinates to 240x320 portrait screen coordinates
    int pixelX = map(p.x, 200, 3800, 0, 240);
    int pixelY = map(p.y, 200, 3800, 0, 320); 

    Serial.print("Mapped -> X: "); Serial.print(pixelX);
    Serial.print(" | Y: "); Serial.println(pixelY);

    // Check if touch is inside the portrait RESET button bounds (X: 45-195, Y: 150-195)
    if (pixelX >= 45 && pixelX <= 195 && pixelY >= 150 && pixelY <= 195) { 
      Serial.println("RESET TRIGGERED!");
      
      // Reset all game stats 
      petHunger = 80.0; 
      petHappiness = 80.0; 
      petHealth = 100.0; 
      petState = "Chilling ~"; 
      isDead = false; 
       
      tft.fillScreen(TFT_BLACK); 
      drawStaticUI(); 
      delay(300); // Debounce delay 
    } 
  } 

  // 3. Update Tamagotchi stats & state periodically (only if alive) 
  unsigned long currentMillis = millis(); 
  if (!isDead && (currentMillis - lastUpdate >= updateInterval)) { 
    lastUpdate = currentMillis; 

    // Check sensor / blowing conditions
    if (rawValue > 850) { 
      // Super blow: Very Happy!
      petHealth += 10.0; 
      petHunger += 20.0; 
      petHappiness += 20.0; 
      petState = "Happy :)"; 
    } 
    else if (rawValue > 650) { 
      // Moderate blow: Tipsy!
      petHealth += 5.0; 
      petHunger += 15.0; 
      petHappiness += 10.0; 
      petState = "Tipsy B)"; 
    } 
    else { 
      // Not blowing: Natural stat decay over time (Chilling)
      petHealth -= 3.0;     
      petHunger -= 2.5; 
      petHappiness -= 2.0; 
       
      if (petHealth < 50.0) {
        petState = "Sad :(";
      } else {
        petState = "Chilling ~";
      }
    } 

    // Clamp stats between 0 and 100 
    petHunger = constrain(petHunger, 0.0, 100.0); 
    petHappiness = constrain(petHappiness, 0.0, 100.0); 
    petHealth = constrain(petHealth, 0.0, 100.0); 

    // Check for Death condition 
    if (petHealth <= 0.0) { 
      isDead = true; 
      petState = "DEAD X_X"; 
    } 
  } else if (!isDead) {
    // Real-time state responsiveness between decay intervals
    if (rawValue > 850) {
      petState = "Happy :)";
    } else if (rawValue > 650) {
      petState = "Tipsy B)";
    } else if (petHealth < 50.0) {
      petState = "Sad :(";
    } else {
      petState = "Chilling ~";
    }
  }

  // 4. Render Dynamic UI across the full 240x320 portrait screen 
  updateUI(voltage, rawValue); 
   
  delay(100); 
} 

void drawStaticUI() {
  // Portrait layout for a 240x320 display.
  tft.drawRect(5, 5, 230, 310, TFT_DARKGREY);
  tft.drawFastHLine(5, 42, 230, TFT_DARKGREY);
  tft.drawFastHLine(5, 165, 230, TFT_DARKGREY);
  tft.drawFastHLine(5, 282, 230, TFT_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(15, 202);
  tft.print("HUNGER:");
  tft.setCursor(15, 230);
  tft.print("HAPPY:");
  tft.setCursor(15, 258);
  tft.print("HEALTH:");
}

void updateUI(float sensorVoltage, int rawValue) {
  // Clear and redraw the state header to prevent stale text.
  tft.fillRect(7, 7, 226, 34, TFT_BLACK);
  tft.setTextSize(2);
  if (isDead) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  } else if (rawValue > 850) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else if (rawValue > 650) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
  }
  tft.setCursor(14, 15);
  tft.print("State: ");
  tft.print(petState);

  // Pet image centered in the main portrait area.
  drawPetFace(petState, 120, 105);

  // Stack the three status bars vertically for portrait orientation.
  drawStatusBar(65, 202, 155, 10, petHunger, TFT_GREEN);
  drawStatusBar(65, 230, 155, 10, petHappiness, TFT_YELLOW);
  drawStatusBar(65, 258, 155, 10, petHealth, TFT_RED);

  // Sensor readout in the footer.
  tft.fillRect(7, 284, 226, 29, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(15, 288);
  if (isDead) {
    tft.print("PET DIED!");
    tft.setCursor(15, 302);
    tft.print("Touch RESET to play again.");
  } else {
    tft.print("Raw: ");
    tft.print(rawValue);
    tft.setCursor(15, 302);
    tft.print("Chill<=650 Tipsy>650 Happy>850");
  }
}

void drawStatusBar(int x, int y, int w, int h, float val, uint16_t color) { 
  tft.drawRect(x, y, w, h, TFT_WHITE); 
  int fillW = (val / 100.0) * (w - 2); 
  tft.fillRect(x + 1, y + 1, fillW, h - 2, color); 
  tft.fillRect(x + 1 + fillW, y + 1, (w - 2) - fillW, h - 2, TFT_BLACK); 
} 

// Helper function to draw animated pet images from pet_images.h
void drawPetFace(String state, int x, int y) {
  // Each animation contains four 64x64 frames stored in PROGMEM.
  const uint16_t* frame = nullptr;
  int frameIndex = (millis() / 250) % 4;

  if (isDead || state == "DEAD X_X") {
    switch (frameIndex) {
      case 0: frame = dead_01; break;
      case 1: frame = dead_02; break;
      case 2: frame = dead_03; break;
      default: frame = dead_04; break;
    }

    tft.fillRect(x - 48, y - 48, 96, 100, TFT_BLACK);
    tft.pushImage(x - 32, y - 32, 64, 64, frame);

    // Keep the existing reset button.
    tft.fillRoundRect(45, 150, 150, 45, 8, TFT_BLUE);
    tft.drawRoundRect(45, 150, 150, 45, 8, TFT_WHITE);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setCursor(90, 164);
    tft.print("RESET");
    return;
  }

  // Draw a square pet container suited to portrait orientation.
  //tft.drawRoundRect(x - 45, y - 45, 90, 90, 12, TFT_WHITE);
  //tft.fillRect(x - 40, y - 40, 80, 80, TFT_BLACK);

  if (state == "Happy :)") {
    switch (frameIndex) {
      case 0: frame = happy_01; break;
      case 1: frame = happy_02; break;
      case 2: frame = happy_03; break;
      default: frame = happy_04; break;
    }
  } else if (state == "Tipsy B)") {
    switch (frameIndex) {
      case 0: frame = tipsy_01; break;
      case 1: frame = tipsy_02; break;
      case 2: frame = tipsy_03; break;
      default: frame = tipsy_04; break;
    }
  } else if (state == "Sad :(") {
    switch (frameIndex) {
      case 0: frame = sad_01; break;
      case 1: frame = sad_02; break;
      case 2: frame = sad_03; break;
      default: frame = sad_04; break;
    }
  } else {
    // Chilling state uses the meh animation frames.
    switch (frameIndex) {
      case 0: frame = meh_01; break;
      case 1: frame = meh_02; break;
      case 2: frame = meh_03; break;
      default: frame = meh_04; break;
    }
  }

  tft.pushImage(x - 32, y - 32, 64, 64, frame);
}
