#!/usr/bin/env bash
# Small desktop-session actions used by the Omarchy Touch Bar layout.
set -euo pipefail

# dfr-touchd deliberately launches desktop actions with a minimal environment.
# Omarchy's CLI requires its packaged root even though it is already on PATH.
export OMARCHY_PATH=${OMARCHY_PATH:-/usr/share/omarchy}

log_action() {
  logger --tag omarchy-t1-touchbar -- "$*" 2>/dev/null || true
}

on_error() {
  local status=$?
  log_action "action=${1:-unknown} failed status=$status"
  exit "$status"
}

trap 'on_error "${1:-unknown}"' ERR

load_hyprland_session() {
  local instances
  instances=$(hyprctl instances -j)
  HYPRLAND_INSTANCE_SIGNATURE=$(jq -r 'sort_by(.time) | last | .instance // empty' <<<"$instances")
  WAYLAND_DISPLAY=$(jq -r 'sort_by(.time) | last | .wl_socket // empty' <<<"$instances")
  [[ -n $HYPRLAND_INSTANCE_SIGNATURE && -n $WAYLAND_DISPLAY ]] || return 1
  export HYPRLAND_INSTANCE_SIGNATURE WAYLAND_DISPLAY
}

chatgpt_address() {
  local active_json address
  active_json=$(hyprctl activewindow -j 2>/dev/null || true)
  address=$(jq -r \
    'select((.class | ascii_downcase) as $class |
      $class == "chatgpt" or $class == "codex" or
      $class == "com.openai.chatgpt") | .address // empty' \
    <<<"$active_json" 2>/dev/null || true)
  if [[ $address =~ ^0x[0-9a-fA-F]+$ ]]; then
    printf '%s\n' "$address"
    return 0
  fi
  address=$(hyprctl clients -j | jq -r \
    '.[] | select((.class | ascii_downcase) as $class |
      $class == "chatgpt" or $class == "codex" or
      $class == "com.openai.chatgpt") | .address' | head -n1)
  [[ $address =~ ^0x[0-9a-fA-F]+$ ]] || return 1
  printf '%s\n' "$address"
}

focus_chatgpt() {
  chatgpt_window=$(chatgpt_address 2>/dev/null || true)
  if [[ -z $chatgpt_window ]]; then
    setsid uwsm-app -- /usr/bin/chatgpt >/dev/null 2>&1 &
    for _ in {1..50}; do
      sleep 0.1
      chatgpt_window=$(chatgpt_address 2>/dev/null || true)
      [[ -n $chatgpt_window ]] && break
    done
  fi
  [[ -n $chatgpt_window ]] || return 1
  hyprctl dispatch "hl.dsp.focus({ window = \"address:$chatgpt_window\" })"
}

send_chatgpt_chord() {
  local mods=$1 key=$2
  case $mods in
    CTRL | 'CTRL ALT' | 'CTRL SHIFT') ;;
    *) return 2 ;;
  esac
  [[ $key =~ ^[A-Z0-9_]+$ ]] || return 2
  focus_chatgpt
  hyprctl dispatch "hl.dsp.send_key_state({ mods = \"$mods\", key = \"$key\", state = \"down\", window = \"address:$chatgpt_window\" })"
  sleep 0.05
  hyprctl dispatch "hl.dsp.send_key_state({ mods = \"$mods\", key = \"$key\", state = \"up\", window = \"address:$chatgpt_window\" })"
}

active_window_address() {
  local address
  address=$(hyprctl activewindow -j | jq -r '.address // empty')
  [[ $address =~ ^0x[0-9a-fA-F]+$ ]] || return 1
  printf '%s\n' "$address"
}

# Send one of the fixed LazyVim leader sequences to the exact window that was
# active when the button fired. No class, title, or touch-provided text enters
# the dispatcher expression.
send_active_sequence() {
  local address key
  address=$(active_window_address)
  for key in "$@"; do
    # Keep this list narrower than arbitrary xkbcommon names. In particular,
    # `ESC` is not a keysym; the valid case-insensitive name is `Escape`.
    case $key in
      ESCAPE | SPACE | COMMA | [A-Z0-9]) ;;
      *) return 2 ;;
    esac
    hyprctl dispatch "hl.dsp.send_key_state({ mods = \"\", key = \"$key\", state = \"down\", window = \"address:$address\" })"
    hyprctl dispatch "hl.dsp.send_key_state({ mods = \"\", key = \"$key\", state = \"up\", window = \"address:$address\" })"
    sleep 0.025
  done
}

recording_active() {
  pgrep -f '^gpu-screen-recorder' >/dev/null 2>&1
}

stop_recording() {
  omarchy screenrecord --stop-recording
}

# dfr-render polls '^gpu-screen-recorder' and /tmp/omarchy-screenrecord-filename
# for the live REC label; keep the process pattern in sync with the renderer.
record_toggle() {
  if recording_active; then
    stop_recording
  else
    omarchy screenrecord
  fi
}

record_fullscreen() {
  if recording_active; then
    stop_recording
  else
    omarchy screenrecord --fullscreen
  fi
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
    log_action "workspace-merge start source=$source_workspace target=$target_workspace"
    mapfile -t workspace_windows < <(
      hyprctl clients -j | jq -r --argjson source "$source_workspace" \
        '.[] | select(.workspace.id == $source and .pinned != true) | .address'
    )
    for window_address in "${workspace_windows[@]}"; do
      hyprctl dispatch "hl.dsp.window.move({ workspace = \"$target_workspace\", follow = false, window = \"address:$window_address\" })"
    done
    hyprctl dispatch "hl.dsp.focus({ workspace = \"$target_workspace\" })"
    log_action "workspace-merge complete source=$source_workspace target=$target_workspace windows=${#workspace_windows[@]}"
    ;;
  dictation-toggle) voxtype record toggle ;;
  gpt-open) focus_chatgpt ;;
  gpt-new) send_chatgpt_chord CTRL N ;;
  gpt-command) send_chatgpt_chord CTRL K ;;
  gpt-files) send_chatgpt_chord CTRL P ;;
  gpt-open-file) send_chatgpt_chord CTRL O ;;
  gpt-sidebar) send_chatgpt_chord CTRL B ;;
  gpt-bottom) send_chatgpt_chord CTRL J ;;
  gpt-terminal) send_chatgpt_chord CTRL GRAVE ;;
  gpt-review) send_chatgpt_chord 'CTRL ALT' B ;;
  gpt-web) send_chatgpt_chord CTRL T ;;
  gpt-voice) send_chatgpt_chord 'CTRL SHIFT' V ;;
  gpt-dictate) send_chatgpt_chord 'CTRL SHIFT' D ;;
  gpt-copy) send_chatgpt_chord CTRL C ;;
  gpt-paste) send_chatgpt_chord CTRL V ;;
  menu-root) omarchy menu toggle root ;;
  menu-hardware) omarchy menu toggle hardware ;;
  menu-system) omarchy menu toggle system ;;
  menu-capture) omarchy menu toggle capture ;;
  menu-clipboard) omarchy menu clipboard ;;
  menu-emoji) omarchy menu emoji ;;
  terminal) omarchy launch terminal ;;
  terminal-tmux) omarchy launch terminal-tmux ;;
  fullscreen) hyprctl dispatch 'hl.dsp.window.fullscreen({ mode = "fullscreen" })' ;;
  nvim-files) send_active_sequence ESCAPE SPACE F F ;;
  nvim-grep) send_active_sequence ESCAPE SPACE S G ;;
  nvim-tree) send_active_sequence ESCAPE SPACE E ;;
  nvim-git) send_active_sequence ESCAPE SPACE G G ;;
  nvim-format) send_active_sequence ESCAPE SPACE C F ;;
  nvim-diag) send_active_sequence ESCAPE SPACE S D ;;
  nvim-terminal) send_active_sequence ESCAPE SPACE F T ;;
  nvim-new) send_active_sequence ESCAPE SPACE F N ;;
  nvim-buffers) send_active_sequence ESCAPE SPACE COMMA ;;
  lock) omarchy system lock ;;
  shot-region) omarchy capture screenshot region ;;
  shot-window) omarchy capture screenshot windows ;;
  shot-full) omarchy capture screenshot fullscreen save ;;
  ocr) omarchy capture text ;;
  record-toggle) record_toggle ;;
  record-full) record_fullscreen ;;
  *)
    printf 'unknown Touch Bar action: %s\n' "${1:-}" >&2
    exit 2
    ;;
esac
