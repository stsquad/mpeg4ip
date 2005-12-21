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
/* for shape coding added by SL@Sarnoff (03/03/99)*/
#include "ShapeBaseCommon.hpp"
#include "BinArCodec.hpp"

#ifdef DATA
#undef DATA
#endif
#define DATA        Int


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

// FPDAM begin: added by Sharp
#define OPAQUE_TILE     1
#define BOUNDA_TILE     2
#define TRANSP_TILE     3
// FPDAM end: added by Sharp

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
	PICTURE   *m_ImageOrg; /* storage for original source */ // added by Sharp (99/2/16)

  
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

	Int m_iTextureTileType; // FPDAM added by Sharp


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
  
  //Int m_iDeringWinSize; //deleted by SL@Sarnoff (03/02/99)
  //Int m_iDeringThreshold;
  Int m_iTargetShapeLev;  //target shape spatial level (added by SL@Sarnoff -- 03/02/99)
  Int m_iFullSizeOut; //full size output of image (added by SL@Sarnoff -- 03/02/99)
	
  Int m_iTargetBitrate;     /* PEZW */
 
	/* for shape coding */
	Int m_iAlphaChannel;
	Int m_iAlphaTh;
	Int m_iChangeCRDisable;
	Int m_iSTOConstAlpha;
	Int m_iSTOConstAlphaValue;
	Int m_iShapeScalable; // shape scalable? (added by SL@Sarnoff -- 03/02/99)
	Int m_iSingleBitFile;
	Char *m_cBitFile;
	Char *m_cBitFileAC;

	Int  m_iOriginX;
	Int  m_iOriginY;
	Int  m_iRealWidth;
	Int  m_iRealHeight;

// FPDAM begin: added by Sharp
	Int m_iObjectOriginX;
	Int m_iObjectOriginY;
	Int m_iObjectWidth;
	Int m_iObjectHeight;
// FPDAM end: added by Sharp

// begin: added by Sharp (99/5/10)
	Int  m_iPictWidth;
	Int  m_iPictHeight;
// end: added by Sharp (99/5/10)
 

	Int  m_iCurSpatialLev;
	Int  m_iCurSNRLev;
	Int  m_iCurColor;

	//Added by Sarnoff for error resilience, 3/5/99
	UShort m_usSegmentThresh;
	UShort m_usPacketThresh;  //bbc, 2/19/99
	UShort m_usErrResiDisable;
	//End Added by Sarnoff for error resilience, 3/5/99

	/* for arithmetic coder */
	Int m_iAcmOrder;       /* 0 - zoro order, 1 - mix order */
	Int m_iAcmMaxFreqChg;  /* 0 - default, 1 - used defined */
	Int *m_iAcmMaxFreq;    /* array of user defined maximum freqs */

// begin: added by Sharp (99/2/16)
  Int m_display_width, m_display_height;
  Int m_tiling_disable;
  Int m_tile_width;
  Int m_tile_height;
	Int m_tiling_jump_table_enable;
  Int m_extension_type;
  Int m_target_tile_id_from;
  Int m_target_tile_id_to;
  Int m_iNumOfTile;
// end: added by Sharp (99/2/16)

	Int m_visual_object_verid; // added by Sharp (99/4/7)

} WVT_CODEC;

class CVTCCommon
{
public:
	WVT_CODEC mzte_codec;
	//begin: added by SL 030399
	Int ObjectWidth, ObjectHeight; 
	Int STO_const_alpha;
	UChar STO_const_alpha_value;
	//end: added by SL 030399
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
	Int download_wavelet_filters(FILTER **filter, Int type); // hjlee 0901 , Modified by Sharp (99/2/16)
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

	// computePSNR.cpp
	Void ComputePSNR(UChar *orgY, UChar *recY,
     UChar *maskY,
     UChar *orgU, UChar *recU,
     UChar *maskU,
     UChar *orgV, UChar *recV,
     UChar *maskV,
     Int width, Int height, Int stat);

	/* other */
	char *check_startcode (unsigned char *stream, long len);
	void one_bit_to_buffer (char bit, char *outbuffer);
	void undo_startcode_check (unsigned char *data, long len);

	/* for bilevel mode: added by Jie Liang */
    /* from PEZW_utils.c */
    PEZW_SPATIAL_LAYER *Init_PEZWdata (int color, int levels, int w, int h);
    void restore_PEZWdata (PEZW_SPATIAL_LAYER **SPlayer);
    int lshift_by_NBit (unsigned char *data, int len, int N);
	//begin added by SL@Sarnoff (03/03/99)
	//ShapeBaseCommon.cpp
	Int	GetContext ( Char a, Char b, Char c, Char d, Char e, Char f, Char g, Char h );
	Char GetShapeVL ( Char a, Char b, Char c, Char d, Char e, Char f, 
					   Char g, Char h, Char i, Char j, Char k, Char l, Int t );
	Void  	UpSampling_Still (	Int x,
			Int y,
			Int blkn,
			Int cr,
			Int blkx,
			UChar **buff,
			UChar **data,
			UChar **shape);
	Void  	AdaptiveUpSampling_Still (	UChar **BABdown,
			UChar **BABresult,
			Int sub_size	);
	Void  	DownSampling_Still (	UChar **buff,
			UChar **data,
			Int b_size,
			Int s_size	);
	Void  	AddBorderToBAB (	Int blkx,
			Int blky,
			Int blksize,
			Int cr_size,
			Int MAX_blkx,
			UChar **BABinput,
			UChar **BABresult,
			UChar **shape,
			Int flag	);
	//ShapeEnhCommon.hpp
	Int  	GetContextEnhBAB_XOR (	UChar *curr_bab_data,
			Int x2,
			Int y2,
			Int width2,
			Int pixel_type);
	Int  	GetContextEnhBAB_FULL (	UChar *lower_bab_data,
			UChar *curr_bab_data,
			Int x2,
			Int y2,
			Int width,
			Int width2	);
	Void  	AddBorderToBABs (	UChar *LowShape,
			UChar *HalfShape,
			UChar *CurShape,
			UChar *lower_bab,
			UChar *half_bab,
			UChar *curr_bab,
			UChar *bordered_lower_bab,
			UChar *bordered_half_bab,
			UChar *bordered_curr_bab,
			Int object_width,
			Int object_height,
			Int blkx,
			Int blky,
			Int mblksize,
			Int max_blkx	);
	Int		DecideScanOrder(UChar *bordered_lower_bab, Int mbsize);

	//ShapeUtil.cpp
	Void * 	mymalloc (	size_t size	);
	UChar ** 	malloc_2d_Char (	Int d1,
			Int d2	);
	Void  	free_2d_Char (	UChar **array_2d,
			Int d1	);
	Int ** 	malloc_2d_Int (	Int d1,
			Int d2	);
	Void  	free_2d_Int (	Int **array_2d,
			Int d1	);

	//end added by SL@Sarnoff (03/03/99)
};

class CVTCEncoder : public CVTCCommon, 
					public VTCIMAGEBOX,
					public VTCDWT,
					public VTCDWTMASK, // hjlee 0901
					public VTCIDWTMASK //added by SL@Sarnoff (03/03/99)
{
public:
	// Constructor and Deconstructor
	~CVTCEncoder ();
	CVTCEncoder ();

	// input/output 
	Char *m_cImagePath;
	Char *m_cOutBitsFile;
	Char *m_cSegImagePath;
	Int used_bits; // added by Sharp (99/2/16)
//added by SL@Sarnoff (03/03/99)

	BSS *ShapeBitstream;
	Int ShapeBitstreamLength;
//end: added by SL@Sarnoff (03/03/99)
	Void init(		
		UInt uiVerID, // added by Sharp (99/11/18)
		Char* cImagePath,
	    UInt uiAlphaChannel,
		Char* cSegImagePath,
		UInt uiAlphaTh,
		UInt uiChangeCRDisable,
// FPDAM begin: deleted by Sharp
//		UInt uiShapeScalable, //added by SL@Sarnoff (03/02/99)
// FPDAM end: deleted by Sharp
		UInt uiSTOConstAlpha, //added by SL@Sarnoff (03/03/99)
		UInt uiSTOConstAlphaValue, //added by SL@Sarnoff (03/02/99)
		Char* cOutBitsFile,
		UInt uiColors,
		UInt uiFrmWidth,
		UInt uiFrmHeight,
// begin: added by Sharp (99/2/16)
    UInt uiTilingEnable,
    UInt uiTilingJump,
    UInt uiTileWidth,
    UInt uiTileHeight,
// end: added by Sharp (99/2/16)
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
		UInt uiTargetShapeLev, //added by SL@Sarnoff (03/02/99)
		UInt uiFullSizeOut, //added by SL@Sarnoff (03/02/99)
// begin: added by Sharp (99/2/16)
    UInt uiTargetTileFrom,
    UInt uiTargetTileTo,
// end: added by Sharp (99/2/16)
		UInt uiQdcY,
		UInt uiQdcUV,
		UInt uiSpatialLev ,
		UInt defaultSpatialScale, // hjlee 0901
		Int  lastWvtDecompInSpaLayer[MAXDECOMPLEV], // hjlee 0901
		SNR_PARAM* Qinfo[],
//Added by Sarnoff for error resilience, 3/5/99
		Int uiErrResiDisable,  
		Int uiPacketThresh,  
		Int uiSegmentThresh
//Added by Sarnoff for error resilience, 3/5/99
);

	// attribute
	Int GetcurSpatialLev() { 
		return mzte_codec.m_iCurSpatialLev; }
	Int GetDCHeight() { 
		return mzte_codec.m_iHeight>>mzte_codec.m_iWvtDecmpLev; }
	Int GetDCWidth() { 
		return mzte_codec.m_iWidth>>mzte_codec.m_iWvtDecmpLev; }

	// operation
	Void encode();

// begin: added by Sharp (99/2/16)
  Int emit_bits_local( UShort data, Int size, FILE *fp );
  Void tile_header_Enc(FILTER **filter, Int tile_id);
  Void get_orgval(DATA **Dst, Int TileID);
  Void perform_DWT_Tile(FILTER **wvtfilter, PICTURE *SrcImg, Int TileID);
  Void init_tile(Int tile_width, Int tile_height);
  long current_fp();
  Int current_put_bits();
// end: added by Sharp (99/2/16)

// FPDAM begin: modified by Sharp
	Void cut_tile_image(PICTURE *DstImage, PICTURE *SrcImage, Int iTile, Int colors, Int tile_width, Int tile_height, FILTER *filter);
	// added by Sharp (99/5/10)
// FPDAM end: modified by Sharp
 

protected:

	// vtcenc.cpp
	Void flush_buffer_file();
	Void close_buffer_file(FILE *fp);
	Void header_Enc_V1(FILTER **wvtfilter); // hjlee 0901
	Void texture_packet_header_Enc(FILTER **wvtfilter); // added by Sharp (99/4/7)
	long header_Enc(FILTER **wvtfilter); // hjlee 0901, modified by Sharp (99/2/16)
	Void header_Enc_Common(FILTER **wvtfilter, Int SkipShape = 0); // added by Sharp (99/2/16) //  @@@@@@
	Void Put_Quant_and_Max(SNR_IMAGE *snr_image, Int spaLayer, Int color); // hjlee0901
	Void Put_Quant_and_Max_SQBB(SNR_IMAGE *snr_image, Int spaLayer,Int color); // hjlee0901

	Void textureLayerDC_Enc();
	Void TextureSpatialLayerSQNSC_enc(Int spa_lev);
	Void TextureSpatialLayerSQ_enc(Int spa_lev, FILE *bitfile);
	Void textureLayerSQ_Enc(FILE *bitfile);
	Void TextureObjectLayer_enc(FILE *bitfile); // hjlee 0901, Modified by Sharp (99/2/16)
	Void TextureObjectLayer_enc_V1(FILTER **wvtfilter); // hjlee 0901
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
		       /* Int change_CR_disable,*/ FILTER *Filter); //modified by SL 03/03/99
// FPDAM begin: added by Sharp
	Void get_real_image(PICTURE *MyImage, Int wvtDecompLev, 
		       Int usemask, Int colors, Int alphaTH, 
		       FILTER *Filter);
// FPDAM end: added by Sharp
	Void get_virtual_image_V1(PICTURE *MyImage, Int wvtDecompLev, 
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
//Added by Sarnoff for error resilience, 3/5/99
	Void init_arith_encoder_model(Int color);
	Void close_arith_encoder_model(Int color, Int mode);
	Void check_end_of_packet(Int color);
	Void force_end_of_packet();
	Void check_end_of_DC_packet(Int numBP);
	Void check_segment_size(Int col);
	Void encodeSQBlocks_ErrResi(Int y, Int x, Int n, Int c);
//End added by Sarnoff for error resilience, 3/5/99

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

	Void emit_bits_checksc(UInt code, Int size);
	Void emit_bits_checksc_init();
	Void write_to_bitstream(UChar *bitbuffer,Int total_bits);
//Added by Sarnoff for error resilience, 3/5/99
	Void write_packet_header_to_file();
//End Added by Sarnoff for error resilience, 3/5/99

	/* for bilevel mode: added by Jie Liang */
    /* from PEZW_textureBQ.c */
    void textureLayerBQ_Enc(FILE *bitfile);

    /* from PEZW_utils.c */
    void PEZW_bitpack (PEZW_SPATIAL_LAYER **SPlayer);
    void PEZW_freeEnc (PEZW_SPATIAL_LAYER **SPlayer);
	// begin: added by SL@Sarnoff (03/03/99)
	// ShapeEncoding.cpp
	Int ShapeEnCoding(UChar *inmask, Int width, Int height, 
		  Int levels,  
		  Int constAlpha,
		  UChar constAlphaValue,
		  Int change_CR_disable,
		  Int shapeScalable,
		  Int startCodeEnable,
		  FILTER **filter);
	Int EncodeShapeHeader( Int constAlpha, UChar constAlphaValue, Int change_CR_disable /*, Int shapeScalable*/ ); // FPDAM : deleted by Sharp
	Int EncodeShapeBaseLayer(UChar *outmask, 
			 Int change_CR_disable,
			 Int coded_width, Int coded_height, 
			 Int levels);
	Int EncodeShapeEnhancedLayer(UChar *outmask, 
			     Int coded_width, Int coded_height, 
			     Int k, FILTER * filter, Int startCodeEnable);
	Int QuantizeShape(UChar *inmask, Int object_width, Int object_height, Int alphaTH);
	Int ByteAlignmentEnc_Still();
	Void PutBitstoStream_Still(Int bits,UInt code);

	 /* ShapeBaseEncoding.c */
	Int  	ShapeBaseEnCoding (UChar *InShape, 
	    Int object_width, Int object_height,
	    Int alphaTH, Int change_CR_disable
	);
	Void  MergeShapeBaseBitstream();
	Int
	ShapeBaseContentEncode(Int i,Int j,Int bsize,UChar **BAB,SBI *infor);
	Int
	CheckBABstatus(Int blkn,UChar **BAB1,UChar **BAB2,Int alphaTH);
	Int
	ShapeBaseHeaderEncode(Int i,Int j,Int blkx,SBI *infor);

	Int
	decide_CR(Int x,Int y,Int blkn,Int blkx, UChar **BAB_org,UChar **BAB_dwn,Int change_CR_disable,Int alphaTH,UChar **shape);


	/* ShapeEnhEncode.cpp */
	Int ShapeEnhEnCoding(UChar *LowShape, UChar *HalfShape, UChar *CurShape, 
			Int object_width, Int object_height, FILTER *filter);

//	Int DecideBABtype(UChar *bordered_lower_bab, UChar *bordered_half_bab,
//		  UChar *bordered_curr_bab,
//		  Int mbsize);
	Int DecideBABtype(UChar *bordered_lower_bab, UChar *bordered_half_bab,
		  UChar *bordered_curr_bab,
		  Int mbsize, 
		  Int scan_order // SAIT_PDAM : added by Samsung AIT 
		  );


	/*Int ShapeEnhHeaderEncode(UChar *bab_mode,
				Int blky,
				Int blkx,
			 FILTER *filter,
				BSS *bitstream,
				ArCoder *ar_coder);
	*/
//	Int ShapeEnhContentEncode(UChar *bordered_lower_bab,
//				UChar *bordered_half_bab,
//				UChar *bordered_curr_bab,
//				Int bab_type,
//				Int mbsize,
//					FILTER *filter,
//				BSS *bitstream,
//				ArCoder *ar_coder);

	Int ShapeEnhContentEncode(UChar *bordered_lower_bab,
				UChar *bordered_half_bab,
				UChar *bordered_curr_bab,
				Int bab_type,
				Int scan_order, // SAIT_PDAM: added by Samsung AIT 
				Int mbsize,
					FILTER *filter,
				BSS *bitstream,
				ArCoder *ar_coder);

	
//	Void ExclusiveORencoding (UChar *bordered_lower_bab,
//				UChar *bordered_half_bab,
//				UChar *bordered_curr_bab,
//				Int mbsize,
//				BSS *bitstream,
//				ArCoder *ar_coder);
	Void ExclusiveORencoding (UChar *bordered_lower_bab,
				UChar *bordered_half_bab,
				UChar *bordered_curr_bab,
				Int mbsize,
				Int scan_order, // SAIT_PDAM: added by Samsung AIT
				BSS *bitstream,
				ArCoder *ar_coder);

	Void FullEncoding (UChar *bordered_lower_bab,
			UChar *bordered_half_bab,
			UChar *bordered_curr_bab,
			Int mbsize,
		   FILTER *filter,
			BSS *bitstream,
			ArCoder *ar_coder);

	Void MergeEnhShapeBitstream();

	//BinArCodec.cpp
	Void  	StartArCoder_Still (	ArCoder *coder);
	Void  	StopArCoder_Still (	ArCoder *coder,
			BSS *bitstream);
	Void  	ArCodeSymbol_Still (	ArCoder *coder,
			BSS *bitstream,
			UChar bit,
			UInt c0);
	Void  	EncRenormalize (	ArCoder *coder,
			BSS *bitstream);
	Void  	BitByItself_Still (	Int bit,
			ArCoder *coder,
			BSS *bitstream);
	Void  	BitPlusFollow_Still (	Int bit,
			ArCoder *coder,
			BSS *bitstream);
	Void  	BitstreamPutBit_Still (	Int bit,
			BSS *bitstream);
	Void  	InitBitstream (	Int flag,
			BSS *bitstream);
	Int  	ByteAlignmentEnc (void);
	Void  	PutBitstoStream (	Int bits,
			UInt code,
			BSS *bitstream);
	Void  	PutBitstoStreamMerge (	Int bits,
			UInt code);
	Void  	BitstreamFlushBitsCopy (	Int i,
			BSS *bitstream);
	Int  	BitstreamLookBitCopy (	Int pos,
			BSS *bitstream);
	UInt  	LookBitsFromStreamCopy (	Int bits,
			BSS *bitstream);
	UInt  	GetBitsFromStreamCopy (	Int bits,
			BSS *bitstream	);
	Void  	BitStreamCopy (	Int cnt,
			BSS *bitstream1,
			BSS *bitstream2	);
	Void  	BitStreamMerge (	Int cnt,
			BSS *bitstream	);
	//end: added by SL@Sarnoff (03/03/99)
};

class CVTCDecoder : public CVTCCommon, 
					public VTCIMAGEBOX,
					public VTCIDWT,
					public VTCDWTMASK,
					public VTCIDWTMASK //added by SL@Sarnoff (03/03/99)
{
public:
	// Constructor and Deconstructor
	~CVTCDecoder ();
	CVTCDecoder ();

	Char *m_cInBitsFile;
	Char *m_cRecImageFile;
	
	// vtcdec.cpp
	Void texture_packet_header_Dec(FILTER ***wvtfilter, PICTURE **Image, Int *header_size); // Added by Sharp (99/4/7)
	Void header_Dec_V1(FILTER ***wvtfilter, PICTURE **Image); // hjlee 0901
	Void header_Dec(FILTER ***wvtfilter, PICTURE **Image, Int *header_size); // hjlee 0901 , Modified by Sharp (99/2/16)
	Void header_Dec_Common(FILTER ***wvtfilter, PICTURE **Image, Int *header_size, Int SkipShape=0); // Added by Sharp (99/2/16) // @@@@@@@
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
				   Int  target_snr_levels, FILTER ***pwvtfilter,
           Int iTile, Int count,FILE *bitfile, Int **table, PICTURE *Image); // hjlee 0901 // modified by Sharp (99/2/16)
	Void TextureObjectLayer_dec_V1(Int  target_spatial_levels,
				   Int  target_snr_levels, FILTER ***pwvtfilter); // hjlee 0901


	Void decode(Char *InBitsFile, Char *RecImageFile,
				Int DisplayWidth, Int DisplayHeight,
				Int TargetSpaLev, Int TargetSNRLev, 
				Int TargetShapeLev, Int FullSizeOut, // added by SL@Sarnoff (03/02/99)
				Int TargetTileFrom, Int TargetTileto);  // modified by Sharp (99/2/16)

	// seg.cpp
// hjlee 0901
	Void get_virtual_mask_V1(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, FILTER **filters) ;
	Void get_virtual_mask(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, 
			  Int *target_shape_layer, Int StartCodeEnable,
			  FILTER **filters) ;
// begin: added by Sharp (99/5/10)
// FPDAM begin: modified by Sharp
//	Void perform_IDWT_Tile(FILTER **wvtfilter, UChar **frm, Int iTile, Int TileW);
	Void perform_IDWT_Tile(FILTER **wvtfilter, UChar **frm, UChar **frm_mask, Int iTile, Int TileW);
// FPDAM begin: modified by Sharp

// FPDAM begin
//	Void get_virtual_tile_mask(Int iTile, Int TileX, Int TileY, PICTURE *Image);
	Void get_virtual_tile_mask(PICTURE *MyImage,  Int wvtDecompLev,
				Int object_width, Int object_height, 
				Int tile_width, Int tile_height,
				Int iTile, Int TileX, Int TileY,
				Int usemask, Int tile_type, Int colors, 
			  Int *target_shape_layer, 
				Int StartCodeEnable,
			  FILTER **filters);
// FPDAM end
// end: added by Sharp (99/5/10)
	 

	Void get_virtual_mask_TILE(PICTURE *MyImage,  Int wvtDecompLev,
		      Int w, Int h, Int usemask, Int colors, 
			  Int *target_shape_layer, Int StartCodeEnable,
			  FILTER **filters) ;

			  
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

//Added by Sarnoff for error resilience, 3/5/99
	// ztscanUtil.cpp
	Void get_TU_location(Int LTU);
	Void set_prev_good_TD_segment(Int TU, Int h, Int w);
	Void set_prev_good_BB_segment(Int TU, Int h, Int w);
//End Added by Sarnoff for error resilience, 3/5/99

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
//Added by Sarnoff for error resilience, 3/5/99
	Void mark_not_decodable_TUs(Int start_TU, Int end_TU);
	Void init_arith_decoder_model(Int col);
	Void close_arith_decoder_model(Int color);
	Void check_end_of_packet();
	Void check_end_of_DC_packet(Int numBP);
	Int found_segment_error(Int col);
	Void decodeSQBlocks_ErrResi(Int y, Int x, Int n, Int c);
//End added by Sarnoff for error resilience, 3/5/99

	// bitpack.cpp
	Int align_byte ();
	Int get_param(Int nbits);
	Void restore_arithmetic_offset(Int bits_to_go);
	UInt LookBitFromStream (Int n); //added by SL@Sarnoff (03/03/99)
//Added by Sarnoff for error resilience, 3/5/99
	Void error_bits_stat(Int error_mode);
	Int get_err_resilience_header();
	Void rewind_bits(Int nbits);
	Int end_of_stream();
//End Added by Sarnoff for error resilience, 3/5/99

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
// begin: added by Sharp (99/5/10)
	Void write_image_tile(Char *recImgFile, UChar **frm);

// FPDAM begin: modified by Sharp
	Void write_image_to_buffer(UChar **DstImage, 
		UChar *DstMask[3], // FPDAM added by Sharp
		Int DstWidth, Int DstHeight, Int iTile, Int TileW,
		/* Char *recImgFile,*/ Int colors,
		 Int width, Int height,
		 Int real_width, Int real_height,
		 Int rorigin_x, Int rorigin_y,
		 UChar *outimage[3], UChar *outmask[3],
		 Int usemask, Int fullsize, Int MinLevel);
//	Void write_image_to_buffer(UChar **DstImage, Int DstWidth, Int DstHeight, Int iTile, Int TileW,
// 		/* Char *recImgFile,*/ Int colors,
// 		 Int width, Int height,
// 		 Int real_width, Int real_height,
// 		 Int rorigin_x, Int rorigin_y,
// 		 UChar *outimage[3], UChar *outmask[3],
// 		 Int usemask, Int fullsize, Int MinLevel);
// FPDAM end: modified by Sharp
// end: added by Sharp (99/5/10)


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
// begin: added by Sharp (99/2/16)
	Void search_tile(int tile_id);
// FPDAM begin: modified by Sharp
//  Void tile_header_Dec();
	Void tile_header_Dec(FILTER **wvtfilter, Int iTile, Int TileX, Int count, Int TileY, PICTURE **Image);
// FPDAM end: modified by Sharp
  Void tile_table_Dec(Int *table);
  Void set_decode_tile_id_and_position(Int *iNumOfTile, Int **jump_table, Int **decode_tile_id, Int *table, Int header_size);
  Void copy_coeffs(Int iTile, DATA **frm);
//  Void perform_IDWT_Tile(FILTER **wvtfilter, Char *recImgFile, DATA **frm); // deleted by Sharp (99/5/10)
  Void relative_jump(long size);
  Void clear_coeffinfo();
// end: added by Sharp (99/2/16)
	//begin: added by SL@Sarnoff (03/03/99)
	//ShapeDecoding.cpp
	Int ShapeDeCoding(UChar *mask, Int width, Int height, 
		  Int levels,  Int *targetLevel, 
		  Int *constAlpha,
		  UChar *constAlphaValue, 
		  Int startCodeEnable,
		  Int fullSizeOut,
		  FILTER **filter);

	Int DecodeShapeHeader(Int *constAlpha, UChar *constAlphaValue, 
		      Int *change_CR_disable /*, Int *shapeScalable*/ );
	Int DecodeShapeBaseLayer( UChar *outmask,  Int change_CR_disable,
			 Int coded_width, Int coded_height, Int levels);
	Int DecodeShapeEnhancedLayer( UChar *outmask, 
			      Int coded_width, Int coded_height, 
			      Int k, FILTER *filter, Int startCodeEnable);
	Int ByteAlignmentDec_Still();
	UInt GetBitsFromStream_Still(Int bits);
	Int BitstreamLookBit_Still(Int pos);
	UInt LookBitsFromStream_Still(Int bits);

	//ShapeBaseDecode.cpp
	Int  	ShapeBaseDeCoding (UChar *OutShape,
			    Int object_width, Int object_height,
			    Int change_CR_disable
			    );
	Int
	ShapeBaseContentDecode(Int i,Int j,Int bsize,UChar **BAB,SBI *infor);
	Int
	ShapeBaseHeaderDecode(Int i,Int j,Int blkx,SBI *infor);
	//ShapeEnhDecode.cpp
	Int ShapeEnhDeCoding(UChar *LowShape, UChar *HalfShape, UChar *CurShape,
			Int object_width,
			Int object_height,
			FILTER *filter);

	/*Int ShapeEnhHeaderDecode(UChar *bab_mode,
				Int blky,
				Int blkx,
			 FILTER *filter,
			 ArDecoder *ar_decoder);
     */
	Int ShapeEnhContentDecode(UChar *bordered_lower_bab,
			  UChar *bordered_half_bab, 	
			  UChar *bordered_curr_bab,
			  Int bab_type,
			  Int mbsize,
			  FILTER *filter,
			  ArDecoder *ar_decoder);

//	Void ExclusiveORdecoding (UChar *bordered_lower_bab,UChar *bordered_half_bab,
//			  UChar *bordered_curr_bab,
//			  Int mbsize,
//			  ArDecoder *ar_decoder);

	Void ExclusiveORdecoding (UChar *bordered_lower_bab,UChar *bordered_half_bab,
			  UChar *bordered_curr_bab,
			  Int mbsize,
			  Int scan_order, // SAIT_PDAM: added by Samsung AIT
			  ArDecoder *ar_decoder);

	Void FullDecoding (UChar *bordered_lower_bab, UChar *bordered_half_bab,
		   UChar *bordered_curr_bab,
		   Int mbsize,
		   FILTER *filter,
		   ArDecoder *ar_decoder);
	Void  	StartArDecoder_Still (	ArDecoder *decoder	);
	Void  	StopArDecoder_Still (	ArDecoder *decoder	);
	UChar  	ArDecodeSymbol_Still (	ArDecoder *decoder,
			UInt c0	);
	Void  	AddNextInputBit_Still (	ArDecoder *decoder);
	Void  	DecRenormalize (	ArDecoder *decoder	);
	Int  	ByteAlignmentDec (void);
	Void  	BitstreamFlushBits_Still (	Int i	);
	Int  	BitstreamLookBit (	Int pos	);
	UInt  	LookBitsFromStream (	Int bits	);
	UInt  	GetBitsFromStream (	Int bits	);
	//end: added by SL@Sarnoff (03/03/99)
};

#endif 
