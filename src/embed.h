/*
 * Copyright © 2014 Mark Thomas
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _WAYLAND_EMBED_H_
#define _WAYLAND_EMBED_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define WL_HIDE_DEPRECATED
#include <wayland-server.h>

#include <stdbool.h>

struct weston_embed {
	struct wl_list holes;
};

struct weston_hole {
	struct wl_list link;

	struct wl_resource *resource;

	struct weston_embed *embed;

	struct weston_surface *parent_surface;
	struct wl_listener parent_surface_destroy_listener;

	struct {
		int32_t x;
		int32_t y;
	} position;
	struct {
		int32_t width;
		int32_t height;
	} size;

	uint32_t uid;

	struct weston_subsurface *plugged_sub;
};

bool embed_init(struct wl_display *);
void embed_destroy_plug(struct weston_subsurface *sub);

#ifdef  __cplusplus
}
#endif

#endif
