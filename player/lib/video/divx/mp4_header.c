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
**/
// mp4_header.c //

#include <stdlib.h>
#include <math.h>
#include "divxif.h"
#include "mp4_decoder.h"
#include "global.h"

#include "mp4_header.h"

/**
 *
**/


/***/

int getvolhdr()
{
	int code;

	if (showbits(27) == VO_START_CODE)
	{
		getbits(27); // start_code
		getbits(5); // vo_id
	}

	if (showbits(28) == VOL_START_CODE) {
	  getbits(28);
		mp4_hdr.ident = getbits(4); // vol_id
		mp4_hdr.random_accessible_vol = getbits(1);
		mp4_hdr.type_indication = getbits(8); 
		mp4_hdr.is_object_layer_identifier = getbits(1);

		if (mp4_hdr.is_object_layer_identifier) {
			mp4_hdr.visual_object_layer_verid = getbits(4);
			mp4_hdr.visual_object_layer_priority = getbits(3);
		} 
		else {
			mp4_hdr.visual_object_layer_verid = 1;
			mp4_hdr.visual_object_layer_priority = 1;
		}
		mp4_hdr.aspect_ratio_info = getbits(4);
		mp4_hdr.vol_control_parameters = getbits(1);
		mp4_hdr.shape = getbits(2);
		getbits1(); // marker
		mp4_hdr.time_increment_resolution = getbits(16);
		getbits1(); // marker
		mp4_hdr.fixed_vop_rate = getbits(1);
		
		mp4_hdr.time_increment_bits = 0;
		while (mp4_hdr.time_increment_resolution > 
		       (1 << mp4_hdr.time_increment_bits)) {
		  mp4_hdr.time_increment_bits++;
		}
		
		if (mp4_hdr.fixed_vop_rate) 
		{
		  int temp;
			temp = getbits(mp4_hdr.time_increment_bits);
			mp4_hdr.fps = 
			  mp4_hdr.time_increment_resolution / temp;
		} else {
		  mp4_hdr.fps = mp4_hdr.time_increment_resolution;
		}

		if (mp4_hdr.shape != BINARY_SHAPE_ONLY)  
		{
			if(mp4_hdr.shape == 0)
			{
				getbits1(); // marker
				mp4_hdr.width = getbits(13);
				getbits1(); // marker
				mp4_hdr.height = getbits(13);
				getbits1(); // marker
			}

			mp4_hdr.interlaced = getbits(1);
			mp4_hdr.obmc_disable = getbits(1);
			
			if (mp4_hdr.visual_object_layer_verid == 1) {
				mp4_hdr.sprite_usage = getbits(1);
			} 
			else {
				mp4_hdr.sprite_usage = getbits(2);
			}
			
			code = getbits(1);
			if (code == 1) 
			{
				mp4_hdr.quant_precision = getbits(4);
				mp4_hdr.bits_per_pixel = getbits(4);
			}
			else 
			{
				mp4_hdr.quant_precision = 5;
				mp4_hdr.bits_per_pixel = 8;
			}

			mp4_hdr.quant_type = getbits(1); // quant type
			if (mp4_hdr.quant_type == 1) {
			  getbits(2);
			}
#if 1
			// wmay/dmackie - added 3/26/01
			if (mp4_hdr.visual_object_layer_verid != 1)
			  getbits(1); // vol quarter pixel
#endif
			mp4_hdr.complexity_estimation_disable = getbits(1);
			mp4_hdr.error_res_disable = getbits(1);
			mp4_hdr.data_partitioning = getbits(1);
			if (mp4_hdr.data_partitioning) {
			  printf("Have data partitioning unsupported\n");
#if 0
				exit(102);
#else
				return 0;
#endif
			}	  
			else {
				mp4_hdr.error_res_disable = 1;
			}
			
			mp4_hdr.intra_acdc_pred_disable = 0;
			mp4_hdr.scalability = getbits(1);

			if(mp4_hdr.scalability)	{
			  printf("Have scalability\n");
#if 0
				exit(103);
#else
				return (0);
#endif
			}
			
			// next_start_code();
#if 0
			if (showbits(32) == USER_DATA_START_CODE) {
				exit(104);
			}
#endif
		} 

		return 1;
	} else {
	  getbits(8);
	}
  
  return 0; // no VO start code
}

/***/

int getvophdr(void)
{
	unsigned int temp;
	next_start_code();

	temp = showbits(32);
	if (temp != (int) VOP_START_CODE)
	  {
	    if (((temp >> 4) == VOL_START_CODE) ||
		((temp >> 5) == VO_START_CODE)) {
	      if (getvolhdr() > 0) {
		post_volprocessing();
		next_start_code();
	      }
	    } else 
	      getbits(8);
	    temp = showbits(32);
	    if (temp != (int) VOP_START_CODE) return 0;
	    
	  }
	flushbits(32);
	mp4_hdr.prediction_type = getbits(2);

	while (getbits(1) == 1) // temporal time base
  {
		mp4_hdr.time_base++;
  }
	getbits1(); // marker bit
	mp4_hdr.time_inc = getbits(mp4_hdr.time_increment_bits); // vop_time_increment (1-16 bits)

	getbits1(); // marker bit
	mp4_hdr.vop_coded = getbits(1);
	if (mp4_hdr.vop_coded == 0) 
	{
		next_start_code();
		return 1;
	}
	
	if ((mp4_hdr.shape != BINARY_SHAPE_ONLY) &&
		(mp4_hdr.prediction_type == P_VOP)) 
	{
		mp4_hdr.rounding_type = getbits(1);
	} else {
		mp4_hdr.rounding_type = 0;
	}
	
	if (mp4_hdr.shape != RECTANGULAR)
	{
		if (! (mp4_hdr.sprite_usage == STATIC_SPRITE && 
			mp4_hdr.prediction_type==I_VOP) )
		{
			mp4_hdr.width = getbits(13);
			getbits1();
			mp4_hdr.height = getbits(13);
			getbits1();
			mp4_hdr.hor_spat_ref = getbits(13);
			getbits1();
			mp4_hdr.ver_spat_ref = getbits(13);
		}
		
		mp4_hdr.change_CR_disable = getbits(1);
		
		mp4_hdr.constant_alpha = getbits(1);
		if (mp4_hdr.constant_alpha) {
			mp4_hdr.constant_alpha_value = getbits(8);
		}
  }

	if (! (mp4_hdr.complexity_estimation_disable)) {
		exit(108);
	}

	if (mp4_hdr.shape != BINARY_SHAPE_ONLY)  
  { 
		mp4_hdr.intra_dc_vlc_thr = getbits(3);
		if (mp4_hdr.interlaced) {
			exit(109);
		}
  }
	
	if (mp4_hdr.shape != BINARY_SHAPE_ONLY) 
  { 
		mp4_hdr.quantizer = getbits(mp4_hdr.quant_precision); // vop quant

		if (mp4_hdr.prediction_type != I_VOP) 
		{
			mp4_hdr.fcode_for = getbits(3); 
		}
		
		if (! mp4_hdr.scalability) {
			if (mp4_hdr.shape && mp4_hdr.prediction_type!=I_VOP)
				mp4_hdr.shape_coding_type = getbits(1); // vop shape coding type

			/* motion_shape_texture() */
		}
	} 

	return 1;
}

/***/


void __inline next_start_code(void)
{
	if (juice_flag)
	{
		// juice_flag = 0; // [Ag] before juice needed this changed only first time
		if (! bytealigned(0))
		{
			getbits(1);

			// bytealign
			while (! bytealigned(0)) {
				flushbits(1);
			}
		}
	}
	else
	{
	  // wmay - commented out - this part fails if we ever need to
	  // skip around.
	  //getbits(1);

		// bytealign
		while (! bytealigned(0)) {
			flushbits(1);
		}
	}
}

/***/




