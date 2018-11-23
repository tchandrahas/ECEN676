# includes some tricks from the RPM wizards at PLD:
# http://cvs.pld.org.pl/SPECS/spamassassin.spec
# namely, making the tools RPM for masses, sql, and tools, and
# the perl-Mail-SpamAssassin rpm for the modules only.

#%include        /usr/lib/rpm/macros.perl

%define perl_sitelib %(eval "`%{__perl} -V:installsitelib`"; echo "$installsitelib")

%define pdir    Mail
%define pnam    SpamAssassin
%define debug_package %{nil}

Summary:        a spam filter for email which can be invoked from mail delivery agents
Summary(pl):    Filtr antyspamowy, przeznaczony dla program�w dostarczaj�cych poczt� (MDA)

Group:          Applications/Mail
%define version 2.61
%define real_version 2.61

# Release number can be specified with rpmbuild --define 'release SOMETHING' ...
# If no such --define is used, the release number is 1.
#
# Source archive's extension can be specified with --define 'srcext .foo'
# where .foo is the source archive's actual extension.
# To compile an RPM from a .bz2 source archive, give the command
#   rpmbuild -tb --define 'srcext .bz2' @PACKAGE@-@VERSION@.tar.bz2
#
%if %{?release:0}%{!?release:1}
%define release 1
%endif
%if %{?srcext:0}%{!?srcext:1}
%define srcext .gz
%endif


%define name    spamassassin
%define initdir %{_initrddir}

Name: %{name}
Version: %{version}
Release: %{release}
License: Artistic
URL: http://spamassassin.org/
Source: http://spamassassin.org/released/Mail-SpamAssassin-%{real_version}.tar%{srcext}
Buildroot: %{_tmppath}/%{name}-root
Prefix: %{_prefix}
Prereq: /sbin/chkconfig
Requires: perl-Mail-SpamAssassin = %{version}-%{release}
Distribution: SpamAssassin

%define __find_provides /usr/lib/rpm/find-provides.perl
%define __find_requires /usr/lib/rpm/find-requires.perl

%description
SpamAssassin provides you with a way to reduce, if not completely eliminate,
Unsolicited Bulk Email (or "spam") from your incoming email.  It can be
invoked by a MDA such as sendmail or postfix, or can be called from a procmail
script, .forward file, etc.  It uses a genetic-algorithm-evolved scoring system
to identify messages which look spammy, then adds headers to the message so
they can be filtered by the user's mail reading software.  This distribution
includes the spamd/spamc components which considerably speeds processing of
mail.

%description -l pl
SpamAssassin udost�pnia Ci mo�liwo�� zredukowania, je�li nie
kompletnego wyeliminowania Niezamawianej Komercyjnej Poczty
(Unsolicited Bulk Email, spamu) z Twojej poczty. Mo�e by�
wywo�ywany z MDA, np. Sendmaila czy Postfixa, lub z pliku ~/.forward
itp. U�ywa og�lnego algorytmu oceniania w celu identyfikacji
wiadomo�ci, kt�re wygl�daj� na spam, po czym dodaje nag��wki do
wiadomo�ci, umo�liwiaj�c filtrowanie przez oprogramowanie u�ytkownika.
Ta dystrybucja zawiera programy spamd/spamc, umo�liwiaj�ce
uruchomienie serwera, co znacznie przyspieszy proces przetwarzania
poczty.

%package tools
Summary:        Miscellaneous tools and documentation for SpamAssassin
Summary(pl):    Przer�ne narz�dzia zwi�zane z SpamAssassin
Group:          Applications/Mail
Requires: perl-Mail-SpamAssassin = %{version}-%{release}

%description tools
Miscellaneous tools and documentation from various authors, distributed
with SpamAssassin.  See /usr/share/doc/SpamAssassin-tools-*/.

%description tools -l pl
Przer�ne narz�dzia, dystrybuowane razem z SpamAssassin. Zobacz
/usr/share/doc/SpamAssassin-tools-*/.

%package -n perl-Mail-SpamAssassin
Summary:        %{pdir}::%{pnam} -- SpamAssassin e-mail filter Perl modules
Summary(pl):    %{pdir}::%{pnam} -- modu�y Perla filtru poczty SpamAssassin
Requires: perl >= 5.004 perl(Pod::Usage) perl(HTML::Parser)
# PLD version:
#Group:          Development/Languages/Perl
# Red Hat version:
Group:          Development/Libraries

%description -n perl-Mail-SpamAssassin
Mail::SpamAssassin is a module to identify spam using text analysis and
several internet-based realtime blacklists. Using its rule base, it uses a
wide range of heuristic tests on mail headers and body text to identify
``spam'', also known as unsolicited commercial email. Once identified, the
mail can then be optionally tagged as spam for later filtering using the
user's own mail user-agent application.

%description -n perl-Mail-SpamAssassin -l pl
Mail::SpamAssassin jest pluginem dla Mail::Audit, s�u��cym do
identyfikacji spamu przy u�yciu analizy zawarto�ci i/lub internetowych
czarnych list. Do zidentyfikowania jako ,,spam'' stosuje szeroki
zakres test�w heurystycznych na nag��wkach i tre�ci, posi�kuj�c si�
stworzon� wcze�niej baz� regu�. Po zidentyfikowaniu, poczta mo�e by�
oznaczona jako spam w celu p�niejszego wyfiltrowania, np. przy u�yciu
aplikacji do czytania poczty.


%prep
%setup -q -n %{pdir}-%{pnam}-%{real_version}

%build
CFLAGS="$RPM_OPT_FLAGS"; export CFLAGS
%{__perl} Makefile.PL PREFIX=%{_prefix} SYSCONFDIR=%{_sysconfdir} DESTDIR=$RPM_BUILD_ROOT < /dev/null
%{__make}
%{__make} spamd/libspamc.so

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
# Specify the man dir locations since Perl sometimes gets it wrong... :(
%makeinstall \
	INSTALLMAN1DIR=%{_mandir}/man1 \
	INSTALLMAN3DIR=%{_mandir}/man3 \
	INSTALLSITEMAN1DIR=%{_mandir}/man1 \
	INSTALLSITEMAN3DIR=%{_mandir}/man3 \
	INSTALLVENDORMAN1DIR=%{_mandir}/man1 \
	INSTALLVENDORMAN3DIR=%{_mandir}/man3

install -d %buildroot/%{initdir}
install -d %buildroot/%{_includedir}
install -m 0755 spamd/redhat-rc-script.sh %buildroot/%{initdir}/spamassassin
install -m 0644 spamd/libspamc.so %buildroot/%{_libdir}
install -m 0644 spamd/libspamc.h %buildroot/%{_includedir}/libspamc.h

mkdir -p %{buildroot}/etc/mail/spamassassin

[ -x /usr/lib/rpm/brp-compress ] && /usr/lib/rpm/brp-compress

%files 
%defattr(-,root,root)
%doc README Changes sample-nonspam.txt sample-spam.txt spamd/README.spamd INSTALL BUGS COPYRIGHT License TRADEMARK USAGE Razor2.patch
%attr(755,root,root) %{_bindir}/*
%attr(644,root,root) %{_includedir}/*
%attr(644,root,root) %{_libdir}/*.so
%config(noreplace) %attr(755,root,root) %{initdir}/spamassassin
%{_mandir}/man1/*

%files tools
%defattr(644,root,root,755)
%doc sql tools masses contrib

%files -n perl-Mail-SpamAssassin
%defattr(644,root,root,755)
%{perl_sitelib}/*
%config(noreplace) %{_sysconfdir}/mail/spamassassin
%{_datadir}/spamassassin
%{_mandir}/man3/*

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
/sbin/chkconfig --add spamassassin

# older versions used /etc/sysconfig/spamd whereas it should have been
# spamassassin, so fix it here
if [ -f /etc/sysconfig/spamd ]; then
  %{__sed} -e 's/^OPTIONS=/SPAMDOPTIONS=/' /etc/sysconfig/spamd > /etc/sysconfig/spamassassin
  %{__mv} /etc/sysconfig/spamd /etc/sysconfig/spamassassin.rpmold
fi
# If spamd is running, let's be sure to change the lock file as well ...
if [ -f /var/lock/subsys/spamd ]; then
  %{__mv} /var/lock/subsys/spamd /var/lock/subsys/spamassassin
fi
/sbin/service spamassassin condrestart

%preun
if [ $1 = 0 ]; then
    /sbin/service spamassassin stop >/dev/null 2>&1
    /sbin/chkconfig --del spamassassin
fi

%postun
if [ "$1" -ge "1" ]; then
    /sbin/service spamassassin condrestart > /dev/null 2>&1
fi

%changelog
* Sun Sep 28 2003 Theo Van Dinter <felicity@kluge.net> 2.61-1
- updated to 2.61
- allow builds of tar.gz or tar.bz2 via the --define "srcext .bz2" option
- allow release to be overriden via the --define "release 1_rh8" option
- allow CFLAGS to be modified in the usual method (RPM_OPT_FLAGS)
- add more documentation files to be installed

* Thu Sep 09 2003 Malte S. Stretz <spamassassin-contrib@msquadrat.de>
- take advantage of the new simplified build system

* Wed May 28 2003 Theo Van Dinter <felicity@kluge.net> 2.60-1
- updated to 2.60

* Thu Apr 03 2003 Theo Van Dinter <felicity@kluge.net> 2.54-1
- updated to 2.54

* Thu Apr 03 2003 Theo Van Dinter <felicity@kluge.net> 2.53-1
- updated to 2.53

* Mon Mar 24 2003 Theo Van Dinter <felicity@kluge.net> 2.52-1
- updated to 2.52

* Thu Mar 13 2003 Theo Van Dinter <felicity@kluge.net> 2.51-1
- updated to 2.51

* Tue Feb 25 2003 Theo Van Dinter <felicity@kluge.net> -3
- changed "make install" call to properly set where the man pages go.
  Fixes oddities between MakeMaker and RPM.  <grumble>

* Tue Feb 25 2003 Theo Van Dinter <felicity@kluge.net> -2
- put in a patch to fix dependency problems with RPM 4.1

* Thu Feb 20 2003 Theo Van Dinter <felicity@kluge.net> 2.50-1
- upgraded to real 2.50 release

* Sun Feb 02 2003 Theo Van Dinter <felicity@kluge.net>
- instead of us trying to do a restart, call service condrestart to do
  it for us. :)

* Wed Dec 18 2002 Justin Mason <jm-spec@jmason.org>
- fixed specfile to work with Duncan's new Makefile.PL changes

* Tue Sep 18 2002 Justin Mason <jm-spec@jmason.org>
- merged 3-package system from b2_4_0 into 2.5x development

* Tue Sep 11 2002 Justin Mason <jm-spec@jmason.org>
- merged Michael Brown's libspamc support into 2.50 specfile
- made "perl Makefile.PL" read from /dev/null to avoid interactivity issues

* Mon Sep 10 2002 Michael Brown <michaelb@opentext.com>
- Added building, installation and packaging of libspamc.{h,so}

* Tue Sep 03 2002 Theo Van Dinter <felicity@kluge.net>
- added INSTALL to documentation files
- install man pages via _manpage macro to make things consistent
- added perl requires statement
- cleaned out some cruft
- fixed "file listed twice" bug

* Wed Aug 28 2002 Justin Mason <jm-spec@jmason.org>
- merged code from PLD rpm, split into spamassassin, perl-Mail-SpamAssassin,
  and spamassassin-tools rpms

* Mon Jul 29 2002 Justin Mason <jm-spec@jmason.org>
- removed migrate_cfs code, obsolete

* Thu Jul 25 2002 Justin Mason <jm-spec@jmason.org>
- removed findbin patch, obsolete

* Fri Apr 19 2002 Theo Van Dinter <felicity@kluge.net>
- Updated for 2.20 release
- made /etc/mail/spamassassin a config directory so local.cf doesn't get wiped out
- added a patch to remove findbin stuff

* Wed Feb 27 2002 Craig Hughes <craig@hughes-family.org>
- Updated for 2.1 release

* Sat Feb 02 2002 Theo Van Dinter <felicity@kluge.net>
- Updates for 2.01 release
- Fixed rc file
- RPM now buildable as non-root
- fixed post_service errors
- fixed provides to include perl modules
- use file find instead of manually specifying files

* Tue Jan 15 2002 Craig Hughes <craig@hughes-family.org>
- Updated for 2.0 release

* Wed Dec 05 2001 Craig Hughes <craig@hughes-family.org>
- Updated for final 1.5 distribution.

* Sun Nov 18 2001 Craig Hughes <craig@hughes-family.org>
- first version of rpm.

