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
	const char *expected[] = { "fn", "media", "ctrl", "alt", "meta" };

	if (DFR_NLAYOUTS != 5)
		return fail("all", DFR_NLAYOUTS, "unexpected layout count");

	for (int l = 0; l < DFR_NLAYOUTS; l++) {
		const struct dfr_layout *layout = &dfr_layouts[l];
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
			if (key->action == DFR_ACT_KEY && key->keycode <= 0)
				return fail(layout->name, k, "key action has no keycode");
			if (key->action == DFR_ACT_CMD && (!key->cmd || !*key->cmd))
				return fail(layout->name, k, "command action has no command");
			if (x0 < 0 || x1 > 2170 || x0 >= x1)
				return fail(layout->name, k, "invalid pixel extent");
			if (dfr_zone_from_nx(layout, midpoint) != k)
				return fail(layout->name, k, "touch midpoint maps to wrong key");
		}
	}

	puts("custom layout geometry/actions: PASS");
	return 0;
}
