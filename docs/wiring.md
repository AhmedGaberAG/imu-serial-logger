# Wiring and bring-up

## Parts

- ESP32 DevKit
- MPU6050 on a GY-521 breakout
- USB data cable
- Four jumper wires

## Connections

| MPU6050 (GY-521) | ESP32 DevKit |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 (D21) |
| SCL | GPIO 22 (D22) |
| AD0 | GND (address `0x68`) |
| INT | not connected |

The MPU6050 and its breakout are 3.3 V devices. Do not power the module from
5 V while it is connected to the ESP32. Most GY-521 boards include I2C
pull-ups, so no external resistors are needed for a single short bus.

## Bring-up sequence

1. Flash the scanner in `firmware/i2c_scanner` with the Arduino IDE, or use the
   PlatformIO project to build the streamer.
2. Open the serial monitor at 115200 baud.
3. Confirm the scanner reports `Found device at 0x68` and
   `WHO_AM_I = 0x68`.
4. Flash `firmware/esp32` and confirm CSV lines appear:

   ```
   # t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps
   1520341,0.0012,-0.0031,0.9987,0.15,-0.09,0.04
   ```

If AD0 is tied high, the address is `0x69`. Change `kMpuAddress` in the
firmware accordingly.

## Troubleshooting

| Symptom | Check |
|---|---|
| No device found | SDA/SCL swapped, loose wires, module not powered |
| `WHO_AM_I = 0x00` | Wrong address, bus wiring, or power |
| `WHO_AM_I = 0xFF` | SDA stuck high, device not responding |
| Device resets while logging | USB cable is charge-only or the port is underpowered |
| No serial device | Check `dmesg` for `ttyUSB0`/`ttyACM0`, add the user to `dialout` |
