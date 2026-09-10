// SPDX-License-Identifier: GPL-2.0
/* Omarchy's existing artwork and actions over the public T1Bridge v1 IPC.
 * No DRM, HID, USB, uinput, firmware or protected fingerprint state access.
 * The legacy main is linked out; only its drawing/indicator functions are
 * reused.
 */
#define _GNU_SOURCE
#define main legacy_renderer_main
/* The Prism deck renderer below supersedes three legacy entry points while
 * reusing every drawing and indicator helper from dfr-render.c in place. */
#define draw_layout legacy_draw_layout
#define do_preview legacy_do_preview
#define ind_refresh legacy_ind_refresh
#include "dfr-render.c"
#undef draw_layout
#undef do_preview
#undef ind_refresh
#undef main
#include <json-c/json.h>
#include <poll.h>
#include <sys/wait.h>
#include <xkbcommon/xkbcommon.h>

static int ipc = -1, shared_fd = -1;
static uint32_t request_id, width, height;
static uint64_t frame_id, released;
static uint32_t *shared;
static int desired, visible = -1, previous, fn_last, dirty = 1, held = -1,
                    held_layout;
static int held_workspace, drag_workspace, suppressed, contact_present,
    input_changed;
static uint32_t contact_x;
static int input_fn, overlay, overlay_progress = -1;
static int64_t last_action, last_fn, last_refresh, last_frame;
static char action_path[PATH_MAX], context_path[PATH_MAX], executable[PATH_MAX];
static pid_t context_pid = -1;
static struct xkb_context *xkb;
static struct xkb_keymap *keymap;
static unsigned char packet[65536];
static uint16_t rd16(const void *p) {
  uint16_t v;
  memcpy(&v, p, 2);
  return le16toh(v);
}
static uint32_t rd32(const void *p) {
  uint32_t v;
  memcpy(&v, p, 4);
  return le32toh(v);
}
static uint64_t rd64(const void *p) {
  uint64_t v;
  memcpy(&v, p, 8);
  return le64toh(v);
}
static void wr16(void *p, uint16_t v) {
  v = htole16(v);
  memcpy(p, &v, 2);
}
static void wr32(void *p, uint32_t v) {
  v = htole32(v);
  memcpy(p, &v, 4);
}
static void wr64(void *p, uint64_t v) {
  v = htole64(v);
  memcpy(p, &v, 8);
}

static uint32_t send_packet(uint16_t type, const void *body, size_t len,
                            int fd) {
  unsigned char data[256] = {0}, control[CMSG_SPACE(sizeof(int))] = {0};
  if (len > sizeof(data) - 16 || request_id == UINT32_MAX)
    return 0;
  memcpy(data, "T1HW", 4);
  wr16(data + 4, 1);
  wr16(data + 6, type);
  wr32(data + 8, (uint32_t)len);
  wr32(data + 12, ++request_id);
  if (len)
    memcpy(data + 16, body, len);
  struct iovec iov = {data, len + 16};
  struct msghdr msg = {.msg_iov = &iov, .msg_iovlen = 1};
  if (fd >= 0) {
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);
    struct cmsghdr *c = CMSG_FIRSTHDR(&msg);
    c->cmsg_level = SOL_SOCKET;
    c->cmsg_type = SCM_RIGHTS;
    c->cmsg_len = CMSG_LEN(sizeof(fd));
    memcpy(CMSG_DATA(c), &fd, sizeof(fd));
  }
  if (sendmsg(ipc, &msg, MSG_NOSIGNAL) != (ssize_t)(len + 16))
    return 0;
  return request_id;
}
static int receive_packet(int timeout, uint16_t *type, uint32_t *id,
                          size_t *len) {
  struct pollfd p = {ipc, POLLIN, 0};
  int n = poll(&p, 1, timeout);
  if (n < 0 && errno == EINTR)
    return 0;
  if (n <= 0)
    return n;
  ssize_t got = recv(ipc, packet, sizeof(packet), MSG_TRUNC);
  if (got < 16 || got > (ssize_t)sizeof(packet) || memcmp(packet, "T1HW", 4) ||
      rd16(packet + 4) != 1)
    return -1;
  *type = rd16(packet + 6);
  *id = rd32(packet + 12);
  *len = rd32(packet + 8);
  if (*len != (size_t)got - 16)
    return -1;
  unsigned char *b = packet + 16;
  if (*type == 0x9001) {
    if (*len != 12 || rd32(b) != 1)
      return -1;
    released = rd64(b + 4);
  }
  if (*type == 0x9002) {
    if (*len < 12 || b[8] > 1 || b[9] > 10 || *len != 12u + 12u * b[9])
      return -1;
    input_fn = b[8];
    contact_present = 0;
    input_changed = 1;
    for (unsigned i = 0; i < b[9]; i++) {
      unsigned char *c = b + 12 + i * 12;
      if (c[1] > 1 || c[2] > 1 || rd32(c + 4) >= width || rd32(c + 8) >= height)
        return -1;
      if (c[1] && c[2]) {
        contact_present++;
        contact_x = rd32(c + 4);
      }
    }
  }
  return 1;
}
static int wait_reply(uint32_t wanted, uint16_t expected) {
  int64_t end = now_ms() + 3000;
  while (wanted && now_ms() < end && !g_stop) {
    uint16_t t;
    uint32_t id;
    size_t len;
    int r = receive_packet(100, &t, &id, &len);
    if (r < 0)
      return -1;
    if (!r)
      continue;
    if (id == wanted) {
      if (t == 0x8003) {
        fprintf(stderr, "Hardware request rejected (code %u)\n",
                len == 4 ? rd32(packet + 16) : 0);
        return -1;
      }
      return t == expected ? 0 : -1;
    }
  }
  return -1;
}
static int request(uint16_t type, const void *body, size_t len) {
  return wait_reply(send_packet(type, body, len, -1), 0x8002);
}
static void launch(const char *command) {
  if (!command || !*command)
    return;
  pid_t p = fork();
  if (p != 0)
    return;
  execl("/bin/sh", "sh", "-c", command, (char *)NULL);
  _exit(127);
}
static void layout_command(const char *command) {
  const char *prefix = "/run/t1-custom-bar/touchbar-action";
  if (!strncmp(command, prefix, strlen(prefix))) {
    char out[2048];
    int n = snprintf(out, sizeof out, "\"$OMARCHY_TOUCHBAR_ACTION\"%s",
                     command + strlen(prefix));
    if (n > 0 && n < (int)sizeof out)
      launch(out);
  } else
    launch(command); /* fixed, source-controlled commands from dfr-layout.h */
}
static int workspace_at(int layout, int zone) {
  if (zone < 0 || zone >= dfr_layouts[layout].nkeys)
    return 0;
  int i = dfr_layouts[layout].keys[zone].indicator;
  return i >= DFR_IND_WORKSPACE_1 && i <= DFR_IND_WORKSPACE_10
             ? i - DFR_IND_WORKSPACE_1 + 1
             : 0;
}

/* ------------------------------------------------------------------ */
/* Prism deck renderer: gradients, halos, live REC state and the        */
/* on-demand APPS deck. Sits on top of dfr-render.c drawing helpers.    */
/* ------------------------------------------------------------------ */
#define PRISM_INK 0xFF0B0E14u          /* dark text on accent fills */
#define PRISM_BORDER 0xFF232839u       /* neutral pill outline */
#define PRISM_BORDER_DIM 0xFF171B27u  /* empty workspace outline */
#define PRISM_SURFACE_TOP 0xFF151926u
#define PRISM_SURFACE_BOT 0xFF090B11u
#define PRISM_MUTED 0xFF565F89u        /* empty workspace numerals */
#define PRISM_GLOSS 10u                /* top-edge gloss alpha */
#define PRISM_HILITE 0xFFFFFFFFu

static const struct { const char *deck; uint32_t color; const char *glyph; }
prism_deck_themes[] = {
    {"browser", DFR_COL_SKIP, DFR_GLYPH_WEB},
    {"terminal", DFR_COL_PLAY, DFR_GLYPH_TERM},
    {"code", DFR_COL_CODE2, DFR_GLYPH_CODE},
    {"nvim", DFR_COL_SHOT, DFR_GLYPH_CODE},
    {"files", DFR_COL_BRIGHT, DFR_GLYPH_FOLDER},
    {"comms", DFR_COL_VOICE, DFR_GLYPH_CHAT},
    {"document", DFR_COL_STATUS, DFR_GLYPH_DOC},
    {"reader", 0xFFA9B1D6u, DFR_GLYPH_READ},
    {"music", DFR_COL_VOICE, DFR_GLYPH_MUSIC},
    {"meta", DFR_COL_CHATGPT, DFR_GLYPH_ROBOT},
    {"universal", DFR_COL_BRIGHT, DFR_GLYPH_VIEWGRID},
    {"capture", DFR_COL_SHOT, DFR_GLYPH_CAMERA},
};

static char prism_hint_path_buf[PATH_MAX];
static int prism_hint_path_ready;
static int prism_preview_fake;
static char g_appdeck_deck[32];
static char g_appdeck_label[48];
static uint32_t g_appdeck_color = DFR_COL_ACTION;
static int g_recording;
static char g_rec_label[48];

static const char *prism_hint_path(void) {
  if (!prism_hint_path_ready) {
    prism_hint_path_ready = 1;
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    if (runtime && runtime[0] == '/' && geteuid() != 0)
      snprintf(prism_hint_path_buf, sizeof prism_hint_path_buf,
               "%s/t1-touchbar-app-deck", runtime);
    else
      snprintf(prism_hint_path_buf, sizeof prism_hint_path_buf,
               "/run/user/%d/t1-touchbar-app-deck", geteuid());
  }
  return prism_hint_path_buf;
}

/* The APPS key tint and icon preview the deck currently on offer. */
static void prism_appdeck_compose(const char *deck) {
  g_appdeck_color = DFR_COL_ACTION;
  const char *glyph = DFR_GLYPH_GRID;
  for (size_t i = 0; i < sizeof prism_deck_themes / sizeof prism_deck_themes[0];
       i++)
    if (!strcmp(deck, prism_deck_themes[i].deck)) {
      g_appdeck_color = prism_deck_themes[i].color;
      glyph = prism_deck_themes[i].glyph;
    }
  snprintf(g_appdeck_label, sizeof g_appdeck_label, "%s APPS", glyph);
}

static uint32_t prism_mix(uint32_t from, uint32_t to, int t) {
  if (t < 0) t = 0;
  if (t > 255) t = 255;
  unsigned r = (((from >> 16) & 0xff) * (255 - t) + ((to >> 16) & 0xff) * t) / 255;
  unsigned g = (((from >> 8) & 0xff) * (255 - t) + ((to >> 8) & 0xff) * t) / 255;
  unsigned b = (((from)&0xff) * (255 - t) + ((to)&0xff) * t) / 255;
  return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static int prism_isqrt(int v) {
  int r = 1;
  while (r * r <= v)
    r++;
  return r - 1;
}

/* Rounded vertical gradient: integer lerp per row, corner test like
 * fill_round_phys but with a per-row color. */
static void fill_gradient_round(int px, int py, int w, int h, int radius,
                                uint32_t top, uint32_t bottom) {
  if (w <= 0 || h <= 0)
    return;
  if (radius < 0)
    radius = 0;
  if (radius > w / 2)
    radius = w / 2;
  if (radius > h / 2)
    radius = h / 2;
  for (int y = 0; y < h; y++) {
    uint32_t c = prism_mix(top, bottom, h > 1 ? y * 255 / (h - 1) : 0);
    int x0 = 0, x1 = w;
    if (y < radius) {
      int dy = radius - 1 - y;
      int dx = prism_isqrt(radius * radius - dy * dy);
      x0 = radius - 1 - dx;
      x1 = w - (radius - 1 - dx);
    } else if (y >= h - radius && radius > 0) {
      int dy = y - (h - radius);
      int dx = prism_isqrt(radius * radius - dy * dy);
      x0 = radius - 1 - dx;
      x1 = w - (radius - 1 - dx);
    }
    if (x1 <= x0)
      continue;
    for (int x = x0; x < x1; x++)
      put_phys(px + x, py + y, c);
  }
}

/* Uniform rounded fill alpha-blended over the backdrop: halo rings. */
static void fill_round_blend(int px, int py, int w, int h, int radius,
                             uint32_t c, uint8_t alpha) {
  if (w <= 0 || h <= 0 || !alpha)
    return;
  if (radius < 0)
    radius = 0;
  if (radius > w / 2)
    radius = w / 2;
  if (radius > h / 2)
    radius = h / 2;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      if ((x >= radius && x < w - radius) || (y >= radius && y < h - radius)) {
        blend_phys(px + x, py + y, c, alpha);
        continue;
      }
      int cx = x < radius ? radius - 1 : w - radius;
      int cy = y < radius ? radius - 1 : h - radius;
      int dx = x - cx, dy = y - cy;
      if (dx * dx + dy * dy <= radius * radius)
        blend_phys(px + x, py + y, c, alpha);
    }
  }
}

/* Decode one UTF-8 codepoint; byte length via len when non-NULL. */
static uint32_t prism_first_cp(const char *s, int *len) {
  const unsigned char *p = (const unsigned char *)s;
  if (!p[0])
    return 0;
  int n = 1;
  uint32_t cp = p[0];
  if (p[0] >= 0xF0) {
    cp = ((uint32_t)(p[0] & 0x07) << 18) | ((uint32_t)(p[1] & 0x3F) << 12) |
         ((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
    n = 4;
  } else if (p[0] >= 0xE0) {
    cp = ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) |
         (p[2] & 0x3F);
    n = 3;
  } else if (p[0] >= 0xC0) {
    cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    n = 2;
  }
  if (len)
    *len = n;
  return cp;
}

/* Visual grouping: workspace pills / deck badges + home / everything else. */
static int prism_key_class(const struct dfr_key *k, const struct dfr_layout *l) {
  if (k->indicator >= DFR_IND_WORKSPACE_1 && k->indicator <= DFR_IND_WORKSPACE_10)
    return 0;
  if (k->action == DFR_ACT_LAYER)
    return 1;
  (void)l;
  return 2;
}

static void prism_halo(int bx, int by, int bw, int bh, int radius,
                       uint32_t color) {
  fill_round_blend(bx - 2, by - 2, bw + 4, bh + 4, radius > 2 ? radius + 2 : 0,
                   color, 30);
  fill_round_blend(bx - 1, by - 1, bw + 2, bh + 2, radius > 1 ? radius + 1 : 0,
                   color, 60);
}

/* Draw fitted (and optionally letter-spaced) centered text. px <= 0 selects
 * the biggest size that fits the pill; px < 0 caps that fit at |px|. */
static void prism_text(int bx, int bw, const char *label, int px, uint32_t col) {
  int asc, box, tw;
  int cap = px < 0 ? -px : 0;
  if (px <= 0) {
    px = cap ? (cap < FONT_PX_MAX ? cap : FONT_PX_MAX) : FONT_PX_MAX;
    for (;;) {
      tw = text_width_ft(label, px);
      text_vmetrics(px, &asc, &box);
      if ((tw <= bw - 12 && box <= g_short - 16) || px <= FONT_PX_MIN)
        break;
      px -= 2;
    }
  } else {
    tw = text_width_ft(label, px);
    text_vmetrics(px, &asc, &box);
  }
  draw_text_ft(bx + (bw - tw) / 2, (g_short - box) / 2 + asc, label, px, col);
}

/* Icon-over-label key: a dominant glyph on top and a small word underneath.
 * hero keys use a larger icon; both shrink to fit the pill width. */
static void prism_stacked(int bx, int bw, int by, int bh, const char *label,
                          int hero, uint32_t gcol, uint32_t wcol) {
  int icon_px = hero ? 30 : 26, word_px = hero ? 12 : 11;
  int len = 0;
  char glyph[8];
  prism_first_cp(label, &len);
  snprintf(glyph, sizeof glyph, "%.*s", len, label);
  const char *word = label + len;
  while (*word == ' ')
    word++;
  int gw, ww, ga, gb, wa, wb;
  for (;;) {
    gw = text_width_ft(glyph, icon_px);
    ww = word[0] ? text_width_ft(word, word_px) : 0;
    text_vmetrics(icon_px, &ga, &gb);
    text_vmetrics(word_px, &wa, &wb);
    if ((gw <= bw - 10 && ww <= bw - 10) || icon_px <= 16)
      break;
    icon_px -= 2;
    if (word_px > 10)
      word_px -= 1;
  }
  draw_text_ft(bx + (bw - gw) / 2, by + 3 + ga, glyph, icon_px, gcol);
  if (word[0])
    draw_text_ft(bx + (bw - ww) / 2, by + bh - 5, word, word_px, wcol);
}

/* The APPS key exposes the focused application's deck on demand; tapping it
 * again returns Home. The tint tracks the deck that is currently on offer. */
static void prism_open_appdeck(void) {
  char deck[32] = "";
  read_file_str(prism_hint_path(), deck, sizeof deck);
  int l = dfr_layout_index(deck);
  if (l < 0)
    l = dfr_layout_index("universal");
  desired = desired == l ? dfr_layout_index("media") : l;
  dirty = 1;
}

static void prism_appdeck_refresh(int *changed) {
  if (prism_preview_fake)
    return;
  char deck[32] = "";
  read_file_str(prism_hint_path(), deck, sizeof deck);
  if (dfr_layout_index(deck) < 0)
    deck[0] = 0;
  uint32_t color_before = g_appdeck_color;
  prism_appdeck_compose(deck);
  if (strcmp(deck, g_appdeck_deck) || color_before != g_appdeck_color) {
    snprintf(g_appdeck_deck, sizeof g_appdeck_deck, "%s", deck);
    *changed = 1;
  }
}

/* Live recording state: gpu-screen-recorder's presence flips the REC key into
 * a stop button with an elapsed timer from the recorder's start file mtime. */
static void prism_recording_refresh(int *changed) {
  if (prism_preview_fake)
    return;
  char count_buf[32];
  int count = 0;
  if (run_cmd_line("pgrep -c -f '^gpu-screen-recorder' 2>/dev/null", count_buf,
                   sizeof count_buf) == 0)
    count = atoi(count_buf);
  int recording = count > 0;
  long elapsed = -1;
  if (recording) {
    struct stat st;
    if (stat("/tmp/omarchy-screenrecord-filename", &st) == 0) {
      struct timespec now;
      clock_gettime(CLOCK_REALTIME, &now);
      long started = (long)st.st_mtim.tv_sec * 1000 + st.st_mtim.tv_nsec / 1000000;
      long now_ms_real = (long)now.tv_sec * 1000 + now.tv_nsec / 1000000;
      elapsed = (now_ms_real - started) / 1000;
      if (elapsed < 0)
        elapsed = 0;
      if (elapsed > 99 * 60 + 59)
        elapsed = 99 * 60 + 59;
    }
  }
  char label[48];
  if (recording && elapsed >= 0)
    snprintf(label, sizeof label, DFR_GLYPH_STOP " %ld:%02ld", elapsed / 60,
             elapsed % 60);
  else if (recording)
    snprintf(label, sizeof label, DFR_GLYPH_STOP " REC");
  else
    snprintf(label, sizeof label, DFR_GLYPH_RECORD " REC");
  if (recording != g_recording || strcmp(label, g_rec_label)) {
    g_recording = recording;
    snprintf(g_rec_label, sizeof g_rec_label, "%s", label);
    *changed = 1;
  }
}

static int is_rec_key(const struct dfr_key *k) {
  return k->action == DFR_ACT_CMD && k->cmd &&
         strstr(k->cmd, " record-toggle") != NULL;
}

static void draw_layout(const struct dfr_layout *l) {
  fill_phys(0, 0, g_long, g_short, COL_BG);
  int prev_class = -1;
  for (int i = 0; i < l->nkeys; i++) {
    int x0, x1;
    dfr_key_extent(l, i, g_long, &x0, &x1);
    const struct dfr_key *k = &l->keys[i];
    int workspace = k->indicator >= DFR_IND_WORKSPACE_1 &&
                    k->indicator <= DFR_IND_WORKSPACE_10
                        ? k->indicator - DFR_IND_WORKSPACE_1 + 1
                        : 0;
    int occupied = workspace && (g_workspace_mask & (1u << (workspace - 1)));
    int active = workspace && workspace == g_active_workspace;
    int badge = k->action == DFR_ACT_LAYER && k->cmd && !strcmp(k->cmd, l->name);
    int home = k->action == DFR_ACT_LAYER && k->cmd &&
               !strcmp(k->cmd, "media") && !badge;
    int rec = is_rec_key(k);
    int appdeck = k->action == DFR_ACT_APPDECK;

    int cls = prism_key_class(k, l);
    int gap = i > 0 && cls != prev_class ? 5 : 0;
    prev_class = cls;
    int margin = workspace ? 2 : 3;
    int bx = x0 + margin + (i == 0 ? 1 : 0) + gap;
    int bw = (x1 - x0) - 2 * margin - gap - (i == 0 ? 1 : 0);
    int by = workspace ? 5 : 4;
    int bh = g_short - 2 * by;
    int radius = workspace ? 10 : badge ? 14 : 13;
    uint32_t accent = k->color ? k->color : COL_ACCENT;

    int glyph_only = 0, stacked = 0;
    if (!workspace) {
      int len = 0;
      uint32_t cp = prism_first_cp(k->label, &len);
      glyph_only = cp >= 0xE000 && k->label[len] == 0;
      stacked = !glyph_only && cp >= 0xE000 && k->label[len] == ' ' &&
                k->label[len + 1];
    }

    if (workspace) {
      if (active) {
        prism_halo(bx, by, bw, bh, radius, DFR_COL_WORKSPACE);
        fill_round_phys(bx, by, bw, bh, radius,
                        prism_mix(DFR_COL_WORKSPACE, PRISM_HILITE, 60));
        fill_gradient_round(bx + 1, by + 1, bw - 2, bh - 2, radius - 1,
                            prism_mix(DFR_COL_WORKSPACE, PRISM_HILITE, 95),
                            DFR_COL_WORKSPACE);
        prism_text(bx, bw, k->label, 17, PRISM_INK);
      } else {
        fill_round_phys(bx, by, bw, bh, radius,
                        occupied ? prism_mix(DFR_COL_WORKSPACE, COL_BG, 55)
                                 : PRISM_BORDER_DIM);
        fill_gradient_round(bx + 1, by + 1, bw - 2, bh - 2, radius - 1,
                            occupied ? PRISM_SURFACE_TOP
                                     : prism_mix(PRISM_SURFACE_TOP, COL_BG, 45),
                            occupied ? PRISM_SURFACE_BOT
                                     : prism_mix(PRISM_SURFACE_BOT, COL_BG, 45));
        prism_text(bx, bw, k->label, 17, occupied ? COL_FG : PRISM_MUTED);
        if (occupied)
          fill_round_phys(bx + bw / 2 - 2, by + bh - 9, 5, 5, 2,
                          DFR_COL_WORKSPACE);
      }
      continue;
    }

    if (badge) {
      prism_halo(bx, by, bw, bh, radius, accent);
      fill_round_phys(bx, by, bw, bh, radius, prism_mix(accent, COL_BG, 25));
      fill_gradient_round(bx + 1, by + 1, bw - 2, bh - 2, radius - 1,
                          prism_mix(accent, PRISM_HILITE, 80), accent);
      prism_text(bx, bw, k->label, -23, PRISM_INK);
      continue;
    }

    if (rec) {
      if (g_recording) {
        prism_halo(bx, by, bw, bh, radius, DFR_COL_REC);
        fill_round_phys(bx, by, bw, bh, radius, prism_mix(DFR_COL_REC, COL_BG, 20));
        fill_gradient_round(bx + 1, by + 1, bw - 2, bh - 2, radius - 1,
                            prism_mix(DFR_COL_REC, COL_BG, 62),
                            prism_mix(DFR_COL_REC, COL_BG, 80));
        prism_stacked(bx, bw, by, bh, g_rec_label, 1, DFR_COL_REC, DFR_COL_REC);
      } else {
        fill_round_phys(bx, by, bw, bh, radius,
                        prism_mix(DFR_COL_REC, COL_BG, 55));
        fill_gradient_round(bx + 1, by + 1, bw - 2, bh - 2, radius - 1,
                            PRISM_SURFACE_TOP, PRISM_SURFACE_BOT);
        prism_stacked(bx, bw, by, bh, k->label, 1, DFR_COL_REC, DFR_COL_REC);
      }
      continue;
    }

    uint32_t border = PRISM_BORDER;
    uint32_t tcol = COL_FG;
    if (appdeck) {
      border = prism_mix(g_appdeck_color, COL_BG, 20);
    } else if (k->indicator > DFR_IND_NONE &&
               k->indicator <= DFR_IND_KBDLIGHT && g_ind_color[k->indicator]) {
      border = prism_mix(g_ind_color[k->indicator], COL_BG, 55);
      tcol = g_ind_color[k->indicator];
    } else if (home) {
      border = prism_mix(DFR_COL_OMARCHY, COL_BG, 45);
      tcol = DFR_COL_OMARCHY;
    } else if (k->color) {
      border = prism_mix(k->color, COL_BG, 55);
      tcol = k->color;
    }
    if (!strcmp(k->label, "ESC"))
      tcol = DFR_COL_ESC;

    fill_round_phys(bx, by, bw, bh, radius, border);
    fill_gradient_round(bx + 1, by + 1, bw - 2, bh - 2, radius - 1,
                        PRISM_SURFACE_TOP, PRISM_SURFACE_BOT);
    if (appdeck)
      fill_round_blend(bx + 1, by + 1, bw - 2, bh - 2, radius - 1,
                       g_appdeck_color, 26);
    /* one quiet gloss row plus a bottom rail carrying the action identity */
    fill_round_blend(bx + radius - 2, by + 3, bw - 2 * (radius - 2), 1, 0,
                     PRISM_HILITE, PRISM_GLOSS);
    if (!home && !appdeck && k->color && !glyph_only && !stacked)
      fill_round_phys(bx + 12, by + bh - 5, bw - 24, 2, 1, k->color);
    if (glyph_only)
      prism_text(bx, bw, k->label, 28, tcol);
    else if (appdeck)
      prism_stacked(bx, bw, by, bh, g_appdeck_label, 1, g_appdeck_color, tcol);
    else if (stacked)
      prism_stacked(bx, bw, by, bh, k->label, 0, tcol, tcol);
    else
      prism_text(bx, bw, k->label, 0, tcol);
  }
}

/* Refresh every live label; returns 1 if anything visible changed. */
static int ind_refresh(void) {
  int changed = legacy_ind_refresh();
  prism_appdeck_refresh(&changed);
  prism_recording_refresh(&changed);
  return changed;
}

static void key_action(const struct dfr_key *key) {
  int k = key->keycode;
  if (!key->mods && (k == KEY_ESC || (k >= KEY_F1 && k <= KEY_F10) ||
                     k == KEY_F11 || k == KEY_F12)) {
    unsigned char b[12] = {1};
    wr16(b + 4, (uint16_t)k);
    if (request(4, b, sizeof b) < 0)
      g_stop = 1;
    return;
  }
  if (!key->mods) {
    switch (k) {
    case KEY_PREVIOUSSONG:
      launch("playerctl previous");
      return;
    case KEY_PLAYPAUSE:
      launch("playerctl play-pause");
      return;
    case KEY_NEXTSONG:
      launch("playerctl next");
      return;
    case KEY_MUTE:
      launch("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle");
      return;
    case KEY_VOLUMEDOWN:
      launch("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-");
      return;
    case KEY_VOLUMEUP:
      launch("wpctl set-volume -l 1.0 @DEFAULT_AUDIO_SINK@ 5%+");
      return;
    case KEY_BRIGHTNESSDOWN:
    case KEY_BRIGHTNESSUP: {
      unsigned char b = k == KEY_BRIGHTNESSUP;
      if (request(8, &b, 1) < 0)
        fprintf(stderr, "Display brightness unavailable\n");
      return;
    }
    }
  }
  const xkb_keysym_t *syms = NULL;
  char name[128], mods[64] = "", command[1024];
  if (xkb_keymap_key_get_syms_by_level(keymap, (xkb_keycode_t)(k + 8), 0, 0,
                                       &syms) != 1 ||
      xkb_keysym_get_name(syms[0], name, sizeof name) <= 0)
    return;
  for (const char *s = name; *s; s++)
    if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') ||
          (*s >= '0' && *s <= '9') || *s == '_'))
      return;
  if (key->mods & DFR_MOD_CTRL)
    strcat(mods, "CTRL ");
  if (key->mods & DFR_MOD_ALT)
    strcat(mods, "ALT ");
  if (key->mods & DFR_MOD_SHIFT)
    strcat(mods, "SHIFT ");
  if (key->mods & DFR_MOD_META)
    strcat(mods, "SUPER ");
  snprintf(command, sizeof command,
           "hyprctl dispatch 'hl.dsp.send_key_state({mods=\"%s\", key=\"%s\", "
           "state=\"down\"})'; hyprctl dispatch "
           "'hl.dsp.send_key_state({mods=\"%s\", key=\"%s\", state=\"up\"})'",
           mods, name, mods, name);
  launch(command);
}
/* Offline preview of the Prism deck renderer. DFR_PREVIEW_HINT=NAME fakes the
 * APPS deck hint and DFR_PREVIEW_RECORDING=1 fakes an active recording so the
 * dynamic key states can be eyeballed without touching the hardware. */
static int do_preview(const struct dfr_layout *l, const char *path) {
  const char *fake_deck = getenv("DFR_PREVIEW_HINT");
  if (fake_deck && dfr_layout_index(fake_deck) >= 0) {
    snprintf(g_appdeck_deck, sizeof g_appdeck_deck, "%s", fake_deck);
    prism_appdeck_compose(fake_deck);
  }
  if (getenv("DFR_PREVIEW_RECORDING")) {
    snprintf(g_rec_label, sizeof g_rec_label, DFR_GLYPH_STOP " 12:34");
    g_recording = 1;
  }
  prism_preview_fake = fake_deck != NULL || getenv("DFR_PREVIEW_RECORDING") != NULL;
  g_short = 60;
  g_long = 2170;
  g_pitch32 = g_short;
  g_fb = calloc((size_t)g_short * g_long, 4);
  if (!g_fb)
    return 1;
  if (dfr_layout_has_indicators(l))
    ind_refresh();
  draw_layout(l);
  FILE *f = fopen(path, "w");
  if (!f) {
    fprintf(stderr, "fopen %s: %s\n", path, strerror(errno));
    return 1;
  }
  fprintf(f, "P6\n%d %d\n255\n", g_long, g_short);
  for (int py = 0; py < g_short; py++)
    for (int px = 0; px < g_long; px++) {
      int fx = g_flip_short ? (g_short - 1 - py) : py;
      int fy = g_flip_long ? (g_long - 1 - px) : px;
      uint32_t c = g_fb[(size_t)fy * g_pitch32 + fx];
      fputc((c >> 16) & 0xff, f);
      fputc((c >> 8) & 0xff, f);
      fputc(c & 0xff, f);
    }
  fclose(f);
  printf("preview (%s, layout '%s') -> %s (2170x60 PPM)\n",
         (g_flip_long || g_flip_short) ? "flipped" : "default orientation",
         l->name, path);
  free(g_fb);
  return 0;
}
static void activate(int layout, int zone) {
  if (zone < 0 || zone >= dfr_layouts[layout].nkeys)
    return;
  const struct dfr_key *k = &dfr_layouts[layout].keys[zone];
  switch (k->action) {
  case DFR_ACT_LAYER: {
    int l = dfr_layout_index(k->cmd);
    if (l >= 0) {
      desired = l;
      dirty = 1;
    }
    break;
  }
  case DFR_ACT_APPDECK:
    prism_open_appdeck();
    break;
  case DFR_ACT_CMD:
    layout_command(k->cmd);
    break;
  default:
    key_action(k);
    break;
  }
}
static void handle_input(void) {
  if (!input_changed)
    return;
  input_changed = 0;
  int64_t t = now_ms();
  if (input_fn && !fn_last && t - last_fn >= 90) {
    if (desired == 1)
      desired = previous;
    else {
      previous = desired;
      desired = 1;
    }
    dirty = 1;
    last_fn = t;
    suppressed = contact_present > 0;
    held = -1;
  }
  fn_last = input_fn;
  if (contact_present > 1) {
    suppressed = 1;
    held = -1;
    return;
  }
  if (suppressed) {
    if (!contact_present)
      suppressed = 0;
    return;
  }
  if (contact_present) {
    if (held < 0) {
      if (dirty || visible < 0) {
        suppressed = 1;
        return;
      }
      if (overlay) {
        held = contact_x >= width - 220 ? 999 : 998;
        held_layout = visible;
        return;
      }
      held = dfr_zone_from_nx(&dfr_layouts[visible],
                              (float)contact_x / (width - 1));
      held_layout = visible;
      held_workspace = workspace_at(held_layout, held);
      drag_workspace = held_workspace;
    } else if (held_workspace) {
      int z = dfr_zone_from_nx(&dfr_layouts[held_layout],
                               (float)contact_x / (width - 1));
      drag_workspace = workspace_at(held_layout, z);
    }
  } else if (held >= 0) {
    if (t - last_action >= 160) {
      last_action = t;
      if (held == 999) {
        if (request(7, NULL, 0) < 0)
          fprintf(stderr, "Touch ID cancellation unavailable\n");
      } else if (held < 998 && !overlay) {
        if (held_workspace && drag_workspace &&
            held_workspace != drag_workspace) {
          char cmd[160];
          snprintf(cmd, sizeof cmd,
                   "\"$OMARCHY_TOUCHBAR_ACTION\" workspace-merge %d %d",
                   held_workspace, drag_workspace);
          launch(cmd);
        } else
          activate(held_layout, held);
      }
    }
    held = -1;
    held_workspace = 0;
    drag_workspace = 0;
  }
}
static void update_overlay(void) {
  int state = 0, progress = -1;
  struct json_object *j =
      json_object_from_file("/run/t1bridge/touch-id-state.json");
  if (j) {
    struct json_object *p, *s, *v;
    if (json_object_object_get_ex(j, "pid", &p) &&
        json_object_object_get_ex(j, "state", &s) &&
        json_object_object_get_ex(j, "version", &v) &&
        json_object_get_type(p) == json_type_int &&
        json_object_get_type(s) == json_type_string &&
        (json_object_get_int(v) == 1 || json_object_get_int(v) == 2)) {
      int pid = json_object_get_int(p);
      const char *name = json_object_get_string(s);
      if (pid > 0 && (kill(pid, 0) == 0 || errno == EPERM)) {
        const char *states[] = {"enrollment", "authenticate", "approve",
                                "retry", "success"};
        for (int i = 0; i < 5; i++)
          if (!strcmp(name, states[i]))
            state = i + 1;
        if (state == 1 && json_object_object_get_ex(j, "progress", &p) &&
            json_object_get_type(p) == json_type_int) {
          int n = json_object_get_int(p);
          if (n >= 0 && n <= 100)
            progress = n;
        }
      }
    }
    json_object_put(j);
  }
  if (state != overlay || progress != overlay_progress) {
    overlay = state;
    overlay_progress = progress;
    dirty = 1;
    held = -1;
    suppressed = contact_present > 0;
  }
}
static int submit_frame(void) {
  draw_layout(&dfr_layouts[desired]);
  if (overlay) {
    const char *labels[] = {"",
                            "Touch ID: lift and touch your finger",
                            "Touch ID: touch the power button",
                            "Touch ID: approve",
                            "Touch ID: try again",
                            "Touch ID: matched"};
    fill_phys(0, 0, g_long, g_short, COL_BG);
    draw_text_ft(24, 39, labels[overlay], 25, COL_FG);
    draw_text_ft(g_long - 185, 39, "CANCEL", 23, COL_ACCENT);
    if (overlay_progress >= 0)
      fill_phys(0, g_short - 3, g_long * overlay_progress / 100, 3, COL_ACCENT);
  }
  for (unsigned y = 0; y < height; y++)
    for (unsigned x = 0; x < width; x++)
      shared[(size_t)y * width + x] = get_phys((int)x, (int)y);
  unsigned char b[16] = {0};
  wr32(b, 1);
  wr64(b + 4, ++frame_id);
  if (request(3, b, sizeof b) < 0)
    return -1;
  int64_t end = now_ms() + 3000;
  while (released != frame_id && now_ms() < end && !g_stop) {
    uint16_t t;
    uint32_t id;
    size_t len;
    if (receive_packet(100, &t, &id, &len) < 0)
      return -1;
  }
  if (released != frame_id)
    return -1;
  visible = desired;
  dirty = 0;
  last_frame = now_ms();
  /* A contact received during submission cannot activate an unconfirmed hit
   * map. */
  if (contact_present && held < 0)
    suppressed = 1;
  return 0;
}
static int connect_hardware(void) {
  ipc = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
  if (ipc < 0)
    return -1;
  struct sockaddr_un a = {.sun_family = AF_UNIX};
  strcpy(a.sun_path, "/run/t1bridge/touchbar.sock");
  if (connect(ipc, (struct sockaddr *)&a, sizeof a) < 0)
    return -1;
  struct ucred peer;
  socklen_t plen = sizeof peer;
  if (getsockopt(ipc, SOL_SOCKET, SO_PEERCRED, &peer, &plen) < 0 ||
      peer.uid != 0)
    return -1;
  unsigned char hello[12] = {0};
  wr16(hello, 1);
  wr64(hello + 4, 0x27);
  if (wait_reply(send_packet(1, hello, sizeof hello, -1), 0x8001) < 0 ||
      rd32(packet + 8) != 20)
    return -1;
  width = rd32(packet + 20);
  height = rd32(packet + 24);
  if (width != 2170 || height != 60 || rd32(packet + 28) != 1)
    return -1;
  shared_fd =
      memfd_create("omarchy-touchbar-frame", MFD_CLOEXEC | MFD_ALLOW_SEALING);
  size_t bytes = (size_t)width * height * 4;
  if (shared_fd < 0 || ftruncate(shared_fd, (off_t)bytes) < 0 ||
      fcntl(shared_fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL) <
          0)
    return -1;
  shared = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, shared_fd, 0);
  if (shared == MAP_FAILED)
    return -1;
  unsigned char b[16];
  wr32(b, 1);
  wr32(b + 4, width * 4);
  wr64(b + 8, bytes);
  return wait_reply(send_packet(2, b, sizeof b, shared_fd), 0x8002);
}
int main(int argc, char **argv) {
  if (geteuid() == 0) {
    fprintf(stderr, "Run this renderer as the graphical user.\n");
    return 1;
  }
  if (argc > 1) {
    if (argc != 5 || strcmp(argv[1], "-l") || strcmp(argv[3], "--preview")) {
      fprintf(stderr, "Usage: %s [-l LAYOUT --preview FILE.ppm]\n", argv[0]);
      return 2;
    }
    int l = dfr_layout_index(argv[2]);
    if (l < 0 || ft_init() < 0)
      return 2;
    return do_preview(&dfr_layouts[l], argv[4]);
  }
  ssize_t n = readlink("/proc/self/exe", executable, sizeof executable - 1);
  if (n < 0)
    return 1;
  executable[n] = 0;
  char directory[PATH_MAX];
  snprintf(directory, sizeof directory, "%s", executable);
  char *slash = strrchr(directory, '/');
  if (!slash)
    return 1;
  *slash = 0;
  if (snprintf(action_path, sizeof action_path, "%s/touchbar-action",
               directory) >= (int)sizeof action_path ||
      snprintf(context_path, sizeof context_path, "%s/dfr-contextd",
               directory) >= (int)sizeof context_path)
    return 1;
  setenv("OMARCHY_TOUCHBAR_ACTION", action_path, 1);
  setenv("OMARCHY_TOUCHBAR_RENDERER", executable, 1);
  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, on_stop);
  signal(SIGTERM, on_stop);
  signal(SIGUSR1, on_usr1);
  signal(SIGUSR2, on_usr2);
  sigset_t signals;
  sigemptyset(&signals);
  sigaddset(&signals, SIGUSR1);
  for (int i = 0; i < DFR_NLAYOUTS; i++) {
    signal(SIGRTMIN + i, on_rt);
    sigaddset(&signals, SIGRTMIN + i);
  }
  if (ft_init() < 0 || connect_hardware() < 0) {
    fprintf(stderr, "Custom renderer could not connect to T1Bridge.\n");
    return 1;
  }
  g_short = (int)height;
  g_long = (int)width;
  g_pitch32 = g_short;
  g_fb = calloc((size_t)width * height, 4);
  if (!g_fb)
    return 1;
  xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!xkb)
    return 1;
  struct xkb_rule_names names = {.layout = "us"};
  keymap = xkb_keymap_new_from_names(xkb, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!keymap)
    return 1;
  ind_refresh();
  if (submit_frame() < 0)
    return 1;
  context_pid = fork();
  if (context_pid == 0) {
    execl(context_path, context_path, (char *)NULL);
    _exit(127);
  }
  fprintf(stderr, "Omarchy custom Touch Bar connected (%ux%u).\n", width,
          height);
  int failed = 0;
  while (!g_stop) {
    int requested = -1;
    if (take_layout_request(&signals, desired, &requested) < 0) {
      failed = 1;
      break;
    }
    if (requested >= 0 && requested < DFR_NLAYOUTS && requested != desired) {
      desired = requested;
      dirty = 1;
    }
    int64_t t = now_ms();
    if (t - last_refresh >= 750) {
      if (ind_refresh())
        dirty = 1;
      update_overlay();
      last_refresh = t;
    }
    handle_input();
    if (dirty && held < 0 && !contact_present && t - last_frame >= 120) {
      if (submit_frame() < 0) {
        failed = 1;
        break;
      }
    }
    uint16_t type;
    uint32_t id;
    size_t len;
    if (receive_packet(50, &type, &id, &len) < 0) {
      failed = 1;
      break;
    }
  }
  if (context_pid > 0)
    kill(context_pid, SIGTERM);
  close(ipc);
  if (shared && shared != MAP_FAILED)
    munmap(shared, (size_t)width * height * 4);
  if (shared_fd >= 0)
    close(shared_fd);
  free(g_fb);
  xkb_keymap_unref(keymap);
  xkb_context_unref(xkb);
  return failed;
}
