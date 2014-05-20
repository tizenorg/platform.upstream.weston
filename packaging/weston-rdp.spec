%bcond_with wayland

Name:           weston-rdp
Version:        1.4.0
Release:        0
Summary:        RDP compositor for Weston
License:        MIT
Group:          Graphics & UI Framework/Wayland Window System
Url:            http://weston.freedesktop.org/

#Git-Clone:	git://anongit.freedesktop.org/wayland/weston
#Git-Web:	http://cgit.freedesktop.org/wayland/weston/
Source0:         weston-%version.tar.xz
Source1: 	weston-rdp.manifest
BuildRequires:	autoconf >= 2.64, automake >= 1.11
BuildRequires:  expat-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libtool >= 2.2
BuildRequires:  pkgconfig
BuildRequires:  xz
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(freerdp)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(xkbcommon) >= 0.3.0
Requires:       weston

%if !%{with wayland}
ExclusiveArch:
%endif


%description
This package provides a RDP compositor allowing to do remote rendering
through the network.

%prep
%setup -q
cp %{SOURCE1} .

%build
%autogen --disable-static --disable-setuid-install --disable-weston-launch --disable-simple-clients --with-cairo=image --disable-egl --disable-clients --disable-libunwind --disable-xwayland --disable-xwayland-test --disable-x11-compositor --disable-drm-compositor --disable-fbdev-compositor --disable-headless-compositor --disable-wayland-compositor --disable-rpi-compositor --enable-rdp-compositor %{?extra_config_options:%extra_config_options}
make %{?_smp_mflags}

%install
%make_install

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%_libdir/weston/rdp-backend.so
%exclude %_bindir
%exclude %_includedir
%exclude %_prefix/lib/debug
%exclude %_libdir/pkgconfig
%exclude %_libdir/weston/desktop-shell.so
%exclude %_datadir

%changelog
