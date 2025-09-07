SUBDIRS = man

EXTRA_DIST = autogen.sh LICENSE CMakeLists.txt CharismaConfig.cmake.in config.cmake.in README.md extras/README.md

AUTOMAKE_OPTIONS = subdir-objects

include_HEADERS = charisma.h

lib_LIBRARIES = libcharisma.a
libcharisma_a_SOURCES = charisma.c charisma.h
libcharisma_a_CFLAGS = -I$(top_srcdir)

bin_PROGRAMS = charisma
charisma_SOURCES = extras/charisma-cli.c
charisma_LDADD = libcharisma.a
