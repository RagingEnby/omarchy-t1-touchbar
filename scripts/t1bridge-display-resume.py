#!/usr/bin/python3
"""Restore the T1 panel wake report omitted by T1Bridge 0.1.7 resume.

Local MacBookPro13,2 workaround. Only display HID feature report 3 is touched;
the USB configuration, NCM link, firmware and biometric services are untouched.
Install as /usr/lib/systemd/system-sleep/t1bridge-display-resume.
"""

import fcntl
import os
from pathlib import Path
import signal
import stat
import sys


def feature(fd, report, write=False):
    request = (3 << 30) | (len(report) << 16) | (ord("H") << 8)
    fcntl.ioctl(fd, request | (6 if write else 7), report)
    if report[0] != 3:
        raise RuntimeError("unexpected display report ID")
    return report[1]


def main():
    if len(sys.argv) != 3 or sys.argv[1] != "post":
        return
    signal.alarm(8)
    if Path("/sys/class/dmi/id/product_name").read_text().strip() != "MacBookPro13,2":
        return
    devices = []
    for device in Path("/sys/bus/usb/devices").glob("*"):
        try:
            if (device / "idVendor").read_text().strip() != "05ac":
                continue
            if (device / "idProduct").read_text().strip() != "8600":
                continue
            if (device / "bConfigurationValue").read_text().strip() != "2":
                continue
            display = Path(str(device) + ":2.3") / "driver"
            if display.resolve().name != "appletbdrm":
                continue
            devices.extend(Path(str(device) + ":2.6").glob("*/hidraw/hidraw*"))
        except FileNotFoundError:
            continue
    if len(devices) != 1:
        print("t1bridge-display-resume: no unique configured T1 display; skipped")
        return
    fd = os.open("/dev/" + devices[0].name, os.O_RDWR | os.O_CLOEXEC | os.O_NOFOLLOW)
    try:
        if not stat.S_ISCHR(os.fstat(fd).st_mode):
            raise RuntimeError("display HID node is not a character device")
        mode = feature(fd, bytearray([3] + [0] * 14))
        if mode == 2:
            print("t1bridge-display-resume: panel already awake")
            return
        if mode != 1:
            raise RuntimeError("unrecognized display mode; left unchanged")
        # Inverse of appletbdrm_t1_park's 03 01 f4 01 report. The 03 02
        # wake report is also documented in Seborider/mbp-t1-touchbar.
        feature(fd, bytearray([3, 2, 0xf4, 1] + [0] * 11), write=True)
        if feature(fd, bytearray([3] + [0] * 14)) != 2:
            raise RuntimeError("panel did not acknowledge awake mode")
        print("t1bridge-display-resume: restored panel sleep -> awake")
    finally:
        os.close(fd)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError) as exc:
        print(f"t1bridge-display-resume: {exc}", file=sys.stderr)
        sys.exit(1)
