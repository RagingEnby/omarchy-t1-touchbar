#!/usr/bin/env bash
# Build and preview the Omarchy T1 custom bar without changing the live device.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work_root=${T1_CUSTOM_WORKDIR:-"$repo_root/.t1-work/custom-bar"}
source_root="$work_root/macbook-t1-linux"
dfrd_root="$source_root/touch-bar/dfrd"
kmod_root="$source_root/touch-bar/kernel/t1-touchbar-display"
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

patch_file="$repo_root/custom-bar/patches/dfr-layer-actions.patch"
if git -C "$source_root" apply --reverse --check "$patch_file" 2>/dev/null; then
  : # already applied in this disposable build tree
elif git -C "$source_root" apply --check "$patch_file"; then
  git -C "$source_root" apply "$patch_file"
else
  printf 'ERROR: custom dfrd patch no longer applies cleanly\n' >&2
  exit 1
fi

# Install the Omarchy layout after applying the userspace behavior patch.
install -m0644 "$repo_root/custom-bar/dfr-layout.h" "$dfrd_root/dfr-layout.h"

make -C "$dfrd_root" clean
make -C "$dfrd_root" -j"$(nproc)"
make -C "$kmod_root" clean
make -C "$kmod_root" -j"$(nproc)"

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
printf 'Kernel modules: %s/{appletbdrm,apple_dfr_cfgsel}.ko\n' "$kmod_root"
printf 'Previews: %s\n' "$preview_root"
printf 'USB configuration remains unchanged.\n'
