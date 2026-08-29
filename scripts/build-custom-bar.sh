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
  "$repo_root/custom-bar/patches/dfr-workspace-drag.patch"; do
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
if [[ $skip_kernel != 1 ]]; then
  make -C "$kmod_root" clean
  make -C "$kmod_root" -j"$(nproc)"
fi

cc -O2 -Wall -Wextra -Werror \
  -I"$repo_root/custom-bar" \
  -o "$work_root/test-layout" "$repo_root/custom-bar/test-layout.c"
"$work_root/test-layout"

for layout in media fn ctrl alt meta; do
  "$dfrd_root/dfr-render" -l "$layout" --preview "$preview_root/$layout.ppm"
  if command -v magick >/dev/null 2>&1; then
    magick "$preview_root/$layout.ppm" "$preview_root/$layout.png"
  fi
done

printf '\nCustom bar built without touching the live T1.\n'
printf 'Binaries: %s/{dfr-render,dfr-touchd,dfr-fnd}\n' "$dfrd_root"
if [[ $skip_kernel == 1 ]]; then
  printf 'Kernel modules: skipped (T1_CUSTOM_SKIP_KERNEL=1).\n'
else
  printf 'Kernel modules: %s/{appletbdrm,apple_dfr_cfgsel}.ko\n' "$kmod_root"
fi
printf 'Previews: %s\n' "$preview_root"
printf 'USB configuration remains unchanged.\n'
