/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	read mpeg headers
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *	30.02.2002	intra_dc_threshold support
 *	04.12.2001	support for additional headers
 *	16.12.2001	inital version; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/

#include "../portab.h"

#include "bitreader.h"
#include "decoder.h"

// comment any #defs we dont use

#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1	/* ??? */
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
//#define VIDSESERR_ERROR_CODE	0x000001b4
#define VISOBJ_START_CODE		0x000001b5
#define VOP_START_CODE			0x000001b6
//#define SLICE_START_CODE		0x000001b7
//#define EXT_START_CODE			0x000001b8


#define VISOBJ_TYPE_VIDEO			1
//#define VISOBJ_TYPE_STILLTEXTURE	2
//#define VISOBJ_TYPE_MESH			3
//#define VISOBJ_TYPE_FBA				4
//#define VISOBJ_TYPE_3DMESH			5


#define VIDOBJLAY_TYPE_SIMPLE			1
//#define VIDOBJLAY_TYPE_SIMPLE_SCALABLE	2
#define VIDOBJLAY_TYPE_CORE				3
#define VIDOBJLAY_TYPE_MAIN				4


//#define VIDOBJLAY_AR_SQUARE				1
//#define VIDOBJLAY_AR_625TYPE_43			2
//#define VIDOBJLAY_AR_525TYPE_43			3
//#define VIDOBJLAY_AR_625TYPE_169		8
//#define VIDOBJLAY_AR_525TYPE_169		9
#define VIDOBJLAY_AR_EXTPAR				15


#define VIDOBJLAY_SHAPE_RECTANGULAR		0
#define VIDOBJLAY_SHAPE_BINARY			1
#define VIDOBJLAY_SHAPE_BINARY_ONLY		2
#define VIDOBJLAY_SHAPE_GRAYSCALE		3


#define MARKER()	bs_skip(bs, 1)


static int __inline log2bin(int value)
{
	int n = 0;
	while (value)
	{
		value >>= 1;
		n++;
	}
	return n;
}


static const uint32_t intra_dc_threshold_table[] =
{
	32,	/* never use */
	13,
	15,
	17,
	19,
	21,
	23,
	1,
};

/*
decode headers
returns coding_type, or -1 if error
*/


int bs_headers(BITREADER * bs, DECODER * dec, uint32_t * rounding, uint32_t * quant, uint32_t * fcode, uint32_t * intra_dc_threshold)
{
	uint32_t vol_ver_id;
	uint32_t time_inc_resolution;
	uint32_t coding_type;
	uint32_t start_code;
	
	do
	{
		bs_bytealign(bs);
		start_code = bs_show(bs, 32);

		if (start_code == VISOBJSEQ_START_CODE)
		{
			// DEBUG("visual_object_sequence");
			bs_skip(bs, 32);				// visual_object_sequence_start_code
			bs_skip(bs, 8);					// profile_and_level_indication
		}
		else if (start_code == VISOBJSEQ_STOP_CODE)
		{
			bs_skip(bs, 32);				// visual_object_sequence_stop_code
		}
		else if (start_code == VISOBJ_START_CODE)
		{
			// DEBUG("visual_object");
			bs_skip(bs,32);					// visual_object_start_code
			if (bs_get1(bs))				// is_visual_object_identified
			{
				vol_ver_id = bs_get(bs,4);	// visual_object_ver_id
				bs_skip(bs, 3);				// visual_object_priority
			}
			else
			{
				vol_ver_id = 1;
			}

			if (bs_show(bs, 4) != VISOBJ_TYPE_VIDEO)	// visual_object_type
			{
				DEBUG("visual_object_type != video");
				return -1;
			}
			bs_skip(bs, 4);

			// video_signal_type

			if (bs_get1(bs))				// video_signal_type
			{
				DEBUG("+ video_signal_type");
				bs_skip(bs, 3);				// video_format
				bs_skip(bs, 1);				// video_range
				if (bs_get1(bs))			// color_description
				{
					DEBUG("+ color_description");
					bs_skip(bs, 8);			// color_primaries
					bs_skip(bs, 8);			// transfer_characteristics
					bs_skip(bs, 8);			// matrix_coefficients
				}
			}
		}
		else if ((start_code & ~0x1f) == VIDOBJ_START_CODE)
		{
			bs_skip(bs, 32);		// video_object_start_code
		} 
		else if ((start_code & ~0xf) == VIDOBJLAY_START_CODE)
		{
			// DEBUG("video_object_layer");
			bs_skip(bs, 32);					// video_object_layer_start_code

			bs_skip(bs, 1);									// random_accessible_vol

			// video_object_type_indication
			if (bs_show(bs, 8) != VIDOBJLAY_TYPE_SIMPLE &&
				bs_show(bs, 8) != VIDOBJLAY_TYPE_CORE &&
				bs_show(bs, 8) != VIDOBJLAY_TYPE_MAIN &&
				bs_show(bs, 8) != 0)		// BUGGY DIVX
			{
				DEBUG1("video_object_type_indication not supported", bs_show(bs, 8));
				return -1;
			}
			bs_skip(bs, 8);


			if (bs_get1(bs))					// is_object_layer_identifier
			{
				DEBUG("+ is_object_layer_identifier");
				vol_ver_id = bs_get(bs,4);		// video_object_layer_verid
				bs_skip(bs, 3);					// video_object_layer_priority
			}
			else
			{
				vol_ver_id = 1;
			}
			//DEBUGI("vol_ver_id", vol_ver_id);

			if (bs_get(bs, 4) == VIDOBJLAY_AR_EXTPAR)	// aspect_ratio_info
			{
				DEBUG("+ aspect_ratio_info");
				bs_skip(bs, 8);						// par_width
				bs_skip(bs, 8);						// par_height
			}

			if (bs_get1(bs))		// vol_control_parameters
			{
				DEBUG("+ vol_control_parameters");
				bs_skip(bs, 2);						// chroma_format
				bs_skip(bs, 1);						// low_delay
				if (bs_get1(bs))					// vbv_parameters
				{
					DEBUG("+ vbv_parameters");
					bs_skip(bs, 15);				// first_half_bitrate
					MARKER();
					bs_skip(bs, 15);				// latter_half_bitrate
					MARKER();
					bs_skip(bs, 15);				// first_half_vbv_buffer_size
					MARKER();
					bs_skip(bs, 3);					// latter_half_vbv_buffer_size
					bs_skip(bs, 11);				// first_half_vbv_occupancy
					MARKER();
					bs_skip(bs, 15);				// latter_half_vbv_occupancy
					MARKER();
				
				}
			}

			dec->shape = bs_get(bs, 2);	// video_object_layer_shape
			// DEBUG1("shape", dec->shape);
			
			if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1)
			{
				bs_skip(bs, 4);		// video_object_layer_shape_extension
			}

			MARKER();

			time_inc_resolution = bs_get(bs, 16);	// vop_time_increment_resolution
			time_inc_resolution--;
			if (time_inc_resolution > 0)
			{
				dec->time_inc_bits = log2bin(time_inc_resolution);
			}
			else
			{
				// dec->time_inc_bits = 0;

				// for "old" xvid compatibility, set time_inc_bits = 1
				dec->time_inc_bits = 1;
			}

			MARKER();

			if (bs_get1(bs))						// fixed_vop_rate
			{
				bs_skip(bs, dec->time_inc_bits);	// fixed_vop_time_increment
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
			{

				if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR)
				{
					uint32_t width, height;

					MARKER();
					width = bs_get(bs, 13);			// video_object_layer_width
					//DEBUGI("width", width);
					MARKER();
					height = bs_get(bs, 13);		// video_object_layer_height
					//DEBUGI("height", height);	
					MARKER();

					if (width != dec->width || height != dec->height)
					{
						DEBUG("FATAL: video dimension discrepancy ***");
						DEBUG2("bitstream width/height", width, height);
						DEBUG2("param width/height", dec->width, dec->height);
						return -1;
					}

				}
			
				if (bs_get1(bs))				// interlaced
				{
					DEBUG("TODO: interlaced");
					// TODO
					return -1;
				}

				if (!bs_get1(bs))				// obmc_disable
				{
					DEBUG("IGNORED/TODO: !obmc_disable");
					// TODO
					// fucking divx4.02 has this enabled
				}

				if (bs_get(bs, (vol_ver_id == 1 ? 1 : 2)))  // sprite_enable
				{
					DEBUG("sprite_enable; not supported");
					return -1;
				}
			
				if (vol_ver_id != 1 && dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
				{
					bs_skip(bs, 1);					// sadct_disable
				}

				if (bs_get1(bs))						// not_8_bit
				{
					DEBUG("+ not_8_bit [IGNORED/TODO]");
					dec->quant_bits = bs_get(bs, 4);	// quant_precision
					bs_skip(bs, 4);						// bits_per_pixel
				}
				else
				{
					dec->quant_bits = 5;
				}

				if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE)
				{
					bs_skip(bs, 1);			// no_gray_quant_update
					bs_skip(bs, 1);			// composition_method
					bs_skip(bs, 1);			// linear_composition
				}

				dec->quant_type = bs_get1(bs);		// quant_type
				// DEBUG1("**** quant_type", dec->quant_type);

				if (dec->quant_type)
				{
					if (bs_get1(bs))		// load_intra_quant_mat
					{
						DEBUG("TODO: load_intra_quant_mat");
						// TODO
						return -1;
					}
					if (bs_get1(bs))		// load_inter_quant_mat
					{
						DEBUG("TODO: load_inter_quant_mat");
						// TODO
						return -1;
					}

					if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE)
					{
						// TODO
						DEBUG("TODO: grayscale matrix stuff");
						return -1;
					}
				}

			
				if (vol_ver_id != 1)
				{
					dec->quarterpel = bs_get1(bs);	// quarter_sampe
					if (dec->quarterpel)
					{
						DEBUG("IGNORED/TODO: quarter_sample");
					}
				}
				else
				{
					dec->quarterpel = 0;
				}

				if (!bs_get1(bs))			// complexity_estimation_disable
				{
					DEBUG("TODO: complexity_estimation header");
					// TODO
					return -1;
				}

				if (!bs_get1(bs))			// resync_marker_disable
				{
					DEBUG("IGNORED/TODO: !resync_marker_disable");
					// TODO
				}

				if (bs_get1(bs))		// data_partitioned
				{
					DEBUG("+ data_partitioned");
					bs_skip(bs, 1);		// reversible_vlc
				}

				if (vol_ver_id != 1)
				{
					if (bs_get1(bs))			// newpred_enable
					{
						DEBUG("+ newpred_enable");
						bs_skip(bs, 2);			// requested_upstream_message_type
						bs_skip(bs, 1);			// newpred_segment_type
					}
					if (bs_get1(bs))			// reduced_resolution_vop_enable
					{
						DEBUG("TODO: reduced_resolution_vop_enable");
						// TODO
						return -1;
					}
				}
				
				if (bs_get1(bs))	// scalability
				{
					// TODO
					DEBUG("TODO: scalability");
					return -1;
				}
			}
			else	// dec->shape == BINARY_ONLY
			{
				if (vol_ver_id != 1)
				{
					if (bs_get1(bs))	// scalability
					{
						// TODO
						DEBUG("TODO: scalability");
						return -1;
					}
				}
				bs_skip(bs, 1);			// resync_marker_disable

			}

		}
		else if (start_code == GRPOFVOP_START_CODE)
		{
			// DEBUG("group_of_vop");
			bs_skip(bs, 32);
			{
				int hours, minutes, seconds;
				hours = bs_get(bs, 5);
				minutes = bs_get(bs, 6);
				MARKER();
				seconds = bs_get(bs, 6);
				// DEBUG3("hms", hours, minutes, seconds);
			}
			bs_skip(bs, 1);			// closed_gov
			bs_skip(bs, 1);			// broken_link
		}
		else if (start_code == VOP_START_CODE)
		{
			// DEBUG("vop_start_code");
			bs_skip(bs, 32);						// vop_start_code

			coding_type = bs_get(bs, 2);		// vop_coding_type
			//DEBUG1("coding_type", coding_type);

			while (bs_get1(bs) != 0) ;			// time_base
	
			MARKER();
	 
			//DEBUG1("time_inc_bits", dec->time_inc_bits);
			//DEBUG1("vop_time_incr", bs_show(bs, dec->time_inc_bits));
			if (dec->time_inc_bits)
			{
				bs_skip(bs, dec->time_inc_bits);	// vop_time_increment
			}

			MARKER();

			if (!bs_get1(bs))					// vop_coded
			{
				return N_VOP;
			}

			/* if (newpred_enable)
			{
			}
			*/

			if (coding_type != I_VOP)
			{
				*rounding = bs_get1(bs);	// rounding_type
				//DEBUG1("rounding", *rounding);
			}

			/* if (reduced_resolution_enable)
			{
			}
			*/

			if (dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
			{
				uint32_t width, height;
				uint32_t horiz_mc_ref, vert_mc_ref;
				
				width = bs_get(bs, 13);
				MARKER();
				height = bs_get(bs, 13);
				MARKER();
				horiz_mc_ref = bs_get(bs, 13);
				MARKER();
				vert_mc_ref = bs_get(bs, 13);
				MARKER();

				// DEBUG2("vop_width/height", width, height);
				// DEBUG2("ref             ", horiz_mc_ref, vert_mc_ref);

				bs_skip(bs, 1);				// change_conv_ratio_disable
				if (bs_get1(bs))			// vop_constant_alpha
				{
					bs_skip(bs, 8);			// vop_constant_alpha_value
				}
			}
				

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
			{
				// intra_dc_vlc_threshold
				*intra_dc_threshold = intra_dc_threshold_table[ bs_get(bs,3) ];

				/* if (interlaced)
					{
						bs_skip(bs, 1);		// top_field_first
						bs_skip(bs, 1);		// alternative_vertical_scan_flag
				*/
			}
						
			*quant = bs_get(bs, dec->quant_bits);		// vop_quant
			//DEBUG1("quant", *quant);
						
			if (coding_type != I_VOP)
			{
				*fcode = bs_get(bs, 3);			// fcode_forward
			}
				
			if (coding_type == B_VOP)
			{
				// *fcode_backward = bs_get(bs, 3);		// fcode_backward
			}
			return coding_type;
		}
		else if (start_code == USERDATA_START_CODE)
		{
			// DEBUG("user_data");
			bs_skip(bs, 32);		// user_data_start_code
		}
		else  // start_code == ?
		{
			if (bs_show(bs, 24) == 0x000001)
			{
				DEBUG1("*** WARNING: unknown start_code", bs_show(bs, 32));
			}
			bs_skip(bs, 8);
		}
	}
	while (bs_length(bs) < bs->length);

	DEBUG("*** WARNING: no vop_start_code found");
	return -1; /* ignore it */
}
