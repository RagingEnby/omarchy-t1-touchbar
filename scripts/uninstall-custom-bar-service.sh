#!/usr/bin/env bash
# Remove persistent custom mode and restore the firmware Touch Bar.
set -euo pipefail

unit=t1-custom-bar-live.service
resume_unit=t1-custom-bar-resume.service
lid_unit=t1-custom-bar-lid.service
install_root=/usr/local/lib/omarchy-t1-touchbar
sleep_policy=/etc/systemd/sleep.conf.d/80-t1-touchbar-sleep.conf

if [[ $(id -u) -ne 0 ]]; then
  printf 'Run with sudo: sudo %s\n' "$0" >&2
  exit 1
fi

systemctl disable "$resume_unit" >/dev/null 2>&1 || true
systemctl disable --now "$lid_unit" >/dev/null 2>&1 || true
systemctl disable --now "$unit" >/dev/null 2>&1 || true
rm -f -- \
  "/etc/systemd/system/$unit" \
  "/etc/systemd/system/$resume_unit" \
  "/etc/systemd/system/$lid_unit" \
  "$sleep_policy" \
  /etc/modules-load.d/brcmfmac.conf \
  /etc/udev/rules.d/86-mbp132-thunderbolt-d3cold.rules \
  /usr/lib/systemd/system-sleep/mbp132-thunderbolt-sleep \
  /etc/limine-entry-tool.d/20-macbookpro13-t1-suspend.conf \
  /etc/udev/rules.d/99-touchbar-dfr.rules \
  "$install_root/dfr-render" \
  "$install_root/dfr-touchd" \
  "$install_root/dfr-fnd" \
  "$install_root/dfrd-run.sh" \
  "$install_root/touchbar-action" \
  "$install_root/custom-bar-service-runner" \
  "$install_root/t1-lid-watcher" \
  "$install_root/t1-custom-bar-recover" \
  "$install_root/kernel-release" \
  "$install_root/appletbdrm.ko" \
  "$install_root/apple_dfr_cfgsel.ko"
rmdir -- "$install_root" 2>/dev/null || true
systemctl daemon-reload
udevadm control --reload
modprobe -a apple_ibridge apple_touchbar >/dev/null 2>&1 || true
if command -v limine-mkinitcpio >/dev/null 2>&1; then
  limine-mkinitcpio
fi

printf 'Persistent custom mode removed; firmware Touch Bar restored.\n'
printf 'The live sleep mode is unchanged until reboot; configure it explicitly if needed.\n'
