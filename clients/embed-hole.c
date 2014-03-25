/*
 * Copyright © 2010 Intel Corporation
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2012-2013 Collabora, Ltd.
 * Copyright © 2014 Mark Thomas
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

static const struct weston_option options[] = {
	{ WESTON_OPTION_BOOLEAN, "help", 'h', &option_help },
};


static const char help_text[] =
"Usage: %s [options]\n"
"\n"
"  -h, --help\t\tshow this help text.\n"
"\n"
"Creates a window with four embed holes.  The UIDs for these holes are\n"
"printed to standard output.\n"
"\n"
"Run weston-embed-plug, passing a hole UID to the --embed option to\n"
"embed that plug into one of the holes."
"\n"
"Key controls:\n"
"\n"
" 1 to 4 - remove the plug from the numbered hole.\n"
"\n";

/************** The toytoolkit application code: *********************/

struct embed_hole {
	struct display *display;
	struct window *window;
	struct widget *widget;
	struct embed *embed;
	struct hole *hole[4];
};


static void
redraw_handler(struct widget *widget, void *data)
{
	struct embed_hole *app = data;
	cairo_t *cr;
	struct rectangle allocation;
	time_t time;

	widget_get_allocation(app->widget, &allocation);

	cr = widget_cairo_create(widget);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgba(cr, 0, 0.8, 0, 0.8);
	cairo_fill(cr);

	time = widget_get_last_time(widget);
	cairo_set_source_rgba(cr, 0.5, 1.0, 0.5, 1.0);

	cairo_destroy(cr);

	DBG("%dx%d @ %d,%d, last time %u\n",
	    allocation.width, allocation.height,
	    allocation.x, allocation.y, time);
}

static void
resize_handler(struct widget *widget,
	       int32_t width, int32_t height, void *data)
{
	struct embed_hole *app = data;
	struct rectangle area;
	int32_t hw;
	int32_t hh;

	widget_get_allocation(widget, &area);
	hw = area.width/2;
	hh = area.height/2;

	if (app->hole[0]) {
		hole_configure(app->hole[0],
			       area.x, area.y,
			       hw, hh);
	}
	if (app->hole[1]) {
		hole_configure(app->hole[1],
			       area.x+hw, area.y,
			       area.width-hw, hh);
	}
	if (app->hole[2]) {
		hole_configure(app->hole[2],
			       area.x, area.y+hh,
			       hw, area.height-hh);
	}
	if (app->hole[3]) {
		hole_configure(app->hole[3],
			       area.x+hw, area.y+hh,
			       area.width-hw, area.height-hh);
	}
}

static void
keyboard_focus_handler(struct window *window,
		       struct input *device, void *data)
{
	struct embed_hole *app = data;

	window_schedule_redraw(app->window);
}

static void
key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym,
	    enum wl_keyboard_key_state state, void *data)
{
	struct embed_hole *app = data;
	struct rectangle winrect;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	switch (sym) {
	case XKB_KEY_space:
		window_schedule_redraw(window);
		break;
	case XKB_KEY_Up:
		window_get_allocation(window, &winrect);
		winrect.height -= 100;
		if (winrect.height < 150)
			winrect.height = 150;
		window_schedule_resize(window, winrect.width, winrect.height);
		break;
	case XKB_KEY_Down:
		window_get_allocation(window, &winrect);
		winrect.height += 100;
		if (winrect.height > 600)
			winrect.height = 600;
		window_schedule_resize(window, winrect.width, winrect.height);
		break;
	case XKB_KEY_Escape:
		display_exit(app->display);
		break;
	case XKB_KEY_1:
	case XKB_KEY_2:
	case XKB_KEY_3:
	case XKB_KEY_4:
		hole_remove_plug(app->hole[sym - XKB_KEY_1]);
		break;
	}
}

static void
embed_hole_global_handler (struct display *display,
			uint32_t name,
			const char *interface,
			uint32_t version, void *data)
{
	struct embed_hole *embed_hole = data;
	if (!strcmp(interface, "embed")) {
		embed_hole->embed =
			display_bind(display, name, &embed_interface, 1);
	}
}

static void
embed_hole_hole_uid_assigned(void *data, struct hole *hole, uint32_t id)
{
	printf("0x%08x\n", id);
}

static const struct hole_listener embed_hole_hole_listener = {
	.uid_assigned = embed_hole_hole_uid_assigned
};

static struct embed_hole *
embed_hole_create(struct display *display)
{
	struct embed_hole *app;
	int i;

	app = xmalloc(sizeof *app);
	memset(app, 0, sizeof *app);

	app->display = display;
	display_set_user_data(app->display, app);

	app->window = window_create(app->display);
	app->widget = window_frame_create(app->window, app);
	window_set_title(app->window, "Wayland Embed Demo");

	window_set_key_handler(app->window, key_handler);
	window_set_user_data(app->window, app);
	window_set_keyboard_focus_handler(app->window, keyboard_focus_handler);

	widget_set_redraw_handler(app->widget, redraw_handler);
	widget_set_resize_handler(app->widget, resize_handler);

	display_set_global_handler(display, embed_hole_global_handler);

	if (app->embed != NULL)
	{
		for (i=0; i<4; i++) {
			app->hole[i] = embed_create_hole(app->embed, window_get_wl_surface(app->window));
			hole_add_listener(app->hole[i], &embed_hole_hole_listener, app);
			hole_assign_uid(app->hole[i]);
		}
	}

	/* minimum size */
	widget_schedule_resize(app->widget, 100, 100);

	/* initial size */
	widget_schedule_resize(app->widget, 400, 300);

	return app;
}

static void
embed_hole_destroy(struct embed_hole *app)
{
	widget_destroy(app->widget);
	window_destroy(app->window);
	free(app);
}

int
main(int argc, char *argv[])
{
	struct display *display;
	struct embed_hole *app;

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

	app = embed_hole_create(display);

	display_run(display);

	embed_hole_destroy(app);
	display_destroy(display);

	return 0;
}
