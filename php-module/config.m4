dnl $Id$
dnl config.m4 for extension "gprolog"

PHP_ARG_WITH(gprolog, for gprolog support,
[  --with-gprolog[=shared]     Include GNU Prolog support])

if test "$PHP_GPROLOG" != "no"; then
  AC_DEFINE(HAVE_GPROLOG,1,[ ])
  PHP_EXTENSION(gprolog, $ext_shared)
fi
