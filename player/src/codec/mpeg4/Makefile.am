libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mpeg4_iso_plugin.la
mpeg4_iso_plugin_la_LDFLAGS = -module
mpeg4_iso_plugin_la_SOURCES = \
	mpeg4.cpp \
	mpeg4.h \
	mpeg4_file.cpp \
	mpeg4_file.h

mpeg4_dir=common/video/mpeg4-2000

mpeg4_iso_plugin_la_LIBADD = \
	-lm -lstdc++ \
	$(top_builddir)/lib/fposrec/libfposrec.la \
	$(top_builddir)/lib/gnu/libmpeg4ip_gnu.la \
	$(top_builddir)/$(mpeg4_dir)/sys/decoder/libobj_sys_decoder.la \
	$(top_builddir)/$(mpeg4_dir)/type/libobj_type.la \
	$(top_builddir)/$(mpeg4_dir)/sys/libobj_sys.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/main/libobj_vtc_main_enc.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/main/libobj_vtc_main.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/ztq/libobj_vtc_ztq.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/zte/libobj_vtc_zte.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/pezw/libobj_vtc_pezw.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/wavelet/libobj_vtc_wavelet.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/shape/libobj_vtc_shape.la \
	$(top_builddir)/$(mpeg4_dir)/vtc/main/libobj_vtc_main_dec.la \
	$(top_builddir)/$(mpeg4_dir)/tools/entropy/libobj_tools_entropy.la \
	$(top_builddir)/$(mpeg4_dir)/tools/sadct/libobj_tools_sadct.la \
	$(top_builddir)/lib/mp4av/libmp4av.la

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include -I$(top_builddir)/$(mpeg4_dir) -I$(top_builddir)/include -I$(top_builddir)/lib -I$(top_srcdir)/lib/mp4v2

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@ -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_OBSS_


AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@ -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_OBSS_

EXTRA_DIST = mpeg4_iso_plugin60.dsp
