
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <PNGdec.h>
#include <SD.h>

#include "server_link.h"

// ---------------- Server Upload ----------------

unsigned long lastServerPublish = 0;
const unsigned long serverPublishInterval = 5000;

// ---------------- TFT and Touch ----------------

TFT_eSPI tft = TFT_eSPI();

#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

#define SD_CS 5

SPIClass touchscreenSPI(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// ---------------- Sensors ----------------

const int MQ3_PIN = 35;
const int BATTERY_PIN = 34;

// ---------------- Battery ----------------

int batteryPercent = 0;

unsigned long lastBatteryUpdate = 0;
const unsigned long batteryUpdateInterval = 5000;

// ---------------- CPU Usage ----------------

// Measures active execution time of the main application loop.
// This is an application workload estimate, not total dual-core CPU usage.

float cpuUsage = 0.0;

uint64_t loopWorkTime = 0;

unsigned long cpuWindowStart = 0;
const unsigned long cpuUpdateInterval = 1000;

// ---------------- Pet Stats ----------------

float petHunger = 80.0;
float petHappiness = 80.0;
float petHealth = 100.0;

String petState = "Chilling ~";
bool isDead = false;

unsigned long lastUpdate = 0;
const long updateInterval = 2000;

bool deathScreenDrawn = false;

// ---------------- PNG Decoder ----------------

PNG png;

uint16_t lineBuffer[128];

int currentFrame = -1;
String currentImageState = "";

// ---------------- PNG File Callbacks ----------------

File pngFile;

void *pngOpen(const char *filename, int32_t *size) {
  pngFile = SD.open(filename, FILE_READ);

  if (!pngFile) {
    Serial.print("Could not open PNG: ");
    Serial.println(filename);
    return nullptr;
  }

  *size = pngFile.size();
  return &pngFile;
}

void pngClose(void *handle) {
  File *file = (File *)handle;

  if (file) {
    file->close();
  }
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length) {
  File *file = (File *)page->fHandle;

  if (!file) return 0;

  return file->read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position) {
  File *file = (File *)page->fHandle;

  if (!file) return 0;

  return file->seek(position);
}

// ---------------- Battery Reading ----------------

int readBatteryPercent() {
  uint32_t total = 0;

  for (int i = 0; i < 20; i++) {
    total += analogReadMilliVolts(BATTERY_PIN);
    delay(2);
  }

  // CYD battery divider: 1:2
  float voltage = (total / 20.0) * 2.0 / 1000.0;

  const float voltages[] = {
    3.30, 3.50, 3.60, 3.70, 3.75,
    3.80, 3.85, 3.90, 3.95, 4.00,
    4.10, 4.20
  };

  const int percentages[] = {
    0, 5, 10, 20, 30,
    40, 50, 60, 70, 80,
    90, 100
  };

  if (voltage <= 3.30) return 0;
  if (voltage >= 4.20) return 100;

  for (int i = 0; i < 11; i++) {
    if (voltage <= voltages[i + 1]) {
      float fraction =
        (voltage - voltages[i]) /
        (voltages[i + 1] - voltages[i]);

      return percentages[i] +
        fraction * (percentages[i + 1] - percentages[i]);
    }
  }

  return 100;
}

// ---------------- CPU Usage Reading ----------------

void updateCPUUsage(unsigned long loopExecutionTime) {

  // Accumulate active application execution time
  loopWorkTime += loopExecutionTime;

  unsigned long now = millis();

  if (now - cpuWindowStart >= cpuUpdateInterval) {

    unsigned long elapsed = now - cpuWindowStart;

    // Percentage of elapsed time spent executing the main loop
    cpuUsage =
      (loopWorkTime / (elapsed * 1000.0)) * 100.0;

    cpuUsage = constrain(cpuUsage, 0.0, 100.0);

    // Reset measurement window
    loopWorkTime = 0;
    cpuWindowStart = now;
  }
}

// ---------------- Draw PNG Scanlines ----------------

int pngDraw(PNGDRAW *pDraw) {

  int x = 56;
  int y = 65;

  png.getLineAsRGB565(
    pDraw,
    lineBuffer,
    PNG_RGB565_BIG_ENDIAN,
    0xFFFFFF
  );

  tft.pushImage(
    x,
    y + pDraw->y,
    pDraw->iWidth,
    1,
    lineBuffer
  );

  return 1;
}

// ---------------- Draw Pet Image ----------------

void drawPetFace(String state, int frame) {

  String filename;

  if (isDead || state == "DEAD X_X") {
    filename = "/ghost_0" + String(frame + 1) + ".png";
  }
  else if (state == "Happy :)") {
    filename = "/happy_0" + String(frame + 1) + ".png";
  }
  else if (state == "Tipsy B)") {
    filename = "/tipsy_0" + String(frame + 1) + ".png";
  }
  else if (state == "Sad :(") {
    filename = "/sad_0" + String(frame + 1) + ".png";
  }
  else {
    filename = "/chill_0" + String(frame + 1) + ".png";
  }

  int16_t result = png.open(
    filename.c_str(),
    pngOpen,
    pngClose,
    pngRead,
    pngSeek,
    pngDraw
  );

  if (result == PNG_SUCCESS) {
    png.decode(nullptr, 0);
    png.close();
  }
}

// ---------------- Static UI ----------------

void drawStaticUI() {

  tft.drawRect(5, 5, 230, 310, TFT_BLACK);

  tft.drawFastHLine(5, 42, 230, TFT_DARKGREY);
  tft.drawFastHLine(5, 205, 230, TFT_DARKGREY);
  tft.drawFastHLine(5, 282, 230, TFT_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  if (!isDead) {

    tft.setCursor(15, 218);
    tft.print("HUNGER:");

    tft.setCursor(15, 240);
    tft.print("HAPPY:");

    tft.setCursor(15, 262);
    tft.print("HEALTH:");
  }
}

// ---------------- Status Bars ----------------

void drawStatusBar(
  int x,
  int y,
  int w,
  int h,
  float val,
  uint16_t color
) {

  tft.drawRect(x, y, w, h, TFT_WHITE);

  int fillW = (val / 100.0) * (w - 2);

  tft.fillRect(
    x + 1,
    y + 1,
    fillW,
    h - 2,
    color
  );

  tft.fillRect(
    x + 1 + fillW,
    y + 1,
    (w - 2) - fillW,
    h - 2,
    TFT_BLACK
  );
}

// ---------------- Dynamic UI ----------------

void updateUI(int rawValue) {

  // Header
  tft.fillRect(7, 7, 226, 34, TFT_WHITE);

  // Pet state
  tft.setTextSize(2);

  if (isDead) {
    tft.setTextColor(TFT_RED, TFT_WHITE);
  }
  else if (rawValue > 850) {
    tft.setTextColor(TFT_GREEN, TFT_WHITE);
  }
  else if (rawValue > 650) {
    tft.setTextColor(TFT_YELLOW, TFT_WHITE);
  }
  else {
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
  }

  tft.setCursor(14, 18);
  tft.print(petState);

  // Battery percentage
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  tft.setCursor(187, 12);
  tft.print("BAT:");
  tft.print(batteryPercent);
  tft.print("%");

  // CPU workload
  tft.setCursor(187, 28);
  tft.print("CPU:");
  tft.print(cpuUsage, 0);
  tft.print("%");

  // Animation
  int frame = (millis() / 500) % 6;

  if (frame != currentFrame ||
      petState != currentImageState) {

    currentFrame = frame;
    currentImageState = petState;

    drawPetFace(petState, frame);
  }

  // Handle death screen
  if (isDead && !deathScreenDrawn) {

    tft.fillRect(7, 207, 226, 75, TFT_WHITE);

    deathScreenDrawn = true;
  }

  // Reset button ONLY when dead
  if (isDead) {
    drawResetButton();
  }

  // Status bars only when alive
  if (!isDead) {

    drawStatusBar(65, 218, 155, 10, petHunger, TFT_GREEN);
    drawStatusBar(65, 240, 155, 10, petHappiness, TFT_YELLOW);
    drawStatusBar(65, 262, 155, 10, petHealth, TFT_RED);
  }

  // Footer
  tft.fillRect(7, 284, 226, 29, TFT_WHITE);

  tft.setTextSize(1);
  tft.setTextColor(TFT_ORANGE, TFT_WHITE);

  tft.setCursor(15, 288);

  if (isDead) {

    tft.print("PET DIED!");

    tft.setCursor(15, 302);
    tft.print("Touch RESET to play again.");
  }
  else {

    tft.print("Raw: ");
    tft.print(rawValue);

    tft.setCursor(15, 302);
    tft.print("Chill<=650 Tipsy>650 Happy>850");
  }
}

// ---------------- Draw Reset Button ----------------

void drawResetButton() {

  // Button background
  tft.fillRoundRect(45, 190 + 31, 150, 45, 8, TFT_RED);

  // Button outline
  tft.drawRoundRect(45, 190 + 31, 150, 45, 8, TFT_BLACK);

  // Button text
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_RED);

  tft.setCursor(87, 205 + 31);
  tft.print("RESET");
}

// ---------------- Reset Pet ----------------

void resetPet() {

  petHunger = 80.0;
  petHappiness = 80.0;
  petHealth = 100.0;

  petState = "Chilling ~";

  deathScreenDrawn = false;
  isDead = false;

  currentFrame = -1;
  currentImageState = "";

  tft.fillScreen(TFT_WHITE);

  drawStaticUI();
}

// ---------------- Touchscreen Serial Debug ----------------

// Uncomment if needed.

/*
void printTouchData() {

  if (ts.touched()) {

    TS_Point p = ts.getPoint();

    int pixelX = map(p.x, 200, 3800, 0, 240);
    int pixelY = map(p.y, 200, 3800, 0, 320);

    Serial.print("Touch | Raw X: ");
    Serial.print(p.x);

    Serial.print(" | Raw Y: ");
    Serial.print(p.y);

    Serial.print(" | Pixel X: ");
    Serial.print(pixelX);

    Serial.print(" | Pixel Y: ");
    Serial.println(pixelY);
  }
}
*/

// ---------------- Setup ----------------

void setup() {

  Serial.begin(115200);
  delay(1000);

  // Touchscreen
  touchscreenSPI.begin(
    XPT2046_CLK,
    XPT2046_MISO,
    XPT2046_MOSI,
    XPT2046_CS
  );

  ts.begin(touchscreenSPI);
  ts.setRotation(0);

  // TFT
  tft.init();
  tft.invertDisplay(false);

  // Battery ADC
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  batteryPercent = readBatteryPercent();

  // SD Card
  Serial.println("Initializing SD card");

  if (!SD.begin(SD_CS)) {

    Serial.println("SD card initialization failed!");
  }
  else {

    Serial.println("SD card initialized.");
    Serial.println("Files on SD card:");

    File root = SD.open("/");
    File file = root.openNextFile();

    while (file) {

      Serial.println(file.name());

      file.close();
      file = root.openNextFile();
    }

    root.close();
  }

  Serial.println("SD card section finished");

  // WiFi + server uploads (connects in the background)
  serverLinkBegin();

  // Initialize CPU measurement
  cpuWindowStart = millis();
  loopWorkTime = 0;

  // Boot screen
  tft.fillScreen(TFT_WHITE);

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(48, 155);

  tft.print("Tamagotchi Booting...");

  delay(2000);

  tft.fillScreen(TFT_WHITE);

  drawStaticUI();

  Serial.println("Setup finished");
}

// ---------------- Main Loop ----------------

void loop() {

  // Start measuring active application execution time
  unsigned long loopStart = micros();

  // Read MQ-3
  int rawValue = analogRead(MQ3_PIN);

  float voltage = rawValue * (3.3 / 4095.0);

  // Uncomment for touchscreen debugging
  // printTouchData();

  // Update battery every 5 seconds
  if (millis() - lastBatteryUpdate >= batteryUpdateInterval) {

    lastBatteryUpdate = millis();

    batteryPercent = readBatteryPercent();
  }

  // Touch input for reset
  if (isDead && ts.touched()) {

    TS_Point p = ts.getPoint();

    int pixelX = map(p.x, 200, 3800, 0, 240);
    int pixelY = map(p.y, 200, 3800, 0, 320);

    if (
      pixelX >= 45 && pixelX <= 200 &&
      pixelY >= 225 && pixelY <= 277
    ) {

      resetPet();

      delay(300);
    }
  }

  // Update pet stats
  unsigned long currentMillis = millis();

  if (!isDead &&
      currentMillis - lastUpdate >= updateInterval) {

    lastUpdate = currentMillis;

    if (rawValue > 850) {

      petHealth += 10.0;
      petHunger += 20.0;
      petHappiness += 20.0;

      petState = "Happy :)";
    }
    else if (rawValue > 650) {

      petHealth += 5.0;
      petHunger += 15.0;
      petHappiness += 10.0;

      petState = "Tipsy B)";
    }
    else {

      petHealth -= 10.0;
      petHunger -= 2.5;
      petHappiness -= 2.0;

      if (petHealth < 50.0) {
        petState = "Sad :(";
      }
      else {
        petState = "Chilling ~";
      }
    }

    // Clamp stats
    petHunger = constrain(petHunger, 0.0, 100.0);
    petHappiness = constrain(petHappiness, 0.0, 100.0);
    petHealth = constrain(petHealth, 0.0, 100.0);

    // Death condition
    if (petHealth <= 0.0) {

      isDead = true;
      petState = "DEAD X_X";

      currentFrame = -1;
    }
  }

  // Real-time state responsiveness
  if (!isDead) {

    if (rawValue > 850) {
      petState = "Happy :)";
    }
    else if (rawValue > 650) {
      petState = "Tipsy B)";
    }
    else if (petHealth < 50.0) {
      petState = "Sad :(";
    }
    else {
      petState = "Chilling ~";
    }
  }

  // Send a snapshot to the server every 5 seconds
  if (millis() - lastServerPublish >= serverPublishInterval) {

    lastServerPublish = millis();

    TelemetrySnapshot snap;

    snap.raw = rawValue;
    snap.voltage = voltage;
    snap.hunger = petHunger;
    snap.happiness = petHappiness;
    snap.health = petHealth;
    snap.battery = batteryPercent;
    snap.cpu = cpuUsage;
    snap.dead = isDead;
    snap.uptimeMs = millis();

    strlcpy(snap.state, petState.c_str(), sizeof(snap.state));

    serverLinkPublish(snap);
  }

  // Render UI
  updateUI(rawValue);

  // Finish CPU workload measurement
  unsigned long loopExecutionTime = micros() - loopStart;

  updateCPUUsage(loopExecutionTime);

  delay(100);
}
