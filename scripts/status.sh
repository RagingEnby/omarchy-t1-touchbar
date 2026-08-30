#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# Read-only and safe to paste into a public issue.
set -u

model=unknown
[[ -r /sys/class/dmi/id/product_name ]] && model=$(</sys/class/dmi/id/product_name)
printf 'Model: %s\n' "$model"
printf 'Kernel: %s\n' "$(uname -r)"
if [[ -r /sys/power/mem_sleep ]]; then
  mem_sleep=$(</sys/power/mem_sleep)
  printf 'Sleep mode: %s\n' "$mem_sleep"
  if [[ $mem_sleep == *'[deep]'* ]]; then
    printf 'Sleep warning: deep S3 leaves the T1 display black until reboot; select s2idle\n'
  fi
fi
if grep -qw pcie_port_pm=off /proc/cmdline; then
  printf 'Alpine Ridge boot guard: active (pcie_port_pm=off)\n'
elif [[ -r /etc/limine-entry-tool.d/20-macbookpro13-t1-suspend.conf ]] &&
     grep -qw pcie_port_pm=off /etc/limine-entry-tool.d/20-macbookpro13-t1-suspend.conf; then
  printf 'Alpine Ridge boot guard: staged; reboot required to activate\n'
else
  printf 'Alpine Ridge boot guard: missing\n'
fi
if command -v omarchy >/dev/null 2>&1; then
  printf 'Omarchy: %s\n' "$(omarchy version 2>/dev/null || printf unknown)"
fi

has_8600=0
has_1281=0
ibridge_config=unknown
lsusb -d 05ac:8600 >/dev/null 2>&1 && has_8600=1
lsusb -d 05ac:1281 >/dev/null 2>&1 && has_1281=1

if (( has_8600 == 1 )); then
  printf 'T1 USB state: production iBridge (05ac:8600)\n'
elif (( has_1281 == 1 )); then
  printf 'T1 USB state: recovery (05ac:1281)\n'
else
  printf 'T1 USB state: not detected\n'
fi

if (( has_8600 == 1 )); then
  for node in /sys/bus/usb/devices/*; do
    [[ -r $node/idVendor && -r $node/idProduct ]] || continue
    [[ $(<"$node/idVendor") == 05ac && $(<"$node/idProduct") == 8600 ]] || continue
    config=unknown
    power=unknown
    [[ -r $node/bConfigurationValue ]] && config=$(<"$node/bConfigurationValue")
    ibridge_config=$config
    [[ -r $node/power/control ]] && power=$(<"$node/power/control")
    printf 'iBridge USB: configuration=%s power/control=%s\n' "$config" "$power"
  done
fi

if command -v dkms >/dev/null 2>&1; then
  dkms_line=$(dkms status 2>/dev/null | awk '/apple-ib-drv/{print; exit}')
  printf 'DKMS: %s\n' "${dkms_line:-apple-ib-drv not installed}"
fi

for module in apple_ibridge apple_touchbar; do
  if lsmod 2>/dev/null | awk -v wanted="$module" '$1 == wanted { found=1 } END { exit !found }'; then
    printf 'Module %s: loaded\n' "$module"
  else
    printf 'Module %s: not loaded\n' "$module"
  fi
done

custom_cfgsel=0
custom_drm=0
custom_service=0
custom_input=0
if lsmod 2>/dev/null | awk '$1 == "apple_dfr_cfgsel" { found=1 } END { exit !found }'; then
  custom_cfgsel=1
fi
if lsmod 2>/dev/null | awk '$1 == "appletbdrm" { found=1 } END { exit !found }'; then
  custom_drm=1
fi
if systemctl is-active --quiet t1-custom-bar-live.service 2>/dev/null ||
   systemctl is-active --quiet dfrd.service 2>/dev/null; then
  custom_service=1
fi
if rg -q 'T1 Touch Bar \(dfrd\)' /proc/bus/input/devices 2>/dev/null; then
  custom_input=1
fi
if [[ $ibridge_config == 2 ]]; then
  printf 'Custom display: cfgsel=%s DRM=%s service=%s input=%s\n' \
    "$custom_cfgsel" "$custom_drm" "$custom_service" "$custom_input"
  if systemctl is-active --quiet t1-custom-bar-lid.service 2>/dev/null; then
    printf 'Lid recovery: active\n'
  else
    printf 'Lid recovery: inactive\n'
  fi
fi

if [[ -r /etc/modules-load.d/brcmfmac.conf ]] &&
   rg -q '^[[:space:]]*brcmfmac[[:space:]]*$' /etc/modules-load.d/brcmfmac.conf; then
  printf 'Wi-Fi early load: configured\n'
fi

physical=0
virtual=0
for hid in /sys/bus/hid/devices/*; do
  [[ -e $hid ]] || continue
  name=${hid##*/}
  driver=unbound
  [[ -L $hid/driver ]] && driver=$(basename "$(readlink -f "$hid/driver")")
  if [[ $name == *05AC:8600* && $driver == apple-ibridge-hid ]]; then
    ((physical += 1))
  fi
  if [[ $name == *1D6B:0301* && $driver == apple-touchbar ]]; then
    ((virtual += 1))
  fi
done
printf 'HID topology: physical-iBridge=%d virtual-Touch-Bar=%d\n' "$physical" "$virtual"

if rg -qi 'Touch Bar|iBridge Virtual HID' /proc/bus/input/devices 2>/dev/null; then
  printf 'Linux input: Touch Bar registered\n'
else
  printf 'Linux input: Touch Bar not registered\n'
fi

modprobe_file=/etc/modprobe.d/apple-touchbar.conf
if [[ -r $modprobe_file ]] && rg -q 'skip_acpi_power=1' "$modprobe_file"; then
  printf 'Freeze guard: configured (skip_acpi_power=1)\n'
else
  printf 'Freeze guard: missing or unreadable\n'
fi

if [[ -r /etc/udev/rules.d/39-usbmuxd.rules ]]; then
  if rg -q '^[[:space:]]*SUBSYSTEM=.*8600' /etc/udev/rules.d/39-usbmuxd.rules; then
    printf 'usbmuxd local override: WARNING, still matches 05ac:8600\n'
  else
    printf 'usbmuxd local override: production iBridge excluded\n'
  fi
else
  printf 'usbmuxd local override: absent\n'
fi

efi_root=
for candidate in /boot /boot/efi; do
  if [[ -d $candidate/EFI/APPLE/EMBEDDEDOS ]]; then
    efi_root=$candidate/EFI/APPLE/EMBEDDEDOS
    break
  fi
done
if [[ -n $efi_root ]]; then
  printf 'EFI EmbeddedOS: directory present\n'
  for name in combined.memboot FDRData version.plist; do
    if [[ -f $efi_root/$name ]]; then
      size=$(stat -c %s "$efi_root/$name" 2>/dev/null || printf unknown)
      printf '  %s: present (%s bytes)\n' "$name" "$size"
    else
      printf '  %s: missing\n' "$name"
    fi
  done
else
  if findmnt -rn -T /boot -o FSTYPE 2>/dev/null | rg -q '^(vfat|fat|msdos)$' &&
      [[ ! -x /boot ]]; then
    printf 'EFI EmbeddedOS: ESP is mounted but root-protected; rerun status with sudo to inspect filenames\n'
  else
    printf 'EFI EmbeddedOS: directory not found at /boot or /boot/efi\n'
  fi
fi

if (( has_8600 == 1 && custom_cfgsel == 1 && custom_drm == 1 && custom_service == 1 && custom_input == 1 )) &&
   [[ $ibridge_config == 2 ]]; then
  printf 'Overall: healthy live programmable Touch Bar topology\n'
elif (( has_8600 == 1 && physical >= 2 && virtual >= 1 )); then
  printf 'Overall: healthy live firmware Touch Bar topology\n'
elif (( has_1281 == 1 )); then
  printf 'Overall: T1 recovery; driver alone cannot light the bar\n'
else
  printf 'Overall: incomplete; follow docs/OMARCHY.md without resetting blindly\n'
fi
