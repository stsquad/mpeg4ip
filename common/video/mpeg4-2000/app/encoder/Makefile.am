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
	$(top_builddir)/lib/fposrec/libfposrec.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
  $(top_builddir)/common/video/mpeg4-2000/tools/entropy/libobj_tools_entropy.la \
  $(top_builddir)/common/video/mpeg4-2000/sys/encoder/libobj_sys_encoder.la \
  $(top_builddir)/common/video/mpeg4-2000/tools/entropy/libobj_tools_entropy.la \
  $(top_builddir)/common/video/mpeg4-2000/type/libobj_type.la \
  $(top_builddir)/common/video/mpeg4-2000/sys/libobj_sys.la \
  $(top_builddir)/common/video/mpeg4-2000/vtc/main/libobj_vtc_main_enc.la \
  $(top_builddir)/common/video/mpeg4-2000/vtc/main/libobj_vtc_main_dec.la \
  $(top_builddir)/common/video/mpeg4-2000/vtc/ztq/libobj_vtc_ztq.la \
  $(top_builddir)/common/video/mpeg4-2000/vtc/main/libobj_vtc_main.la \
  $(top_builddir)/common/video/mpeg4-2000/vtc/zte/libobj_vtc_zte.la \
  $(top_builddir)/common/video/mpeg4-2000/vtc/pezw/libobj_vtc_pezw.la \
 $(top_builddir)/common/video/mpeg4-2000/vtc/shape/libobj_vtc_shape.la \
  $(top_builddir)/common/video/mpeg4-2000/vtc/wavelet/libobj_vtc_wavelet.la \
  $(top_builddir)/common/video/mpeg4-2000/tools/sadct/libobj_tools_sadct.la \
	-lstdc++ -lm 
