# Purpose:  The .spec file for building Mahogany RPM
# Version:  $Id$

# version and release
%define VERSION 0.22a
%define RELEASE 1

# so far I didn't find how to make a relocatable package...
%define prefix /seth/zeitlin/test

Summary: Mahogany email and news client
Name: mahogany
Version: %VERSION
Release: %RELEASE
Copyright: Mahogany Artistic License
Group: X11/Applications/Networking
Source: ftp://ronnie.phy.hw.ac.uk/pub/Mahogany/mahogany-%{VERSION}.tar.gz
URL: http://mahogany.home.dhs.org/
Packager: Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
Requires: gtk+ >= 1.0.6
Provides: mua
Prefix: %prefix
Icon: mahogany.gif

%description
This package contains Mahogany, a powerful, scriptable GUI mail and news client
using GTK+ toolkit. Mahogany supports remote POP3, IMAP4, NNTP servers as well
as local MBOX and news spool folders and sending mail using SMTP.

%prep
%setup

%build
if [ ! -f configure ]; then
  autoconf
fi

CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%prefix

make dep
make clean

if [ "$SMP" != "" ]; then
  export MAKE="make -j $SMP"
fi

(cd extra && make all && cd ../src && make semistatic)

(make doc; exit 0)

%install
export PATH=/sbin:$PATH
make -k install

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%doc README TODO
/%prefix/bin/M
/%prefix/share/M

