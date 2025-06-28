#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include <Arduino.h>

/*Don't forget to set Sketchbook location in File/Preferences to the path of your UI project (the parent foder of this INO file)*/

/*Change to your screen resolution*/
static const uint16_t screenWidth  = __UI_PROJECT_HOR_RES__;
static const uint16_t screenHeight = __UI_PROJECT_VER_RES__;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    uint16_t touchX = 0, touchY = 0;

    bool touched = false;//tft.getTouch( &touchX, &touchY, 600 );

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
    }
}

#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Example image assets (replace with actual image data)
const uint16_t image1[320 * 240] = { /* Image 1 data */ };
const uint16_t image2[320 * 240] = { /* Image 2 data */ };

bool showImage1 = true; // Flag to toggle between images
String label1 = "Speed: 0 km/h";
String label2 = "Battery: 100%";

void drawUI()
{
    // Clear the screen
    tft.fillScreen(TFT_BLACK);

    // Draw labels
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(label1, 10, 10, 2); // Speed label
    tft.drawString(label2, 10, 40, 2); // Battery label

    // Draw the initial image
    if (showImage1)
    {
        tft.pushImage(100, 100, 120, 120, image1); // Centered image
    }
    else
    {
        tft.pushImage(100, 100, 120, 120, image2);
    }
}

void updateSpeedLabel(int speed)
{
    // Clear the previous speed label area
    tft.fillRect(10, 10, 300, 20, TFT_BLACK);
    tft.drawString("Speed: " + String(speed) + " km/h", 10, 10, 2);
    label1 = "Speed: " + String(speed) + " km/h";
}

void updateBatteryLabel(int battery)
{
    // Clear the previous battery label area
    tft.fillRect(10, 40, 300, 20, TFT_BLACK);
    tft.drawString("Battery: " + String(battery) + "%", 10, 40, 2);
    label2 = "Battery: " + String(battery) + "%";
}

void toggleImage()
{
    showImage1 = !showImage1;
    if (showImage1)
    {
        tft.pushImage(100, 100, 120, 120, image1);
    }
    else
    {
        tft.pushImage(100, 100, 120, 120, image2);
    }
}

void setup()
{
    Serial.begin(115200);

    // Initialize TFT
    tft.init();
    tft.setRotation(1);

    // Draw the initial UI
    drawUI();
}

void loop()
{
    // Example: Update labels and toggle image every 5 seconds
    static unsigned long lastUpdate = 0;
    static int speed = 0;
    static int battery = 100;

    if (millis() - lastUpdate > 5000)
    {
        speed = (speed + 10) % 200; // Simulate speed update
        battery = (battery - 1) % 100; // Simulate battery update

        updateSpeedLabel(speed);
        updateBatteryLabel(battery);
        toggleImage();

        lastUpdate = millis();
    }
}
