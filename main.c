//
// wayland backend
// all credits to Will Thomas, I followed his tutorial!
//
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "xdg-shell-client-protocol.h"

//
// macros
//
#define ARGB_PLANES 4

//
// global variables
// g_ to avoid confusion with local
// variables.
//
struct wl_compositor *g_compositor;
struct wl_surface *g_surface;
struct wl_buffer *g_surface_buffer;
struct wl_shm *g_shared_memory;
unsigned char *g_pixels;
unsigned short g_width = 200;
unsigned short g_height = 200;
struct xdg_wm_base *g_xdg_shell;
struct xdg_toplevel *g_xdg_toplevel;
bool g_should_close;
struct wl_seat *g_seat;
struct wl_keyboard *g_keyboard;

//
// declaration of listeners
//
static struct xdg_wm_base_listener g_xdg_wm_base_listener;
static struct xdg_toplevel_listener g_xdg_toplevel_listener;
static struct xdg_surface_listener g_xdg_surface_listener;
static struct wl_registry_listener g_registry_listener;
static struct wl_callback_listener g_callback_listener;
static struct wl_keyboard_listener g_keyboard_listener;
static struct wl_seat_listener g_seat_listener;

//
// functions
//
static int alloc_shared_memory(size_t bytes) {
	char name[8];
	int length;
	int fd;

	name[0] = '/';
	name[1] = (rand() & 'z') + 'a';
	name[2] = (rand() & 'z') + 'a';
	name[3] = (rand() & 'z') + 'a';
	name[4] = (rand() & 'z') + 'a';
	name[5] = (rand() & 'z') + 'a';
	name[6] = (rand() & 'z') + 'a';
	name[7] = '\0';

	fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL,
			S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
	ftruncate(fd, bytes);

	return fd;
}

static void pixels_resize(void) {
	int fd;
	unsigned int bytes;
	struct wl_shm_pool *pool;

	bytes = g_width * g_height * ARGB_PLANES;
	fd = alloc_shared_memory(bytes);

	g_pixels = mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	pool = wl_shm_create_pool(g_shared_memory, fd, bytes);
	g_surface_buffer = wl_shm_pool_create_buffer(pool, 0, g_width, g_height,
			g_width * 4, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
}

static void pixels_draw(void) {
	memset(g_pixels, 0xFF, g_width * g_height * ARGB_PLANES);

	wl_surface_attach(g_surface, g_surface_buffer, 0, 0);
	wl_surface_damage(g_surface, 0, 0, g_width, g_height);
	wl_surface_commit(g_surface);
}

static void xdg_toplevel_configure(void *data,
				struct xdg_toplevel *xdg_toplevel,
				int32_t width,
				int32_t height,
				struct wl_array *states) {
	if (width == 0 || height == 0) {
		return;
	}

	if (g_width != width || g_height != height) {
		munmap(g_pixels, g_width * g_height * ARGB_PLANES);
		g_width = width;
		g_height = height;
		pixels_resize();
	}
}

static void xdg_toplevel_close(void *data,
				struct xdg_toplevel *xdg_toplevel) {
	g_should_close = true;
}

static void xdg_surface_configure(void *data,
				struct xdg_surface *xdg_surface,
				uint32_t serial) {
	xdg_surface_ack_configure(xdg_surface, serial);

	if (g_pixels == NULL) {
		pixels_resize();
	}

	pixels_draw();
}

static void xdg_wm_base_ping(void *data,
				struct xdg_wm_base *xdg_wm_base,
				uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static void registry_global(void *data,
				struct wl_registry *wl_registry,
				uint32_t name,
				const char *interface,
				uint32_t version) {
	if (!strcmp(interface, wl_compositor_interface.name)) {
		g_compositor = wl_registry_bind(wl_registry, name,
				&wl_compositor_interface, 4);
	} else if (!strcmp(interface, wl_shm_interface.name)) {
		g_shared_memory = wl_registry_bind(wl_registry, name,
				&wl_shm_interface, 1);
	} else if (!strcmp(interface, xdg_wm_base_interface.name)) {
		g_xdg_shell = wl_registry_bind(wl_registry, name,
				&xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(g_xdg_shell, &g_xdg_wm_base_listener, NULL);
	} else if (!strcmp(interface, wl_seat_interface.name)) {
		g_seat = wl_registry_bind(wl_registry, name,
				&wl_seat_interface, 1);
		wl_seat_add_listener(g_seat, &g_seat_listener, NULL);
	}
}

static void registry_global_remove(void *data,
				struct wl_registry *wl_registry,
				uint32_t name) {
}

static void callback_frame_new(void *data,
				struct wl_callback *wl_callback,
				uint32_t callback_data) {
	wl_callback_destroy(wl_callback);

	wl_callback = wl_surface_frame(g_surface);
	wl_callback_add_listener(wl_callback, &g_callback_listener, NULL);

	pixels_draw();
}

void keyboard_keymap(void *data,
		   struct wl_keyboard *wl_keyboard,
		   uint32_t format,
		   int32_t fd,
		   uint32_t size) {
}

void keyboard_enter(void *data,
		  struct wl_keyboard *wl_keyboard,
		  uint32_t serial,
		  struct wl_surface *surface,
		  struct wl_array *keys) {
}

static void keyboard_leave(void *data,
				  struct wl_keyboard *wl_keyboard,
				  uint32_t serial,
				  struct wl_surface *surface) {
}

static void keyboard_key(void *data,
				struct wl_keyboard *wl_keyboard,
				uint32_t serial,
				uint32_t time,
				uint32_t key,
				uint32_t state) {
	if (key == 0x1) {
		g_should_close = true;
	}
}

static void keyboard_modifiers(void *data,
				  struct wl_keyboard *wl_keyboard,
				  uint32_t serial,
				  uint32_t mods_depressed,
				  uint32_t mods_latched,
				  uint32_t mods_locked,
				  uint32_t group) {
}

static void keyboard_repeat_info(void *data,
					struct wl_keyboard *wl_keyboard,
					int32_t rate,
					int32_t delay) {
}

static void seat_capabilities(void *data,
				 struct wl_seat *wl_seat,
				 uint32_t capabilities) {
	if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && g_keyboard == NULL) {
		g_keyboard = wl_seat_get_keyboard(g_seat);
		wl_keyboard_add_listener(g_keyboard, &g_keyboard_listener, NULL);
	}
}

static void seat_name(void *data,
				 struct wl_seat *wl_seat,
				 const char *name) {
}

//
// definition of listeners
//
static struct xdg_wm_base_listener g_xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping
};

static struct xdg_toplevel_listener g_xdg_toplevel_listener = {
	.configure = xdg_toplevel_configure,
	.close = xdg_toplevel_close
};

static struct xdg_surface_listener g_xdg_surface_listener = {
	.configure = xdg_surface_configure
};

static struct wl_registry_listener g_registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove
};

static struct wl_callback_listener g_callback_listener = {
	.done = callback_frame_new
};

static struct wl_keyboard_listener g_keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info
};

static struct wl_seat_listener g_seat_listener = {
	.capabilities = seat_capabilities,
	.name = seat_name
};

//
// entry point
//
int main(int argc, char *argv[]) {
	struct wl_display *display;
	struct wl_registry *registry;

	srand(time(0));

	display = wl_display_connect(0);
	registry = wl_display_get_registry(display);
	
	wl_registry_add_listener(registry, &g_registry_listener, NULL);
	wl_display_roundtrip(display);

	g_surface = wl_compositor_create_surface(g_compositor);

	struct wl_callback *callback = wl_surface_frame(g_surface);
	wl_callback_add_listener(callback, &g_callback_listener, NULL);

	struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(
			g_xdg_shell, g_surface);
	xdg_surface_add_listener(xdg_surface, &g_xdg_surface_listener, NULL);
	
	g_xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
	xdg_toplevel_add_listener(g_xdg_toplevel, &g_xdg_toplevel_listener, NULL);
	xdg_toplevel_set_title(g_xdg_toplevel, "Wclient");
	wl_surface_commit(g_surface);

	while (wl_display_dispatch(display)) {
		if (g_should_close) {
			break;
		}
	}

	if (g_keyboard != NULL) {
		wl_keyboard_destroy(g_keyboard);
	}
	wl_seat_release(g_seat);
	if (g_surface_buffer != NULL) {
		wl_buffer_destroy(g_surface_buffer);
	}
	xdg_toplevel_destroy(g_xdg_toplevel);
	xdg_surface_destroy(xdg_surface);
	wl_surface_destroy(g_surface);
	wl_display_disconnect(display);
	return 0;
}
