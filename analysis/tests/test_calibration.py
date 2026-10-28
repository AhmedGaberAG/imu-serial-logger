import numpy as np
import pytest

from imu_analysis.calibration import (
    apply_accel_calibration,
    apply_gyro_bias,
    estimate_accel_calibration,
    estimate_gyro_bias,
)


def test_estimate_gyro_bias() -> None:
    gyro = np.tile([0.5, -1.0, 2.0], (10, 1))

    bias = estimate_gyro_bias(gyro)

    np.testing.assert_allclose(bias, [0.5, -1.0, 2.0])


def test_apply_gyro_bias_removes_offset() -> None:
    gyro = np.tile([0.5, -1.0, 2.0], (10, 1))

    corrected = apply_gyro_bias(gyro, np.array([0.5, -1.0, 2.0]))

    np.testing.assert_allclose(corrected, 0.0)


def test_accel_calibration_recovers_ideal_values() -> None:
    bias = np.array([0.10, -0.20, 0.05])
    scale = np.array([1.02, 0.98, 1.05])

    ideal = np.array(
        [
            [1.0, 0.0, 0.0],
            [-1.0, 0.0, 0.0],
            [0.0, 1.0, 0.0],
            [0.0, -1.0, 0.0],
            [0.0, 0.0, 1.0],
            [0.0, 0.0, -1.0],
        ]
    )
    raw = ideal / scale + bias

    estimated_bias, estimated_scale = estimate_accel_calibration(raw)
    corrected = apply_accel_calibration(
        raw, estimated_bias, estimated_scale
    )

    np.testing.assert_allclose(estimated_bias, bias)
    np.testing.assert_allclose(estimated_scale, scale)
    np.testing.assert_allclose(corrected, ideal, atol=1e-12)


def test_accel_calibration_requires_both_extremes() -> None:
    raw = np.array(
        [
            [1.0, 1.0, 0.0],
            [1.0, -1.0, 0.0],
            [1.0, 1.0, 1.0],
            [1.0, -1.0, 1.0],
        ]
    )

    with pytest.raises(ValueError, match="minimum and a maximum"):
        estimate_accel_calibration(raw)
