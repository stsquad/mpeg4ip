lib_LTLIBRARIES = libmpeg4ip_fposrec.la
libmpeg4ip_fposrec_la_SOURCES = \
	fposrec.cpp \
	fposrec.h

EXTRA_DIST = libfileposrec60.dsp 

INCLUDES = -I../../include

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

