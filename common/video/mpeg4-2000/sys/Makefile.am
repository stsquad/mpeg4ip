SUBDIRS = decoder encoder

AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ @BILLS_CWARNINGS@ -fexceptions -D_OBSS_

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D__DOUBLE_PRECISION_ -D_OBSS_

INCLUDES = -I../type -I../tools -I../../../../include -Iencoder -Idecoder

noinst_LTLIBRARIES = libobj_sys.la

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
	block.hpp \
	cae.h \
	codehead.h \
	dct.hpp \
	global.hpp \
	mb.hpp \
	mode.hpp \
	shape.hpp \
	tps_enhcbuf.hpp \
	vopses.hpp \
	newpred.cpp \
	newpred.hpp \
	rrv.cpp \
	rrv.hpp \
	gmc_util.cpp \
	gmc_motion.cpp

