#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// Gunakan I2C pada PB6 (SCL) dan PB7 (SDA)
TwoWire myWire(PB7, PB6);

// Alamat default BNO055 = 0x28
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &myWire);

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("STM32L432KC + BNO055");
  Serial.println("================================");

  myWire.begin();

  if (!bno.begin())
  {
    Serial.println("BNO055 NOT DETECTED!");
    while (1);
  }

  delay(1000);

  bno.setExtCrystalUse(true);

  Serial.println("BNO055 Initialized");
}

void loop()
{
  sensors_event_t orientationData;
  sensors_event_t angVelocityData;
  sensors_event_t linearAccelData;
  sensors_event_t accelData;
  sensors_event_t magnetometerData;
  sensors_event_t gravityData;

  bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
  bno.getEvent(&angVelocityData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);
  bno.getEvent(&accelData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&magnetometerData, Adafruit_BNO055::VECTOR_MAGNETOMETER);
  bno.getEvent(&gravityData, Adafruit_BNO055::VECTOR_GRAVITY);

  Serial.println("================================");

  Serial.print("Yaw   : ");
  Serial.print(orientationData.orientation.x);
  Serial.print("  Pitch : ");
  Serial.print(orientationData.orientation.y);
  Serial.print("  Roll : ");
  Serial.println(orientationData.orientation.z);

  Serial.print("Gyro X : ");
  Serial.print(angVelocityData.gyro.x);
  Serial.print("  Y : ");
  Serial.print(angVelocityData.gyro.y);
  Serial.print("  Z : ");
  Serial.println(angVelocityData.gyro.z);

  Serial.print("Accel X : ");
  Serial.print(accelData.acceleration.x);
  Serial.print("  Y : ");
  Serial.print(accelData.acceleration.y);
  Serial.print("  Z : ");
  Serial.println(accelData.acceleration.z);

  Serial.print("Linear X : ");
  Serial.print(linearAccelData.acceleration.x);
  Serial.print("  Y : ");
  Serial.print(linearAccelData.acceleration.y);
  Serial.print("  Z : ");
  Serial.println(linearAccelData.acceleration.z);

  Serial.print("Mag X : ");
  Serial.print(magnetometerData.magnetic.x);
  Serial.print("  Y : ");
  Serial.print(magnetometerData.magnetic.y);
  Serial.print("  Z : ");
  Serial.println(magnetometerData.magnetic.z);

  Serial.print("Gravity X : ");
  Serial.print(gravityData.acceleration.x);
  Serial.print("  Y : ");
  Serial.print(gravityData.acceleration.y);
  Serial.print("  Z : ");
  Serial.println(gravityData.acceleration.z);

  uint8_t sys, gyro, accel, mag;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  Serial.print("Calibration -> ");
  Serial.print("SYS:");
  Serial.print(sys);
  Serial.print(" G:");
  Serial.print(gyro);
  Serial.print(" A:");
  Serial.print(accel);
  Serial.print(" M:");
  Serial.println(mag);

  delay(100);
}