# Purpose:  The .spec file for building Mahogany RPM
# Version:  $Id$

# version and release
%define VERSION 0.62
%define RELEASE 0

# default installation directory
%define prefix /usr/local

# we can build 2 different packages from this spec file: a semistatically
# linked version (only common libs linked dynamically) or a quartstatically
# one (only wxGTK linked statically, GTK libs dynamically)
#
# not used any more
#
#%define MAKETARGET semistatic
#%define MAKETARGET quartstatic

Summary: Mahogany email and news client
Name: mahogany
Version: %VERSION
Release: %RELEASE
Copyright: Mahogany Artistic License or GPL
Group: X11/Applications/Networking
Source: ftp://ftp.sourceforge.net/pub/sourceforge/mahogany/mahogany-%{VERSION}.tar.gz
URL: http://mahogany.sourceforge.net/
Packager: Vadim Zeitlin <vadim@wxwindows.org>
Provides: mua
Requires: wxwin

Prefix: %prefix
BuildRoot: /var/tmp/%{name}-root
Icon: mahogany.gif

%description
This package contains Mahogany, a powerful, scriptable GUI mail and news client
using GTK+ toolkit. Mahogany supports remote POP3, IMAP4, NNTP servers as well
as local MBOX and news spool folders and sending mail using SMTP or local MTA.

%prep
# the name is the same whether the package name is mahogany or mahogany-dynamic
%setup -n mahogany-%{VERSION}

%build
if [ ! -f configure ]; then
  autoconf
fi

CFLAGS="$RPM_OPT_FLAGS" ./configure --without-threads \
	--prefix=$RPM_BUILD_ROOT/%prefix

# we have to fix M_PREFIX in config.h because the package will be later
# installed in just %prefix, so fallback paths hardcoded into the binary
# shouldn't contain RPM_BUILD_ROOT
sed "s@$RPM_BUILD_ROOT/@@g" include/config.h > include/config.h.new &&
mv include/config.h.new include/config.h

# if MAKE is not set, find the best value ourselves
if [ "x$MAKE" = "x" ]; then
  if [ "$SMP" != "" ]; then
    export MAKE="make -j $SMP"
  else
    export MAKE=make
  fi
fi

make

%install
export PATH=/sbin:$PATH
make -k install
make install_rpm

%clean
rm -rf $RPM_BUILD_ROOT

%post

echo -e "# added by rpm installation\nGlobalDir=%prefix" >> %prefix/share/Mahogany/M.conf

%postun

%files -f filelist
