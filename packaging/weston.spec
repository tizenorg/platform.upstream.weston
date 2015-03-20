%bcond_with wayland
%bcond_with libva
%bcond_with mobile
%bcond_with rdp

%if %{with mobile}
%define extra_config_options1 --disable-drm-compositor
%endif

%if %{with rdp}
%define extra_config_options2 --enable-rdp-compositor --enable-screen-sharing
%endif

%if "%{profile}" == "common"
%define extra_config_options3 --enable-sys-uid --disable-ivi-shell
%endif

%if "%{profile}" == "tv"
%define extra_config_options4 --enable-sys-uid --disable-ivi-shell
%endif

%if "%{profile}" == "mobile"
%define extra_config_options5 --enable-sys-uid --disable-ivi-shell
%endif

Name:           weston
Version:        1.7.0
Release:        0
Summary:        Wayland Compositor Infrastructure
License:        MIT
Group:          Graphics & UI Framework/Wayland Window System
Url:            http://weston.freedesktop.org/

#Git-Clone:	git://anongit.freedesktop.org/wayland/weston
#Git-Web:	http://cgit.freedesktop.org/wayland/weston/
Source0:        %name-%version.tar.xz
Source1:        %name.ini
Source1001: 	%name.manifest
BuildRequires:	autoconf >= 2.64, automake >= 1.11
BuildRequires:  expat-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libtool >= 2.2
BuildRequires:  libvpx-devel
BuildRequires:  pam-devel
BuildRequires:  pkgconfig
BuildRequires:  xz
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(cairo-egl) >= 1.11.3
BuildRequires:  pkgconfig(egl) >= 7.10
%if %{with rdp}
BuildRequires:  pkgconfig(freerdp)
%endif
BuildRequires:  pkgconfig(gbm)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
%if !%{with mobile}
BuildRequires:  pkgconfig(libdrm) >= 2.4.30
%endif
BuildRequires:  pkgconfig(libffi)
BuildRequires:  pkgconfig(libinput) >= 0.8.0
BuildRequires:  pkgconfig(libsystemd-login)
BuildRequires:  pkgconfig(libudev) >= 136
%if %{with libva}
BuildRequires:  pkgconfig(libva)
%endif
BuildRequires:  pkgconfig(mtdev) >= 1.1.0
BuildRequires:  pkgconfig(pangocairo)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(poppler-glib)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-egl)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(xkbcommon) >= 0.3.0
Requires:       weston-startup
Requires(pre):  /usr/sbin/groupadd

%if !%{with wayland}
ExclusiveArch:
%endif

%description
Weston is the reference implementation of a Wayland compositor, and a
useful compositor in its own right. Weston has various backends that
lets it run on Linux kernel modesetting and evdev input as well as
under X11. Weston ships with a few example clients, from simple
clients that demonstrate certain aspects of the protocol to more
complete clients and a simplistic toolkit. There is also a quite
capable terminal emulator (weston-terminal) and an toy/example
desktop shell. Finally, weston also provides integration with the
Xorg server and can pull X clients into the Wayland desktop and act
as a X window manager.

%package devel
Summary: Development files for package %{name}
Group:   Graphics & UI Framework/Development
%description devel
This package provides header files and other developer releated files
for package %{name}.

%package clients
Summary: Sample clients for package %{name}
Group:   Graphics & UI Framework/Development
%description clients
This package provides a set of example wayland clients useful for
validating the functionality of wayland with very little dependencies
on other system components.

%if %{with rdp}
%package rdp
Summary: RDP compositor for %{name}
Group:   Graphics & UI Framework/Development
%description rdp
This package provides a RDP compositor allowing to do remote rendering
through the network.
%endif

%if "%{profile}" == "ivi"
%package ivi-shell
Summary: %{name} IVI Shell
Group:   Graphics & UI Framework/Wayland Window System
%description ivi-shell
A reference Weston shell designed for use in IVI systems.

%package ivi-shell-config
Summary: Tizen IVI %{name} configuration
Group:   Automotive/Configuration
Conflicts: weston-ivi-config
Conflicts: ico-uxf-weston-plugin
%description ivi-shell-config
This package contains Tizen IVI-specific configuration.
%endif

%prep
%setup -q
cp %{SOURCE1001} .

%build
%autogen --disable-static \
         --disable-setuid-install \
         --enable-simple-clients \
         --enable-clients \
         --disable-libunwind \
         --disable-xwayland \
         --disable-xwayland-test \
         --disable-x11-compositor \
         --disable-rpi-compositor \
         --with-cairo=glesv2 \
         %{?extra_config_options1:%extra_config_options1} \
         %{?extra_config_options2:%extra_config_options2} \
         %{?extra_config_options3:%extra_config_options3} \
         %{?extra_config_options4:%extra_config_options4} \
         %{?extra_config_options5:%extra_config_options5}

make %{?_smp_mflags}

%install
%make_install

# install example clients
install -m 755 weston-calibrator %{buildroot}%{_bindir}
install -m 755 weston-simple-touch %{buildroot}%{_bindir}
install -m 755 weston-simple-shm %{buildroot}%{_bindir}
install -m 755 weston-simple-egl %{buildroot}%{_bindir}
install -m 755 weston-simple-damage %{buildroot}%{_bindir}
install -m 755 weston-presentation-shm %{buildroot}%{_bindir}
install -m 755 weston-nested-client %{buildroot}%{_bindir}
install -m 755 weston-nested %{buildroot}%{_bindir}
install -m 755 weston-multi-resource %{buildroot}%{_bindir}
install -m 755 weston-flower %{buildroot}%{_bindir}
install -m 755 weston-image %{buildroot}%{_bindir}
install -m 755 weston-cliptest %{buildroot}%{_bindir}
install -m 755 weston-dnd %{buildroot}%{_bindir}
install -m 755 weston-editor %{buildroot}%{_bindir}
install -m 755 weston-stacking %{buildroot}%{_bindir}
install -m 755 weston-smoke %{buildroot}%{_bindir}
install -m 755 weston-scaler %{buildroot}%{_bindir}
install -m 755 weston-resizor %{buildroot}%{_bindir}
install -m 755 weston-eventdemo %{buildroot}%{_bindir}
install -m 755 weston-clickdot %{buildroot}%{_bindir}
install -m 755 weston-subsurfaces %{buildroot}%{_bindir}
install -m 755 weston-transformed %{buildroot}%{_bindir}
install -m 755 weston-fullscreen %{buildroot}%{_bindir}

%if "%{profile}" == "ivi"
mkdir -p %{buildroot}%{_sysconfdir}/xdg
install -m 644 %{SOURCE1} %{buildroot}%{_sysconfdir}/xdg
%endif

# The weston.service unit file must be provided by the weston-startup
# virtual package, i.e. "Provide: weston-startup".  The weston-startup
# virtual package requirement is intended to force Tizen profile
# maintainers to add the necessary start-up script or systemd unit
# file to start weston. Otherwise it becomes possible to install
# weston without an automated means to start weston at boot, which may
# lead to confusion.  This approach allows startup related files to be
# maintained outside of this weston package.

rm -rf %{buildroot}%{_datadir}/wayland-sessions

%pre
getent group weston-launch >/dev/null || %{_sbindir}/groupadd -o -r weston-launch

%docs_package

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%{_bindir}/wcap-*
%{_bindir}/weston
%{_bindir}/weston-info
%attr(4755,root,root) %{_bindir}/weston-launch
%{_bindir}/weston-terminal
%{_libexecdir}/weston-*
%{_libdir}/weston/desktop-shell.so
%if !%{with mobile}
%{_libdir}/weston/drm-backend.so
%endif
%{_libdir}/weston/fbdev-backend.so
%{_libdir}/weston/fullscreen-shell.so
%{_libdir}/weston/headless-backend.so
%{_libdir}/weston/wayland-backend.so
%{_libdir}/weston/gl-renderer.so
%{_datadir}/weston
# exclude ivi-shell-specific files
%exclude %{_libexecdir}/weston-ivi-shell-user-interface
%exclude %{_datadir}/weston/background.png
%exclude %{_datadir}/weston/panel.png
%exclude %{_datadir}/weston/tiling.png
%exclude %{_datadir}/weston/sidebyside.png
%exclude %{_datadir}/weston/fullscreen.png
%exclude %{_datadir}/weston/random.png
%exclude %{_datadir}/weston/home.png
%exclude %{_datadir}/weston/icon_ivi_simple-egl.png
%exclude %{_datadir}/weston/icon_ivi_simple-shm.png
%exclude %{_datadir}/weston/icon_ivi_smoke.png
%exclude %{_datadir}/weston/icon_ivi_flower.png
%exclude %{_datadir}/weston/icon_ivi_clickdot.png

%files devel
%manifest %{name}.manifest
%{_includedir}/weston/*.h
%{_libdir}/pkgconfig/*.pc

%files clients
%manifest %{name}.manifest
%{_bindir}/weston-simple-touch
%{_bindir}/weston-simple-shm
%{_bindir}/weston-simple-egl
%{_bindir}/weston-simple-damage
%{_bindir}/weston-presentation-shm
%{_bindir}/weston-nested-client
%{_bindir}/weston-nested
%{_bindir}/weston-multi-resource
%{_bindir}/weston-flower
%{_bindir}/weston-image
%{_bindir}/weston-cliptest
%{_bindir}/weston-dnd
%{_bindir}/weston-editor
%{_bindir}/weston-stacking
%{_bindir}/weston-smoke
%{_bindir}/weston-scaler
%{_bindir}/weston-resizor
%{_bindir}/weston-eventdemo
%{_bindir}/weston-clickdot
%{_bindir}/weston-subsurfaces
%{_bindir}/weston-transformed
%{_bindir}/weston-fullscreen
%{_bindir}/weston-calibrator

%if %{with rdp}
%files rdp
%manifest %{name}.manifest
%{_libdir}/weston/rdp-backend.so
%{_libdir}/weston/screen-share.so
%endif

%if "%{profile}" == "ivi"
%files ivi-shell
%manifest %{name}.manifest
%{_libdir}/weston/hmi-controller.so
%{_libdir}/weston/ivi-shell.so
%{_libexecdir}/weston-ivi-shell-user-interface
%{_datadir}/weston/background.png
%{_datadir}/weston/panel.png
%{_datadir}/weston/tiling.png
%{_datadir}/weston/sidebyside.png
%{_datadir}/weston/fullscreen.png
%{_datadir}/weston/random.png
%{_datadir}/weston/home.png
%{_datadir}/weston/icon_ivi_simple-egl.png
%{_datadir}/weston/icon_ivi_simple-shm.png
%{_datadir}/weston/icon_ivi_smoke.png
%{_datadir}/weston/icon_ivi_flower.png
%{_datadir}/weston/icon_ivi_clickdot.png

%files ivi-shell-config
%manifest %{name}.manifest
%config  %{_sysconfdir}/xdg/weston.ini
%endif

%changelog
