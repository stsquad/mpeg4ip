AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -Wall -Wpointer-arith -Wcast-align -Wmissing-prototypes -fexceptions

AM_CXXFLAGS = -D__TRACE_AND_STATS -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type
noinst_LTLIBRARIES = libobj_vtc_ztq.la
libobj_vtc_ztq_la_LDFLAGS = -static

libobj_vtc_ztq_la_SOURCES = \
			QMInit.cpp \
			QMUtils.cpp \
			Utils.cpp \
			decQM.cpp \
			encQM.cpp \
			errorHandler.cpp \
			msg.cpp \
			quant.cpp
