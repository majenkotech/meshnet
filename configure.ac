AC_PREREQ([2.10])
AC_INIT([meshnet], [1.0.0], [matt@majenko.co.uk])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects no-dependencies])
AC_CONFIG_MACRO_DIR([m4])

AM_SILENT_RULES([yes])

AC_SEARCH_LIBS(mcrypt_module_open, mcrypt,[], [
    AC_MSG_ERROR([libmcrypt-dev is needed])
])

AC_SEARCH_LIBS(mhash_keygen, mhash,[], [
    AC_MSG_ERROR([libmhash-dev is needed])
])

AC_SEARCH_LIBS(LZ4_compress_default, lz4,[], [
    AC_MSG_ERROR([liblz4-dev is needed])
])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX

AC_CONFIG_FILES([
    Makefile 
    src/Makefile
    data/Makefile
])
AC_OUTPUT
