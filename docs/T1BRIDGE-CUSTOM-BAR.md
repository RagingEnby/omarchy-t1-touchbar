# Custom Omarchy bar with T1Bridge

The existing coloured layouts can run as an unprivileged T1Bridge renderer.
This uses the official hardware service for display, touch events, Fn events,
Escape/function keys and brightness. The same layout header, artwork, fonts,
workspace indicators, app-context matching and desktop action helper are used.
T1Bridge owns the device and Touch ID; do not enable the older root custom-bar
service, firmware-mode selector, or legacy USB configuration rules alongside it.

Build without installing or changing kernel modules:

```sh
./scripts/build-t1bridge-renderer.sh
```

The build reuses the patched drawing source in the disposable custom-bar build
cache. If the cache is absent it prepares that cache with kernel compilation
explicitly disabled. After changing the underlying patch set, regenerate the
cache with `T1_CUSTOM_SKIP_KERNEL=1 ./scripts/build-custom-bar.sh` first.

Install the three generated files from `.t1-work/t1bridge-renderer/` together
in a user-owned directory: `omarchy-touchbar-renderer`, `dfr-contextd`, and
`touchbar-action`. Set `~/.config/t1bridge/renderer` to an executable symlink
targeting the renderer and restart `systemctl --user restart t1-touchbar`.
The existing T1Bridge service, not an additional hardware daemon, supervises it.
To return to the stock bar, remove that symlink and restart the same user service.

The adapter keeps the original layout geometry and sends landscape XRGB pixels
over the documented T1Bridge socket using one sealed memfd buffer. It waits
for the frame-release event before reusing the buffer or changing the hit map.
This is the service's presentation contract, not a measurement of panel pixels.
Contacts that start during a layout transition are suppressed until lifted.
Tap actions execute on release, workspace drags retain their original source
layout, and Fn toggles the function row. App-context changes use the existing
context daemon with a user-supplied renderer executable path. Display ownership
never moves out of T1Bridge.

Media actions use playerctl/wpctl as the desktop user. Application chords use
Hyprland's key-state dispatcher; menu and workspace commands use the existing
finite action helper. The adapter accepts only the installed 2170×60 geometry.
It fails back to T1Bridge's built-in renderer on process failure. It has no
runtime path that opens DRM, HID or uinput devices or changes USB configuration.

Touch ID enrollment/authentication progress appears as a cosmetic overlay with
a Cancel button using T1Bridge's documented cancellation request. Fingerprint
results and protected state remain entirely in the broker. Sudo, Polkit and
Omarchy lock integration must be configured separately after successful
standard fprintd enrollment and verification, preserving password access.

The old separate Ctrl+Fn keyboard watcher and its idle-blank timer are not
started by this adapter. The System layout remains selectable using its layout
signal; ordinary Fn switching and the layout menu buttons are supported.
System suspend/resume is still an upstream T1Bridge limitation and is not solved
by changing the renderer. Reboot persistence and physical controls need testing
on the installed machine; a successful build or preview alone is insufficient.

## Local panel resume workaround (T1Bridge 0.1.7)

On this MacBookPro13,2, one s2idle cycle resumed the host and Touch ID but left
the panel black. The renderer still contained a valid nonblack frame. HID
display feature report 3 remained in mode 1: the driver's suspend callback
sends `03 01 f4 01`, but its resume callback never restores mode 2. Sending
`03 02 f4 01` and restarting the renderer restored the custom bar, confirmed
visually by the user without rebooting or resetting the T1.

`scripts/t1bridge-display-resume.py` is a narrowly scoped local workaround:
install root-owned, mode 0755, at
`/usr/lib/systemd/system-sleep/t1bridge-display-resume`. It runs only after
sleep on this model, dynamically finds configuration 2's display HID interface,
and changes mode 1 to mode 2. It leaves an already awake panel alone and checks
the acknowledgement. It does not restart services or access the private NCM
link or biometric state. The first automatic test passed on September 9, 2026: approximately 32 seconds
in s2idle, a logged mode 1 -> 2 correction, and user confirmation that both the
custom Touch Bar and Touch ID worked after reopening, without restarting the
renderer. Repeated cycles and longer sleep remain unverified.
Remove the installed hook to undo the workaround. Reassess it when upstream
ships a complete sleep/wake fix; this does not establish overnight battery
efficiency or general T1 suspend reliability.

## Immediate wake and CPU power saving

After a later restart, a 34.5-minute lid closure produced 68 immediate wakeups
and only 1.68 seconds of actual sleep. The firmware's GPE 6D handler notifies
the power button, internal USB controller and audio device together. Disabling
only the T1 USB device's `power/wakeup` stopped the loop: two subsequent tests
stayed suspended until lid-open (22.49 and 232.87 seconds), with the custom bar
and Touch ID physically confirmed working afterward.

Install `scripts/99-t1bridge-no-remote-wake.rules` root-owned mode 0644 in
`/etc/udev/rules.d/` and reload udev rules. Include its absolute installed path
in mkinitcpio `FILES` so it also applies to early USB enumeration. This does not
disable lid wake, USB data, or Touch ID authentication. Rollback is to remove
the rule and its mkinitcpio inclusion, regenerate the boot image, and restore
`enabled` to the matching T1 USB device's `power/wakeup` attribute.

The old `intel_idle.max_cstate=1` boot restriction also prevents deeper CPU
idle states. It has been removed from the local boot configuration and this
repository's Limine fragment; all NVMe and PCIe stability settings remain.
The boot image was backed up before rebuilding. This CPU change needs a reboot
and stability/battery verification; it is not yet a measured battery-life fix.
The short-test battery readings rose while unplugged, so they cannot establish
an overnight drain rate.
