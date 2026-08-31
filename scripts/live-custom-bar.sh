#!/usr/bin/env bash
# Session-only custom T1 Touch Bar smoke test with a config-1 rollback.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
work_root=${T1_CUSTOM_WORKDIR:-"$repo_root/.t1-work/custom-bar"}
source_root="$work_root/macbook-t1-linux"
dfrd_root="$source_root/touch-bar/dfrd"
kmod_root="$source_root/touch-bar/kernel/t1-touchbar-display"
state_root=/run/t1-custom-bar
runtime_rule=/run/udev/rules.d/99-touchbar-dfr-live.rules
unit=t1-custom-bar-live.service

fail() {
  printf 'ERROR: %s\n' "$*" >&2
  exit 1
}

usb_path() {
  local path
  for path in /sys/bus/usb/devices/*; do
    [[ -r "$path/idVendor" && -r "$path/idProduct" ]] || continue
    [[ $(<"$path/idVendor") == 05ac && $(<"$path/idProduct") == 8600 ]] || continue
    printf '%s\n' "$path"
    return 0
  done
  return 1
}

wait_for_config() {
  local expected=$1 path n
  for n in {1..40}; do
    path=$(usb_path 2>/dev/null || true)
    if [[ -n $path && -r "$path/bConfigurationValue" ]] &&
       [[ $(<"$path/bConfigurationValue") == "$expected" ]]; then
      return 0
    fi
    sleep 0.25
  done
  return 1
}

restore_firmware_bar() {
  local hid_path hid driver path config
  systemctl stop "$unit" >/dev/null 2>&1 || true
  rm -f -- \
    "$state_root/layout.committed" \
    "$state_root/layout.pending" \
    "$state_root/contextd.pid" \
    "$state_root/contextd.pid.tmp"

  # These are loaded directly from the build tree with insmod, so use rmmod;
  # modprobe -r may not resolve an out-of-tree module absent from modules.dep.
  rmmod apple_dfr_cfgsel >/dev/null 2>&1 || true
  rmmod appletbdrm >/dev/null 2>&1 || true

  # Deregistering the configuration selector can leave this kernel's USB
  # authorization state at zero. If config 1 does not return immediately,
  # cycle only the verified hardwired internal USB port to re-enumerate the
  # already-running 05ac:8600 production firmware.
  path=$(usb_path 2>/dev/null || true)
  config=''
  [[ -n $path && -r "$path/bConfigurationValue" ]] &&
    config=$(<"$path/bConfigurationValue")
  if [[ $config != 1 ]]; then
    local port=/sys/bus/usb/devices/usb1/1-0:1.0/usb1-port3
    if [[ -r $port/connect_type && $(<"$port/connect_type") == hardwired ]]; then
      printf '1\n' > "$port/disable"
      sleep 1
      printf '0\n' > "$port/disable"
    fi
  fi

  wait_for_config 1 || true
  modprobe -a apple_ibridge apple_touchbar >/dev/null 2>&1 || true

  # hid-sensor-hub can win one physical iBridge interface during the config-1
  # reprobe. Give every 05ac:8600 physical HID back to apple-ibridge-hid so
  # the complete firmware topology is restored, not merely the visible row.
  for hid_path in /sys/bus/hid/devices/0003:05AC:8600.*; do
    [[ -e $hid_path ]] || continue
    driver=$(basename "$(readlink -f "$hid_path/driver" 2>/dev/null)" 2>/dev/null || true)
    [[ $driver == hid-sensor-hub ]] || continue
    hid=${hid_path##*/}
    printf '%s\n' "$hid" > /sys/bus/hid/drivers/hid-sensor-hub/unbind
    printf '%s\n' "$hid" > /sys/bus/hid/drivers/apple-ibridge-hid/bind
  done

  if [[ -e $runtime_rule ]]; then
    unlink "$runtime_rule"
    udevadm control --reload >/dev/null 2>&1 || true
  fi
}

start_live() {
  [[ $(id -u) -eq 0 ]] || fail "run with sudo: sudo $0 start"
  [[ $(</sys/class/dmi/id/product_name) == MacBookPro13,2 ]] ||
    fail 'live custom mode is guarded for MacBookPro13,2'
  [[ -d $source_root/.git ]] ||
    fail 'run ./scripts/build-custom-bar.sh first'
  [[ $(git -C "$source_root" rev-parse HEAD) == 982d97a61a0c834d03e7cf7af5203b11ad7bae44 ]] ||
    fail 'unexpected custom-display source revision'

  local required
  for required in \
    "$dfrd_root/dfr-render" "$dfrd_root/dfr-touchd" "$dfrd_root/dfr-fnd" \
    "$dfrd_root/dfr-contextd" \
    "$dfrd_root/dfrd-run.sh" "$kmod_root/appletbdrm.ko" \
    "$kmod_root/apple_dfr_cfgsel.ko"; do
    [[ -e $required ]] || fail "missing build artifact: $required"
  done

  grep -q 'slot = dev_id - appleib_sub_hid_ids' \
    /usr/src/apple-ib-drv-0.1/apple-ibridge.c ||
    fail 'installed apple_ibridge lacks the safe sub-device slot fix'

  local path
  path=$(usb_path) || fail 'production T1 05ac:8600 not found'
  [[ $(<"$path/bConfigurationValue") == 1 ]] ||
    fail 'T1 is not in the known-good firmware configuration 1'
  systemctl is-active --quiet "$unit" && fail 'custom bar is already active'

  mkdir -p "$state_root" /run/udev/rules.d
  rm -f -- \
    "$state_root/layout.committed" \
    "$state_root/layout.pending" \
    "$state_root/contextd.pid" \
    "$state_root/contextd.pid.tmp"
  install -m0755 "$dfrd_root/dfr-render" "$state_root/dfr-render"
  install -m0755 "$dfrd_root/dfr-touchd" "$state_root/dfr-touchd"
  install -m0755 "$dfrd_root/dfr-fnd" "$state_root/dfr-fnd"
  install -m0755 "$dfrd_root/dfr-contextd" "$state_root/dfr-contextd"
  install -m0755 "$dfrd_root/dfrd-run.sh" "$state_root/dfrd-run.sh"
  install -m0755 "$repo_root/custom-bar/touchbar-action.sh" "$state_root/touchbar-action"
  install -m0644 "$dfrd_root/99-touchbar-dfr.rules" "$runtime_rule"
  udevadm control --reload

  trap 'printf "Live test failed; restoring firmware bar...\n" >&2; restore_firmware_bar' ERR

  modprobe -r apple_touchbar
  modprobe -r apple_ibridge
  insmod "$kmod_root/appletbdrm.ko"
  insmod "$kmod_root/apple_dfr_cfgsel.ko"
  wait_for_config 2
  path=$(usb_path) || fail 'T1 disappeared after configuration-2 switch'
  printf 'on\n' > "$path/power/control"

  local started_at
  started_at=$(date --iso-8601=seconds)
  systemd-run --quiet --unit "$unit" --collect --property=Type=simple \
    /usr/bin/bash -c \
    'set -uo pipefail
context_pid=
dfrd_pid=
cleanup() {
  local child
  trap - INT TERM
  for child in "$dfrd_pid" "$context_pid"; do
    [[ $child =~ ^[1-9][0-9]*$ ]] && kill "$child" 2>/dev/null || true
  done
  for child in "$dfrd_pid" "$context_pid"; do
    [[ $child =~ ^[1-9][0-9]*$ ]] && wait "$child" 2>/dev/null || true
  done
  rm -f -- /run/t1-custom-bar/contextd.pid /run/t1-custom-bar/contextd.pid.tmp
}
trap "cleanup; exit 0" INT TERM
"/run/t1-custom-bar/dfr-contextd" &
context_pid=$!
printf "%s\n" "$context_pid" > /run/t1-custom-bar/contextd.pid.tmp
mv -f -- /run/t1-custom-bar/contextd.pid.tmp /run/t1-custom-bar/contextd.pid
"/run/t1-custom-bar/dfrd-run.sh" media &
dfrd_pid=$!
wait -n "$context_pid" "$dfrd_pid" 2>/dev/null || true
cleanup
exit 1'

  for _ in {1..40}; do
    systemctl is-active --quiet "$unit" && break
    sleep 0.25
  done
  systemctl is-active --quiet "$unit"

  local card driver found=0
  for card in /sys/class/drm/card[0-9]*; do
    driver=$(basename "$(readlink -f "$card/device/driver" 2>/dev/null)" 2>/dev/null || true)
    if [[ $driver == appletbdrm ]]; then
      found=1
      break
    fi
  done
  [[ $found -eq 1 ]] || fail 'configuration 2 is active but appletbdrm has no DRM card'

  local ready=0 context_pid context_exe dfrd_line
  local render_pid render_exe touch_pid touch_exe fnd_pid fnd_exe
  for _ in {1..40}; do
    systemctl is-active --quiet "$unit"
    context_pid=''
    context_exe=''
    render_pid=''
    render_exe=''
    touch_pid=''
    touch_exe=''
    fnd_pid=''
    fnd_exe=''
    [[ -r $state_root/contextd.pid ]] && read -r context_pid < "$state_root/contextd.pid"
    if [[ $context_pid =~ ^[1-9][0-9]*$ ]] && kill -0 "$context_pid" 2>/dev/null; then
      context_exe=$(readlink -f "/proc/$context_pid/exe" 2>/dev/null || true)
    fi
    dfrd_line=$(journalctl -u "$unit" --since "$started_at" --no-pager -o cat |
      sed -n '/dfrd: up\./p' | tail -n1)
    render_pid=$(sed -n 's/.*render=\([0-9][0-9]*\).*/\1/p' <<<"$dfrd_line")
    touch_pid=$(sed -n 's/.*touchd=\([0-9][0-9]*\).*/\1/p' <<<"$dfrd_line")
    fnd_pid=$(sed -n 's/.*fnd=\([0-9][0-9]*\).*/\1/p' <<<"$dfrd_line")
    if [[ $render_pid =~ ^[1-9][0-9]*$ ]] && kill -0 "$render_pid" 2>/dev/null; then
      render_exe=$(readlink -f "/proc/$render_pid/exe" 2>/dev/null || true)
    fi
    if [[ $touch_pid =~ ^[1-9][0-9]*$ ]] && kill -0 "$touch_pid" 2>/dev/null; then
      touch_exe=$(readlink -f "/proc/$touch_pid/exe" 2>/dev/null || true)
    fi
    if [[ $fnd_pid =~ ^[1-9][0-9]*$ ]] && kill -0 "$fnd_pid" 2>/dev/null; then
      fnd_exe=$(readlink -f "/proc/$fnd_pid/exe" 2>/dev/null || true)
    fi
    if [[ $context_exe == "$state_root/dfr-contextd" ]] &&
       [[ $render_exe == "$state_root/dfr-render" ]] &&
       [[ $touch_exe == "$state_root/dfr-touchd" ]] &&
       [[ $fnd_exe == "$state_root/dfr-fnd" ]] &&
       [[ -s $state_root/layout.committed ]] &&
       [[ ! -e $state_root/layout.pending ]]; then
      ready=1
      break
    fi
    sleep 0.25
  done
  [[ $ready -eq 1 ]] || fail 'renderer, touch, Fn and context daemons did not reach their ready state'

  trap - ERR
  printf 'Custom Touch Bar live test: ACTIVE\n'
  printf 'USB configuration: 2\n'
  printf 'DRM card: %s\n' "${card##*/}"
  printf 'Rollback: sudo %s stop\n' "$0"
  printf 'No boot persistence or reboot was configured.\n'
}

stop_live() {
  [[ $(id -u) -eq 0 ]] || fail "run with sudo: sudo $0 stop"
  restore_firmware_bar
  wait_for_config 1 || fail 'T1 did not return to firmware configuration 1'
  printf 'Custom Touch Bar live test: STOPPED\n'
  printf 'Firmware Touch Bar restored in USB configuration 1.\n'
}

show_status() {
  local path config=missing service=inactive drm=missing card driver
  path=$(usb_path 2>/dev/null || true)
  [[ -n $path ]] && config=$(<"$path/bConfigurationValue")
  systemctl is-active --quiet "$unit" 2>/dev/null && service=active
  for card in /sys/class/drm/card[0-9]*; do
    driver=$(basename "$(readlink -f "$card/device/driver" 2>/dev/null)" 2>/dev/null || true)
    [[ $driver == appletbdrm ]] && drm=${card##*/}
  done
  printf 'USB configuration: %s\n' "$config"
  printf 'Custom service: %s\n' "$service"
  printf 'Touch Bar DRM: %s\n' "$drm"
}

case ${1:-status} in
  start) start_live ;;
  stop) stop_live ;;
  status) show_status ;;
  *) fail 'usage: live-custom-bar.sh {start|stop|status}' ;;
esac
