"""Static calibration helpers for accelerometer and gyroscope data."""

from __future__ import annotations

import numpy as np


def _samples(x: np.ndarray, name: str) -> np.ndarray:
    data = np.asarray(x, dtype=np.float64)
    if data.ndim != 2 or data.shape[1] != 3:
        raise ValueError(f"{name} must have shape (N, 3)")
    if data.shape[0] == 0:
        raise ValueError(f"{name} must not be empty")
    if not np.all(np.isfinite(data)):
        raise ValueError(f"{name} contains NaN or infinity")
    return data


def estimate_gyro_bias(gyro: np.ndarray) -> np.ndarray:
    """Mean gyro output while the sensor is stationary, in deg/s."""

    data = _samples(gyro, "gyro")
    return np.mean(data, axis=0)


def apply_gyro_bias(gyro: np.ndarray, bias: np.ndarray) -> np.ndarray:
    data = _samples(gyro, "gyro")
    bias = np.asarray(bias, dtype=np.float64)
    if bias.shape != (3,):
        raise ValueError("bias must have shape (3,)")
    return data - bias


def estimate_accel_calibration(
    accel: np.ndarray, gravity: float = 1.0
) -> tuple[np.ndarray, np.ndarray]:
    """Six-position calibration from min/max readings on each axis.

    Rotate the sensor so every axis points up and down. Returns the bias and
    the per-axis scale that maps the readings to units of gravity.
    """

    data = _samples(accel, "accel")
    if not np.isfinite(gravity) or gravity <= 0.0:
        raise ValueError("gravity must be positive")

    minimum = np.min(data, axis=0)
    maximum = np.max(data, axis=0)
    span = maximum - minimum
    if np.any(span <= np.finfo(np.float64).eps):
        raise ValueError("each axis needs both a minimum and a maximum")

    bias = (maximum + minimum) / 2.0
    scale = (2.0 * gravity) / span
    return bias, scale


def apply_accel_calibration(
    accel: np.ndarray, bias: np.ndarray, scale: np.ndarray
) -> np.ndarray:
    data = _samples(accel, "accel")
    bias = np.asarray(bias, dtype=np.float64)
    scale = np.asarray(scale, dtype=np.float64)
    if bias.shape != (3,) or scale.shape != (3,):
        raise ValueError("bias and scale must have shape (3,)")
    return (data - bias) * scale
