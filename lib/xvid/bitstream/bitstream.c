 /******************************************************************************
  *                                                                            *
  *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
  *                                                                            *
  *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
  *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
  *  software module in hardware or software products are advised that its     *
  *  use may infringe existing patents or copyrights, and any such use         *
  *  would be at such party's own risk.  The original developer of this        *
  *  software module and his/her company, and subsequent editors and their     *
  *  companies, will have no liability for use of this software or             *
  *  modifications or derivatives thereof.                                     *
  *                                                                            *
  *  XviD is free software; you can redistribute it and/or modify it           *
  *  under the terms of the GNU General Public License as published by         *
  *  the Free Software Foundation; either version 2 of the License, or         *
  *  (at your option) any later version.                                       *
  *                                                                            *
  *  XviD is distributed in the hope that it will be useful, but               *
  *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
  *  GNU General Public License for more details.                              *
  *                                                                            *
  *  You should have received a copy of the GNU General Public License         *
  *  along with this program; if not, write to the Free Software               *
  *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *                                                                            *
  *  bitstream.c                                                               *
  *                                                                            *
  *  Copyright (C) 2001 - Peter Ross <pross@cs.rmit.edu.au>                    *
  *                                                                            *
  *  For more information visit the XviD homepage: http://www.xvid.org         *
  *                                                                            *
  ******************************************************************************/

 /******************************************************************************
  *																			   *	
  *  Revision history:                                                         *
  *                                                                            *
  *  26.03.2002 interlacing support
  *  03.03.2002 qmatrix writing												   *
  *  03.03.2002 merged BITREADER and BITWRITER								   *
  *	 30.02.2002	intra_dc_threshold support									   *
  *	 04.12.2001	support for additional headers								   *
  *	 16.12.2001	inital version                                           	   *
  *																			   *
  ******************************************************************************/


#include "bitstream.h"
#include "zigzag.h"
#include "../quant/quant_matrix.h"

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


void bs_get_matrix(Bitstream * bs, uint8_t * matrix) 
{ 
	int i = 0; 
    int last, value = 0; 
    
    do 
	{ 
		last = value; 
        value = BitstreamGetBits(bs, 8); 
        matrix[ scan_tables[0][i++] ]  = value; 
    } 
    while (value != 0 && i < 64); 
    
	while (i < 64) 
	{ 
		matrix[ scan_tables[0][i++] ]  = last; 
	} 
} 

/*
decode headers
returns coding_type, or -1 if error
*/

int BitstreamReadHeaders(Bitstream * bs, DECODER * dec, uint32_t * rounding, 
			 uint32_t * quant, uint32_t * fcode, uint32_t * intra_dc_threshold,
			 int findvol)
{
	uint32_t vol_ver_id;
	uint32_t time_inc_resolution;
	uint32_t coding_type;
	uint32_t start_code;
	
	do
	{
		BitstreamByteAlign(bs);
		start_code = BitstreamShowBits(bs, 32);

		if (start_code == VISOBJSEQ_START_CODE)
		{
			// DEBUG("visual_object_sequence");
			BitstreamSkip(bs, 32);				// visual_object_sequence_start_code
			BitstreamSkip(bs, 8);					// profile_and_level_indication
		}
		else if (start_code == VISOBJSEQ_STOP_CODE)
		{
			BitstreamSkip(bs, 32);				// visual_object_sequence_stop_code
		}
		else if (start_code == VISOBJ_START_CODE)
		{
			// DEBUG("visual_object");
			BitstreamSkip(bs,32);					// visual_object_start_code
			if (BitstreamGetBit(bs))				// is_visual_object_identified
			{
				vol_ver_id = BitstreamGetBits(bs,4);	// visual_object_ver_id
				BitstreamSkip(bs, 3);				// visual_object_priority
			}
			else
			{
				vol_ver_id = 1;
			}

			if (BitstreamShowBits(bs, 4) != VISOBJ_TYPE_VIDEO)	// visual_object_type
			{
				DEBUG("visual_object_type != video");
				return -1;
			}
			BitstreamSkip(bs, 4);

			// video_signal_type

			if (BitstreamGetBit(bs))				// video_signal_type
			{
				DEBUG("+ video_signal_type");
				BitstreamSkip(bs, 3);				// video_format
				BitstreamSkip(bs, 1);				// video_range
				if (BitstreamGetBit(bs))			// color_description
				{
					DEBUG("+ color_description");
					BitstreamSkip(bs, 8);			// color_primaries
					BitstreamSkip(bs, 8);			// transfer_characteristics
					BitstreamSkip(bs, 8);			// matrix_coefficients
				}
			}
		}
		else if ((start_code & ~0x1f) == VIDOBJ_START_CODE)
		{
			BitstreamSkip(bs, 32);		// video_object_start_code
		} 
		else if ((start_code & ~0xf) == VIDOBJLAY_START_CODE)
		{
			// DEBUG("video_object_layer");
			BitstreamSkip(bs, 32);					// video_object_layer_start_code

			BitstreamSkip(bs, 1);									// random_accessible_vol

			// video_object_type_indication
			if (BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_SIMPLE &&
				BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_CORE &&
				BitstreamShowBits(bs, 8) != VIDOBJLAY_TYPE_MAIN &&
				BitstreamShowBits(bs, 8) != 0)		// BUGGY DIVX
			{
				DEBUG1("video_object_type_indication not supported", BitstreamShowBits(bs, 8));
				return -1;
			}
			BitstreamSkip(bs, 8);


			if (BitstreamGetBit(bs))					// is_object_layer_identifier
			{
				DEBUG("+ is_object_layer_identifier");
				vol_ver_id = BitstreamGetBits(bs,4);		// video_object_layer_verid
				BitstreamSkip(bs, 3);					// video_object_layer_priority
			}
			else
			{
				vol_ver_id = 1;
			}
			//DEBUGI("vol_ver_id", vol_ver_id);

			if (BitstreamGetBits(bs, 4) == VIDOBJLAY_AR_EXTPAR)	// aspect_ratio_info
			{
				DEBUG("+ aspect_ratio_info");
				BitstreamSkip(bs, 8);						// par_width
				BitstreamSkip(bs, 8);						// par_height
			}

			if (BitstreamGetBit(bs))		// vol_control_parameters
			{
				DEBUG("+ vol_control_parameters");
				BitstreamSkip(bs, 2);						// chroma_format
				BitstreamSkip(bs, 1);						// low_delay
				if (BitstreamGetBit(bs))					// vbv_parameters
				{
					DEBUG("+ vbv_parameters");
					BitstreamSkip(bs, 15);				// first_half_bitrate
					READ_MARKER();
					BitstreamSkip(bs, 15);				// latter_half_bitrate
					READ_MARKER();
					BitstreamSkip(bs, 15);				// first_half_vbv_buffer_size
					READ_MARKER();
					BitstreamSkip(bs, 3);					// latter_half_vbv_buffer_size
					BitstreamSkip(bs, 11);				// first_half_vbv_occupancy
					READ_MARKER();
					BitstreamSkip(bs, 15);				// latter_half_vbv_occupancy
					READ_MARKER();
				
				}
			}

			dec->shape = BitstreamGetBits(bs, 2);	// video_object_layer_shape
			// DEBUG1("shape", dec->shape);
			
			if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE && vol_ver_id != 1)
			{
				BitstreamSkip(bs, 4);		// video_object_layer_shape_extension
			}

			READ_MARKER();

			time_inc_resolution = BitstreamGetBits(bs, 16);	// vop_time_increment_resolution
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

			READ_MARKER();

			if (BitstreamGetBit(bs))						// fixed_vop_rate
			{
				BitstreamSkip(bs, dec->time_inc_bits);	// fixed_vop_time_increment
			}

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
			{

				if (dec->shape == VIDOBJLAY_SHAPE_RECTANGULAR)
				{
					uint32_t width, height;

					READ_MARKER();
					width = BitstreamGetBits(bs, 13);			// video_object_layer_width
					//DEBUGI("width", width);
					READ_MARKER();
					height = BitstreamGetBits(bs, 13);		// video_object_layer_height
					//DEBUGI("height", height);	
					READ_MARKER();

					if (findvol == 0) {
					  if (width != dec->width || height != dec->height)
					    {
					      DEBUG("FATAL: video dimension discrepancy ***");
					      DEBUG2("bitstream width/height", width, height);
					      DEBUG2("param width/height", dec->width, dec->height);
					      return -1;
					    }
					} else {
					  dec->width = width;
					  dec->height = height;
					}

				}

				if ((dec->interlacing = BitstreamGetBit(bs)))
				{
					DEBUG("vol: interlacing");
				}

				if (!BitstreamGetBit(bs))				// obmc_disable
				{
					DEBUG("IGNORED/TODO: !obmc_disable");
					// TODO
					// fucking divx4.02 has this enabled
				}

				if (BitstreamGetBits(bs, (vol_ver_id == 1 ? 1 : 2)))  // sprite_enable
				{
					DEBUG("sprite_enable; not supported");
					return -1;
				}
			
				if (vol_ver_id != 1 && dec->shape != VIDOBJLAY_SHAPE_RECTANGULAR)
				{
					BitstreamSkip(bs, 1);					// sadct_disable
				}

				if (BitstreamGetBit(bs))						// not_8_bit
				{
					DEBUG("+ not_8_bit [IGNORED/TODO]");
					dec->quant_bits = BitstreamGetBits(bs, 4);	// quant_precision
					BitstreamSkip(bs, 4);						// bits_per_pixel
				}
				else
				{
					dec->quant_bits = 5;
				}

				if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE)
				{
					BitstreamSkip(bs, 1);			// no_gray_quant_update
					BitstreamSkip(bs, 1);			// composition_method
					BitstreamSkip(bs, 1);			// linear_composition
				}

				dec->quant_type = BitstreamGetBit(bs);		// quant_type
				// DEBUG1("**** quant_type", dec->quant_type);

				if (dec->quant_type)
				{
					if (BitstreamGetBit(bs))		// load_intra_quant_mat
					{
						uint8_t matrix[64];
						bs_get_matrix(bs, matrix);
						set_intra_matrix(matrix);
					}
					else
						set_intra_matrix(get_default_intra_matrix());

					if (BitstreamGetBit(bs))		// load_inter_quant_mat
					{
						uint8_t matrix[64]; 
						bs_get_matrix(bs, matrix);
						set_inter_matrix(matrix);
					}
					else
						set_inter_matrix(get_default_inter_matrix());

					if (dec->shape == VIDOBJLAY_SHAPE_GRAYSCALE)
					{
						// TODO
						DEBUG("TODO: grayscale matrix stuff");
						return -1;
					}

				}

			
				if (vol_ver_id != 1)
				{
					dec->quarterpel = BitstreamGetBit(bs);	// quarter_sampe
					if (dec->quarterpel)
					{
						DEBUG("IGNORED/TODO: quarter_sample");
					}
				}
				else
				{
					dec->quarterpel = 0;
				}

				if (!BitstreamGetBit(bs))			// complexity_estimation_disable
				{
					DEBUG("TODO: complexity_estimation header");
					// TODO
					return -1;
				}

				if (!BitstreamGetBit(bs))			// resync_marker_disable
				{
					DEBUG("IGNORED/TODO: !resync_marker_disable");
					// TODO
				}

				if (BitstreamGetBit(bs))		// data_partitioned
				{
					DEBUG("+ data_partitioned");
					BitstreamSkip(bs, 1);		// reversible_vlc
				}

				if (vol_ver_id != 1)
				{
					if (BitstreamGetBit(bs))			// newpred_enable
					{
						DEBUG("+ newpred_enable");
						BitstreamSkip(bs, 2);			// requested_upstream_message_type
						BitstreamSkip(bs, 1);			// newpred_segment_type
					}
					if (BitstreamGetBit(bs))			// reduced_resolution_vop_enable
					{
						DEBUG("TODO: reduced_resolution_vop_enable");
						// TODO
						return -1;
					}
				}
				
				if (BitstreamGetBit(bs))	// scalability
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
					if (BitstreamGetBit(bs))	// scalability
					{
						// TODO
						DEBUG("TODO: scalability");
						return -1;
					}
				}
				BitstreamSkip(bs, 1);			// resync_marker_disable

			}
			if (findvol != 0) return 1;
		}
		else if (start_code == GRPOFVOP_START_CODE)
		{
			// DEBUG("group_of_vop");
			BitstreamSkip(bs, 32);
			{
				int hours, minutes, seconds;
				hours = BitstreamGetBits(bs, 5);
				minutes = BitstreamGetBits(bs, 6);
				READ_MARKER();
				seconds = BitstreamGetBits(bs, 6);
				// DEBUG3("hms", hours, minutes, seconds);
			}
			BitstreamSkip(bs, 1);			// closed_gov
			BitstreamSkip(bs, 1);			// broken_link
		}
		else if (start_code == VOP_START_CODE)
		{
		  if (findvol != 0) return -1;
			// DEBUG("vop_start_code");
			BitstreamSkip(bs, 32);						// vop_start_code

			coding_type = BitstreamGetBits(bs, 2);		// vop_coding_type
			//DEBUG1("coding_type", coding_type);

			while (BitstreamGetBit(bs) != 0) ;			// time_base
	
			READ_MARKER();
	 
			//DEBUG1("time_inc_bits", dec->time_inc_bits);
			//DEBUG1("vop_time_incr", BitstreamShowBits(bs, dec->time_inc_bits));
			if (dec->time_inc_bits)
			{
				BitstreamSkip(bs, dec->time_inc_bits);	// vop_time_increment
			}

			READ_MARKER();

			if (!BitstreamGetBit(bs))					// vop_coded
			{
				return N_VOP;
			}

			/* if (newpred_enable)
			{
			}
			*/

			if (coding_type != I_VOP)
			{
				*rounding = BitstreamGetBit(bs);	// rounding_type
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
				
				width = BitstreamGetBits(bs, 13);
				READ_MARKER();
				height = BitstreamGetBits(bs, 13);
				READ_MARKER();
				horiz_mc_ref = BitstreamGetBits(bs, 13);
				READ_MARKER();
				vert_mc_ref = BitstreamGetBits(bs, 13);
				READ_MARKER();

				// DEBUG2("vop_width/height", width, height);
				// DEBUG2("ref             ", horiz_mc_ref, vert_mc_ref);

				BitstreamSkip(bs, 1);				// change_conv_ratio_disable
				if (BitstreamGetBit(bs))			// vop_constant_alpha
				{
					BitstreamSkip(bs, 8);			// vop_constant_alpha_value
				}
			}
				

			if (dec->shape != VIDOBJLAY_SHAPE_BINARY_ONLY)
			{
				// intra_dc_vlc_threshold
				*intra_dc_threshold = intra_dc_threshold_table[ BitstreamGetBits(bs,3) ];

				if (dec->interlacing)
				{
					if ((dec->top_field_first = BitstreamGetBit(bs)))
					{
						DEBUG("vop: top_field_first");
					}
					if ((dec->alternate_vertical_scan = BitstreamGetBit(bs)))
					{
						DEBUG("vop: alternate_vertical_scan");
					}
				}
			}
						
			*quant = BitstreamGetBits(bs, dec->quant_bits);		// vop_quant
			//DEBUG1("quant", *quant);
						
			if (coding_type != I_VOP)
			{
				*fcode = BitstreamGetBits(bs, 3);			// fcode_forward
			}
				
			if (coding_type == B_VOP)
			{
				// *fcode_backward = BitstreamGetBits(bs, 3);		// fcode_backward
			}
			return coding_type;
		}
		else if (start_code == USERDATA_START_CODE)
		{
			// DEBUG("user_data");
			BitstreamSkip(bs, 32);		// user_data_start_code
		}
		else  // start_code == ?
		{
			if (BitstreamShowBits(bs, 24) == 0x000001)
			{
				DEBUG1("*** WARNING: unknown start_code", BitstreamShowBits(bs, 32));
			}
			BitstreamSkip(bs, 8);
		}
	}
	while ((BitstreamPos(bs) >> 3) < bs->length);

	DEBUG("*** WARNING: no vop_start_code found");
	if (findvol != 0) return 0;
	return -1; /* ignore it */
}


/* write custom quant matrix */

static void bs_put_matrix(Bitstream * bs, const int16_t *matrix)
{
	int i, j;
	const int last = matrix[scan_tables[0][63]];

	for (j = 63; j >= 0 && matrix[scan_tables[0][j - 1]] == last; j--) ;

	for (i = 0; i <= j; i++)
	{
		BitstreamPutBits(bs, matrix[scan_tables[0][i]], 8);
	}

	if (j < 63)
	{
		BitstreamPutBits(bs, 0, 8);
	}
}


/*
	write vol header
*/
void BitstreamWriteVolHeader(Bitstream * const bs,
						const MBParam * pParam)
{
	// video object_start_code & vo_id
    BitstreamPad(bs);
	BitstreamPutBits(bs, VO_START_CODE, 27);
    BitstreamPutBits(bs, 0, 5);

	// video_object_layer_start_code & vol_id
	BitstreamPutBits(bs, VOL_START_CODE, 28);
	BitstreamPutBits(bs, 0, 4);

	BitstreamPutBit(bs, 0);				// random_accessible_vol
	BitstreamPutBits(bs, 0, 8);			// video_object_type_indication
	BitstreamPutBit(bs, 0);				// is_object_layer_identified (0=not given)
	BitstreamPutBits(bs, 1, 4);			// aspect_ratio_info (1=1:1)
	BitstreamPutBit(bs, 0);				// vol_control_parameters (0=not given)
	BitstreamPutBits(bs, 0, 2);			// video_object_layer_shape (0=rectangular)

	WRITE_MARKER();
    
	/* time_increment_resolution; ignored by current decore versions
		eg. 2fps		res=2		inc=1
			25fps		res=25		inc=1
			29.97fps	res=30000	inc=1001
	*/
#ifdef MPEG4IP
	BitstreamPutBits(bs, pParam->fbase, 16);
#else
	BitstreamPutBits(bs, 2, 16);
#endif

	WRITE_MARKER();

	// fixed_vop_rate
	BitstreamPutBit(bs, 0);

	// fixed_time_increment: value=nth_of_sec, nbits = log2(resolution)
	// BitstreamPutBits(bs, 0, 15);

	WRITE_MARKER();
	BitstreamPutBits(bs, pParam->width, 13);		// width
	WRITE_MARKER();
	BitstreamPutBits(bs, pParam->height, 13);		// height
	WRITE_MARKER();
	
	BitstreamPutBit(bs, pParam->global_flags & XVID_INTERLACING);		// interlace
	BitstreamPutBit(bs, 1);		// obmc_disable (overlapped block motion compensation)
	BitstreamPutBit(bs, 0);		// sprite_enable
	BitstreamPutBit(bs, 0);		// not_in_bit

	// quant_type   0=h.263  1=mpeg4(quantizer tables)
	BitstreamPutBit(bs, pParam->quant_type);
	
	if (pParam->quant_type)
	{
		BitstreamPutBit(bs, get_intra_matrix_status());	// load_intra_quant_mat
		if (get_intra_matrix_status())
		{
			bs_put_matrix(bs, get_intra_matrix());
		}

		BitstreamPutBit(bs, get_inter_matrix_status());		// load_inter_quant_mat
		if (get_inter_matrix_status())
		{
			bs_put_matrix(bs, get_inter_matrix());
		}

	}

	BitstreamPutBit(bs, 1);		// complexity_estimation_disable
	BitstreamPutBit(bs, 1);		// resync_marker_disable
	BitstreamPutBit(bs, 0);		// data_partitioned
	BitstreamPutBit(bs, 0);		// scalability
}


/*
  write vop header

  NOTE: doesnt handle bother with time_base & time_inc
  time_base = n seconds since last resync (eg. last iframe)
  time_inc = nth of a second since last resync
  (decoder uses these values to determine precise time since last resync)
*/
void BitstreamWriteVopHeader(Bitstream * const bs,
						const MBParam * pParam)
{
    BitstreamPad(bs);
    BitstreamPutBits(bs, VOP_START_CODE, 32);

    BitstreamPutBits(bs, pParam->coding_type, 2);
    
	// time_base = 0  write n x PutBit(1), PutBit(0)
	BitstreamPutBits(bs, 0, 1);

	WRITE_MARKER();

	// time_increment: value=nth_of_sec, nbits = log2(resolution)
#ifdef MPEG4IP
	BitstreamPutBits(bs, pParam->fincr, pParam->time_inc_bits);		
#else
	BitstreamPutBits(bs, 1, 1);
#endif

	WRITE_MARKER();

	BitstreamPutBits(bs, 1, 1);				// vop_coded

	if (pParam->coding_type != I_VOP)
		BitstreamPutBits(bs, pParam->rounding_type, 1);
    
	BitstreamPutBits(bs, 0, 3);				// intra_dc_vlc_threshold

	if (pParam->global_flags & XVID_INTERLACING)
	{
		BitstreamPutBit(bs, 1);		// top field first
		BitstreamPutBit(bs, 0);		// alternate vertical scan
	}

 	BitstreamPutBits(bs, pParam->quant, 5);			// quantizer
	
	if (pParam->coding_type != I_VOP)
		BitstreamPutBits(bs, pParam->fixed_code, 3);		// fixed_code = [1,4]
}
