AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

noinst_LTLIBRARIES = libobj_type.la
libobj_type_la_LDFLAGS = -static

libobj_type_la_SOURCES = \
	basic.cpp \
	basic.hpp \
	geom.cpp \
	geom.hpp \
	grayc.cpp \
	grayc.hpp \
	grayf.cpp \
	grayf.hpp \
	grayi.cpp \
	grayi.hpp \
	header.h \
	svd.cpp \
	svd.h \
	transf.hpp \
	typeapi.cpp \
	typeapi.h \
	vop.cpp \
	vop.hpp \
	warp.cpp \
	warp.hpp \
	yuvac.cpp \
	yuvac.hpp \
	yuvai.cpp \
	yuvai.hpp
	
