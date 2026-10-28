import numpy as np
import pytest

from imu_analysis.filters import (
    complementary_filter,
    lowpass,
    moving_average,
)


def test_moving_average_preserves_constant() -> None:
    signal = np.full(20, 3.5)

    filtered = moving_average(signal, 5)

    np.testing.assert_allclose(filtered, signal)


def test_moving_average_is_centered_with_edge_padding() -> None:
    signal = np.array([0.0, 0.0, 3.0, 0.0, 0.0])

    filtered = moving_average(signal, 3)

    np.testing.assert_allclose(filtered, [0.0, 1.0, 1.0, 1.0, 0.0])


def test_moving_average_handles_multiple_columns() -> None:
    signal = np.column_stack([np.ones(9), np.zeros(9)])

    filtered = moving_average(signal, 3)

    np.testing.assert_allclose(filtered[:, 0], 1.0)
    np.testing.assert_allclose(filtered[:, 1], 0.0)


def test_moving_average_rejects_even_window() -> None:
    with pytest.raises(ValueError, match="odd"):
        moving_average(np.ones(5), 4)


def test_lowpass_preserves_constant() -> None:
    signal = np.full(100, -2.0)

    filtered = lowpass(signal, cutoff_hz=5.0, sample_rate_hz=100.0)

    np.testing.assert_allclose(filtered, signal)


def test_lowpass_attenuates_high_frequency() -> None:
    rate = 100.0
    time = np.arange(1000) / rate
    signal = np.sin(2.0 * np.pi * 25.0 * time)

    filtered = lowpass(signal, cutoff_hz=5.0, sample_rate_hz=rate)

    assert np.std(filtered[100:]) < 0.3 * np.std(signal[100:])


def test_lowpass_rejects_cutoff_above_nyquist() -> None:
    with pytest.raises(ValueError, match="Nyquist"):
        lowpass(np.ones(10), cutoff_hz=60.0, sample_rate_hz=100.0)


def test_complementary_filter_stationary_is_level() -> None:
    samples = 50
    accel = np.tile([0.0, 0.0, 1.0], (samples, 1))
    gyro = np.zeros((samples, 3))

    roll, pitch = complementary_filter(accel, gyro, sample_rate_hz=100.0)

    np.testing.assert_allclose(roll, 0.0, atol=1e-9)
    np.testing.assert_allclose(pitch, 0.0, atol=1e-9)


def test_complementary_filter_tracks_accel_tilt() -> None:
    samples = 20
    accel = np.tile([0.0, 1.0, 0.0], (samples, 1))
    gyro = np.zeros((samples, 3))

    roll, pitch = complementary_filter(accel, gyro, sample_rate_hz=100.0)

    np.testing.assert_allclose(roll, 90.0, atol=1e-9)
    np.testing.assert_allclose(pitch, 0.0, atol=1e-9)
