#!/usr/bin/env bash
# Build and preview the Omarchy T1 custom bar without changing the live device.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work_root=${T1_CUSTOM_WORKDIR:-"$repo_root/.t1-work/custom-bar"}
source_root="$work_root/macbook-t1-linux"
dfrd_root="$source_root/touch-bar/dfrd"
kmod_root="$source_root/touch-bar/kernel/t1-touchbar-display"
skip_kernel=${T1_CUSTOM_SKIP_KERNEL:-0}
preview_root="$work_root/previews"
upstream=https://github.com/xeeban/macbook-t1-linux.git
pin=982d97a61a0c834d03e7cf7af5203b11ad7bae44

mkdir -p "$work_root" "$preview_root"

if [[ ! -d "$source_root/.git" ]]; then
  git clone --filter=blob:none "$upstream" "$source_root"
fi

actual_origin=$(git -C "$source_root" remote get-url origin)
[[ $actual_origin == "$upstream" ]] || {
  printf 'ERROR: unexpected custom-bar source origin: %s\n' "$actual_origin" >&2
  exit 1
}

git -C "$source_root" fetch --quiet origin "$pin"
git -C "$source_root" checkout --quiet --detach "$pin"

# This checkout is an ignored, disposable build cache. Restore only the files
# owned by our patch stack so repeated builds always begin from the pinned
# source and incremental patches do not confuse reverse-apply detection.
git -C "$source_root" restore --source "$pin" --staged --worktree -- \
  touch-bar/dfrd/dfr-fnd.c \
  touch-bar/dfrd/dfr-layout.h \
  touch-bar/dfrd/dfr-render.c \
  touch-bar/dfrd/dfr-touchd.c \
  touch-bar/dfrd/dfrd-run.sh \
  touch-bar/kernel/t1-touchbar-display/appletbdrm.c

for patch_file in \
  "$repo_root/custom-bar/patches/dfr-layer-actions.patch" \
  "$repo_root/custom-bar/patches/dfr-workspace-occupancy.patch" \
  "$repo_root/custom-bar/patches/dfr-workspace-drag.patch" \
  "$repo_root/custom-bar/patches/dfr-context-engine.patch" \
  "$repo_root/custom-bar/patches/dfr-oled-efficiency.patch"; do
  if ! git -C "$source_root" apply --check "$patch_file"; then
    printf 'ERROR: custom patch no longer applies cleanly: %s\n' "$patch_file" >&2
    exit 1
  fi
  git -C "$source_root" apply "$patch_file"
done

# Install the Omarchy layout after applying the userspace behavior patch.
install -m0644 "$repo_root/custom-bar/dfr-layout.h" "$dfrd_root/dfr-layout.h"

make -C "$dfrd_root" clean
make -C "$dfrd_root" -j"$(nproc)"
cc -O2 -Wall -Wextra -Werror \
  -I"$repo_root/custom-bar" \
  -o "$dfrd_root/dfr-contextd" "$repo_root/custom-bar/dfr-contextd.c"
if [[ $skip_kernel != 1 ]]; then
  make -C "$kmod_root" clean
  make -C "$kmod_root" -j"$(nproc)"
fi

cc -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers \
  -I"$repo_root/custom-bar" \
  -o "$work_root/test-layout" "$repo_root/custom-bar/test-layout.c"
"$work_root/test-layout"

mapfile -t layout_actions < <(
  sed -n 's#.*touchbar-action \([a-z0-9-]*\).*#\1#p' \
    "$repo_root/custom-bar/dfr-layout.h" | sort -u
)
for action in "${layout_actions[@]}"; do
  grep -Eq "^[[:space:]]*${action}\\)" \
    "$repo_root/custom-bar/touchbar-action.sh" || {
      printf 'ERROR: layout references missing touchbar-action: %s\n' "$action" >&2
      exit 1
    }
done

# Hyprland resolves these names through xkbcommon. `ESC` looks plausible but
# is not a real keysym name, so keep a regression guard beside the build.
if grep -Eq 'send_active_sequence[[:space:]]+ESC([[:space:]]|$)' \
    "$repo_root/custom-bar/touchbar-action.sh"; then
  printf 'ERROR: Nvim sequence uses invalid xkbcommon keysym ESC; use ESCAPE\n' >&2
  exit 1
fi
grep -Eq 'nvim-files\)[[:space:]]+send_active_sequence[[:space:]]+ESCAPE[[:space:]]' \
  "$repo_root/custom-bar/touchbar-action.sh" || {
    printf 'ERROR: Nvim actions must begin with the Escape keysym\n' >&2
    exit 1
  }

assert_context_layout() {
  local expected=$1 class=$2 title=${3:-} actual
  if [[ -n $title ]]; then
    actual=$("$dfrd_root/dfr-contextd" --class "$class" "$title")
  else
    actual=$("$dfrd_root/dfr-contextd" --class "$class")
  fi
  if [[ $actual != "$expected" ]]; then
    printf 'ERROR: context map for class=%q title=%q returned %q, expected %q\n' \
      "$class" "$title" "$actual" "$expected" >&2
    exit 1
  fi
}

assert_context_layout meta chatgpt
assert_context_layout terminal foot
assert_context_layout browser chromium
assert_context_layout browser Brave-browser
assert_context_layout browser Vivaldi-stable
assert_context_layout browser helium
assert_context_layout code code
assert_context_layout nvim org.omarchy.nvim
assert_context_layout terminal org.codeberg.dnkl.foot
assert_context_layout terminal org.omarchy.btop
assert_context_layout terminal TUI.float
assert_context_layout files org.gnome.Nautilus
assert_context_layout files org.kde.dolphin
assert_context_layout comms bluebubbles
assert_context_layout comms org.telegram.desktop
assert_context_layout document obsidian
assert_context_layout universal DesktopEditors
assert_context_layout universal libreoffice-writer
assert_context_layout reader org.gnome.Evince
assert_context_layout universal org.pwmt.zathura
assert_context_layout music mpv
assert_context_layout music io.github.celluloid_player.Celluloid
assert_context_layout universal Bitwarden
assert_context_layout browser chromium ChatGPT
assert_context_layout comms chromium Discord
assert_context_layout music chromium YouTube
assert_context_layout universal org.localsend.localsend_app
assert_context_layout comms chromium WhatsApp
assert_context_layout universal org.gnome.Decoder
assert_context_layout universal citizen
assert_context_layout terminal foot 'YouTube uploader'
assert_context_layout code code 'Discord integration'
assert_context_layout browser chromium 'Element API Reference'
assert_context_layout browser chromium 'Signal Processing Handbook'

layouts=(
  fn media ctrl alt meta browser terminal code nvim files comms document reader
  music universal
)
for layout in "${layouts[@]}"; do
  "$dfrd_root/dfr-render" -l "$layout" --preview "$preview_root/$layout.ppm"
  if command -v magick >/dev/null 2>&1; then
    magick "$preview_root/$layout.ppm" "$preview_root/$layout.png"
  fi
done

printf '\nCustom bar built without touching the live T1.\n'
printf 'Binaries: %s/{dfr-render,dfr-touchd,dfr-fnd,dfr-contextd}\n' "$dfrd_root"
if [[ $skip_kernel == 1 ]]; then
  printf 'Kernel modules: skipped (T1_CUSTOM_SKIP_KERNEL=1).\n'
else
  printf 'Kernel modules: %s/{appletbdrm,apple_dfr_cfgsel}.ko\n' "$kmod_root"
fi
printf 'Previews: %s\n' "$preview_root"
printf 'USB configuration remains unchanged.\n'
