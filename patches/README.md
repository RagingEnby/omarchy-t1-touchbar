# Third-party build patches

- `libirecovery-x619.patch` adds the verified `iBridge1,1/x619ap` device-table
  row to libirecovery commit
  `95dec3aa25b1e30654ca107eb971971f6a216520`. libirecovery is LGPL.
- `usbmuxd-t1-restored.patch` makes the private usbmuxd see an already-present
  T1 restored device regardless of USB device class and carries the enum rename
  needed with current libplist. It targets usbmuxd commit
  `79c8b38d1488a6b07e1e68f39d8caec3f1a45622`. usbmuxd is GPL-2.0-or-later.
- `apple-touchbar-s2idle-resume.patch` connects the fixed-function T1 driver's
  existing reset-resume routine to the normal HID resume callback. `s2idle`
  retains the iBridge USB device, so Linux calls `.resume` rather than
  `.reset_resume`. It targets the driver commit pinned by
  `scripts/install-driver-omarchy.sh`; that driver is GPL-2.0.

The recovery-stack patches are built into an ignored private prefix. The
Touch Bar patch is staged into the DKMS source tree without modifying its
pinned checkout. Do not add T1 serials, logs, tickets, FDR data, or
personalized images here.
