dnl Process this file with autoconf to produce a configure script.

AC_INIT(Mahogany, 0.67)

rm -f conftestdata
if ln -s X conftestdata 2>/dev/null
then	rm -f conftestdata
else	AC_MSG_ERROR([Sorry, this build system will not work on a
	system without symbolic links.  It depends on the ability
	to create links to directories, so this is not something
	that can be worked around.])
fi

dnl OSTYPE may be set by the user to override the default which is just uname
if test "x$OSTYPE" = "x"; then
        OSTYPE=`uname`
fi

case "$OSTYPE" in
    darwin* | Darwin*)
	USE_MAC=1
	MAKE_USE_MAC="USE_MAC=yes"
	USE_MINGW=0
	MAKE_USE_MINGW=""
	USE_RESOURCES=0
	MAKE_USE_RESOURCES=""
	;;
    cygwin* | CYGWIN*)
	USE_MAC=0
	MAKE_USE_MAC=""
	USE_MINGW=0
	MAKE_USE_MINGW=""
	USE_RESOURCES=1
	MAKE_USE_RESOURCES="USE_RESOURCES=yes"
	;;
    mingw* | MINGW*)
	USE_MAC=0
	MAKE_USE_MAC=""
	USE_MINGW=1
	MAKE_USE_MINGW="USE_MINGW=yes"
	USE_RESOURCES=1
	MAKE_USE_RESOURCES="USE_RESOURCES=yes"
	;;
    *)
	USE_MAC=0
	MAKE_USE_MAC=""
	USE_MINGW=0
	MAKE_USE_MINGW=""
	USE_RESOURCES=0
	MAKE_USE_RESOURCES=""
	;;
esac
AC_SUBST(MAKE_USE_MAC)
AC_SUBST(MAKE_USE_MINGW)
AC_SUBST(MAKE_USE_RESOURCES)

echo ""
echo "*** Configuring Mahogany for $OSTYPE ***"
echo ""

AC_CONFIG_HEADER(include/config.h)
AC_CONFIG_AUX_DIR(extra/scripts)
AC_CONFIG_MACRO_DIR([extra/m4])
### HACK: unfortunately, the Makefiles in subdirectories can use this
### install program without qualification.  We really shouldn't do it
### this way, but we force the install-sh found here to be a full path.
case "$ac_install_sh" in
/*)	;;
../*)	ac_install_sh=`cd .; exec pwd``expr "$ac_install_sh" : '\.\.\(.*\)'` ;;
*)	ac_install_sh=`cd $srcdir; exec pwd`/$ac_install_sh ;;
esac

dnl --------------------------------------------------------------------------
dnl Mahogany parameter evaluation
dnl Usage: M_OVERRIDES(package,message,variable,default,help)
dnl --------------------------------------------------------------------------

M_OVERRIDES(debug,debugging,USE_DEBUG,0,
--enable-debug          generate code with debugging information)

case "$USE_DEBUG" in
    dnl options which are incompatible with debugging
    0|'')
	M_OVERRIDES(optimize,optimization,USE_OPT,1,
	--disable-optimize      disable optimization (automatic if debugging))
	debug_option="no"
	;;

    dnl options which only make sense if debug was given
    *)
	M_OVERRIDES(wxdebug,wxWin debug libs,USE_WXDEBUG,0,
	--with-wxdebug          use debug wxWidgets libraries)

	M_OVERRIDES(dmalloc,dmalloc library,USE_DMALLOC,0,
	--with-dmalloc          use dmalloc (http://dmalloc.com/) for debugging)

	M_OVERRIDES(leaktracer,LeakTracer,USE_LEAKTRACER,0,
	--with-leaktracer       use LeakTracer for debugging)

	M_OVERRIDES(efence,ElectricFence,USE_EFENCE,0,
	--with-efence           use ElectricFence for debugging)

	debug_option="yes"
esac

M_OVERRIDES(profile,profiling,USE_PROFILER,0,
--enable-profile        generate code for profiling)

M_OVERRIDES(strict,strict compiler options,USE_STRICT,0,
--enable-strict         strict compiler flags - may not work on all systems)

M_OVERRIDES(nls,i18n support,USE_I18N,1,
--disable-nls           no internationalization support)

M_OVERRIDES(python,Python scripting,USE_PYTHON,1,
--with-python=xxx       one of none/static/dynamic)

M_OVERRIDES(dspam,DSPAM spam filter,USE_DSPAM,1,
--with-dspam            use DSPAM for spam filtering)

dnl choose how to use Python: normally, dynamic loading is preferred, but it
dnl doesn't work under Mac (we need Python.bundle, but all we have is Python
dnl framework which is a dylib and not a bundle)
if test "$USE_PYTHON" = 1; then
    if test "$USE_MAC" = 1; then
	USE_PYTHON="static"
    else
	USE_PYTHON="dynamic"
    fi
fi

case "$USE_PYTHON" in
    static|Static|STATIC|dynamic|Dynamic|DYNAMIC|1)
	M_OVERRIDES(swig,whether we should use SWIG,USE_SWIG,1,
	--without-swig          don't use swig even if it is available)
	;;
esac

dnl dialup is not currently supported under Mac OS X
if test "$USE_MAC" = 0; then
    M_OVERRIDES(dialup,special dial-up support,USE_DIALUP,1,
    --disable-dialup        disable dial-up support for modem users)
fi

M_OVERRIDES(experimental,experimental code,USE_EXPERIMENTAL,0,
--with-experimental     compile experimental code)

M_OVERRIDES(threads,multi-threading,USE_THREADS,0,
--with-threads          multi-threaded version (DOES NOT WORK))

M_OVERRIDES(palm,Palm Pilot support,USE_PISOCK,0,
--with-palm             include support for Palm Pilot)

M_OVERRIDES(ccostype,c-client OS type,USE_CCOSTYPE,,
--with-ccostype=xxx     use OS type xxx for c-client compilation)

M_OVERRIDES(modules,modules,USE_MODULES,auto,
--with-modules=xxx      one of none/static/dynamic)

M_OVERRIDES(ssl,SSL support,USE_SSL,1,
--with-ssl=ssldir       specify path where OpenSSL is installed)

AC_MSG_CHECKING(for install location)
case "$prefix" in
NONE)	AC_CACHE_VAL(m_cv_prefix,m_cv_prefix=$ac_default_prefix);;
*)	m_cv_prefix=$prefix ;;
esac
AC_MSG_RESULT($m_cv_prefix)
case "$m_cv_prefix" in /*) ;; *)
	AC_MSG_WARN([--prefix=$prefix must be an absolute path name, using $ac_default_prefix])
	m_cv_prefix=$ac_default_prefix
esac
prefix=$m_cv_prefix

WX_CONFIG_OPTIONS

dnl save parameter settings in case stuff below aborts
AC_CACHE_SAVE

dnl --------------------------------------------------------------------------
dnl testing operating system and compilers
dnl --------------------------------------------------------------------------

AC_CANONICAL_HOST
dnl setting the OS type for information
AC_DEFINE_UNQUOTED(M_CANONICAL_HOST,"$host")
AC_DEFINE_UNQUOTED(M_OSINFO,_T("`uname -s -r -m`"))
AC_DEFINE_UNQUOTED(M_PREFIX,"$prefix")

dnl set the top level directories
dnl use "exec pwd" to avoid shell built-in
dnl use "cd ." so FreeBSD shell will fork subshell
SOURCEDIR=`cd $srcdir; exec pwd`
AC_SUBST(SOURCEDIR)
AC_DEFINE_UNQUOTED(M_TOP_SOURCEDIR, _T("$SOURCEDIR"))
BUILDDIR=`cd .; exec pwd`
AC_SUBST(BUILDDIR)
AC_DEFINE_UNQUOTED(M_TOP_BUILDDIR, _T("$BUILDDIR"))

dnl C-compiler checks
dnl =================

dnl use what compiler?
CFLAGS=${CFLAGS=}	# magic incantation so CFLAGS not set by AC_PROG_CC
AC_PROG_CC
dnl   defines CC with the compiler to use
dnl   defines GCC with yes if using gcc, else empty
dnl   defines CFLAGS

dnl fixups for UNIX variants
AC_ISC_POSIX

dnl does compiler support -c and -o simultaniously?
dnl   defines NO_MINUS_C_MINUS_O if compiler does not accept 
dnl                              both switches simultaniously
AC_PROG_CC_C_O
# I dislike delving so deeply into autoconf's internal values, but this
# seems to be the only accurate way of finding out what it determined.
case `eval echo '$'{ac_cv_prog_cc_${ac_cc}_c_o}` in yes) ;; *)
	AC_MSG_ERROR([Sorry, this capability is used extensively by the build
	system, including third-party code that we do not control.  You'll
	have to use a compiler that supports this.]) ;;
esac

dnl what is the c-preprocessor?
AC_PROG_CPP
dnl   defines CPP with the c-preprocessor

dnl is -traditional needed for correct compilations?
AC_PROG_GCC_TRADITIONAL
dnl   adds -traditional for gcc if needed

dnl Checks for compiler characteristics.
AC_C_CONST
AC_C_INLINE

dnl C++-compiler checks
dnl ===================

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

dnl use what compiler?
CXXFLAGS=${CXXFLAGS=}	# magic incantation so CXXFLAGS not set by AC_PROG_CXX

dnl AC_PROG_CXX macro
dnl   defines CXX with the compiler to use
dnl   defines GXX with yes if using gxx, else empty
dnl   defines CXXFLAGS
AC_PROG_CXX

dnl We need C++11 support, so check for it.
AX_CXX_CHECK_FLAG([-std=c++11],,,[CXXFLAGS="$CXXFLAGS -std=c++11"], [
	AC_MSG_ERROR([
Sorry, $CXX doesn't support -std=c++11 option and can't be used to compile this program.
	])
])

dnl disable support for exceptions if the compiler is g++ and
dnl new enough to support it.
M_ADD_CXXFLAGS_IF_SUPPORTED([-fno-exceptions])

dnl check whether the C++ supports the new logical operator names
dnl c-client library uses some of them as field names
AC_CACHE_CHECK(if ${CXX} supports logical operator names,
	m_cv_cxx_opernames,
	[echo 'int f(int a, int b){return not a or b;}' >conftest.cc
	if ${CXX} -c conftest.cc >/dev/null 2>&1
	then	m_cv_cxx_opernames=yes
	else	m_cv_cxx_opernames=no
	fi
	rm -f conftest*])

dnl really a subroutine to avoid nesting source lines too deeply
AC_DEFUN([M_OPER_NAMES],
[AC_CACHE_CHECK(if logical operator names can be disabled,
	m_cv_cxx_disableopernames,
	[case "$GXX" in
	yes)	echo 'void f(){}' >conftest.cc
		case "`${CXX-g++} -c -fno-operator-names conftest.cc 2>&1`" in
		'')	m_cv_cxx_disableopernames="-fno-operator-names"
		esac
		rm -f conftest* ;;
	esac
	### other tests here???
	case "$m_cv_cxx_disableopernames" in '')
		m_cv_cxx_disableopernames="none supported"
	esac])])

dnl really a subroutine to avoid nesting source lines too deeply
AC_DEFUN([M_GCC_BUGGY_NO_OPERATOR_NAMES],
[AC_CACHE_CHECK(if disabling the logical operator names is buggy,
	m_cv_gcc_buggy_fno_operator_names,
	[case "$GCC" in
	yes)	echo '#define TEST 1
#if defined(TEST)
#undef TEST
#endif' >conftest.cc
		case "`${CXX-g++} -c -fno-operator-names conftest.cc 2>&1`" in
		'')	m_cv_gcc_buggy_fno_operator_names="no" ;;
		*)  m_cv_gcc_buggy_fno_operator_names="yes" ;;
		esac
		rm -f conftest* ;;
	esac])])

dnl if the compiler treats "or" and "not" as keywords, we must to get a
dnl workaround for compiling the c-client header files
case "$m_cv_cxx_opernames" in yes)
	dnl check whether is possible to redefine the logical operator
	dnl names.  cclient headers will not compile if the compiler
	dnl understands them as keywords.
	AC_CACHE_CHECK(if ${CXX} can redefine logical operator names,
		m_cv_cxx_redefineopernames,
		[echo '#define or m_or' >conftest.cc
		if ${CXX} -c conftest.cc >/dev/null 2>&1
		then	m_cv_cxx_redefineopernames=yes
		else	m_cv_cxx_redefineopernames=no
		fi
		rm -f conftest*])
	dnl if the operator names can't be redefined try to check
	dnl whether the compiler accepts some flag to disable them
	case "$m_cv_cxx_redefineopernames" in
	yes)	AC_DEFINE(M_LOGICAL_OP_NAMES) ;;
	no)	M_OPER_NAMES
		case "$m_cv_cxx_disableopernames" in
		"none supported") dnl Houston, we have a problem
	AC_MSG_ERROR([Sorry, the c-client library used by Mahogany uses some
	names which your compiler treats as keywords (the new logical
	operator names: and, or, not, etc.).  We currently don't know a
	workaround for your compiler.  If it does accept some flag for
	disabling them, please go to http://mahogany.sourceforge.net/bugz
	and submit a bug against the Build component telling us which
	flag will work.]) ;;
		*) M_GCC_BUGGY_NO_OPERATOR_NAMES
			case "$m_cv_gcc_buggy_fno_operator_names" in
			yes)
			if test -f /etc/redhat-release -a ! grep "7\.0" /etc/redhat-release
			then workarroundmsg="
	It seems you are using Red Hat Linux 7.0. A workaround for your
	problem is to use egcs to compile wxWidgets and Mahogany:
		- at your bash prompt type:
			export CC=egcs
			export CXX=egcs++
		- reconfigure and rebuild wxWidgets and Mahogany
		(don't forget to remove the config.cache files
		located in the source code directories of
		wxWidgets and Mahogany before reconfiguring them."
			else workarroundmsg=""
			fi
			AC_MSG_ERROR([Sorry, it seems that you have a buggy gcc version which is
	not able to compile Mahogany (gcc-2.96-69 shipped by Red Hat with Red
	Hat Linux 7.0 is one of them). Please upgrade it.$workaroundmsg]) ;;
			no)
            CXXFLAGS="$CXXFLAGS $m_cv_cxx_disableopernames"
            esac ;;
		esac ;;
	esac
esac

dnl PCH support is currently for gcc 3.4+ only
MAKE_USE_PCH=
if test "$GXX" = "yes"; then
    AC_CACHE_CHECK(if ${CXX} supports precompiled headers,
	m_cv_cxx_pch,
	[
	    AC_TRY_COMPILE([],
		[
		    #if !defined(__GNUC__) || !defined(__GNUC_MINOR__)
			There is no PCH support
		    #endif
		    #if (__GNUC__ < 3)
			There is no PCH support
		    #endif
		    #if (__GNUC__ == 3) && (__GNUC_MINOR__ < 4)
			There is no PCH support
		    #endif
		],
		m_cv_cxx_pch=yes,
		m_cv_cxx_pch=no
	    )
	]
    )

    if test "$m_cv_cxx_pch" = "yes"; then
	AC_DEFINE(USE_PCH)
	MAKE_USE_PCH="USE_PCH=1"
    fi
fi

AC_SUBST(MAKE_USE_PCH)

AC_LANG_RESTORE

dnl Various other commands needed
dnl =============================

dnl set RANLIB to ranlib location or nothing
AC_PROG_RANLIB

dnl check for M4
AC_PATH_PROG(M4, m4)

dnl set YACC to yacc location: don't use AC_PROG_YACC as it does *not* fail if
dnl yacc is not available (!)
AC_CHECK_PROGS(YACC, byacc yacc 'bison -y', none)
if test "$YACC" = "none"; then
    AC_MSG_WARN([yacc not found, you will not be able to modify *.y files.])
    YACC=
fi

AC_PROG_INSTALL

AC_PROG_MAKE_SET

dnl --------------------------------------------------------------------------
dnl check for features and functions
dnl --------------------------------------------------------------------------

dnl we use C++ so we always have prototypes - Python.h needs this
AC_DEFINE(HAVE_PROTOTYPES)

dnl used by python
AC_CACHE_CHECK(for variable length prototypes and stdarg.h,
	m_cv_stdarg_prototypes,
	[AC_TRY_COMPILE(
	[#include <stdarg.h>
	int foo(int x, ...) {
		va_list va;
		va_start(va, x);
		va_arg(va, int);
		va_arg(va, char *);
		va_arg(va, double);
		return 0;
	}], [return foo(10, "", 3.14);],
		m_cv_stdarg_prototypes=yes, m_cv_stdarg_prototypes=no)])
case "$m_cv_stdarg_prototypes" in yes)
	AC_DEFINE(HAVE_STDARG_PROTOTYPES)dnl
esac

AC_CACHE_CHECK(for bad exec* prototypes, m_cv_bad_exec,
	[AC_TRY_COMPILE([#include <unistd.h>], [char **t;execve("@",t,t);],
		m_cv_bad_exec=yes, m_cv_bad_exec=no)])
case "$m_cv_bad_exec" in yes)
	AC_DEFINE(BAD_EXEC_PROTOTYPES)dnl
esac

dnl Checks for typedefs and structures.
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_CHECK_FUNCS(strsep)

dnl --------------------------------------------------------------------------
dnl check for the debugging options
dnl --------------------------------------------------------------------------

dnl memory debug libraries get tested last to make sure they are linked last

dnl electric fence
case "$USE_EFENCE" in 1)
	AC_CHECK_LIB(efence,main,
		LIB_EFENCE="-lefence",
		AC_MSG_WARN(efence library configured but not available.))
esac

dnl dmalloc debugging lib
case "$USE_DMALLOC" in
1) M_CHECK_MYLIB(dmalloc, main,
    [ /usr/lib /usr/local/lib $prefix/lib ],
    [
	LIB_DMALLOC="-ldmalloc"
	AC_DEFINE(USE_DMALLOC)
    ],
    AC_MSG_WARN(dmalloc library not available, get it from http://dmalloc.com/.)
   )
esac

dnl LeakTracer debugging lib
case "$USE_LEAKTRACER" in
1)	MAKE_USE_LEAKTRACER="USE_LEAKTRACER=yes" ;;
*)	MAKE_USE_LEAKTRACER="" ;;
esac
AC_SUBST(MAKE_USE_LEAKTRACER)

dnl set debug/optimisation flags

case "$USE_DEBUG" in
0|'')	DEBUG_OPTS="" ;;
1)	DEBUG_OPTS="${USER-$LOGNAME}" ;;
*)	DEBUG_OPTS="`echo $USE_DEBUG | tr -s -- '-,/:+ ' '_ '`" ;;
esac

MAKE_DEBUG='DEBUG :='
case "$DEBUG_OPTS" in '') ;; *)
	AC_DEFINE(DEBUG)
	CPPFLAGS="$CPPFLAGS -DDEBUG"
	for i in $DEBUG_OPTS
	do	CPPFLAGS="$CPPFLAGS -DDEBUG_$i"
		MAKE_DEBUG="$MAKE_DEBUG $i"
	done  
	USE_DEBUG_INFO=1
esac
AC_SUBST(MAKE_DEBUG)

if test "$USE_WXDEBUG" = 1; then
   USE_OPT=0
   dnl FIXME: shouldn't hard code wxWin version here!
fi

case "$USE_DEBUG_INFO.$GXX.$ac_cv_prog_cc_g" in
1.yes.*)	CODE_GEN_FLAGS="$CODE_GEN_FLAGS -g3" 
		USE_OPT=0 ;;
1.no.yes)	CODE_GEN_FLAGS="$CODE_GEN_FLAGS -g" ;;
esac

dnl i18n support
if test "$USE_I18N" = 1; then
   AC_DEFINE(USE_I18N)
fi

dnl --------------------------------------------------------------------------
dnl check for misc other compilation options
dnl --------------------------------------------------------------------------

case "$USE_OPT" in
1)	CODE_GEN_FLAGS="$CODE_GEN_FLAGS -O2" ;;
*)	CODE_GEN_FLAGS="$CODE_GEN_FLAGS -O0" ;;
esac


case "$USE_PROFILER" in 1)
	CODE_GEN_FLAGS="$CODE_GEN_FLAGS -pg"
	LDFLAGS="$LDFLAGS -pg"
esac

case "$USE_EXPERIMENTAL" in
0|'')	EXPERIMENTAL="" ;;
1)	EXPERIMENTAL="${USER-$LOGNAME}" ;;
*)	EXPERIMENTAL="`echo $USE_EXPERIMENTAL | tr -s -- '-,/:+ ' '_ '`" ;;
esac

MAKE_EXPERIMENTAL='EXPERIMENTAL :='
case "$EXPERIMENTAL" in '') ;; *)
	# including this on the command line interferes
	# with its usage in wxMDialogs.cpp
	#CPPFLAGS="$CPPFLAGS -DEXPERIMENTAL"
	for i in $EXPERIMENTAL
	do	CPPFLAGS="$CPPFLAGS -DEXPERIMENTAL_$i"
		MAKE_EXPERIMENTAL="$MAKE_EXPERIMENTAL $i"
	done  
esac
AC_SUBST(MAKE_EXPERIMENTAL)

if test "x$USE_DIALUP" = "x1"; then
    AC_DEFINE(USE_DIALUP)
fi

dnl --------------------------------------------------------------------------
dnl check for SSL
dnl --------------------------------------------------------------------------

case "$USE_SSL" in
0)	ssl_option=no
	;;

1|/*)	dnl be smart: if --with-ssl value already ends in include, don't do
	dnl anything, but if it doesn't then add it
	case "$USE_SSL" in
	/*/include) ssl_inc="$USE_SSL" ;;
	/*)	ssl_inc="$USE_SSL/include" ;;
	*)	# explicitly using -/usr/local/include is a bad idea with gcc,
		# it complains about it ("changing the order of system
		# directories search")
		if test "$prefix" != "/usr/local"; then
		    ssl_inc="$prefix/include"
		fi
		;;
	esac

	dnl there is a subtlety here: we have to look for ssl.h and not for
	dnl openssl/ssl.h directly because if the latter is in a standard
	dnl location (e.g. /usr/include) we have no way of knowing which -I
	dnl option we should use for c-client which wants to directly include
	dnl <ssl.h> and not <openssl/ssl.h> -- using /usr/include/openssl
	dnl as we used to do doesn't work in case when the SSL headers live
	dnl under /usr/local
	dnl
	dnl OTOH, in the newer versions of OpenSSL ssl.h itself includes the
	dnl other files as <openssl/foo.h> so when testing the headers
	dnl existence with -Iprefix/openssl we must *also* have -Iprefix or it
	dnl doesn't risk to work.
	dnl
	dnl also, RH9 install of OpenSSL is broken and krb5.h file included by
	dnl ssl.h is in a non standard directory and this breaks compilation.
	dnl Luckily we don't need it anyhow so we can just disable using
	dnl Kerberos completely.
	dnl
	dnl the others -DNO_XXX don't seem to exist any more in the new
	dnl OpenSSL versions anyhow but we keep them here as apparently they
	dnl were needed for the older versions, for whatever reasons...
	SSL_CPPFLAGS="-DNO_IDEA -DNO_DSA -DOPENSSL_NO_KRB5"

	CPPFLAGS_OLD=$CPPFLAGS
	CPPFLAGS="${CPPFLAGS} ${SSL_CPPFLAGS} -I${ssl_inc} -I/usr/ssl/include -I/usr/local/ssl/include"
	M_CHECK_MYHEADER(ssl.h,
	[ \
		$ssl_inc/openssl \
		/usr/include/openssl \
		/usr/ssl/include/openssl \
		/usr/local/include/openssl \
		/usr/local/ssl/include/openssl \
	],
	[
	   ssl_option=yes
	   ssl_inc=$m_cv_header_ssl_h
	],
	   ssl_option=no
	)

	CPPFLAGS=$CPPFLAGS_OLD

	if test "$ssl_option" != "no"; then
	    dnl remove the openssl suffix now
	    ssl_inc=`echo $ssl_inc | sed 's@/openssl$@@'`
	    ssl_lib=`echo $ssl_inc | sed 's@include$@lib@'`

	    LIBS_SSL=""

	    dnl check for -lcrypto first as libssl needs it under some
	    dnl platforms (notably FreeBSD)
	    M_CHECK_MYLIB(crypto, main,
		[ \
		    $ssl_lib \
		    /usr/lib \
		    /usr/local/lib \
		    /home/local/lib \
		],
		LIBS_SSL="$libpath_crypto_main -lcrypto"
	    )

	    dnl now we can check for -lssl itself
	    M_CHECK_MYLIB(ssl, main,
		[ \
		    $ssl_lib \
		    /usr/lib \
		    /usr/local/lib \
		    /home/local/lib \
		],
		LIBS_SSL="$LIBS_SSL $libpath_ssl_main -lssl",
		ssl_option=no,
		$LIBS_SSL
	    )
	fi

	dnl only test for other functions if ssl.h was found
	if test "$ssl_option" != "no"; then
	    AC_CHECK_FUNC(socket,,
	            AC_CHECK_LIB(socket,socket,, ssl_option=no))
	    AC_CHECK_FUNC(gethostname,,
	            AC_CHECK_LIB(resolv,gethostname,, ssl_option=no))
	    AC_CHECK_FUNC(gethostbyname,,
	            AC_CHECK_LIB(nsl,gethostbyname,, ssl_option=no))
	fi

	case "$ssl_option" in
	no*) 	AC_MSG_WARN([Cannot find libraries for SSL - SSL support disabled])
		;;
	esac
	;;
*)	AC_MSG_WARN([--with-ssl=$USE_SSL ignored, you must specify absolute path])
	ssl_option=no ;;
esac

if test "$ssl_option" = "yes"; then
    AC_DEFINE(USE_SSL)

    dnl have to give the location of SSL headers to cclient if it is in a
    dnl non standard place: note that imap2000 includes <ssl.h> and not
    dnl <openssl/ssl.h> but SSL headers themselves use the second form, so
    dnl we need both
    case "$ssl_inc" in
	/usr/include)
	    dnl no need to -I/usr/include to the compiler options
	    ;;
	*)
	    SSL_CPPFLAGS="-I$ssl_inc $SSL_CPPFLAGS"
    esac

    IMAP_CFLAGS="-I$ssl_inc/openssl $SSL_CPPFLAGS $IMAP_CFLAGS"
    CPPFLAGS="$SSL_CPPFLAGS $CPPFLAGS"

    dnl c-client default is "nopwd" which means that plain text authentication
    dnl is disabled for non encrypted session, we prefer traditional Unix
    dnl behaviour (which can be less safe but let's people do what they want)
    IMAP_SSLTYPE="unix"
else
    dnl tell c-client that we don't want to use SSL
    IMAP_SSLTYPE="none"
fi

AC_SUBST(IMAP_SSLTYPE)

dnl --------------------------------------------------------------------------
dnl check for other libraries
dnl --------------------------------------------------------------------------

dnl we might need dl (depending on wxGTK and python config),
dnl so link with it if it is available 
AC_CHECK_LIB(dl,main)

dnl c-client library needs the crypt() function which is either in the 
dnl c library or in libcrypt
AC_CHECK_FUNC(crypt,,
  [AC_CHECK_LIB(crypt,crypt,,
    [AC_MSG_ERROR([Cannot find crypt function.
It appears that the c library does not contain it, nor is libcrypt
installed on your system.  It is needed for password authentication,
so before compiling Mahogany, you need to install libcrypt.  You
can download the source for a suitable libcrypt from the Mahogany
ftp server (ftp://ronnie.phy.hw.ac.uk/pub/Mahogany/Libraries).])])]) 

dnl If we have a libshadow, link with it. I am not sure whether we
dnl really need this.
AC_CHECK_LIB(shadow,main)

dnl libresolv or libbind (or simply libc under FreeBSD)
USE_RBL=1
AC_CHECK_FUNC(res_query,,
    AC_CHECK_LIB(resolv,res_query,,
	AC_CHECK_LIB(bind,res_query,,
	    [AC_MSG_WARN(cannot use RBL spam checker.); USE_RBL=0])))
case "$USE_RBL" in 1) AC_DEFINE(USE_RBL) ;; esac

dnl --------------------------------------------------------------------------
dnl check for python
dnl --------------------------------------------------------------------------

dnl which Python version are we going to use?
PYTHON_VER=

dnl Check for Python headers first: we need this for both dynamic and static
dnl linking
case "$USE_PYTHON" in
static|Static|STATIC|dynamic|Dynamic|DYNAMIC|1)
	for ver in 2.7 2.6 2.5 2.4 2.3 2.2 2.1 2.0 1.5; do
	    M_CHECK_MYHEADER_VER(
		Python.h,
		$ver,
		[ \
		    $prefix/include/python$ver \
		    /usr/include/python$ver \
		    /usr/local/include/python$ver \
		    /System/Library/Frameworks/Python.framework/Versions/$ver/include/python$ver \
		],
		[
		    PYTHON_VER=$ver
		    break
		]
	    )
	done
	if test "x$PYTHON_VER" = "x"; then
	    USE_PYTHON=0
	fi
	;;

none|0)
	USE_PYTHON=0
	;;

*)
	USE_PYTHON=0
	AC_MSG_WARN(Unrecognized --with-python option value.)
	;;
esac

dnl now check for the Python lib but only if linking statically
case "$USE_PYTHON" in
dynamic|Dynamic|DYNAMIC)
	if test "$USE_MAC" = 1; then
	    AC_MSG_ERROR([Dynamic Python loading is not supported under Mac OS])
	fi
	AC_DEFINE(USE_PYTHON_DYNAMIC)
	;;

static|Static|STATIC|1)
	if test "$USE_MAC" = 1; then
	    PYTHON_LIBS="-framework Python"
	    AC_CACHE_CHECK(if we can link with Python framework,
			   m_cv_framework_python,
			   [
			    LIBS_OLD=$LIBS
			    LIBS="$LIBS $PYTHON_LIBS"
			    AC_TRY_LINK([#include "python.h"],
					[Py_Initialize();],
					m_cv_framework_python=yes,
					m_cv_framework_python=no)
			    LIBS="$LIBS_OLD"
			   ])
	    if test "$m_cv_framework_python" != "yes"; then
		PYTHON_LIBS=""
	    fi
	fi

	if test "x$PYTHON_LIBS" = "x"; then
	    M_CHECK_MYLIB(python$PYTHON_VER, main,
	    [ \
		$prefix/lib \
		$prefix/lib/python$PYTHON_VER \
		$prefix/lib/python$PYTHON_VER/config \
		/usr/lib \
		/usr/lib/python$PYTHON_VER \
		/usr/lib/python$PYTHON_VER/config \
		/usr/local/lib \
		/usr/local/lib/python$PYTHON_VER \
		/usr/local/lib/python$PYTHON_VER/config \
	    ],
	    [
		VARNAME="libpath_python`echo $PYTHON_VER | sed 's/\./_/'`_main"
		PYTHON_LIBS="`eval echo $\`echo $VARNAME\`` -lpython$PYTHON_VER"
	    ],
	    [
		USE_PYTHON=0
	    ])
	    if test "$USE_PYTHON" != 0; then
		dnl extra libs required by python on freebsd
		case "$OSTYPE" in freebsd* | FreeBSD*)
		    PYTHON_LIBS="-lreadline $PYTHON_LIBS"
		    ;;
		esac

		dnl Python2 uses forkpty() and openpty() which are not in libc
		dnl under Linux but in libutil
		if test "$PYTHON_VER" != "1.5"; then
		    AC_CHECK_FUNC(openpty,,
			    AC_CHECK_LIB(util,openpty,
				PYTHON_LIBS="-lutil $PYTHON_LIBS"))
		fi
	    fi
	fi
	;;

0)
	;;

*)
	AC_MSG_WARN(configure logic error, please report this bug)
	;;
esac

case "$USE_PYTHON" in
0)	MAKE_USE_PYTHON=''
	AC_MSG_WARN([Mahogany will be built without embedded Python interpreter])
	PYTHON_LIBS=''
	PYTHON_VER=""
	;;

*)	AC_DEFINE(USE_PYTHON)
	MAKE_USE_PYTHON="USE_PYTHON=yes"
	if test "x$USE_SWIG" = "x1"; then
	    dnl We used to have rather elaborate checks for SWIG version
	    dnl because it used to change a lot in not always backwards
	    dnl compatible ways but this seems to not be the case any more
	    dnl now, at least for the simple ways we use it in. So now we just
	    dnl assume that everything newer the oldest knowing to work
	    dnl version is ok until the next major version number change.

	    dnl if this changes, don't forget to change the tests for swig_ver
	    dnl below as well
	    swig_min_ver=1.3.25

	    AC_PATH_PROG(SWIG, swig)
	    if test "x$SWIG" = "x"; then
		AC_MSG_WARN([
*** Cannot find SWIG -- will use precompiled files for Python support.
*** If you want to modify the interface files, please install SWIG >= $swig_min_ver.])
		USE_SWIG=0
	    else
		AC_MSG_CHECKING([for swig version])
		swig_ver=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/SWIG Version //'`
		AC_MSG_RESULT([$swig_ver])

		case "$swig_ver" in
		    1.1.* | 1.2.* )
			swig_ver_status="old"
			;;

		    1.3.*)
			swig_ver_micro=`echo $swig_ver | sed 's/1\.3\.//'`
			if test $swig_ver_micro -lt 25; then
			    swig_ver_status="old"
			fi
			;;

		    2.*.*)
			;;

		    [[3-9]]*.[[0-9]]*.[[0-9]]*)
			swig_ver_status="new"
			;;

		    *)
			AC_MSG_WARN([Unrecognized swig version format "$swig_ver"])
			swig_ver_status="bad"
			;;
		esac

		case "$swig_ver_status" in
		    bad )
			AC_MSG_WARN([
*** Version of your swig program ($SWIG) couldn't be determined, please report
*** this to us. Assuming it does not work and continuing without swig.])
			USE_SWIG=0
			;;

		    old )
			AC_MSG_WARN([
*** Your swig program ($SWIG) is version $swig_ver but at least $swig_min_ver is required.])
			USE_SWIG=0
			;;

		    new )
			AC_MSG_WARN([
*** Your swig program ($SWIG) is version $swig_ver but the last tested
*** version is 1.3.36 -- still continuing hoping that it is going to work,
*** but if it doesn't, please rerun configure with --without-swig option.])
			;;
		esac
	    fi
	fi
	;;
esac
AC_SUBST(MAKE_USE_PYTHON)

dnl check for xgettext/msgmerge
MAKE_XLATE='TRANSLATIONS := yes'
AC_PATH_PROG(XGETTEXT,xgettext)
AC_PATH_PROG(MSGMERGE,msgmerge)
case ".$XGETTEXT.$MSGMERGE." in *..*)
	AC_MSG_WARN([cannot rebuild message translations.
		    You must install xgettext/msgmerge in order to do it.])
	MAKE_XLATE='TRANSLATIONS := no'
esac
AC_SUBST(MAKE_XLATE)

# compile/install binary translations
AC_PATH_PROG(MSGFMT,msgfmt)
case "$MSGFMT" in '')
	AC_MSG_WARN([cannot compile message translations.
		    Binary translations will not be installed.  You need the
		    'msgfmt' program to compile message translations.]) ;;
esac

dnl --------------------------------------------------------------------------
dnl check for (wx)gtk libraries
dnl --------------------------------------------------------------------------

dnl check for wxWidgets installation
MIN_WX_VERSION=2.9.4

dnl notice that libraries option should use correct order, otherwise statically
dnl linking won't work!
wxlibs=html,adv,qa,core,xml,net,base

WX_CONFIG_CHECK($MIN_WX_VERSION, [wxOK=1],[wxOK=0],[$wxlibs], [--debug=$debug_option])

if test "$wxOK" != 1; then
    if test $debug_option = "yes"; then
	wxkind=debug
    else
	wxkind=release
    fi

    AC_MSG_ERROR([
	   Mahogany requires wxWidgets to be installed on your system
	   but the required wxWidgets version couldn't be found.

	   Please check that wx-config is in PATH, the directory
	   where wxWidgets libraries are installed (returned by
	   'wx-config --libs' command) is in LD_LIBRARY_PATH or
	   equivalent variable and wxWidgets version is $MIN_WX_VERSION or above
	   and is available in $wxkind version.
   ])
fi

dnl set all flags: notice that currently (as of version 2.5.1)
dnl WX_CXXFLAGS_ONLY does not include C flags but we do need them for C++
dnl files compilation as well
CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
CXXFLAGS="$CXXFLAGS $WX_CFLAGS_ONLY $WX_CXXFLAGS_ONLY"

dnl normal linking mode is fully dynamic
GUI_LIBS="$WX_LIBS"
AC_SUBST(GUI_LIBS)

dnl 1/4 static means link wxWin statically, the rest dynamically
GUI_LIBS_QUARTSTATIC="$WX_LIBS_STATIC"
AC_SUBST(GUI_LIBS_QUARTSTATIC)

dnl 1/2 static means to link GTK+ statically as well
GUI_LIBS_SEMISTATIC=`echo $GUI_LIBS_QUARTSTATIC | sed '
    s/-lgtk/-Wl,-Bstatic &/g
    s/-lglib/& -Wl,-Bdynamic/g
'`
AC_SUBST(GUI_LIBS_SEMISTATIC)

dnl finally, fully static
GUI_LIBS_STATIC=`echo $WX_LIBS_STATIC | sed 's/-rdynamic//g'`
AC_SUBST(GUI_LIBS_STATIC)

dnl Mac-specific post-link step
if test "$USE_MAC" = 1; then
    AC_PATH_PROG(SETFILE, SetFile, true, [$PATH:/Developer/Tools])

    REZFLAGS=`$WX_CONFIG_WITH_ARGS --rezflags`
    AC_SUBST(REZFLAGS)
fi

dnl we want to have a user-readable wxWidgets port name
WX_NAME=`$WX_CONFIG_WITH_ARGS --basename | sed 's/^wx_//'`
WX_NAME_NO_DEBUG=`echo $WX_NAME | sed 's/d$//'`
if test "$WX_NAME_NO_DEBUG" != "$WX_NAME"; then
    WX_NAME="$WX_NAME_NO_DEBUG (debug build)"
fi

case "$USE_THREADS" in 1)
	AC_CACHE_CHECK(if wxWidgets was compiled with threads,
		m_cv_wx_threads,
		[AC_TRY_RUN([#include "wx/setup.h"
			int main(){
			#ifdef wxUSE_THREADS
				return 0;
			#else
				return 1;
			#endif
			}],
			m_cv_wx_threads=yes,
			m_cv_wx_threads=no,
			m_cv_wx_threads=x)])
	case "$m_cv_wx_threads" in
	yes)	AC_DEFINE(USE_THREADS) ;;
	no)	m_cv_USE_THREADS=0	# turn off our option
		AC_MSG_WARN([Threads configured but not available]) ;;
	x)	AC_MSG_WARN([Cross-compiling are we?
		    Geeze, I hope you know what you're doing...])
		m_cv_wx_threads=yes
		AC_DEFINE(USE_THREADS) ;;
	esac
esac

dnl --------------------------------------------------------------------------
dnl c-client library: set the proper OSTYPE and compiler variables
dnl --------------------------------------------------------------------------

dnl we need to distinguish between different Linux versions before guessing
dnl CCOSTYPE
case "$OSTYPE" in
   linux* | Linux*)
      AC_CHECK_LIB(pam, pam_end, CCOSTYPE=lnp, CCOSTYPE=slx)
      ;;
esac

dnl we need to set the target for c-client library except for mingw, for which
dnl we use separate makefile
case "$USE_MINGW" in 
0)
case "$USE_CCOSTYPE" in
''|0|1)	AC_MSG_CHECKING(for c-client library OS type)
	case "$OSTYPE" in
	linux* | Linux*)
		;; dnl CCOSTYPE already set above
	Solaris* | solaris* | SunOS*)
		case "$GXX" in
		yes)	CCOSTYPE=gso ;;
		*)	CCOSTYPE=sol ;;
		esac ;;
	freebsd* | FreeBSD*)
		CCOSTYPE=bsf ;;
	hpux_9*)
		CCOSTYPE=hpp ;;
	hpux_10*)
		CCOSTYPE=hpx ;;
	OSF1*)
		case "$GXX" in
		yes)	CCOSTYPE=gof ;;
		*)	CCOSTYPE=osf ;;
		esac ;;
	IRIX*)
		case "$GXX" in
		yes)	CCOSTYPE=gsg ;;
		*)	dnl should use sg6 for IRIX 6.5, sgi otherwise
			case `uname -r` in
			    6.5*) CCOSTYPE=sg6 ;;
			    *) 	  CCOSTYPE=sgi ;;
			esac
		esac ;;
	darwin* | Darwin*)
		CCOSTYPE=osx ;;
	OpenBSD*)
		CCOSTYPE=bso ;;
	NetBSD*)
		CCOSTYPE=neb ;;
	cygwin* | CYGWIN*)
		CCOSTYPE=cyg ;;
	*)
		AC_MSG_RESULT(unknown)
		AC_MSG_ERROR([cannot guess c-client OS type for $OSTYPE,
please have a look at lib/imap/Makefile and use --with-ccostype=xxx
with xxx being the most appropriate system type from this file.])
		;;
	esac
	dnl cache it to be picked up as a parameter next time
	m_cv_USE_CCOSTYPE="$CCOSTYPE"
	AC_MSG_RESULT($CCOSTYPE) ;;
*)	CCOSTYPE="$USE_CCOSTYPE" ;;
esac
;;
1)
;;
esac
AC_SUBST(CCOSTYPE)
case "$CCOSTYPE" in
lnp)	LIB_PAM="-lpam" ;;
*)	LIB_PAM= ;;
esac

dnl this must come after we checked for wxGTK and network libraries
dnl Check if we have the pi-sock library and headers for PalmOS support.
MAKE_PISOCKLIB=
case "$USE_PISOCK" in 
1)
    AC_CHECK_HEADER(pi-source.h,
     AC_CHECK_LIB(pisock, main,
      [ AC_DEFINE(USE_PISOCK)
	MAKE_PISOCKLIB="PISOCK_LIB:=-lpisock"
	AC_CHECK_LIB(pisock, pi_setmaxspeed,AC_DEFINE(HAVE_PI_SETMAXSPEED),
	AC_MSG_WARN(Your pisock library is old - some functionality will be disabled.))
	AC_CHECK_LIB(pisock, pi_accept_to,AC_DEFINE(HAVE_PI_ACCEPT_TO),
	AC_MSG_WARN(Your pisock library is old - some functionality will be disabled.))
      ],
      AC_MSG_WARN(Cannot find pisock library - support for PalmOS disabled.)
     ),
     AC_MSG_WARN(Cannot find pisock headers - support for PalmOS disabled.)
    )
    ;;
esac
AC_SUBST(MAKE_PISOCKLIB)

dnl Check if we have libmal source in lib/libmal
MAKE_HAVE_LIBMAL=
case "$MAKE_PISOCKLIB" in
'') ;;
*)
	AC_MSG_CHECKING(for libmal in lib)
	if test ! -d $srcdir/lib/libmal 
	then	AC_MSG_RESULT(not found - no MAL synch for PalmOS)
	elif test -f $srcdir/lib/libmal/Makefile.in \
		-a "$SOURCEDIR" != "$BUILDDIR"
	then	AC_MSG_RESULT(old-style libmal not supported in separate build tree)
	else	AC_MSG_RESULT(found - MAL synch for PalmOS activated)
		MAKE_HAVE_LIBMAL="HAVE_LIBMAL=yes"
	fi
esac
AC_SUBST(MAKE_HAVE_LIBMAL)

dnl --------------------------------------------------------------------------
dnl check whether we use dspam
dnl --------------------------------------------------------------------------

MAKE_USE_DSPAM=
using_dspam="no"
case "$USE_DSPAM" in
    1)
	using_dspam="yes"
	MAKE_USE_DSPAM="USE_DSPAM=1"
	;;
esac

AC_SUBST(MAKE_USE_DSPAM)

dnl --------------------------------------------------------------------------
dnl How do we want to use modules?
dnl --------------------------------------------------------------------------

if test "$USE_MAC" = 1; then
    dnl back-linking doesn't seem to work under Mac OS X: we can create the
    dnl libraries but they can't be loaded at run-time, so until this is fixed
    dnl we simply always link the modules statically
    USE_MODULES="static"
fi

dnl must be done after configuring wxWin for auto detection to work
MODULELDFLAGS=
case "$USE_MODULES" in
none|None|NONE|0|'')
	m_cv_USE_MODULES=none
	MAKE_USE_MODULES=
	AC_MSG_WARN(The program will be hardly usable without modules.)
	;;
dynamic|Dynamic|DYNAMIC)
	m_cv_USE_MODULES=dynamic
	AC_DEFINE(USE_MODULES)
	MAKE_USE_MODULES="USE_MODULES=dynamic"
	;;
static|Static|STATIC)
	m_cv_USE_MODULES=static
	AC_DEFINE(USE_MODULES_STATIC)
	AC_DEFINE(USE_MODULES)
	MAKE_USE_MODULES="USE_MODULES=static"
	;;
auto|Auto|AUTO|1)
	AC_MSG_CHECKING(how to link modules)
	dnl check for wxGTK dynamic linkage
	if test "$USE_MAC" = 0; then
	    LDD="ldd"
	else
	    LDD="otool -L"
	fi
	echo "int main() { return 0; }" >conftest.c
	if $CXX -o conftest $WX_LIBS conftest.c 2>link.log >&2 && \
		$LDD conftest | fgrep libwx 2>/dev/null >&2
	then
		m_cv_USE_MODULES=dynamic
		MAKE_USE_MODULES="USE_MODULES=dynamic"
		AC_MSG_RESULT(dynamically)
	else
		m_cv_USE_MODULES=static
		MAKE_USE_MODULES="USE_MODULES=static"
		AC_DEFINE(USE_MODULES_STATIC)
		AC_MSG_RESULT(statically)
	fi
	rm -f conftest*
	AC_DEFINE(USE_MODULES)
	;;
*)
	AC_MSG_ERROR(Set --with-modules= to one of static/dynamic/none)
esac

if test "$m_cv_USE_MODULES" = "dynamic"; then
    dnl modules need to be compiled as PIC
    if test "$GCC" = "yes"; then
	SHARED_CFLAGS="-fPIC"
    fi

    dnl under some systems we have to use -Wl,-export-dynamic or the
    dnl modules don't find symbols in M
    case "$OSTYPE" in
	linux* | Linux* | freebsd* | FreeBSD*)
		MODULELDFLAGS="-export-dynamic"
		;;
    esac
fi

AC_SUBST(MAKE_USE_MODULES)
AC_SUBST(MODULELDFLAGS)
AC_SUBST(SHARED_CFLAGS)

dnl --------------------------------------------------------------------------
dnl set up generators for class documentation
dnl --------------------------------------------------------------------------

dnl use scandoc
DOCHTML="\$(SCANDOCHTML)"	AC_SUBST(DOCHTML)
DOCTEX="\$(SCANDOCTEX)"		AC_SUBST(DOCTEX)

AC_DEFUN([M_DOC_TOOL], [AC_PATH_PROG($1,$2)dnl
case "$$1" in '') MAKE_HAVE_DOCTOOLS='' $1='false "No $2 available"';; esac])dnl

dnl check for latex/dvips
MAKE_HAVE_DOCTOOLS='HAVE_DOCTOOLS=yes'
M_DOC_TOOL(PERL,perl)
M_DOC_TOOL(LATEX,latex)
M_DOC_TOOL(MAKEINDEX,makeindex)
M_DOC_TOOL(DVIPS,dvips)
M_DOC_TOOL(PSTOPDF,ps2pdf)
M_DOC_TOOL(LATEX2HTML,latex2html)
case "$MAKE_HAVE_DOCTOOLS" in
    '') AC_MSG_WARN([One of the tools required to build documentation
		    is missing.  Documentation cannot be built on this system
		    and you will need a pre-built Mdocs.tar.gz instead.])
    ;;
esac
AC_SUBST(MAKE_HAVE_DOCTOOLS)

dnl --------------------------------------------------------------------------
dnl add our extra directories to the search path
dnl --------------------------------------------------------------------------

dnl set up the include directories
CPPFLAGS="-I$SOURCEDIR/include $CPPFLAGS"
case "$BUILDDIR" in "$SOURCEDIR") ;; *)
	CPPFLAGS="-I$BUILDDIR/include $CPPFLAGS" ;;
esac

dnl set include flags for cygwin resources
dnl wx installation directory is needed for cygwin resources
WX_INSTALL_DIR=`$WX_CONFIG_WITH_ARGS --prefix`

RESFLAGS="--include-dir=$WX_INSTALL_DIR/include --include-dir=$SOURCEDIR/res"

RESFLAGS="--include-dir=$SOURCEDIR/include $RESFLAGS"
case "$BUILDDIR" in "$SOURCEDIR") ;; *)
	RESFLAGS="--include-dir=$BUILDDIR/include $RESFLAGS" ;;
esac
AC_SUBST(RESFLAGS)

dnl add 3rd party libraries directories to the search paths
dnl	c-client
CPPFLAGS="$CPPFLAGS -I$BUILDDIR/lib/imap/c-client"
LIBS="$BUILDDIR/lib/imap/c-client/c-client.a $LIBS"
dnl	compface
CPPFLAGS="$CPPFLAGS -I$SOURCEDIR/lib/compface -I$SOURCEDIR/src/wx/vcard"
LIBS="$BUILDDIR/lib/compface/libcompface.a $LIBS"

dnl --------------------------------------------------------------------------
dnl final settings, concatenation of args, etc
dnl --------------------------------------------------------------------------

dnl add other flags
CFLAGS="$CFLAGS $CODE_GEN_FLAGS"
CXXFLAGS="$CXXFLAGS $CODE_GEN_FLAGS"
LIBS="$LIBS $PYTHON_LIBS $LIB_EFENCE $LIB_DMALLOC $LIB_PAM"

dnl imap
IMAP_CFLAGS="$CFLAGS $IMAP_CFLAGS"
dnl there are too many warnings in c-client sources which its author
dnl refuses to fix, suppress them if possible.
AX_C_CHECK_FLAG(-Wno-pointer-sign,,,[
	IMAP_CFLAGS="$IMAP_CFLAGS -Wno-pointer-sign"
])

dnl assume that if -Wno-pointer-to-int-cast is supported, then so is the
dnl opposite option
AX_C_CHECK_FLAG(-Wno-pointer-to-int-cast,,,[
	IMAP_CFLAGS="$IMAP_CFLAGS -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast"
])
AC_SUBST(IMAP_CFLAGS)

dnl compface
CFACE_CPPFLAGS="${CPPFLAGS}"
CFACE_CFLAGS="${CFLAGS}"
case "$OSTYPE" in
    darwin* | Darwin*)
	# Apple gcc version is buggy and requires this switch to compile
	# compface sources and twofish.c (at least for 3.1 from Dec 2002 dev
	# tools)
	CFACE_CFLAGS="${CFACE_CFLAGS} -no-cpp-precomp"
	;;
esac

AC_SUBST(CFACE_CPPFLAGS)
AC_SUBST(CFACE_CFLAGS)

dnl libmal
LIBMAL_CPPFLAGS="${CPPFLAGS}"
LIBMAL_CFLAGS="${CFLAGS}"
LIBMAL_CXXFLAGS="${CXXFLAGS}"
AC_SUBST(LIBMAL_CPPFLAGS)
AC_SUBST(LIBMAL_CFLAGS)
AC_SUBST(LIBMAL_CXXFLAGS)

dnl Only now can we add them, or otherwise some tests would have failed
case "$GCC" in yes)
	case "$USE_STRICT" in
	1)	CFLAGS="$CFLAGS -Werror -pedantic -Wno-long-long" ;;
	*)	CFLAGS="$CFLAGS -Wall" ;;
	esac ;;	
esac

dnl Only now can we add them, or otherwise some tests would have failed
dnl how to generate dependencies...
dnl
dnl for now, we just support -MD on g++ and also OSF/1 Compaq cc compiler which
dnl supports the same options
DEPFLAGS=
if test "x$GXX" = "xyes" -o "x$CCOSTYPE" = "xosf"; then
    DEPFLAGS="-MMD"
else
    dnl non-gcc compiler switches depend on platform (we assume that we use
    dnl the native compiler if we don't use gcc)
    case "$OSTYPE" in
	OSF1*)
	    DEPFLAGS="-MMD"
	    ;;

	Solaris* | solaris* | SunOS*)
	    dnl Sun Forte C/C++ compiler is reported to have these
	    DEPFLAGS="-xM1"
	    ;;

	IRIX*)
	    dnl see http://mahogany.sourceforge.net/cgi-bin/show_bug.cgi?id=699
	    DEPFLAGS="-M"
	    ;;
    esac
fi

dnl now check that these flags are really supported by the compiler
if test "x$DEPFLAGS" != "x"; then
    dnl check that the compiler really accepts it unless it is gcc for which
    dnl we're sure that it always does
    if test "x$GXX" != "xyes"; then
	AC_CACHE_CHECK(whether ${CXX} accepts ${DEPFLAGS},
		m_cv_cxx_depflags,
		[
		    echo 'void f(){}' >conftest.cc
		    case "`${CXX} ${DEPFLAGS} -c conftest.cc 2>&1`" in
			'')	m_cv_cxx_depflags=yes ;;
			*)	m_cv_cxx_depflags=no ;;
		    esac
		    rm -f conftest*
		])
	if test "x$m_cv_cxx_depflags" != "xyes"; then
	    DEPFLAGS=""
	fi
    fi

    if test "x$DEPFLAGS" != "x"; then
	if test "x$GCC" != "xyes"; then
	    AC_CACHE_CHECK(whether ${CC} accepts ${DEPFLAGS},
		    m_cv_c_depflags,
		    [
			echo 'void f(){}' >conftest.cc
			case "`${CXX} ${DEPFLAGS} -c conftest.cc 2>&1`" in
			    '')	m_cv_c_depflags=yes ;;
			    *)	m_cv_c_depflags=no ;;
			esac
			rm -f conftest*
		    ])
	    if test "x$m_cv_c_depflags" != "xyes"; then
		DEPFLAGS=""
	    fi
	fi
    fi
fi

dnl finally use the DEPFLAGS we found above
if test "x$DEPFLAGS" != "x"; then
    CXXFLAGS="$CXXFLAGS $DEPFLAGS"
    CFLAGS="$CFLAGS $DEPFLAGS"
else
    AC_MSG_WARN([*** dependencies not supported ***
		    Dependencies are not supported for this compiler.  If this
		    message appears, then we need to talk -- we think we know
		    how to fix this, but we don't have a machine where we can
		    test it.  Please go to http://mahogany.sourceforge.net/bugz
		    and submit a bug against the Build component so we can
		    give you some code to try.])
fi

dnl Python code produces warnings, so don't break compilation:
CXXFLAGS_RELAXED="$CXXFLAGS"
case "$GXX" in yes)
	case "$USE_STRICT" in
	1)	CXXFLAGS_RELAXED="$CXXFLAGS_RELAXED -Wall -pedantic -Wno-long-long"
		CXXFLAGS="$CXXFLAGS -Werror -pedantic -Wno-long-long" ;;
	*)	CXXFLAGS_RELAXED="$CXXFLAGS_RELAXED -Wall"
		CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"

		dnl Unfortunately we have to disable this warning globally for the
		dnl compiler versions giving it (8+), as the use of PCH prevents
		dnl disabling it locally in wxWidgets from working. This can only be
		dnl avoided if we stop using the PCH or if we change all the code using
		dnl wx event table macros to use Bind() instead.
		M_ADD_CXXFLAGS_IF_SUPPORTED(-Wno-cast-function-type)
		;;
	esac ;;	
esac
AC_SUBST(CXXFLAGS_RELAXED)

# gather all directories where we want to create .src links in DIR and all
# Makefiles in COPY
OUTPUT="makeopts"
COPY=Makefile
DIR=
for dir in `cd $srcdir; find include lib extra src locale doc res -type d -print`; do
    case "$dir" in
	CVS | */CVS | \
	lib/imap/*)
	    # ignore
	    ;;

	*)
	    # normal case
	    if test -f $srcdir/$dir/Makefile.M; then
		    COPY="$COPY $dir/Makefile.M"
	    elif test -f $srcdir/$dir/Makefile; then
		    COPY="$COPY $dir/Makefile"
	    elif test ! -f $srcdir/$dir/Names.mk; then
		continue
	    fi
	    DIR="$DIR $dir"
    esac
done
# HACK for backward compatibility; clean it up when old libmal is dead
case "$MAKE_HAVE_LIBMAL" in '') ;; *)
	if test -f $srcdir/extra/src/libmal/Makefile.in
	then	OUTPUT="$OUTPUT extra/src/libmal/Makefile"
	else	COPY="$COPY extra/src/libmal/Makefile"
	fi ;;
esac

AC_OUTPUT_COMMANDS(
[
	case "$SRC" in
	    "$OBJ")	# source and object jumbled together
		for dir in . $DIR
		do
		    rm -f $dir/.src
		    ln -s . $dir/.src
		done
		;;

	    *)	# using separate object directories
		rm -f .src Makefile makerules
		ln -s $src .src
		ln -s .src/Makefile Makefile
		ln -s .src/makerules makerules
		for dir in $DIR
		do
		    test -d $dir || mkdir $dir
		    rm -f $dir/.src
		    case "$src" in
		    /*)	ln -s $src/$dir $dir/.src ;;
		    *)	lnk=`echo "$dir" | sed 's,\([^/][^/]*\),..,g'`
			ln -s $lnk/$src/$dir $dir/.src ;;
		    esac
		done
		for mf in $COPY
		do
		    echo "linking $mf"
		    rm -f $mf
		    ln -s .src/`basename $mf` $mf
		done
		;;
	esac

	# be careful to use only standard BRE features: this is why we use
	# \{1,\} instead of + for example
	grep "^#define M_VERSION_[MR]" $src/include/Mversion.h | \
	    sed 's/^#define \{1,\}\([A-Z_]\{1,\}\) \{1,\}\([0-9]\{1,\}\)/\1=\2/' > makeversion
],
src="$srcdir" SRC="$SOURCEDIR" OBJ="$BUILDDIR"
DIR="$DIR"
COPY="$COPY")

AC_OUTPUT($OUTPUT)

dnl show the configuration used
. ./makeversion

echo
echo "Configured Mahogany ${M_VERSION_MAJOR}.${M_VERSION_MINOR}.${M_VERSION_RELEASE} for ${host}."
echo ""
echo "	wxWidgets port used:		${WX_NAME} ${WX_VERSION}"
echo "	Debug mode:			${debug_option}"
echo "	SSL support:			${ssl_option}"
echo "	Modules support:		${m_cv_USE_MODULES}"
echo "	Embedded Python:		${PYTHON_VER:-none}"
if test "$USE_PYTHON" != 0 -a "$USE_SWIG" = 1; then
    echo "	      with swig:		${SWIG}"
fi
echo "	DSPAM spam filtering:		${using_dspam}"
echo ""
echo "Please look at config.log for more details."

exit 0

dnl vi: set noet ts=8 sw=4 nolist sta:
