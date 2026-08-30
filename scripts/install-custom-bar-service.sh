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
lid_unit=t1-custom-bar-lid.service
sleep_policy=80-t1-touchbar-sleep.conf

fail() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 1
}

[[ $(id -u) -eq 0 ]] || fail "run with sudo: sudo $0"
[[ $(</sys/class/dmi/id/product_name) == MacBookPro13,2 ]] ||
  fail 'persistent custom mode is guarded for MacBookPro13,2'
grep -qw s2idle /sys/power/mem_sleep ||
  fail 'this kernel does not expose s2idle in /sys/power/mem_sleep'
rg -q '\.resume = appletb_reset_resume' /usr/src/apple-ib-drv-0.1/apple-touchbar.c 2>/dev/null ||
  fail 'patched firmware resume is missing; run ./scripts/install-driver-omarchy.sh first'

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
  "$repo_root/scripts/t1-lid-watcher" \
  "$repo_root/scripts/t1-custom-bar-recover" \
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
  "$repo_root/config/systemd/$lid_unit" \
  /etc/systemd/system/
install -m0644 "$repo_root/config/brcmfmac.modules-load.conf" \
  /etc/modules-load.d/brcmfmac.conf
install -m0644 "$repo_root/config/86-mbp132-thunderbolt-d3cold.rules" \
  /etc/udev/rules.d/86-mbp132-thunderbolt-d3cold.rules
install -m0755 "$repo_root/scripts/mbp132-thunderbolt-sleep" \
  /usr/lib/systemd/system-sleep/mbp132-thunderbolt-sleep
install -m0755 "$repo_root/scripts/t1-custom-bar-sleep" \
  /usr/lib/systemd/system-sleep/t1-custom-bar-sleep
install -d -m0755 /etc/limine-entry-tool.d
install -m0644 "$repo_root/config/limine/20-macbookpro13-t1-suspend.conf" \
  /etc/limine-entry-tool.d/20-macbookpro13-t1-suspend.conf
install -d -m0755 /etc/systemd/sleep.conf.d
install -m0644 "$repo_root/config/systemd/$sleep_policy" \
  "/etc/systemd/sleep.conf.d/$sleep_policy"
printf s2idle > /sys/power/mem_sleep

systemctl disable --now t1-touchbar-boot.service >/dev/null 2>&1 || true
systemctl daemon-reload
udevadm control --reload
udevadm trigger --subsystem-match=pci
systemctl enable --now "$unit"
systemctl enable "$resume_unit"
systemctl enable --now "$lid_unit"

if command -v limine-mkinitcpio >/dev/null 2>&1; then
  limine-mkinitcpio
fi

printf 'Persistent custom Touch Bar installed and started.\n'
printf 'Service: %s (enabled at every boot)\n' "$unit"
printf 'Sleep: s2idle (deep S3 cannot restore the T1 display)\n'
printf 'Resume: firmware mode before sleep, custom mode after wake\n'
printf 'Lid: %s (pre-sleep safety net and reopen fallback)\n' "$lid_unit"
printf 'Wi-Fi: brcmfmac forced during early boot\n'
printf 'PCIe: guarded Alpine Ridge pre-remove/rescan hook installed\n'
printf 'Rollback: sudo %s/uninstall-custom-bar-service.sh\n' "$repo_root/scripts"
