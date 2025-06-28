#ifndef TFT_CONFIG_H
#define TFT_CONFIG_H

#define USER_SETUP_LOADED      1
#define ILI9341_DRIVER        1
#define TFT_DRIVER            0x9341

// TFT Pins
#define TFT_MISO              50
#define TFT_MOSI              51
#define TFT_SCLK              52
#define TFT_CS                53
#define TFT_DC                48
#define TFT_RST               49

// Screen dimensions
#define TFT_WIDTH             240
#define TFT_HEIGHT            320

// SPI Frequency
#define SPI_FREQUENCY         27000000

// Font selection
#define LOAD_GLCD            1
#define LOAD_FONT2           1
#define LOAD_FONT4           1
#define LOAD_FONT6           1
#define LOAD_FONT7           1
#define LOAD_FONT8           1
#define LOAD_GFXFF           1
#define SMOOTH_FONT          1

#endif // TFT_CONFIG_H
