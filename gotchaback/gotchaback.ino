#include <SPI.h> 
#include <TFT_eSPI.h> 
#include <XPT2046_Touchscreen.h> 

// Initialize TFT screen (Landscape mode 320x240) 
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
  ts.setRotation(1); // Match landscape orientation 

  // Initialize the TFT screen 
  tft.init(); 
  tft.setRotation(1); // Landscape mode (320x240 pixels) 
  tft.fillScreen(TFT_BLACK); 

  // Boot Screen 
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(2); 
  tft.setCursor(45, 110); 
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

    // Map raw touch coordinates to 320x240 landscape screen coordinates 
    int pixelX = map(p.y, 200, 3800, 0, 320); 
    int pixelY = map(p.x, 3800, 200, 0, 240); 

    Serial.print("Mapped -> X: "); Serial.print(pixelX);
    Serial.print(" | Y: "); Serial.println(pixelY);

    // Check if touch is inside the RESET button bounds (X: 180-240, Y: 60-180) 
    if (pixelX >= 180 && pixelX <= 240 && pixelY >= 60 && pixelY <= 180) { 
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

  // 4. Render Dynamic UI across the full 320x240 screen 
  updateUI(voltage, rawValue); 
   
  delay(100); 
} 

void drawStaticUI() { 
  // Draw full screen outer border box 
  tft.drawRect(5, 5, 310, 230, TFT_DARKGREY); 
   
  // Header and Footer Divider Lines 
  tft.drawFastHLine(5, 40, 310, TFT_DARKGREY); 
  tft.drawFastHLine(5, 190, 310, TFT_DARKGREY); 
   
  // Static labels for the bottom status bars 
  tft.setTextSize(1); 
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
   
  tft.setCursor(15, 200); 
  tft.print("HGR:"); 
   
  tft.setCursor(115, 200); 
  tft.print("HPS:"); 
   
  tft.setCursor(215, 200); 
  tft.print("HLT:"); 
} 

void updateUI(float sensorVoltage, int rawValue) { 
  // Update State Header across the top 
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
  tft.setCursor(15, 12); 
  tft.print("State: "); 
  tft.print(petState); 
  tft.print("   "); // Clear trailing text artifacts 

  // Draw Pet Face or Reset Button if dead 
  drawPetFace(petState, 160, 115); 

  // Draw three widescreen Status Bars at the bottom 
  drawStatusBar(45, 198, 60, 10, petHunger, TFT_GREEN); 
  drawStatusBar(145, 198, 60, 10, petHappiness, TFT_YELLOW); 
  drawStatusBar(245, 198, 60, 10, petHealth, TFT_RED); 

  // Draw Sensor Readout at the very bottom footer 
  tft.setTextSize(1); 
  tft.setCursor(15, 218); 
  tft.setTextColor(TFT_ORANGE, TFT_BLACK); 
  if (isDead) { 
    tft.print("PET DIED! Touch RESET to play again.  "); 
  } else { 
    tft.print("Raw: "); 
    tft.print(rawValue); 
    tft.print(" (Chill <=650 | Tipsy >650 | Happy >850)  "); 
  } 
} 

void drawStatusBar(int x, int y, int w, int h, float val, uint16_t color) { 
  tft.drawRect(x, y, w, h, TFT_WHITE); 
  int fillW = (val / 100.0) * (w - 2); 
  tft.fillRect(x + 1, y + 1, fillW, h - 2, color); 
  tft.fillRect(x + 1 + fillW, y + 1, (w - 2) - fillW, h - 2, TFT_BLACK); 
} 

// Helper function to draw pixel art faces or reset button 
void drawPetFace(String state, int x, int y) { 
  if (isDead) { 
    // Clear face area and draw Reset Button 
    tft.fillRect(x - 50, y - 35, 100, 60, TFT_BLACK); 
     
    // Draw Touch Button 
    tft.fillRoundRect(85, 130, 150, 45, 8, TFT_BLUE); 
    tft.drawRoundRect(85, 130, 150, 45, 8, TFT_WHITE); 
    tft.setTextSize(2); 
    tft.setTextColor(TFT_WHITE, TFT_BLUE); 
    tft.setCursor(130, 144); 
    tft.print("RESET"); 
    return; 
  } 

  // Outer widescreen body container box 
  tft.drawRoundRect(x - 55, y - 40, 110, 75, 12, TFT_WHITE); 
  tft.fillRect(x - 45, y - 28, 90, 55, TFT_BLACK);
   
  if (state == "Happy :)") { 
    // Eyes 
    tft.fillRect(x - 28, y - 16, 12, 12, TFT_WHITE); 
    tft.fillRect(x + 16, y - 16, 12, 12, TFT_WHITE); 
    // Smile 
    tft.drawFastHLine(x - 20, y + 14, 40, TFT_WHITE); 
    tft.drawPixel(x - 21, y + 13, TFT_WHITE); 
    tft.drawPixel(x + 21, y + 13, TFT_WHITE); 
  }  
  else if (state == "Tipsy B)") { 
    // Happy/Drunk eyes ^_^ 
    tft.setTextSize(2); 
    tft.setTextColor(TFT_GREEN, TFT_BLACK); 
    tft.setCursor(x - 28, y - 18); 
    tft.print("^  ^"); 
    // Grin 
    tft.setCursor(x - 14, y + 4); 
    tft.print("D"); 
  } 
  else if (state == "Sad :(") {
    // Sad eyes & Frown
    tft.fillRect(x - 28, y - 16, 12, 12, TFT_WHITE); 
    tft.fillRect(x + 16, y - 16, 12, 12, TFT_WHITE); 
    tft.drawFastHLine(x - 20, y + 18, 40, TFT_WHITE); 
    tft.drawPixel(x - 21, y + 17, TFT_WHITE); 
    tft.drawPixel(x + 21, y + 17, TFT_WHITE);
  }
  else { 
    // Chilling ~ (Relaxed eyes and neutral mouth)
    tft.drawFastHLine(x - 28, y - 10, 12, TFT_WHITE); 
    tft.drawFastHLine(x + 16, y - 10, 12, TFT_WHITE); 
    tft.drawFastHLine(x - 12, y + 12, 24, TFT_WHITE); 
  } 
}