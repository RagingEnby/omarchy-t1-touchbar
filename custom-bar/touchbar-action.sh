#!/usr/bin/env bash
# Small desktop-session actions used by the Omarchy Touch Bar layout.
set -euo pipefail

# dfr-touchd deliberately launches desktop actions with a minimal environment.
# Omarchy's CLI requires its packaged root even though it is already on PATH.
export OMARCHY_PATH=${OMARCHY_PATH:-/usr/share/omarchy}

load_hyprland_session() {
  local instances
  instances=$(hyprctl instances -j)
  HYPRLAND_INSTANCE_SIGNATURE=$(jq -r 'sort_by(.time) | last | .instance // empty' <<<"$instances")
  WAYLAND_DISPLAY=$(jq -r 'sort_by(.time) | last | .wl_socket // empty' <<<"$instances")
  [[ -n $HYPRLAND_INSTANCE_SIGNATURE && -n $WAYLAND_DISPLAY ]] || return 1
  export HYPRLAND_INSTANCE_SIGNATURE WAYLAND_DISPLAY
}

chatgpt_address() {
  hyprctl clients -j | jq -r \
    '.[] | select((.class | ascii_downcase) == "chatgpt") | .address' | head -n1
}

focus_chatgpt() {
  chatgpt_window=$(chatgpt_address)
  if [[ -z $chatgpt_window ]]; then
    setsid uwsm-app -- /usr/bin/chatgpt >/dev/null 2>&1 &
    for _ in {1..50}; do
      sleep 0.1
      chatgpt_window=$(chatgpt_address)
      [[ -n $chatgpt_window ]] && break
    done
  fi
  [[ -n $chatgpt_window ]] || return 1
  hyprctl dispatch "hl.dsp.focus({ window = \"address:$chatgpt_window\" })"
}

send_chatgpt_key() {
  local key=$1
  focus_chatgpt
  hyprctl dispatch "hl.dsp.send_key_state({ mods = \"CTRL\", key = \"$key\", state = \"down\", window = \"address:$chatgpt_window\" })"
  sleep 0.05
  hyprctl dispatch "hl.dsp.send_key_state({ mods = \"CTRL\", key = \"$key\", state = \"up\", window = \"address:$chatgpt_window\" })"
}

load_hyprland_session

case ${1:-} in
  workspace)
    [[ ${2:-} =~ ^([1-9]|10)$ ]] || exit 2
    hyprctl dispatch "hl.dsp.focus({ workspace = \"$2\" })"
    ;;
  workspace-merge)
    [[ ${2:-} =~ ^([1-9]|10)$ && ${3:-} =~ ^([1-9]|10)$ ]] || exit 2
    source_workspace=$2
    target_workspace=$3
    [[ $source_workspace != "$target_workspace" ]] || exit 0
    mapfile -t workspace_windows < <(
      hyprctl clients -j | jq -r --argjson source "$source_workspace" \
        '.[] | select(.workspace.id == $source and .pinned != true) | .address'
    )
    for window_address in "${workspace_windows[@]}"; do
      hyprctl dispatch "hl.dsp.window.move({ workspace = \"$target_workspace\", follow = false, window = \"address:$window_address\" })"
    done
    hyprctl dispatch "hl.dsp.focus({ workspace = \"$target_workspace\" })"
    ;;
  dictation-toggle) voxtype record toggle ;;
  gpt-open) focus_chatgpt ;;
  gpt-new) send_chatgpt_key N ;;
  gpt-command) send_chatgpt_key K ;;
  gpt-copy) send_chatgpt_key C ;;
  gpt-paste) send_chatgpt_key V ;;
  menu-root) omarchy menu toggle root ;;
  menu-hardware) omarchy menu toggle hardware ;;
  menu-system) omarchy menu toggle system ;;
  menu-capture) omarchy menu toggle capture ;;
  menu-clipboard) omarchy menu clipboard ;;
  menu-emoji) omarchy menu emoji ;;
  terminal) omarchy launch terminal ;;
  lock) omarchy system lock ;;
  *)
    printf 'unknown Touch Bar action: %s\n' "${1:-}" >&2
    exit 2
    ;;
esac
