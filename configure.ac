dnl Process this file with autoconf to produce a configure script.
AC_INIT([mod_wsman],[1.0])

AC_PREREQ(2.59)
AC_CONFIG_AUX_DIR([pkg])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR(src/mod_wsman.c)

dnl AC_CHECK_COMPILERS
AC_PROG_CC
AC_PROG_RANLIB

# Find apxs
AC_ARG_WITH(apxs, AC_HELP_STRING([--with-apxs=NAME],
            [Path to the apxs executable [[apxs]]]),
            [APXS="$with_apxs"])
if test -z "${APXS}"; then
    AC_PATH_PROGS(APXS, apxs2 apxs, [apxs],
                  [$PATH:/usr/local/apache/bin:/usr/sbin])
fi
AC_SUBST(APXS)

PKG_CHECK_MODULES(OPENWSMAN, [openwsman-server >= 0.5.0])

AC_OUTPUT(Makefile pkg/mod_wsman.spec)
