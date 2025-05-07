#include <Arduino.h>

#define SIF_PIN 2
#define FRAME_BITS 96
#define FRAME_BYTES (FRAME_BITS/8)


volatile byte sifData[FRAME_BYTES];
volatile byte packetData[FRAME_BYTES];
volatile int bitIndex = -1;
volatile bool frameReady = false;

unsigned long lastTime = 0;
unsigned long lastDuration = 0;


// heeheehaha silly goofy fardriver parser amogus sus xd
void sifChange() {
  unsigned long now      = micros();
  unsigned long duration = now - lastTime;
  lastTime = now;

  int val = digitalRead(SIF_PIN);
  if (val == LOW && lastDuration > 0) {
    // very long pulse marks bitIndex=0
    if (round((float)lastDuration / duration) >= 31) {
      bitIndex = 0;
      memset((void*)sifData, 0, sizeof(sifData));
    }
    // If in the middle of a frame, decode a bit
    else if (bitIndex >= 0 && bitIndex < FRAME_BITS) {
      float ratio = (float)lastDuration / duration;
      bool bitValue;
      if      (ratio > 1.5)       bitValue = 0;
      else if ((1.0 / ratio) > 1.5) bitValue = 1;
      else {
        // ambiguous pulse: skip
        lastDuration = duration;
        return;
      }

      // set/clear the correct bit
      if (bitValue)  bitSet  (sifData[bitIndex/8], 7 - (bitIndex%8));
      else           bitClear(sifData[bitIndex/8], 7 - (bitIndex%8));

      bitIndex++;

      // full frame received?
      if (bitIndex == FRAME_BITS) {
        // snapshot for loop()
        for (int i = 0; i < FRAME_BYTES; i++) {
          packetData[i] = sifData[i];
        }
        frameReady = true;
        bitIndex   = -1;  // wait for next preamble
      }
    }
  }

  lastDuration = duration;
}

void setup() {
  Serial.begin(115200);
  pinMode(SIF_PIN, INPUT);
  memset((void*)sifData, 0, sizeof(sifData));
  lastTime = micros();

  // Install interrupt on both edges
  attachInterrupt(digitalPinToInterrupt(SIF_PIN), sifChange, CHANGE);
}

void loop() {
  if (frameReady) {
    // Safely snapshot packetData
    noInterrupts();
      byte dataCopy[FRAME_BYTES];
      memcpy(dataCopy, (const void*)packetData, FRAME_BYTES);
      frameReady = false;
    interrupts();

    // CRC check
    byte crc = 0;
    for (int i = 0; i < FRAME_BYTES - 1; i++) {
      crc ^= dataCopy[i];
    }

    if (crc != dataCopy[FRAME_BYTES - 1]) {
      Serial.print("CRC FAILURE: ");
      Serial.print(crc, HEX);
      Serial.print(" != ");
      Serial.println(dataCopy[FRAME_BYTES - 1], HEX);
    } else {
      // parse fields
      byte batteryPercent   = dataCopy[9];
      short currentAmps     = dataCopy[6];
      byte currentPercent   = dataCopy[10];
      int   rpmRaw          = (dataCopy[7] << 8) | dataCopy[8];
      float rpm             = rpmRaw * 1.91;
      bool  brakeActive     = bitRead(dataCopy[4], 5);
      bool  regenActive     = bitRead(dataCopy[4], 3);

      // print to Serial
      Serial.print("Battery %: ");    Serial.print(batteryPercent);
      Serial.print("   Current %: "); Serial.print(currentPercent);
      Serial.print("   Current A: "); Serial.print(currentAmps);
      Serial.print("   RPM: ");       Serial.print(rpm, 1);
      if (brakeActive) Serial.print("   BRAKE");
      if (regenActive) Serial.print("   REGEN");
      Serial.println();
    }
  }
}
