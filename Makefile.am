SUBDIRS = man

EXTRA_DIST = autogen.sh LICENSE CMakeLists.txt CharismaConfig.cmake.in config.cmake.in README.md extras/README.md

AUTOMAKE_OPTIONS = subdir-objects

include_HEADERS = charisma.h

lib_LTLIBRARIES = libcharisma.la
libcharisma_la_SOURCES = charisma.c charisma.h
libcharisma_la_LDFLAGS = -no-undefined -version-info 0:0:0

bin_PROGRAMS = charisma
charisma_SOURCES = extras/charisma-cli.c
charisma_LDADD = libcharisma.la
