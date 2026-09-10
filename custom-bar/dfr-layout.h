/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Omarchy layout overlay for xeeban/macbook-t1-linux dfrd.
 *
 * Derived from dfrd/dfr-layout.h at commit
 * 982d97a61a0c834d03e7cf7af5203b11ad7bae44. The renderer and touch daemon
 * compile this same file, so the painted zones and touch targets stay aligned.
 *
 * dfr-fnd expects the first five layout names below. We retain those internal
 * names while assigning Omarchy-friendly roles, then add focus-driven decks:
 *   media = home row (default/manual workspace deck)
 *   fn    = Escape and F1-F12
 *   ctrl  = live system row
 *   alt   = media/brightness row
 *   meta  = AI/productivity row
 *   browser, terminal, code, nvim, files, comms, document, reader, music,
 *   universal, capture
 *          = control surfaces
 *
 * App decks are opt-in: dfr-contextd only records which deck matches the
 * focused application, and the APPS button on the home row exposes that deck
 * on demand (tapping APPS again returns Home). The daemon never forces a
 * deck change by itself.
 */
#ifndef DFR_LAYOUT_H
#define DFR_LAYOUT_H

#include <linux/input-event-codes.h>

enum dfr_action {
	DFR_ACT_KEY = 0,
	DFR_ACT_CMD = 1,
	DFR_ACT_LAYER = 2,
	DFR_ACT_HOTKEY = 3,
	DFR_ACT_APPDECK = 4,
};

enum dfr_mod {
	DFR_MOD_NONE  = 0,
	DFR_MOD_CTRL  = 1u << 0,
	DFR_MOD_ALT   = 1u << 1,
	DFR_MOD_SHIFT = 1u << 2,
	DFR_MOD_META  = 1u << 3,
};

enum dfr_indicator {
	DFR_IND_NONE     = 0,
	DFR_IND_BATTERY  = 1,
	DFR_IND_WIFI     = 2,
	DFR_IND_BT       = 3,
	DFR_IND_KBDLIGHT = 4,
	DFR_IND_WORKSPACE_1 = 5,
	DFR_IND_WORKSPACE_2 = 6,
	DFR_IND_WORKSPACE_3 = 7,
	DFR_IND_WORKSPACE_4 = 8,
	DFR_IND_WORKSPACE_5 = 9,
	DFR_IND_WORKSPACE_6 = 10,
	DFR_IND_WORKSPACE_7 = 11,
	DFR_IND_WORKSPACE_8 = 12,
	DFR_IND_WORKSPACE_9 = 13,
	DFR_IND_WORKSPACE_10 = 14,
	DFR_IND_COUNT = 15,
};

struct dfr_key {
	const char *label;
	int keycode;
	float weight;
	int action;
	const char *cmd;
	int indicator;
	unsigned color;
	unsigned mods;
};

struct dfr_layout {
	const char *name;
	const struct dfr_key *keys;
	int nkeys;
};

#define DFR_GLYPH_PREV      "\U000F04AE"
#define DFR_GLYPH_PLAYPAUSE "\U000F040E"
#define DFR_GLYPH_NEXT      "\U000F04AD"
#define DFR_GLYPH_MUTE      "\U000F075F"
#define DFR_GLYPH_VOLDOWN   "\U000F075E"
#define DFR_GLYPH_VOLUP     "\U000F075D"
#define DFR_GLYPH_BRDOWN    "\U000F00DE"
#define DFR_GLYPH_BRUP      "\U000F00E0"
#define DFR_GLYPH_MIC       "\U000F036C"
#define DFR_GLYPH_HOME      "\U000F02DC"
#define DFR_GLYPH_WEB       "\U000F059F"
#define DFR_GLYPH_TERM      "\U000F018D"
#define DFR_GLYPH_CODE      "\U000F0174"
#define DFR_GLYPH_FOLDER    "\U000F024B"
#define DFR_GLYPH_CHAT      "\U000F0362"
#define DFR_GLYPH_DOC       "\U000F0219"
#define DFR_GLYPH_READ      "\U000F00BA"
#define DFR_GLYPH_MUSIC     "\U000F075A"
#define DFR_GLYPH_APP       "\U000F05C6"
#define DFR_GLYPH_BACK      "\U000F004D"
#define DFR_GLYPH_FORWARD   "\U000F0054"
#define DFR_GLYPH_RELOAD    "\U000F0450"
#define DFR_GLYPH_SEARCH    "\U000F0349"
#define DFR_GLYPH_COPY      "\U000F018F"
#define DFR_GLYPH_PASTE     "\U000F0192"
#define DFR_GLYPH_SAVE      "\U000F0193"
#define DFR_GLYPH_UNDO      "\U000F054C"
#define DFR_GLYPH_REDO      "\U000F044E"
#define DFR_GLYPH_PLUS      "\U000F0415"
#define DFR_GLYPH_CLOSE     "\U000F0156"
#define DFR_GLYPH_UP        "\U000F005D"
#define DFR_GLYPH_TRASH     "\U000F01B4"
#define DFR_GLYPH_FULLSCREEN "\U000F0293"
#define DFR_GLYPH_GRID      "\U000F003B"
#define DFR_GLYPH_CAMERA    "\U000F0100"
#define DFR_GLYPH_WINDOW    "\U000F10AC"
#define DFR_GLYPH_RECORD    "\U000F044A"
#define DFR_GLYPH_STOP      "\U000F04DB"
#define DFR_GLYPH_OCR       "\U000F113A"
#define DFR_GLYPH_MONITOR   "\U000F0379"
#define DFR_GLYPH_ROBOT     "\U000F06A9"
#define DFR_GLYPH_VIEWGRID  "\U000F0570"

/* Tokyo Night accents from the active Omarchy theme. The renderer uses these
 * sparingly for glyphs and rails over its neutral Prism surfaces. */
#define DFR_COL_TEXT      0xFFC0CAF5u
#define DFR_COL_ESC       0xFFF7768Eu
#define DFR_COL_WORKSPACE 0xFF0DB9D7u
#define DFR_COL_CHATGPT   0xFF9ECE6Au
#define DFR_COL_OMARCHY   0xFFE0AF68u
#define DFR_COL_VOICE     0xFFBB9AF7u
#define DFR_COL_ACTION    0xFFAD8EE6u
#define DFR_COL_STATUS    0xFF449DABu
#define DFR_COL_BRIGHT    0xFFE0AF68u
#define DFR_COL_SKIP      0xFF7AA2F7u
#define DFR_COL_PLAY      0xFF9ECE6Au
#define DFR_COL_MUTE      0xFFF7768Eu
#define DFR_COL_VOL       0xFF7AA2F7u
#define DFR_COL_SHOT      0xFF73DACAu
#define DFR_COL_REC       0xFFF7768Eu
#define DFR_COL_CODE2     0xFF7DCFFFu

#define DFR_HOTKEY(_label, _key, _weight, _mods, _color) \
	{ (_label), (_key), (_weight), DFR_ACT_HOTKEY, 0, DFR_IND_NONE, (_color), (_mods) }
#define DFR_LAYER(_label, _weight, _layer, _color) \
	{ (_label), 0, (_weight), DFR_ACT_LAYER, (_layer), DFR_IND_NONE, (_color), DFR_MOD_NONE }
#define DFR_COMMAND(_label, _weight, _command, _color) \
	{ (_label), 0, (_weight), DFR_ACT_CMD, (_command), DFR_IND_NONE, (_color), DFR_MOD_NONE }
#define DFR_KEY(_label, _key, _weight, _color) \
	{ (_label), (_key), (_weight), DFR_ACT_KEY, 0, DFR_IND_NONE, (_color), DFR_MOD_NONE }
#define DFR_APPDECK(_label, _weight, _color) \
	{ (_label), 0, (_weight), DFR_ACT_APPDECK, 0, DFR_IND_NONE, (_color), DFR_MOD_NONE }

/* Fn: the complete physical function row. F9 already has Omarchy's
 * push-to-talk press/release bindings, so holding the F9 zone dictates. */
static const struct dfr_key dfr_keys_fn[] = {
	{ "ESC", KEY_ESC, 1.5f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_ESC },
	{ "F1", KEY_F1, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F2", KEY_F2, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F3", KEY_F3, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F4", KEY_F4, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F5", KEY_F5, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F6", KEY_F6, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F7", KEY_F7, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F8", KEY_F8, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F9", KEY_F9, 1.1f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_VOICE },
	{ "F10", KEY_F10, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F11", KEY_F11, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
	{ "F12", KEY_F12, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_TEXT },
};

/* Default home row. Commands go straight to Hyprland/Omarchy, avoiding
 * synthetic F13-F24 bindings. GPT and MEDIA switch the bar itself. APPS
 * exposes the deck that matches the focused application; the SHOT and REC
 * zones drive Omarchy capture one tap away. */
static const struct dfr_key dfr_keys_media[] = {
	{ "ESC", KEY_ESC, 0.85f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_ESC },
	{ "1", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 1", DFR_IND_WORKSPACE_1, DFR_COL_WORKSPACE },
	{ "2", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 2", DFR_IND_WORKSPACE_2, DFR_COL_WORKSPACE },
	{ "3", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 3", DFR_IND_WORKSPACE_3, DFR_COL_WORKSPACE },
	{ "4", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 4", DFR_IND_WORKSPACE_4, DFR_COL_WORKSPACE },
	{ "5", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 5", DFR_IND_WORKSPACE_5, DFR_COL_WORKSPACE },
	{ "6", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 6", DFR_IND_WORKSPACE_6, DFR_COL_WORKSPACE },
	{ "7", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 7", DFR_IND_WORKSPACE_7, DFR_COL_WORKSPACE },
	{ "8", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 8", DFR_IND_WORKSPACE_8, DFR_COL_WORKSPACE },
	{ "9", 0, 0.66f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 9", DFR_IND_WORKSPACE_9, DFR_COL_WORKSPACE },
	{ "10", 0, 0.76f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 10", DFR_IND_WORKSPACE_10, DFR_COL_WORKSPACE },
	DFR_APPDECK(DFR_GLYPH_GRID " APPS", 1.00f, DFR_COL_ACTION),
	DFR_COMMAND(DFR_GLYPH_CAMERA " SHOT", 0.95f, "/run/t1-custom-bar/touchbar-action shot-region", DFR_COL_SHOT),
	DFR_COMMAND(DFR_GLYPH_RECORD " REC", 1.00f, "/run/t1-custom-bar/touchbar-action record-toggle", DFR_COL_REC),
	{ "GPT", 0, 0.95f, DFR_ACT_LAYER, "meta", DFR_IND_NONE, DFR_COL_CHATGPT },
	{ "MEDIA", 0, 1.05f, DFR_ACT_LAYER, "alt", DFR_IND_NONE, DFR_COL_PLAY },
	{ "OM", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-root", DFR_IND_NONE, DFR_COL_OMARCHY },
	{ DFR_GLYPH_MIC, 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action dictation-toggle", DFR_IND_NONE, DFR_COL_VOICE },
};

/* Ctrl+Fn: live status plus safe Omarchy system surfaces. */
static const struct dfr_key dfr_keys_ctrl[] = {
	DFR_KEY("ESC", KEY_ESC, 1.0f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.8f, "media", DFR_COL_OMARCHY),
	{ "KBD", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-hardware", DFR_IND_KBDLIGHT, DFR_COL_STATUS },
	{ "BAT", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-system", DFR_IND_BATTERY, DFR_COL_STATUS },
	{ "WIFI", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-hardware", DFR_IND_WIFI, DFR_COL_STATUS },
	{ "BT", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-hardware", DFR_IND_BT, DFR_COL_STATUS },
	DFR_LAYER(DFR_GLYPH_CAMERA " CAPTURE", 1.4f, "capture", DFR_COL_SHOT),
	{ "LOCK", 0, 1.1f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action lock", DFR_IND_NONE, DFR_COL_ESC },
};

/* Option+Fn: media and brightness. */
static const struct dfr_key dfr_keys_alt[] = {
	DFR_KEY("ESC", KEY_ESC, 1.0f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.8f, "media", DFR_COL_OMARCHY),
	{ DFR_GLYPH_BRDOWN, KEY_BRIGHTNESSDOWN, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_BRIGHT },
	{ DFR_GLYPH_BRUP, KEY_BRIGHTNESSUP, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_BRIGHT },
	{ DFR_GLYPH_PREV, KEY_PREVIOUSSONG, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_SKIP },
	{ DFR_GLYPH_PLAYPAUSE, KEY_PLAYPAUSE, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_PLAY },
	{ DFR_GLYPH_NEXT, KEY_NEXTSONG, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_SKIP },
	{ DFR_GLYPH_MUTE, KEY_MUTE, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_MUTE },
	{ DFR_GLYPH_VOLDOWN, KEY_VOLUMEDOWN, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_VOL },
	{ DFR_GLYPH_VOLUP, KEY_VOLUMEUP, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_VOL },
};

/* ChatGPT/Codex deck. Each fixed helper focuses the desktop app before sending
 * the shortcut, so this row behaves correctly whether the user opened it from
 * Home, the APPS button or another app deck. */
static const struct dfr_key dfr_keys_meta[] = {
	DFR_KEY("ESC", KEY_ESC, 1.00f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.75f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_ROBOT " AI", 1.20f, "meta", DFR_COL_CHATGPT),
	DFR_COMMAND("NEW", 0.90f, "/run/t1-custom-bar/touchbar-action gpt-new", DFR_COL_CHATGPT),
	DFR_COMMAND("CMD", 0.90f, "/run/t1-custom-bar/touchbar-action gpt-command", DFR_COL_CHATGPT),
	DFR_COMMAND("FILES", 0.95f, "/run/t1-custom-bar/touchbar-action gpt-files", DFR_COL_SKIP),
	DFR_COMMAND("OPEN", 0.90f, "/run/t1-custom-bar/touchbar-action gpt-open-file", DFR_COL_SKIP),
	DFR_COMMAND("SIDE", 0.85f, "/run/t1-custom-bar/touchbar-action gpt-sidebar", DFR_COL_ACTION),
	DFR_COMMAND("BOTTOM", 1.05f, "/run/t1-custom-bar/touchbar-action gpt-bottom", DFR_COL_ACTION),
	DFR_COMMAND("TERM", 0.90f, "/run/t1-custom-bar/touchbar-action gpt-terminal", DFR_COL_STATUS),
	DFR_COMMAND("REVIEW", 1.10f, "/run/t1-custom-bar/touchbar-action gpt-review", DFR_COL_BRIGHT),
	DFR_COMMAND("WEB", 0.85f, "/run/t1-custom-bar/touchbar-action gpt-web", DFR_COL_SKIP),
	DFR_COMMAND("VOICE", 1.05f, "/run/t1-custom-bar/touchbar-action gpt-voice", DFR_COL_VOICE),
	DFR_COMMAND("DICTATE", 1.15f, "/run/t1-custom-bar/touchbar-action gpt-dictate", DFR_COL_VOICE),
	DFR_LAYER("MEDIA", 1.00f, "alt", DFR_COL_VOL),
};

/* Automatic browser deck. The badge is deliberately a same-layer action: it
 * acts as a stable visual title without launching anything if tapped. */
static const struct dfr_key dfr_keys_browser[] = {
	DFR_KEY("ESC", KEY_ESC, 1.10f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.85f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_WEB " WEB", 1.35f, "browser", DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_BACK, KEY_LEFT, 0.85f, DFR_MOD_ALT, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_FORWARD, KEY_RIGHT, 0.85f, DFR_MOD_ALT, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_RELOAD, KEY_R, 0.85f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_HOTKEY("URL", KEY_L, 1.00f, DFR_MOD_CTRL, DFR_COL_CHATGPT),
	DFR_HOTKEY("TAB-", KEY_TAB, 0.95f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_ACTION),
	DFR_HOTKEY("TAB+", KEY_TAB, 0.95f, DFR_MOD_CTRL, DFR_COL_ACTION),
	DFR_HOTKEY(DFR_GLYPH_PLUS, KEY_T, 0.80f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_HOTKEY(DFR_GLYPH_CLOSE, KEY_W, 0.80f, DFR_MOD_CTRL, DFR_COL_ESC),
	DFR_HOTKEY(DFR_GLYPH_SEARCH, KEY_F, 0.85f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_COMMAND(DFR_GLYPH_CAMERA, 0.75f, "/run/t1-custom-bar/touchbar-action shot-region", DFR_COL_SHOT),
	DFR_LAYER("MEDIA", 1.15f, "alt", DFR_COL_VOL),
};

static const struct dfr_key dfr_keys_terminal[] = {
	DFR_KEY("ESC", KEY_ESC, 1.10f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.85f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_TERM " TERM", 1.45f, "terminal", DFR_COL_CHATGPT),
	DFR_HOTKEY(DFR_GLYPH_COPY " COPY", KEY_C, 1.20f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_PASTE " PASTE", KEY_V, 1.25f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_PLAY),
	DFR_HOTKEY("INT", KEY_C, 0.90f, DFR_MOD_CTRL, DFR_COL_ESC),
	DFR_HOTKEY("CLEAR", KEY_L, 1.05f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY(DFR_GLYPH_UP " PAGE", KEY_PAGEUP, 0.85f, DFR_MOD_SHIFT, DFR_COL_ACTION),
	DFR_KEY(DFR_GLYPH_UP, KEY_UP, 0.70f, DFR_COL_SKIP),
	DFR_KEY("DOWN", KEY_DOWN, 0.75f, DFR_COL_SKIP),
	DFR_COMMAND("NEW", 1.00f, "/run/t1-custom-bar/touchbar-action terminal", DFR_COL_PLAY),
	DFR_COMMAND("TMUX", 1.10f, "/run/t1-custom-bar/touchbar-action terminal-tmux", DFR_COL_STATUS),
	DFR_LAYER("AI", 0.80f, "meta", DFR_COL_CHATGPT),
	DFR_LAYER("MEDIA", 1.10f, "alt", DFR_COL_VOL),
};

static const struct dfr_key dfr_keys_code[] = {
	DFR_KEY("ESC", KEY_ESC, 1.05f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.80f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_CODE " CODE", 1.35f, "code", DFR_COL_CHATGPT),
	DFR_HOTKEY(DFR_GLYPH_SAVE " SAVE", KEY_S, 1.10f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_HOTKEY(DFR_GLYPH_UNDO, KEY_Z, 0.75f, DFR_MOD_CTRL, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_REDO, KEY_Z, 0.75f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_SEARCH, KEY_F, 0.75f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY("REPLACE", KEY_H, 1.10f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY("CMD", KEY_P, 0.90f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_ACTION),
	DFR_HOTKEY("TERM", KEY_GRAVE, 0.95f, DFR_MOD_CTRL, DFR_COL_STATUS),
	DFR_HOTKEY("FORMAT", KEY_F, 1.05f, DFR_MOD_ALT | DFR_MOD_SHIFT, DFR_COL_CHATGPT),
	DFR_KEY("RUN", KEY_F5, 0.80f, DFR_COL_PLAY),
	DFR_LAYER("AI", 0.75f, "meta", DFR_COL_CHATGPT),
	DFR_LAYER("MEDIA", 1.05f, "alt", DFR_COL_VOL),
};

/* Omarchy's default Neovim/LazyVim deck. Multi-key leader mappings use only
 * fixed, allowlisted sequences in touchbar-action; Save remains a native key. */
static const struct dfr_key dfr_keys_nvim[] = {
	DFR_KEY("ESC", KEY_ESC, 1.00f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.75f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_CODE " NVIM", 1.25f, "nvim", DFR_COL_PLAY),
	DFR_COMMAND("FILES", 0.95f, "/run/t1-custom-bar/touchbar-action nvim-files", DFR_COL_SKIP),
	DFR_COMMAND("GREP", 0.90f, "/run/t1-custom-bar/touchbar-action nvim-grep", DFR_COL_BRIGHT),
	DFR_COMMAND("TREE", 0.90f, "/run/t1-custom-bar/touchbar-action nvim-tree", DFR_COL_SKIP),
	DFR_COMMAND("GIT", 0.80f, "/run/t1-custom-bar/touchbar-action nvim-git", DFR_COL_OMARCHY),
	DFR_HOTKEY(DFR_GLYPH_SAVE, KEY_S, 0.70f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_COMMAND("FORMAT", 1.00f, "/run/t1-custom-bar/touchbar-action nvim-format", DFR_COL_PLAY),
	DFR_COMMAND("DIAG", 0.85f, "/run/t1-custom-bar/touchbar-action nvim-diag", DFR_COL_ESC),
	DFR_COMMAND("TERM", 0.85f, "/run/t1-custom-bar/touchbar-action nvim-terminal", DFR_COL_STATUS),
	DFR_COMMAND("NEW", 0.75f, "/run/t1-custom-bar/touchbar-action nvim-new", DFR_COL_CHATGPT),
	DFR_COMMAND("BUFS", 0.80f, "/run/t1-custom-bar/touchbar-action nvim-buffers", DFR_COL_ACTION),
	DFR_LAYER("AI", 0.70f, "meta", DFR_COL_CHATGPT),
	DFR_LAYER("MEDIA", 0.95f, "alt", DFR_COL_VOL),
};

static const struct dfr_key dfr_keys_files[] = {
	DFR_KEY("ESC", KEY_ESC, 1.05f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.80f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_FOLDER " FILES", 1.45f, "files", DFR_COL_BRIGHT),
	DFR_HOTKEY(DFR_GLYPH_BACK, KEY_LEFT, 0.80f, DFR_MOD_ALT, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_FORWARD, KEY_RIGHT, 0.80f, DFR_MOD_ALT, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_UP, KEY_UP, 0.80f, DFR_MOD_ALT, DFR_COL_STATUS),
	DFR_HOTKEY("HOME", KEY_HOME, 0.95f, DFR_MOD_ALT, DFR_COL_OMARCHY),
	DFR_HOTKEY(DFR_GLYPH_SEARCH, KEY_F, 0.80f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY("PATH", KEY_L, 0.90f, DFR_MOD_CTRL, DFR_COL_STATUS),
	DFR_HOTKEY(DFR_GLYPH_PLUS " DIR", KEY_N, 1.00f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_PLAY),
	DFR_KEY("RENAME", KEY_F2, 1.00f, DFR_COL_ACTION),
	DFR_HOTKEY("HIDDEN", KEY_H, 1.05f, DFR_MOD_CTRL, DFR_COL_ACTION),
	DFR_HOTKEY(DFR_GLYPH_COPY, KEY_C, 0.75f, DFR_MOD_CTRL, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_PASTE, KEY_V, 0.75f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_COMMAND("TERM", 0.95f, "/run/t1-custom-bar/touchbar-action terminal", DFR_COL_STATUS),
	DFR_LAYER("MEDIA", 1.05f, "alt", DFR_COL_VOL),
};

static const struct dfr_key dfr_keys_comms[] = {
	DFR_KEY("ESC", KEY_ESC, 1.05f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.80f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_CHAT " CHAT", 1.40f, "comms", DFR_COL_VOICE),
	DFR_HOTKEY("NEW", KEY_N, 0.95f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_HOTKEY(DFR_GLYPH_SEARCH, KEY_F, 0.85f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY(DFR_GLYPH_UNDO, KEY_Z, 0.75f, DFR_MOD_CTRL, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_COPY, KEY_C, 0.75f, DFR_MOD_CTRL, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_PASTE, KEY_V, 0.75f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_COMMAND("DICTATE", 1.20f, "/run/t1-custom-bar/touchbar-action dictation-toggle", DFR_COL_VOICE),
	DFR_COMMAND("EMOJI", 1.05f, "/run/t1-custom-bar/touchbar-action menu-emoji", DFR_COL_ACTION),
	DFR_COMMAND(DFR_GLYPH_CAMERA " SHOT", 1.05f, "/run/t1-custom-bar/touchbar-action shot-region", DFR_COL_SHOT),
	DFR_LAYER("AI", 0.75f, "meta", DFR_COL_CHATGPT),
	DFR_LAYER("MEDIA", 1.05f, "alt", DFR_COL_VOL),
};

static const struct dfr_key dfr_keys_document[] = {
	DFR_KEY("ESC", KEY_ESC, 1.05f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.80f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_DOC " DOC", 1.30f, "document", DFR_COL_BRIGHT),
	DFR_HOTKEY(DFR_GLYPH_SAVE, KEY_S, 0.80f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_HOTKEY(DFR_GLYPH_UNDO, KEY_Z, 0.75f, DFR_MOD_CTRL, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_REDO, KEY_Z, 0.75f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_SEARCH, KEY_F, 0.75f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY("BOLD", KEY_B, 0.90f, DFR_MOD_CTRL, DFR_COL_TEXT),
	DFR_HOTKEY("ITALIC", KEY_I, 0.95f, DFR_MOD_CTRL, DFR_COL_TEXT),
	DFR_HOTKEY("ZOOM-", KEY_MINUS, 0.95f, DFR_MOD_CTRL, DFR_COL_STATUS),
	DFR_HOTKEY("ZOOM+", KEY_EQUAL, 0.95f, DFR_MOD_CTRL, DFR_COL_STATUS),
	DFR_HOTKEY("PRINT", KEY_P, 0.95f, DFR_MOD_CTRL, DFR_COL_ACTION),
	DFR_COMMAND(DFR_GLYPH_FULLSCREEN, 0.75f, "/run/t1-custom-bar/touchbar-action fullscreen", DFR_COL_CHATGPT),
	DFR_LAYER("AI", 0.75f, "meta", DFR_COL_CHATGPT),
	DFR_LAYER("MEDIA", 1.00f, "alt", DFR_COL_VOL),
};

static const struct dfr_key dfr_keys_reader[] = {
	DFR_KEY("ESC", KEY_ESC, 1.00f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.75f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_READ " READ", 1.25f, "reader", DFR_COL_BRIGHT),
	DFR_KEY("PAGE-", KEY_PAGEUP, 1.00f, DFR_COL_SKIP),
	DFR_KEY("PAGE+", KEY_PAGEDOWN, 1.00f, DFR_COL_SKIP),
	DFR_KEY("FIRST", KEY_HOME, 0.90f, DFR_COL_STATUS),
	DFR_KEY("LAST", KEY_END, 0.85f, DFR_COL_STATUS),
	DFR_HOTKEY(DFR_GLYPH_SEARCH, KEY_F, 0.75f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY("ZOOM-", KEY_MINUS, 0.90f, DFR_MOD_CTRL, DFR_COL_STATUS),
	DFR_HOTKEY("ZOOM+", KEY_EQUAL, 0.90f, DFR_MOD_CTRL, DFR_COL_STATUS),
	DFR_HOTKEY("PRINT", KEY_P, 0.85f, DFR_MOD_CTRL, DFR_COL_ACTION),
	DFR_COMMAND(DFR_GLYPH_FULLSCREEN, 0.70f, "/run/t1-custom-bar/touchbar-action fullscreen", DFR_COL_CHATGPT),
	DFR_LAYER("AI", 0.70f, "meta", DFR_COL_CHATGPT),
	DFR_LAYER("MEDIA", 0.95f, "alt", DFR_COL_VOL),
};

static const struct dfr_key dfr_keys_music[] = {
	DFR_KEY("ESC", KEY_ESC, 1.05f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.80f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_MUSIC " NOW", 1.45f, "music", DFR_COL_VOICE),
	DFR_KEY(DFR_GLYPH_PREV, KEY_PREVIOUSSONG, 1.10f, DFR_COL_SKIP),
	DFR_KEY(DFR_GLYPH_PLAYPAUSE, KEY_PLAYPAUSE, 1.35f, DFR_COL_PLAY),
	DFR_KEY(DFR_GLYPH_NEXT, KEY_NEXTSONG, 1.10f, DFR_COL_SKIP),
	DFR_KEY(DFR_GLYPH_MUTE, KEY_MUTE, 0.95f, DFR_COL_MUTE),
	DFR_KEY(DFR_GLYPH_VOLDOWN, KEY_VOLUMEDOWN, 1.05f, DFR_COL_VOL),
	DFR_KEY(DFR_GLYPH_VOLUP, KEY_VOLUMEUP, 1.05f, DFR_COL_VOL),
	DFR_KEY(DFR_GLYPH_BRDOWN, KEY_BRIGHTNESSDOWN, 0.95f, DFR_COL_BRIGHT),
	DFR_KEY(DFR_GLYPH_BRUP, KEY_BRIGHTNESSUP, 0.95f, DFR_COL_BRIGHT),
	DFR_COMMAND("DICTATE", 1.20f, "/run/t1-custom-bar/touchbar-action dictation-toggle", DFR_COL_VOICE),
};

static const struct dfr_key dfr_keys_universal[] = {
	DFR_KEY("ESC", KEY_ESC, 1.05f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.80f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_VIEWGRID " APP", 1.30f, "universal", DFR_COL_CHATGPT),
	DFR_HOTKEY(DFR_GLYPH_UNDO, KEY_Z, 0.75f, DFR_MOD_CTRL, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_REDO, KEY_Z, 0.75f, DFR_MOD_CTRL | DFR_MOD_SHIFT, DFR_COL_SKIP),
	DFR_HOTKEY("CUT", KEY_X, 0.80f, DFR_MOD_CTRL, DFR_COL_ESC),
	DFR_HOTKEY(DFR_GLYPH_COPY, KEY_C, 0.75f, DFR_MOD_CTRL, DFR_COL_SKIP),
	DFR_HOTKEY(DFR_GLYPH_PASTE, KEY_V, 0.75f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_HOTKEY(DFR_GLYPH_SEARCH, KEY_F, 0.75f, DFR_MOD_CTRL, DFR_COL_BRIGHT),
	DFR_HOTKEY(DFR_GLYPH_SAVE, KEY_S, 0.75f, DFR_MOD_CTRL, DFR_COL_PLAY),
	DFR_COMMAND(DFR_GLYPH_FULLSCREEN, 0.75f, "/run/t1-custom-bar/touchbar-action fullscreen", DFR_COL_CHATGPT),
	DFR_COMMAND(DFR_GLYPH_CAMERA " SHOT", 0.90f, "/run/t1-custom-bar/touchbar-action shot-region", DFR_COL_SHOT),
	DFR_COMMAND(DFR_GLYPH_MIC, 0.65f, "/run/t1-custom-bar/touchbar-action dictation-toggle", DFR_COL_VOICE),
	DFR_LAYER("AI", 0.75f, "meta", DFR_COL_CHATGPT),
	DFR_LAYER("MEDIA", 1.00f, "alt", DFR_COL_VOL),
	DFR_COMMAND("OM", 0.75f, "/run/t1-custom-bar/touchbar-action menu-root", DFR_COL_OMARCHY),
};

/* Omarchy capture deck: every screenshot and recording mode one tap away.
 * REC toggles whatever recording is configured; RECFULL starts a full-screen
 * recording (or stops the active one). */
static const struct dfr_key dfr_keys_capture[] = {
	DFR_KEY("ESC", KEY_ESC, 1.00f, DFR_COL_ESC),
	DFR_LAYER(DFR_GLYPH_HOME, 0.75f, "media", DFR_COL_OMARCHY),
	DFR_LAYER(DFR_GLYPH_CAMERA " CAPTURE", 1.50f, "capture", DFR_COL_SHOT),
	DFR_COMMAND(DFR_GLYPH_CAMERA " AREA", 1.05f, "/run/t1-custom-bar/touchbar-action shot-region", DFR_COL_SHOT),
	DFR_COMMAND(DFR_GLYPH_WINDOW " WIN", 0.95f, "/run/t1-custom-bar/touchbar-action shot-window", DFR_COL_SKIP),
	DFR_COMMAND(DFR_GLYPH_MONITOR " FULL", 0.95f, "/run/t1-custom-bar/touchbar-action shot-full", DFR_COL_SKIP),
	DFR_COMMAND(DFR_GLYPH_OCR " OCR", 0.85f, "/run/t1-custom-bar/touchbar-action ocr", DFR_COL_BRIGHT),
	DFR_COMMAND(DFR_GLYPH_RECORD " REC", 1.00f, "/run/t1-custom-bar/touchbar-action record-toggle", DFR_COL_REC),
	DFR_COMMAND(DFR_GLYPH_MONITOR " RECFULL", 1.15f, "/run/t1-custom-bar/touchbar-action record-full", DFR_COL_REC),
	DFR_COMMAND(DFR_GLYPH_MIC, 0.90f, "/run/t1-custom-bar/touchbar-action dictation-toggle", DFR_COL_VOICE),
	DFR_LAYER("MEDIA", 1.00f, "alt", DFR_COL_VOL),
};

static const struct dfr_layout dfr_layouts[] = {
	{ "fn", dfr_keys_fn, (int)(sizeof(dfr_keys_fn) / sizeof(dfr_keys_fn[0])) },
	{ "media", dfr_keys_media, (int)(sizeof(dfr_keys_media) / sizeof(dfr_keys_media[0])) },
	{ "ctrl", dfr_keys_ctrl, (int)(sizeof(dfr_keys_ctrl) / sizeof(dfr_keys_ctrl[0])) },
	{ "alt", dfr_keys_alt, (int)(sizeof(dfr_keys_alt) / sizeof(dfr_keys_alt[0])) },
	{ "meta", dfr_keys_meta, (int)(sizeof(dfr_keys_meta) / sizeof(dfr_keys_meta[0])) },
	{ "browser", dfr_keys_browser, (int)(sizeof(dfr_keys_browser) / sizeof(dfr_keys_browser[0])) },
	{ "terminal", dfr_keys_terminal, (int)(sizeof(dfr_keys_terminal) / sizeof(dfr_keys_terminal[0])) },
	{ "code", dfr_keys_code, (int)(sizeof(dfr_keys_code) / sizeof(dfr_keys_code[0])) },
	{ "nvim", dfr_keys_nvim, (int)(sizeof(dfr_keys_nvim) / sizeof(dfr_keys_nvim[0])) },
	{ "files", dfr_keys_files, (int)(sizeof(dfr_keys_files) / sizeof(dfr_keys_files[0])) },
	{ "comms", dfr_keys_comms, (int)(sizeof(dfr_keys_comms) / sizeof(dfr_keys_comms[0])) },
	{ "document", dfr_keys_document, (int)(sizeof(dfr_keys_document) / sizeof(dfr_keys_document[0])) },
	{ "reader", dfr_keys_reader, (int)(sizeof(dfr_keys_reader) / sizeof(dfr_keys_reader[0])) },
	{ "music", dfr_keys_music, (int)(sizeof(dfr_keys_music) / sizeof(dfr_keys_music[0])) },
	{ "universal", dfr_keys_universal, (int)(sizeof(dfr_keys_universal) / sizeof(dfr_keys_universal[0])) },
	{ "capture", dfr_keys_capture, (int)(sizeof(dfr_keys_capture) / sizeof(dfr_keys_capture[0])) },
};
#define DFR_NLAYOUTS ((int)(sizeof(dfr_layouts) / sizeof(dfr_layouts[0])))

static inline int dfr_layout_index(const char *name)
{
	for (int i = 0; i < DFR_NLAYOUTS; i++) {
		const char *a = dfr_layouts[i].name, *b = name;
		while (*a && *a == *b) { a++; b++; }
		if (*a == 0 && *b == 0)
			return i;
	}
	return -1;
}

static inline float dfr_total_weight(const struct dfr_layout *layout)
{
	float total = 0.0f;
	for (int i = 0; i < layout->nkeys; i++)
		total += layout->keys[i].weight;
	return total;
}

static inline int dfr_zone_from_nx(const struct dfr_layout *layout, float nx)
{
	float acc = 0.0f;
	if (nx < 0.0f) nx = 0.0f;
	if (nx > 0.99999f) nx = 0.99999f;
	float target = nx * dfr_total_weight(layout);
	for (int i = 0; i < layout->nkeys; i++) {
		acc += layout->keys[i].weight;
		if (target < acc)
			return i;
	}
	return layout->nkeys - 1;
}

static inline void dfr_key_extent(const struct dfr_layout *layout, int idx,
				  int width_px, int *x0, int *x1)
{
	float total = dfr_total_weight(layout), acc = 0.0f;
	*x0 = 0;
	*x1 = width_px;
	for (int i = 0; i <= idx && i < layout->nkeys; i++) {
		float lo = acc / total;
		acc += layout->keys[i].weight;
		float hi = acc / total;
		if (i == idx) {
			*x0 = (int)(lo * (float)width_px + 0.5f);
			*x1 = (int)(hi * (float)width_px + 0.5f);
		}
	}
	if (*x1 > width_px) *x1 = width_px;
	if (*x0 < 0) *x0 = 0;
}

static inline int dfr_layout_has_indicators(const struct dfr_layout *layout)
{
	for (int i = 0; i < layout->nkeys; i++)
		if (layout->keys[i].indicator != DFR_IND_NONE)
			return 1;
	return 0;
}

#endif
