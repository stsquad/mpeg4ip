
/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
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
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *
 *  rc_hist.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions to record and access the history     */
/* information for rate control functions.                                */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if !defined(WIN32)
#  include <unistd.h>
#endif

#include <ctype.h>

#include "momusys.h"
#include "mom_structs.h"
#include "vm_enc_defs.h"

#include "rc.h"

/***********************************************************CommentBegin******
 *
 * -- rch_init --
 *
 * Purpose :
 *      Initialization of history data
 *
 * Arguments in :
 *      RC_HIST  *rch    -  History data
 *      Int      size    -  Size of history
 *
 ***********************************************************CommentEnd********/

void rch_init(
RC_HIST *rch,
Int size
)
{
	Int i;
	for(i=0; i<3; i++)
	{
		rch[i].size=size;
		rch[i].n=0;
		rch[i].prev=0;
		rch[i].ptr=0;
//		rch[i].proc_stat=0;

		rch[i].bits_text=(Int *)calloc(sizeof(Int),size);
		rch[i].pixels=(Int *)calloc(sizeof(Int),size);
		rch[i].mad_text=(Double *)calloc(sizeof(Double),size);
		rch[i].dist=(Double *)calloc(sizeof(Double),size);
		rch[i].bits_vop=(Int *)calloc(sizeof(Int),size);
		rch[i].qp=(Int *)calloc(sizeof(Int),size);

//		rch[i].total_dist=0;
//		rch[i].total_bits_text=0;
//		rch[i].total_bits=0;
//		rch[i].total_frames=0;
//		rch[i].last_pred_bits_text=0;

		#ifdef _RC_DEBUG_
		rch_print(&rch[i], stdout, "RC: RCH_INIT");
		#endif
	}
}


/***********************************************************CommentBegin******
 *
 * -- rch_free --
 *
 * Purpose :
 *      Free reserved memory for history
 *
 * Arguments in :
 *      RC_HIST  *rch    -  History data
 *
 ***********************************************************CommentEnd********/

Void rch_free(
RC_HIST *rch
)

{
	Int i;

	for(i=0; i<3; i++)
	{
		free(rch[i].bits_text);
		free(rch[i].pixels);
		free(rch[i].mad_text);
		free(rch[i].dist);
		free(rch[i].bits_vop);
		free(rch[i].qp);
	}

}


/***********************************************************CommentBegin******
 *
 * -- rch_store_before --
 *
 * Purpose :
 *      Storing of data of a frame available BEFORE its encoding
 *      but after obtaining its prediction error.
 *
 * Arguments in :
 *      RC_HIST  *rch    -  History data
 *      Int      pixels  -  current number of pixels to store
 *      Double   mad     -  current mad to store
 *      Int      qp      -  current quantization parameter to store
 *
 ***********************************************************CommentEnd********/

void rch_store_before(
RC_HIST *rch,
Int      pixels,								  /* Num. pixels of VOP */
Double   mad,									  /* MAD of prediction error */
Int      qp
)
{

	rch->pixels[rch->ptr]  = pixels;
	rch->mad_text[rch->ptr]= mad;
	rch->qp[rch->ptr]      = qp;

	rch->prev=1;

}


/***********************************************************CommentBegin******
 *
 * -- rch_store_after --
 *
 * Purpose :
 *      Storing of data of a frame available AFTER its encoding
 *
 * Arguments in :
 *      RC_HIST  *rch       -  History data
 *      Int      bits_text  -  number of pixels for texture
 *      Int      bits_vop   -  number of pixels for vop
 *      Double   dist       -  obtained distortion value
 *
 ***********************************************************CommentEnd********/

void rch_store_after(
RC_HIST      *rch,
Int          bits_text,
Int          bits_vop,
Double       dist
)
{

	/* Save only for actual frame type: */

	if (rch->mad_text[rch->ptr] != 0.) {
		rch->bits_text[rch->ptr] = bits_text;
		rch->bits_vop[rch->ptr]  = bits_vop;
		rch->dist[rch->ptr]      = dist;

	//	rch->total_dist      += dist;
	//	rch->total_bits_text += bits_text;
	//	rch->total_bits      += bits_vop;
	//	rch->total_frames++;

		rch->ptr++;

		if (rch->ptr == rch->size)
			rch->ptr = 0;
		if (rch->n < rch->size)
			rch->n++;
	}

	rch->prev=0;

}


/***********************************************************CommentBegin******
 *
 * -- Auxiliary functions (no modification of data structure) --
 *
 ***********************************************************CommentEnd********/

/***********************************************************CommentBegin******
 *
 * -- rch_get_last_qp ---
 *
 * Purpose :
 *      Obtainment of last QP
 *
 * Arguments in :
 *      RC_HIST  *rch       -  History data
 *
 ***********************************************************CommentEnd********/

Int rch_get_last_qp(
RC_HIST *rch
)
{

	if (rch->ptr == 0)
		return rch->qp[rch->n-1];
	else
		return rch->qp[rch->ptr-1];
}


/***********************************************************CommentBegin******
 *
 * -- rch_get_bits_vop ---
 *
 * Purpose :
 *      Obtainmemt of total bits of last VOP
 *
 * Arguments in :
 *      RC_HIST  *rch       -  History data
 *
 ***********************************************************CommentEnd********/

Int rch_get_last_bits_vop(
RC_HIST *rch
)
{

	if (rch->ptr == 0)
		return rch->bits_vop[rch->n-1];
	else
		return rch->bits_vop[rch->ptr-1];
}


/***********************************************************CommentBegin******
 *
 * -- rch_get_last_mad_text ---
 *
 * Purpose :
 *      Obtainment of last MAD of texture
 *
 * Arguments in :
 *      RC_HIST  *rch       -  History data
 *
 ***********************************************************CommentEnd********/

Double rch_get_last_mad_text(
RC_HIST *rch
)
{
	if (rch->n == 0)
	{
		return 0;
	}

	if (rch->ptr == 0)
		return rch->mad_text[rch->n-1];
	else
		return rch->mad_text[rch->ptr-1];
}


/***********************************************************CommentBegin******
 *
 * -- rch_get_plast_mad_text ---
 *
 * Purpose :
 *      Obtainment of previous-to-last MAD of texture
 *
 * Arguments in :
 *      RC_HIST  *rch       -  History data
 *
 ***********************************************************CommentEnd********/

Double rch_get_plast_mad_text(
RC_HIST *rch
)
{
	if (rch->ptr == 0)
		return rch->mad_text[rch->n-2];
	else if (rch->ptr == 1)
		return rch->mad_text[rch->n-1];
	else
		return rch->mad_text[rch->ptr-2];
}


/***********************************************************CommentBegin******
 *
 * -- rch_inc_mod ---
 *
 * Purpose :
 *      Module-n number increase
 *
 * Arguments in :
 *      RC_HIST  *rch       -  History data
 *      Int      i          -  module number
 *
 ***********************************************************CommentEnd********/

Int rch_inc_mod(
RC_HIST *rch,
Int i
)
{
	i++;
	if (i==rch->size)
		return 0;
	else
		return i;
}


/***********************************************************CommentBegin******
 *
 * -- rch_dec_mod ---
 *
 * Purpose :
 *      Module-n number decrease
 *
 * Arguments in :
 *      RC_HIST  *rch       -  History data
 *      Int      i          -  module number
 *
 ***********************************************************CommentEnd********/

Int rch_dec_mod(
RC_HIST *rch,
Int i
)
{
	i--;
	if (i==-1)
		return rch->size-1;
	else
		return i;
}

