Summary: M email and news client
Name: M
Version: 0.01
Release: 0
Copyright: M Artistic License
Group: Applications/Email
Source: ftp://sunsite.unc.edu/pub/Linux/apps/X11/M-0.01.tar.gz
URL: http://Ballueder.home.ml.org/M/

%description
This package contains M, a powerful, scriptable mail and news client for X11.

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

