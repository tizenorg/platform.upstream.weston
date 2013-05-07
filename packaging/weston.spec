%define _unitdir_user /usr/lib/systemd/user

Name:           weston
Version:        1.1.1
Release:        0
Summary:        Wayland Compositor Infrastructure
License:        MIT
Group:          Graphics/Wayland Window System
Url:            http://weston.freedesktop.org/

#Git-Clone:	git://anongit.freedesktop.org/wayland/weston
#Git-Web:	http://cgit.freedesktop.org/wayland/weston/
Source0:         %name-%version.tar.xz
Source1:        weston.service
Source2:        weston.target
Source3:        99-vtc1000-quirk.rules
Source4:        99-chelong-quirk.rules
BuildRequires:	autoconf >= 2.64, automake >= 1.11
BuildRequires:  gcc-c++
BuildRequires:  expat-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libtool >= 2.2
BuildRequires:  libvpx-devel
BuildRequires:  pam-devel
BuildRequires:  pkgconfig
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
Requires(pre):  /usr/sbin/groupadd

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
Group: Development/Libraries
%description devel
This package provides header files and other developer releated files for package %{name}.


%prep
%setup -q

%build
%autogen --disable-static --disable-setuid-install  --enable-simple-clients --enable-clients --disable-libunwind
make %{?_smp_mflags};

%install
%make_install

install -d %{buildroot}/%{_unitdir_user}/weston.target.wants
install -m 644 %{SOURCE1} %{buildroot}%{_unitdir_user}/weston.service
install -m 644 %{SOURCE2} %{buildroot}%{_unitdir_user}/weston.target
ln -sf ../weston.service %{buildroot}/%{_unitdir_user}/weston.target.wants/
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/udev/rules.d/
install -m 0644 %{SOURCE3} $RPM_BUILD_ROOT/%{_sysconfdir}/udev/rules.d/
install -m 0644 %{SOURCE4} $RPM_BUILD_ROOT/%{_sysconfdir}/udev/rules.d/

%pre
getent group weston-launch >/dev/null || %{_sbindir}/groupadd -o -r weston-launch

%docs_package 

%files
%defattr(-,root,root)
%license COPYING
%_bindir/wcap-*
%_bindir/weston
%_bindir/weston-info
%attr(4755,root,root) %{_bindir}/weston-launch
%{_bindir}/weston-terminal
%_libexecdir/weston-*
%_libdir/weston
%_datadir/weston
%{_unitdir_user}/weston.service
%{_unitdir_user}/weston.target
%{_unitdir_user}/weston.target.wants/weston.service
%{_sysconfdir}/udev/rules.d/*


%files devel
%_includedir/weston/*.h
%_libdir/pkgconfig/*.pc

%changelog
