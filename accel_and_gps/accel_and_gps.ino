#include <Adafruit_GPS.h>
#include <Wire.h>
#include <due_can.h>

Adafruit_GPS GPS(&Serial3);

char c;

// I2C address of the MPU-6050. If AD0 pin is set to  HIGH, the I2C address will
// be 0x69.
const int MPU_ADDR = 0x68;

void setup() {
  // CAN
  Can0.init(CAN_BPS_500K);
  // GPS
  Serial.begin(9600);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  // Accel
  Wire.begin();
  // Begins a transmission to the I2C slave (GY-521 board)
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}

void loop() {
  // GPS
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
  Serial.print(GPS.milliseconds);

  Serial.print(" | Date: ");
  Serial.print(GPS.day, DEC);
  Serial.print('/');
  Serial.print(GPS.month, DEC);
  Serial.print("/20");
  Serial.print(GPS.year, DEC);

  Serial.print(" | Fix: ");
  Serial.print(GPS.fix);
  Serial.print(" quality: ");
  Serial.print(GPS.fixquality);
  Serial.print(" | Satellites: ");
  Serial.println(GPS.satellites);

  if (GPS.fix) {
    Serial.print("Location: ");
    Serial.print(GPS.latitude, 4);
    Serial.print(GPS.lat);
    Serial.print(", ");
    Serial.print(GPS.longitude, 4);
    Serial.print(GPS.lon);
    Serial.print(" | Google Maps location: ");
    Serial.print(GPS.latitudeDegrees, 4);
    Serial.print(", ");
    Serial.print(GPS.longitudeDegrees, 4);

    Serial.print(" | Speed (knots): ");
    Serial.print(GPS.speed);
    Serial.print(" | Heading: ");
    Serial.print(GPS.angle);
    Serial.print(" | Altitude: ");
    Serial.println(GPS.altitude);
  }

  // Accel
  Wire.beginTransmission(MPU_ADDR);

  // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and
  // MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.write(0x3B);

  // the parameter indicates that the Arduino will send a restart.
  // As a result, the connection is kept active.
  Wire.endTransmission(false);

  // request a total of 7*2=14 registers
  Wire.requestFrom(MPU_ADDR, 7 * 2, true);

  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in
  // the same variable

  // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  int16_t accelerometer_x = Wire.read() << 8 | Wire.read();

  // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  int16_t accelerometer_y = Wire.read() << 8 | Wire.read();

  // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  int16_t accelerometer_z = Wire.read() << 8 | Wire.read();

  // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  int16_t temperature = Wire.read() << 8 | Wire.read();

  // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  int16_t gyro_x = Wire.read() << 8 | Wire.read();

  // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  int16_t gyro_y = Wire.read() << 8 | Wire.read();

  // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
  int16_t gyro_z = Wire.read() << 8 | Wire.read();

  CAN_FRAME can_frame;
  can_frame.id = 0x21;
  can_frame.length = 8;
  can_frame.data.int16[0] = temperature;
  can_frame.data.int16[1] = accelerometer_x;
  can_frame.data.int16[2] = accelerometer_y;
  can_frame.data.int16[3] = accelerometer_z;
  Can0.sendFrame(can_frame);

  can_frame.id = 0x22;
  can_frame.length = 6;
  can_frame.data.int16[0] = gyro_x;
  can_frame.data.int16[1] = gyro_y;
  can_frame.data.int16[2] = gyro_z;
  can_frame.data.int16[3] = 0;
  Can0.sendFrame(can_frame);

  // print out data
  Serial.print("aX = ");
  Serial.print(accelerometer_x);
  Serial.print(" | aY = ");
  Serial.print(accelerometer_y);
  Serial.print(" | aZ = ");
  Serial.print(accelerometer_z);
  // the following equation was taken from the documentation [MPU-6000/MPU-6050
  // Register Map and Description, p.30]
  Serial.print(" | tmp = ");
  Serial.print(temperature / 340.00 + 36.53);

  Serial.print(" | gX = ");
  Serial.print(gyro_x);
  Serial.print(" | gY = ");
  Serial.print(gyro_y);
  Serial.print(" | gZ = ");
  Serial.println(gyro_z);
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
