from pathlib import Path

import numpy as np
import pytest

from imu_analysis.csvio import load_csv


def writeLog(path: Path, rows: str) -> Path:
    path.write_text(rows, encoding="utf-8")
    return path


def test_loads_firmware_and_logger_format(tmp_path: Path) -> None:
    path = writeLog(
        tmp_path / "run.csv",
        "# t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps\r\n"
        "t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps\r\n"
        "1000000,0.0,0.0,1.0,10.0,20.0,30.0\r\n"
        "\r\n"
        "1010000,0.1,-0.1,0.9,-10.0,-20.0,-30.0\r\n",
    )

    data = load_csv(path)

    assert len(data) == 2
    np.testing.assert_allclose(data.time_s, [0.0, 0.01])
    np.testing.assert_allclose(data.accel[1], [0.1, -0.1, 0.9])
    np.testing.assert_allclose(data.gyro[0], [10.0, 20.0, 30.0])
    assert data.sample_rate_hz == pytest.approx(100.0)


def test_rejects_wrong_field_count(tmp_path: Path) -> None:
    path = writeLog(tmp_path / "bad.csv", "1,2,3\n4,5,6\n")

    with pytest.raises(ValueError, match="expected 7 fields"):
        load_csv(path)


def test_rejects_non_increasing_time(tmp_path: Path) -> None:
    path = writeLog(
        tmp_path / "bad.csv",
        "1000,0,0,1,0,0,0\n"
        "1000,0,0,1,0,0,0\n",
    )

    with pytest.raises(ValueError, match="strictly increase"):
        load_csv(path)


def test_rejects_short_log(tmp_path: Path) -> None:
    path = writeLog(tmp_path / "short.csv", "1000,0,0,1,0,0,0\n")

    with pytest.raises(ValueError, match="at least two"):
        load_csv(path)
