/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	write vol/vop headers
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
 *	16.11.2001	rewrite; (c)2001 peter ross <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "bitwriter.h"


#define VO_START_CODE	0x8
#define VOL_START_CODE	0x12
#define VOP_START_CODE	0x1b6


#define MARKER()	BitstreamPutBit(bs, 1)


/*
	write vol header
*/
void BitstreamVolHeader(Bitstream * const bs,
						const int width,
						const int height,
						const int quant_type)
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

	MARKER();
    
	/* time_increment_resolution; ignored by current decore versions
		eg. 2fps		res=2		inc=1
			25fps		res=25		inc=1
			29.97fps	res=30000	inc=1001
	*/
	BitstreamPutBits(bs, 2, 16);

	MARKER();

	// fixed_vop_rate
	BitstreamPutBit(bs, 0);

	// fixed_time_increment: value=nth_of_sec, nbits = log2(resolution)
	// BitstreamPutBits(bs, 0, 15);

	MARKER();
	BitstreamPutBits(bs, width, 13);		// width
	MARKER();
	BitstreamPutBits(bs, height, 13);		// height
	MARKER();
	
	BitstreamPutBit(bs, 0);		// interlace
	BitstreamPutBit(bs, 1);		// obmc_disable (overlapped block motion compensation)
	BitstreamPutBit(bs, 0);		// sprite_enable
	BitstreamPutBit(bs, 0);		// not_in_bit

	// quant_type   0=h.263  1=mpeg4(quantizer tables)
	BitstreamPutBit(bs, quant_type);
	if (quant_type)
	{
		BitstreamPutBit(bs, 0);		// load_intra_quant_mat
		BitstreamPutBit(bs, 0);		// load_inter_quant_mat
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
void BitstreamVopHeader(Bitstream * const bs,
			  VOP_TYPE prediction_type,
#ifdef MPEG4IP
			  const int time_incr_bits,
#endif
			  const int rounding_type,
			  const uint32_t quant,
			  const uint32_t fcode)
{
    BitstreamPad(bs);
    BitstreamPutBits(bs, VOP_START_CODE, 32);

    BitstreamPutBits(bs, prediction_type, 2);
    
	// time_base = 0  write n x PutBit(1), PutBit(0)
	BitstreamPutBits(bs, 0, 1);

	MARKER();

	// time_increment: value=nth_of_sec, nbits = log2(resolution)
#ifdef MPEG4IP
	BitstreamPutBits(bs, 1, (time_incr_bits ? time_incr_bits : 1));
#else
	BitstreamPutBits(bs, 1, 1);
#endif

	MARKER();

	BitstreamPutBits(bs, 1, 1);				// vop_coded

	if (prediction_type != I_VOP)
		BitstreamPutBits(bs, rounding_type, 1);
    
	BitstreamPutBits(bs, 0, 3);				// intra_dc_vlc_threshold

 	BitstreamPutBits(bs, quant, 5);			// quantizer
	
	if (prediction_type != I_VOP)
		BitstreamPutBits(bs, fcode, 3);		// fixed_code = [1,4]
}
