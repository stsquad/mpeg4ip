noinst_LTLIBRARIES = libismacryp.la

INCLUDES=-I$(top_srcdir)/include \
          @AESICM_INC@

libismacryp_la_SOURCES = ismacryplib.c \
	ismacryplib.h \
	ismacryplib_priv.h

@ISMACRYP_FLAGS@

EXTRA_DIST = \
	libismacryp.dsp \
	libismacryp_st.dsp \
	README
