# Specify the project and version + the minimum automake version required.
AC_PREREQ(2.66)
AC_INIT([charisma], [1.0.0])

# Instruct automake to place all intermediary files into a seperate directory.
# If this isn't specified, then it will pollute the main directory.
AC_CONFIG_AUX_DIR([config])

# Sanity check to verify this is indeed the source directory.
AC_CONFIG_SRCDIR(charisma.c)

# Create a header with the detected features.
AC_CONFIG_HEADERS([config.h])

# Initialize automake and use "foreign" so it doesn't attmpet to look for or
# create a GNU project structure: COPYING, NEWS, etc...
AM_INIT_AUTOMAKE([foreign])

# Check for the --with-endian option
AC_ARG_WITH([endian],
    [AS_HELP_STRING([--with-endian=BIG|LITTLE], [Specify the endianness (BIG or LITTLE)])],
    [],
    [with_endian=auto])  # Default to "auto" if no value is provided

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
LT_INIT([win32-dll])

# Perform all tests in C.
AC_LANG([C])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h], [], [AC_MSG_ERROR([expected stdint.h])])
AC_CHECK_HEADERS([assert.h], [], [AC_MSG_ERROR([expected assert.h])])

# If user specified "BIG" or "LITTLE", define the constant accordingly.
AS_IF([test "$with_endian" = "BIG"], [
  AC_DEFINE([HAVE_BIG_ENDIAN], [1], [Define to 1 if the system is big-endian.])
], [
  AS_IF([test "$with_endian" = "LITTLE"], [
    # Default behavior.
  ], [
    # Fallback to auto-detecting the endianness if not specified.
    AC_C_BIGENDIAN
    AS_IF([test "$ac_cv_c_bigendian" = "yes"], [
      AC_DEFINE([HAVE_BIG_ENDIAN], [1], [Define to 1 if the system is big-endian.])
    ], [])
  ])
])

# Begin searching for a byte swap function.
have_bswap="no"

# Check for FreeBSD bswap functions.
AS_IF([test "${have_bswap}" = "no"], [
  AC_MSG_CHECKING(for bswap16 and bswap32)
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
        #include <sys/endian.h>
        #include <stdint.h>
      ]],[[
        uint16_t x;
        uint32_t y;
        x = bswap16(&x);
        y = bswap32(&y);
      ]]
    )], [have_bswap="yes"])
  AC_MSG_RESULT(${have_bswap})
  AS_IF([test "${have_bswap}" = "yes"], [
    AC_DEFINE_UNQUOTED([HAVE_FREEBSD_BSWAP], [1], [Define to 1 if you have the `bswap` functions.])
  ])
])

# Check for Linux bswap functions.
AS_IF([test "${have_bswap}" = "no"], [
  AC_MSG_CHECKING(for bswap_16 and bswap_32)
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
        #include <byteswap.h>
        #include <stdint.h>
      ]],[[
        uint16_t x;
        uint32_t y;
        x = bswap_16(&x);
        y = bswap_32(&y);
      ]]
    )], [have_bswap="yes"])
  AC_MSG_RESULT(${have_bswap})
  AS_IF([test "${have_bswap}" = "yes"], [
    AC_DEFINE_UNQUOTED([HAVE_LINUX_BSWAP], [1], [Define to 1 if you have the `bswap_` functions.])
  ])
])

# Check for compiler intrinsic functions.
AS_IF([test "${have_bswap}" = "no"], [
  AC_MSG_CHECKING(for __builtin_bswap)
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
        #include <stdint.h>
      ]],[[
        uint16_t x;
        uint32_t y;
        x = __builtin_bswap16(&x);
        y = __builtin_bswap32(&y);
      ]]
    )], [have_bswap="yes"])
  AC_MSG_RESULT(${have_bswap})
  AS_IF([test "${have_bswap}" = "yes"], [
    AC_DEFINE_UNQUOTED([HAVE_GETTIMEOFDAY], [1], [Define to 1 if you have the `__builtin_bswap` intrinsics.])
  ])
])

# Check for MSVC CRT functions (Windows)
AS_IF([test "${have_bswap}" = "no"], [
  AC_MSG_CHECKING(for _byteswap)
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
        #include <stdint.h>
        #include <stdlib.h>
      ]],[[
        uint16_t x;
        uint32_t y;
        if (sizeof(uint16_t) != sizeof(unsigned short) ||
            sizeof(uint32_t) != sizeof(unsigned long)) {
            return 1;
        }
        x = _byteswap_ushort(&x);
        y = _byteswap_ulong(&y);
      ]]
    )], [have_bswap="yes"])
  AC_MSG_RESULT(${have_bswap})
  AS_IF([test "${have_bswap}" = "yes"], [
    AC_DEFINE_UNQUOTED([HAVE_MSVC_BYTESWAP], [1], [Define to 1 if you have the `_byteswap` intrinsics.])
  ])
])

AC_CONFIG_FILES([
  Makefile
  man/Makefile
  charisma.pc
])
AC_OUTPUT
