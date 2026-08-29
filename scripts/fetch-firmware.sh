#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail

firmware_url='https://swcdn.apple.com/content/downloads/22/59/001-72525-A_7H83CSQW4K/p9dd3a0vdtdssud9qlxd4i73pn389rxugu/EmbeddedOSFirmware.pkg'
firmware_sha256=0c97ab746ec635b34b1bdea4e4722cd0173443e2ba6ede54cc6af3b5e220d230

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)
work_dir="$repo_root/.t1-work"
download_dir="$work_dir/download"
package="$download_dir/EmbeddedOSFirmware-14Y901.pkg"
firmware_dir="$work_dir/firmware"
bundle_relative=usr/standalone/firmware/iBridge1_1Customer.bundle

fail() {
  printf 'T1 firmware: FAILED: %s\n' "$*" >&2
  exit 1
}

if [[ ${EUID} -eq 0 ]]; then
  fail 'download and extract as your normal user, not root'
fi

for command in curl sha256sum bsdtar cpio python3; do
  command -v "$command" >/dev/null 2>&1 ||
    fail "missing $command; run ./scripts/setup-omarchy.sh"
done

if [[ -f $firmware_dir/$bundle_relative/Contents/Resources/BuildManifest.plist &&
      -f $firmware_dir/$bundle_relative/Contents/Resources/Restore.plist &&
      -f $firmware_dir/$bundle_relative/Contents/version.plist ]]; then
  printf 'T1 firmware: verified bundle already exists at %s\n' "$firmware_dir/$bundle_relative"
  exit 0
fi
if [[ -e $firmware_dir ]]; then
  fail "existing firmware directory is incomplete; inspect it manually: $firmware_dir"
fi

mkdir -p "$download_dir"
download_tmp="$package.part.$$"
extract_tmp=
cleanup() {
  rm -f -- "$download_tmp"
  if [[ -n ${extract_tmp:-} && -d $extract_tmp ]]; then
    rm -rf -- "$extract_tmp"
  fi
}
trap cleanup EXIT

if [[ ! -f $package ]]; then
  printf 'T1 firmware: downloading Apple EmbeddedOS build 14Y901\n'
  curl --fail --location --proto '=https' --tlsv1.2 "$firmware_url" -o "$download_tmp"
  printf '%s  %s\n' "$firmware_sha256" "$download_tmp" | sha256sum --check
  mv "$download_tmp" "$package"
else
  printf '%s  %s\n' "$firmware_sha256" "$package" | sha256sum --check ||
    fail 'existing Apple package hash does not match'
fi

extract_tmp=$(mktemp -d "$work_dir/.firmware-extract.XXXXXX")
mkdir -p "$extract_tmp/xar" "$extract_tmp/expanded"
bsdtar -xf "$package" -C "$extract_tmp/xar"
[[ -f $extract_tmp/xar/Payload ]] || fail 'Apple package has no Payload'

python3 "$script_dir/pbzx.py" "$extract_tmp/xar/Payload" >"$extract_tmp/Payload.cpio"

cpio -it <"$extract_tmp/Payload.cpio" >"$extract_tmp/file-list"
while IFS= read -r entry; do
  case "$entry" in
    /*|../*|*/../*|*/..) fail "unsafe path in Apple payload: $entry" ;;
  esac
done <"$extract_tmp/file-list"

(
  cd "$extract_tmp/expanded"
  cpio -idmu --no-absolute-filenames <"$extract_tmp/Payload.cpio"
)

bundle="$extract_tmp/expanded/$bundle_relative"
resources="$bundle/Contents/Resources"
[[ -f $resources/BuildManifest.plist ]] || fail 'BuildManifest.plist is missing'
[[ -f $resources/Restore.plist ]] || fail 'Restore.plist is missing'
[[ -f $bundle/Contents/version.plist ]] || fail 'version.plist is missing'
x619_component=$(find "$resources" -maxdepth 1 -type f -name '*x619*' -print -quit)
[[ -n $x619_component ]] || fail 'x619 firmware components are missing'

mv "$extract_tmp/expanded" "$firmware_dir"
printf '\nT1 firmware: verified Apple bundle extracted at\n  %s\n' "$firmware_dir/$bundle_relative"
printf 'The package and extracted firmware remain ignored and must not be committed.\n'
