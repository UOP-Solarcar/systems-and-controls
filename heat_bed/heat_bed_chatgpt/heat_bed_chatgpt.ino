#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/wdt.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pins
const uint8_t relayPins[]  = {2, 3, 4, 5};
const uint8_t muxPins[]    = {6, 7, 8, 9};
const uint8_t muxEN        = 10;
const uint8_t muxSIG       = A0;
const uint8_t potPin       = A1;

// Timing constants (ms)
const unsigned long READ_INTERVAL    = 200;   // read sensors every 100 ms
const unsigned long DISPLAY_INTERVAL = 500;   // update OLED every 500 ms
const unsigned long RELAY_PULSE_MS   = 100;   // pulse relay ON time
const unsigned long RELAY_PAUSE_MS   = 5;     // pause between relay pulses

// Thermistor & pullup
const float R_PULLUP = 10000.0f;
const float R0       = 100000.0f;
const float T0       = 298.15f;
const float BETA     = 4000.0f;

// State
float lastTemps[4]      = {0,0,0,0};
int   potThreshold      = 80;
unsigned long prevReadTime    = 0;
unsigned long prevDisplayTime = 0;

// Forward declarations
float  readMuxTemp(uint8_t channel);
int    readPot();
void   updateRelays(const float temps[], int threshold);
void   redrawDisplay(const float temps[], int threshold);

void setup() {
  // Serial.begin(115200);
  wdt_enable(WDTO_2S);       // 2 s watchdog
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);  // fatal OLED init failure
  }
  display.clearDisplay();
  display.display();

  // Pin modes
  for (auto p : relayPins) pinMode(p, OUTPUT), digitalWrite(p, LOW);
  for (auto p : muxPins)   pinMode(p, OUTPUT), digitalWrite(p, LOW);
  pinMode(muxEN, OUTPUT);
  pinMode(muxSIG, INPUT);
  pinMode(potPin, INPUT);

  // Initial display
  redrawDisplay(lastTemps, potThreshold);
}

void loop() {
  unsigned long now = millis();
  wdt_reset();

  // 1) Periodic sensor read
  if (now - prevReadTime >= READ_INTERVAL) {
    prevReadTime = now;
    float temps[4];
    for (uint8_t ch = 0; ch < 4; ch++) {
      temps[ch] = readMuxTemp(ch);
    }
    potThreshold = readPot();
    updateRelays(temps, potThreshold);

    // Store for display comparison
    for (int i=0; i<4; i++) lastTemps[i] = temps[i];
  }

  // 2) Periodic display update
  if (now - prevDisplayTime >= DISPLAY_INTERVAL) {
    prevDisplayTime = now;
    redrawDisplay(lastTemps, potThreshold);
  }
}

// Read one channel via mux, convert to °C
float readMuxTemp(uint8_t ch) {
  // select mux channel
  for (uint8_t b=0; b<4; b++) {
    digitalWrite(muxPins[b], (ch & (1<<b)) ? HIGH : LOW);
  }
  digitalWrite(muxEN, LOW);
  delayMicroseconds(50);          // small settling time
  int raw = analogRead(muxSIG);
  digitalWrite(muxEN, HIGH);

  if (raw <= 0) return -273.15f;
  float Rtherm = R_PULLUP * (1023.0f - raw) / raw;
  float invT   = (1.0f/T0) + (1.0f/BETA) * log(Rtherm / R0);
  float tK     = 1.0f / invT;
  return tK - 273.15f;
}

// Map pot reading to temperature threshold
int readPot() {
  return map(analogRead(potPin), 0, 1023, 20, 180);
}

// Pulse each relay if its temp is below threshold
void updateRelays(const float temps[], int threshold) {
  for (uint8_t i = 0; i < 4; i++) {
    if (temps[i] < threshold) {
      digitalWrite(relayPins[i], HIGH);
      delay(RELAY_PULSE_MS);
      digitalWrite(relayPins[i], LOW);
      delay(RELAY_PAUSE_MS);
    }
  }
}

// Draw setpoint and 4 temps
void redrawDisplay(const float temps[], int threshold) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Set:");
  display.print(threshold);
  display.print("C");

  // Smaller text for channel readings
  display.setTextSize(1);
  char buf[16];
  for (uint8_t i = 0; i < 4; i++) {
    int x = (i % 2) * (SCREEN_WIDTH / 2);
    int y = 16 + (i / 2) * (SCREEN_HEIGHT / 2);
    display.setCursor(x, y);
    // format into buf, then print
    snprintf(buf, sizeof(buf), "%d: %.1fC", i + 1, temps[i]);
    display.print(buf);
  }

  display.display();
}
