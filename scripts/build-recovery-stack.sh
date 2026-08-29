#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail

libirecovery_commit=95dec3aa25b1e30654ca107eb971971f6a216520
usbmuxd_commit=79c8b38d1488a6b07e1e68f39d8caec3f1a45622

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
work_dir="$repo_root/.t1-work"
source_dir="$work_dir/sources"
prefix="$work_dir/prefix"
jobs=${T1_BUILD_JOBS:-$(nproc)}

fail() {
  printf 'T1 build: FAILED: %s\n' "$*" >&2
  exit 1
}

for command in git autoconf automake libtoolize pkg-config make gcc rg; do
  command -v "$command" >/dev/null 2>&1 ||
    fail "missing $command; run ./scripts/setup-omarchy.sh"
done

if [[ ${EUID} -eq 0 ]]; then
  fail 'build the private stack as your normal user, not root'
fi

mkdir -p "$source_dir" "$prefix"

checkout_pinned() {
  local name=$1
  local url=$2
  local commit=$3
  local destination="$source_dir/$name"
  if [[ ! -d $destination/.git ]]; then
    printf 'T1 build: cloning %s\n' "$name"
    git clone "$url" "$destination"
  fi
  if [[ $(git -C "$destination" rev-parse HEAD) != "$commit" ]]; then
    [[ -z $(git -C "$destination" status --porcelain) ]] ||
      fail "$name has local changes and is not at its pinned commit"
    git -C "$destination" fetch --quiet origin "$commit"
    git -C "$destination" checkout --quiet --detach "$commit"
  fi
  [[ $(git -C "$destination" rev-parse HEAD) == "$commit" ]] ||
    fail "$name commit verification failed"
}

apply_once() {
  local checkout=$1
  local patch=$2
  if git -C "$checkout" apply --check "$patch" >/dev/null 2>&1; then
    git -C "$checkout" apply "$patch"
  elif git -C "$checkout" apply --reverse --check "$patch" >/dev/null 2>&1; then
    printf 'T1 build: patch already applied: %s\n' "$(basename "$patch")"
  else
    fail "patch does not apply cleanly: $patch"
  fi
}

checkout_pinned libirecovery https://github.com/libimobiledevice/libirecovery.git "$libirecovery_commit"
apply_once "$source_dir/libirecovery" "$repo_root/patches/libirecovery-x619.patch"

checkout_pinned usbmuxd https://github.com/libimobiledevice/usbmuxd.git "$usbmuxd_commit"
apply_once "$source_dir/usbmuxd" "$repo_root/patches/usbmuxd-t1-restored.patch"

export PATH="$prefix/bin:$PATH"
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$prefix/lib64/pkgconfig"
export LD_LIBRARY_PATH="$prefix/lib:$prefix/lib64"

printf 'T1 build: building private libirecovery\n'
(
  cd "$source_dir/libirecovery"
  NOCONFIGURE=1 ./autogen.sh
  ./configure --prefix="$prefix" --without-udev --disable-silent-rules
  make -j"$jobs"
  make install
)

printf 'T1 build: building private usbmuxd\n'
(
  cd "$source_dir/usbmuxd"
  NOCONFIGURE=1 ./autogen.sh
  ./configure --prefix="$prefix" --without-systemd --disable-silent-rules
  make -j"$jobs"
)

printf 'T1 build: building guarded idevicerestore\n'
(
  cd "$repo_root"
  NOCONFIGURE=1 ./autogen.sh
  ./configure --prefix="$prefix" --disable-silent-rules
  make -j"$jobs"
  make install
)

restore_bin="$prefix/bin/idevicerestore"
private_mux="$source_dir/usbmuxd/src/usbmuxd"
[[ -x $restore_bin ]] || fail 'guarded idevicerestore binary was not installed'
[[ -x $private_mux ]] || fail 'private usbmuxd binary was not built'

rg -a -q 'T1 phase-14 exact preflight replay' "$restore_bin" ||
  fail 'idevicerestore binary does not contain the phase-14 guard'
rg -a -q 'exact T1 phase-11 EmbeddedOS restore options' "$restore_bin" ||
  fail 'idevicerestore binary does not contain the phase-11 guard'

printf '\nT1 build: complete\n'
printf '  restore tool: %s\n' "$restore_bin"
printf '  private mux:  %s\n' "$private_mux"
printf 'No system library, daemon, firmware, or T1 state was changed.\n'
