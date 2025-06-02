#include <Arduino.h>

#define SIF_PIN 2           // pin that carries the SIF signal

// parsed values
short  battery         = 0;
short  current         = 0;
short  currentPercent  = 0;
short  rpm             = 0;
long   faultCode       = 0;
bool   regen           = false;
bool   brake           = false;

// SIF-decoder state
unsigned long lastTime      = 0;
unsigned long lastDuration  = 0;
byte          lastCrc       = 0;
byte          data[12]      = {0};
int           bitIndex      = -1;

void sifChange()
{
    int val = digitalRead(SIF_PIN);
    unsigned long duration = micros() - lastTime;
    lastTime              = micros();

    if (val == LOW && lastDuration > 0)
    {
        bool  bitComplete = false;
        float ratio       = float(lastDuration) / float(duration);

        if (round(float(lastDuration) / duration) >= 31)
        {
            bitIndex = 0;                       // start-of-frame marker
        }
        else if (ratio > 1.5)
        {
            bitClear(data[bitIndex / 8], 7 - (bitIndex % 8));   // 0-bit
            bitComplete = true;
        }
        else if ((1.0f / ratio) > 1.5)
        {
            bitSet(data[bitIndex / 8], 7 - (bitIndex % 8));     // 1-bit
            bitComplete = true;
        }
        else
        {
            Serial.println(String(duration) + "-" + String(lastDuration));
        }

        if (bitComplete && ++bitIndex == 96)                    // full frame
        {
            bitIndex = 0;

            byte crc = 0;
            for (int i = 0; i < 11; ++i) crc ^= data[i];        // CRC-8

            if (crc != data[11])
            {
                Serial.println("CRC FAILURE: " + String(crc) + "-" +
                               String(data[11]));
            }
            else if (crc != lastCrc)                            // new frame
            {
                lastCrc = crc;

                for (int i = 0; i < 12; ++i) Serial.println(data[i], HEX);
                Serial.println();

                battery        = data[9];
                current        = data[6];
                currentPercent = data[10];
                rpm            = ((data[7] << 8) | data[8]) * 1.91;
                brake          = bitRead(data[4], 5);
                regen          = bitRead(data[4], 3);

                Serial.print("Battery %: "  + String(battery));
                Serial.print(" Current %: " + String(currentPercent));
                Serial.print(" Current A: " + String(current));
                Serial.print(" RPM: "       + String(rpm));
                if (brake) Serial.print(" BRAKE");
                if (regen) Serial.print(" REGEN");
                Serial.println();
            }
        }
    }

    lastDuration = duration;
}

void setup()
{
    Serial.begin(115200);
    pinMode(SIF_PIN, INPUT);
    lastTime = micros();
    attachInterrupt(digitalPinToInterrupt(SIF_PIN), sifChange, CHANGE);
}

void loop()
{
    // nothing to do; everything happens in the ISR
}
