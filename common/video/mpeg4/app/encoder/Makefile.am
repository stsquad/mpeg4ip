#

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror

INCLUDES = -I.. -I../../type -I../../tools -I../../../../../include \
  -I../../sys -I../../sys/encoder -I../../vtc/include \
  -I../../vtc/main -I../../vtc/pezw -I../../vtc/zte -I../../vtc/ztq \
  -I../../vtc/wavelet

bin_PROGRAMS = mp4venc

mp4venc_SOURCES = encoder.cpp

LDADD= \
  ../../sys/encoder/libobj_sys_encoder.la \
  ../../type/libobj_type.la \
  ../../sys/libobj_sys.la \
  ../../vtc/main/libobj_vtc_main_dec.la \
  ../../vtc/main/libobj_vtc_main_enc.la \
  ../../vtc/main/libobj_vtc_main.la \
  ../../vtc/ztq/libobj_vtc_ztq.la \
  ../../vtc/zte/libobj_vtc_zte.la \
  ../../vtc/pezw/libobj_vtc_pezw.la \
  ../../vtc/wavelet/libobj_vtc_wavelet.la \
  ../../tools/entropy/libobj_tools_entropy.la \
  -lm
