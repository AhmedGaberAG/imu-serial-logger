/*
 * I2C scanner + MPU6050 identity check.
 *
 * This version is configured for the ESP32 DevKit:
 *   VCC -> 3V3, GND -> GND, SDA -> GPIO21, SCL -> GPIO22, AD0 -> GND.
 *
 * For an Arduino Uno, use 5V/GND/A4/A5, call Wire.begin() with no pin
 * arguments, and replace Serial.printf with Serial.print (AVR does not
 * provide printf on Serial).
 *
 * Serial monitor: 115200 baud.
 * Expected output: "Found device at 0x68" and "WHO_AM_I = 0x68".
 */

#include <Wire.h>

// I2C pins for the ESP32 DevKit. These are ignored on AVR boards.
constexpr uint8_t kSdaPin = 21;
constexpr uint8_t kSclPin = 22;

// 0x68 when AD0 is low, 0x69 when AD0 is high.
constexpr uint8_t kMpuAddress = 0x68;

// Read-only identity register. An MPU6050 answers 0x68.
constexpr uint8_t kWhoAmIRegister = 0x75;

uint8_t readRegister(uint8_t address, uint8_t reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);

  // Repeated START: keep the bus for the read that follows.
  Wire.endTransmission(false);

  Wire.requestFrom(address, static_cast<uint8_t>(1));
  return Wire.available() ? Wire.read() : 0;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(kSdaPin, kSclPin);
  Wire.setClock(400000);
  delay(500);
  Serial.println("I2C scanner started");
}

void loop() {
  // Scan every valid 7-bit address. A return value of 0 means the device
  // acknowledged its address, which is how the scanner detects it.
  for (uint8_t address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", address);
    }
  }

  // Identity check: avoid decoding data from the wrong device.
  Serial.printf("WHO_AM_I = 0x%02X (expect 0x68)\n",
                readRegister(kMpuAddress, kWhoAmIRegister));

  delay(2000);
}
