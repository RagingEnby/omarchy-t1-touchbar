#!/usr/bin/env bash
# Install the already-built custom bar as a persistent systemd service.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work_root=${T1_CUSTOM_WORKDIR:-"$repo_root/.t1-work/custom-bar"}
source_root="$work_root/macbook-t1-linux"
dfrd_root="$source_root/touch-bar/dfrd"
kmod_root="$source_root/touch-bar/kernel/t1-touchbar-display"
install_root=/usr/local/lib/omarchy-t1-touchbar
unit=t1-custom-bar-live.service
resume_unit=t1-custom-bar-resume.service

fail() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 1
}

[[ $(id -u) -eq 0 ]] || fail "run with sudo: sudo $0"
[[ $(</sys/class/dmi/id/product_name) == MacBookPro13,2 ]] ||
  fail 'persistent custom mode is guarded for MacBookPro13,2'

for required in \
  "$dfrd_root/dfr-render" "$dfrd_root/dfr-touchd" "$dfrd_root/dfr-fnd" \
  "$dfrd_root/dfrd-run.sh" "$kmod_root/appletbdrm.ko" \
  "$kmod_root/apple_dfr_cfgsel.ko"; do
  [[ -e $required ]] || fail 'run ./scripts/build-custom-bar.sh first'
done

module_vermagic=$(modinfo -F vermagic "$kmod_root/appletbdrm.ko")
[[ $module_vermagic == "$(uname -r) "* ]] ||
  fail "kernel module was built for '$module_vermagic', not '$(uname -r)'; rebuild first"

install -d -m0755 "$install_root"
install -m0755 \
  "$dfrd_root/dfr-render" \
  "$dfrd_root/dfr-touchd" \
  "$dfrd_root/dfr-fnd" \
  "$dfrd_root/dfrd-run.sh" \
  "$repo_root/scripts/custom-bar-service-runner" \
  "$install_root/"
install -m0755 "$repo_root/custom-bar/touchbar-action.sh" \
  "$install_root/touchbar-action"
install -m0644 \
  "$kmod_root/appletbdrm.ko" \
  "$kmod_root/apple_dfr_cfgsel.ko" \
  "$install_root/"
printf '%s\n' "$(uname -r)" > "$install_root/kernel-release"
install -m0644 "$dfrd_root/99-touchbar-dfr.rules" \
  /etc/udev/rules.d/99-touchbar-dfr.rules
install -m0644 \
  "$repo_root/config/systemd/$unit" \
  "$repo_root/config/systemd/$resume_unit" \
  /etc/systemd/system/

systemctl disable --now t1-touchbar-boot.service >/dev/null 2>&1 || true
systemctl daemon-reload
udevadm control --reload
systemctl enable --now "$unit"
systemctl enable "$resume_unit"

printf 'Persistent custom Touch Bar installed and started.\n'
printf 'Service: %s (enabled at every boot)\n' "$unit"
printf 'Resume: %s (relights after suspend/hibernate)\n' "$resume_unit"
printf 'Rollback: sudo %s/uninstall-custom-bar-service.sh\n' "$repo_root/scripts"
