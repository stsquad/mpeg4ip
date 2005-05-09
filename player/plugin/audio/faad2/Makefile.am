libdir = @MPEG4IP_PLAYER_PLUGIN_DIR@

lib_LTLIBRARIES = aac_plugin.la
aac_plugin_la_LDFLAGS = -module
aac_plugin_la_SOURCES = \
	faad2.cpp \
	faad2.h \
	aa_file.cpp \
	aa_file.h

aac_plugin_la_LIBADD = \
	$(top_builddir)/libfaad/libfaad.la \
	-lm -lmp4av -lmp4v2


INCLUDES =  -I$(top_srcdir)/include

AM_CFLAGS = -D_REENTRANT -fexceptions 

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions 


