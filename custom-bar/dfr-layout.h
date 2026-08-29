/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Omarchy layout overlay for xeeban/macbook-t1-linux dfrd.
 *
 * Derived from dfrd/dfr-layout.h at commit
 * 982d97a61a0c834d03e7cf7af5203b11ad7bae44. The renderer and touch daemon
 * compile this same file, so the painted zones and touch targets stay aligned.
 *
 * dfr-fnd expects the layout names below. We retain those internal names while
 * assigning Omarchy-friendly roles:
 *   media = home row (default; tap Fn again to return)
 *   fn    = Escape and F1-F12
 *   ctrl  = live system row
 *   alt   = media/brightness row
 *   meta  = AI/productivity row
 */
#ifndef DFR_LAYOUT_H
#define DFR_LAYOUT_H

#include <linux/input-event-codes.h>

enum dfr_action {
	DFR_ACT_KEY = 0,
	DFR_ACT_CMD = 1,
	DFR_ACT_LAYER = 2,
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

#define DFR_COL_TEXT      0xFFF8FAFCu
#define DFR_COL_ESC       0xFFFF5F57u
#define DFR_COL_WORKSPACE 0xFF7DD3FCu
#define DFR_COL_CHATGPT   0xFF74E0B5u
#define DFR_COL_OMARCHY   0xFFFFB86Cu
#define DFR_COL_VOICE     0xFFFF79C6u
#define DFR_COL_ACTION    0xFFBD93F9u
#define DFR_COL_STATUS    0xFF8BE9FDu
#define DFR_COL_BRIGHT    0xFFFFC107u
#define DFR_COL_SKIP      0xFF64B5F6u
#define DFR_COL_PLAY      0xFF66BB6Au
#define DFR_COL_MUTE      0xFFFF7043u
#define DFR_COL_VOL       0xFF42A5F5u

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
 * synthetic F13-F24 bindings. GPT and MEDIA switch the bar itself. */
static const struct dfr_key dfr_keys_media[] = {
	{ "ESC", KEY_ESC, 1.35f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_ESC },
	{ "1", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 1", DFR_IND_WORKSPACE_1, DFR_COL_WORKSPACE },
	{ "2", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 2", DFR_IND_WORKSPACE_2, DFR_COL_WORKSPACE },
	{ "3", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 3", DFR_IND_WORKSPACE_3, DFR_COL_WORKSPACE },
	{ "4", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 4", DFR_IND_WORKSPACE_4, DFR_COL_WORKSPACE },
	{ "5", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 5", DFR_IND_WORKSPACE_5, DFR_COL_WORKSPACE },
	{ "6", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 6", DFR_IND_WORKSPACE_6, DFR_COL_WORKSPACE },
	{ "7", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 7", DFR_IND_WORKSPACE_7, DFR_COL_WORKSPACE },
	{ "8", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 8", DFR_IND_WORKSPACE_8, DFR_COL_WORKSPACE },
	{ "9", 0, 0.72f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 9", DFR_IND_WORKSPACE_9, DFR_COL_WORKSPACE },
	{ "10", 0, 0.82f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action workspace 10", DFR_IND_WORKSPACE_10, DFR_COL_WORKSPACE },
	{ "GPT", 0, 1.25f, DFR_ACT_LAYER, "meta", DFR_IND_NONE, DFR_COL_CHATGPT },
	{ "MEDIA", 0, 1.45f, DFR_ACT_LAYER, "alt", DFR_IND_NONE, DFR_COL_PLAY },
	{ "OM", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-root", DFR_IND_NONE, DFR_COL_OMARCHY },
	{ DFR_GLYPH_MIC, 0, 0.8f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action dictation-toggle", DFR_IND_NONE, DFR_COL_VOICE },
};

/* Ctrl+Fn: live status plus safe Omarchy system surfaces. */
static const struct dfr_key dfr_keys_ctrl[] = {
	{ "KBD", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-hardware", DFR_IND_KBDLIGHT, DFR_COL_STATUS },
	{ "BAT", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-system", DFR_IND_BATTERY, DFR_COL_STATUS },
	{ "WIFI", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-hardware", DFR_IND_WIFI, DFR_COL_STATUS },
	{ "BT", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-hardware", DFR_IND_BT, DFR_COL_STATUS },
	{ "CAPTURE", 0, 1.4f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-capture", DFR_IND_NONE, DFR_COL_ACTION },
	{ "LOCK", 0, 1.1f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action lock", DFR_IND_NONE, DFR_COL_ESC },
};

/* Option+Fn: media and brightness. */
static const struct dfr_key dfr_keys_alt[] = {
	{ "BACK", 0, 1.4f, DFR_ACT_LAYER, "media", DFR_IND_NONE, DFR_COL_ESC },
	{ DFR_GLYPH_BRDOWN, KEY_BRIGHTNESSDOWN, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_BRIGHT },
	{ DFR_GLYPH_BRUP, KEY_BRIGHTNESSUP, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_BRIGHT },
	{ DFR_GLYPH_PREV, KEY_PREVIOUSSONG, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_SKIP },
	{ DFR_GLYPH_PLAYPAUSE, KEY_PLAYPAUSE, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_PLAY },
	{ DFR_GLYPH_NEXT, KEY_NEXTSONG, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_SKIP },
	{ DFR_GLYPH_MUTE, KEY_MUTE, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_MUTE },
	{ DFR_GLYPH_VOLDOWN, KEY_VOLUMEDOWN, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_VOL },
	{ DFR_GLYPH_VOLUP, KEY_VOLUMEUP, 1.0f, DFR_ACT_KEY, 0, DFR_IND_NONE, DFR_COL_VOL },
};

/* Command+Fn: AI and daily tools. VOICE calls Voxtype directly instead of
 * synthesizing F9, which makes a quick tap a reliable start/stop toggle. */
static const struct dfr_key dfr_keys_meta[] = {
	{ "BACK", 0, 1.2f, DFR_ACT_LAYER, "media", DFR_IND_NONE, DFR_COL_ESC },
	{ "OPEN", 0, 1.2f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action gpt-open", DFR_IND_NONE, DFR_COL_CHATGPT },
	{ "VOICE", 0, 1.4f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action dictation-toggle", DFR_IND_NONE, DFR_COL_VOICE },
	{ "NEW", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action gpt-new", DFR_IND_NONE, DFR_COL_CHATGPT },
	{ "CMD", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action gpt-command", DFR_IND_NONE, DFR_COL_CHATGPT },
	{ "CLIP", 0, 1.15f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-clipboard", DFR_IND_NONE, DFR_COL_ACTION },
	{ "SHOT", 0, 1.15f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-capture", DFR_IND_NONE, DFR_COL_ACTION },
	{ "COPY", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action gpt-copy", DFR_IND_NONE, DFR_COL_ACTION },
	{ "PASTE", 0, 1.0f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action gpt-paste", DFR_IND_NONE, DFR_COL_ACTION },
	{ "TERM", 0, 1.15f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action terminal", DFR_IND_NONE, DFR_COL_ACTION },
	{ "EMOJI", 0, 1.25f, DFR_ACT_CMD, "/run/t1-custom-bar/touchbar-action menu-emoji", DFR_IND_NONE, DFR_COL_ACTION },
};

static const struct dfr_layout dfr_layouts[] = {
	{ "fn", dfr_keys_fn, (int)(sizeof(dfr_keys_fn) / sizeof(dfr_keys_fn[0])) },
	{ "media", dfr_keys_media, (int)(sizeof(dfr_keys_media) / sizeof(dfr_keys_media[0])) },
	{ "ctrl", dfr_keys_ctrl, (int)(sizeof(dfr_keys_ctrl) / sizeof(dfr_keys_ctrl[0])) },
	{ "alt", dfr_keys_alt, (int)(sizeof(dfr_keys_alt) / sizeof(dfr_keys_alt[0])) },
	{ "meta", dfr_keys_meta, (int)(sizeof(dfr_keys_meta) / sizeof(dfr_keys_meta[0])) },
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
