AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@ -D_OBSS_

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = \
	-I../../../../../include \
	-I../../type \
	-I../../sys

noinst_LTLIBRARIES = libobj_tools_sadct.la

libobj_tools_sadct_la_SOURCES = \
   sadct.cpp
