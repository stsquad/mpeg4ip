lib_LTLIBRARIES = libmp4v2.la

INCLUDES = -I$(top_srcdir)/include

libmp4v2_la_SOURCES = \
	mp4.h \
	mp4.cpp \
	mp4array.h \
	mp4atom.h \
	mp4atom.cpp \
	mp4common.h \
	mp4descriptor.h \
	mp4descriptor.cpp \
	mp4file.h \
	mp4file.cpp \
	mp4property.h \
	mp4property.cpp \
	mp4util.h \
	mp4util.cpp \
	atoms.cpp \
	descriptors.h \
	descriptors.cpp 

check_PROGRAMS = ctest cpptest

ctest_SOURCES = ctest.c
ctest_LDADD = libmp4v2.la

cpptest_SOURCES = cpptest.cpp
cpptest_LDADD = libmp4v2.la

