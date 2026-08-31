/* SPDX-License-Identifier: GPL-2.0 */
#include <stdio.h>
#include <string.h>

#include "dfr-layout.h"

static int fail(const char *layout, int key, const char *why)
{
	fprintf(stderr, "layout test failed: %s[%d]: %s\n", layout, key, why);
	return 1;
}

int main(void)
{
	const char *expected[] = {
		"fn", "media", "ctrl", "alt", "meta", "browser", "terminal",
		"code", "nvim", "files", "comms", "document", "reader", "music",
		"universal",
	};
	const unsigned allowed_mods = DFR_MOD_CTRL | DFR_MOD_ALT |
		DFR_MOD_SHIFT | DFR_MOD_META;
	int hotkey_count = 0;

	if (DFR_NLAYOUTS != 15)
		return fail("all", DFR_NLAYOUTS, "unexpected layout count");

	for (int l = 0; l < DFR_NLAYOUTS; l++) {
		const struct dfr_layout *layout = &dfr_layouts[l];
		int has_escape = 0;

		if (strcmp(layout->name, expected[l]) != 0)
			return fail(layout->name, l, "unexpected name/order");
		if (layout->nkeys < 1 || dfr_total_weight(layout) <= 0.0f)
			return fail(layout->name, -1, "empty layout or invalid weight");

		for (int k = 0; k < layout->nkeys; k++) {
			const struct dfr_key *key = &layout->keys[k];
			int x0, x1;
			dfr_key_extent(layout, k, 2170, &x0, &x1);
			float midpoint = ((float)x0 + (float)x1) / (2.0f * 2170.0f);

			if (!key->label || !*key->label || key->weight <= 0.0f)
				return fail(layout->name, k, "bad label or weight");
			if (key->action < DFR_ACT_KEY || key->action > DFR_ACT_HOTKEY)
				return fail(layout->name, k, "unknown action type");
			if (key->action == DFR_ACT_KEY && key->keycode <= 0)
				return fail(layout->name, k, "key action has no keycode");
			if (key->action == DFR_ACT_HOTKEY) {
				hotkey_count++;
				if (key->keycode <= 0)
					return fail(layout->name, k,
						    "hotkey action has no keycode");
				if (key->mods == DFR_MOD_NONE ||
				    (key->mods & ~allowed_mods) != 0)
					return fail(layout->name, k,
						    "hotkey has invalid modifiers");
			}
			if (key->action == DFR_ACT_CMD && (!key->cmd || !*key->cmd))
				return fail(layout->name, k, "command action has no command");
			if (key->action == DFR_ACT_LAYER &&
			    (!key->cmd || dfr_layout_index(key->cmd) < 0))
				return fail(layout->name, k, "layer action has an invalid target");
			if (key->indicator < DFR_IND_NONE || key->indicator >= DFR_IND_COUNT)
				return fail(layout->name, k, "indicator is out of range");
			if (l == 1 && k >= 1 && k <= 10 &&
			    key->indicator != DFR_IND_WORKSPACE_1 + k - 1)
				return fail(layout->name, k, "workspace indicator does not match button");
			if (x0 < 0 || x1 > 2170 || x0 >= x1)
				return fail(layout->name, k, "invalid pixel extent");
			if (dfr_zone_from_nx(layout, midpoint) != k)
				return fail(layout->name, k, "touch midpoint maps to wrong key");
			if (key->action == DFR_ACT_KEY && key->keycode == KEY_ESC)
				has_escape = 1;
		}

		if (!has_escape)
			return fail(layout->name, -1,
				    "layout has no Escape key");
	}
	if (hotkey_count == 0)
		return fail("all", -1, "no native hotkey actions were tested");

	if (strcmp(dfr_keys_media[14].cmd,
		   "/run/t1-custom-bar/touchbar-action dictation-toggle") != 0)
		return fail("media", 14, "home microphone action is missing");
	puts("custom layout geometry/actions: PASS");
	return 0;
}
