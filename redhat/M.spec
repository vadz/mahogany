# Purpose:  The .spec file for building Mahogany RPM
# Version:  $Id$

# version and release
%define VERSION 0.60
%define RELEASE 1

# default installation directory
%define prefix /usr/local

# we can build 2 different packages from this spec file: a semistatically
# linked version (only common libs linked dynamically) or a quartstatically
# one (only wxGTK linked statically, GTK libs dynamically)
#%define MAKETARGET semistatic
%define MAKETARGET quartstatic

Summary: Mahogany email and news client
Name: mahogany
Version: %VERSION
Release: %RELEASE
Copyright: Mahogany Artistic License
Group: X11/Applications/Networking
Source: ftp://ftp.wxwindows.org/pub/M/%{VERSION}/mahogany-%{VERSION}.tar.gz
URL: http://mahogany.home.dhs.org/
Packager: Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
Provides: mua
Prefix: %prefix
BuildRoot: /var/tmp/%{name}-root
Icon: mahogany.gif

%description
This package contains Mahogany, a powerful, scriptable GUI mail and news client
using GTK+ toolkit. Mahogany supports remote POP3, IMAP4, NNTP servers as well
as local MBOX and news spool folders and sending mail using SMTP.

%prep
# the name is the same whether the package name is mahogany or mahogany-dynamic
%setup -n mahogany-%{VERSION}

%build
(cd include ; make -i ) || true
make -i allclean || true
if [ ! -f configure ]; then
  autoconf
fi

CFLAGS="$RPM_OPT_FLAGS" ./configure --without-threads \
	--prefix=$RPM_BUILD_ROOT/%prefix

(cd include ; make ) || true
make -i depend || true
make -i clean

if [ "$SMP" != "" ]; then
  export MAKE="make -j $SMP"
fi

(make -C include && make all && cd ../src && make %MAKETARGET)

(make doc; exit 0)

%install
export PATH=/sbin:$PATH
make -k install
make install_rpm

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files -f filelist
