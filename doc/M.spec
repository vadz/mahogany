Summary: Mahogany email and news client
Name: mahogany
Version: 0.22a
Release: 0
Copyright: Mahogany Artistic License
Group: Applications/Email
Source: ftp://ronnie.phy.hw.ac.uk/pub/Mahogany/mahogany-0.22a.tar.gz
URL: http://mahogany.home.dhs.org/

%description
This package contains Mahogany, a powerful, scriptable mail and news client for X11.

%prep
%setup

%build
autoconf
CFLAGS="$RPM_OPT_FLAGS" ./configure 

make dep
make clean
make

%install
export PATH=/sbin:$PATH
make -k install

%clean
rm -rf $RPM_BUILD_ROOT

%post

%postun

%files
/usr/local/bin/M
/usr/local/share/M

