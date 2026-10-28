#!/usr/bin/env python3
"""Plot acceleration and angular velocity from a logger CSV file."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import matplotlib

from imu_analysis.csvio import load_csv
from imu_analysis.filters import lowpass


def parseArgs() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="CSV file produced by imu_logger")
    parser.add_argument(
        "--save",
        type=Path,
        help="save the figure to this path instead of opening a window",
    )
    parser.add_argument(
        "--lowpass",
        type=float,
        metavar="HZ",
        help="overlay a first-order low-pass filtered version",
    )
    parser.add_argument("--title", help="figure title")
    return parser.parse_args()


def main() -> int:
    args = parseArgs()
    if args.save:
        matplotlib.use("Agg")

    import matplotlib.pyplot as plt

    data = load_csv(args.csv)
    rate = data.sample_rate_hz
    time_s = data.time_s

    accel = data.accel
    gyro = data.gyro
    if args.lowpass is not None:
        accel = lowpass(accel, args.lowpass, rate)
        gyro = lowpass(gyro, args.lowpass, rate)

    figure, axes = plt.subplots(2, 1, figsize=(11, 7), sharex=True)
    axis_labels = ("x", "y", "z")

    for index, label in enumerate(axis_labels):
        axes[0].plot(time_s, accel[:, index], linewidth=0.8, label=f"a{label}")
        axes[1].plot(time_s, gyro[:, index], linewidth=0.8, label=f"g{label}")

    axes[0].set_ylabel("acceleration [g]")
    axes[1].set_ylabel("angular velocity [deg/s]")
    axes[1].set_xlabel("time [s]")
    for axis in axes:
        axis.grid(True, alpha=0.3)
        axis.legend(loc="upper right", ncol=3)

    title = args.title or f"IMU log: {args.csv.name}"
    axes[0].set_title(f"{title} ({len(data)} samples, {rate:.1f} Hz)")
    figure.tight_layout()

    if args.save:
        args.save.parent.mkdir(parents=True, exist_ok=True)
        figure.savefig(args.save, dpi=150)
        print(f"wrote {args.save}")
    else:
        plt.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
