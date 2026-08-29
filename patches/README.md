# Third-party build patches

- `libirecovery-x619.patch` adds the verified `iBridge1,1/x619ap` device-table
  row to libirecovery commit
  `95dec3aa25b1e30654ca107eb971971f6a216520`. libirecovery is LGPL.
- `usbmuxd-t1-restored.patch` makes the private usbmuxd see an already-present
  T1 restored device regardless of USB device class and carries the enum rename
  needed with current libplist. It targets usbmuxd commit
  `79c8b38d1488a6b07e1e68f39d8caec3f1a45622`. usbmuxd is GPL-2.0-or-later.

These patches are built into an ignored private prefix. They do not replace
distro binaries. Do not add T1 serials, logs, tickets, FDR data, or personalized
images here.
