bin_PROGRAMS = faac

faac_SOURCES = \
	aac_se_enc.c \
	bitstream.c \
	enc_tf.c \
	encoder.c \
	mc_enc.c \
	ms.c \
	psych.c \
	pulse.c \
	tns.c \
	transfo.c \
	fastfft.c \
	nok_ltp_enc.c \
	nok_pitch.c \
	rateconv.c \
	faac.c \
	quant.c \
	huffman.c

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/server/lib/sndfile/src

LDADD = $(top_builddir)/server/lib/sndfile/src/libsndfile.la -lm
