dnl Macros that test for specific features.
dnl This file is part of the Autoconf packaging for Ethereal.
dnl Copyright (C) 1998-2000 by Gerald Combs.
dnl
dnl $Id: acinclude.m4,v 1.19 2000/01/15 21:01:04 guy Exp $
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
dnl 02111-1307, USA.
dnl
dnl As a special exception, the Free Software Foundation gives unlimited
dnl permission to copy, distribute and modify the configure scripts that
dnl are the output of Autoconf.  You need not follow the terms of the GNU
dnl General Public License when using or distributing such scripts, even
dnl though portions of the text of Autoconf appear in them.  The GNU
dnl General Public License (GPL) does govern all other use of the material
dnl that constitutes the Autoconf program.
dnl
dnl Certain portions of the Autoconf source text are designed to be copied
dnl (in certain cases, depending on the input) into the output of
dnl Autoconf.  We call these the "data" portions.  The rest of the Autoconf
dnl source text consists of comments plus executable code that decides which
dnl of the data portions to output in any given case.  We call these
dnl comments and executable code the "non-data" portions.  Autoconf never
dnl copies any of the non-data portions into its output.
dnl
dnl This special exception to the GPL applies to versions of Autoconf
dnl released by the Free Software Foundation.  When you make and
dnl distribute a modified version of Autoconf, you may extend this special
dnl exception to the GPL to apply to your modified version as well, *unless*
dnl your modified version has the potential to copy into its output some
dnl of the text that was the non-data portion of the version that you started
dnl with.  (In other words, unless your change moves or copies text from
dnl the non-data portions to the data portions.)  If your modification has
dnl such potential, you must delete any notice of this special exception
dnl to the GPL from your modified version.
dnl
dnl Written by David MacKenzie, with help from
dnl Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
dnl Roland McGrath, Noah Friedman, david d zuhn, and many others.

#
# AC_ETHEREAL_ADD_DASH_L
#
# Add to the variable specified as the first argument a "-L" flag for the
# directory specified as the second argument, and, on Solaris, add a
# "-R" flag for it as well.
#
# XXX - IRIX, and other OSes, may require some flag equivalent to
# "-R" here.
#
AC_DEFUN(AC_ETHEREAL_ADD_DASH_L,
[$1="$$1 -L$2"
case "$host_os" in
  solaris*)
    $1="$$1 -R$2"
  ;;
esac
])


#
# AC_ETHEREAL_STRUCT_SA_LEN
#
dnl AC_STRUCT_ST_BLKSIZE extracted from the file in question,
dnl "acspecific.m4" in GNU Autoconf 2.12, and turned into
dnl AC_ETHEREAL_STRUCT_SA_LEN, which checks if "struct sockaddr"
dnl has the 4.4BSD "sa_len" member, and defines HAVE_SA_LEN; that's
dnl what's in this file.
dnl Done by Guy Harris <guy@alum.mit.edu> on 1998-11-14. 

dnl ### Checks for structure members

AC_DEFUN(AC_ETHEREAL_STRUCT_SA_LEN,
[AC_CACHE_CHECK([for sa_len in struct sockaddr], ac_cv_ethereal_struct_sa_len,
[AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>], [struct sockaddr s; s.sa_len;],
ac_cv_ethereal_struct_sa_len=yes, ac_cv_ethereal_struct_sa_len=no)])
if test $ac_cv_ethereal_struct_sa_len = yes; then
  AC_DEFINE(HAVE_SA_LEN)
fi
])

#
# AC_ETHEREAL_IPV6_STACK
#
# By Jun-ichiro "itojun" Hagino, <itojun@iijlab.net>
#
AC_DEFUN(AC_ETHEREAL_IPV6_STACK,
[
	v6type=unknown
	v6lib=none

	AC_MSG_CHECKING([ipv6 stack type])
	for i in v6d toshiba kame inria zeta linux; do
		case $i in
		v6d)
			AC_EGREP_CPP(yes, [dnl
#include </usr/local/v6/include/sys/types.h>
#ifdef __V6D__
yes
#endif],
				[v6type=$i; v6lib=v6;
				v6libdir=/usr/local/v6/lib;
				CFLAGS="-I/usr/local/v6/include $CFLAGS"])
			;;
		toshiba)
			AC_EGREP_CPP(yes, [dnl
#include <sys/param.h>
#ifdef _TOSHIBA_INET6
yes
#endif],
				[v6type=$i; v6lib=inet6;
				v6libdir=/usr/local/v6/lib;
				CFLAGS="-DINET6 $CFLAGS"])
			;;
		kame)
			AC_EGREP_CPP(yes, [dnl
#include <netinet/in.h>
#ifdef __KAME__
yes
#endif],
				[v6type=$i; v6lib=inet6;
				v6libdir=/usr/local/v6/lib;
				CFLAGS="-DINET6 $CFLAGS"])
			;;
		inria)
			AC_EGREP_CPP(yes, [dnl
#include <netinet/in.h>
#ifdef IPV6_INRIA_VERSION
yes
#endif],
				[v6type=$i; CFLAGS="-DINET6 $CFLAGS"])
			;;
		zeta)
			AC_EGREP_CPP(yes, [dnl
#include <sys/param.h>
#ifdef _ZETA_MINAMI_INET6
yes
#endif],
				[v6type=$i; v6lib=inet6;
				v6libdir=/usr/local/v6/lib;
				CFLAGS="-DINET6 $CFLAGS"])
			;;
		linux)
			if test -d /usr/inet6; then
				v6type=$i
				v6lib=inet6
				v6libdir=/usr/inet6
				CFLAGS="-DINET6 $CFLAGS"
			fi
			;;
		esac
		if test "$v6type" != "unknown"; then
			break
		fi
	done

	if test "$v6lib" != "none"; then
		for dir in $v6libdir /usr/local/v6/lib /usr/local/lib; do
			if test -d $dir -a -f $dir/lib$v6lib.a; then
				LIBS="-L$dir $LIBS -l$v6lib"
				break
			fi
		done
		enable_ipv6="yes"
	else
		enable_ipv6="no"
	fi
	AC_MSG_RESULT(["$v6type, $v6lib"])
])

#
# AC_ETHEREAL_GETHOSTBY_LIB_CHECK
#
# Checks whether we need "-lnsl" to get "gethostby*()", which we use
# in "resolv.c".
#
# Adapted from stuff in the AC_PATH_XTRA macro in "acspecific.m4" in
# GNU Autoconf 2.13; the comment came from there.
# Done by Guy Harris <guy@alum.mit.edu> on 2000-01-14. 
#
AC_DEFUN(AC_ETHEREAL_GETHOSTBY_LIB_CHECK,
[
    # msh@cis.ufl.edu says -lnsl (and -lsocket) are needed for his 386/AT,
    # to get the SysV transport functions.
    # chad@anasazi.com says the Pyramid MIS-ES running DC/OSx (SVR4)
    # needs -lnsl.
    # The nsl library prevents programs from opening the X display
    # on Irix 5.2, according to dickey@clark.net.
    AC_CHECK_FUNC(gethostbyname)
    if test $ac_cv_func_gethostbyname = no; then
      AC_CHECK_LIB(nsl, gethostbyname, NSL_LIBS="-lnsl")
    fi
    AC_SUBST(NSL_LIBS)
])

#
# AC_ETHEREAL_SOCKET_LIB_CHECK
#
# Checks whether we need "-lsocket" to get "socket()", which is used
# by libpcap on some platforms - and, in effect, "gethostby*()" on
# most if not all platforms (so that it can use NIS or DNS or...
# to look up host names).
#
# Adapted from stuff in the AC_PATH_XTRA macro in "acspecific.m4" in
# GNU Autoconf 2.13; the comment came from there.
# Done by Guy Harris <guy@alum.mit.edu> on 2000-01-14. 
#
# We use "connect" because that's what AC_PATH_XTRA did.
#
AC_DEFUN(AC_ETHEREAL_SOCKET_LIB_CHECK,
[
    # lieder@skyler.mavd.honeywell.com says without -lsocket,
    # socket/setsockopt and other routines are undefined under SCO ODT
    # 2.0.  But -lsocket is broken on IRIX 5.2 (and is not necessary
    # on later versions), says simon@lia.di.epfl.ch: it contains
    # gethostby* variants that don't use the nameserver (or something).
    # -lsocket must be given before -lnsl if both are needed.
    # We assume that if connect needs -lnsl, so does gethostbyname.
    AC_CHECK_FUNC(connect)
    if test $ac_cv_func_connect = no; then
      AC_CHECK_LIB(socket, connect, SOCKET_LIBS="-lsocket",
		AC_MSG_ERROR(Function 'socket' not found.), $NSL_LIBS)
    fi
    AC_SUBST(SOCKET_LIBS)
])

#
# AC_ETHEREAL_PCAP_CHECK
#
AC_DEFUN(AC_ETHEREAL_PCAP_CHECK,
[
	# Evidently, some systems have pcap.h, etc. in */include/pcap
	AC_MSG_CHECKING(for extraneous pcap header directories)
	found_pcap_dir=""
	for pcap_dir in /usr/include/pcap /usr/local/include/pcap $prefix/include
	do
	  if test -d $pcap_dir ; then
	    CFLAGS="$CFLAGS -I$pcap_dir"
	    CPPFLAGS="$CPPFLAGS -I$pcap_dir"
	    found_pcap_dir=" $found_pcap_dir -I$pcap_dir"
	  fi
	done

	if test "$found_pcap_dir" != "" ; then
	  AC_MSG_RESULT(found --$found_pcap_dir added to CFLAGS)
	else
	  AC_MSG_RESULT(not found)
	fi

	# Pcap header checks
	AC_CHECK_HEADER(net/bpf.h,,
	    AC_MSG_ERROR([[Header file net/bpf.h not found; if you installed libpcap from source, did you also do \"make install-incl\"?]]))
	AC_CHECK_HEADER(pcap.h,, AC_MSG_ERROR(Header file pcap.h not found.))

	#
	# Try various directories to find libpcap
	#
	AC_CHECK_LIB(pcap, pcap_open_live,
	  [
	    PCAP_LIBS=-lpcap
	    AC_DEFINE(HAVE_LIBPCAP)
	  ],
	  [
	    #
	    # Throw away the cached "we didn't find it"
	    # answer, and see if it's in "/usr/local/lib".
	    #
	    unset ac_cv_lib_pcap_pcap_open_live
	    ethereal_save_LIBS="$LIBS"
	    AC_ETHEREAL_ADD_DASH_L(LIBS, /usr/local/lib)
	    AC_CHECK_LIB(pcap, pcap_open_live,
	      [
		#
		# Throw away the cached "we found it" answer, so that if
		# we rerun "configure", we don't just blow off the above
		# checks and blithely assume that we don't need to search
		# "/usr/local/lib".
		#
		# XXX - autoconf really needs a way to test for a given
		# routine in a given library *and* to test whether additional
		# "-L"/"-R"/whatever flags are needed *before* the "-l"
		# flag for the library and to test whether additional libraries
		# are needed after the library *and* to cache all that
		# information.
		#
		unset ac_cv_lib_pcap_pcap_open_live
		AC_ETHEREAL_ADD_DASH_L(PCAP_LIBS, /usr/local/lib)
		PCAP_LIBS="$PCAP_LIBS -lpcap"
		AC_DEFINE(HAVE_LIBPCAP)
		LIBS="$ethereal_save_LIBS"
	      ],
	      [
		#
		# Throw away the cached "we didn't find it"
		# answer, and see if it's in "$prefix/lib".
		#
		unset ac_cv_lib_pcap_pcap_open_live
		LIBS="$ethereal_save_LIBS -L$prefix/lib"
		AC_CHECK_LIB(pcap, pcap_open_live,
		  [
		    #
		    # Throw away the cached "we found it" answer, so that if
		    # we rerun "configure", we don't just blow off the above
		    # checks and blithely assume that we don't need to search
		    # "$prefix/lib".
		    #
		    unset ac_cv_lib_pcap_pcap_open_live
		    AC_ETHEREAL_ADD_DASH_L(PCAP_LIBS, $prefix/lib)
		    PCAP_LIBS="$PCAP_LIBS -lpcap"
		    AC_DEFINE(HAVE_LIBPCAP)
		    LIBS="$ethereal_save_LIBS"
		  ],
		  AC_MSG_ERROR(Library libpcap not found.),
		  $SOCKET_LIBS $NSL_LIBS)
	      ], $SOCKET_LIBS $NSL_LIBS)
	  ], $SOCKET_LIBS $NSL_LIBS)
	AC_SUBST(PCAP_LIBS)
])

#
# AC_ETHEREAL_ZLIB_CHECK
#
AC_DEFUN(AC_ETHEREAL_ZLIB_CHECK,
[
        AC_CHECK_HEADER(zlib.h,,enable_zlib=no)

        dnl
        dnl Check for "gzgets()" in zlib, because we need it, but
        dnl some older versions of zlib don't have it.  It appears
        dnl from the ChangeLog that any released version of zlib
        dnl with "gzgets()" should have the other routines we
        dnl depend on, such as "gzseek()", "gztell()", and "zError()".
        dnl
        AC_CHECK_LIB(z, gzgets,,enable_zlib=no)
])

#
# AC_ETHEREAL_UCDSNMP_CHECK
#
AC_DEFUN(AC_ETHEREAL_UCDSNMP_CHECK,
[
	want_ucdsnmp=yes

	AC_ARG_WITH(ucdsnmp,
	[  --with-ucdsnmp=DIR      use UCD SNMP client library, located in directory DIR.], [
	if test $withval = no
	then
		want_ucdsnmp=no
	else
		want_ucdsnmp=yes
		ucdsnmp_user_dir=$withval
	fi
	])

	if test $want_ucdsnmp = yes
	then
		ucdsnmpdir=""

		for d in $ucdsnmp_user_dir $prefix
		do
			if test x$d != xNONE 
			then
				AC_MSG_CHECKING($d for ucd-snmp)

				if test x$d != x/usr/local && test -f $d/include/ucd-snmp/snmp.h
				then
					AC_MSG_RESULT(found)
					ucdsnmpdir=$d
					break
				else
					AC_MSG_RESULT(not found)
				fi
			fi
		done

		if test x$ucdsnmpdir != x
		then
			AC_MSG_RESULT(added $d to paths)
			CFLAGS="$CFLAGS -I${ucdsnmpdir}/include"
			CPPFLAGS="$CPPFLAGS -I${ucdsnmpdir}/include"
			LIBS="$LIBS -L${ucdsnmpdir}/lib"
		fi
	fi
])
