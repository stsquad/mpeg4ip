
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
 *  mot_est_comp.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions to do motion estimation and          */
/* for the current image in one pass.                                     */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "vm_common_defs.h"
#include "mom_util.h"
#include "mot_util.h"
#include "mot_est_mb.h"

extern FILE *ftrace;

/* For correct rounding of chrominance vectors */
static Int roundtab16[] = {0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2};

Void  	MotionEstCompPicture (	Vop *curr_vop,
			SInt *prev,
			SInt *prev_ipol_y,
			SInt *prev_ipol_u,
			SInt *prev_ipol_v,
			Int prev_x,
			Int prev_y,
			Int vop_width,
			Int vop_height,
			Int enable_8x8_mv,
			Int edge,
			Int f_code,
			Int rounding_type,
			Int br_x,
			Int br_y,
			Int br_width,
			Int br_height,
			SInt *curr_comp_y,
			SInt *curr_comp_u,
			SInt *curr_comp_v,
			Float *mad,
			Float *mv16_w,
			Float *mv16_h,
			Float *mv8_w,
			Float *mv8_h,
			SInt *mode16
	);
Void GetPred_Chroma (
			Int x_curr,
			Int y_curr,
			Int dx,
			Int dy,
			SInt *prev_u,
			SInt *prev_v,
			SInt *comp_u,
			SInt *comp_v,
			Int width,
			Int width_prev,
			Int prev_x_min,
			Int prev_y_min,
			Int prev_x_max,
			Int prev_y_max,
			Int rounding_control
	);

/***********************************************************CommentBegin******
 *
 * -- MotionEstimationCompensation -- Estimates the motion vectors and 
 *										do motion compensation
 *
 * Purpose :
 *      Estimates the motion vectors in advanced/not_advanced unrestricted
 *      mode. The output are four images containing x/y components of
 *      MV's (one per 8x8 block), modes (one value per MB). The motion
 *		compensation is also performed.
 *
 * Description :
 *      1)  memory is allocated for these images.
 *      2)  mot_x/mot_y have 4 identical vector for a MB coded inter 16
 *      3)  mot_x/mot_y have 4 identical zero vectors for a MB coded intra
 *      4)  if _NO_ESTIMATION_ is used, the output is the following:
 *              - mot_x     :    all MV's are 0.0
 *              - mot_y     :    all MV's are 0.0
 *              - mode      :    all modes are MB_INTRA (IGNORING THE SHAPE)
 *
 *      Based on: CodeOneOrTwo (TMN5, coder.c)
 *
 ***********************************************************CommentEnd********/

Void
MotionEstimationCompensation (
Vop    *curr_vop,								  /* <-- current Vop (for luminance)                  */
Vop    *prev_rec_vop,							  /* <-- reference Vop (reconstructed)(1/2 pixel)     */
Int    enable_8x8_mv,							  /* <-- 8x8 MV (=1) or only 16x16 MV (=0)            */
Int    edge,									  /* <-- restricted(==0)/unrestricted(==edge) mode    */
Int    f_code,									  /* <-- MV search range 1/2 pel: 1=32,2=64,...,7=2048*/
Vop    *curr_comp_vop,							  /* <-> motion compensated current VOP				  */
Float  *mad,									  /* <-> mad value of the ME/MC result                */
Image  **mot_x,									  /* --> horizontal MV coordinates                    */
Image  **mot_y,									  /* --> vertical MV coordinates                      */
Image  **mode									  /* --> modes for each MB                            */
)
{
	Image   *pr_rec_y;							  /* Reference image (reconstructed)   */
	Image   *pi_y;								  /* Interp.ref.image Y                */

	Image   *mode16;
	Image   *mv16_w;
	Image   *mv16_h;
	Image   *mv8_w;
	Image   *mv8_h;

	SInt    *prev_ipol_y,						  /* Interp.ref.image Y (subimage)            */
			*prev_orig_y;						  /* Ref. original image with edge (subimage) */

	Int     vop_width, vop_height;

	Int     br_x;
	Int     br_y;
	Int     br_height;
	Int     br_width;
	Int     mv_h, mv_w;

	/*GETBBR*/
	br_y=curr_vop->ver_spat_ref;
	br_x=curr_vop->hor_spat_ref;
	br_height=curr_vop->height;
	br_width=curr_vop->width;
	mv_h=br_height/MB_SIZE;
	mv_w=br_width/MB_SIZE;

	/*
	 ** WE SUPPOSE prev_rec_vop & prev_vop HAVE EQUAL SIZE AND POSITIONS
	 */

	vop_width=prev_rec_vop->width;
	vop_height=prev_rec_vop->height;

	/*
	 ** Get images and interpolate them
	 */

	pr_rec_y = prev_rec_vop->y_chan;
	prev_orig_y = (SInt*)GetImageData(pr_rec_y);
	pi_y = AllocImage (2*vop_width, 2*vop_height, SHORT_TYPE);
	InterpolateImage(pr_rec_y, pi_y, GetVopRoundingType(curr_vop));
	prev_ipol_y = (SInt*)GetImageData(pi_y);

	/*
	 * allocate memory for mv's and modes data
	 *
	 */

	mode16=AllocImage (mv_w,mv_h,SHORT_TYPE);
	SetConstantImage (mode16,(Float)MBM_INTRA);

	/*
	 **   mv16 have 2x2 repeted the mv value to share the functions of
	 **   mv prediction between CodeVopVotion and MotionEstimation
	 */

	mv16_w=AllocImage (mv_w*2,mv_h*2,FLOAT_TYPE);
	mv16_h=AllocImage (mv_w*2,mv_h*2,FLOAT_TYPE);
	mv8_w =AllocImage (mv_w*2,mv_h*2,FLOAT_TYPE);
	mv8_h =AllocImage (mv_w*2,mv_h*2,FLOAT_TYPE);
	SetConstantImage (mv16_h,+0.0);
	SetConstantImage (mv16_w,+0.0);
	SetConstantImage (mv8_h,+0.0);
	SetConstantImage (mv8_w,+0.0);

	SetConstantImage (curr_comp_vop->u_chan, 0);
	SetConstantImage (curr_comp_vop->v_chan, 0);

	/* Compute motion vectors and modes for each MB
	 ** (TMN5 functions)
	 */

	MotionEstCompPicture(
		curr_vop,
		prev_orig_y,							  /* Y padd with edge */
		prev_ipol_y,							  /* Y interpolated (from pi_y) */
		(SInt*)GetImageData(prev_rec_vop->u_chan) + (vop_width/2) * (16/2) + (16/2),
		(SInt*)GetImageData(prev_rec_vop->v_chan) + (vop_width/2) * (16/2) + (16/2),
		prev_rec_vop->hor_spat_ref,
		prev_rec_vop->ver_spat_ref,
		vop_width,vop_height,
		enable_8x8_mv,
		edge,
		f_code,
		GetVopRoundingType(curr_vop),
		br_x,br_y,								  /* pos. of the bounding rectangle */
		br_width,br_height,						  /* dims. of the bounding rectangle */
		(SInt*)GetImageData(curr_comp_vop->y_chan),
		(SInt*)GetImageData(curr_comp_vop->u_chan),
		(SInt*)GetImageData(curr_comp_vop->v_chan),
		mad,
		(Float*)GetImageData(mv16_w),
		(Float*)GetImageData(mv16_h),
		(Float*)GetImageData(mv8_w),
		(Float*)GetImageData(mv8_h),
		(SInt*) GetImageData(mode16)
		);

	/* Convert output to MOMUSYS format */
	GetMotionImages(mv16_w, mv16_h, mv8_w, mv8_h, mode16, mot_x, mot_y, mode);

	/* Deallocate dynamic memory */
	FreeImage(mv16_w); FreeImage(mv16_h);
	FreeImage(mv8_w);  FreeImage(mv8_h);
	FreeImage(mode16);
	FreeImage(pi_y);
}

/***********************************************************CommentBegin******
 *
 * -- MotionEstCompPicture -- Computes MV's and predictor errors and 
 *										do motion compensation
 *
 * Purpose :
 *      Computes MV's (8x8 and 16x16) and predictor errors for the whole
 *      vop. Perform motion compensation for the whole vop.
 *
 ***********************************************************CommentEnd********/

Void
MotionEstCompPicture(
Vop     *curr_vop,								  /* <-- Current VOP                                  */
SInt    *prev,									  /* <-- Original Y padd with edge                    */
SInt    *prev_ipol,								  /* <-- Y interpolated (from pi_y)                  */
SInt    *prev_u,								  /* <-- Original U padd with edge                   */
SInt    *prev_v,								  /* <-- Original V padd with edge                  */
Int     prev_x,									  /* <-- absolute horiz. position of previous vop     */
Int     prev_y,									  /* <-- absolute verti. position of  previous vop    */
Int     vop_width,								  /* <-- horizontal previous vop dimension            */
Int     vop_height,								  /* <-- vertical previous vop dimension              */
Int     enable_8x8_mv,							  /* <-- 8x8 MV (=1) or only 16x16 MV (=0)            */
Int     edge,									  /* <-- edge arround the reference vop               */
Int     f_code,          /* MW QPEL 07-JUL-1998 *//* <-- MV search range 1/2 or 1/4 pel: 1=32,2=64,...,7=2048*/
Int		rounding_type,							  /* <-- The rounding type of the current vop         */
Int     br_x,									  /* <-- absolute horiz. position of the current vop  */
Int     br_y,									  /* <-- absolute verti. position of the current vop  */
Int     br_width,								  /* <-- current bounding rectangule width            */
Int     br_height,								  /* <-- current bounding rectangle height            */
SInt	*curr_comp_y,							  /* <-> motion compensated current Y                 */
SInt	*curr_comp_u,							  /* <-> motion compensated current U                 */
SInt	*curr_comp_v,							  /* <-> motion compensated current V                 */
Float	*mad,									  /* <-> the mad value of current ME/MC result        */
Float   *mv16_w,								  /* --> predicted horizontal 16x16 MV(if approp.)    */
Float   *mv16_h,								  /* --> predicted vertical 16x16 MV  (if approp.)    */
Float   *mv8_w,									  /* --> predicted horizontal 8x8 MV  (if approp.)    */
Float   *mv8_h,									  /* --> predicted vertical 8x8 MV    (if approp.)    */
SInt    *mode16									  /* --> mode of the preditect motion vector          */
)
{
	Int		i, j, k;
	SInt	curr_mb[MB_SIZE][MB_SIZE];
	SInt	curr_comp_mb_16[MB_SIZE][MB_SIZE];
	SInt	curr_comp_mb_8[MB_SIZE][MB_SIZE];
	Int		sad8 = MV_MAX_ERROR, sad16, sad;
	Int		imas_w, imas_h,	Mode;
	Int		posmode, pos16,	pos8;
	Int		min_error16,
			min_error8_0, min_error8_1, min_error8_2, min_error8_3;
	SInt	*curr = (SInt *)GetImageData(GetVopY(curr_vop));
	/***************************************************************************
	array of flags, which contains for the MB and for each one of the 4 Blocks
	the following info sequentially:
	  xm, 1 if the lower search (x axis) is completed (no 1/2 search can be done)
			 0, do 1/2 search in the lower bound (x axis)
	  xM, 1 if the upper search (x axis) is completed (no 1/2 search can be done)
			 0, do 1/2 search in the upper bound (x axis)
	  ym, 1 if the lower search (y axis) is completed (no 1/2 search can be done)
			 0, do 1/2 search in the lower bound (y axis)
	  yM, 1 if the upper search (y axis) is completed (no 1/2 search can be done)
			 0, do 1/2 search in the upper bound (y axis)
	***************************************************************************/
	SInt	*halfpelflags;
	Float	hint_mv_w, hint_mv_h;
	Int		xsum,ysum,dx,dy;
	Int		prev_x_min,prev_x_max,prev_y_min,prev_y_max;

	prev_x_min = 2 * prev_x + 2 * 16;
	prev_y_min = 2 * prev_y + 2 * 16;
	prev_x_max = prev_x_min + 2 * vop_width - 4 * 16;
	prev_y_max = prev_y_min + 2 * vop_height - 4 * 16; 

	imas_w=br_width/MB_SIZE;
	imas_h=br_height/MB_SIZE;

	/* Do motion estimation and store result in array */

	halfpelflags=(SInt*)malloc(5*4*sizeof(SInt));
	/*	halfpelflags=(SInt*)malloc(9*4*sizeof(SInt)); */
	sad = 0;

	for ( j=0; j< (br_height/MB_SIZE); j++)
	{
		hint_mv_w = hint_mv_h = 0.f;
		for ( i=0; i< (br_width/MB_SIZE); i++)
		{
			/* Integer pel search */
			Int min_error;

			posmode =          j * imas_w +   i;
			pos16   = pos8 = 2*j*2*imas_w + 2*i;

			MBMotionEstimation(curr_vop, prev, br_x, br_y,
				br_width, i, j, prev_x, prev_y,
				vop_width, vop_height, enable_8x8_mv, edge,
				f_code, GetVopSearchRangeFor(curr_vop), 
				hint_mv_w, hint_mv_h, // the hint seeds
				mv16_w, mv16_h,
				mv8_w, mv8_h, &min_error, halfpelflags);

			/* Inter/Intra decision */
			Mode = ChooseMode((SInt *)GetImageData(GetVopY(curr_vop)),
				i*MB_SIZE,j*MB_SIZE, min_error, br_width);

			hint_mv_w = mv16_w[pos16];
			hint_mv_h = mv16_h[pos16];

			LoadArea(curr, i*MB_SIZE, j*MB_SIZE, 16, 16, br_width, (SInt *)curr_mb);

			/* 0==MBM_INTRA,1==MBM_INTER16||MBM_INTER8 */
			if ( Mode != 0)
			{
				FindSubPel (i*MB_SIZE,j*MB_SIZE, prev_ipol,
					&curr_mb[0][0], 16, 16 , 0,
					br_x-prev_x,br_y-prev_y,vop_width, vop_height,
					edge, halfpelflags, &curr_comp_mb_16[0][0],
					&mv16_w[pos16], &mv16_h[pos16], &min_error16);

				/* sad16(0,0) already decreased by Nb/2+1 in FindHalfPel() */
				sad16 = min_error16;
				mode16[posmode] = MBM_INTER16;

				if (enable_8x8_mv)
				{
					xsum = 0; ysum = 0;

					FindSubPel(i*MB_SIZE, j*MB_SIZE, prev_ipol,
						&curr_mb[0][0], 8, 8 , 0,
						br_x-prev_x,br_y-prev_y, vop_width, vop_height,
						edge, halfpelflags, &curr_comp_mb_8[0][0],
						&mv8_w[pos8], &mv8_h[pos8],	&min_error8_0);
					xsum += (Int)(2*(mv8_w[pos8]));
					ysum += (Int)(2*(mv8_h[pos8]));

					FindSubPel(i*MB_SIZE, j*MB_SIZE, prev_ipol,
						&curr_mb[0][8], 8, 8 , 1,
						br_x-prev_x,br_y-prev_y, vop_width,vop_height,
						edge, halfpelflags, &curr_comp_mb_8[0][8],
						&mv8_w[pos8+1], &mv8_h[pos8+1],	&min_error8_1);
					xsum += (Int)(2*(mv8_w[pos8+1]));
					ysum += (Int)(2*(mv8_h[pos8+1]));

					pos8+=2*imas_w;

					FindSubPel(i*MB_SIZE, j*MB_SIZE, prev_ipol,
						&curr_mb[8][0], 8, 8 , 2,
						br_x-prev_x,br_y-prev_y, vop_width,vop_height,
						edge, halfpelflags, &curr_comp_mb_8[8][0], 
						&mv8_w[pos8], &mv8_h[pos8],	&min_error8_2);
					xsum += (Int)(2*(mv8_w[pos8]));
					ysum += (Int)(2*(mv8_h[pos8]));

					FindSubPel(i*MB_SIZE, j*MB_SIZE, prev_ipol, 
						&curr_mb[8][8], 8, 8 , 3,
						br_x-prev_x,br_y-prev_y, vop_width,vop_height,
						edge, halfpelflags, &curr_comp_mb_8[8][8], 
						&mv8_w[pos8+1], &mv8_h[pos8+1],	&min_error8_3);
					xsum += (Int)(2*(mv8_w[pos8+1]));
					ysum += (Int)(2*(mv8_h[pos8+1]));

					sad8 = min_error8_0+min_error8_1+min_error8_2+min_error8_3;

					/* Choose 8x8 or 16x16 vectors */
					if (sad8 < (sad16 -(128+1)))
						mode16[posmode] = MBM_INTER8;
				}							  /* end of enable_8x8_mv mode */

				/* When choose 16x16 vectors */
				/* sad16(0,0) was decreased by MB_Nb, now add it back */
				if ((mv16_w[pos16]==0.0) && (mv16_h[pos16]==0.0) && (mode16[posmode]==MBM_INTER16))
					sad16 += 128+1;

				/* calculate motion vectors for chroma compensation */
				if(mode16[posmode] == MBM_INTER8)
				{
					dx = SIGN (xsum) * (roundtab16[ABS (xsum) % 16] + (ABS (xsum) / 16) * 2);
					dy = SIGN (ysum) * (roundtab16[ABS (ysum) % 16] + (ABS (ysum) / 16) * 2);
					sad += sad8;
				}
				else /* mode == MBM_INTER16 */
				{
					dx = (Int)(2 * mv16_w[pos16]);
					dy = (Int)(2 * mv16_h[pos16]);
					dx = ( dx % 4 == 0 ? dx >> 1 : (dx>>1)|1 );
					dy = ( dy % 4 == 0 ? dy >> 1 : (dy>>1)|1 );
					sad += sad16;
				}

				GetPred_Chroma (i*MB_SIZE, j*MB_SIZE, dx, dy, 
					prev_u, prev_v, curr_comp_u, curr_comp_v,
					br_width, vop_width,
					prev_x_min/4,prev_y_min/4,prev_x_max/4,prev_y_max/4, rounding_type);

			}								  /* end of mode non zero */
			else							  /* mode = 0 INTRA */
			{
				for (k = 0; k < MB_SIZE*MB_SIZE; k++) 
				{
					// for INTRA MB, set compensated 0 to generate correct error image
					curr_comp_mb_16[k/MB_SIZE][k%MB_SIZE] = 0;
					// for INTRA_MB, SAD is recalculated from image instead of using min_error
					sad += curr_mb[k/MB_SIZE][k%MB_SIZE];
				}
			}

			if (mode16[posmode] == MBM_INTER8)
				SetArea((SInt*)curr_comp_mb_8, i*MB_SIZE, j*MB_SIZE, 16, 16, br_width, curr_comp_y);
			else
				SetArea((SInt*)curr_comp_mb_16, i*MB_SIZE, j*MB_SIZE, 16, 16, br_width, curr_comp_y);
		}   /* end of i loop */
	}	/* end of j loop */

	*mad = (float)sad/(br_width*br_height);

	free((Char*)halfpelflags);
	return;
}


/***********************************************************CommentBegin******
 *
 * unrestricted_MC_chro
 *
 * Purpose :
 *	To make unrestricted MC
 *
 ***********************************************************CommentEnd********/
/*Int unrestricted_MC_chro(Int x,Int y, Int npix,
	Int prev_x_min,Int prev_y_min,
	Int prev_x_max, Int prev_y_max)
{

	if (x < prev_x_min) x = prev_x_min;
	else if (x >=prev_x_max) x = prev_x_max-1;

	if (y < prev_y_min) y = prev_y_min;
	else if (y >=prev_y_max) y = prev_y_max-1;
	return(x+y*npix);
}*/

#define unrestricted_MC_chro(x,y,npix,prev_x_min,prev_y_min,prev_x_max,prev_y_max) ((x)+(y)*(npix))


/***********************************************************CommentBegin******
 *
 * -- GetPred_Chroma -- Predicts chrominance macroblock
 *
 * Purpose :
 *	Does the chrominance prediction for P-frames
 *
 * Arguments in :
 *	current position in image,
 *	motionvectors,
 *	pointers to compensated and previous Vops,
 *	width of the compensated Vop
 *  width of the previous/reference Vop
 *
 ***********************************************************CommentEnd********/

Void GetPred_Chroma (
	Int x_curr,
	Int y_curr,
	Int dx,
	Int dy,
	SInt *prev_u,
	SInt *prev_v,
	SInt *comp_u,
	SInt *comp_v,
	Int width,
	Int width_prev,
	Int prev_x_min,
	Int prev_y_min,
	Int prev_x_max,
	Int prev_y_max,
	Int rounding_control)
{
	Int m,n;

	Int x, y, ofx, ofy, lx;
	Int xint, yint;
	Int xh, yh;
	Int index1,index2,index3,index4;

	lx = width_prev/2;

	x = x_curr>>1;
	y = y_curr>>1;

	xint = dx>>1;
	xh = dx & 1;
	yint = dy>>1;
	yh = dy & 1;

	if (!xh && !yh)
	{
		for (n = 0; n < 8; n++)
		{
			for (m = 0; m < 8; m++)
			{
				ofx = x + xint + m;
				ofy = y + yint + n;
				index1 = unrestricted_MC_chro(ofx,ofy,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);
				comp_u[(y+n)*width/2+x+m]
					= *(prev_u+index1);
				comp_v[(y+n)*width/2+x+m]
					= *(prev_v+index1);
			}
		}
	}
	else if (!xh && yh)
	{
		for (n = 0; n < 8; n++)
		{
			for (m = 0; m < 8; m++)
			{
				ofx = x + xint + m;
				ofy = y + yint + n;
				index1 =  unrestricted_MC_chro(ofx,ofy,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);
				index2 =  unrestricted_MC_chro(ofx,ofy+yh,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);

				comp_u[(y+n)*width/2+x+m]
					= (*(prev_u+index1) +
					*(prev_u+index2) + 1- rounding_control)>>1;

				comp_v[(y+n)*width/2+x+m]
					= (*(prev_v+index1) +
					*(prev_v+index2) + 1- rounding_control)>>1;
			}
		}
	}
	else if (xh && !yh)
	{
		for (n = 0; n < 8; n++)
		{
			for (m = 0; m < 8; m++)
			{
				ofx = x + xint + m;
				ofy = y + yint + n;
				index1 =  unrestricted_MC_chro(ofx,ofy,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);
				index2 =  unrestricted_MC_chro(ofx+xh,ofy,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);

				comp_u[(y+n)*width/2+x+m]
					= (*(prev_u+index1) +
					*(prev_u+index2) + 1- rounding_control)>>1;

				comp_v[(y+n)*width/2+x+m]
					= (*(prev_v+index1) +
					*(prev_v+index2) + 1- rounding_control)>>1;
			}
		}
	}
	else
	{											  /* xh && yh */
		for (n = 0; n < 8; n++)
		{
			for (m = 0; m < 8; m++)
			{
				ofx = x + xint + m;
				ofy = y + yint + n;
				index1 =  unrestricted_MC_chro(ofx,ofy,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);
				index2 =  unrestricted_MC_chro(ofx+xh,ofy,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);
				index3 =  unrestricted_MC_chro(ofx,ofy+yh,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);
				index4 =  unrestricted_MC_chro(ofx+xh,ofy+yh,lx,prev_x_min,
					prev_y_min,prev_x_max,prev_y_max);

				comp_u[(y+n)*width/2+x+m]
					= (*(prev_u+index1)+
					*(prev_u+index2)+
					*(prev_u+index3)+
					*(prev_u+index4)+
					2- rounding_control)>>2;

				comp_v[(y+n)*width/2+x+m]
					= (*(prev_v+index1)+
					*(prev_v+index2)+
					*(prev_v+index3)+
					*(prev_v+index4)+
					2- rounding_control)>>2;
			}
		}
	}
	return;
}

