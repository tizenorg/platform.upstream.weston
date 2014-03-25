/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2012-2013 Collabora, Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

#include <linux/input.h>
#include <wayland-client.h>

#include "window.h"
#include "embed-client-protocol.h"

#if 0
#define DBG(fmt, ...) \
	fprintf(stderr, "%d:%s " fmt, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DBG(...) do {} while (0)
#endif

static int32_t option_help;
static uint32_t option_hole;
static char *option_type = "none";

static const struct weston_option options[] = {
	{ WESTON_OPTION_UNSIGNED_INTEGER, "embed", 'e', &option_hole },
	{ WESTON_OPTION_BOOLEAN, "help", 'h', &option_help },
	{ WESTON_OPTION_STRING, "type", 't', &option_type },
};


static const char help_text[] =
"Usage: %s [options]\n"
"\n"
"  -h, --help\t\tshow this help text.\n"
"  -e, --embed <UID>\t\tembed within the hole with this UID.\n"
"  -t, --type <type>\t\tselect the type of plug to use.\n"
"\n"
"Creates a surface and embeds it within the hole with the UID\n"
"specified.  See weston-embed-hole for an example of how to create\n"
"a window with embeddable holes.\n"
"\n"
"Types:\n"
"\n"
"  none   - plain blue region\n"
"  flash  - flashing red region\n"
"\n";

/************** The toytoolkit application code: *********************/

struct embed_plug {
	struct display *display;
	struct wl_surface *wl_surface;
	cairo_surface_t *surface;
	struct embed *embed;
	struct plug *plug;
	struct wl_callback *callback;
	int width;
	int height;
};

static void redraw(struct embed_plug *app);

static void redraw_cb(void *data, struct wl_callback *callback, uint32_t time)
{
	struct embed_plug *app = data;
	if (callback)
		wl_callback_destroy(callback);
	redraw(app);
}

static const struct wl_callback_listener frame_listener = {
	redraw_cb
};

static void
redraw(struct embed_plug *app)
{
	bool animate = false;
	cairo_t *cr;

	cr = cairo_create(app->surface);
	if (strcmp(option_type, "flash") == 0)
	{
		struct timespec ts;
		long off;
		clock_gettime(CLOCK_REALTIME_COARSE, &ts);
		off = ts.tv_nsec % 500000000;
		if (off > 250000000) off = 500000000 - off;
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr, 0, 0, app->width, app->height);
		cairo_set_source_rgba(cr, off/250000000., 0, 0, 0.8);
		cairo_fill(cr);
		animate = true;
	}
	else
	{
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr, 0, 0, app->width, app->height);
		cairo_set_source_rgba(cr, 0, 0, 0.8, 0.8);
		cairo_fill(cr);
	}

	cairo_destroy(cr);
	wl_surface_attach(app->wl_surface,
			  display_get_buffer_for_surface(app->display, app->surface),
			  0, 0);
	wl_surface_damage(app->wl_surface, 0, 0, app->width, app->height);
	if (animate)
	{
		app->callback = wl_surface_frame(app->wl_surface);
		wl_callback_add_listener(app->callback, &frame_listener, app);
	}
	wl_surface_commit(app->wl_surface);
}

static void
resize(struct embed_plug *app, int width, int height)
{
	struct rectangle rect = { 0, 0, width ?: 1, height ?: 1 };
	app->width = width;
	app->height = height;
	if (app->surface != NULL)
		cairo_surface_destroy(app->surface);
	app->surface = display_create_surface(app->display, app->wl_surface, &rect, SURFACE_SHM);
	redraw(app);
}

static void
embed_plug_global_handler (struct display *display,
			uint32_t name,
			const char *interface,
			uint32_t version, void *data)
{
	struct embed_plug *embed_plug = data;

	if (!strcmp(interface, "embed")) {
		embed_plug->embed =
			display_bind(display, name, &embed_interface, 1);
	}
}

static void plug_configure(void *data, struct plug *plug, int32_t width, int32_t height)
{
	struct embed_plug *app = data;
	resize(app, width, height);
}

static void plug_removed(void *data, struct plug *plug)
{
	struct embed_plug *app = data;
	display_exit(app->display);
}

static const struct plug_listener embed_plug_plug_listener = {
	.configure = plug_configure,
	.removed = plug_removed
};

static struct embed_plug *
embed_plug_create(struct display *display)
{
	struct embed_plug *app;

	app = xmalloc(sizeof *app);
	memset(app, 0, sizeof *app);

	app->display = display;
	display_set_user_data(app->display, app);

	app->wl_surface = wl_compositor_create_surface(display_get_compositor(app->display));
        wl_surface_set_user_data(app->wl_surface, NULL);
	resize(app, 100, 100);

	display_set_global_handler(display, embed_plug_global_handler);

	if (app->embed != NULL && option_hole)
	{
		app->plug = embed_create_plug(app->embed, app->wl_surface, option_hole);
		plug_add_listener(app->plug, &embed_plug_plug_listener, app);
	}

	return app;
}

static void
embed_plug_destroy(struct embed_plug *app)
{
	free(app);
}

int
main(int argc, char *argv[])
{
	struct display *display;
	struct embed_plug *app;

	parse_options(options, ARRAY_LENGTH(options), &argc, argv);
	if (option_help) {
		printf(help_text, argv[0]);
		return 0;
	}

	display = display_create(&argc, argv);
	if (display == NULL) {
		fprintf(stderr, "failed to create display: %m\n");
		return -1;
	}

	app = embed_plug_create(display);

	display_run(display);

	embed_plug_destroy(app);
	display_destroy(display);

	return 0;
}
