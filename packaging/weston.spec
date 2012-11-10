Name:           weston
%define lname	libweston
Version:        1.0.0
Release:        0
Summary:        Wayland Compositor Infrastructure
License:        MIT
Group:          Development/Libraries/C and C++
Url:            http://weston.freedesktop.org/

#Git-Clone:	git://anongit.freedesktop.org/wayland/weston
#Git-Web:	http://cgit.freedesktop.org/wayland/weston/
Source:         %name-%version.tar.xz
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:	autoconf >= 2.64, automake >= 1.11
BuildRequires:  gcc-c++
BuildRequires:  expat-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libtool >= 2.2
BuildRequires:  libvpx-devel
BuildRequires:  pam-devel
BuildRequires:  pkgconfig
#BuildRequires:  rsvg-view
BuildRequires:  xz
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(cairo-egl) >= 1.11.3
BuildRequires:	pkgconfig(cairo-xcb)
BuildRequires:  pkgconfig(egl) >= 7.10
BuildRequires:  pkgconfig(gbm)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(libdrm) >= 2.4.30
BuildRequires:  pkgconfig(libffi)
BuildRequires:  pkgconfig(libsystemd-login)
BuildRequires:  pkgconfig(libudev) >= 136
BuildRequires:  pkgconfig(mtdev) >= 1.1.0
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(poppler-glib)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-egl)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(xkbcommon) >= 0.0.578
BuildRequires:	pkgconfig(xcb)
BuildRequires:	pkgconfig(xcb-xfixes)
BuildRequires:	pkgconfig(xcursor)
BuildRequires:  pkgconfig(glu) >= 9.0.0

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

%prep
%setup -q

%build
%autogen
%configure --disable-static --disable-setuid-install  --enable-simple-clients --enable-clients
make %{?_smp_mflags};

%install
%make_install

%files
%defattr(-,root,root)
%_bindir/wcap-*
%_bindir/weston*
%_libexecdir/weston-*
%_libdir/weston
%_datadir/weston
/usr/share/man/man1/weston.1.gz

%changelog
