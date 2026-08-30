# Programmable T1 Touch Bar on Omarchy

## What is possible

The T1 panel can be used as a custom 2170×60 display with touch input. Public
work in [xeeban/macbook-t1-linux](https://github.com/xeeban/macbook-t1-linux)
demonstrates:

- USB configuration 2 selected at enumeration;
- a T1-enabled `appletbdrm` DRM device;
- RGB framebuffer output to the strip;
- touch reports from HID interrupt endpoint `0x83`;
- a userspace renderer, touch mapper, and `uinput` key injection;
- persistent boot and layered layouts.

That is enough to build an Omarchy-native strip with workspace pills, active
workspace color, media controls, brightness, an Omarchy menu button, focused-app
actions, battery/network status, or any other small surface.

## Built Omarchy overlay

This repository now includes a compiled and offline-render-tested layout
overlay under [`custom-bar/`](../custom-bar/README.md). It pins the public
display stack at commit `982d97a61a0c834d03e7cf7af5203b11ad7bae44` and adds:

- a default Esc + workspaces 1-10 + GPT + media + Omarchy + dictation row;
- live filled highlights for workspaces that contain app windows;
- drag-to-merge for moving all windows between numbered workspaces;
- an Fn row containing Esc and F1-F12;
- a MEDIA submenu with Back, brightness, playback and volume;
- a GPT submenu with Back, ChatGPT/voice/clipboard/capture/copy/paste tools;
- a Control+Fn live status and system row.

The home-row workspace actions dispatch directly to current Omarchy Hyprland,
so they do not depend on synthetic F13-F22 bindings. GPT and MEDIA are true
internal bar menus. The renderer reads Hyprland's local command socket every
400 ms and fills occupied workspace buttons cyan while leaving empty ones dim.
Only a changed occupancy mask triggers a frame, so the indicator stays current
without shell polling or unnecessary T1 traffic.

The digitizer reports continuous horizontal position, not just taps. Press a
number and slide onto another numbered button before lifting to merge every
non-pinned window from the first workspace into the second, then focus the
destination. Lifting on the original number is an ordinary workspace tap;
lifting outside the numbered strip falls back to that same tap.

The pink microphone button is a direct `voxtype record toggle`: tap once to
start Omarchy dictation and tap again to stop, transcribe, and type into the
currently focused field. The GPT menu's `VOICE` button does the same. Calling
Voxtype directly avoids the repeated sub-second recordings that synthetic F9
press/release events could create on this digitizer.

The userspace patch also coalesces noisy Fn up/down events:
tap Fn once for the function row and again for home. A 90 ms edge filter keeps
fast taps, while a latest-state-wins renderer schedules at most one full-frame
USB push every 120 ms. Fn releases never redraw the bar.

The same patch fixes the deeper freeze cause in `appletbdrm`: T1 can queue
multiple short frame-completion/status packets, while the original adaptation
read only one. That backlog eventually blocked the bulk OUT endpoint with
`-ETIMEDOUT (-110)`. Draining the bounded completion queue after every frame
kept both 250 ms switching and a 20-toggle/50 ms stress run error-free.

![Omarchy home row](../custom-bar/previews/home.png)

![Omarchy AI row](../custom-bar/previews/ai.png)

Build and render every row without changing the live T1:

```bash
./scripts/build-custom-bar.sh
```

The build checks every drawn zone against the touch-coordinate mapping and
compiles the renderer, touch daemon and Fn watcher. It deliberately does not
install or activate display mode.

A guarded session-only smoke test is available after reviewing the code:

```bash
sudo ./scripts/live-custom-bar.sh start
./scripts/live-custom-bar.sh status
sudo ./scripts/live-custom-bar.sh stop
```

The live test verifies the exact model, pinned source revision, safe iBridge
slot fix, configuration transition, DRM card and all three daemons. Its udev
rule and service live under `/run`; it neither enables boot persistence nor
reboots. Any failed gate attempts to restore the firmware configuration-1 bar.

After the guarded live test succeeds, install the same built artifacts as a
persistent boot service:

```bash
sudo ./scripts/install-custom-bar-service.sh
systemctl status t1-custom-bar-live.service
```

The installer copies the reviewed binaries and kernel modules to
`/usr/local/lib/omarchy-t1-touchbar`, installs the T1-only udev rule in `/etc`,
and enables `t1-custom-bar-live.service` at `multi-user.target`. It does not
depend on the Git checkout after installation. The service switches from the
firmware strip to programmable USB configuration 2 on every boot and restores
configuration 1 whenever it is stopped. The installer also enables
`t1-custom-bar-resume.service` and installs a systemd sleep policy selecting
`s2idle`. A pre-sleep hook stops custom USB configuration 2 and parks the T1 in
Apple firmware configuration 1 before the kernel suspends devices. The
fixed-function `apple_touchbar` driver is installed with its existing
reset-resume routine also connected to normal `.resume`; retained USB devices
use that callback during `s2idle`. After wake, one serialized delayed action
returns the device to the programmable renderer. A separate
`t1-custom-bar-lid.service` performs the same pre-close handoff and supplies a
reopen fallback when no systemd sleep target is entered.

Do not select `deep` S3 on this model. Apple firmware does not restore the T1
display pipeline from that state, and neither a USB configuration bounce nor a
driver reload can recover the OLED; it stays black until the host reboots.
`s2idle` is selected both immediately and persistently by the installer.

The MacBookPro13,2 also has two Alpine Ridge Thunderbolt domains that can spend
more than a minute retrying inaccessible PCI devices after `s2idle`. The
installer adds `pcie_port_pm=off`, keeps both NHI devices out of D3cold, and
installs a guarded pre-sleep remove/post-wake rescan hook. The hook refuses PCI
removal if either Thunderbolt tree contains a block device. Do not suspend with
mounted Thunderbolt storage attached.

The in-tree BCM43602 driver is explicitly listed in `modules-load.d` because
PCI coldplug was missed on two observed boots. Wi-Fi is never reloaded from the
sleep hook: this firmware can lose usable radio state after a live reload.
These resume protections follow the independently tested
[MacBookPro13,2 hardware runbook](https://gist.github.com/bgausden/c7f8a3737c1e52a260dfcdb1fb2e90b2).

To remove persistent mode and return to the firmware strip:

```bash
sudo ./scripts/uninstall-custom-bar-service.sh
```

On the originating MacBookPro13,2, the live test reached stable configuration
2, created the 2170x60 DRM card, rendered the home row, opened the config-2
touch digitizer, created its uinput keyboard, and started the Fn watcher. The
same script then restored a healthy two-interface firmware configuration-1
topology and successfully reactivated custom mode, without a host reboot.
The home/media/GPT layer swaps, direct workspace dispatch, kernel-timeout
checks, reboot persistence, and deep-S3 failure mode have been exercised on the
originating machine. The proactive custom-to-firmware-to-custom handoff has
been verified without suspending the host. The updated lid sequence still
requires a physical OLED pass after a clean reboot before it is described as
fully validated. Lid-close recovery must use the installed `s2idle` policy.

## Stable firmware mode versus custom display mode

| Property | Firmware strip | Custom DFR strip |
|---|---|---|
| USB configuration | 1 | 2 |
| Renderer | T1 EmbeddedOS | Linux userspace |
| Esc/F-keys | built in | must be drawn and mapped |
| Custom pixels | no | yes |
| Custom touch actions | limited HID keys | yes |
| Webcam | exposed in config 1 | must be revalidated |
| Complexity | low after activation | kernel + DRM + daemon |
| Current recommendation | daily-driver default | opt-in experiment |

Do not install both modes as if they can own the device simultaneously.
Configuration 2 replaces the stable firmware strip for that boot. Always keep
an Esc button in every custom layout and document a config-1 rollback.

## Omarchy integration

The current layout uses the unprivileged helper
[`touchbar-action.sh`](../custom-bar/touchbar-action.sh). The root touch daemon
drops to the logged-in user before calling it. Workspace buttons use current
Omarchy's native Hyprland Lua dispatcher; ChatGPT buttons focus the native app
before sending its standard shortcuts. The helper supplies `OMARCHY_PATH` to
the touch daemon's intentionally minimal environment so Omarchy menu actions
remain callable. No file below `/usr/share/omarchy` is
edited, and the older F13-F24 examples remain available only as legacy design
references.

## Live visual workspace layer

The renderer now discovers the newest logged-in Hyprland command socket under
`/run/user`, requests workspace JSON directly, and maps window-bearing IDs
1-10 onto the corresponding buttons. The action path remains:

```text
T1 touch ----> root mapper ---- drops privileges ----> touchbar-action
                                                        |
                                                        v
                                              Hyprland/Omarchy IPC
```

The command allowlist is compiled into the reviewed layout. Arbitrary touch
input cannot supply shell text, and desktop commands refuse to run as root.

## Suggested build phases

1. Keep the proven configuration-1 firmware strip as the recovery baseline.
2. Build the pinned custom-display project and verify all offline previews.
3. Compile its two kernel modules against the exact running kernel.
4. Verify the installed `apple_ibridge` has the safe sub-device slot fix.
5. Port and review persistence for the tested `apple-ib-drv/0.1` source layout.
6. Switch to configuration 2 for one live test with a config-1 rollback ready.
7. Validate display, touch, Esc, Fn layer, webcam, suspend/resume, and shutdown.
8. Add occupied-workspace IPC tinting after the static workspace row is proven.
9. Only then enable the custom daemon at boot.

Phases 1-6 and the phase-8 occupancy indicator are complete. The repository's
Omarchy-specific persistent service packages the exact tested artifacts rather
than invoking upstream's GNOME-oriented installer. Phase 7 still needs broader
suspend and kernel-upgrade testing; reinstall after a kernel upgrade so the
copied out-of-tree display modules match the running kernel. The service
detects a mismatch and stops safely instead of crash-looping an incompatible
module.

The current upstream installer contains GNOME-specific commands and assumptions
about an older `apple-ib-drv` DKMS source version. Running it unchanged on this
stack is not recommended. The display protocol is proven; the remaining work is
a careful Omarchy packaging/integration port.

## Rollback design

A custom-mode installer should provide one command that:

- disables its renderer and config-2 selector services;
- removes its blacklist of `apple_ibridge/apple_touchbar`;
- restores the configuration-1 udev rule;
- reinstalls/rebuilds the known-good firmware driver;
- reloads udev configuration;
- asks the user to reboot, without doing so automatically.

Never make a custom strip the only way to produce Esc until this rollback has
been tested.

## Reference

- [T1 custom-display implementation and protocol notes](https://github.com/xeeban/macbook-t1-linux/tree/main/touch-bar)
- [Omarchy workspace binding example](../examples/hypr/touchbar-workspaces.lua)
