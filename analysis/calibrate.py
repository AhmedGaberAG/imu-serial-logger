#!/usr/bin/env python3
"""Estimate static calibration values from a logger CSV file."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from imu_analysis.calibration import (
    estimate_accel_calibration,
    estimate_gyro_bias,
)
from imu_analysis.csvio import load_csv


def parseArgs() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="stationary CSV log")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="write the calibration values as JSON to this path",
    )
    parser.add_argument(
        "--gravity",
        type=float,
        default=1.0,
        help="gravity magnitude in the acceleration units (default 1.0 g)",
    )
    parser.add_argument(
        "--accel",
        action="store_true",
        help="also estimate accelerometer bias/scale (requires six positions)",
    )
    return parser.parse_args()


def main() -> int:
    args = parseArgs()
    data = load_csv(args.csv)
    gyro_bias = estimate_gyro_bias(data.gyro)

    values = {
        "source": str(args.csv),
        "samples": len(data),
        "sample_rate_hz": data.sample_rate_hz,
        "gyro_bias_dps": [float(value) for value in gyro_bias],
    }

    print("gyro bias [deg/s]: " + ", ".join(f"{v:+.4f}" for v in gyro_bias))
    if args.accel:
        print("note: --accel expects a log covering all six orientations")
        accel_bias, accel_scale = estimate_accel_calibration(
            data.accel, gravity=args.gravity
        )
        values["accel_bias_g"] = [float(value) for value in accel_bias]
        values["accel_scale"] = [float(value) for value in accel_scale]
        print(
            "accel bias [g]: "
            + ", ".join(f"{v:+.4f}" for v in accel_bias)
        )
        print(
            "accel scale: " + ", ".join(f"{v:.4f}" for v in accel_scale)
        )

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            json.dumps(values, indent=2) + "\n", encoding="utf-8"
        )
        print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
