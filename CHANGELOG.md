# Changelog

## v1.0.0 - 2026-10-28

### Added

- ESP32 firmware that verifies the MPU6050 and streams CSV at 100 Hz.
- C++17 host logger with POSIX serial support, strict CSV parsing, and
  signal-safe shutdown.
- Python analysis package with CSV loading, moving average, low-pass,
  complementary filter, and static calibration helpers.
- Plotting and calibration command-line tools.
- Parser unit tests, a pseudo-terminal logger integration test, and analysis
  tests.
- GitHub Actions workflow for the logger, analysis, and firmware builds.
- Wiring, protocol, and design documentation.
- Real hardware plots and a 29-second demo video.
- Synthetic sample log for hardware-free demos.

### Verified

- I2C scanner detects the MPU6050 at `0x68`; `WHO_AM_I` returns `0x68`.
- Hardware capture: 800 samples over 8.0 s at 100.0 Hz.
- `ctest`: 2/2 tests pass.
- `pytest`: 17/17 tests pass.
