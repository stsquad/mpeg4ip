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
	mp4track.h \
	mp4track.cpp \
	mp4util.h \
	mp4util.cpp \
	atoms.cpp \
	descriptors.h \
	descriptors.cpp 

bin_PROGRAMS = mp4dump mp4extract

mp4dump_SOURCES = mp4dump.cpp
mp4dump_LDADD = libmp4v2.la

mp4extract_SOURCES = mp4extract.cpp
mp4extract_LDADD = libmp4v2.la

check_PROGRAMS = apitest nullcreate nullvplayer

apitest_SOURCES = apitest.c
apitest_LDADD = libmp4v2.la

nullcreate_SOURCES = nullcreate.cpp
nullcreate_LDADD = libmp4v2.la

nullvplayer_SOURCES = nullvplayer.cpp
nullvplayer_LDADD = libmp4v2.la

