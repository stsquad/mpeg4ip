#

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I.. -I../../type -I../../tools -I../../../../../include \
  -I../../sys -I../../sys/encoder -I../../vtc/include \
  -I../../vtc/main -I../../vtc/pezw -I../../vtc/zte -I../../vtc/ztq \
  -I../../vtc/wavelet

check_PROGRAMS = mp4vdec

mp4vdec_SOURCES = decoder.cpp

mp4vdec_LDADD= \
	-lm \
	$(top_builddir)/lib/fposrec/libfposrec.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/common/video/mpeg4/sys/decoder/libobj_sys_decoder.la \
	$(top_builddir)/common/video/mpeg4/type/libobj_type.la \
	$(top_builddir)/common/video/mpeg4/sys/libobj_sys.la \
	$(top_builddir)/common/video/mpeg4/vtc/main/libobj_vtc_main_enc.la \
	$(top_builddir)/common/video/mpeg4/vtc/main/libobj_vtc_main.la \
	$(top_builddir)/common/video/mpeg4/vtc/ztq/libobj_vtc_ztq.la \
	$(top_builddir)/common/video/mpeg4/vtc/zte/libobj_vtc_zte.la \
	$(top_builddir)/common/video/mpeg4/vtc/pezw/libobj_vtc_pezw.la \
	$(top_builddir)/common/video/mpeg4/vtc/wavelet/libobj_vtc_wavelet.la \
	$(top_builddir)/common/video/mpeg4/vtc/main/libobj_vtc_main_dec.la \
	$(top_builddir)/common/video/mpeg4/tools/entropy/libobj_tools_entropy.la 
