/*
 * Copyright © 2011 Intel Corporation
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

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <linux/input.h>
#include <assert.h>

#include "mobile-shell-server-protocol.h"
#include "input-method-server-protocol.h"
#include "tizen.h"
#include <weston/compositor.h>
#include <wayland-server.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define MAX_PIDS_SIZE 256
extern void wayland_vconf_set_hide_pids(char **pstring);
extern void wayland_vconf_init();

enum {
	STATE_STARTING,
	STATE_HOME,
	STATE_TASK
};

struct shell_surface;

struct client_task {
	struct wl_client *client;
	struct wl_list shsurf_list;
	struct wl_list link;
	/* is there some case one pid with severl client? */
	/* Let's group through pid */
	pid_t pid;
};

struct input_panel_surface {
	struct wl_resource *resource;
	struct wl_signal destroy_signal;

	struct mobile_shell *shell;

	struct wl_list link;
	struct weston_surface *surface;
	struct wl_listener surface_destroy_listener;

	struct weston_output *output;
	uint32_t panel;
};

struct msclient_list {
	struct wl_list task_list;
	struct client_task *menuscreen_task;
};

struct mobile_shell {
	struct wl_resource *resource;

	struct wl_listener unlock_listener;
	struct wl_listener destroy_listener;
	struct wl_listener show_input_panel_listener;
	struct wl_listener hide_input_panel_listener;

	struct weston_compositor *compositor;
	struct weston_process process;
	struct wl_client *client;
	
	struct weston_surface *indicator_surface;
	struct weston_layer indicator_layer;

	struct weston_surface *menuscreen_surface;
	struct weston_layer homescreen_layer;

	struct weston_layer application_layer;
	struct weston_layer input_panel_layer;
	struct weston_layer hide_layer;

	bool locked;
	bool showing_input_panels;

	struct msclient_list msclient_list;
	struct {
		struct wl_resource *binding;
		struct wl_list surfaces;
	} input_panel;
	int state, previous_state;
	struct {
		struct weston_surface *surface;
		pixman_box32_t cursor_rectangle;
	} text_input;
	struct wl_list shsubsurf_list;
	struct wl_resource *control_resource;
};

struct shell_subsurface {
	struct wl_list link;
	struct weston_surface *surface;
	struct wl_listener surface_destroy_listener;
	uint32_t scalex;
	uint32_t scaley;
	struct weston_transform scale;
	void (*original_subsurface_configure)(struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height);
	struct wl_listener listener;
};

struct shell_surface {
	struct wl_resource *resource;
	struct wl_signal destroy_signal;

	struct wl_resource *surface_control_resource;

	struct weston_surface *surface;
	struct weston_surface *parent_surface;
	struct wl_listener surface_destroy_listener;
	struct mobile_shell *shell;

	struct weston_output *output;
	struct wl_list link;

	int is_fullscreen;
	struct {
		enum wl_shell_surface_fullscreen_method type;
		struct weston_transform transform;
		uint32_t framerate;
	} fullscreen;

        int is_visible;

	struct client_task *client_task;
	const struct weston_shell_client *client;
};

static void
input_panel_configure(struct weston_surface *surface, int32_t sx, int32_t sy, int32_t width, int32_t height);

static struct mobile_shell *
get_shell(struct weston_compositor *compositor);

static void
mobile_shell_destroy(struct wl_listener *listener, void *data);

static void
shell_surface_configure(struct weston_surface *surface,
			int32_t sx, int32_t sy, int32_t w, int32_t h);

static void
homeui_surface_configure(struct weston_surface *es,
			       int32_t sx, int32_t sy, int32_t w, int32_t h);

static void
put_on_output(struct weston_surface *surface, struct weston_output *output, int32_t w, int32_t h);

static void
mobile_shell_set_state(struct mobile_shell *shell, int state);

static void
switch_task(struct mobile_shell *shell);

static void
client_task_create(struct shell_surface *shsurf,
			    struct wl_client *client);

static struct shell_subsurface *shell_subsurface_create(struct weston_surface *surface);
static void *shell_subsurface_configure (struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height) ;

static struct shell_subsurface * get_shell_subsurface_from_subsurface_list(struct weston_surface *surface)
{
   struct shell_subsurface *shsub = NULL;
   struct mobile_shell *shell = surface->compositor->shell_interface.shell;
   wl_list_for_each(shsub, &shell->shsubsurf_list, link)
   {
       if (shsub && (surface == shsub->surface))
           return shsub;
   }
   return NULL;
}

static bool is_weston_subsurface(struct weston_surface *surface)
{
   return (surface->configure != NULL &&
          surface->configure != homeui_surface_configure &&
          surface->configure != shell_surface_configure &&
          surface->configure != input_panel_configure);
}

static struct shell_subsurface * get_shell_subsurface(struct weston_surface *surface)
{
    struct shell_subsurface *shsubsurf;
    if (is_weston_subsurface(surface))
    {
         shsubsurf = get_shell_subsurface_from_subsurface_list(surface);
         if (!shsubsurf) {
            shsubsurf = shell_subsurface_create(surface);
            assert(shsubsurf);
         }
         return shsubsurf;
    }
    else
         return NULL;
}

static void *shell_subsurface_configure (struct weston_surface *es, int32_t sx, int32_t sy, int32_t width, int32_t height)
{
	struct shell_subsurface *shsubsurface;
	struct weston_matrix *matrix;
	float cx, cy;

	shsubsurface = get_shell_subsurface(es);
	assert(shsubsurface);

	shsubsurface->original_subsurface_configure(es, sx, sy, width, height);

	if (es->geometry.width == 0 || es->geometry.height == 0)
	{
		weston_log("Incorrect w/h\n");
		return;
	}

	if (shsubsurface->scalex > 0 && shsubsurface->scaley > 0 )
	{
		cx =  (float)shsubsurface->scalex/es->geometry.width;
		cy =  (float)shsubsurface->scaley/es->geometry.height;
		if ( fabs(cx-1) > 0.001 || fabs(cy-1) > 0.001 )
		{
			wl_list_remove(&shsubsurface->scale.link);
			wl_list_init(&shsubsurface->scale.link);
			weston_surface_geometry_dirty(es);
			matrix = &shsubsurface->scale.matrix;
			weston_matrix_init(matrix);
			weston_matrix_scale(matrix, cx, cy, 1.0f);
			wl_list_insert(
					&es->geometry.transformation_list,
					&shsubsurface->scale.link);
		} else {
			wl_list_remove(&shsubsurface->scale.link);
			wl_list_init(&shsubsurface->scale.link);
		}
	}
	weston_compositor_schedule_repaint(es->compositor);
}

static void
subsurface_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct shell_subsurface *shsubsurf =
		container_of(listener, struct shell_subsurface,
			     surface_destroy_listener);

        wl_list_remove(&shsubsurf->link);
        wl_list_remove(&shsubsurf->surface_destroy_listener.link);
	shsubsurf->surface->configure = NULL;
        free(shsubsurf);
}

static struct shell_subsurface *shell_subsurface_create(struct weston_surface *surface)
{
   struct shell_subsurface *shsubsurface;
   struct mobile_shell *shell = surface->compositor->shell_interface.shell;
   shsubsurface  = get_shell_subsurface_from_subsurface_list(surface);
   if (shsubsurface)
       return shsubsurface;
   shsubsurface = (struct shell_subsurface *)malloc(sizeof(struct shell_subsurface));
   shsubsurface->original_subsurface_configure = surface->configure;
   shsubsurface->surface = surface;
   surface->configure = shell_subsurface_configure;

   /*Add destroy listener for shell_subsurface*/
   shsubsurface->surface_destroy_listener.notify = subsurface_handle_surface_destroy;
   wl_signal_add(&surface->destroy_signal,
	      &shsubsurface->surface_destroy_listener);

   wl_list_insert(&shell->shsubsurf_list, &shsubsurface->link);
   wl_list_init(&shsubsurface->scale.link);
   return shsubsurface;
}

static struct shell_surface *
get_shell_surface(struct weston_surface *surface)
{
	if (surface->configure == shell_surface_configure)
		return surface->configure_private;
	else if (surface->configure == homeui_surface_configure)
		return surface->configure_private;
	else
		return NULL;
}

static void
destroy_shell_surface(struct shell_surface *shsurf)
{
	wl_list_remove(&shsurf->surface_destroy_listener.link);
	shsurf->surface->configure = NULL;

	free(shsurf);
}

static void
shell_destroy_shell_surface(struct wl_resource *resource)
{
	struct shell_surface *shsurf = wl_resource_get_user_data(resource);
	struct mobile_shell *shell = shsurf->shell;

	if (weston_surface_is_mapped(shsurf->surface)) {
		shsurf->surface->output = NULL;
		if(&shsurf->surface->layer_link)
			wl_list_remove(&shsurf->surface->layer_link);
		shsurf->is_visible == SURFACE_CONTROL_VISIBILITY_INVISIBLE;
	}

	if(shsurf->client_task) {
		wl_list_remove(&shsurf->link);
		if (!wl_list_length(&shsurf->client_task->shsurf_list)) {
			wl_list_remove(&shsurf->client_task->link);	
			switch_task(shell);
			free(shsurf->client_task);
		}
		shsurf->client_task = NULL;
	}

	destroy_shell_surface(shsurf);
}

static void
shell_destroy_shell(struct wl_resource *resource)
{
	/*destroy the shell related infos like the visibility, scale info*/
}

static void
shell_destroy_wl_control(struct wl_resource *resource)
{
	/*destroy the wl control related infos like the visibility, scale info*/
}

static void
shell_destroy_surface_control(struct wl_resource *resource)
{
	struct shell_surface *shsurf = wl_resource_get_user_data(resource);
	struct mobile_shell *shell = shsurf->shell;
	/*destroy the surface control related infos like the visibility, scale info*/

}

static void
shell_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct shell_surface *shsurf =
		container_of(listener, struct shell_surface,
			     surface_destroy_listener);

	if (shsurf->resource)
		wl_resource_destroy(shsurf->resource);
	else
		destroy_shell_surface(shsurf);
}

static void
shell_stack_fullscreen(struct shell_surface *shsurf)
{
	struct weston_surface *surface = shsurf->surface;
	struct mobile_shell *shell = shsurf->shell;

	wl_list_remove(&surface->layer_link);
	wl_list_insert(&shell->application_layer.surface_list,
		       &surface->layer_link);
	weston_surface_damage(surface);
}

static void
configure(struct mobile_shell *shell, struct weston_surface *surface,
	  GLfloat x, GLfloat y, int32_t width, int32_t height)
{
	struct shell_surface *shsurf;

	shsurf = get_shell_surface(surface);
	surface->geometry.x = x;
	surface->geometry.y = y;
	surface->geometry.width = width;
	surface->geometry.height = height;
	weston_surface_geometry_dirty(surface);

	shell_stack_fullscreen(shsurf);
	put_on_output(surface, surface->output, width, height);
	weston_surface_update_transform(surface);
}

static void
shell_surface_configure(struct weston_surface *surface,
			int32_t sx, int32_t sy, int32_t w, int32_t h)
{
	struct mobile_shell *shell = get_shell(surface->compositor);
	struct shell_surface *shsurf = get_shell_surface(surface);
	struct weston_seat *seat;

	if (shsurf->is_visible == SURFACE_CONTROL_VISIBILITY_INVISIBLE) {
		weston_compositor_schedule_repaint(surface->compositor);
		return;
	} else if (shsurf->is_visible == NULL) {
		shsurf->is_visible = SURFACE_CONTROL_VISIBILITY_VISIBLE;
	}

	if (!weston_surface_is_mapped(surface)) {
		mobile_shell_set_state(shell, STATE_TASK);
		weston_surface_configure(surface, 0, 0, w, h);
		wl_list_insert(&shell->application_layer.surface_list,
			       &surface->layer_link);
		weston_surface_update_transform(surface);
		put_on_output(surface, surface->output, w, h);
		shell_stack_fullscreen(shsurf);
		wl_list_for_each(seat,
				 &surface->compositor->seat_list,
				 link)
			weston_surface_activate(surface, seat);
		weston_compositor_schedule_repaint(surface->compositor);
	}
	if (sx != 0 || sy != 0 ||
		surface->geometry.width != w ||
		surface->geometry.height != h) {
		GLfloat from_x, from_y;
		GLfloat to_x, to_y;

		weston_surface_to_global_float(surface, 0, 0,
					       &from_x, &from_y);
		weston_surface_to_global_float(surface, sx, sy,
					       &to_x, &to_y);
		configure(shell, surface,
			  surface->geometry.x + to_x - from_x,
			  surface->geometry.y + to_y - from_y,
			  w, h);
	}

	wl_list_for_each(seat,
			 &surface->compositor->seat_list,
			 link)
		weston_surface_activate(surface, seat);

}

static void
shell_surface_pong(struct wl_client *client, struct wl_resource *resource,
		uint32_t serial)
{
	return;
}

static void
shell_surface_set_title(struct wl_client *client,
			struct wl_resource *resource, const char *title)
{
	return;
}

static void
shell_surface_set_class(struct wl_client *client,
			struct wl_resource *resource, const char *class)
{
	struct shell_surface *shsurf = wl_resource_get_user_data(resource);
	struct weston_surface *es = shsurf->surface;
	struct mobile_shell *shell = shsurf->shell;
	struct client_task *menuscreen_task = shell->msclient_list.menuscreen_task;
	pid_t pid;
	uid_t uid;
	gid_t gid;

	if (strcmp(class, "indicator") == 0) {
		shell->indicator_surface = es;
		shell->indicator_surface->configure = homeui_surface_configure;

		wl_list_insert(&shell->indicator_layer.surface_list,
				&es->layer_link);
		weston_surface_set_position(shell->indicator_surface, 0, 0);
		/*we did not add indicator task to the task list for now*/
		if (shsurf->client_task) {
			wl_list_remove(&shsurf->client_task->link);
			free(shsurf->client_task);
			shsurf->client_task = NULL;
		}
		shsurf->is_visible == SURFACE_CONTROL_VISIBILITY_VISIBLE;

	} else if (strcmp(class, "menu-screen") == 0) {
		shell->menuscreen_surface = es;
		shell->menuscreen_surface->configure = homeui_surface_configure;

		wl_list_insert(&shell->homescreen_layer.surface_list,
				&es->layer_link);
		shell->state = STATE_HOME;
		shell->menuscreen_surface->configure_private = shsurf;
		weston_surface_set_position(shell->menuscreen_surface, 0, 0);
		
		wl_client_get_credentials(client, &pid, &uid, &gid);
		shell->msclient_list.menuscreen_task->client = client;
		shell->msclient_list.menuscreen_task->pid = pid;

		if (!shsurf->client_task) {
			shsurf->client_task = menuscreen_task;
		} else if (shsurf->client_task != menuscreen_task) {
			wl_list_remove(&shsurf->client_task->link);
			free(shsurf->client_task);
			shsurf->client_task = menuscreen_task;
		} else {
			wl_list_remove(&shsurf->client_task->link);
			wl_list_init(&menuscreen_task->link);
			wl_list_init(&menuscreen_task->shsurf_list);
		}
		wl_list_insert(menuscreen_task->shsurf_list.prev, &shsurf->link);
		shsurf->is_visible == SURFACE_CONTROL_VISIBILITY_VISIBLE;
	}
}

static void
shell_surface_move(struct wl_client *client, struct wl_resource *resource,
			struct wl_resource *seat_resource, uint32_t serial)
{
	return;
}

static void
shell_surface_resize(struct wl_client *client, struct wl_resource *resource,
			struct wl_resource *seat_resource, uint32_t serial,
			uint32_t edges)
{
	return;
}

static void
shell_surface_set_toplevel(struct wl_client *client,
			struct wl_resource *resource)
{
	return;
}

static void
shell_surface_set_transient(struct wl_client *client,
			struct wl_resource *resource,
			struct wl_resource *parent_resource,
			int x, int y, uint32_t flags)
{
	return;
}

static void
shell_surface_set_fullscreen(struct wl_client *client,
			struct wl_resource *resource,
			uint32_t method,
			uint32_t framerate,
			struct wl_resource *output_resource)
{
	struct shell_surface *shsurf = wl_resource_get_user_data(resource);
	struct weston_surface *es = shsurf->surface;

	if (output_resource)
		shsurf->output = output_wl_resource_get_user_data(resource);
	else
		shsurf->output = container_of(es->compositor->output_list.next,
			    struct weston_output, link);

	shsurf->client->send_configure(shsurf->surface, 0,
				       shsurf->output->current_mode->width,
				       shsurf->output->current_mode->height);
}

static void
shell_surface_set_popup(struct wl_client *client,
			struct wl_resource *resource,
			struct wl_resource *seat_resource,
			uint32_t serial,
			struct wl_resource *parent_resource,
			int32_t x, int32_t y, uint32_t flags)
{
	return;
}

static void
shell_surface_set_maximized(struct wl_client *client,
			struct wl_resource *resource,
			struct wl_resource *output_resource)
{
	return;
}

static const struct wl_shell_surface_interface shell_surface_implementation = {
	shell_surface_pong,
	shell_surface_move,
	shell_surface_resize,
	shell_surface_set_toplevel,
	shell_surface_set_transient,
	shell_surface_set_fullscreen,
	shell_surface_set_popup,
	shell_surface_set_maximized,
	shell_surface_set_title,
	shell_surface_set_class
};

static void
surface_control_set_dest_rect(struct wl_client *client,
                       struct wl_resource *resource,
                       struct wl_resource *surface_resource,
                       uint32_t width, uint32_t height)
{
   struct weston_surface *surface = surface_wl_resource_get_user_data(resource);
   struct shell_subsurface *shsubsurf = get_shell_subsurface(surface);
   if (shsubsurf)
   {
       shsubsurf->scalex = width;
       shsubsurf->scaley = height;
   }
}

static void
surface_control_set_visibility(struct wl_client *client,
                       struct wl_resource *resource,
                       struct wl_resource *surface_resource,
                       uint32_t visibility)
{
	struct weston_surface *surface = surface_wl_resource_get_user_data(resource);
	struct shell_surface *shsurf = get_shell_surface(surface);
	struct mobile_shell *shell = get_shell(surface->compositor);
	struct weston_seat *seat;

	weston_surface_configure(surface, 0, 0, surface->geometry.width, surface->geometry.height);
	if (shsurf->is_visible == NULL) {
		if ( visibility == SURFACE_CONTROL_VISIBILITY_INVISIBLE ) {
			wl_list_insert(&shell->hide_layer.surface_list,
					&surface->layer_link);

			if(!wl_list_length(&shell->application_layer.surface_list))
				mobile_shell_set_state(shell, STATE_HOME);
		} else {
			wl_list_insert(&shell->application_layer.surface_list,
					&surface->layer_link);
			weston_surface_update_transform(surface);
			put_on_output(surface, surface->output, surface->geometry.width, surface->geometry.height);
			shell_stack_fullscreen(shsurf);
			shsurf->is_visible = SURFACE_CONTROL_VISIBILITY_VISIBLE;
			wl_list_for_each(seat,
					&surface->compositor->seat_list,
					link)
				weston_surface_activate(surface, seat);

			mobile_shell_set_state(shell, STATE_TASK);
		}

		weston_compositor_schedule_repaint(surface->compositor);
		return;
	}

	if ( visibility == SURFACE_CONTROL_VISIBILITY_INVISIBLE ) {
		wl_list_remove(&surface->layer_link);
		wl_list_insert(&shell->hide_layer.surface_list,
				&surface->layer_link);
	} else {
		wl_list_remove(&surface->layer_link);
		wl_list_insert(&shell->application_layer.surface_list,
				&surface->layer_link);
		weston_surface_update_transform(surface);
		put_on_output(surface, surface->output, surface->geometry.width, surface->geometry.height);
		shell_stack_fullscreen(shsurf);
		wl_list_for_each(seat,
				&surface->compositor->seat_list,
				link)
			weston_surface_activate(surface, seat);
	}

	shsurf->is_visible = visibility;

	if(!wl_list_length(&shell->application_layer.surface_list))
		mobile_shell_set_state(shell, STATE_HOME);
	else
		mobile_shell_set_state(shell, STATE_TASK);

	weston_compositor_schedule_repaint(surface->compositor);
}




static const struct surface_control_interface surface_control_implementation = {
       surface_control_set_visibility,
       surface_control_set_dest_rect
};

static void
controller_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
controller_get_control(struct wl_client *client,
			  struct wl_resource *resource, uint32_t id,
			  struct wl_resource *resource_usr)
{
	struct shell_surface *shsurf = wl_resource_get_user_data(resource_usr);

	shsurf->surface_control_resource =
		wl_resource_create(client, &surface_control_interface,1,id);
	wl_resource_set_implementation(shsurf->surface_control_resource,
				       &surface_control_implementation,
				       shsurf, shell_destroy_surface_control);
}

static const struct wl_control_interface wl_control_implementation = {
       controller_get_control,
       controller_destroy
};

static void
bind_wl_control(struct wl_client *client,
	      void *data, uint32_t version, uint32_t id)
{
	struct mobile_shell *shell = data;

	shell->control_resource = wl_resource_create(client, &wl_control_interface, 1, id);
	wl_resource_set_implementation(shell->control_resource, &wl_control_implementation, shell, shell_destroy_wl_control);
}

static struct shell_surface *
create_shell_surface(void *shell, struct weston_surface *surface,
			const struct weston_shell_client *client)
{
	struct shell_surface *shsurf;

	if (surface->configure) {
		weston_log("surface->configure already set\n");
		return NULL;
	}

	shsurf = calloc(1, sizeof *shsurf);
	if (!shsurf) {
		weston_log("no memory to allocate shell surface\n");
		return NULL;
	}

	surface->configure = shell_surface_configure;
	surface->configure_private = shsurf;

	shsurf->shell = (struct mobile_shell *) shell;
	shsurf->surface = surface;
	shsurf->is_visible = NULL;

	wl_signal_init(&shsurf->destroy_signal);
	shsurf->surface_destroy_listener.notify = shell_handle_surface_destroy;
	wl_signal_add(&surface->destroy_signal,
		      &shsurf->surface_destroy_listener);

	/* init link so its safe to always remove it in destroy_shell_surface */
	wl_list_init(&shsurf->link);
        /* empty when not in use */

	shsurf->client = client;

	return shsurf;
}

static void
send_configure(struct weston_surface *surface,
	       uint32_t edges, int32_t width, int32_t height)
{
	struct shell_surface *shsurf = get_shell_surface(surface);
	if (!shsurf)
		return;

	wl_shell_surface_send_configure(shsurf->resource,
					edges, width, height);
}

static const struct weston_shell_client shell_client = {
	send_configure
};

static void
switch_task(struct mobile_shell *shell)
{
	struct shell_surface *shsurf;
	struct client_task *task;
	struct weston_seat *seat;

	if (!wl_list_length(&shell->application_layer.surface_list)) {
		if (shell->menuscreen_surface) {
			wl_list_for_each(seat, &shell->compositor->seat_list, link)
				weston_surface_activate(shell->menuscreen_surface, seat);
			weston_surface_damage(shell->menuscreen_surface);
		}
		mobile_shell_set_state(shell, STATE_HOME);
		return;
	} 

	mobile_shell_set_state(shell, STATE_TASK);

	task = container_of(shell->msclient_list.task_list.next,
				struct client_task, link);
	shsurf = container_of(task->shsurf_list.next,
			struct shell_surface, link);
	if (weston_surface_is_mapped(shsurf->surface)) {
		weston_surface_damage(shsurf->surface);
		wl_list_for_each(seat, &shell->compositor->seat_list, link)
			weston_surface_activate(shsurf->surface, seat);
		weston_surface_schedule_repaint(shsurf->surface);
	}
}

static void
gohome(struct weston_seat *seat, uint32_t time, uint32_t key,
		   void *data)
{
	struct mobile_shell *shell = data;
	struct weston_surface *surface;
	struct weston_surface *next;
	struct shell_surface *shsurf;
	struct client_task *task;
	char *pstring;
	char pstring_pre[MAX_PIDS_SIZE+1];

	pstring = malloc(MAX_PIDS_SIZE);

	wl_list_for_each_safe(surface, next, &shell->application_layer.surface_list, layer_link)
	{
		wl_list_remove(&surface->layer_link);
		wl_list_insert(&shell->hide_layer.surface_list,
				&surface->layer_link);
		shsurf = get_shell_surface(surface);
		shsurf->is_visible = SURFACE_CONTROL_VISIBILITY_INVISIBLE;
		if (shsurf->surface_control_resource)
			surface_control_send_visibility_change(shsurf->surface_control_resource,
					SURFACE_CONTROL_VISIBILITY_INVISIBLE);
	}

	weston_log("%d client pids\n", wl_list_length(&shell->msclient_list.task_list));
	snprintf(pstring_pre, 2, " ");
	wl_list_for_each(task, &shell->msclient_list.task_list, link) {
		snprintf(pstring, MAX_PIDS_SIZE, " %d %s", task->pid, pstring_pre);
		snprintf(pstring_pre, MAX_PIDS_SIZE, "%s", pstring);
	}

	weston_log("string is %s\n", pstring);
	wayland_vconf_set_hide_pids(&pstring);

	if (shell->menuscreen_surface) {
		wl_list_for_each(seat, &shell->compositor->seat_list, link)
			weston_surface_activate(shell->menuscreen_surface, seat);
		weston_surface_damage(shell->menuscreen_surface);
		weston_surface_schedule_repaint(shell->menuscreen_surface);
	}
	mobile_shell_set_state(shell, STATE_HOME);

	free(pstring);

}
static void
force_kill_binding(struct weston_seat *seat, uint32_t time, uint32_t key,
		   void *data)
{
	struct mobile_shell *shell = data;
	struct wl_client *client;
	struct weston_surface *focus;
	struct shell_surface *shsurf = NULL;
	struct shell_surface *shsurf_tmp = NULL;
	pid_t pid;
	uid_t uid;
	gid_t gid;

	if (!seat->keyboard->focus)
		return;
	focus = container_of(shell->application_layer.surface_list.next,
			     struct weston_surface, layer_link);
	if (focus)
		shsurf = get_shell_surface(focus);
	if (!shsurf)
		return;

	if (shell->state != STATE_TASK)
		return;

	if (shsurf->client_task) {
		wl_list_for_each(shsurf_tmp, &shsurf->client_task->shsurf_list, link) {
			weston_surface_unmap(shsurf_tmp->surface);
			shsurf->is_visible == SURFACE_CONTROL_VISIBILITY_INVISIBLE;
		}
	}

	client = wl_resource_get_client(focus->resource);
	wl_client_get_credentials(client, &pid, &uid, &gid);

	kill(pid, SIGKILL);

	weston_compositor_schedule_repaint(shell->compositor);
}

static void
client_task_create(struct shell_surface *shsurf,
			    struct wl_client *client)
{
	struct client_task *task;
	struct mobile_shell *shell = shsurf->shell;
	pid_t pid;
	uid_t uid;
	gid_t gid;

	wl_client_get_credentials(client, &pid, &uid, &gid);
	/*check whether the client task with this pid already created*/	
	wl_list_for_each(task, &shell->msclient_list.task_list, link) {
		if (task->pid == pid) {
			shsurf->client_task = task;
		}
	}

	if (!shsurf->client_task ) {
		task = calloc(1, sizeof *task);
		wl_list_init(&task->link);
		wl_list_init(&task->shsurf_list);
		wl_list_insert(&shell->msclient_list.task_list,
				&task->link);
		task->pid = pid;
		shsurf->client_task = task;

	} else if (shsurf->client_task->pid != pid) {
		/*here should be the first time the shell surface get the task client,
		 * so this should not happen, the client task already exist, and the pid
		 * is not the same, but we can enable it in the debug mode*/
		wl_list_remove(&shsurf->client_task->link);
		free(shsurf->client_task);
		shsurf->client_task = NULL;

		task = calloc(1, sizeof *task);
		wl_list_init(&task->link);
		wl_list_init(&task->shsurf_list);
		wl_list_insert(&shell->msclient_list.task_list,
				&task->link);
		task->pid = pid;
		shsurf->client_task = task;

	} else {
		/*this task already exist, we should do nothing here*/
	}

	return;
}



static void
shell_get_shell_surface(struct wl_client *client,
			struct wl_resource *resource,
			uint32_t id,
			struct wl_resource *surface_resource)
{
	struct weston_surface *surface = surface_wl_resource_get_user_data(resource);
	struct mobile_shell *shell = wl_resource_get_user_data(resource);
	struct shell_surface *shsurf;

	if (get_shell_surface(surface)) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "mobile_shell::get_shell_surface already requested");
		return;
	}

	shsurf = create_shell_surface(shell, surface, &shell_client);
	if (!shsurf) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface->configure already set");
		return;
	}


	shsurf->resource =
		wl_resource_create(client,
				   &wl_shell_surface_interface, 1, id);
	wl_resource_set_implementation(shsurf->resource,
				       &shell_surface_implementation,
				       shsurf, shell_destroy_shell_surface);


	shsurf->surface_control_resource = NULL;
	client_task_create(shsurf, client);
	wl_list_insert(shsurf->client_task->shsurf_list.prev, &shsurf->link);
}

static const struct wl_shell_interface shell_implementation = {
	shell_get_shell_surface
};

static void
bind_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct mobile_shell *shell = data;

        shell->resource = wl_resource_create(client, &wl_shell_interface, 1, id);
	wl_resource_set_implementation(shell->resource, &shell_surface_implementation, shell, shell_destroy_shell);
}

static struct mobile_shell *
get_shell(struct weston_compositor *compositor)
{
	struct wl_listener *l;

	l = wl_signal_get(&compositor->destroy_signal, mobile_shell_destroy);
	if (l)
		return container_of(l, struct mobile_shell, destroy_listener);

	return NULL;
}

static void
mobile_shell_set_state(struct mobile_shell *shell, int state)
{
	static const char *states[] = {
		"STARTING", "HOME", "TASK"
	};

	weston_log("switching to state %s (from %s)\n",
		states[state], states[shell->state]);
	shell->previous_state = shell->state;
	shell->state = state;
}

static void
homeui_surface_configure(struct weston_surface *surface,
			       int32_t sx, int32_t sy, int32_t w, int32_t h)
{
	struct mobile_shell *shell = get_shell(surface->compositor);
	struct weston_compositor *compositor = shell->compositor;
	struct weston_seat *seat;

	if (weston_surface_is_mapped(surface))
		return;

        if (w == 0)
                return;
	weston_surface_configure(surface, 0, 0, w, h);

	/*FIXME: This is the case the homeui surfaces are unmapped.
 	  We should do something here.*/
	if (surface == shell->indicator_surface) {
		// It is added in shell_surface_set_class.
	} else if (surface == shell->menuscreen_surface) {
		// It is added in shell_surface_set_class.
	}
	weston_surface_update_transform(surface);
}

static void
put_on_output(struct weston_surface *surface, struct weston_output *output, int32_t w, int32_t h)
{
	weston_surface_configure(surface, output->x, output->y, w, h);
}

static void
input_panel_configure(struct weston_surface *surface, int32_t sx, int32_t sy, int32_t width, int32_t height)
{
	struct input_panel_surface *ip_surface = surface->configure_private;
	struct mobile_shell *shell = ip_surface->shell;
	struct weston_mode *mode;
	float x, y;
	uint32_t show_surface = 0;

	if (width == 0)
		return;

	if (!weston_surface_is_mapped(surface)) {
		if (!shell->showing_input_panels)
			return;

		show_surface = 1;
	}

	fprintf(stderr, "%s panel: %d, output: %p\n", __FUNCTION__, ip_surface->panel, ip_surface->output);

	if (ip_surface->panel) {
		x = shell->text_input.surface->geometry.x + shell->text_input.cursor_rectangle.x2;
		y = shell->text_input.surface->geometry.y + shell->text_input.cursor_rectangle.y2;
	} else {
		mode = ip_surface->output->current_mode;

		x = ip_surface->output->x + (mode->width - width) / 2;
		y = ip_surface->output->y + mode->height - height;
	}

	weston_surface_configure(surface,
				 x, y,
				 width, height);

	if (show_surface) {
		wl_list_insert(&shell->input_panel_layer.surface_list,
			       &surface->layer_link);
		weston_surface_update_transform(surface);
		weston_surface_damage(surface);
		weston_slide_run(surface, surface->geometry.height, 0, NULL, NULL);
	}
}

static void
destroy_input_panel_surface(struct input_panel_surface *input_panel_surface)
{
	wl_list_remove(&input_panel_surface->surface_destroy_listener.link);
	wl_list_remove(&input_panel_surface->link);

	input_panel_surface->surface->configure = NULL;

	free(input_panel_surface);
}

static struct input_panel_surface *
get_input_panel_surface(struct weston_surface *surface)
{
	if (surface->configure == input_panel_configure) {
		return surface->configure_private;
	} else {
		return NULL;
	}
}

static void
input_panel_handle_surface_destroy(struct wl_listener *listener, void *data)
{
	struct input_panel_surface *ipsurface = container_of(listener,
							     struct input_panel_surface,
							     surface_destroy_listener);
	if (ipsurface->resource) {
		wl_resource_destroy(ipsurface->resource);
	} else {
		destroy_input_panel_surface(ipsurface);
	}
}

static struct input_panel_surface *
create_input_panel_surface(struct mobile_shell *shell,
			   struct weston_surface *surface)
{
	struct input_panel_surface *input_panel_surface;

	input_panel_surface = calloc(1, sizeof *input_panel_surface);
	if (!input_panel_surface)
		return NULL;

	surface->configure = input_panel_configure;
	surface->configure_private = input_panel_surface;

	input_panel_surface->shell = shell;

	input_panel_surface->surface = surface;

	wl_signal_init(&input_panel_surface->destroy_signal);
	input_panel_surface->surface_destroy_listener.notify = input_panel_handle_surface_destroy;
	wl_signal_add(&surface->destroy_signal,
		      &input_panel_surface->surface_destroy_listener);

	wl_list_init(&input_panel_surface->link);

	return input_panel_surface;
}

static void
input_panel_surface_set_toplevel(struct wl_client *client,
				 struct wl_resource *resource,
				 struct wl_resource *output_resource,
				 uint32_t position)
{
	struct input_panel_surface *input_panel_surface = wl_resource_get_user_data(resource);
	struct mobile_shell *shell = input_panel_surface->shell;

	wl_list_insert(&shell->input_panel.surfaces,
		       &input_panel_surface->link);

	input_panel_surface->output = output_wl_resource_get_user_data(resource);
	input_panel_surface->panel = 0;
}

static void
input_panel_surface_set_overlay_panel(struct wl_client *client,
				      struct wl_resource *resource)
{
	struct input_panel_surface *input_panel_surface = wl_resource_get_user_data(resource);
	struct mobile_shell *shell = input_panel_surface->shell;

	wl_list_insert(&shell->input_panel.surfaces,
		       &input_panel_surface->link);

	input_panel_surface->panel = 1;
}

static const struct wl_input_panel_surface_interface input_panel_surface_implementation = {
	input_panel_surface_set_toplevel,
	input_panel_surface_set_overlay_panel
};

static void
destroy_input_panel_surface_resource(struct wl_resource *resource)
{
	struct input_panel_surface *ipsurf = wl_resource_get_user_data(resource);

	destroy_input_panel_surface(ipsurf);
}

static void
input_panel_get_input_panel_surface(struct wl_client *client,
				    struct wl_resource *resource,
				    uint32_t id,
				    struct wl_resource *surface_resource)
{
	struct weston_surface *surface = surface_wl_resource_get_user_data(resource);
	struct mobile_shell *shell = wl_resource_get_user_data(resource);
	struct input_panel_surface *ipsurf;

	if (get_input_panel_surface(surface)) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "wl_input_panel::get_input_panel_surface already requested");
		return;
	}

	ipsurf = create_input_panel_surface(shell, surface);
	if (!ipsurf) {
		wl_resource_post_error(surface_resource,
				       WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "surface->configure already set");
		return;
	}

	ipsurf->resource = wl_resource_create(client,
					      &wl_input_panel_surface_interface, 1, id);
        wl_resource_set_implementation(ipsurf->resource,
                           &input_panel_surface_implementation,
                           ipsurf,
                           destroy_input_panel_surface_resource);

}

static const struct wl_input_panel_interface input_panel_implementation = {
	input_panel_get_input_panel_surface
};

static void
unbind_input_panel(struct wl_resource *resource)
{
	struct mobile_shell *shell = wl_resource_get_user_data(resource);

	shell->input_panel.binding = NULL;
	free(resource);
}

static void
bind_input_panel(struct wl_client *client,
          void *data, uint32_t version, uint32_t id)
{
    struct mobile_shell *shell = data;
    struct wl_resource *resource;

    resource = wl_resource_create(client,
                      &wl_input_panel_interface, 1, id);

    if (shell->input_panel.binding == NULL) {
        wl_resource_set_implementation(resource,
                           &input_panel_implementation,
                           shell, unbind_input_panel);
        shell->input_panel.binding = resource;
        return;
    }

    wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                   "interface object already bound");
    wl_resource_destroy(resource);
}

static void
show_input_panels(struct wl_listener *listener, void *data)
{
	struct mobile_shell *shell =
		container_of(listener, struct mobile_shell,
			     show_input_panel_listener);
	struct input_panel_surface *surface, *next;
	struct weston_surface *ws;

	shell->text_input.surface = (struct weston_surface*)data;

	if (shell->showing_input_panels)
		return;

	shell->showing_input_panels = true;

	if (!shell->locked)
		wl_list_insert(&shell->indicator_layer.link,
			       &shell->input_panel_layer.link);

	wl_list_for_each_safe(surface, next,
			      &shell->input_panel.surfaces, link) {
		ws = surface->surface;
		if (!ws->buffer_ref.buffer)
			continue;
		wl_list_insert(&shell->input_panel_layer.surface_list,
			       &ws->layer_link);
		weston_surface_geometry_dirty(ws);
		weston_surface_update_transform(ws);
		weston_surface_damage(ws);
		weston_slide_run(ws, ws->geometry.height, 0, NULL, NULL);
	}
}

static void
hide_input_panels(struct wl_listener *listener, void *data)
{
	struct mobile_shell *shell =
		container_of(listener, struct mobile_shell,
			     hide_input_panel_listener);
	struct weston_surface *surface, *next;

	if (!shell->showing_input_panels)
		return;

	shell->showing_input_panels = false;

	if (!shell->locked)
		wl_list_remove(&shell->input_panel_layer.link);

	wl_list_for_each_safe(surface, next,
			      &shell->input_panel_layer.surface_list, layer_link)
		weston_surface_unmap(surface);
}

static void
menuscreen_task_create(struct mobile_shell *shell)
{
	struct client_task *menuscreen_task;
	menuscreen_task = calloc(1, sizeof *menuscreen_task);
	/*the pid to be added in set_class*/
	menuscreen_task->pid = -1;
	if (!menuscreen_task) {
		weston_log("no memory to allocate client task\n");
		return;
	}
	shell->msclient_list.menuscreen_task = menuscreen_task;
	wl_list_init(&menuscreen_task->link);
	wl_list_init(&menuscreen_task->shsurf_list);
}

static void
msclient_list_init(struct mobile_shell *shell)
{
	menuscreen_task_create(shell);
	wl_list_init(&shell->msclient_list.task_list);
}

static void
mobile_shell_destroy(struct wl_listener *listener, void *data)
{
	struct mobile_shell *shell =
		container_of(listener, struct mobile_shell, destroy_listener);

	if (shell->menuscreen_surface)
		shell->menuscreen_surface->configure = NULL;
	wl_list_remove(&shell->show_input_panel_listener.link);
	wl_list_remove(&shell->hide_input_panel_listener.link);

	free(shell);
}

WL_EXPORT int
module_init(struct weston_compositor *compositor, int *argc, char *argv[])
{
	struct mobile_shell *shell;
	//struct wl_event_loop *loop;

	shell = malloc(sizeof *shell);
	if (shell == NULL)
		return -1;

	memset(shell, 0, sizeof *shell);
	shell->compositor = compositor;
	compositor->shell_interface.shell = shell;

	shell->destroy_listener.notify = mobile_shell_destroy;
	wl_signal_add(&compositor->destroy_signal, &shell->destroy_listener);
	shell->show_input_panel_listener.notify = show_input_panels;
	wl_signal_add(&compositor->show_input_panel_signal,
		      &shell->show_input_panel_listener);
	shell->hide_input_panel_listener.notify = hide_input_panels;
	wl_signal_add(&compositor->hide_input_panel_signal,
		      &shell->hide_input_panel_listener);

	wl_list_init(&shell->input_panel.surfaces);
	wl_list_init(&shell->shsubsurf_list);
	/* FIXME: This will make the object available to all clients. */
	if (wl_display_add_global(compositor->wl_display, &wl_shell_interface,
				  shell, bind_shell) == NULL)
		return -1;

	if (wl_display_add_global(compositor->wl_display,
				  &wl_input_panel_interface,
				  shell, bind_input_panel) == NULL)
		return -1;

	if (wl_display_add_global(compositor->wl_display, &wl_control_interface,
                                 shell, bind_wl_control) == NULL)
               return -1;

	/* Use keyboard to simulate events */
	weston_compositor_add_key_binding(compositor, KEY_TIZEN_RETURN, 0,
					  force_kill_binding, shell);

	weston_compositor_add_key_binding(compositor, KEY_TIZEN_HOME, 0,
					  gohome, shell);

	weston_layer_init(&shell->indicator_layer,
			  &compositor->cursor_layer.link);
	weston_layer_init(&shell->application_layer,
			  &shell->indicator_layer.link);
	weston_layer_init(&shell->homescreen_layer,
			  &shell->application_layer.link);
	weston_layer_init(&shell->input_panel_layer, NULL);
	weston_layer_init(&shell->hide_layer, &shell->homescreen_layer.link);
	msclient_list_init(shell);

	mobile_shell_set_state(shell, STATE_STARTING);

	wayland_vconf_init();
	return 0;
}
