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

- a default Esc + workspaces 1-10 + GPT + media + Omarchy row;
- an Fn row containing Esc and F1-F12, including F9 push-to-talk;
- a MEDIA submenu with Back, brightness, playback and volume;
- a GPT submenu with Back, ChatGPT/voice/clipboard/capture/copy/paste tools;
- a Control+Fn live status and system row.

The home-row workspace actions dispatch directly to current Omarchy Hyprland,
so they do not depend on synthetic F13-F22 bindings. GPT and MEDIA are true
internal bar menus. The userspace patch also coalesces noisy Fn release events:
the function row appears immediately, and the home row returns after a 600 ms
quiet release instead of flooding the DRM device with redraws.

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

On the originating MacBookPro13,2, the live test reached stable configuration
2, created the 2170x60 DRM card, rendered the home row, opened the config-2
touch digitizer, created its uinput keyboard, and started the Fn watcher. The
same script then restored a healthy two-interface firmware configuration-1
topology and successfully reactivated custom mode, without a host reboot.
The home/media/GPT layer swaps, direct workspace dispatch and kernel-timeout
checks have been exercised on the originating machine. Longer suspend, reboot,
shutdown and person-at-keyboard testing are still required before persistence
is considered.

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
before sending its standard shortcuts. No file below `/usr/share/omarchy` is
edited, and the older F13-F24 examples remain available only as legacy design
references.

## Full visual workspace layer

A future enhancement can tint the active workspace by feeding Hyprland state
back to the renderer. The current action path is:

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
8. Add active-workspace IPC tinting after the static workspace row is proven.
9. Only then enable the custom daemon at boot.

Phases 1-6 are complete as a non-persistent live test on the originating
Omarchy machine. Phase 7 still needs person-at-keyboard testing, and persistent
installation remains gated: the upstream installer assumes a different DKMS
source version and must not be run unchanged.

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
