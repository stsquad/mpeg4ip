libdir = @PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = mpeg4_iso_plugin.la
mpeg4_iso_plugin_la_LDFLAGS = -module
mpeg4_iso_plugin_la_SOURCES = \
	mpeg4.cpp \
	mpeg4.h \
	mpeg4_file.cpp \
	mpeg4_file.h

mpeg4_iso_plugin_la_LIBADD = \
	-lm -lstdc++ \
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
	$(top_builddir)/common/video/mpeg4/tools/entropy/libobj_tools_entropy.la \
	$(top_builddir)/lib/mp4av/libmp4av.la

INCLUDES = -I.. -I../.. -I../../../lib -I$(top_srcdir)/lib/SDL/include -I../../../../common/video/mpeg4 -I../../../../include -I../../../../lib -I$(top_srcdir)/lib/mp4v2

AM_CFLAGS = -D_REENTRANT -fexceptions @BILLS_CWARNINGS@

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions @BILLS_CPPWARNINGS@

EXTRA_DIST = mpeg4_iso_plugin60.dsp
