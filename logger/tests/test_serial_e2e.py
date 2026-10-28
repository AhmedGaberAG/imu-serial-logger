#!/usr/bin/env python3
"""Feed CSV lines through a pseudo-terminal to the logger binary."""

import os
import pty
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def wait_for_rows(path: Path, row_count: int, timeout: float = 5.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if path.exists():
            lines = path.read_text(encoding="utf-8").splitlines()
            if len(lines) >= row_count:
                return
        time.sleep(0.02)
    raise AssertionError("logger did not write the expected rows")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_serial_e2e.py <imu_logger>", file=sys.stderr)
        return 2
    binary = sys.argv[1]

    with tempfile.TemporaryDirectory() as temp_dir:
        output = Path(temp_dir) / "run.csv"
        master, slave = pty.openpty()
        process = subprocess.Popen(
            [binary, "--port", os.ttyname(slave), "--out", str(output)],
            stderr=subprocess.PIPE,
            text=True,
        )
        try:
            time.sleep(0.2)
            payload = (
                b"# t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps\n"
                b"t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps\n"
                b"1000000,0.0,0.0,1.0,0.0,0.0,0.0\n"
                b"1010000,0.1,-0.1,0.9,1.0,2.0,3.0\n"
                b"bad line\n"
                b"1020000,0.2,-0.2,0.8,4.0,5.0,6.0\r\n"
            )
            os.write(master, payload)
            wait_for_rows(output, 4)
            process.send_signal(signal.SIGINT)
            _, stderr = process.communicate(timeout=5)
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            os.close(master)
            os.close(slave)

        if process.returncode != 0:
            print(f"logger exited with {process.returncode}", file=sys.stderr)
            print(stderr, file=sys.stderr)
            return 1

        lines = output.read_text(encoding="utf-8").splitlines()
        if lines[0] != "t_us,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps":
            print("unexpected header", file=sys.stderr)
            return 1
        if len(lines) != 4:
            print(f"expected 3 samples, got {len(lines) - 1}", file=sys.stderr)
            return 1
        if "wrote 3 samples" not in stderr:
            print("missing sample summary", file=sys.stderr)
            return 1
        if "ignored 1 malformed lines" not in stderr:
            print("missing malformed summary", file=sys.stderr)
            return 1

    print("serial end-to-end test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
