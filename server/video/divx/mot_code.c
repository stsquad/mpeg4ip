
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
 *  mot_code.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions to code the motion data of the image.*/
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "vm_common_defs.h"
#include "bitstream.h"

#include "putvlc.h"

/* ------------------------------------------------------------------------- */

/* Specific macros (include references to internal vars. of the functions) */

/* 16x16 MV obtainment */
#define MBV_H(h,v)      (ph[2*(v)*2*hdim+2*(h)])
#define MBV_V(h,v)      (pv[2*(v)*2*hdim+2*(h)])

/* 8x8 MV obtainment */
#define BV_H(h,v,h2,v2) (ph[(2*(v)+(v2))*2*hdim+2*(h)+(h2)])
#define BV_V(h,v,h2,v2) (pv[(2*(v)+(v2))*2*hdim+2*(h)+(h2)])

/* MB mode obtainment */
#define MB_MODE(h,v)    ( ((h)<0||(h)>=hdim||(v)<0||(v)>=vdim ? MBM_OUT : (pm[(v)*hdim+(h)])) )

/* get 8x8 MV component */
#define BV(p,xdim,h,v,h2,v2) (p[(2*(v)+(v2))*(xdim)+2*(h)+(h2)])

/* ------------------------------------------------------------------------- */


/* declaration for functions in this file */
Int WriteMVcomponent(Int f_code, Int dmv, Image *bs);
Void ScaleMVD (Int f_code, Int diff_vector, Int *residual, Int *vlc_code_mag);
Void  	find_pmvs (Image *mot_x, Image *mot_y, Image *MB_decisions, Image *B_decisions,
	Int x, Int y, Int block, Int transparent_value, Int quarter_pel, Int *error_flag,
	Int *mvx, Int *mvy, Int newgob);
SInt ModeMB (Image *MB_decision, Int i, Int j);


/***********************************************************CommentBegin******
 *
 * -- Bits_CountMB_Motion --
 *
 * Purpose :
 *      Encodes the MV's Images acording to modes and alpha Images
 *      (see notes below). The codewords are appended to the bitstream.
 *      Included in text_code.h .
 *
 * Return values :
 *      Int     mv_bits   Returns the number of bits sent to the bitstream
 *
 * Description :
 *      1) No checking is made for the consistence of image sizes
 *      2) It assumes the output image has been previously allocated.
 *      3) Transparent MB's are not coded (no codeword is transmited).
 *      4) Motion vectors for 8x8 transparent blocks within
 *         non-totally-transparent MB's are transmitted as MV (0,0) (not MVD
 *         (0,0)). This is made in the  hope that this MV's are neither
 *         employed for block reconstruction nor for MV prediction.
 *
 ***********************************************************CommentEnd********/

Int
Bits_CountMB_Motion(
Image   *mot_h,									  /* <-- motion vectors (Float) - per block    */
Image   *mot_v,									  /* <-- motion vectors (Float) - per block    */
Image   *alpha,									  /* <-- macroblocks modes (SInt) - per block  */
Image   *modes,									  /* <-- macroblocks modes (SInt) - per MB     */
Int     h,										  /* <-- horizontal coordinate of the MB       */
Int     v,										  /* <-- vertical coordinate of the MB         */
Int     f_code,									  /* <-- MV range in 1/2 or 1/4 pel units 1=32,2=64,...,7=2048 */
												  /* <-- flag for quarter pel MC mode     */
Int     quarter_pel,       /* MW QPEL 07-JUL-1998 */
Image   *bs,									  /* --> output (SInt)                         */
Int     error_res_disable,
Int     after_marker,
Int     **slice_nb,
Int arbitrary_shape
)
{
	Int     vdim, hdim;							  /* Dimensions in macroblocks */
		Float   *ph, *pv;						  /* Motion vectors            */
		SInt    *pm;							  /* Modes                     */
		SInt    *pa;							  /* Alpha                     */
		Int     mode;
		Int     bits_mot = 0;

	/* From switch statement */
		Int     i, error_flag=0,mvx=0,mvy=0;
		Float   pred_h, pred_v;
		Float   diff_h, diff_v;
		Int     bh, bv;
		Int     local_f_code;					  /* MW QPEL 07-JUL-1998 */
		Float   subdim;							  /* MW QPEL 07-JUL-1998 */

		vdim = (Int)modes->y;
		hdim = (Int)modes->x;
		ph=(Float*)GetImageData(mot_h);
		pv=(Float*)GetImageData(mot_v);
		pm=(SInt*)GetImageData(modes);
		pa=NULL;//(SInt*)GetImageData(alpha);


	/* MW QPEL 07-JUL-1998 >> */
	/* Set local_f_code and subdim according to quarter_pel */
		if (quarter_pel)
	{
		local_f_code = f_code+1;
			subdim = 4.0;
	}
	else
	{
		local_f_code = f_code;
			subdim = 2.0;
	}
	/* << MW QPEL 07-JUL-1998 */

	switch (mode=MB_MODE(h,v))
	{
		case MBM_INTRA:
		break;

		case MBM_INTER16:
		/* Prediction obtainment */
		find_pmvs(mot_h,mot_v,modes,alpha,h,v,0,MBM_TRANSPARENT,
									  /* MW QPEL 07-JUL-1998 */
		quarter_pel,&error_flag,&mvx,&mvy,0);

		pred_h=((Float)mvx)/subdim;			  /* MW QPEL 07-JUL-1998 */
		pred_v=((Float)mvy)/subdim;		  /* MW QPEL 07-JUL-1998 */

										  /* MW QPEL 07-JUL-1998 */
		bits_mot += WriteMVcomponent(local_f_code, (Int)(subdim*(MBV_H(h,v) - pred_h)), bs);
										  /* MW QPEL 07-JUL-1998 */
		bits_mot += WriteMVcomponent(local_f_code, (Int)(subdim*(MBV_V(h,v) - pred_v)), bs);

		break;

		case MBM_INTER8:
		i=1;
		for (bv=0; bv<=1; bv++)
			for (bh=0; bh<=1; bh++)
		{
			find_pmvs(mot_h,mot_v,modes,alpha,h,v,i,MBM_TRANSPARENT,
								  /* MW QPEL 07-JUL-1998 */
			quarter_pel,&error_flag,&mvx,&mvy,0);

			pred_h=((Float)mvx)/subdim;		  /* MW QPEL 07-JUL-1998 */
			pred_v=((Float)mvy)/subdim;	  /* MW QPEL 07-JUL-1998 */

			i++;

			diff_h=BV_H(h,v,bh,bv)-pred_h;
			diff_v=BV_V(h,v,bh,bv)-pred_v;

									  /* MW QPEL 07-JUL-1998 */
			bits_mot += WriteMVcomponent(local_f_code, (Int)(subdim*diff_h), bs);
									  /* MW QPEL 07-JUL-1998 */
			bits_mot += WriteMVcomponent(local_f_code, (Int)(subdim*diff_v), bs);
		}
		break;
	}

	return bits_mot;
}


/***********************************************************CommentBegin******
 *
 * -- WriteMVcomponent -- Encodes a single motion vector component
 *
 * Purpose :
 *      Scales the motion vector difference, VLC the most significant part,
 *      then outputs the residual (least significant part) as a FLC.
 *
 * Arguments in :
 *      Int   f_code,         Range for MV, (1/2/3) => (32/64/128) 1/2 units
 *      Float diff_vector,    MV Diff. component (in 1/2 units (in field))
 *      Image bs              Bitstream output
 *
 * Return values :
 *      The number of bits for this encoding
 *
 * Side effects :
 *      The encoded motion vector is added to the bitstream
 *
 ***********************************************************CommentEnd********/

Int
WriteMVcomponent(
	Int     f_code,
	Int     dmv,
	Image   *bs
)
{
	Int   residual, vlc_code_mag, bits, entry;

		ScaleMVD(f_code, dmv, &residual, &vlc_code_mag);

		if (vlc_code_mag < 0)
		entry = vlc_code_mag + 65;
		else
		entry = vlc_code_mag;

		bits = PutMV (entry, bs);

		if ((f_code != 1) && (vlc_code_mag != 0))
	{
		BitstreamPutBits(bs, residual, f_code-1);
			bits += f_code - 1;
	}
	return(bits);
}


/***********************************************************CommentBegin******
 *
 * -- ScaleMVD -- Scales MVD component acording to the MV range
 *
 * Purpose :
 *      Scales a Motion Vector Difference (MVD) component (x or y) according
 *      to the MV range. The maximum range that can be represented is
 *      determined by the f_code encoded in the VOP header. Two values,
 *      vlc_code_mag and residual, are generated.
 *
 * Description :
 *      1) The range of the MV's is computed according to the f_code.
 *      2) The range of the MVD component is reduced to fall in the
 *         correct range.
 *      3) Two values are generated:
 *         vlc_code_mag: It will be VLC coded in other function.
 *         residual    : It will be FLC coded in other function.
 *
 ***********************************************************CommentEnd********/

Void
ScaleMVD (
	Int  f_code,									  /* <-- MV range in 1/2 units: 1=32,2=64,...,7=2048     */
	Int  diff_vector,								  /* <-- MV Difference commponent in 1/2 units           */
	Int  *residual,									  /* --> value to be FLC coded                           */
	Int  *vlc_code_mag								  /* --> value to be VLC coded                           */
)
{
	Int   range;
		Int   scale_factor;
		Int   r_size;
		Int   low;
		Int   high;
		Int   aux;

		r_size = f_code-1;
		scale_factor = 1<<r_size;
		range = 32*scale_factor;
		low   = -range;
		high  =  range-1;
		if (diff_vector < low)
	{
		diff_vector += 2*range;
	}
	else if (diff_vector > high)
	{
		diff_vector -= 2*range;
	}

	if (diff_vector==0)
	{
		*vlc_code_mag = 0;
			*residual = 0;
	}
	else if (scale_factor==1)
	{
		*vlc_code_mag = diff_vector;
			*residual = 0;
	}
	else
	{
		aux = ABS(diff_vector) + scale_factor - 1;
			*vlc_code_mag = aux>>r_size;

			if (diff_vector<0)
			*vlc_code_mag = -*vlc_code_mag;

			*residual = aux & (scale_factor-1);
	}
}


/***********************************************************CommentBegin******
 *
 * -- find_pmvs --
 *
 * Purpose :
 *      Makes the motion vectors prediction for block 'block' (0 = whole MB)
 *
 ***********************************************************CommentEnd********/

Void											  /* MVP/Noel */
find_pmvs(
Image  *mot_x,									  /* x-motion vector field                             */
Image  *mot_y,									  /* y-motion vector field                             */
Image  *MB_decisions,							  /* MB modes                                          */
Image  *B_decisions,							  /* field with number of vectors per MB               */
Int    x,										  /* xpos of the MB in multiples of 16 (hor coord)     */
Int    y,										  /* ypos of the MB in multiples of 16 (ver coord)     */
Int    block,									  /* block number (0 if one vector per MB, 1..4 else)  */
Int    transparent_value,						  /* value of the transparency (0=enc, 2=dec)     */
Int    quarter_pel,   /* MW QPEL 06-JUL-1998 */	  /* flag to indicate quarter pel mc                   */
Int    *error_flag,								  /* set if an error occured                      */
Int    *mvx,									  /* hor predicted motion vector [ in half-pixels units ]  */
Int    *mvy,									  /* ver predicted motion vector [ in half-pixels units ]  */
Int    newgob
)
{
	Float   p1x,p2x,p3x;
	Float   p1y,p2y,p3y;
	Int     xin1, xin2, xin3;
	Int     yin1, yin2, yin3;
	Int     vec1, vec2, vec3;
	Int     rule1, rule2, rule3;
	Int     subdim;								  /* MW QPEL 06-JUL-1998 */
	Float   *motxdata = (Float *) GetImageData(mot_x);
	Float   *motydata = (Float *) GetImageData(mot_y);
	Int     xM = GetImageSizeX(mot_x);
	Int xB = xM;
	Int     mb_mode, sum;

	/* MW QPEL 06-JUL-1998 >> */
	if (quarter_pel)
	{
		subdim=4;
	}
	else
	{
		subdim=2;
	}
	/* << MW QPEL 06-JUL-1998 */

	/* In a previous version, a MB vector (block = 0) was predicted the same way
	   as block 1, which is the most likely interpretation of the VM.

	   Therefore, if we have advanced pred. mode, and if all MBs around have
	   only one 16x16 vector each, we chose the appropiate block as if these
	   MBs have 4 vectors.

	   This different prediction affects only 16x16 vectors of MBs with
	   transparent blocks.

	   In the current version, we choose for the 16x16 mode the first
	non-transparent block in the surrounding MBs
	*/

	switch (block)
	{
		case 0:
			vec1 = 1 ; yin1 = y  ; xin1 = x-1;
			vec2 = 2 ; yin2 = y-1; xin2 = x;
			vec3 = 2 ; yin3 = y-1; xin3 = x+1;
			break;
		case 1:
			vec1 = 1 ; yin1 = y  ; xin1 = x-1;
			vec2 = 2 ; yin2 = y-1; xin2 = x;
			vec3 = 2 ; yin3 = y-1; xin3 = x+1;
			break;
		case 2:
			vec1 = 0 ; yin1 = y  ; xin1 = x;
			vec2 = 3 ; yin2 = y-1; xin2 = x;
			vec3 = 2 ; yin3 = y-1; xin3 = x+1;
			break;
		case 3:
			vec1 = 3 ; yin1 = y  ; xin1 = x-1;
			vec2 = 0 ; yin2 = y  ; xin2 = x;
			vec3 = 1 ; yin3 = y  ; xin3 = x;
			break;
		case 4:
			vec1 = 2 ; yin1 = y  ; xin1 = x;
			vec2 = 0 ; yin2 = y  ; xin2 = x;
			vec3 = 1 ; yin3 = y  ; xin3 = x;
			break;
		default:
			printf("Illegal block number in find_pmv (mot_decode.c)");
			*error_flag = 1;
			*mvx=*mvy=0;
			return ;
	}

	if (block==0)
	{
		/* according to the motion encoding, we must choose a first non-transparent
		   block in the surrounding MBs (16-mode)
		 */

		if (x>0 /*&& ValidCandidateMVP(x,y,xin1,yin1,vec1,xB,transparent_value,
			MB_decisions,dcsn_data)*/)
			rule1 = 0;
		else
			rule1 = 1;

		if ((y>0 && newgob==0) /*&& ValidCandidateMVP(x,y,xin2,yin2,vec2,xB,transparent_value,
			MB_decisions,dcsn_data)*/)
			rule2 = 0;
		else
			rule2 = 1;

		if ((x != xB/2 -1) &&
			((y>0 && newgob==0)) /*&& ValidCandidateMVP(x,y,xin3,yin3,vec3,xB,transparent_value,
			MB_decisions,dcsn_data)*/)
			rule3 = 0;
		else
			rule3 = 1;
	}

	else
	{
		/* check borders for single blocks (advanced mode) */
		/* rule 1 */
												  /* left border */
		if (((block == 1 || block == 3) && (x == 0))  /*||
			/* left block/MB is transparent *
			(!ValidCandidateMVP(x,y,xin1,yin1,vec1,xB,transparent_value,
			MB_decisions,dcsn_data))*/)
			rule1 = 1;
		else
			rule1 = 0;

		/* rule 2 */
												  /* top border */
		if (((block == 1 || block == 2) && (y == 0))  /*||
			/* top block/MB is transparent *
			(!ValidCandidateMVP(x,y,xin2,yin2,vec2,xB,transparent_value,
			MB_decisions,dcsn_data))*/)
			rule2 = 1;
		else
			rule2 = 0;

		/* rule 3 */
		if (((block == 1 || block == 2) && (x == xB/2 -1 || y == 0)) /*||
			/* right & top border *
			/* right block/MB is transparent *
			(!ValidCandidateMVP(x,y,xin3,yin3,vec3,xB,transparent_value,
			MB_decisions,dcsn_data))*/)
			rule3 = 1;
		else
			rule3 = 0;

	}

	if (rule1 )
	{
		p1x = p1y = 0;
	}
	else if (((mb_mode = ModeMB(MB_decisions,xin1,yin1)) >= MBM_FIELD00) && (mb_mode <= MBM_FIELD11))
	{
		/* MW QPEL 06-JUL-1998 */
		sum = subdim*(BV(motxdata, xM, xin1, yin1, 0, 0) + BV(motxdata, xM, xin1, yin1, 1, 0));
		p1x = (Float)((sum & 3) ? ((sum | 2) >> 1) : (sum >> 1))/(Float)subdim;
		sum = subdim*(BV(motydata, xM, xin1, yin1, 0, 0) + BV(motydata, xM, xin1, yin1, 1, 0));
		p1y = (Float)((sum & 3) ? ((sum | 2) >> 1) : (sum >> 1))/(Float)subdim;
	}
	else
	{
		p1x = BV(motxdata, xM, xin1, yin1, vec1&0x1, vec1>>1 );
		p1y = BV(motydata, xM, xin1, yin1, vec1&0x1, vec1>>1 );
	}

	if (rule2)
	{
		p2x = p2y = 0 ;
	}
	else if (((mb_mode = ModeMB(MB_decisions,xin2,yin2)) >= MBM_FIELD00) && (mb_mode <= MBM_FIELD11))
	{
		/* MW QPEL 06-JUL-1998 */
		sum = subdim*(BV(motxdata, xM, xin2, yin2, 0, 0) + BV(motxdata, xM, xin2, yin2, 1, 0));
		p2x = (Float)((sum & 3) ? ((sum | 2) >> 1) : (sum >> 1))/(Float)subdim;
		sum = subdim*(BV(motydata, xM, xin2, yin2, 0, 0) + BV(motydata, xM, xin2, yin2, 1, 0));
		p2y = (Float)((sum & 3) ? ((sum | 2) >> 1) : (sum >> 1))/(Float)subdim;
	}
	else
	{
		p2x = BV(motxdata, xM, xin2, yin2, vec2&0x1, vec2>>1 );
		p2y = BV(motydata, xM, xin2, yin2, vec2&0x1, vec2>>1 );
	}

	if (rule3 )
	{
		p3x = p3y =0;
	}
	else if (((mb_mode = ModeMB(MB_decisions,xin3,yin3)) >= MBM_FIELD00) && (mb_mode <= MBM_FIELD11))
	{
		/* MW QPEL 06-JUL-1998 */
		sum = subdim*(BV(motxdata, xM, xin3, yin3, 0, 0) + BV(motxdata, xM, xin3, yin3, 1, 0));
		p3x = (Float)((sum & 3) ? ((sum | 2) >> 1) : (sum >> 1))/(Float)subdim;
		sum = subdim*(BV(motydata, xM, xin3, yin3, 0, 0) + BV(motydata, xM, xin3, yin3, 1, 0));
		p3y = (Float)((sum & 3) ? ((sum | 2) >> 1) : (sum >> 1))/(Float)subdim;
	}
	else
	{
		p3x = BV(motxdata, xM, xin3, yin3, vec3&0x1, vec3>>1 );
		p3y = BV(motydata, xM, xin3, yin3, vec3&0x1, vec3>>1 );
	}

	if (rule1 && rule2 && rule3 )
	{
		/* all MBs are outside the VOP */
		*mvx=*mvy=0;
	}
	else if (rule1+rule2+rule3 == 2)
	{
		/* two of three are zero */
		/* MW QPEL 06-JUL-1998 */
		*mvx=(Int) subdim*(p1x+p2x+p3x);		  /* MW QPEL 06-JUL-1998 */
		*mvy=(Int) subdim*(p1y+p2y+p3y);		  /* MW QPEL 06-JUL-1998 */
	}
	else
	{
		/* MW QPEL 06-JUL-1998 */
												  /* MW QPEL 06-JUL-1998 */
		*mvx=(Int)(subdim*(p1x+p2x+p3x-MAX(p1x,MAX(p2x,p3x))-MIN(p1x,MIN(p2x,p3x))));
												  /* MW QPEL 06-JUL-1998 */
		*mvy=(Int)(subdim*(p1y+p2y+p3y-MAX(p1y,MAX(p2y,p3y))-MIN(p1y,MIN(p2y,p3y))));
	}

	#ifdef _DEBUG_PMVS_
	fprintf(stdout,"find_pmvs (%2d,%2d, rule %1d%1d%1d) :\np1 %6.2f / %6.2f\np2 %6.2f / %6.2f\np3 %6.2f / %6.2f\n",x,y,rule1,rule2,rule3,p1x,p1y,p2x,p2y,p3x,p3y);
	#endif

	return;
}


/***********************************************************CommentBegin******
 *
 * -- ModeMB -- Get the MB mode
 *
 * Purpose :
 *	Get the MB mode
 *
 ***********************************************************CommentEnd********/

SInt ModeMB (Image *MB_decision, Int i, Int j)
{
	Int   width = MB_decision->x;
	SInt  *p = (SInt *)GetImageData(MB_decision);

	return p[width*j+i];
}

