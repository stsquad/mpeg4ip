#

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -D_OBSS_ -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

INCLUDES = -I.. -I../../type -I../../tools -I../../../../../include \
  -I../../sys -I../../sys/encoder -I../../vtc/include \
  -I../../vtc/main -I../../vtc/pezw -I../../vtc/zte -I../../vtc/ztq \
  -I../../vtc/wavelet -I../../vtc/shape

check_PROGRAMS = mp4venc

mp4venc_SOURCES = encoder.cpp

EXTRA_DIST = encoder60.dsp

mp4venc_LDADD= \
  $(top_builddir)/tools/entropy/libobj_tools_entropy.la \
  $(top_builddir)/sys/encoder/libobj_sys_encoder.la \
  $(top_builddir)/tools/entropy/libobj_tools_entropy.la \
  $(top_builddir)/type/libobj_type.la \
  $(top_builddir)/sys/libobj_sys.la \
  $(top_builddir)/vtc/main/libobj_vtc_main_enc.la \
  $(top_builddir)/vtc/main/libobj_vtc_main_dec.la \
  $(top_builddir)/vtc/ztq/libobj_vtc_ztq.la \
  $(top_builddir)/vtc/main/libobj_vtc_main.la \
  $(top_builddir)/vtc/zte/libobj_vtc_zte.la \
  $(top_builddir)/vtc/pezw/libobj_vtc_pezw.la \
 $(top_builddir)/vtc/shape/libobj_vtc_shape.la \
  $(top_builddir)/vtc/wavelet/libobj_vtc_wavelet.la \
  $(top_builddir)/tools/sadct/libobj_tools_sadct.la \
	-lstdc++ -lm 
