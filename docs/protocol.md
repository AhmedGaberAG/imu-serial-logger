# Serial CSV protocol

## Link settings

| Setting | Value |
|---|---|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | none |
| Stop bits | 1 |
| Flow control | none |

The ESP32 sends one line per sample at 100 Hz. Lines starting with `#` are
comments and carry metadata such as the header and error messages.

## Columns

```
# t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps
```

| Column | Unit | Notes |
|---|---|---|
| `t_us` | microseconds | Monotonic time since boot |
| `ax_g`, `ay_g`, `az_g` | g | Accelerometer, +-2 g full scale |
| `gx_dps`, `gy_dps`, `gz_dps` | deg/s | Gyroscope, +-250 deg/s full scale |

The sensor is configured with the 44 Hz digital low-pass filter and a 100 Hz
sample-rate divider. The timestamp comes from `esp_timer_get_time()`, so it
does not wrap during normal logging sessions.

## Logger output

`imu_logger` writes the same columns with six decimal places. It adds a header
line without the `#` prefix so the file is a regular CSV. Comment lines,
headers, blank lines, and partial serial lines are ignored. Malformed lines are
counted and reported on stderr when the logger exits.

Example:

```csv
t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps
1520341,0.001200,-0.003100,0.998700,0.150000,-0.090000,0.040000
1530341,0.001400,-0.002900,0.998500,0.140000,-0.100000,0.050000
```

Stop the logger with `Ctrl+C`; the output is flushed before exit.
