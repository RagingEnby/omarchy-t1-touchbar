#!/usr/bin/env bash
# Remove persistent custom mode and restore the firmware Touch Bar.
set -euo pipefail

unit=t1-custom-bar-live.service
install_root=/usr/local/lib/omarchy-t1-touchbar

if [[ $(id -u) -ne 0 ]]; then
  printf 'Run with sudo: sudo %s\n' "$0" >&2
  exit 1
fi

systemctl disable --now "$unit" >/dev/null 2>&1 || true
rm -f -- \
  "/etc/systemd/system/$unit" \
  /etc/udev/rules.d/99-touchbar-dfr.rules \
  "$install_root/dfr-render" \
  "$install_root/dfr-touchd" \
  "$install_root/dfr-fnd" \
  "$install_root/dfrd-run.sh" \
  "$install_root/touchbar-action" \
  "$install_root/custom-bar-service-runner" \
  "$install_root/kernel-release" \
  "$install_root/appletbdrm.ko" \
  "$install_root/apple_dfr_cfgsel.ko"
rmdir -- "$install_root" 2>/dev/null || true
systemctl daemon-reload
udevadm control --reload
modprobe -a apple_ibridge apple_touchbar >/dev/null 2>&1 || true

printf 'Persistent custom mode removed; firmware Touch Bar restored.\n'
