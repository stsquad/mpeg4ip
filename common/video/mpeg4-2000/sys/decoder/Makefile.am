AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ @BILLS_CWARNINGS@

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I.. -I../../type -I../../tools -I../../idct -I../../../../../include

noinst_LTLIBRARIES = libobj_sys_decoder.la

libobj_sys_decoder_la_SOURCES = \
			blkdec.cpp \
			errdec.cpp \
			mbdec.cpp \
			mbheaddec.cpp \
			mbinterlacedec.cpp \
			mcdec.cpp \
			mvdec.cpp \
			rvlcdec.cpp \
			shpdec.cpp \
			sptdec.cpp \
			vopmbdec.cpp \
			vopsedec.cpp \
			blkdec.hpp \
			enhcbufdec.hpp \
			mbdec.hpp \
			shpdec.hpp \
			vopmbdec.hpp \
			vopsedec.hpp \
			newpdec.cpp

