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

#include "config.h"

#include <stdlib.h>
#include <assert.h>

#include "compositor.h"
#include "embed.h"
#include "embed-server-protocol.h"

#define HOLE_MAGIC   0x80000000
#define UID_MASK     0x0FFFFFFF

static struct weston_hole *
find_hole_by_uid(struct weston_embed *embed, uint32_t uid)
{
	struct weston_hole *hole;
	wl_list_for_each(hole, &embed->holes, link) {
		if (hole->uid == uid) {
			return hole;
		}
	}
	return NULL;
}

static void
hole_assign_uid(struct wl_client *client, struct wl_resource *resource)
{
	struct weston_hole *hole = wl_resource_get_user_data(resource);
	hole_send_uid_assigned(resource, hole->uid);
}

static void
hole_configure(struct wl_client *client, struct wl_resource *resource,
		 uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct weston_hole *hole = wl_resource_get_user_data(resource);

	hole->position.x = x;
	hole->position.y = y;
	hole->size.width = width;
	hole->size.height = height;

	if (!hole->plugged_sub)
		return;

	hole->plugged_sub->position.x = x;
	hole->plugged_sub->position.y = y;
	hole->plugged_sub->position.set = 1;

	plug_send_configure(hole->plugged_sub->resource, width, height);
}

static void
hole_remove_plug(struct wl_client *client, struct wl_resource *resource)
{
	struct weston_hole *hole = wl_resource_get_user_data(resource);

	if (!hole->plugged_sub)
		return;

	if (weston_surface_is_mapped(hole->plugged_sub->surface))
		weston_surface_unmap(hole->plugged_sub->surface);

	weston_subsurface_unlink_parent(hole->plugged_sub);

	plug_send_removed(hole->plugged_sub->resource);

	hole->plugged_sub = NULL;
}

static void
hole_destroy(struct weston_hole *hole)
{
	if (hole->plugged_sub) {
		plug_send_removed(hole->plugged_sub->resource);
		hole->plugged_sub->hole = NULL;
		hole->plugged_sub->embed = NULL;
	}

	wl_list_remove(&hole->link);
	free(hole);
}

static void
hole_resource_destroy(struct wl_resource *resource)
{
	struct weston_hole *hole = wl_resource_get_user_data(resource);
	if (hole)
		hole_destroy(hole);
}

static void
hole_handle_parent_surface_destroy(struct wl_listener *listener, void *data)
{
	struct weston_hole *hole = container_of(listener,
						struct weston_hole,
						parent_surface_destroy_listener);
	assert(data == &hole->parent_surface->resource);

	if (hole->resource)
		wl_resource_set_user_data(hole->resource, NULL);

	hole_destroy(hole);
}

static const struct hole_interface hole_impl = {
	hole_assign_uid,
	hole_configure,
        hole_remove_plug
};

static void
plug_resource_destroy(struct wl_resource *resource)
{
	struct weston_subsurface *sub = wl_resource_get_user_data(resource);
	if (sub)
		weston_subsurface_destroy(sub);
}

static void
plug_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct plug_interface plug_impl = {
	plug_destroy
};

static void
embed_create_hole(struct wl_client *client, struct wl_resource *resource,
			  uint32_t id, struct wl_resource *surface_resource)
{
	struct weston_embed *embed = wl_resource_get_user_data(resource);
	struct weston_hole *hole;

	hole = calloc(1, sizeof *hole);
	if (hole == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	while (hole->uid == 0 || find_hole_by_uid(embed, hole->uid) != NULL) {
		hole->uid = HOLE_MAGIC | (random() & UID_MASK);
	}
	hole->embed = embed;
	wl_list_insert(&embed->holes, &hole->link);

	hole->parent_surface = wl_resource_get_user_data(surface_resource);
	hole->parent_surface_destroy_listener.notify =
		hole_handle_parent_surface_destroy;
	wl_signal_add(&hole->parent_surface->destroy_signal,
		      &hole->parent_surface_destroy_listener);

	hole->resource =
		wl_resource_create(client, &hole_interface, 1, id);
	if (hole->resource == NULL) {
		free(hole);
		wl_resource_post_no_memory(resource);
		return;
	}
	wl_resource_set_implementation(hole->resource, &hole_impl,
				       hole, hole_resource_destroy);
}

static void
embed_create_plug(struct wl_client *client, struct wl_resource *resource,
			uint32_t id, struct wl_resource *surface_resource, uint32_t hole_uid)
{
	struct weston_embed *embed = wl_resource_get_user_data(resource);
	struct weston_surface *surface = wl_resource_get_user_data(surface_resource);
	struct weston_hole *hole;
	struct weston_subsurface *sub;
	struct weston_view *view;
	static const char where[] = "embed_create_plug: plug@";

	hole = find_hole_by_uid(embed, hole_uid);

	if (hole == NULL) {
		wl_resource_post_error(resource,
			EMBED_ERROR_BAD_UID,
			"%s%d: uid %08x is not an embed hole",
			where, id, hole_uid);
		return;
	}

	if (hole->plugged_sub) {
		wl_resource_post_error(resource,
			EMBED_ERROR_BAD_UID,
			"%s%d: embed hole with uid %08x is occupied",
			where, id, hole_uid);
		return;
	}

	sub = weston_subsurface_create(surface, hole->parent_surface);
	if (sub == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	sub->resource =
		wl_resource_create(client, &plug_interface, 1, id);
	if (sub->resource == NULL) {
		free(sub);
		wl_resource_post_no_memory(resource);
		return;
	}
	wl_resource_set_implementation(sub->resource, &plug_impl,
				       sub, plug_resource_destroy);

	weston_subsurface_create_complete(sub);

	sub->synchronized = 0;
	sub->embed = embed;
	sub->hole = hole;
	sub->hole->plugged_sub = sub;
	sub->position.x = hole->position.x;
	sub->position.y = hole->position.y;
	sub->position.set = 1;

	wl_list_for_each(view, &hole->parent_surface->views, surface_link)
		weston_view_geometry_dirty(view);

	plug_send_configure(sub->resource, hole->size.width, hole->size.height);
	weston_surface_damage(hole->parent_surface);
}

static const struct embed_interface embed_impl = {
	embed_create_hole,
	embed_create_plug
};

void
embed_destroy_plug(struct weston_subsurface *sub)
{
	if (sub->hole) {
		assert(sub->hole->plugged_sub = sub);
		sub->hole->plugged_sub = NULL;
		sub->hole = NULL;
		sub->embed = NULL;
	}
}

static void
bind_embed(struct wl_client *client,
		 void *data, uint32_t version, uint32_t id)
{
	struct weston_embed *embed = data;
	struct wl_resource *resource;

	resource =
		wl_resource_create(client, &embed_interface, 1, id);
	if (resource == NULL) {
		free(embed);
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &embed_impl,
				       embed, NULL);
}

bool
embed_init(struct wl_display *display)
{
	struct weston_embed *embed = calloc(1, sizeof *embed);
	assert(embed != NULL);
	wl_list_init(&embed->holes);
	if (!wl_global_create(display, &embed_interface, 1,
			      embed, bind_embed))
		return false;
	return true;
}

