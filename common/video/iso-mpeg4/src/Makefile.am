
AM_CXXFLAGS = -D__TRACE_AND_STATS_ -D__DOUBLE_PRECISION_ -D_REENTRANT -DNOCONTROLS -fexceptions -Wall -Werror -D_OBSS_ @BILLS_CPPWARNINGS@

INCLUDES = -I$(srcdir)/../include

lib_LTLIBRARIES = libmpeg4ip_mpeg4_iso.la
check_PROGRAMS = mp4vdec convertpar mp4venc

mp4vdec_SOURCES = \
	app_decoder_decoder.cpp 
mp4vdec_LDADD = \
	-lm \
	libmpeg4ip_mpeg4_iso.la 

convertpar_SOURCES = \
	app_convertpar_convertpar.cpp 

mp4venc_SOURCES = \
	app_encoder_encoder.cpp \
	sys_encoder_blkenc.cpp \
	sys_encoder_errenc.cpp \
	sys_encoder_gmc_enc_util.cpp \
	sys_encoder_gme_for_iso.cpp \
	sys_encoder_mbenc.cpp \
	sys_encoder_mbinterlace.cpp \
	sys_encoder_mcenc.cpp \
	sys_encoder_motest.cpp \
	sys_encoder_mvenc.cpp \
	sys_encoder_newpenc.cpp \
	sys_encoder_padenc.cpp \
	sys_encoder_paramset.cpp \
	sys_encoder_sesenc.cpp \
	sys_encoder_shpenc.cpp \
	sys_encoder_sptenc.cpp \
	sys_encoder_tm5rc.cpp \
	sys_encoder_vopmbenc.cpp \
	sys_encoder_vopseenc.cpp 

mp4venc_LDADD = libmpeg4ip_mpeg4_iso.la -lstdc++ -lm

libmpeg4ip_mpeg4_iso_la_SOURCES = \
	idct_idct.cpp \
	sys_block.cpp \
	sys_cae.cpp \
	sys_dct.cpp\
	sys_decoder_blkdec.cpp \
	sys_decoder_errdec.cpp \
	sys_decoder_mbdec.cpp \
	sys_decoder_mbheaddec.cpp \
	sys_decoder_mbinterlacedec.cpp \
	sys_decoder_mcdec.cpp \
	sys_decoder_mvdec.cpp \
	sys_decoder_newpdec.cpp \
	sys_decoder_rvlcdec.cpp \
	sys_decoder_shpdec.cpp \
	sys_decoder_sptdec.cpp \
	sys_decoder_vopmbdec.cpp \
	sys_decoder_vopsedec.cpp \
	sys_error.cpp \
	sys_gmc_motion.cpp \
	sys_gmc_util.cpp \
	sys_mb.cpp \
	sys_mc.cpp \
	sys_mcpad.cpp \
	sys_mode.cpp \
	sys_mv.cpp \
	sys_newpred.cpp \
	sys_rrv.cpp \
	sys_shape.cpp \
	sys_spt.cpp \
	sys_tps_bfshape.cpp \
	sys_tps_enhcbuf.cpp \
	sys_vopses.cpp \
	tools_entropy_bitstrm.cpp \
	tools_entropy_huffman.cpp \
	tools_sadct_sadct.cpp \
	type_basic.cpp \
	type_geom.cpp \
	type_grayc.cpp \
	type_grayf.cpp \
	type_grayi.cpp \
	type_svd.cpp \
	type_typeapi.cpp \
	type_vop.cpp \
	type_warp.cpp \
	type_yuvac.cpp \
	type_yuvai.cpp \
	vtc_main_computePSNR.cpp \
	vtc_main_read_image.cpp \
	vtc_main_seg.cpp \
	vtc_main_vtcdec.cpp \
	vtc_main_vtcenc.cpp \
	vtc_main_wavelet.cpp \
	vtc_main_write_image.cpp \
	vtc_pezw_PEZW_ac.cpp \
	vtc_pezw_PEZW_globals.cpp \
	vtc_pezw_PEZW_textureLayerBQ.cpp \
	vtc_pezw_PEZW_utils.cpp \
	vtc_pezw_wvtPEZW.cpp \
	vtc_pezw_wvtpezw_tree_decode.cpp \
	vtc_pezw_wvtpezw_tree_encode.cpp \
	vtc_pezw_wvtpezw_tree_init_decode.cpp \
	vtc_pezw_wvtpezw_tree_init_encode.cpp \
	vtc_shape_BinArCodec.cpp \
	vtc_shape_ShapeBaseCommon.cpp \
	vtc_shape_ShapeBaseDecode.cpp \
	vtc_shape_ShapeBaseEncode.cpp \
	vtc_shape_ShapeDecoding.cpp \
	vtc_shape_ShapeEncoding.cpp \
	vtc_shape_ShapeEnhCommon.cpp \
	vtc_shape_ShapeEnhDecode.cpp \
	vtc_shape_ShapeEnhEncode.cpp \
	vtc_shape_ShapeUtil.cpp \
	vtc_wavelet_download_filter.cpp \
	vtc_wavelet_dwt.cpp \
	vtc_wavelet_dwt_aux.cpp  \
	vtc_wavelet_dwtmask.cpp \
	vtc_wavelet_idwt.cpp \
	vtc_wavelet_idwt_aux.cpp \
	vtc_wavelet_idwtmask.cpp \
	vtc_wavelet_imagebox.cpp \
	vtc_zte_ac.cpp \
	vtc_zte_bitpack.cpp \
	vtc_zte_ztscan_dec.cpp \
	vtc_zte_ztscan_enc.cpp \
	vtc_zte_ztscanUtil.cpp \
	vtc_ztq_decQM.cpp \
	vtc_ztq_encQM.cpp \
	vtc_ztq_errorHandler.cpp \
	vtc_ztq_msg.cpp \
	vtc_ztq_QMInit.cpp \
	vtc_ztq_QMUtils.cpp \
	vtc_ztq_quant.cpp \
	vtc_ztq_Utils.cpp 

EXTRA_DIST=example.par
