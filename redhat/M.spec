# Purpose:  The .spec file for building Mahogany RPM
# Version:  $Id$

# version and release
%define VERSION 0.23a
%define RELEASE 1

# so far I didn't find how to make a relocatable package...
%define prefix /usr/local

# we can build 2 different packages from this spec file: a semistatically
# linked version (only common libs linked dynamically) or a quartstatically
# one (only wxGTK linked statically, GTK libs dynamically)
#%define MAKETARGET semistatic
%define MAKETARGET quartstatic

Summary: Mahogany email and news client
Name: mahogany-dynamic
Version: %VERSION
Release: %RELEASE
Copyright: Mahogany Artistic License
Group: X11/Applications/Networking
Source: ftp://ronnie.phy.hw.ac.uk/pub/Mahogany/mahogany-%{VERSION}.tar.gz
URL: http://mahogany.home.dhs.org/
Packager: Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
Provides: mua
Prefix: %prefix
BuildRoot: /home/zeitlin/build/rpm
Icon: mahogany.gif

%description
This package contains Mahogany, a powerful, scriptable GUI mail and news client
using GTK+ toolkit. Mahogany supports remote POP3, IMAP4, NNTP servers as well
as local MBOX and news spool folders and sending mail using SMTP.

%prep
# the name is the same whether the package name is mahogany or mahogany-dynamic
%setup -n mahogany-%{VERSION}

%build
if [ ! -f configure ]; then
  autoconf
fi

CFLAGS="$RPM_OPT_FLAGS" ./configure --without-threads --without-python \
	--prefix=$RPM_BUILD_ROOT/%prefix

make dep
make clean

if [ "$SMP" != "" ]; then
  export MAKE="make -j $SMP"
fi

(cd extra && make all && cd ../src && make %MAKETARGET)

(make doc; exit 0)

%install
export PATH=/sbin:$PATH
make -k install_all
make install_rpm

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files -f filelist
