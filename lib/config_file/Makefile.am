lib_LTLIBRARIES = libconfig_file.la
libconfig_file_la_LDFLAGS = -static

libconfig_file_la_SOURCES = \
	config_file.cpp \
	config_file.h

EXTRA_DIST = libconfig_file60.dsp

INCLUDES = -I$(top_srcdir)/include
AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -Wall -Werror
