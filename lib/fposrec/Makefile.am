noinst_LTLIBRARIES = libfposrec.la
libfposrec_la_LDFLAGS = -static
libfposrec_la_SOURCES = \
	fposrec.cpp 

INCLUDES = -I../../include

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

