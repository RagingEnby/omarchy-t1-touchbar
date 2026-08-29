# Omarchy custom Touch Bar overlay

This directory contains a GPL-2.0 layout overlay for the programmable T1
Touch Bar userspace stack in
[`xeeban/macbook-t1-linux`](https://github.com/xeeban/macbook-t1-linux).

It does not contain Apple firmware or machine-personalized data. The build
script fetches the pinned public source, applies the small userspace patch in
`patches/`, overlays `dfr-layout.h`, compiles the three daemons, and renders
offline previews. Building does not switch the T1
to USB configuration 2, unload a driver, install a service, or reboot.

Internal layout names are retained for compatibility with upstream `dfr-fnd`:

| Gesture | Internal name | Visible row |
|---|---|---|
| default | `media` | Esc, workspaces 1-10, GPT, media, Omarchy, dictation |
| Fn | `fn` | Esc and F1-F12 |
| Ctrl+Fn | `ctrl` | status and system actions |
| MEDIA button | `alt` | Back, brightness, playback and volume |
| GPT button | `meta` | Back, ChatGPT, voice and productivity actions |

Workspace and Omarchy buttons launch scoped commands as the logged-in desktop
user. GPT and MEDIA are internal layer actions: they repaint the bar without
launching anything until a button inside the selected menu is tapped.
The action helper supplies Omarchy's packaged root explicitly because the root
touch daemon drops privileges into a deliberately minimal desktop environment.

The home row reads Hyprland's local IPC state every 400 ms. Workspace buttons
with one or more app windows get a filled cyan background, bright outline and
white number; empty workspaces stay dim. The renderer redraws only when the
occupied-workspace mask changes, and no shell command is launched for polling.

Workspace zones also accept a horizontal drag. A tap opens that workspace;
dragging a number onto a different number moves every non-pinned window from
the source workspace into the destination, then opens the destination. The
merge runs only after release over another numbered zone, so buttons outside
the workspace strip are never drag targets.

The pink microphone on the home row and `VOICE` in the GPT row directly toggle
Omarchy's Voxtype recorder. Tap once to start, dictate, and tap again to stop,
transcribe, and type into the focused field. This avoids synthetic F9 timing;
the Fn row still exposes an ordinary F9 key for applications that need it.

## Button guide

The complete custom interface is source-controlled in `dfr-layout.h`; the
commands it may launch are allowlisted in `touchbar-action.sh`.

| Row | Button | Action |
|---|---|---|
| Home | `ESC` | Sends Escape |
| Home | `1`–`10` tap | Opens that Hyprland workspace |
| Home | `1`–`10` drag | Moves all non-pinned windows from the first workspace to the drop workspace and opens it |
| Home | highlighted number | Indicates that workspace currently contains at least one app window |
| Home | `GPT` | Opens the GPT/productivity row |
| Home | `MEDIA` | Opens brightness, playback, and volume controls |
| Home | `OM` | Opens the main Omarchy menu |
| Home | microphone | Starts or stops Omarchy/Voxtype dictation |
| Function | `ESC`, `F1`–`F12` | Sends the corresponding ordinary keyboard key |
| System | `BACK` | Returns home |
| System | `KBD`, `WIFI`, `BT` | Opens the Omarchy hardware menu |
| System | `BAT` | Opens the Omarchy system menu |
| System | `CAPTURE` | Opens Omarchy capture tools |
| System | `LOCK` | Locks the computer |
| Media | `BACK` | Returns home |
| Media | brightness icons | Changes display brightness |
| Media | previous/play/next | Controls media playback |
| Media | mute/volume icons | Controls audio volume |
| GPT | `BACK` | Returns home |
| GPT | `OPEN` | Opens or focuses ChatGPT |
| GPT | `VOICE` | Starts or stops Omarchy/Voxtype dictation |
| GPT | `NEW` | Sends ChatGPT's new-chat shortcut |
| GPT | `CMD` | Sends ChatGPT's command/search shortcut |
| GPT | `CLIP` | Opens Omarchy clipboard history |
| GPT | `SHOT` | Opens Omarchy capture tools |
| GPT | `COPY`, `PASTE` | Sends the corresponding ChatGPT shortcut |
| GPT | `TERM` | Opens the Omarchy terminal |
| GPT | `EMOJI` | Opens the Omarchy emoji picker |

Tap Fn to switch between the home and function rows. `Ctrl+Fn` opens the
system row. `GPT`, `MEDIA`, and `BACK` change rows entirely inside the Touch
Bar; they do not launch desktop windows by themselves.

The patch also filters the T1 keyboard's false Fn up/down pulses. Tap Fn once
to show F1-F12 and tap it again to return home. Legitimate taps can be only
90 ms apart. The renderer settles changes for 35 ms, keeps only the newest
requested layout, and caps full-frame USB pushes at one every 120 ms. Rapid
taps therefore remain responsive without flooding `appletbdrm`.

The kernel half drains every queued T1 frame-completion/status packet after an
upload. Reading only the first short packet eventually filled the bulk IN queue
and made later bulk OUT transfers time out with `-110`. The combined fix passed
12 alternating layouts at 250 ms and a 20-toggle test at 50 ms with no errors.

## Build and preview safely

```bash
./scripts/build-custom-bar.sh
```

Artifacts and previews are written below ignored `.t1-work/custom-bar`; they
are not installed. Live activation remains an explicit, separately guarded
step because display mode replaces the currently working firmware strip.

After reviewing the build and rollback, a session-only test can be started:

```bash
sudo ./scripts/live-custom-bar.sh start
```

For a friend to reproduce the tested path, they need the exact supported T1
model, an already activated production iBridge (`05ac:8600`), Omarchy/Arch,
the required build packages installed by `scripts/setup-omarchy.sh`, and a
working `voxtype` installation if they want dictation. ChatGPT is optional;
its buttons simply cannot focus an app that is not installed. Start with the
root README's read-only `scripts/status.sh` check and follow the path matching
the reported USB state. Do not copy another Mac's personalized T1 files.

It uses only `/run`, does not configure boot persistence, and automatically
restores the firmware bar if a validation gate fails. Return manually with:

```bash
sudo ./scripts/live-custom-bar.sh stop
```

Once the live test is proven on a `MacBookPro13,2`, make it survive shutdowns
and reboots with:

```bash
sudo ./scripts/install-custom-bar-service.sh
```

Rollback to Apple's firmware-rendered strip with:

```bash
sudo ./scripts/uninstall-custom-bar-service.sh
```

## Previews

Home row:

![Home row](previews/home.png)

Function row:

![Function row](previews/function.png)

AI/productivity row:

![AI row](previews/ai.png)

Media row:

![Media row](previews/media.png)

## Licensing and provenance

`dfr-layout.h` is derived from the GPL-2.0 upstream layout at commit
`982d97a61a0c834d03e7cf7af5203b11ad7bae44`. It retains the GPL-2.0 SPDX
identifier. The rest of this repository remains under the licenses described
in the root README.
