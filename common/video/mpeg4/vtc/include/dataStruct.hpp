/*************************************************************************

This software module was originally developed by 


in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). 
This software module is an implementation of a part of one or more MPEG-4 Video tools 
as specified by the MPEG-4 Video. 
ISO/IEC gives users of the MPEG-4 Video free license to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the MPEG-4 Video. 
Those intending to use this software module in hardware or software products are advised that its use may infringe existing patents. 
The original developer of this software module and his/her company, 
the subsequent editors and their companies, 
and ISO/IEC have no liability for use of this software module or modifications thereof in an implementation. 
Copyright is not released for non MPEG-4 Video conforming products. 
Microsoft retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997, 1998.

Module Name:

	vtcEnc.hpp

Abstract:

	Encoder for one still image using wavelet VTC.

Revision History:

*************************************************************************/

#ifndef __VTCENC_HPP_ 
#define __VTCENC_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "basic.hpp"
#include "quant.hpp"
#include "ac.hpp"
//#include "context.hpp"
#include "dwt.h"

/* for bilevel mode */
#include "PEZW_ac.hpp"
#include "wvtPEZW.hpp"
#include "PEZW_zerotree.hpp"
#include "PEZW_mpeg4.hpp"

#ifdef DATA
#undef DATA
#endif
#define DATA        Short


#define MAXLEV      12
#define NCOLOR      3

#define SINGLE_Q    1
#define MULTIPLE_Q  2
#define BILEVEL_Q   3

/* zero tree symbols - if changed please change MapTypeToText variable */
#define IZ       0  /* Isolated Zero */
#define VAL      1  /* Value */
#define ZTR      2  /* Zero-Tree Root */
#define VZTR     3  /* Valued Zero-Tree Root */
#define ZTR_D    4  /* Parent has type of ZTR, VZTR, or ZTR_D (not coded) */
#define VLEAF    5  /* Leaf coefficient with non-zero value (not coded) */
#define ZLEAF    6  /* Leaf coefficient with zero value (not coded) */
#define UNTYPED  7  /* so far only for clearing of ZTR_Ds */

#define MAXDECOMPLEV 10
#define INF_RES       1024


#define MONO 1

#define FULLSIZE 0
#define PROGRESSIVE 0
#define MASK_VAL (0xff)

#define SKIP_NONE        0 /* Not in skip mode */
#define SKIP_TNYC        1 /* Skip mode - Type Not Yet Coded */
#define SKIP_ZTR         2 /* Skip mode - previous type coded was ZTR */
#define SKIP_IZ          3 /* Skip mode - previous type coded was IZ */

/* QValArithModel field types - if changed please change mapArithModelToText
   variable */
#if 0 // hjlee 0901
#define ACM_NONE  0 /* When there's no value to code */
#define ACM_SKIP  1
#define ACM_ROOT  2
#define ACM_VALZ  3
#define ACM_VALNZ 4
#define ACM_RESID 5 /* should have one for each set of coeffs arising in 
		       different initial spatial layers */
#define ACM_DC    6
#endif // hjlee 0901

// hjlee 0901
#define ACM_NONE  0 /* When there's no value to code */
#define ACM_ROOT  1
#define ACM_VALZ  2
#define ACM_VALNZ 3
#define ACM_RESID 4 /* should have one for each set of coeffs arising in 
		       different initial spatial layers */
#define ACM_DC    5

typedef short SInt;
typedef U8  UChar;

typedef struct {
  /* not updated */
	SInt  wvt_coeff;    /*  Original value in encoding.*/
	SInt  rec_coeff;	/*  Reconstructed value in decoding 
							Put here for comparing with originals
							at decoder. Reconstructed values can
							be put in original when memory is an
							issue and we don't want stats to be
							computed at decoder. */

	/* updated by quantization */
  SInt           quantized_value; /* quantized value                    */
  quantState     qState;          /* state of quantizer for coefficient */

  /* updated by marking */
	UChar  state;          /* state of coefficient                   */
	UChar  type;           /* MZTE tree types: ZTR, IZ, VZTR, or VAL */
	UChar  skip;  /* Skip coding of coefficient value (not type)     */

  /* updated by Shipeng */
	UChar  mask;

} COEFFINFO;

typedef struct {
  Int   num_ZTR;
  Int   num_VZTR;
  Int   num_VAL;
} STATINFO;

typedef struct  {
  Int    height;
  Int    width;
  UChar  *mask;
  Void   *data; 
} PICTURE;

typedef struct {
   SInt         quant;
   UChar        allzero;
   Int          root_max;   /* three maximum values for AC magnitude coding */
   Int          valz_max;
   Int          valnz_max;
   Int          residual_max;

// hjlee 0901
   Int          wvtDecompNumBitPlanes[MAXDECOMPLEV];
   Int          wvtDecompResNumBitPlanes;
   Int          wvtDecompMax[MAXDECOMPLEV]; /* for _NEW_CONTEXT_ */

   STATINFO     stat;
} SNR_IMAGE;

typedef struct {
   SNR_IMAGE  snr_image; 
} SNR_LAYER;

typedef struct {
   SInt         height;
   SInt         width;
   SInt         SNR_scalability_levels;
   COEFFINFO    **coeffinfo;
   SNR_LAYER    SNRlayer;
} SPATIAL_LAYER;


typedef struct snr_param {
   Int SNR_scalability_levels;
   Int *Quant;
} SNR_PARAM;

typedef struct wvt_codec 
{

  Int		m_iBitDepth; /* number bits per pixel (spatial) */
  Int		m_iColors;    /* number of color components: 0 = mono, 3=yuv */
  Int		m_iColorFormat;  /* 4:4:4, 4:2:2, or 4:2:0 ???? */
  PICTURE   *m_Image; /* spatial source */
  PICTURE   *m_SegImage; /* spatial source */
  
  Int       m_iWvtType;      /* Type of filter */
  Int       m_iWvtDownload;
  Int       m_iWvtDecmpLev; 
  Int       m_iWvtUniform;  // hjlee 0901
  Int       *m_WvtFilters; /* Wavetfilter numbers: 0-9 */ // hjlee 0901


  Int       m_iMean[NCOLOR]; /* mean of wvt coeffs in DC band ???? */
  Int       m_iQDC[NCOLOR];  
  Int       m_iOffsetDC;
  Int       m_iMaxDC; /* max quantized DC coeff - pre-shifting */
  Int       m_iDCWidth;
  Int		m_iDCHeight;
  
  // hjlee 0901
  Int       m_lastWvtDecompInSpaLayer[MAXDECOMPLEV][NCOLOR];
  Int       m_spaLayerWidth[MAXDECOMPLEV][NCOLOR];
  Int       m_spaLayerHeight[MAXDECOMPLEV][NCOLOR];
  UChar     m_defaultSpatialScale;


  Int       m_iWidth;
  Int		m_iHeight; 
  Int       m_iSpatialLev;
  Int		m_iQuantType;
  Int		m_iScanDirection;
  Int	    m_iScanOrder;
  Bool		m_bStartCodeEnable;

  SPATIAL_LAYER   m_SPlayer[NCOLOR];
  SNR_PARAM       *m_Qinfo[NCOLOR];
  
  Int m_iTargetSpatialLev;
  Int m_iTargetSNRLev;
  Int m_iDeringWinSize;
  Int m_iDeringThreshold;
  Int m_iTargetBitrate;     /* PEZW */
 
	/* for shape coding */
	Int m_iAlphaChannel;
	Int m_iAlphaTh;
	Int m_iChangeCRDisable;
	Int m_iSTOConstAlpha;
	Int m_iSTOConstAlphaValue;

	Int m_iSingleBitFile;
	Char *m_cBitFile;
	Char *m_cBitFileAC;

	Int  m_iOriginX;
	Int  m_iOriginY;
	Int  m_iRealWidth;
	Int  m_iRealHeight;

	Int  m_iCurSpatialLev;
	Int  m_iCurSNRLev;
	Int  m_iCurColor;

	/* for arithmetic coder */
	Int m_iAcmOrder;       /* 0 - zoro order, 1 - mix order */
	Int m_iAcmMaxFreqChg;  /* 0 - default, 1 - used defined */
	Int *m_iAcmMaxFreq;    /* array of user defined maximum freqs */


} WVT_CODEC;

Class CVTCCommon
{
public:
	WVT_CODEC mzte_codec;

	// Utils.cpp
	Void setSpatialLevelAndDimensions(Int spLayer, Int c);
	Void updateResidMaxAndAssignSkips(Int c);
    Int  xy2wvtDecompLev(Int x, Int y);

	// vtcdec.cpp
	Void setSpatialLayerDimsSQ(Int band);  // hjlee 0901
	Void getSpatialLayerDims(); //hjlee 0901
	Int ceilLog2(Int x); // hjlee 0901

	// QMInit.cpp
	Int ztqQListInit();
	Void ztqQListExit();
	Int ztqInitDC(Int decode, Int c);
	Int ztqInitAC(Int decode, Int c);

	// quant.cpp
	Void initQuantSingleStage(quantState *state, 
			Int *statePrevQ, Int initialVal);
	Void initInvQuantSingleStage(quantState *state, 
			Int *statePrevQ);
	Int quantRefLev(Int curQ, Int *lastQUsed, Int whichQ);
	Int invQuantSingleStage(Int QIndex, Int Q, 
			quantState *state, Int *statePrevQ,Int updatePrevQ);
	
	// QMUtils.cpp
	Int findChild(Int x, Int y, Int xc[], Int yc[], Int c);
	Int isIndexInRootBands(Int x, Int y, Int c);
	Void spatialLayerChangeUpdate(Int c);
	Int coordToSpatialLev(Int x, Int y, Int c);
	Void updateCoeffAndDescState(Int x, Int y, Int c);
	Void markCoeff(Int x, Int y, UChar valuedDes, Int c);
	Void updateState(Int x, Int y, Int type, Int c);


	// msg.cpp
	Void errorHandler(Char *s, ...);
	Void noteStat(Char *s, ...);
	Void noteDebug(Char *s, ...);
	Void noteDetail(Char *s, ...);
	Void noteProgress(Char *s, ...);
	Void noteProgressNoNL(Char *s, ...);
	Void noteWarning(Char *s, ...);
	Void noteError(Char *s, ...);
	Void noteErrorNoPre(Char *s, ...);

	//download_filter.cpp
	Void check_marker(Int marker_bit);
	Void check_symmetry(FILTER *filter);
	Void upload_wavelet_filters(FILTER *filter);
	Void download_wavelet_filters(FILTER **filter, Int type); // hjlee 0901
	// wavelet.cpp
	Void choose_wavelet_filter(FILTER **anafilter,FILTER **synfilter,
			   Int type);

	// bitpack.cpp
	Void init_bit_packing_fp(FILE *fp, Int clearByte);
	Void emit_bits(UShort code, Int size);
	Int get_X_bits(Int nbits);
	Void flush_bytes1();
	Int nextinputbit();


	// ztscanUtil.cpp
	Void clear_ZTR_D(COEFFINFO **coeffinfo, Int width, 
					Int height);
	Void probModelInitSQ(Int col);  // hjlee 0901
	Void probModelFreeSQ(Int col); // hjlee 0901
	Void setProbModelsSQ(Int col); // hjlee 0901
	Void probModelInitMQ(Int col); // hjlee 0901
	Void probModelFreeMQ(Int col); // hjlee 0901
	Void setProbModelsMQ(Int col); // hjlee 0901
	Void init_acm_maxf_enc(); // hjlee 0901
	Void init_acm_maxf_dec(); // hjlee 0901

	//ac.cpp
	Void mzte_update_model(ac_model *acm,Int sym); // hjlee 0901
	Void mzte_ac_model_init(ac_model *acm,Int nsym,
							UShort *ifreq,Int adapt,Int inc);
	Void mzte_ac_model_done(ac_model *acm);

	/* other */
	char *check_startcode (unsigned char *stream, long len);
	void one_bit_to_buffer (char bit, char *outbuffer);
	void undo_startcode_check (unsigned char *data, long len);

	/* for bilevel mode: added by Jie Liang */
    /* from PEZW_utils.c */
    PEZW_SPATIAL_LAYER *Init_PEZWdata (int color, int levels, int w, int h);
    void restore_PEZWdata (PEZW_SPATIAL_LAYER **SPlayer);
    int lshift_by_NBit (unsigned char *data, int len, int N);
	//computePSNR.cpp, added by U. Benzler 981117 
    Void ComputePSNR(UChar *orgY, UChar *recY, 
                UChar *maskY,
                UChar *orgU, UChar *recU, 
                UChar *maskU,
                UChar *orgV, UChar *recV, 
                UChar *maskV,
                Int width, Int height, Int stat);
};

Class CVTCEncoder : public CVTCCommon, 
					public VTCIMAGEBOX,
					public VTCDWT,
					public VTCDWTMASK // hjlee 0901
{
public:
	// Constructor and Deconstructor
	~CVTCEncoder ();
	CVTCEncoder ();

	// input/output 
	Char *m_cImagePath;
	Char *m_cOutBitsFile;
	Char *m_cSegImagePath;

	Void init(		
		Char* cImagePath,
	    UInt uiAlphaChannel,
		Char* cSegImagePath,
		UInt uiAlphaTh,
		UInt uiChangeCRDisable,
		Char* cOutBitsFile,
		UInt uiColors,
		UInt uiFrmWidth,
		UInt uiFrmHeight,
		UInt uiWvtType,
		UInt uiWvtDownload,  // hjlee 0901
		UInt uiWvtDecmpLev,
		UInt uiWvtUniform, // hjlee 0901
		Int* iWvtFilters,  // hjlee 0901
		UInt uiQuantType,
		UInt uiScanDirection,
		Bool bStartCodeEnable,
		UInt uiTargetSpatialLev,
		UInt uiTargetSNRLev,
		UInt uiQdcY,
		UInt uiQdcUV,
		UInt uiSpatialLev ,
		UInt defaultSpatialScale, // hjlee 0901
		Int  lastWvtDecompInSpaLayer[MAXDECOMPLEV], // hjlee 0901
		SNR_PARAM* Qinfo[]);


	// attribute
	Int GetcurSpatialLev() { 
		return mzte_codec.m_iCurSpatialLev; }
	Int GetDCHeight() { 
		return mzte_codec.m_iHeight>>mzte_codec.m_iWvtDecmpLev; }
	Int GetDCWidth() { 
		return mzte_codec.m_iWidth>>mzte_codec.m_iWvtDecmpLev; }

	// operation
	Void encode();


protected:

	// vtcenc.cpp
	Void flush_buffer_file();
	Void close_buffer_file(FILE *fp);
	Void header_Enc(FILTER **wvtfilter); // hjlee 0901
	Void Put_Quant_and_Max(SNR_IMAGE *snr_image, Int spaLayer, Int color); // hjlee0901
	Void Put_Quant_and_Max_SQBB(SNR_IMAGE *snr_image, Int spaLayer,Int color); // hjlee0901

	Void textureLayerDC_Enc();
	Void TextureSpatialLayerSQNSC_enc(Int spa_lev);
	Void TextureSpatialLayerSQ_enc(Int spa_lev, FILE *bitfile);
	Void textureLayerSQ_Enc(FILE *bitfile);
	Void TextureObjectLayer_enc(FILTER **wvtfilter); // hjlee 0901
	Void textureLayerMQ_Enc(FILE *bitfile);
	Void TextureSNRLayerMQ_encode(Int spa_lev, Int snr_lev, FILE *fp);


	// read_image.cpp
	Void read_image(Char *img_path, 
					Int img_width, 
					Int img_height, 
					Int img_colors, 
					Int img_bit_depth,
					PICTURE *img);	

	Int read_segimage(Char *seg_path, Int seg_width, Int seg_height, 
		  Int img_colors,
		  PICTURE *MyImage);

	Void get_virtual_image(PICTURE *MyImage, Int wvtDecompLev, 
		       Int usemask, Int colors, Int alphaTH, 
		       Int change_CR_disable, FILTER *Filter);


	// wavelet.cpp
	Void perform_DWT(FILTER **wvtfilter); // hjlee 0901

	// ac.cpp
	Void mzte_output_bit(ac_encoder *ace,Int bit);
	Void mzte_bit_plus_follow(ac_encoder *ace,Int bit);
	Void mzte_ac_encoder_init(ac_encoder *ace);
	Int  mzte_ac_encoder_done(ac_encoder *ace);
	Int  mzte_ac_encode_symbol(ac_encoder *ace, ac_model *acm, Int sym); // hjlee 0901

	// ztscan_enc.cpp
	SInt DC_pred_pix(Int i, Int j);
	Void DC_predict(Int color);
	Void wavelet_dc_encode(Int c);
	Void cacll_encode();
	Void wavelet_higher_bands_encode_SQ_band(Int col);
	Void cachb_encode_SQ_band(SNR_IMAGE *snr_image);
//	Void encode_pixel_SQ_band(Int h,Int w);
	Void wavelet_higher_bands_encode_SQ_tree();
	Void cachb_encode_SQ_tree();  // hjlee 0928
//	Void encode_pixel_SQ_tree(Int h,Int w);  // hjlee 0928
	Void encode_pixel_SQ(Int h,Int w);  // 1124
	Void mag_sign_encode_SQ(Int h,Int w);
	Void wavelet_higher_bands_encode_MQ(Int scanDirection);
	Void mark_ZTR_D(Int h,Int w);
	Void cachb_encode_MQ_band();
//	Void encode_pixel_MQ_band(Int h,Int w);
	Void cachb_encode_MQ_tree(); // hjlee 0928
//	Void encode_pixel_MQ_tree(Int h,Int w);  // hjlee 0928
	Void encode_pixel_MQ(Int h,Int w);  // 1124
	Void mag_sign_encode_MQ(Int h,Int w);
	Void bitplane_encode(Int val,Int l,Int max_bplane); // hjlee 0901
	Void bitplane_res_encode(Int val,Int l,Int max_bplane); // hjlee 0901
//	Void encodeBlocks(Int y, Int x, Int n);  // 1124
	Void encodeSQBlocks(Int y, Int x, Int n);
	Void encodeMQBlocks(Int y, Int x, Int n);


	// encQM.cpp
	Void quantizeCoeff(Int x, Int y, Int c);
	Int quantizeAndMarkCoeffs(Int x, Int y, Int c);
	Int encQuantizeDC(Int c);
	Int encQuantizeAndMarkAC(Int c);
	Int encUpdateStateAC(Int c);


	// quant.cpp
	Int quantSingleStage(Int Q, quantState *state, 
			Int *statePrevQ,Int updatePrevQ);
	
	// bitpack.cpp
	Int get_total_bit_rate();
	Int get_total_bit_rate_dec();
	Int get_total_bit_rate_junk();
	Void flush_bytes();
	Void flush_bits1 ();
	Void flush_bits ();
	Void flush_bits_zeros ();
	Int put_param(Int value, Int nbits);
	UInt LookBitsFromStream (Int n);
	Void emit_bits_checksc(UInt code, Int size);
	Void emit_bits_checksc_init();
	Void write_to_bitstream(UChar *bitbuffer,Int total_bits);

	/* for bilevel mode: added by Jie Liang */
    /* from PEZW_textureBQ.c */
    void textureLayerBQ_Enc(FILE *bitfile);

    /* from PEZW_utils.c */
    void PEZW_bitpack (PEZW_SPATIAL_LAYER **SPlayer);
    void PEZW_freeEnc (PEZW_SPATIAL_LAYER **SPlayer);
};

Class CVTCDecoder : public CVTCCommon, 
					public VTCIMAGEBOX,
					public VTCIDWT,
					public VTCDWTMASK
{
public:
	// Constructor and Deconstructor
	~CVTCDecoder ();
	CVTCDecoder ();

	Char *m_cInBitsFile;
	Char *m_cRecImageFile;
	
	// vtcdec.cpp
	Void header_Dec(FILTER ***wvtfilter, PICTURE **Image); // hjlee 0901
	Void Get_Quant_and_Max(SNR_IMAGE *snr_image, Int spaLayer, 
					Int color); // hjlee 0901
	Void Get_Quant_and_Max_SQBB(SNR_IMAGE *snr_image, Int spaLayer, 
				   Int color); // hjlee 0901
	Void textureLayerDC_Dec();
	Void TextureSpatialLayerSQNSC_dec(Int spa_lev);
	Void TextureSpatialLayerSQ_dec(Int spa_lev, FILE *bitfile);
	Void textureLayerSQ_Dec(FILE *bitfile);
	Void TextureSNRLayerMQ_decode(Int spa_lev, Int snr_lev,FILE *fp);
// hjlee 0901
	Void textureLayerMQ_Dec(FILE *bitfile, 
			       Int  target_spatial_levels,
			       Int  target_snr_levels,
			       FILTER **wvtfilter); // hjlee 0901
				   
// hjlee 0901
	Void TextureObjectLayer_dec(Int  target_spatial_levels,
				   Int  target_snr_levels, FILTER ***pwvtfilter); // hjlee 0901


	Void decode(Char *InBitsFile, Char *RecImageFile,
				Int TargetSpaLev, Int TargetSNRLev); 

	// seg.cpp
// hjlee 0901
	Void get_virtual_mask(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, FILTER **filters) ;

			  
	// decQM.c
	Void iQuantizeCoeff(Int x, Int y, Int c);
	Void iQuantizeCoeffs(Int x, Int y, Int c);
	Int decIQuantizeDC(Int c);
	Int decIQuantizeAC(Int c);
	Int decIQuantizeAC_spa(Int spa_lev,Int c);
	Int decUpdateStateAC(Int c);
	Int markCoeffs(Int x, Int y, Int c);
	Int decMarkAC(Int c);
	Int decUpdateStateAC_spa(Int c);  // hjlee 0901

	// ac.cpp
	Int mzte_input_bit(ac_decoder *acd);
	Void mzte_ac_decoder_done(ac_decoder *acd);
	Void mzte_ac_decoder_init(ac_decoder *acd);
	Int mzte_ac_decode_symbol(ac_decoder *acd,ac_model *acm); // hjlee 0901

	//ztscan_dec.cpp
	Short  iDC_pred_pix(Int i, Int j);
	Void   iDC_predict(Int color);
	Void wavelet_dc_decode(Int c);
	Void callc_decode();
	Void wavelet_higher_bands_decode_SQ_band(Int col);
	Void cachb_decode_SQ_band(SNR_IMAGE *snr_image);
//	Void decode_pixel_SQ_band(Int h,Int w); // 1124
	Void wavelet_higher_bands_decode_SQ_tree();
	Void cachb_decode_SQ_tree(); // hjlee 0901
//	Void decode_pixel_SQ_tree(Int h0,Int w0); // hjlee 0901
	Void decode_pixel_SQ(Int h,Int w); // 1124
	Void mag_sign_decode_SQ(Int h,Int w);
	Void wavelet_higher_bands_decode_MQ(Int scanDirection);
	Void cachb_decode_MQ_band();
//	Void decode_pixel_MQ_band(Int h,Int w);
	Void decode_pixel_MQ(Int h,Int w);
	Void cachb_decode_MQ_tree();
//	Void decode_pixel_MQ_tree(Int h,Int w);
	Void mark_ZTR_D(Int h,Int w);
	Void mag_sign_decode_MQ(Int h,Int w);

	Int bitplane_decode(Int l,Int max_bplane); // hjlee 0901
	Int bitplane_res_decode(Int l,Int max_bplane);  // hjlee 0901
//	Void decodeBlocks(Int y, Int x, Int n);   //1124
	Void decodeSQBlocks(Int y, Int x, Int n);
	Void decodeMQBlocks(Int y, Int x, Int n);

	// bitpack.cpp
	Int align_byte ();
	Int get_param(Int nbits);
	Void restore_arithmetic_offset(Int bits_to_go);

	// wavelet.cpp
	Void perform_IDWT(FILTER **wvtfilter,
		  Char *recImgFile); // hjlee 0901

	// write_image.cpp 
	Void write_image(Char *recImgFile, Int colors,
		 Int width, Int height,
		 Int real_width, Int real_height,
		 Int rorigin_x, Int rorigin_y,
		 UChar *outimage[3], UChar *outmask[3],
		 Int usemask, Int fullsize, Int MinLevel);

	/* for bilevel mode: added by Jie Liang */
    /* from PEZW_textureBQ.c */
    void textureLayerBQ_Dec(FILE *bitfile);
    void PEZW_decode_ratecontrol (PEZW_SPATIAL_LAYER **SPlayer, int bytes_decoded);

    /* from PEZW_utils.c */
    void PEZW_bit_unpack (PEZW_SPATIAL_LAYER **SPlayer);
    void PEZW_freeDec (PEZW_SPATIAL_LAYER **SPlayer);

	/* bitpack.cpp */
	Int get_allbits (Char *buffer);
	Int Is_startcode (long startcode);
	int align_byte1 ();
	Int get_X_bits_checksc(Int nbits);
	Void get_X_bits_checksc_init();
	int get_allbits_checksc (unsigned char *buffer);
	Int align_byte_checksc ();
    int decoded_bytes_from_bitstream ();
};

#endif __VTCENC_HPP_
