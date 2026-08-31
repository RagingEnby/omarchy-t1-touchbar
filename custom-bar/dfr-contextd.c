/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatic application context for the Omarchy T1 Touch Bar.
 *
 * The daemon listens to Hyprland's event socket, maps the focused application
 * to one of the finite layouts in dfr-layout.h, and asks dfr-render to paint
 * that layout with a real-time signal. Window metadata is used only in memory:
 * titles are never logged, launched, or interpolated into a command.
 */
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

/* Older hand-written layouts intentionally omit the new trailing mods field. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "dfr-layout.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define DFR_RENDER_EXE "/run/t1-custom-bar/dfr-render"
#define EVENT_BUFFER_SIZE 65536u
#define JSON_BUFFER_SIZE 65536u
#define MAX_SOCKET_CANDIDATES 64u

struct hypr_sockets {
	char command[PATH_MAX];
	char events[PATH_MAX];
	struct timespec modified;
};

static volatile sig_atomic_t g_stop;

static void on_stop(int signal_number)
{
	(void)signal_number;
	g_stop = 1;
}

static int contains_ci(const char *text, const char *needle)
{
	if (!text || !needle || !*needle)
		return 0;

	for (const unsigned char *start = (const unsigned char *)text;
	     *start; start++) {
		const unsigned char *a = start;
		const unsigned char *b = (const unsigned char *)needle;
		while (*a && *b && tolower(*a) == tolower(*b)) {
			a++;
			b++;
		}
		if (!*b)
			return 1;
	}
	return 0;
}

static int equals_ci(const char *left, const char *right)
{
	if (!left || !right)
		return 0;
	while (*left && *right &&
	       tolower((unsigned char)*left) == tolower((unsigned char)*right)) {
		left++;
		right++;
	}
	return !*left && !*right;
}

static int starts_with_ci(const char *text, const char *prefix)
{
	if (!text || !prefix)
		return 0;
	while (*prefix) {
		if (!*text || tolower((unsigned char)*text) !=
		    tolower((unsigned char)*prefix))
			return 0;
		text++;
		prefix++;
	}
	return 1;
}

static int equals_any(const char *text, const char *const *values, size_t count)
{
	for (size_t i = 0; i < count; i++)
		if (equals_ci(text, values[i]))
			return 1;
	return 0;
}

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

static const char *layout_for_window(const char *window_class,
				     const char *window_title)
{
	static const char *const ai_apps[] = {
		"chatgpt", "codex", "com.openai.chatgpt",
	};
	static const char *const terminals[] = {
		"foot", "kitty", "alacritty", "ghostty", "wezterm", "konsole",
		"org.omarchy.terminal", "org.codeberg.dnkl.foot",
		"com.mitchellh.ghostty",
	};
	static const char *const browsers[] = {
		"chromium", "chromium-browser", "google-chrome", "google-chrome-stable",
		"chrome", "firefox", "librewolf",
		"floorp", "brave", "brave-browser", "zen", "vivaldi",
		"vivaldi-stable", "helium", "opera", "opera-stable", "thorium",
		"microsoft-edge",
	};
	static const char *const vscode_editors[] = {
		"visual-studio-code", "vscode", "code", "code-oss", "cursor",
	};
	static const char *const nvim_editors[] = {
		"org.omarchy.nvim", "neovide", "nvim",
	};
	static const char *const file_apps[] = {
		"org.gnome.nautilus", "nautilus", "thunar", "nemo", "dolphin",
		"org.kde.dolphin", "pcmanfm",
	};
	static const char *const communication_apps[] = {
		"bluebubbles", "discord", "vesktop", "slack", "signal", "teams",
		"whatsapp", "telegram", "org.telegram.desktop", "element",
		"mattermost",
	};
	static const char *const communication_titles[] = {
		"bluebubbles", "discord", "slack", "microsoft teams", "whatsapp",
		"telegram", "mattermost", "google messages",
	};
	static const char *const document_apps[] = {
		"obsidian", "md.obsidian.obsidian", "typora",
		"xournal", "com.github.xournalpp.xournalpp", "omawrite",
	};
	static const char *const reader_apps[] = {
		"evince", "org.gnome.evince", "okular", "org.kde.okular",
		"calibre", "papers", "org.gnome.papers",
	};
	static const char *const media_apps[] = {
		"mpv", "vlc", "org.videolan.vlc", "spotify", "com.spotify.client",
		"celluloid", "io.github.celluloid_player.celluloid", "amberol",
		"io.bassi.amberol", "audacious", "rhythmbox", "kodi", "youtube",
	};
	const char *class_name = window_class ? window_class : "";
	const char *title = window_title ? window_title : "";
	int browser = equals_any(class_name, browsers, ARRAY_LEN(browsers)) ||
		starts_with_ci(class_name, "chromium-") ||
		starts_with_ci(class_name, "google-chrome-");

	if (equals_any(class_name, ai_apps, ARRAY_LEN(ai_apps)))
		return "meta";

	/* Title heuristics are allowed only inside a known browser/PWA class. A
	 * terminal named "YouTube uploader" must remain a terminal. */
	if (browser) {
		for (size_t i = 0; i < ARRAY_LEN(communication_titles); i++)
			if (contains_ci(title, communication_titles[i]))
				return "comms";
		if (contains_ci(title, "youtube"))
			return "music";
		return "browser";
	}

	if (equals_any(class_name, nvim_editors, ARRAY_LEN(nvim_editors)))
		return "nvim";
	if (equals_any(class_name, terminals, ARRAY_LEN(terminals)) ||
	    starts_with_ci(class_name, "org.omarchy.") ||
	    starts_with_ci(class_name, "tui."))
		return "terminal";
	if (equals_any(class_name, vscode_editors, ARRAY_LEN(vscode_editors)))
		return "code";
	if (equals_any(class_name, file_apps, ARRAY_LEN(file_apps)))
		return "files";
	if (equals_any(class_name, communication_apps,
		       ARRAY_LEN(communication_apps)))
		return "comms";
	if (equals_any(class_name, document_apps, ARRAY_LEN(document_apps)))
		return "document";
	if (equals_any(class_name, reader_apps, ARRAY_LEN(reader_apps)))
		return "reader";
	if (equals_any(class_name, media_apps, ARRAY_LEN(media_apps)))
		return "music";

	return "universal";
}

static int decimal_name(const char *name)
{
	if (!name || !*name)
		return 0;
	for (const unsigned char *p = (const unsigned char *)name; *p; p++)
		if (!isdigit(*p))
			return 0;
	return 1;
}

static int copy_path(char *destination, size_t destination_size,
		     const char *left, const char *right)
{
	int written = snprintf(destination, destination_size, "%s/%s", left, right);
	return written >= 0 && (size_t)written < destination_size ? 0 : -1;
}

static int candidate_order(const void *left, const void *right)
{
	const struct hypr_sockets *a = left;
	const struct hypr_sockets *b = right;
	if (a->modified.tv_sec != b->modified.tv_sec)
		return a->modified.tv_sec < b->modified.tv_sec ? 1 : -1;
	if (a->modified.tv_nsec != b->modified.tv_nsec)
		return a->modified.tv_nsec < b->modified.tv_nsec ? 1 : -1;
	return 0;
}

static size_t discover_hypr_sockets(struct hypr_sockets *candidates,
				    size_t capacity)
{
	DIR *runtime_users = opendir("/run/user");
	if (!runtime_users)
		return 0;

	size_t count = 0;
	struct dirent *user_entry;
	while ((user_entry = readdir(runtime_users)) != NULL && !g_stop) {
		if (!decimal_name(user_entry->d_name))
			continue;
		errno = 0;
		char *uid_end = NULL;
		unsigned long uid_value = strtoul(user_entry->d_name, &uid_end, 10);
		if (errno || !uid_end || *uid_end || uid_value < 1000 ||
		    uid_value > UINT32_MAX)
			continue;
		uid_t owner = (uid_t)uid_value;

		char user_root[PATH_MAX];
		char hypr_root[PATH_MAX];
		if (copy_path(user_root, sizeof user_root, "/run/user",
			      user_entry->d_name) < 0 ||
		    copy_path(hypr_root, sizeof hypr_root, user_root, "hypr") < 0)
			continue;
		struct stat user_stat;
		struct stat hypr_stat;
		if (lstat(user_root, &user_stat) < 0 ||
		    lstat(hypr_root, &hypr_stat) < 0 ||
		    !S_ISDIR(user_stat.st_mode) || !S_ISDIR(hypr_stat.st_mode) ||
		    user_stat.st_uid != owner || hypr_stat.st_uid != owner)
			continue;

		DIR *instances = opendir(hypr_root);
		if (!instances)
			continue;

		struct dirent *instance_entry;
		while ((instance_entry = readdir(instances)) != NULL && !g_stop) {
			if (!strcmp(instance_entry->d_name, ".") ||
			    !strcmp(instance_entry->d_name, ".."))
				continue;

			char instance_root[PATH_MAX];
			char command_path[PATH_MAX];
			char event_path[PATH_MAX];
			if (copy_path(instance_root, sizeof instance_root, hypr_root,
				      instance_entry->d_name) < 0 ||
			    copy_path(command_path, sizeof command_path, instance_root,
				      ".socket.sock") < 0 ||
			    copy_path(event_path, sizeof event_path, instance_root,
				      ".socket2.sock") < 0)
				continue;

			struct stat instance_stat;
			struct stat command_stat;
			struct stat event_stat;
			if (lstat(instance_root, &instance_stat) < 0 ||
			    lstat(command_path, &command_stat) < 0 ||
			    lstat(event_path, &event_stat) < 0 ||
			    !S_ISDIR(instance_stat.st_mode) ||
			    !S_ISSOCK(command_stat.st_mode) ||
			    !S_ISSOCK(event_stat.st_mode) ||
			    instance_stat.st_uid != owner ||
			    command_stat.st_uid != owner || event_stat.st_uid != owner)
				continue;

			if (count >= capacity)
				continue;
			memcpy(candidates[count].command, command_path,
			       strlen(command_path) + 1);
			memcpy(candidates[count].events, event_path,
			       strlen(event_path) + 1);
			candidates[count].modified = event_stat.st_mtim;
			count++;
		}
		closedir(instances);
	}
	closedir(runtime_users);

	qsort(candidates, count, sizeof candidates[0], candidate_order);
	return count;
}

static int connect_unix_socket(const char *path)
{
	struct sockaddr_un address;
	size_t path_length = strlen(path);
	if (path_length >= sizeof address.sun_path) {
		errno = ENAMETOOLONG;
		return -1;
	}

	int socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (socket_fd < 0)
		return -1;

	memset(&address, 0, sizeof address);
	address.sun_family = AF_UNIX;
	memcpy(address.sun_path, path, path_length + 1);
	if (connect(socket_fd, (struct sockaddr *)&address, sizeof address) < 0) {
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

static int send_all(int socket_fd, const char *message)
{
	size_t sent = 0;
	size_t length = strlen(message);
	while (sent < length) {
		ssize_t result = send(socket_fd, message + sent, length - sent,
				      MSG_NOSIGNAL);
		if (result < 0 && errno == EINTR)
			continue;
		if (result <= 0)
			return -1;
		sent += (size_t)result;
	}
	return 0;
}

static int json_string_field(const char *json, const char *field,
			     char *output, size_t output_size)
{
	char marker[64];
	int marker_length = snprintf(marker, sizeof marker, "\"%s\"", field);
	if (marker_length < 0 || (size_t)marker_length >= sizeof marker ||
	    output_size == 0)
		return -1;

	const char *cursor = json;
	while ((cursor = strstr(cursor, marker)) != NULL) {
		cursor += (size_t)marker_length;
		while (isspace((unsigned char)*cursor))
			cursor++;
		if (*cursor != ':')
			continue;
		cursor++;
		while (isspace((unsigned char)*cursor))
			cursor++;
		if (*cursor != '"')
			continue;
		cursor++;

		size_t used = 0;
		while (*cursor && *cursor != '"') {
			unsigned char value = (unsigned char)*cursor++;
			if (value == '\\' && *cursor) {
				value = (unsigned char)*cursor++;
				switch (value) {
				case 'b': value = '\b'; break;
				case 'f': value = '\f'; break;
				case 'n': value = '\n'; break;
				case 'r': value = '\r'; break;
				case 't': value = '\t'; break;
				case 'u':
					value = '?';
					for (int i = 0; i < 4 && isxdigit((unsigned char)*cursor);
					     i++)
						cursor++;
					break;
				default: break;
				}
			}
			if (used + 1 < output_size)
				output[used++] = (char)value;
		}
		output[used] = 0;
		return *cursor == '"' ? 0 : -1;
	}
	output[0] = 0;
	return -1;
}

static int query_active_window(const char *command_socket, char *window_class,
			       size_t class_size, char *window_title,
			       size_t title_size)
{
	int socket_fd = connect_unix_socket(command_socket);
	if (socket_fd < 0)
		return -1;
	if (send_all(socket_fd, "j/activewindow") < 0) {
		close(socket_fd);
		return -1;
	}
	(void)shutdown(socket_fd, SHUT_WR);

	char response[JSON_BUFFER_SIZE];
	size_t used = 0;
	while (used + 1 < sizeof response && !g_stop) {
		struct pollfd descriptor = { .fd = socket_fd, .events = POLLIN };
		int ready = poll(&descriptor, 1, 1000);
		if (ready < 0 && errno == EINTR)
			continue;
		if (ready <= 0)
			break;
		if (!(descriptor.revents & POLLIN))
			break;
		ssize_t received = read(socket_fd, response + used,
					sizeof response - used - 1);
		if (received < 0 && errno == EINTR)
			continue;
		if (received <= 0)
			break;
		used += (size_t)received;
	}
	close(socket_fd);
	response[used] = 0;
	window_class[0] = 0;
	window_title[0] = 0;
	if (!used)
		return -1;
	(void)json_string_field(response, "class", window_class, class_size);
	(void)json_string_field(response, "title", window_title, title_size);
	return 0;
}

static int renderer_catches_signal(pid_t process_id, int signal_number)
{
	if (signal_number < 1 || signal_number > 64)
		return 0;

	char status_path[64];
	int written = snprintf(status_path, sizeof status_path, "/proc/%ld/status",
			       (long)process_id);
	if (written < 0 || (size_t)written >= sizeof status_path)
		return 0;

	FILE *status = fopen(status_path, "r");
	if (!status)
		return 0;

	char line[256];
	int caught = 0;
	while (fgets(line, sizeof line, status)) {
		if (strncmp(line, "SigCgt:", 7) != 0)
			continue;
		char *hex = line + 7;
		while (isspace((unsigned char)*hex))
			hex++;
		errno = 0;
		unsigned long long mask = strtoull(hex, NULL, 16);
		if (!errno)
			caught = (mask & (1ULL << (signal_number - 1))) != 0;
		break;
	}
	fclose(status);
	return caught;
}

static int open_ready_renderer(int signal_number, pid_t *process_out)
{
	DIR *processes = opendir("/proc");
	if (!processes)
		return -1;

	int result = -1;
	struct dirent *entry;
	while ((entry = readdir(processes)) != NULL) {
		if (!decimal_name(entry->d_name))
			continue;

		char executable_link[PATH_MAX];
		int written = snprintf(executable_link, sizeof executable_link,
				       "/proc/%s/exe", entry->d_name);
		if (written < 0 || (size_t)written >= sizeof executable_link)
			continue;

		char executable[PATH_MAX];
		ssize_t length = readlink(executable_link, executable,
					  sizeof executable - 1);
		if (length < 0)
			continue;
		executable[length] = 0;
		if (strcmp(executable, DFR_RENDER_EXE) != 0)
			continue;

		errno = 0;
		char *end = NULL;
		long numeric_pid = strtol(entry->d_name, &end, 10);
		if (errno || !end || *end || numeric_pid <= 0 || numeric_pid > INT_MAX)
			continue;
		pid_t process_id = (pid_t)numeric_pid;
		int pidfd = (int)syscall(SYS_pidfd_open, process_id, 0);
		if (pidfd < 0)
			continue;

		/* Revalidate after pinning the process. A PID reused between the first
		 * /proc scan and pidfd_open must not receive our real-time signal. */
		length = readlink(executable_link, executable, sizeof executable - 1);
		if (length < 0) {
			close(pidfd);
			continue;
		}
		executable[length] = 0;
		if (strcmp(executable, DFR_RENDER_EXE) != 0) {
			close(pidfd);
			continue;
		}
		if (!renderer_catches_signal(process_id, signal_number)) {
			close(pidfd);
			continue;
		}
		*process_out = process_id;
		result = pidfd;
		break;
	}
	closedir(processes);
	return result;
}

static int maybe_signal_renderer(int layout_index, int force,
				 pid_t *delivered_process,
				 int *delivered_layout)
{
	if (layout_index < 0 || layout_index >= DFR_NLAYOUTS)
		return -1;
	int signal_number = SIGRTMIN + layout_index;
	if (signal_number > SIGRTMAX)
		return -1;

	pid_t renderer = 0;
	int renderer_fd = open_ready_renderer(signal_number, &renderer);
	if (renderer_fd < 0)
		return 0;
	if (!force && renderer == *delivered_process &&
	    layout_index == *delivered_layout) {
		close(renderer_fd);
		return 1;
	}

	if (syscall(SYS_pidfd_send_signal, renderer_fd, signal_number, NULL, 0) < 0) {
		close(renderer_fd);
		*delivered_process = 0;
		*delivered_layout = -1;
		return 0;
	}
	close(renderer_fd);
	*delivered_process = renderer;
	*delivered_layout = layout_index;
	return 1;
}

static int parse_active_window_event(const char *line, size_t line_length,
				     char *window_class, size_t class_size,
				     char *window_title, size_t title_size)
{
	static const char prefix[] = "activewindow>>";
	size_t prefix_length = sizeof prefix - 1;
	if (line_length < prefix_length ||
	    memcmp(line, prefix, prefix_length) != 0 ||
	    class_size == 0 || title_size == 0)
		return 0;

	const char *payload = line + prefix_length;
	size_t payload_length = line_length - prefix_length;
	if (payload_length && payload[payload_length - 1] == '\r')
		payload_length--;
	const char *separator = memchr(payload, ',', payload_length);
	size_t class_length = separator ? (size_t)(separator - payload) : payload_length;
	size_t title_length = separator ? payload_length - class_length - 1 : 0;
	if (class_length >= class_size)
		class_length = class_size - 1;
	if (title_length >= title_size)
		title_length = title_size - 1;

	memcpy(window_class, payload, class_length);
	window_class[class_length] = 0;
	if (separator && title_length)
		memcpy(window_title, separator + 1, title_length);
	window_title[title_length] = 0;
	return 1;
}

static int event_is_title_update(const char *line, size_t line_length)
{
	/* activewindow>> already carries class/title. Hyprland emits a companion
	 * activewindowv2 event for the same focus change, so intentionally ignore
	 * that duplicate. Title churn may change a browser PWA profile, but it never
	 * force-resets a manually selected Home or Media deck when the profile stays
	 * the same. */
	static const char *const prefixes[] = {
		"windowtitle>>", "windowtitlev2>>",
	};
	for (size_t i = 0; i < ARRAY_LEN(prefixes); i++) {
		size_t length = strlen(prefixes[i]);
		if (line_length >= length && !memcmp(line, prefixes[i], length))
			return 1;
	}
	return 0;
}

static int set_desired_layout(const char *window_class, const char *window_title,
			      int force, int *desired_layout, pid_t *delivered_process,
			      int *delivered_layout)
{
	const char *layout_name = layout_for_window(window_class, window_title);
	int layout_index = dfr_layout_index(layout_name);
	if (layout_index < 0)
		return -1;
	int changed = layout_index != *desired_layout;
	*desired_layout = layout_index;
	if (force || changed)
		(void)maybe_signal_renderer(layout_index, 1, delivered_process,
					    delivered_layout);
	return 0;
}

static int serve_hypr_session(const struct hypr_sockets *sockets,
			      int event_socket)
{
	char window_class[512];
	char window_title[2048];
	if (query_active_window(sockets->command, window_class,
				sizeof window_class, window_title,
				sizeof window_title) < 0)
		return -1;

	int desired_layout = -1;
	int delivered_layout = -1;
	pid_t delivered_process = 0;
	if (set_desired_layout(window_class, window_title, 1, &desired_layout,
			       &delivered_process, &delivered_layout) < 0)
		return -1;

	char event_buffer[EVENT_BUFFER_SIZE];
	size_t used = 0;
	while (!g_stop) {
		struct pollfd descriptor = { .fd = event_socket, .events = POLLIN };
		int ready = poll(&descriptor, 1, 1000);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (ready == 0) {
			(void)maybe_signal_renderer(desired_layout, 0,
						    &delivered_process,
						    &delivered_layout);
			continue;
		}

		if (descriptor.revents & POLLIN) {
			ssize_t received = read(event_socket, event_buffer + used,
						sizeof event_buffer - used);
			if (received < 0 && errno == EINTR)
				continue;
			if (received <= 0)
				return -1;
			used += (size_t)received;

			size_t consumed = 0;
			while (consumed < used) {
				char *newline = memchr(event_buffer + consumed, '\n',
						       used - consumed);
				if (!newline)
					break;
				size_t line_length =
					(size_t)(newline - (event_buffer + consumed));
				const char *line = event_buffer + consumed;
				if (parse_active_window_event(line, line_length,
							      window_class,
							      sizeof window_class,
							      window_title,
							      sizeof window_title)) {
					(void)set_desired_layout(window_class, window_title, 1,
							 &desired_layout,
							 &delivered_process,
							 &delivered_layout);
				} else if (event_is_title_update(line, line_length) &&
					   query_active_window(sockets->command,
							       window_class,
							       sizeof window_class,
							       window_title,
							       sizeof window_title) == 0) {
					(void)set_desired_layout(window_class, window_title, 0,
							 &desired_layout,
							 &delivered_process,
							 &delivered_layout);
				}
				consumed += line_length + 1;
			}
			if (consumed) {
				memmove(event_buffer, event_buffer + consumed, used - consumed);
				used -= consumed;
			}
			/* Discard a malformed overlong event instead of stalling forever. */
			if (used == sizeof event_buffer)
				used = 0;
		}

		if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
			return -1;
	}
	return 0;
}

static void wait_milliseconds(int milliseconds)
{
	while (!g_stop && poll(NULL, 0, milliseconds) < 0 && errno == EINTR)
		;
}

static void print_usage(const char *program)
{
	fprintf(stderr, "usage: %s [--class CLASS [TITLE]]\n", program);
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		if (!strcmp(argv[1], "--class") && (argc == 3 || argc == 4)) {
			puts(layout_for_window(argv[2], argc == 4 ? argv[3] : ""));
			return 0;
		}
		if (!strcmp(argv[1], "--help") && argc == 2) {
			print_usage(argv[0]);
			return 0;
		}
		print_usage(argv[0]);
		return 2;
	}

	(void)signal(SIGINT, on_stop);
	(void)signal(SIGTERM, on_stop);

	while (!g_stop) {
		struct hypr_sockets candidates[MAX_SOCKET_CANDIDATES];
		size_t count = discover_hypr_sockets(candidates,
						     ARRAY_LEN(candidates));
		int connected = 0;
		for (size_t i = 0; i < count && !g_stop; i++) {
			int event_socket = connect_unix_socket(candidates[i].events);
			if (event_socket < 0)
				continue;
			char probe_class[2], probe_title[2];
			if (query_active_window(candidates[i].command,
						probe_class, sizeof probe_class,
						probe_title, sizeof probe_title) < 0) {
				close(event_socket);
				continue;
			}
			connected = 1;
			(void)serve_hypr_session(&candidates[i], event_socket);
			close(event_socket);
			break;
		}
		wait_milliseconds(connected ? 250 : 1000);
	}
	return 0;
}
