#include <Adafruit_GPS.h>
// #include <SoftwareSerial.h>
#include "Wire.h"

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data


// SoftwareSerial mySerial(3, 2);
Adafruit_GPS GPS(&Serial3);

char c;

void setup() {
  Serial.begin(9600);
  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(1000);
}

void loop() {

  clearGPS();

  while (!GPS.newNMEAreceived()) {
    c = GPS.read();
  }

  GPS.parse(GPS.lastNMEA());

  Serial.print("Time: ");
  Serial.print(GPS.hour, DEC);
  Serial.print(':');
  Serial.print(GPS.minute, DEC);
  Serial.print(':');
  Serial.print(GPS.seconds, DEC);
  Serial.print('.');
  Serial.println(GPS.milliseconds);

  Serial.print("Date: ");
  Serial.print(GPS.day, DEC);
  Serial.print('/');
  Serial.print(GPS.month, DEC);
  Serial.print("/20");
  Serial.println(GPS.year, DEC);

  Serial.print("Fix: ");
  Serial.print(GPS.fix);
  Serial.print(" quality: ");
  Serial.println(GPS.fixquality);
  Serial.print("Satellites: ");
  Serial.println(GPS.satellites);

  if (GPS.fix) {
    Serial.print("Location: ");
    Serial.print(GPS.latitude, 4);
    Serial.print(GPS.lat);
    Serial.print(", ");
    Serial.print(GPS.longitude, 4);
    Serial.println(GPS.lon);
    Serial.print("Google Maps location: ");
    Serial.print(GPS.latitudeDegrees, 4);
    Serial.print(", ");
    Serial.println(GPS.longitudeDegrees, 4);

    Serial.print("Speed (knots): ");
    Serial.println(GPS.speed);
    Serial.print("Heading: ");
    Serial.println(GPS.angle);
    Serial.print("Altitude: ");
    Serial.println(GPS.altitude);
  }
  Serial.println("-------------------------------------");
}

void clearGPS() {
  while (!GPS.newNMEAreceived()) {
    c = GPS.read();
  }
  GPS.parse(GPS.lastNMEA());

  while (!GPS.newNMEAreceived()) {
    c = GPS.read();
  }
  GPS.parse(GPS.lastNMEA());
}
