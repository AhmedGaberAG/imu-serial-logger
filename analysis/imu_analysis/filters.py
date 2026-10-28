"""Filtering helpers for IMU signals."""

from __future__ import annotations

import numpy as np


def _signal_array(x: np.ndarray, name: str = "x") -> np.ndarray:
    data = np.asarray(x, dtype=np.float64)
    if data.ndim not in (1, 2):
        raise ValueError(f"{name} must be 1-D or 2-D")
    if data.shape[0] == 0:
        raise ValueError(f"{name} must not be empty")
    if not np.all(np.isfinite(data)):
        raise ValueError(f"{name} contains NaN or infinity")
    return data


def moving_average(x: np.ndarray, window: int) -> np.ndarray:
    """Centered moving average along axis 0 with edge padding."""

    data = _signal_array(x)
    if not isinstance(window, (int, np.integer)) or window < 1:
        raise ValueError("window must be a positive integer")
    if window % 2 == 0:
        raise ValueError("window must be odd")
    if window == 1:
        return data.copy()

    pad = window // 2
    padding = ((pad, pad),) + ((0, 0),) * (data.ndim - 1)
    padded = np.pad(data, padding, mode="edge")
    kernel = np.ones(window, dtype=np.float64) / window

    if data.ndim == 1:
        return np.convolve(padded, kernel, mode="valid")

    result = np.empty_like(data)
    for column in range(data.shape[1]):
        result[:, column] = np.convolve(
            padded[:, column], kernel, mode="valid"
        )
    return result


def lowpass(
    x: np.ndarray, cutoff_hz: float, sample_rate_hz: float
) -> np.ndarray:
    """First-order low-pass filter along axis 0."""

    data = _signal_array(x)
    if not np.isfinite(cutoff_hz) or cutoff_hz <= 0.0:
        raise ValueError("cutoff_hz must be positive")
    if not np.isfinite(sample_rate_hz) or sample_rate_hz <= 0.0:
        raise ValueError("sample_rate_hz must be positive")
    if cutoff_hz >= sample_rate_hz / 2.0:
        raise ValueError("cutoff_hz must be below the Nyquist frequency")

    dt = 1.0 / sample_rate_hz
    rc = 1.0 / (2.0 * np.pi * cutoff_hz)
    alpha = dt / (rc + dt)

    result = np.empty_like(data)
    result[0] = data[0]
    for index in range(1, data.shape[0]):
        result[index] = result[index - 1] + alpha * (
            data[index] - result[index - 1]
        )
    return result


def complementary_filter(
    accel: np.ndarray,
    gyro: np.ndarray,
    sample_rate_hz: float,
    alpha: float = 0.98,
) -> tuple[np.ndarray, np.ndarray]:
    """Estimate roll and pitch in degrees from accelerometer and gyroscope."""

    accel = _signal_array(accel, "accel")
    gyro = _signal_array(gyro, "gyro")
    if accel.ndim != 2 or accel.shape[1] != 3:
        raise ValueError("accel must have shape (N, 3)")
    if gyro.shape != accel.shape:
        raise ValueError("gyro must have the same shape as accel")
    if not np.isfinite(sample_rate_hz) or sample_rate_hz <= 0.0:
        raise ValueError("sample_rate_hz must be positive")
    if not 0.0 <= alpha <= 1.0:
        raise ValueError("alpha must be between 0 and 1")

    dt = 1.0 / sample_rate_hz
    ax, ay, az = accel[:, 0], accel[:, 1], accel[:, 2]
    roll_acc = np.degrees(np.arctan2(ay, az))
    pitch_acc = np.degrees(np.arctan2(-ax, np.hypot(ay, az)))

    roll = np.empty(accel.shape[0], dtype=np.float64)
    pitch = np.empty(accel.shape[0], dtype=np.float64)
    roll[0] = roll_acc[0]
    pitch[0] = pitch_acc[0]

    for index in range(1, accel.shape[0]):
        roll_gyro = roll[index - 1] + gyro[index, 0] * dt
        pitch_gyro = pitch[index - 1] + gyro[index, 1] * dt
        roll[index] = alpha * roll_gyro + (1.0 - alpha) * roll_acc[index]
        pitch[index] = (
            alpha * pitch_gyro + (1.0 - alpha) * pitch_acc[index]
        )

    return roll, pitch
