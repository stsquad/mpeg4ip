lib_LTLIBRARIES = libmp4v2.la

INCLUDES = -I$(top_srcdir)/include

libmp4v2_la_SOURCES = \
	mp4.h \
	mp4.cpp \
	mp4array.h \
	mp4atom.h \
	mp4atom.cpp \
	mp4common.h \
	mp4descriptor.h \
	mp4descriptor.cpp \
	mp4file.h \
	mp4file.cpp \
	mp4property.h \
	mp4property.cpp \
	mp4track.h \
	mp4track.cpp \
	mp4util.h \
	mp4util.cpp \
	atoms.h \
	atom_co64.cpp \
	atom_cprt.cpp \
	atom_ctts.cpp \
	atom_dimm.cpp \
	atom_dinf.cpp \
	atom_dmax.cpp \
	atom_dmed.cpp \
	atom_dref.cpp \
	atom_drep.cpp \
	atom_edts.cpp \
	atom_elst.cpp \
	atom_esds.cpp \
	atom_free.cpp \
	atom_ftyp.cpp \
	atom_hdlr.cpp \
	atom_hinf.cpp \
	atom_hint.cpp \
	atom_hmhd.cpp \
	atom_hnti.cpp \
	atom_iods.cpp \
	atom_maxr.cpp \
	atom_mdat.cpp \
	atom_mdhd.cpp \
	atom_mdia.cpp \
	atom_minf.cpp \
	atom_moov.cpp \
	atom_mp4a.cpp \
	atom_mp4s.cpp \
	atom_mp4v.cpp \
	atom_mvhd.cpp \
	atom_nmhd.cpp \
	atom_nump.cpp \
	atom_payt.cpp \
	atom_pmax.cpp \
	atom_root.cpp \
	atom_rtp.cpp \
	atom_sdp.cpp \
	atom_smhd.cpp \
	atom_stbl.cpp \
	atom_stco.cpp \
	atom_stdp.cpp \
	atom_stsc.cpp \
	atom_stsd.cpp \
	atom_stsh.cpp \
	atom_stss.cpp \
	atom_stsz.cpp \
	atom_stts.cpp \
	atom_tkhd.cpp \
	atom_tmax.cpp \
	atom_tmin.cpp \
	atom_tpyl.cpp \
	atom_trak.cpp \
	atom_tref.cpp \
	atom_trpy.cpp \
	atom_udta.cpp \
	atom_url.cpp \
	atom_urn.cpp \
	atom_vmhd.cpp \
	descriptors.h \
	descriptors.cpp

AM_CXXFLAGS = -Wall

if PROVIDE_GETOPTLONG
LDADD_E1 = $(top_builddir)/lib/gnu/getopt/libgetopt_long.la
else
LDADD_E1 = 
endif

bin_PROGRAMS = mp4broadcaster mp4dump mp4extract

mp4broadcaster_SOURCES = mp4broadcaster.cpp
mp4broadcaster_LDADD = libmp4v2.la $(LDADD_E1)

mp4dump_SOURCES = mp4dump.cpp
mp4dump_LDADD = libmp4v2.la $(LDADD_E1)

mp4extract_SOURCES = mp4extract.cpp
mp4extract_LDADD = libmp4v2.la $(LDADD_E1)

check_PROGRAMS = apitest nullcreate nullvplayer

apitest_SOURCES = apitest.c
apitest_LDADD = libmp4v2.la

nullcreate_SOURCES = nullcreate.cpp
nullcreate_LDADD = libmp4v2.la

nullvplayer_SOURCES = nullvplayer.cpp
nullvplayer_LDADD = libmp4v2.la

