SUBDIRS = common image encoder decoder

lib_LTLIBRARIES = libxvid.la

libxvid_la_SOURCES = \
	decore.h \
	encore2.h \
	opendivx.c \
	portab.h \
	xvid.h \
	xvid.c

libxvid_la_LIBADD = \
	common/libxvidcommon.la \
	image/libxvidimage.la \
	encoder/libxvidencoder.la \
	decoder/libxviddecoder.la 

