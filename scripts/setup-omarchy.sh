#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
set -euo pipefail

if [[ ${EUID} -eq 0 ]]; then
  printf 'Run this Omarchy package step as your normal desktop user, not root.\n' >&2
  exit 1
fi

if ! command -v omarchy >/dev/null 2>&1; then
  printf 'This setup script requires Omarchy and its omarchy pkg command.\n' >&2
  exit 1
fi

packages=(
  base-devel
  git
  autoconf
  automake
  libtool
  pkgconf
  curl
  cpio
  libarchive
  libusb
  openssl
  libzip
  zlib
  readline
  fuse3
  systemd
  json-c
  xz
  python
  libplist
  libimobiledevice
  libimobiledevice-glue
  libtatsu
  libusbmuxd
  usbmuxd
  acpi_call-dkms
  dkms
  linux-headers
  usbutils
  ripgrep
)

printf 'Installing missing build/runtime packages through Omarchy...\n'
omarchy pkg add "${packages[@]}"

kernel_build="/usr/lib/modules/$(uname -r)/build/Makefile"
if [[ ! -e ${kernel_build} ]]; then
  printf '\nKernel headers do not match the running kernel: %s\n' "$(uname -r)" >&2
  printf 'Install the matching headers before running the DKMS installer.\n' >&2
  exit 1
fi

printf '\nOmarchy dependencies are ready. No driver, firmware, or service was changed.\n'
printf 'Next: ./scripts/status.sh\n'
