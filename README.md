# IMU Serial Logger

Reads raw acceleration and angular velocity from an MPU6050 over I2C, streams
CSV from an ESP32 over USB serial, logs it with a C++ program, and analyses it
in Python.

**Status:** scaffold. Firmware, logger, and analysis are being built as part of
Phase 1 of the Robotics Software Engineer roadmap.

## Hardware

| Part | Notes |
|---|---|
| ESP32 DevKit | Main controller |
| MPU6050 / GY-521 | 3.3 V, I2C |
| USB data cable | Must support data, not charge-only |
| Jumper wires | 4 wires |

Arduino Uno/Nano/Mega can be used as a fallback, but the ESP32 is the target.

## Wiring

| MPU6050 (GY-521) | ESP32 DevKit |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 (D21) |
| SCL | GPIO 22 (D22) |
| AD0 | GND (address `0x68`) |
| INT | not connected |

The MPU6050 is a 3.3 V device. Do not power it from 5 V while it is connected
to the ESP32.

## Architecture

```
MPU6050 --I2C--> ESP32 --USB serial--> C++ logger --> CSV --> Python analysis
```

Each stage is tested independently:

- firmware: I2C scanner and `WHO_AM_I` check,
- logger: parser unit tests,
- analysis: filter unit tests.

## Planned repository layout

```
imu-serial-logger/
├── firmware/
│   ├── arduino/        # bring-up sketch
│   └── esp32/          # PlatformIO project
├── logger/             # C++ host logger (CMake)
├── analysis/           # Python plots, calibration, filters
├── data/               # logs (CSV files ignored except sample.csv)
├── docs/               # wiring, design, experiments
└── media/              # plots and demo assets
```

## Build and run

Not available yet. Planned commands:

```bash
# Host logger
cmake -S logger -B logger/build -DCMAKE_BUILD_TYPE=Debug
cmake --build logger/build -j
./logger/build/imu_logger --port /dev/ttyUSB0 --out data/run.csv

# Firmware
cd firmware/esp32
pio run -t upload
pio device monitor -b 115200

# Analysis
source .venv/bin/activate
analysis/plot.py data/run.csv --save media/raw_signals.png
```

## Status

- [ ] I2C scanner finds the sensor at `0x68`
- [ ] `WHO_AM_I` returns `0x68`
- [ ] Firmware streams CSV at a fixed rate
- [ ] C++ logger writes CSV
- [ ] Python plots, calibration, and filters
- [ ] Tests and CI
- [ ] Demo video and `v1.0.0` release
