dnl Macros that test for specific features.
dnl This file is part of the Autoconf packaging for Ethereal.
dnl Copyright (C) 1998-2000 by Gerald Combs.
dnl
dnl $Id: acinclude.m4,v 1.5 2003/12/17 02:36:56 guy Exp $
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
# AC_ETHEREAL_IPV6_STACK
#
# By Jun-ichiro "itojun" Hagino, <itojun@iijlab.net>
#
AC_DEFUN(AC_ETHEREAL_IPV6_STACK,
[
	v6type=unknown
	v6lib=none

	AC_MSG_CHECKING([ipv6 stack type])
	for i in v6d toshiba kame inria zeta linux linux-glibc; do
		case $i in
		v6d)
			AC_EGREP_CPP(yes, [
#include </usr/local/v6/include/sys/types.h>
#ifdef __V6D__
yes
#endif],
				[v6type=$i; v6lib=v6;
				v6libdir=/usr/local/v6/lib;
				CFLAGS="-I/usr/local/v6/include $CFLAGS"])
			;;
		toshiba)
			AC_EGREP_CPP(yes, [
#include <sys/param.h>
#ifdef _TOSHIBA_INET6
yes
#endif],
				[v6type=$i; v6lib=inet6;
				v6libdir=/usr/local/v6/lib;
				CFLAGS="-DINET6 $CFLAGS"])
			;;
		kame)
			AC_EGREP_CPP(yes, [
#include <netinet/in.h>
#ifdef __KAME__
yes
#endif],
				[v6type=$i; v6lib=inet6;
				v6libdir=/usr/local/v6/lib;
				CFLAGS="-DINET6 $CFLAGS"])
			;;
		inria)
			AC_EGREP_CPP(yes, [
#include <netinet/in.h>
#ifdef IPV6_INRIA_VERSION
yes
#endif],
				[v6type=$i; CFLAGS="-DINET6 $CFLAGS"])
			;;
		zeta)
			AC_EGREP_CPP(yes, [
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
		linux-glibc)
			AC_EGREP_CPP(yes, [
#include <features.h>
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
#if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || __GLIBC__ > 2
yes
#endif
#endif],
			[v6type=$i; v6lib=inet6; CFLAGS="-DINET6 $CFLAGS"])
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
# AC_ETHEREAL_ADNS_CHECK
#
AC_DEFUN(AC_ETHEREAL_ADNS_CHECK,
[
	want_adns=defaultyes

	AC_ARG_WITH(adns,
changequote(<<, >>)dnl
<<  --with-adns[=DIR]       use GNU ADNS (located in directory DIR, if supplied).   [default=yes, if present]>>,
changequote([, ])dnl
	[
	if   test "x$withval" = "xno";  then
		want_adns=no
	elif test "x$withval" = "xyes"; then
		want_adns=yes
	elif test -d "$withval"; then
		want_adns=yes
	fi
	])

	if test "x$want_adns" = "xdefaultyes"; then
		want_adns=yes
	fi

	if test "x$want_adns" = "xyes"; then
		AC_CHECK_LIB(adns, adns_init,
	    	AC_DEFINE(HAVE_GNU_ADNS, 1, [Define to use GNU ADNS library]),,
		)
	else
		AC_MSG_RESULT(not required)
	fi
])

#
# AC_ETHEREAL_LIBPCRE_CHECK
#
AC_DEFUN(AC_ETHEREAL_LIBPCRE_CHECK,
[
	if test "x$pcre_dir" != "x"
	then
	  #
	  # The user specified a directory in which libpcre resides,
	  # so add the "include" subdirectory of that directory to
	  # the include file search path and the "lib" subdirectory
	  # of that directory to the library search path.
	  #
	  # XXX - if there's also a libpcre in a directory that's
	  # already in CFLAGS, CPPFLAGS, or LDFLAGS, this won't
	  # make us find the version in the specified directory,
	  # as the compiler and/or linker will search that other
	  # directory before it searches the specified directory.
	  #
	  ethereal_save_CFLAGS="$CFLAGS"
	  CFLAGS="$CFLAGS -I$pcre_dir/include"
	  ethereal_save_CPPLAGS="$CPPLAGS"
	  CPPFLAGS="$CPPFLAGS -I$pcre_dir/include"
	  ethereal_save_LIBS="$LIBS"
	  LIBS="$LIBS -lpcre"
	  ethereal_save_LDFLAGS="$LDFLAGS"
	  LDFLAGS="$LDFLAGS -L$pcre_dir/lib"
	fi

	#
	# Make sure we have "pcre.h".  If we don't, it means we probably
	# don't have libpcre, so don't use it.
	#
	AC_CHECK_HEADER(pcre.h,,
	  [
	    if test "x$pcre_dir" != "x"
	    then
	      #
	      # The user used "--with-pcre=" to specify a directory
	      # containing libpcre, but we didn't find the header file
	      # there; that either means they didn't specify the
	      # right directory or are confused about whether libpcre
	      # is, in fact, installed.  Report the error and give up.
	      #
	      AC_MSG_ERROR([libpcre header not found in directory specified in --with-pcre])
	    else
	      if test "x$want_pcre" = "xyes"
	      then
		#
		# The user tried to force us to use the library, but we
		# couldn't find the header file; report an error.
		#
		AC_MSG_ERROR(Header file pcre.h not found.)
	      else
		#
		# We couldn't find the header file; don't use the
		# library, as it's probably not present.
		#
		want_pcre=no
	      fi
	    fi
	  ])

	if test "x$want_pcre" != "xno"
	then
		#
		# Well, we at least have the pcre header file.
		#
		# We're only using standard functions from libpcre,
		# so we don't need to perform extra checks.
		#
		AC_CHECK_LIB(pcre, pcre_compile,
		[
			if test "x$pcre_dir" != "x"
			then
				#
				# Put the "-I" and "-L" flags for pcre at
				# the beginning of CFLAGS, CPPFLAGS,
				# LDFLAGS, and LIBS.
				#
				PCRE_LIBS="-L$pcre_dir/lib -lpcre $ethereal_save_LIBS"
			else
				PCRE_LIBS="-lpcre"
			fi
			AC_DEFINE(HAVE_LIBPCRE, 1, [Define to use libpcre library])
		],[
			if test "x$pcre_dir" != "x"
			then
				#
				# Restore the versions of CFLAGS, CPPFLAGS,
				# LDFLAGS, and LIBS before we added the
				# "--with-pcre=" directory, as we didn't
				# actually find pcre there.
				#
				CFLAGS="$ethereal_save_CFLAGS"
				CPPFLAGS="$ethereal_save_CPPLAGS"
				LDFLAGS="$ethereal_save_LDFLAGS"
				LIBS="$ethereal_save_LIBS"
				PCRE_LIBS=""
			fi
			want_pcre=no
		])
		AC_SUBST(PCRE_LIBS)
	fi
])
