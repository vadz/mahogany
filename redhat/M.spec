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
Source: ftp://mahogany.sourceforge.net/pub/%{VERSION}/mahogany-%{VERSION}.tar.gz
URL: http://mahogany.sourceforge.net/
Packager: Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
Provides: mua
Requires: libwx_gtk-2.2.so.0

# wxGTK doesn't provide libwx_gtk (but only libwx_gtk-%ver) so we have to
# disable AutoReqProv or the package wouldn't install due to missing
# dependency! And so we have to list all requirments manually :-( (FIXME!)
AutoReqProv: no
Requires: ld-linux.so.2
Requires: libX11.so.6
Requires: libXext.so.6
Requires: libc.so.6
Requires: libcrypt.so.1
Requires: libdl.so.2
Requires: libgdk-1.2.so.0
Requires: libglib-1.2.so.0
Requires: libgmodule-1.2.so.0
Requires: libgthread-1.2.so.0
Requires: libgtk-1.2.so.0
Requires: libm.so.6
Requires: libpam.so.0
Requires: libpthread.so.0
Requires: libresolv.so.2
Requires: libstdc++-libc6.1-1.so.2
# Requires: libwx_gtk.so -- no, nobody provides it!
Requires: libc.so.6(GLIBC_2.0)
Requires: libc.so.6(GLIBC_2.1)
Requires: libdl.so.2(GLIBC_2.0)
Requires: libdl.so.2(GLIBC_2.1)
Requires: libm.so.6(GLIBC_2.0)
Requires: libm.so.6(GLIBC_2.1)
Requires: libpthread.so.0(GLIBC_2.0)
Requires: libpthread.so.0(GLIBC_2.1)

Prefix: %prefix
BuildRoot: /var/tmp/%{name}-root
Icon: mahogany.gif

%description
This package contains Mahogany, a powerful, scriptable GUI mail and news client
using GTK+ toolkit. Mahogany supports remote POP3, IMAP4, NNTP servers as well
as local MBOX and news spool folders and sending mail using SMTP or local MDA.

%prep
# the name is the same whether the package name is mahogany or mahogany-dynamic
%setup -n mahogany-%{VERSION}

%build
if [ ! -f configure ]; then
  autoconf
fi

CFLAGS="$RPM_OPT_FLAGS" ./configure --without-threads \
	--prefix=$RPM_BUILD_ROOT/%prefix

# if MAKE is not set, find the best value ourselves
if [ -z $MAKE ]; then
  if [ "$SMP" != "" ]; then
    export MAKE="make -j $SMP"
  else
    export MAKE=make
  fi
fi

# leaving old commands here for now but commenting out
#(cd include ; make ) || true
#make -i depend || true
#make -i clean
#
#(make -C include && make all && cd ../src && make %MAKETARGET)
#
#(make doc; exit 0)

make

%install
export PATH=/sbin:$PATH
make -k install
make install_rpm

%clean
rm -rf $RPM_BUILD_ROOT

%post

export MCONF_FILE=%prefix/share/Mahogany/M.conf
echo "# added by rpm installation" >> $MCONF_FILE
echo "GlobalDir=$MCONF_FILE" >> $MCONF_FILE

%postun

%files -f filelist
