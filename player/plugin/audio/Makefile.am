if HAVE_A52DEC_LIB
ed1=a52dec
else
ed1=
endif

if HAVE_FFMPEG
ed2=ffmpeg
else
ed2=
endif

SUBDIRS = raw celp g711 $(ed1) $(ed2) 

EXTRA_DIST=faad2/aa_file.cpp faad2/aa_file.h \
	faad2/faad2.cpp faad2/faad2.h \
	faad2/faad2_plugin.dsp \
	faad2/faad2_plugin.dsw \
	faad2/Makefile.am \
	faad2/README_WIN32.txt
