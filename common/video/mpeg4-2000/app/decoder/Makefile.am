#

AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D_OBSS_

INCLUDES = -I.. -I../../type -I../../tools -I../../../../../include \
  -I../../sys -I../../sys/encoder -I../../vtc/include \
  -I../../vtc/main -I../../vtc/pezw -I../../vtc/zte -I../../vtc/ztq \
  -I../../vtc/wavelet -I../../vtc/shape -I../../idct

check_PROGRAMS = mp4vdec

mp4vdec_SOURCES = decoder.cpp

mp4vdec_LDADD= \
	-lm \
	$(top_builddir)/sys/decoder/libobj_sys_decoder.la \
	$(top_builddir)/type/libobj_type.la \
	$(top_builddir)/sys/libobj_sys.la \
	$(top_builddir)/vtc/main/libobj_vtc_main_dec.la \
	$(top_builddir)/vtc/ztq/libobj_vtc_ztq.la \
	$(top_builddir)/vtc/main/libobj_vtc_main.la \
	$(top_builddir)/vtc/zte/libobj_vtc_zte.la \
	$(top_builddir)/vtc/pezw/libobj_vtc_pezw.la \
	$(top_builddir)/vtc/shape/libobj_vtc_shape.la \
	$(top_builddir)/vtc/wavelet/libobj_vtc_wavelet.la \
	$(top_builddir)/tools/sadct/libobj_tools_sadct.la \
	$(top_builddir)/idct/libobj_idct.la \
	$(top_builddir)/tools/entropy/libobj_tools_entropy.la
