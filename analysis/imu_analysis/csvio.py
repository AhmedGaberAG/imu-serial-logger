"""Read the CSV format produced by the firmware and the C++ logger."""

from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Union

import numpy as np

COLUMNS = (
    "t_us",
    "ax_g",
    "ay_g",
    "az_g",
    "gx_dps",
    "gy_dps",
    "gz_dps",
)


@dataclass(frozen=True)
class SensorData:
    """Samples from one log, with time in seconds from the first sample."""

    time_s: np.ndarray
    accel: np.ndarray
    gyro: np.ndarray

    def __len__(self) -> int:
        return int(self.time_s.shape[0])

    @property
    def sample_rate_hz(self) -> float:
        if len(self) < 2:
            raise ValueError("at least two samples are needed")
        deltas = np.diff(self.time_s)
        deltas = deltas[deltas > 0]
        if deltas.size == 0:
            raise ValueError("timestamps are not increasing")
        return float(1.0 / np.median(deltas))


def _is_comment_or_header(fields: list[str]) -> bool:
    first = fields[0].strip()
    return first.startswith("#") or first == "t_us"


def load_csv(path: Union[str, Path]) -> SensorData:
    rows: list[list[float]] = []
    with open(path, newline="", encoding="utf-8") as handle:
        reader = csv.reader(handle)
        for line_number, fields in enumerate(reader, start=1):
            if not fields or not any(field.strip() for field in fields):
                continue
            if _is_comment_or_header(fields):
                continue
            if len(fields) != len(COLUMNS):
                raise ValueError(
                    f"{path}:{line_number}: expected {len(COLUMNS)} fields, "
                    f"got {len(fields)}"
                )
            try:
                rows.append([float(field) for field in fields])
            except ValueError as exc:
                raise ValueError(
                    f"{path}:{line_number}: invalid number"
                ) from exc

    if len(rows) < 2:
        raise ValueError(f"{path}: need at least two data rows")

    data = np.asarray(rows, dtype=np.float64)
    time_us = data[:, 0]
    if not np.all(np.isfinite(data)):
        raise ValueError(f"{path}: data contains NaN or infinity")
    if np.any(np.diff(time_us) <= 0):
        raise ValueError(f"{path}: timestamps must strictly increase")

    time_s = (time_us - time_us[0]) / 1_000_000.0
    return SensorData(
        time_s=time_s,
        accel=data[:, 1:4].copy(),
        gyro=data[:, 4:7].copy(),
    )
