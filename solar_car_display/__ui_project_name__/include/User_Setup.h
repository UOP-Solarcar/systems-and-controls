// Custom User_Setup.h for ILI9486 in parallel mode on Mega2560

#define ILI9486_DRIVER

// Define the pins used for the parallel interface
#define TFT_CS   53  // Chip select control pin
#define TFT_DC   48  // Data Command control pin
#define TFT_RST  49  // Reset pin (could connect to Arduino RESET pin)
#define TFT_WR   47  // Write strobe control pin
#define TFT_RD   46  // Read strobe control pin

#define TFT_D0   22  // Data bus pin 0
#define TFT_D1   23  // Data bus pin 1
#define TFT_D2   24  // Data bus pin 2
#define TFT_D3   25  // Data bus pin 3
#define TFT_D4   26  // Data bus pin 4
#define TFT_D5   27  // Data bus pin 5
#define TFT_D6   28  // Data bus pin 6
#define TFT_D7   29  // Data bus pin 7

// Load fonts
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters

// Define the SPI frequency (not used in parallel mode but required by library)
#define SPI_FREQUENCY  40000000
