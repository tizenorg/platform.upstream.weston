export ELM_ENGINE=wayland_egl
export ECORE_EVAS_ENGINE=wayland_egl

# also export dbus session address for dbus clients (details on bug TIVI-1686 [https://bugs.tizen.org/jira/browse/TIVI-1686])
export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$UID/dbus/user_bus_socket
