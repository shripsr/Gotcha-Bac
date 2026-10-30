// =================================================================
// User_Setup.h for ESP32 Cheap Yellow Display (ESP32-2432S028R)
// =================================================================

#define USER_SETUP_ID 24

// 1. Use the alternative ILI9341 driver required for CYD
#define ILI9341_2_DRIVER

// 2. Pin Assignments for the Cheap Yellow Display
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST  -1  // Reset pin (-1 if connected to EN/RST)
#define TFT_BL   21  // LED back-light control pin
#define TFT_BACKLIGHT_ON HIGH

// Touch Controller Chip Select (XPT2046)
#define TOUCH_CS 33

// 3. Fonts to Load
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel high font
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font
#define LOAD_FONT6  // Font 6. Large 48 pixel font
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font
#define LOAD_FONT8  // Font 8. Large 75 pixel font
#define LOAD_GFXFF  // FreeFonts. Include access to scalable fonts
#define SMOOTH_FONT

// 4. SPI Frequencies
#define SPI_FREQUENCY       55000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000