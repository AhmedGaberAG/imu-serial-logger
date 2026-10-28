/*
 * MPU6050 CSV streamer for the ESP32 DevKit.
 *
 * Wiring: VCC -> 3V3, GND -> GND, SDA -> GPIO21, SCL -> GPIO22, AD0 -> GND.
 * Serial monitor: 115200 baud. Output is CSV with a '#' header, 100 Hz.
 */

#include <Arduino.h>
#include <Wire.h>
#include <esp_timer.h>

namespace {

constexpr uint8_t kSdaPin = 21;
constexpr uint8_t kSclPin = 22;
constexpr uint8_t kMpuAddress = 0x68;

constexpr uint8_t kRegSampleRateDiv = 0x19;
constexpr uint8_t kRegConfig = 0x1A;
constexpr uint8_t kRegGyroConfig = 0x1B;
constexpr uint8_t kRegAccelConfig = 0x1C;
constexpr uint8_t kRegAccelXoutH = 0x3B;
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegWhoAmI = 0x75;

constexpr uint8_t kMpuWhoAmI = 0x68;
constexpr float kAccelScale = 16384.0f;  // +-2 g
constexpr float kGyroScale = 131.0f;     // +-250 deg/s
constexpr int64_t kSamplePeriodUs = 10000;  // 100 Hz

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(kMpuAddress);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(kMpuAddress);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(kMpuAddress, static_cast<uint8_t>(1));
  return Wire.available() ? Wire.read() : 0;
}

bool configureSensor() {
  if (readRegister(kRegWhoAmI) != kMpuWhoAmI) {
    return false;
  }
  writeRegister(kRegPwrMgmt1, 0x00);       // wake up
  writeRegister(kRegSampleRateDiv, 0x09);  // 1 kHz / (1 + 9) = 100 Hz
  writeRegister(kRegConfig, 0x03);         // DLPF ~44 Hz
  writeRegister(kRegGyroConfig, 0x00);     // +-250 deg/s
  writeRegister(kRegAccelConfig, 0x00);    // +-2 g
  return true;
}

struct RawSample {
  int16_t ax;
  int16_t ay;
  int16_t az;
  int16_t gx;
  int16_t gy;
  int16_t gz;
};

bool readRawSample(RawSample& sample) {
  constexpr uint8_t kBytes = 14;  // accel(6) + temp(2) + gyro(6)
  Wire.beginTransmission(kMpuAddress);
  Wire.write(kRegAccelXoutH);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(kMpuAddress, static_cast<uint8_t>(kBytes)) != kBytes) {
    return false;
  }

  uint8_t buffer[kBytes];
  for (uint8_t i = 0; i < kBytes; ++i) {
    buffer[i] = Wire.read();
  }

  auto readWord = [&buffer](uint8_t high) {
    return static_cast<int16_t>((buffer[high] << 8) | buffer[high + 1]);
  };
  sample.ax = readWord(0);
  sample.ay = readWord(2);
  sample.az = readWord(4);
  sample.gx = readWord(8);
  sample.gy = readWord(10);
  sample.gz = readWord(12);
  return true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin(kSdaPin, kSclPin);
  Wire.setClock(400000);
  delay(100);

  if (!configureSensor()) {
    Serial.printf("# ERROR: MPU6050 not found at 0x%02X\n", kMpuAddress);
    while (true) {
      delay(1000);
    }
  }

  Serial.println("# t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps");
}

void loop() {
  static int64_t next_sample_us = esp_timer_get_time();
  const int64_t now_us = esp_timer_get_time();
  if (now_us < next_sample_us) {
    return;
  }
  next_sample_us += kSamplePeriodUs;
  if (next_sample_us < now_us - kSamplePeriodUs) {
    next_sample_us = now_us + kSamplePeriodUs;
  }

  RawSample raw;
  if (!readRawSample(raw)) {
    return;
  }

  Serial.printf("%lld,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n", now_us,
                raw.ax / kAccelScale, raw.ay / kAccelScale,
                raw.az / kAccelScale, raw.gx / kGyroScale,
                raw.gy / kGyroScale, raw.gz / kGyroScale);
}
