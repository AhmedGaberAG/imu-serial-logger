# Design

## Data path

```
MPU6050 --I2C--> ESP32 --USB serial--> C++ logger --> CSV --> Python analysis
```

Each stage has a narrow contract:

- The firmware reads 14 bytes from the MPU6050, converts raw counts to g and
  deg/s, and prints one CSV line every 10 ms.
- The logger treats the serial stream as untrusted text. It parses complete
  lines, writes only valid samples, and reports malformed input separately.
- The analysis code loads the CSV into `SensorData`, then applies filters,
  calibration, or plots.

## Firmware

`firmware/esp32/src/main.cpp` is the streaming build. It wakes the sensor,
sets the digital low-pass filter and sample-rate divider, verifies `WHO_AM_I`,
and prints the header once. The main loop compares `esp_timer_get_time()` with
the next scheduled sample so the output rate stays near 100 Hz even if a single
I2C transaction takes longer than usual.

`firmware/i2c_scanner/i2c_scanner.ino` remains the bring-up sketch. It scans
the bus and prints the identity register every two seconds.

## Logger

The logger is split into small pieces:

- `parser.cpp`: CSV field parsing, validation, and formatting. It is independent
  of serial hardware, which keeps the tests fast.
- `serial_port.cpp`: POSIX termios setup, raw 8N1 mode, and supported baud
  rates.
- `main.cpp`: argument handling, signal handling, line reassembly across reads,
  and output management.

The parser rejects negative timestamps, non-finite values, and rows that do not
contain exactly seven fields. Serial input is reassembled in a buffer so a
sample split across two reads is still parsed correctly.

## Analysis

`analysis/imu_analysis/csvio.py` loads both firmware-style (`#` header) and
logger-style CSV files. Time is rebased to start at zero and the sample rate is
estimated from the median timestamp delta.

`filters.py` provides:

- a centered moving average with edge padding,
- a first-order low-pass filter,
- a complementary filter for roll and pitch.

`calibration.py` estimates the gyro bias from a stationary log and supports a
six-position accelerometer calibration. For accelerometer calibration, collect
data with each axis pointing up and down; the helper uses the per-axis minimum
and maximum to compute bias and scale.

## Tests and CI

- `logger/tests/test_parser.cpp` covers valid rows, CRLF endings, headers,
  malformed fields, and formatting round trips.
- `analysis/tests` covers CSV loading, filter behavior, and calibration math.
- CI builds the logger, runs CTest, runs pytest, and compiles the firmware with
  PlatformIO.
