# Purpose:  The .spec file for building Mahogany RPM
# Version:  $Id$

# version and release
%define VERSION 0.22a
%define RELEASE 1

# so far I didn't find how to make a relocatable package...
%define prefix /usr/local

# the location of the global config file (subject to change)
%define CONFFILE %prefix/share/M.conf

Summary: Mahogany email and news client
Name: mahogany
Version: %VERSION
Release: %RELEASE
Copyright: Mahogany Artistic License
Group: X11/Applications/Networking
Source: ftp://ronnie.phy.hw.ac.uk/pub/Mahogany/mahogany-%{VERSION}.tar.gz
URL: http://mahogany.home.dhs.org/
Packager: Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
# although we can build M with any version of GTK from 1.0.6 till 1.2.3, the
# binary package will depend on the version we built it with! How to deal with
# it?
Requires: gtk+ >= 1.2.2
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

CFLAGS="$RPM_OPT_FLAGS" ./configure --without-threads --prefix=%prefix

make dep
make clean

if [ "$SMP" != "" ]; then
  export MAKE="make -j $SMP"
fi

(cd extra && make all && cd ../src && make semistatic)

(make doc; exit 0)

%install
export PATH=/sbin:$PATH
make -k install_all
cat > %CONFFILE <<END
# this is the global config file for Mahogany, it is created
# empty but you may put any global settings into it
END

ln -sf %prefix/share/Mahogany/doc %prefix/doc/Mahogany

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
%doc %prefix/doc/Mahogany
%config %CONFFILE
%prefix/bin/M
%prefix/bin/mahogany
%prefix/share/Mahogany
