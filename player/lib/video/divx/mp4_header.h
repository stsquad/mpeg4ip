/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
// mp4_header.h //

#ifndef _MP4_HEADER_H_
#define _MP4_HEADER_H_

#define VO_START_CODE		0x8
#define VOL_START_CODE	0x12
#define VOP_START_CODE	0x1b6

#define I_VOP		0
#define P_VOP		1
#define B_VOP		2

#define RECTANGULAR				0
#define BINARY						1
#define BINARY_SHAPE_ONLY 2 
#define GREY_SCALE				3

#define STATIC_SPRITE			1

#define USER_DATA_START_CODE	0x01b2

#define NOT_CODED -1
#define INTER			0
#define INTER_Q	  1
#define INTER4V		2
#define INTRA			3
#define INTRA_Q	 	4
#define STUFFING	7

/*** *** ***/

typedef struct _mp4_header {

	// vol
	int ident;
	int random_accessible_vol;
	int type_indication;
	int is_object_layer_identifier;
	int visual_object_layer_verid;
	int visual_object_layer_priority;
	int aspect_ratio_info;
	int vol_control_parameters;
	int shape;
	int time_increment_resolution;
	int fixed_vop_rate;
	int time_increment_bits;
  int fps;
	int width;
	int height;
	int interlaced;
	int obmc_disable;
	int sprite_usage;
	int quant_precision;
	int bits_per_pixel;
	int quant_type;
	int complexity_estimation_disable;
	int error_res_disable;
	int data_partitioning;
	int intra_acdc_pred_disable;
	int scalability;

	// vop
	int prediction_type;
	int time_base;
	int time_inc;
	int vop_coded;
	int rounding_type;
	int hor_spat_ref;
	int ver_spat_ref;
	int change_CR_disable;
	int constant_alpha;
	int constant_alpha_value;
	int intra_dc_vlc_thr;
	int quantizer;
	int fcode_for;
	int shape_coding_type;

	// macroblock
	int not_coded;
	int mcbpc;
	int derived_mb_type;
	int cbpc;
	int ac_pred_flag;
	int cbpy;
	int dquant;
	int cbp;

	// extra/derived
	int mba_size;
	int mb_xsize;
	int mb_ysize;
	int picnum;
	int mba;
	int mb_xpos;
	int mb_ypos;
	int dc_scaler;

} mp4_header;

/*** *** ***/

extern int getvolhdr(void);
extern int getvophdr(void);

extern int __inline nextbits(int nbits);
extern int __inline bytealigned(int nbits);
extern void __inline next_start_code(void);
extern int __inline nextbits_bytealigned(int nbit);

#endif // _MP4_HEADER_H_
