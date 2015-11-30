#
# spec file for package cutecom
#
# Copyright (c) 2012 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#



Name:           cutecom
Version:        0.23.0
Release:        15.0.1
Url:            http://cutecom.sourceforge.net/
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  libqt5-qtbase-devel
Summary:        Serial terminal
License:        GPL-2.0+
Group:          Applications/Communications
Source:         %{name}-%{version}.tar.gz
Source3:        %{name}.desktop
Source4:        %{name}.png

%if 0%{?suse_version}
# for >= 11.1 validity tests
BuildRequires:  gnome-icon-theme
%endif

%description
Qt based serial terminal

%prep
%setup

%build
cmake .
make

%install
mkdir -p $RPM_BUILD_ROOT/%{_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_mandir}/man1
install -s -m 755 %{name} $RPM_BUILD_ROOT/%{_bindir}/
gzip %{name}.1
install -m 644 %{name}.1.gz $RPM_BUILD_ROOT/%{_mandir}/man1/

install -d 755 $RPM_BUILD_ROOT%{_datadir}/applications
install -m 644 %SOURCE3 $RPM_BUILD_ROOT%{_datadir}/applications/
install -d 755 $RPM_BUILD_ROOT%{_datadir}/pixmaps
install -m 644 %SOURCE4 $RPM_BUILD_ROOT%{_datadir}/pixmaps/

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_mandir}/man1/%{name}.1.gz
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png
%doc Changelog TODO COPYING README

%changelog
* Tue Oct 27 2015 cyc1ingsir@gmail.com
- version 0.23 based on Qt5 and command line sessions
* Thu Feb 23 2012 opensuse@dstoecker.de
- display correct license in about dialog
* Thu Jan 12 2012 coolo@suse.com
- change license to be in spdx.org format
* Tue Apr 26 2011 opensuse@dstoecker.de
- fix line breaking issue (patch copied from Debian)
* Tue Jan 27 2009 opensuse@dstoecker.de
- initial setup
