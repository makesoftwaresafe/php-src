PHP_ARG_ENABLE([litespeed],
  [for LiteSpeed support],
  [AS_HELP_STRING([--enable-litespeed],
    [Build PHP as litespeed module])],
  [no])

if test "$PHP_LITESPEED" != "no"; then
  PHP_ADD_MAKEFILE_FRAGMENT([$abs_srcdir/sapi/litespeed/Makefile.frag],
    [$abs_srcdir/sapi/litespeed],
    [sapi/litespeed])
  SAPI_LITESPEED_PATH=sapi/litespeed/lsphp
  PHP_SELECT_SAPI([litespeed], [program], [lsapi_main.c lsapilib.c])
  AS_CASE([$host_alias],
    [*darwin*], [
      BUILD_LITESPEED="\$(CC) \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(NATIVE_RPATHS) \$(PHP_GLOBAL_OBJS:.lo=.o) \$(PHP_BINARY_OBJS:.lo=.o) \$(PHP_LITESPEED_OBJS:.lo=.o) \$(PHP_FRAMEWORKS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_LITESPEED_PATH)"
    ],
    [*cygwin*], [
      SAPI_LITESPEED_PATH=sapi/litespeed/lsphp.exe
      BUILD_LITESPEED="\$(LIBTOOL) --tag=CC --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(PHP_LITESPEED_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_LITESPEED_PATH)"
    ], [
      BUILD_LITESPEED="\$(LIBTOOL) --tag=CC --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS:.lo=.o) \$(PHP_BINARY_OBJS:.lo=.o) \$(PHP_LITESPEED_OBJS:.lo=.o) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_LITESPEED_PATH)"
    ])

  PHP_SUBST([SAPI_LITESPEED_PATH])
  PHP_SUBST([BUILD_LITESPEED])
fi
