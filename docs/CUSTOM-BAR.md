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

## Low-risk Omarchy integration: extended keys

The public custom daemon already has an F13-F24 layer. Mapping those otherwise
unused keys in personal Omarchy configuration keeps the privileged daemon
generic and moves desktop policy into `~/.config/hypr`.

Copy the sample into your personal bindings file:

```bash
sed -n '1,200p' examples/hypr/touchbar-workspaces.lua
```

Then paste the reviewed block into `~/.config/hypr/bindings.lua` and verify:

```bash
hyprctl reload
hyprctl configerrors
```

The sample maps:

- F13-F22 to workspaces 1-10;
- F23 to the previous workspace;
- F24 to the next workspace.

This follows Omarchy's customization boundary: personal bindings stay in
`~/.config/hypr`; no files below `/usr/share/omarchy` are edited.

## Full visual workspace layer

A dedicated layer can label buttons `1` through `10`, tint the active
workspace, and emit F13-F22. The clean architecture is:

```text
Hyprland IPC ---- active workspace/status ----> unprivileged state helper
                                                    |
                                                    v
T1 DRM <---- rendered workspace layer <---- custom userspace renderer
   |
   +---- touch zone ----> uinput F13..F22 ----> Omarchy bindings
```

Prefer emitted extended keys over running arbitrary shell commands from a root
touch daemon. It gives Hyprland/Omarchy the final policy decision and reduces
the privileged attack surface.

If commands are used directly, a dispatch can target the first running Hyprland
instance:

```bash
hyprctl --instance 0 dispatch workspace 3
```

For current Omarchy Lua, the native dispatcher form is preferable:

```lua
o.bind("F15", "Touch Bar: workspace 3", hl.dsp.focus({ workspace = "3" }))
```

## Suggested build phases

1. Keep the proven configuration-1 firmware strip as the recovery baseline.
2. Clone and pin the custom-display project; inspect its current kernel and
   installer assumptions.
3. Port its installer to the tested `apple-ib-drv/0.1` source layout rather
   than applying a patch intended for a different DKMS version.
4. Build the renderer and verify an offline preview.
5. Switch to configuration 2 for one live test with a config-1 rollback ready.
6. Validate display, touch, Esc, Fn layer, webcam, suspend/resume, and shutdown.
7. Add the Omarchy workspace layer and active-workspace IPC.
8. Only then enable the custom daemon at boot.

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
