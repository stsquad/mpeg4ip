AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ @BILLS_CWARNINGS@ -fexceptions -D_OBSS_

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I.. -I../../type -I../../tools -I../../../../../include

noinst_LTLIBRARIES = libobj_sys_encoder.la

libobj_sys_encoder_la_SOURCES = \
	blkenc.cpp \
	blkenc.hpp \
	enhcbufenc.hpp \
	errenc.cpp \
	gmc_enc_util.cpp \
	gme_for_iso.cpp \
	mbenc.cpp \
	mbinterlace.cpp \
	mcenc.cpp \
	motest.cpp \
	mvenc.cpp \
	newpenc.cpp \
	padenc.cpp \
	paramset.cpp \
	paramset.h \
	sesenc.cpp \
	sesenc.hpp \
	shpenc.cpp \
	shpenc.hpp \
	sptenc.cpp \
	tm5rc.cpp \
	tm5rc.hpp \
	vopmbenc.cpp \
	vopmbenc.hpp \
	vopseenc.cpp \
	vopseenc.hpp 

