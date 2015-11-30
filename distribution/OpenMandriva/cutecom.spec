%define ver 0.20.0
%define rel 1

Name:		cutecom
Version:	0.22.0
Release:	%mkrel %rel
URL:		http://cutecom.sourceforge.net/
Summary:	Graphical serial terminal program
License:	GPL
Group:		Communications
Source:		http://cutecom.sourceforge.net/%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-root
BuildRequires:	qt4-devel desktop-file-utils cmake

%description
CuteCom is a graphical serial terminal, like minicom. It is aimed mainly at
hardware developers or other people who need a terminal to talk to their
devices. 

%prep
%setup -q

%build
%cmake
%make

%install
rm -Rf %{buildroot}
%makeinstall_std -C build

desktop-file-install --vendor="" \
  --remove-category="Application" \
  --add-category="System;Settings" \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications cutecom.desktop

%clean
rm -Rf %{buildroot}

%if %mdkversion < 200900
%post
%{update_menus}
%endif

%if %mdkversion < 200900
%postun
%{clean_menus}
%endif

%files
%defattr(-,root,root)
%{_bindir}/cutecom
%{_datadir}/applications/cutecom.desktop
%{_mandir}/man1/cutecom.1*
%doc README Changelog


%changelog
* Sun Aug 22 2010 Funda Wang <fwang@mandriva.org> 0.22.0-1mdv2011.0
+ Revision: 571881
- use standard cmake macro

* Tue Jul 14 2009 Frederik Himpe <fhimpe@mandriva.org> 0.22.0-1mdv2010.0
+ Revision: 396004
- update to new version 0.22.0

* Thu May 14 2009 Frederik Himpe <fhimpe@mandriva.org> 0.21.0-1mdv2010.0
+ Revision: 375720
- update to new version 0.21.0

* Sat Jun 21 2008 Buchan Milne <bgmilne@mandriva.org> 0.20.0-1mdv2009.0
+ Revision: 227733
- New version 0.20.0 (qt4)

  + Pixel <pixel@mandriva.com>
    - rpm filetriggers deprecates update_menus/update_scrollkeeper/update_mime_database/update_icon_cache/update_desktop_database/post_install_gconf_schemas

  + Olivier Blin <oblin@mandriva.com>
    - restore BuildRoot

  + Thierry Vignaud <tv@mandriva.org>
    - kill re-definition of %%buildroot on Pixel's request

* Thu Sep 06 2007 Emmanuel Andry <eandry@mandriva.org> 0.14.1-2mdv2008.0
+ Revision: 81134
- xdg menu
- drop old menu


* Sun Jan 28 2007 Buchan Milne <bgmilne@mandriva.org> 0.14.1-1mdv2007.0
+ Revision: 114553
- New version 0.14.1
- Import cutecom

* Sun Aug 20 2006 Buchan Milne <bgmilne@mandriva.org> 0.14.0-1mdv2007.0
- 0.14.0

* Tue Jan 31 2006 Buchan Milne <bgmilne@mandriva.org> 0.13.2-2mdk
- buildrequire mandriva-create-kde-mdk-menu for kdedesktop2mdkmenu.pl
- buildrequires kdelibs-common for kdeconfig (placement of .desktop file)

* Mon Jan 30 2006 Buchan Milne <bgmilne@mandriva.org> 0.13.2-1mdk
- initial package

