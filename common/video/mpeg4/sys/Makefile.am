SUBDIRS = decoder encoder

AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I../type -I../tools -I../../../../include

noinst_LTLIBRARIES = libobj_sys.la
libobj_sys_la_LDFLAGS = -static

libobj_sys_la_SOURCES = \
			block.cpp \
			cae.cpp \
			dct.cpp \
			error.cpp \
			mb.cpp \
			mc.cpp \
			mcpad.cpp \
			mode.cpp \
			mv.cpp \
			shape.cpp \
			spt.cpp \
			tps_bfshape.cpp \
			tps_enhcbuf.cpp \
			vopses.cpp \
