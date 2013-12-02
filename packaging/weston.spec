%bcond_with mobile
%define _unitdir_user /usr/lib/systemd/user

Name:           weston
Version:        1.3.1
Release:        0
Summary:        Wayland Compositor Infrastructure
License:        MIT
Group:          Graphics & UI Framework/Wayland Window System
Url:            http://weston.freedesktop.org/

#Git-Clone:	git://anongit.freedesktop.org/wayland/weston
#Git-Web:	http://cgit.freedesktop.org/wayland/weston/
Source0:         %name-%version.tar.xz
Source1:        weston.service
Source2:        weston.target
Source3:        99-egalax.rules
Source4:        weston.sh
Source5:        terminal.xml
Source6:        browser.xml
Source7:        browser.png
Source8:        browser
Source9:        weekeyboard.xml
Source1001: 	weston.manifest
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
BuildRequires:  pkgconfig(xkbcommon) >= 0.3.0
BuildRequires:  pkgconfig(glu) >= 9.0.0
Requires(pre):  /usr/sbin/groupadd
Requires(post): /usr/bin/pkg_initdb


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
This package provides header files and other developer releated files for package %{name}.

%package clients
Summary: Sample clients for package %{name}
Group:   Graphics & UI Framework/Development
%description clients
This package provides a set of example wayland clients useful for validating the functionality of wayland
with very little dependencies on other system components

%if %{with mobile}
%package -n mobile-shell
Summary:    mobile-shell
Group:      Graphics & UI Framework
BuildRequires: pkgconfig(weston) > 1.2.2
BuildRequires: pkgconfig(pixman-1)
BuildRequires: pkgconfig(xkbcommon) >= 0.0.578
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(vconf)

%description -n mobile-shell
Weston Plugins for Mobile


%package -n mobile-shell-devel
Summary:    Development files for mobile-shell
Group:      Graphics & UI Framework/Development
%description -n mobile-shell-devel
Development files that expose the wayland extended protocols for Mobile.
%endif

%prep
%setup -q
cp %{SOURCE1001} .

%build
%if %{with mobile}
%autogen --disable-static --disable-setuid-install  --enable-simple-clients --enable-clients --disable-libunwind --disable-xwayland --disable-xwayland-test --enable-mobile-shell
%else
%autogen --disable-static --disable-setuid-install  --enable-simple-clients --enable-clients --disable-libunwind --disable-xwayland --disable-xwayland-test
%endif
make %{?_smp_mflags};

%install
%make_install

# install tizen package metadata for weston-terminal
mkdir -p %{buildroot}%{_datadir}/packages/
mkdir -p %{buildroot}%{_datadir}/icons/default/small
install -m 0644 %{SOURCE5} %{buildroot}%{_datadir}/packages/terminal.xml
ln -sf %{_datadir}/weston/terminal.png %{buildroot}%{_datadir}/icons/default/small/

# install browser package metadata for MiniBrowser
install -m 0644 %{SOURCE6} %{buildroot}%{_datadir}/packages/browser.xml
cp %{SOURCE7} %{buildroot}%{_datadir}/icons/default/small/
install -m 755 %{SOURCE8} %{buildroot}%{_bindir}/browser

# install tizen package metadata for weekeyboard
install -m 0644 %{SOURCE9} %{buildroot}%{_datadir}/packages/weekeyboard.xml

# install example clients
install -m 755 clients/weston-simple-touch %{buildroot}%{_bindir}
install -m 755 clients/weston-simple-shm %{buildroot}%{_bindir}
install -m 755 clients/weston-simple-egl %{buildroot}%{_bindir}
install -m 755 clients/weston-flower %{buildroot}%{_bindir}
install -m 755 clients/weston-image %{buildroot}%{_bindir}
install -m 755 clients/weston-cliptest %{buildroot}%{_bindir}
install -m 755 clients/weston-dnd %{buildroot}%{_bindir}
install -m 755 clients/weston-smoke %{buildroot}%{_bindir}
install -m 755 clients/weston-resizor %{buildroot}%{_bindir}
install -m 755 clients/weston-eventdemo %{buildroot}%{_bindir}
install -m 755 clients/weston-clickdot %{buildroot}%{_bindir}
install -m 755 clients/weston-transformed %{buildroot}%{_bindir}
install -m 755 clients/weston-fullscreen %{buildroot}%{_bindir}
install -m 755 clients/weston-calibrator %{buildroot}%{_bindir}

install -d %{buildroot}/%{_unitdir_user}/weston.target.wants
install -m 644 %{SOURCE1} %{buildroot}%{_unitdir_user}/weston.service
install -m 644 %{SOURCE2} %{buildroot}%{_unitdir_user}/weston.target
ln -sf ../weston.service %{buildroot}/%{_unitdir_user}/weston.target.wants/

mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/udev/rules.d/
install -m 0644 %{SOURCE3} $RPM_BUILD_ROOT/%{_sysconfdir}/udev/rules.d/

mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/profile.d/
install -m 0644 %{SOURCE4} $RPM_BUILD_ROOT/%{_sysconfdir}/profile.d/

%if %{with mobile}
# configurations
%define weston_conf %{_sysconfdir}/xdg/weston
mkdir -p %{buildroot}%{weston_conf} > /dev/null 2>&1
install -m 0644 src/mobile-shell/weston.ini %{buildroot}%{weston_conf}
%endif

%pre
getent group weston-launch >/dev/null || %{_sbindir}/groupadd -o -r weston-launch

%post
/usr/bin/pkg_initdb

%docs_package

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%_bindir/wcap-*
%_bindir/weston
%_bindir/weston-info
%_bindir/browser
%attr(4755,root,root) %{_bindir}/weston-launch
%{_bindir}/weston-terminal
%_libexecdir/weston-*
%_libdir/weston
%_datadir/weston
%{_unitdir_user}/weston.service
%{_unitdir_user}/weston.target
%{_unitdir_user}/weston.target.wants/weston.service
%{_sysconfdir}/udev/rules.d/99-egalax.rules
%{_sysconfdir}/profile.d/*
%{_datadir}/packages/*.xml
%{_datadir}/icons/default/small/*.png

%files devel
%manifest %{name}.manifest
%_includedir/weston/*.h
%_libdir/pkgconfig/*.pc

%files clients
%manifest %{name}.manifest
%_bindir/weston-simple-touch
%_bindir/weston-simple-shm
%_bindir/weston-simple-egl
%_bindir/weston-flower
%_bindir/weston-image
%_bindir/weston-cliptest
%_bindir/weston-dnd
%_bindir/weston-smoke
%_bindir/weston-resizor
%_bindir/weston-eventdemo
%_bindir/weston-clickdot
%_bindir/weston-transformed
%_bindir/weston-fullscreen
%_bindir/weston-calibrator

%if %{with mobile}
%files -n mobile-shell
%defattr(-,root,root,-)
%dir %{_libdir}/weston/
%{_libdir}/weston/mobile-shell.so
%{_libdir}/libmobile-shell-client.so.*
%{weston_conf}/weston.ini

%files -n mobile-shell-devel
%defattr(-,root,root,-)
%{_includedir}/%{name}/input-method-client-protocol.h
%{_includedir}/%{name}/mobile-shell-client-protocol.h
%{_libdir}/libmobile-shell-client.so
%endif

%changelog
