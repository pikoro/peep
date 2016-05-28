%define version 0.4.1
%define soundversion 0.4.0
%define release 1mdk
%define name Peep

Summary: The sounds of cyberspace
Name:		%{name}
Version:	%{version}
Release:	%{release}
Copyright:	GPL
Group:		Toys
Packager:	Devin <XXXXXXXXXXXXXXXXXXX>
Vendor:		Custom
Distribution:	Custom
Source:		Peep-%{version}.src.tar.gz
Source1:		Peep-Sounds.%{soundversion}.tar.gz
Source2:		peepd.rc
BuildRoot:		/var/tmp/%{name}-buildroot
URL:			http://peep.sourceforge.net

%description
Monitor Everything without looking


%package sounds
Group:		Toys
Summary: The sounds of cyberspace
Requires: Peep

%description sounds
Peep Sounds

%prep

%setup -a 1 -n peep-%{version}

%build
[ -z $RPM_BUILD_ROOT ] ||  rm -rf $RPM_BUILD_ROOT

%configure \
	--with-libwrap=/usr \
	--with-device-driver 
%make

cd client/Net/Peep
perl Makefile.PL
%make
# %make test
cd ../../..


%install
%make install  prefix=$RPM_BUILD_ROOT/usr bindir=$RPM_BUILD_ROOT/usr/bin

cd client/Net/Peep
%make install PREFIX=$RPM_BUILD_ROOT/usr
cd ../../..

mkdir -p $RPM_BUILD_ROOT/etc $RPM_BUILD_ROOT/usr/share/peep/client $RPM_BUILD_ROOT/etc/rc.d/init.d $RPM_BUILD_ROOT/usr/share/doc/%{name}/
mkdir -p $RPM_BUILD_ROOT/usr/lib/perl5/site_perl/5.6.0
cp -ax %SOURCE2 $RPM_BUILD_ROOT/etc/rc.d/init.d/peepd
cp -ax docs/peep-doc.html $RPM_BUILD_ROOT/usr/share/doc/%{name}/
cp -ax docs/peep-internals.gif $RPM_BUILD_ROOT/usr/share/doc/%{name}/
cp -ax server/peep.conf $RPM_BUILD_ROOT/etc
#cp -ax ./client/bin/* $RPM_BUILD_ROOT/usr/bin
# mv -f $RPM_BUILD_ROOT/usr/share/peep/client/Peep $RPM_BUILD_ROOT/usr/lib/perl5/site_perl/5.6.0

%clean
[ -z $RPM_BUILD_ROOT ] ||  rm -rf $RPM_BUILD_ROOT


%post
/sbin/chkconfig --add peepd

%preun
/sbin/chkconfig --del peepd


%files
%defattr (-,root,root)
%doc /usr/share/doc/%{name}/peep-doc.html
%doc /usr/share/doc/%{name}/peep-internals.gif
%dir /usr/share/peep
#%dir /usr/share/peep/client
/usr/bin/*
#/usr/share/peep/client/*
/usr/lib/perl5/site_perl/*
%config(noreplace)/etc/peep.conf
%config(noreplace)/etc/rc.d/init.d/peepd

%files sounds
%defattr (-,root,root)
%dir /usr/share/peep/sounds
/usr/share/peep/sounds/*

%changelog
* Thu Jan 15 2001 Devin <XXXXXXXXXXXXXXXXXXX> 0.4.0-1mdk
- Updated to peep 0.4.0

* Thu Jan 15 2001 Devin <XXXXXXXXXXXXXXXXXXX> 0.3.8-1mdk
- Updated to peep 0.3.8

* Thu Dec 21 2000 Devin <XXXXXXXXXXXXXXXXXXX> 0.3.6-2mdk
- Fixed init script and moved sounds to seperate RPM

* Sun Dec 17 2000 Devin <XXXXXXXXXXXXXXXXXXX> 0.3.6-1mdk
- First draft
