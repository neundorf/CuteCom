#
# spec file for package cutecom
#
# Copyright (c) 2015 SUSE LINUX GmbH, Nuernberg, Germany.
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
Version:        0.30.4
Release:        0
Url:            http://github.com/neundorf/CuteCom
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  libqt5-qtbase-devel libqt5-qtserialport-devel
Summary:        Serial terminal
License:        GPL-3.0+
Group:          Applications/Communications
Source:         %{name}-%{version}.tgz

%description
Qt5 based serial terminal

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
install -m 644 distribution/openSUSE/cutecom.desktop $RPM_BUILD_ROOT%{_datadir}/applications/
install -d 755 $RPM_BUILD_ROOT%{_datadir}/pixmaps
install -m 644 images/cutecom.svg $RPM_BUILD_ROOT%{_datadir}/pixmaps/

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_mandir}/man1/%{name}.1.gz
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/cutecom.svg
%doc Changelog TODO LICENSE CREDITS README.md

%changelog

