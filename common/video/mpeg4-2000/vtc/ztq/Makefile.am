AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@ -D_OBSS_

AM_CXXFLAGS = -D__TRACE_AND_STATS -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type -I../shape
noinst_LTLIBRARIES = libobj_vtc_ztq.la

libobj_vtc_ztq_la_SOURCES = \
	decQM.cpp \
	encQM.cpp \
	errorHandler.cpp \
	errorHandler.hpp \
	globals.hpp \
	msg.cpp \
	msg.hpp \
	QM.hpp \
	QMInit.cpp \
	QMUtils.cpp \
	QMUtils.hpp \
	quant.cpp \
	quant.hpp \
	Utils.cpp \
	Utils.hpp
