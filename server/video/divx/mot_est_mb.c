
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
 *  mot_est_mb.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions to do motion estimation and          */
/* for one MacroBlock in one pass.                                        */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "mot_util.h"

#ifdef USE_MMX
#define SAD_Macroblock	SAD_Macroblock_mmx
#endif

/* Obtaining if two floating point values are equal*/
#define ABSDOUBLE(x)   (((x) > 0.0001) ? (x) : (((x) < -0.0001) ? -(x): 0.0 ))
#define ARE_EQUAL(x,y) ( (ABSDOUBLE((Float)(x)-(y))>0.1)?(0):(1) )

/* auxiliar define for indexing in MBMotionEstimation */
#define INDEX_BIG(x,y) ((x)+(y)*(vop_width))
#define INDEX_NOR(x,y) ((x)+(y)*(MB_SIZE))

/* ------------------------------------------------------------------------- */

/***********************************************************CommentBegin******
 *
 * -- RangeInSearchArea -- computes the range of the search area
 *
 * Purpose :
 *      computes the range of the search area for the predicted MV's
 *      INSIDE the overlapped zone between reference and current vops
 *
 ***********************************************************CommentEnd********/

static Void
RangeInSearchArea(
Int     i,										  /* <-- horizontal MBcoordinate in pixels               */
Int     j,										  /* <-- vertical MB coordinate in pixels                */
Int     block,									  /* <-- block position (0 16x16; 1-2-3-4 8x8)           */
Int     prev_x,									  /* <-- absolute horizontal position of the previous vop*/
Int     prev_y,									  /* <-- absolute vertical position of the previous vop  */
Int     vop_width,								  /* <-- horizontal vop dimension                        */
Int     vop_height,								  /* <-- vertical vop dimension                          */
Int     br_x,									  /* <-- absolute horizontal position of the current vop */
Int     br_y,									  /* <-- absolute vertical   position of the current vop */
Int     edge,									  /* <-- edge arround the reference vop                  */
Int     f_code,									  /* <-  MV search range 1/2 (or 1/4) pel: (0=16,) 1=32,2=64,...,7=2048 */
Float   *mv_x_min,								  /* <-- min horizontal range                            */
Float   *mv_x_max,								  /* <-- max horizontal range                            */
Float   *mv_y_min,								  /* <-- min vertical range                              */
Float   *mv_y_max,								  /* <-- max vertical range                              */
Int     *out									  /* --> the search area does not exist (the reference   */												  /*     and current BB does not overlap)                */
)
{
	Int   dim_curr_x_max,
		dim_curr_y_max,
		dim_curr_x_min,
		dim_curr_y_min;
	Int   dim_prev_x_max,
		dim_prev_y_max,
		dim_prev_x_min,
		dim_prev_y_min;
	Int   mb_b_size,
		block_x,
		block_y;

	*out=0;

	switch (block)
	{
		case 0:									  /* 8x8 or 16x16 block search */
			block_x=0;							  /*****************************/
			block_y=0;							  /** 1 2 ********  0  *********/
			mb_b_size=MB_SIZE;					  /** 3 4 ********     *********/
			break;								  /*****************************/
		case 1:
			block_x=0;
			block_y=0;
			mb_b_size=B_SIZE;
			break;
		case 2:
			block_x=B_SIZE;
			block_y=0;
			mb_b_size=B_SIZE;
			break;
		case 3:
			block_x=0;
			block_y=B_SIZE;
			mb_b_size=B_SIZE;
			break;
		case 4:
			block_x=B_SIZE;
			block_y=B_SIZE;
			mb_b_size=B_SIZE;
			break;
		default:
			return;
	}

	/* min x/y */
	dim_curr_x_min=(Int)(br_x+i*MB_SIZE+*mv_x_min+block_x);
	dim_curr_y_min=(Int)(br_y+j*MB_SIZE+*mv_y_min+block_y);
	dim_prev_x_min=prev_x/*-edge*/;
	dim_prev_y_min=prev_y/*-edge*/;

	/* max x/y */
	/*the MB right-pixels inside */
	dim_curr_x_max=(Int)(br_x+i*MB_SIZE+*mv_x_max+mb_b_size+block_x);
	/*the MB bottom-pixels inside */
	dim_curr_y_max=(Int)(br_y+j*MB_SIZE+*mv_y_max+mb_b_size+block_y);
	dim_prev_x_max=prev_x+vop_width /*+edge*/;
	dim_prev_y_max=prev_y+vop_height/*+edge*/;

	/* range x/y min */

	if (dim_curr_x_min > dim_prev_x_max)
	{
		*out=1;
	}
	else if(dim_curr_x_min < dim_prev_x_min)
	{
		*mv_x_min = *mv_x_min + ( dim_prev_x_min - dim_curr_x_min ) ;
	}

	if(!(*out))
	{
		if (dim_curr_y_min > dim_prev_y_max)
		{
			*out=1;
		}
		else if(dim_curr_y_min < dim_prev_y_min)
		{
			*mv_y_min = *mv_y_min + ( dim_prev_y_min - dim_curr_y_min ) ;
		}
	}

	/* range x/y max */
	if(!(*out))
	{
		if(dim_curr_x_max < dim_prev_x_min)
		{
			*out=1;
		}
		if ((!(*out))&&(dim_curr_x_max > dim_prev_x_max))
		{
			*mv_x_max = *mv_x_max - ( dim_curr_x_max - dim_prev_x_max) ;
		}
	}

	if(!(*out))
	{
		if(dim_curr_y_max < dim_prev_y_min)
		{
			*out=1;								  /* already set */
		}
		if ((!(*out))&&(dim_curr_y_max > dim_prev_y_max))
		{
			*mv_y_max = *mv_y_max - ( dim_curr_y_max - dim_prev_y_max) ;
		}
	}

	if(*mv_x_min>*mv_x_max)
	{
		*out=1;
	}

	if ( (!(*out)) && (*mv_y_min>*mv_y_max))
	{
		*out=1;
	}

	return;
}


/***********************************************************CommentBegin******
 *
 * -- Obtain_Range -- computes the range of the search area
 *
 * Purpose :
 *      computes the range of the search area for the predicted MV's
 *      (it can be outside the overlapped zone between the reference
 *      and current vops)
 *
 * Return values :
 *      1 on success, 0 on error
 *
 ***********************************************************CommentEnd********/

static Int
Obtain_Range(
Int     f_code,									  /* <-- MV search range 1/2 (or 1/4 pel): (0=16,) 1=32,2=64,...,7=2048   */
Int     sr,										  /* <-- Serach range (radius)                           */
Int     type,									  /* <-- MBM_INTER16==16x16 search;	MBM_INTER8==8x8 search */
Float   pmv_x,									  /* <-- predicted horizontal motion vector              */
Float   pmv_y,									  /* <-- predicted horizontal motion vector              */
Float   *mv_x_min,								  /* --> min horizontal range                            */
Float   *mv_x_max,								  /* --> max horizontal range                            */
Float   *mv_y_min,								  /* --> min vertical range                              */
Float   *mv_y_max,								  /* --> max vertical range                              */
Int     quarter_pel								  /* <-- quarter pel flag (to allow f_code==0 without sprite) */
)
{
	Int    error;

	Float  aux_x_min, aux_y_min,
		aux_x_max, aux_y_max;

	Float  range;

	error=0;

	if ((f_code==0) && (!quarter_pel))		  /* for Low Latency Sprite */
	{
		*mv_x_min=0;
		*mv_x_max=0;
		*mv_y_min=0;
		*mv_y_max=0;
	}
	else
	{
		range = sr;

		*mv_x_min=-range; *mv_x_max= range - 0.5f;

		*mv_y_min=-range; *mv_y_max= range - 0.5f;
	}

	if (type==MBM_INTER8)
	{
		aux_x_min=pmv_x - DEFAULT_8_WIN;
		aux_y_min=pmv_y - DEFAULT_8_WIN;
		aux_x_max=pmv_x + DEFAULT_8_WIN;
		aux_y_max=pmv_y + DEFAULT_8_WIN;

		if(*mv_x_min < aux_x_min)
			*mv_x_min = aux_x_min;
		if(*mv_y_min < aux_y_min)
			*mv_y_min = aux_y_min;
		if(*mv_x_max > aux_x_max)
			*mv_x_max = aux_x_max;
		if(*mv_y_max > aux_y_max)
			*mv_y_max = aux_y_max;
	}

	if (error==1)
		return(0);
	else
		return(1);
}


/***********************************************************CommentBegin******
 *
 * -- MBMotionEstimation -- Computes INTEGER PRECISION MV's for a MB
 *
 * Purpose :
 *      Computes INTEGER PRECISION MV's for a MB. Also returns
 *      prediction errors. Requires the whole MVs data images to
 *      obtain prediction for the current MV, which determines search
 *      range
 *
 ***********************************************************CommentEnd********/

Void
MBMotionEstimation(
SInt    *curr,								  /* <-- Current vop pointer                          */
SInt    *prev,									  /* <-- extended reference picture                   */
Int     br_x,									  /* <-- horizontal bounding rectangle coordinate     */
Int     br_y,									  /* <-- vertical bounding rectangle coordinate       */
Int     br_width,								  /* <-- bounding rectangule width                    */
Int     i,										  /* <-- horizontal MBcoordinate in pixels            */
Int     j,										  /* <-- vertical MB coordinate in pixels             */
Int     prev_x,									  /* <-- absolute horiz. position of previous vop     */
Int     prev_y,									  /* <-- absolute verti. position of previous vop     */
Int     vop_width,								  /* <-- horizontal vop dimension                     */
Int     vop_height,								  /* <-- vertical vop dimension                       */
Int     enable_8x8_mv,							  /* <-- 8x8 MV (=1) or only 16x16 MV (=0)            */
Int     edge,									  /* <-- edge                                         */
Int     f_code,									  /* <-- MV search range 1/2 (or 1/4) pel: (0=16,) 1=32,2=64,...,7=2048*/
Int     sr,										  /* <-- search range (corresponds to f_code) UB 990215*/
Float	hint_mv_w,                                /* <-- hint seed for horizontal MV                  */
Float	hint_mv_h,                                /* <-- hint seed for vertical MV                    */
Float   *mv16_w,								  /* --> predicted horizontal 16x16 MV(if approp.)    */
Float   *mv16_h,								  /* --> predicted vertical 16x16 MV  (if approp.)    */
Float   *mv8_w,									  /* --> predicted horizontal 8x8 MV  (if approp.)    */
Float   *mv8_h,									  /* --> predicted vertical 8x8 MV    (if approp.)    */
Int     *min_error,								  /* --> Minimum prediction error                     */
SInt    *flags
)
{
	Int     x, y;
	Int     sad, sad_min=MV_MAX_ERROR;
	Int     mv_x, mv_y;
	Int     block;
	Float   pmv_x, pmv_y;
	SInt	act_block[MB_SIZE*MB_SIZE];

	Float   mv_x_min, mv_x_max,
		mv_y_min, mv_y_max;
	Int     int_mv_x_min=0, int_mv_x_max=0,
		int_mv_y_min=0, int_mv_y_max=0;

	Int     pos16, pos8;
	Int     mvm_width   = br_width/MB_SIZE;

	Int     x_curr      = i*MB_SIZE,
		y_curr      = j*MB_SIZE;
	Int     hb,vb;
	Int     out;

	Int     rel_ori_x;
	Int     rel_ori_y;

	Int     min_error16, min_error8 = 0;
//	SInt    *curr = (SInt *)GetImageData(GetVopY(curr_vop));
	#ifndef _FULL_SEARCH_						  /* PIH (NTU) Fast ME 08/08/99 */
	typedef struct
	{
		Int x;
		Int y;
		SInt start_nmbr;
	} DPoint;

	typedef struct
	{
		DPoint point[8];
	} Diamond;

	SInt d_type=1,stop_flag=0,pt_nmbr=0,check_pts,total_check_pts=8,mot_dirn=0;
	Int d_centre_x=0,d_centre_y=0,check_pt_x,check_pt_y;
	Diamond diamond[2]=
	{
		{
			{	{0,1,0},	{1,0,0},	{0,-1,0},	{-1,0,0}	}
		}
		,
		{
			{	{0,2,6},	{1,1,0},	{2,0,0},	{1,-1,2},
				{0,-2,2},	{-1,-1,4},	{-2,0,4},	{-1,1,6}	}
		}
	};
	#endif

	d_centre_x = (int)hint_mv_w;
	d_centre_y = (int)hint_mv_h;

	rel_ori_x=br_x-prev_x;
	rel_ori_y=br_y-prev_y;

	/* Load act_block */
	
	LoadArea(curr, x_curr,y_curr,MB_SIZE,MB_SIZE,br_width, act_block);

	/* Compute 16x16 integer MVs */

	/* Obtain range */

	Obtain_Range (f_code, sr, MBM_INTER16,		  /* UB 990215 added search range */
		0.0, 0.0, &mv_x_min, &mv_x_max,
												  /*UB 990215 added quarter pel support */
		&mv_y_min, &mv_y_max, 0/*GetVopQuarterPel(curr_vop)*/);

	RangeInSearchArea (i,j,0, prev_x, prev_y, vop_width, vop_height,
		br_x, br_y, edge,f_code, &mv_x_min, &mv_x_max,
												  /*UB 990215 added quarter pel support */
		&mv_y_min, &mv_y_max,&out);

	/* Compute */

	if(!out)
	{
		int_mv_x_min=(int)ceil((double)mv_x_min);
		int_mv_y_min=(int)ceil((double)mv_y_min);
		int_mv_x_max=(int)floor((double)mv_x_max);
		int_mv_y_max=(int)floor((double)mv_y_max);
		flags[0]=ARE_EQUAL(int_mv_x_min,mv_x_min);
		flags[1]=ARE_EQUAL(int_mv_x_max,mv_x_max);
		flags[2]=ARE_EQUAL(int_mv_y_min,mv_y_min);
		flags[3]=ARE_EQUAL(int_mv_y_max,mv_y_max);

		sad_min=MV_MAX_ERROR;
		mv_x = mv_y = 2000;						  /* A very large MV */

		#ifdef _FULL_SEARCH_				  /* PIH (NTU)  08/08/99 */
		for (y=int_mv_y_min; y<=int_mv_y_max; y++)
			for (x=int_mv_x_min; x<=int_mv_x_max; x++)
		{
			if (x==0 && y==0)
					sad=SAD_Macroblock(prev+INDEX_BIG(x_curr+rel_ori_x,
					y_curr+rel_ori_y), act_block, 
					(vop_width/*+2*edge*/), MV_MAX_ERROR)
					- (128 + 1);
			else
				sad=SAD_Macroblock(prev+INDEX_BIG(x_curr+x+rel_ori_x,
					y_curr+y+rel_ori_y), act_block, 
					(vop_width/*+2*edge*/), sad_min);

			if (sad<sad_min)
			{
				sad_min=sad;
				mv_x=x;
				mv_y=y;
			}
			else if (sad==sad_min)
			if((ABS(x)+ABS(y)) < (ABS(mv_x)+ABS(mv_y)))
			{
				sad_min=sad;
				mv_x=x;
				mv_y=y;
			}
		}									  /*for*/
		#else								  /* PIH (NTU) Fast ME 08/08/99 */
		sad = SAD_Macroblock(prev+INDEX_BIG(x_curr+rel_ori_x,
			y_curr+rel_ori_y), act_block, (vop_width/*+2*edge*/), MV_MAX_ERROR)
			- (128 + 1);
		if (sad<sad_min)
		{
			sad_min=sad;
			mv_x = mv_y = 0;
		}
		do
		{
			check_pts=total_check_pts;

			do
			{
				check_pt_x = diamond[d_type].point[pt_nmbr].x + d_centre_x;
				check_pt_y = diamond[d_type].point[pt_nmbr].y + d_centre_y;

				/* Restrict the search to the searching window ; Note: This constraint can be removed */
				if ( check_pt_x < int_mv_x_min || check_pt_x > int_mv_x_max || check_pt_y < int_mv_y_min || check_pt_y > int_mv_y_max)
				{
					sad = MV_MAX_ERROR;
				}
				else
				{
					sad=SAD_Macroblock(prev+INDEX_BIG(x_curr+check_pt_x+rel_ori_x,
						y_curr+check_pt_y+rel_ori_y), act_block, 
						(vop_width/*+2*edge*/), sad_min);
					#ifdef _SAD_EXHAUS_
					fprintf(stdout,"+o+ [%2d,%2d] sad16(%3d,%3d)=%4d\n",i,j,x,y,sad);
					#endif
				}
				if (sad<sad_min)
				{
					sad_min=sad;
					mv_x=check_pt_x;
					mv_y=check_pt_y;
					mot_dirn=pt_nmbr;
				}
				else if (sad==sad_min)
				if((ABS(check_pt_x)+ABS(check_pt_y)) < (ABS(mv_x)+ABS(mv_y)))
				{
					sad_min=sad;
					mv_x=check_pt_x;
					mv_y=check_pt_y;
					mot_dirn=pt_nmbr;
				}

				pt_nmbr+=1;
				if((pt_nmbr)>= 8) pt_nmbr-=8;
				check_pts-=1;
			}
			while(check_pts>0);

			if( d_type == 0)
			{
				stop_flag = 1;

			}
			else
			{
				if( (mv_x == d_centre_x) && (mv_y == d_centre_y) )
				{
					d_type=0;
					pt_nmbr=0;
					total_check_pts = 4;
				}
				else
				{
					if((mv_x==d_centre_x) ||(mv_y==d_centre_y))
						total_check_pts=5;
					else
						total_check_pts=3;
					pt_nmbr=diamond[d_type].point[mot_dirn].start_nmbr;
					d_centre_x = mv_x;
					d_centre_y = mv_y;

				}
			}
		}
		while(stop_flag!=1);
		#endif

		flags[0]=flags[0] && (mv_x==int_mv_x_min);
		flags[1]=flags[1] && (mv_x==int_mv_x_max);
		flags[2]=flags[2] && (mv_y==int_mv_y_min);
		flags[3]=flags[3] && (mv_y==int_mv_y_max);
	}
	else
	{
		mv_x=mv_y=0;
		sad_min=MV_MAX_ERROR;
	}

	/* Store result */

	out |=((mv_x==2000)||(mv_y==2000));
	if(out)
	{
		mv_x=mv_y=0;
		sad_min=MV_MAX_ERROR;
	}

	pos16=2*j*2*mvm_width + 2*i;
	mv16_w[pos16]=   mv_x; mv16_h[pos16]=   mv_y;
	mv16_w[pos16+1]= mv_x; mv16_h[pos16+1]= mv_y;
	pos16+=2*mvm_width;
	mv16_w[pos16]=   mv_x; mv16_h[pos16]=   mv_y;
	mv16_w[pos16+1]= mv_x; mv16_h[pos16+1]= mv_y;
	min_error16 = sad_min;

	*min_error = min_error16;

	/* Compute 8x8 MVs */

	if(enable_8x8_mv==1)
	{
		if(!out)
		{
			for (block=0;block<4;block++)
			{
				/* Obtain range */
				if(block==0)
				{
					hb=vb=0;
				}
				else if (block==1)
				{
					hb=1;vb=0;
				}
				else if (block==2)
				{
					hb=0;vb=1;
				}
				else
				{
					hb=vb=1;
				}

				/* VM 2.*: restrict the search range based on the current 16x16 MV
				 *         inside a window around it
				 */

				pmv_x=mv16_w[pos16]; pmv_y=mv16_h[pos16];

											  /* UB 990215 added search range */
				Obtain_Range(f_code, sr, MBM_INTER8,
					pmv_x, pmv_y, &mv_x_min,
											  /*UB 990215 added quarter pel support */
					&mv_x_max, &mv_y_min, &mv_y_max, 0 /*GetVopQuarterPel(curr_vop)*/);

				RangeInSearchArea(i,j, block+1, prev_x, prev_y,
					vop_width, vop_height, br_x, br_y, edge,f_code,
											  /*UB 990215 added quarter pel support */
					&mv_x_min, &mv_x_max, &mv_y_min,&mv_y_max,&out);

				if(!out)
				{
					int_mv_x_min=(int)ceil((double)mv_x_min);
					int_mv_y_min=(int)ceil((double)mv_y_min);
					int_mv_x_max=(int)floor((double)mv_x_max);
					int_mv_y_max=(int)floor((double)mv_y_max);
					flags[4+block*4]=ARE_EQUAL(int_mv_x_min,mv_x_min);
					flags[4+block*4+1]=ARE_EQUAL(int_mv_x_max,mv_x_max);
					flags[4+block*4+2]=ARE_EQUAL(int_mv_y_min,mv_y_min);
					flags[4+block*4+3]=ARE_EQUAL(int_mv_y_max,mv_y_max);

					sad_min=MV_MAX_ERROR;
					mv_x = mv_y = 2000;		  /* A very large MV */

					for (y=int_mv_y_min; y<=int_mv_y_max; y++)
						for (x=int_mv_x_min; x<=int_mv_x_max; x++)
					{
						sad=SAD_Block(prev+
							INDEX_BIG(x_curr + x + 8*(block==1||block==3)+rel_ori_x,
							y_curr + y + 8*(block==2||block==3)+rel_ori_y),
							act_block+INDEX_NOR(8*(block==1||block==3),
							8*(block==2||block==3)),
							(vop_width /*+2*edge*/), sad_min);

						if (sad<sad_min)
						{
							sad_min=sad;
							mv_x=x;
							mv_y=y;
						}
						else if (sad==sad_min)
						if((ABS(x)+ABS(y)) < (ABS(mv_x)+ABS(mv_y)))
						{
							sad_min=sad;
							mv_x=x;
							mv_y=y;
						}
					}						  /*for*/

					flags[4+block*4]   = flags[4+block*4]   && (mv_x==int_mv_x_min);
					flags[4+block*4+1] = flags[4+block*4+1] && (mv_x==int_mv_x_max);
					flags[4+block*4+2] = flags[4+block*4+2] && (mv_y==int_mv_y_min);
					flags[4+block*4+3] = flags[4+block*4+3] && (mv_y==int_mv_y_max);
				}
				else
				{
					mv_x=mv_y=0;
					sad_min=MV_MAX_ERROR;
					/* punish the OUTER blocks with MV_MAX_ERROR in order to
					   be INTRA CODED */
				}

				/* Store result */

				if(block==0)
				{
					pos8=2*j*2*mvm_width + 2*i;
					min_error8 += sad_min;
				}
				else if (block==1)
				{
					pos8=2*j*2*mvm_width + 2*i+1;
					min_error8 += sad_min;
				}
				else if (block==2)
				{
					pos8=(2*j+1)*2*mvm_width + 2*i;
					min_error8 += sad_min;
				}
				else
				{
					pos8=(2*j+1)*2*mvm_width + 2*i+1;
					min_error8 += sad_min;
				}

				/* Store result: absolute coordinates (note that in restricted mode
				   pmv is (0,0)) */
				mv8_w[pos8]=mv_x;
				mv8_h[pos8]=mv_y;
			}									  /*for block*/

			if (min_error8 < *min_error)
				*min_error = min_error8;
		}
		else
		{										  /* all the four blocks are out */
			pos8=2*j*2*mvm_width + 2*i;      mv8_w[pos8]=mv8_h[pos8]=0.0;
			pos8=2*j*2*mvm_width + 2*i+1;    mv8_w[pos8]=mv8_h[pos8]=0.0;
			pos8=(2*j+1)*2*mvm_width + 2*i;  mv8_w[pos8]=mv8_h[pos8]=0.0;
			pos8=(2*j+1)*2*mvm_width + 2*i+1;mv8_w[pos8]=mv8_h[pos8]=0.0;
			min_error8 = MV_MAX_ERROR;
		}
	}											  /* enable_8x8_mv*/

}


/***********************************************************CommentBegin******
 *
 * -- FindSubPel -- Computes MV with half-pixel accurary	
 *
 * Purpose :
 *      Computes MV with sub-pixel accurary for a 16x16 or 8x8 block
 *
 * Description :
 *    1) The preference for the 16x16 null motion vector must be applied
 *       again (now, the SAD is recomputed for the recontructed interpolated
 *       reference)
 *
 ***********************************************************CommentEnd********/

Void
FindSubPel(
Int    x,										  /* <-- horizontal block coordinate in pixels            */
Int    y,										  /* <-- vertical blocl coordinate in pixels              */
SInt   *prev,                                     /* <-- interpolated reference vop                       */
SInt   *curr,                                	  /* <-- current block                                    */											  
Int    bs_x,              						  /* <-- hor. block size (8/16)                           */
Int    bs_y,									  /* <-- vert. block size (8/16)                          */
Int    comp,									  /* <-- block position in MB (0,1,2,3)                   */
Int    rel_x,									  /* <-- relative horiz. position between curr & prev vops*/
Int    rel_y,									  /* <-- relative verti. position between curr & prev vops*/
Int    pels,									  /* <-- vop width                                        */
Int    lines,									  /* <-- vop height                                       */
Int    edge,									  /* <-- edge                                             */
SInt   *flags,									  /* <-- flags                                            */
SInt   *curr_comp_mb,                             /* <-> motion compensated current mb                    */
Float  *mvx,									  /* <-> horizontal motion vector                         */
Float  *mvy,									  /* <-> vertical motion vector                           */
Int    *min_error								  /* --> prediction error                                 */
)
{
	static PixPoint	search[9] = 
	{	
		{0, 0}, {-1, -1}, {0, -1}, {1, -1},
		{-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}
	};
	/* searching map
		1 2 3
		4 0 5
		6 7 8
	*/

	Int			i, m, n;
	Int			new_x, new_y,
				lx, size_x;								  /* MW QPEL 07-JUL-1998 */
	Int			min_pos;
	Int			AE, AE_min;
	Int			flag_pos;
	SInt		*pRef, *pComp;

	int flag_search[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

	Int SubDimension = 2;

	lx = 2*(pels /*+ 2*edge*/);

	new_x = (Int)((x + *mvx + rel_x)*(SubDimension));
	new_y = (Int)((y + *mvy + rel_y)*(SubDimension));
	new_x += ((comp&1)<<3)*SubDimension;
	new_y += ((comp&2)<<2)*SubDimension;

	size_x=16;

	/** in 1-pixel search we check if we are inside the range; so don't check
		it again
	**/

	/* avoids trying outside the reference image */
	/* we also check with flags if we are inside */
	/* search range (1==don't make 1/x search by */
	/* this side                                 */

	if (bs_x==8)								  
		flag_pos=4+comp*4;
	else										  /* ==16*/
		flag_pos=0;

	if (((new_x/SubDimension) <= 0/*-edge*/) || *(flags+flag_pos)) {
		flag_search[1] = flag_search[4] = flag_search[6] = 0;
	} else if  (((new_x/SubDimension) >= (pels - bs_x /*+ edge*/)) || *(flags+flag_pos+1)) {
		flag_search[3] = flag_search[5] = flag_search[8] = 0;
	};

	if (((new_y/SubDimension) <= 0/*-edge*/) || *(flags+flag_pos+2)) {
		flag_search[1] = flag_search[2] = flag_search[3] = 0;
	} else if  (((new_y/SubDimension) >= (lines- bs_y /*+ edge*/)) || *(flags+flag_pos+3)) {
		flag_search[6] = flag_search[7] = flag_search[8] = 0;
	};

	AE_min = MV_MAX_ERROR;
	min_pos = 0;
	for (i = 0; i < 9; i++)
	{
		if (flag_search[i])
		{
			AE = 0;
			/* estimate on the already interpolated half-pel image */
			pRef = prev + new_x + search[i].x + (new_y + search[i].y) * lx;
			pComp = curr;

			n = bs_y;
			while (n--) {
				m = bs_x;
				while (m--) {
					AE += abs((Int)*pRef - (Int)*pComp);
					pRef += 2;
					pComp ++;
				}
				pRef += 2 * lx - 2 * bs_x;
				pComp += size_x - bs_x;
			}

			if (i==0 && bs_y==16 && *mvx==0 && *mvy==0)
				AE -= (128 + 1);

			if (AE < AE_min)
			{
				AE_min = AE;
				min_pos = i;
			}
		}
		/* else don't look outside the reference */
	}

	/* Store optimal values */
	*min_error = AE_min;
	*mvx += ((Float)search[min_pos].x)/(Float)(SubDimension);
	*mvy += ((Float)search[min_pos].y)/(Float)(SubDimension);

	// generate comp mb data for the minimum point
	pRef = prev + new_x + search[min_pos].x + (new_y + search[min_pos].y) * lx;
	pComp = curr_comp_mb;

	n = bs_y;
	while (n--) {
		m = bs_x;
		while (m--) {
			*(pComp ++) = *pRef;
			pRef += 2;
		}
		pRef += 2 * lx - 2 * bs_x;
		pComp += 16 - bs_x;
	}

	return;
}


