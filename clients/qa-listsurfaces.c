#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <wayland-client.h>
#include "qa-client-protocol.h"

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct qa *qa;
};


static void
surfaces_listed (void *data, struct qa *qa, const char *list)
{
	printf ("SURFACES LIST : %s\n", list);
}

static const struct qa_listener qa_listener = {
        surfaces_listed
};


static void
registry_handle_global (void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	struct display *d = data;

	if (!strcmp(interface, "qa")) {
                d->qa = wl_registry_bind (registry, id, &qa_interface, 1);
		qa_add_listener (d->qa, &qa_listener, d);
        }
}

static void
registry_handle_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};


int main (int argc, char *argv[])
{
	struct display *display;
	display = malloc (sizeof *display);

	display->display = wl_display_connect (NULL);
	assert (display->display != NULL);

	display->registry = wl_display_get_registry (display->display);
	wl_registry_add_listener (display->registry, &registry_listener, display);

	wl_display_roundtrip (display->display);
	assert (display->qa != NULL);


	qa_list_surfaces (display->qa);

        int result = 0;
        while (result != -1)
                result = wl_display_dispatch (display->display);

	qa_destroy (display->qa);
	wl_registry_destroy (display->registry);
	wl_display_flush (display->display);
	wl_display_disconnect (display->display);


	return 0;
}
