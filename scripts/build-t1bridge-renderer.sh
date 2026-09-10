#!/usr/bin/env bash
# Build (and optionally install) the unprivileged T1Bridge Prism renderer.
# Usage: build-t1bridge-renderer.sh [--install]
#   --install copies the three artifacts into the user-owned install directory
#   and restarts the t1-touchbar user service.
set -euo pipefail
repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
dfrd_root="$repo_root/.t1-work/custom-bar/macbook-t1-linux/touch-bar/dfrd"
build_root="$repo_root/.t1-work/t1bridge-renderer"
install_root="${HOME}/.local/lib/omarchy-t1-touchbar-t1bridge"
do_install=0
for arg in "$@"; do
  case $arg in
    --install) do_install=1 ;;
    *) printf 'unknown option: %s\n' "$arg" >&2; exit 2 ;;
  esac
done
if [[ ! -f $dfrd_root/dfr-render.c ]]; then
 T1_CUSTOM_SKIP_KERNEL=1 "$repo_root/scripts/build-custom-bar.sh"
fi
mkdir -p "$build_root"
install -m644 "$repo_root/custom-bar/dfr-layout.h" "$dfrd_root/dfr-layout.h"
cc -O2 -Wall -Wextra -Wno-missing-field-initializers -Werror \
 -ffunction-sections -fdata-sections -Wl,--gc-sections \
 -I"$dfrd_root" $(pkg-config --cflags libdrm freetype2 fontconfig xkbcommon json-c) \
 "$repo_root/custom-bar/t1bridge-render.c" \
 $(pkg-config --libs libdrm freetype2 fontconfig xkbcommon json-c) \
 -o "$build_root/omarchy-touchbar-renderer"
cc -O2 -Wall -Wextra -Werror -I"$repo_root/custom-bar" \
 "$repo_root/custom-bar/dfr-contextd.c" -o "$build_root/dfr-contextd"
install -m755 "$repo_root/custom-bar/touchbar-action.sh" "$build_root/touchbar-action"

cc -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers \
  -I"$repo_root/custom-bar" \
  -o "$build_root/test-layout" "$repo_root/custom-bar/test-layout.c"
"$build_root/test-layout"

# Render every layout with the actual Prism renderer, including the faked
# dynamic APPS/recording states, so design changes are reviewable offline.
preview_root="$build_root/previews"
mkdir -p "$preview_root"
layouts=(fn media ctrl alt meta browser terminal code nvim files comms document
         reader music universal capture)
for layout in "${layouts[@]}"; do
  "$build_root/omarchy-touchbar-renderer" -l "$layout" \
    --preview "$preview_root/$layout.ppm" >/dev/null
  if command -v magick >/dev/null 2>&1; then
    magick "$preview_root/$layout.ppm" "$preview_root/$layout.png"
  fi
done
DFR_PREVIEW_HINT=browser "$build_root/omarchy-touchbar-renderer" -l media \
  --preview "$preview_root/media-apps-browser.ppm" >/dev/null
DFR_PREVIEW_HINT=terminal DFR_PREVIEW_RECORDING=1 \
  "$build_root/omarchy-touchbar-renderer" -l media \
  --preview "$preview_root/media-apps-recording.ppm" >/dev/null
if command -v magick >/dev/null 2>&1; then
  magick "$preview_root/media-apps-browser.ppm" "$preview_root/media-apps-browser.png"
  magick "$preview_root/media-apps-recording.ppm" "$preview_root/media-apps-recording.png"
fi

if [[ $do_install == 1 ]]; then
  mkdir -p "$install_root"
  install -m755 "$build_root/omarchy-touchbar-renderer" "$install_root/"
  install -m755 "$build_root/dfr-contextd" "$install_root/"
  install -m755 "$build_root/touchbar-action" "$install_root/"
  systemctl --user restart t1-touchbar
  printf 'Installed to %s and restarted t1-touchbar.\n' "$install_root"
else
  printf 'Built %s\n' "$build_root/omarchy-touchbar-renderer"
  printf 'Previews: %s\n' "$preview_root"
fi
