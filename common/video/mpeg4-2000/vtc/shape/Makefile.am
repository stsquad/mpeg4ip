AM_CFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -fexceptions @BILLS_CWARNINGS@ -D_OBSS_

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I../include -I../main -I../pezw -I../wavelet -I../zte -I../ztq -I../../type -I../shape
noinst_LTLIBRARIES = libobj_vtc_shape.la

libobj_vtc_shape_la_SOURCES = \
	BinArCodec.cpp \
BinArCodec.hpp \
ShapeBaseCommon.cpp \
ShapeBaseCommon.hpp \
ShapeBaseDecode.cpp \
ShapeBaseEncode.cpp \
ShapeDecoding.cpp \
shape_def.hpp \
ShapeEncoding.cpp \
ShapeEnhCommon.cpp \
ShapeEnhDecode.cpp \
ShapeEnhDef.hpp \
ShapeEnhEncode.cpp \
ShapeUtil.cpp
