lib_LTLIBRARIES = libmp4v2.la

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/include/stl 

libmp4v2_la_SOURCES = \
	mp4.h \
	mp4.cpp \
	mp4atom.h \
	mp4atom.cpp \
	mp4file.h \
	mp4file.cpp \
	mp4property.h \
	mp4property.cpp \
	mp4util.h \
	atoms.cpp \
	descriptors.h \
	descriptors.cpp 

check_PROGRAMS = ctest cpptest

ctest_SOURCES = ctest.c
ctest_LDADD = libmp4v2.la

cpptest_SOURCES = cpptest.cpp
cpptest_LDADD = libmp4v2.la

