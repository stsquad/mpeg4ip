
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
 *  mot_util.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some utility functions to for motion estimation and */
/* compensation.                                                          */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "mom_structs.h"
#include "vm_common_defs.h"

#include "mot_util.h"

/* ------------------------------------------------------------------------- */

/* Macro to compute the MB absolute error difference of the inside the shape */

//static Int P_diff;
#define DIFF1(v1,v2,idx) (P_diff = (v1[idx]-v2[idx]), ABS(P_diff))

/***********************************************************CommentBegin******
 *
 * -- InterpolateImage -- Interpolates a complete  (SInt)image
 *
 * Purpose :
 *      Interpolates a complete  (SInt) image for easier half pel prediction
 *
 ***********************************************************CommentEnd********/

Void
InterpolateImage(
Image   *input_image,							  /* <-- image to interpolate (SInt) */
Image   *output_image,							  /* --> interpolated image (SInt)  */
Int     rounding_control
)
{
	SInt   *ii, *oo;
	UInt    i, j;
	UInt   width, height;

	width = input_image->x;
	height = input_image->y;
	ii = (SInt*)GetImageData(output_image);
	oo = (SInt*)GetImageData(input_image);

	/* main image */
	for (j = 0; j < height-1; j++)
	{
		for (i = 0; i  < width-1; i++)
		{
			*(ii + (i<<1)) = *(oo + i);
			*(ii + (i<<1)+1) = (*(oo + i) + *(oo + i + 1) + 1- rounding_control)>>1;
			*(ii + (i<<1)+(width<<1)) = (*(oo + i) + *(oo + i + width) + 1-
				rounding_control)>>1;
			*(ii + (i<<1)+1+(width<<1)) = (*(oo+i) + *(oo+i+1) +
				*(oo+i+width) + *(oo+i+1+width) + 2-
				rounding_control)>>2;
		}
		/* last pels on each line */
		*(ii+ (width<<1) - 2) = *(oo + width - 1);
		*(ii+ (width<<1) - 1) = *(oo + width - 1);
		*(ii+ (width<<1)+ (width<<1)-2) = (*(oo+width-1)+*(oo+width+width-1)+1-
			rounding_control)>>1;
		*(ii+ (width<<1)+ (width<<1)-1) = (*(oo+width-1)+*(oo+width+width-1)+1-
			rounding_control)>>1;
		ii += (width<<2);
		oo += width;
	}

	/* last lines */
	for (i = 0; i < width-1; i++)
	{
		*(ii+ (i<<1)) = *(oo + i);
		*(ii+ (i<<1)+1) = (*(oo + i) + *(oo + i + 1) + 1- rounding_control)>>1;
		*(ii+ (width<<1)+ (i<<1)) = *(oo + i);
		*(ii+ (width<<1)+ (i<<1)+1) = (*(oo + i) + *(oo + i + 1) + 1-
			rounding_control)>>1;
	}

	/* bottom right corner pels */
	*(ii + (width<<1) - 2) = *(oo + width -1);
	*(ii + (width<<1) - 1) = *(oo + width -1);
	*(ii + (width<<2) - 2) = *(oo + width -1);
	*(ii + (width<<2) - 1) = *(oo + width -1);

	return;
}


/***********************************************************CommentBegin******
 *
 * -- GetMotionImages --
 *
 * Purpose :
 *      Translate the MVs data from the resulting separated four
 *      (Float) Images (16x16/8x8, X,Y) of the motion estimation process
 *      into two Images, which contain the 16x16 and 8x8 MVs
 *      values acording to the modes (MBM_INTRA, MBM_INTER16,
 *      MBM_INTER8). It also
 *      makes a copy of imode16 (SInt-Image of modes).
 *
 * Return values :
 *      1 on success, -1 on error
 *
 ***********************************************************CommentEnd********/

Int
GetMotionImages(
Image   *imv16_w,								  /* <-- 16x16 horiz. MV Float-Image (MxN) (cuad.)   */
Image   *imv16_h,								  /* <-- 16x16 verti. MV Float-Image   (MxN) (cuad.) */
Image   *imv8_w,								  /* <-- 8x8  horizontal MV Float-Image  (MxN)       */
Image   *imv8_h,								  /* <-- 8x8  vertical MV Float-Image    (MxN)       */
Image   *imode16,								  /* <-- SInt-Image of modes            (M/2xN/2)    */
Image   **mv_x,									  /* --> horizontal MV Float-Image      (MxN)        */
Image   **mv_y,									  /* --> vertical   MV Float-Image      (MxN)        */
Image   **mode									  /* --> SInt-Image of modes           (M/2xN/2)     */
)
{

	Int     i, j;
	Int     width, height, base;
	Float   val_x, val_y;
	Float   *data_x, *data_y,
			*mv16_w, *mv16_h,
			*mv8_w,  *mv8_h;
	SInt    *mode16, *data_mode;
	SInt    modo;

	width = imode16->x; height = imode16->y;

	(*mode)=AllocImage(width,height,SHORT_TYPE);
	(*mv_x)=AllocImage(width*2,height*2,FLOAT_TYPE);
	(*mv_y)=AllocImage(width*2,height*2,FLOAT_TYPE);
	data_x = (Float*)GetImageData((*mv_x));
	data_y = (Float*)GetImageData((*mv_y));
	data_mode = (SInt*)GetImageData((*mode));
	mode16=(SInt*)GetImageData(imode16);
	mv16_w=(Float*)GetImageData(imv16_w);
	mv16_h=(Float*)GetImageData(imv16_h);
	mv8_w=(Float*)GetImageData(imv8_w);
	mv8_h=(Float*)GetImageData(imv8_h);

	for(j=0;j<height;j++)
	{
		for(i=0;i< width;i++)
		{
			modo=data_mode[j*width+i]=mode16[j*width+i];
			if ( modo==MBM_INTRA)				  /*INTRA*/
			{
				base=2*j*2*width+2*i;
				data_x[base]=0.0;  data_x[base+1]=0.0;
				data_y[base]=0.0;  data_y[base+1]=0.0;
				base+=width*2;
				data_x[base]=0.0;  data_x[base+1]=0.0;
				data_y[base]=0.0;  data_y[base+1]=0.0;
			}
			else if(modo==MBM_INTER16)			  /*INTER 16*/
			{
				base=2*j*2*width+2*i;
				val_x=mv16_w[base];val_y=mv16_h[base];

				data_x[base]=val_x; data_x[base+1]=val_x;
				data_y[base]=val_y; data_y[base+1]=val_y;
				base+=width*2;
				data_x[base]=val_x; data_x[base+1]=val_x;
				data_y[base]=val_y; data_y[base+1]=val_y;
			}
			else if (modo==MBM_INTER8)			  /*INTER4*8*/
			{
				base=2*j*2*width+2*i;

				data_x[base]   = mv8_w[base];
				data_y[base]   = mv8_h[base];
				data_x[base+1] = mv8_w[base+1];
				data_y[base+1] = mv8_h[base+1];
				base+=width*2;
				data_x[base]   = mv8_w[base];
				data_y[base]   = mv8_h[base];
				data_x[base+1] = mv8_w[base+1];
				data_y[base+1] = mv8_h[base+1];
			}
		}										  /* end for*/
	}											  /* end for*/

	return(1);
}


/***********************************************************CommentBegin******
 *
 * -- ChooseMode -- chooses coding mode INTRA/INTER dependig on the SAD values
 *
 * Purpose :
 *      chooses coding mode INTRA/INTER dependig on the SAD values
 *
 * Return values :
 *      1 for INTER, 0 for INTRA
 *
 ***********************************************************CommentEnd********/

Int
ChooseMode(
SInt   *curr,									  /* <-- current Y values (not extended)          */
Int    x_pos,									  /* <-- upper-left MB corner hor. coor.          */
Int    y_pos,									  /* <-- upper-left MB corner ver. coor.          */
Int    min_SAD,									  /* <-- min SAD (from integer pel search)        */
UInt   width									  /* <-- current Y picture width                  */
)
{
	Int   i, j;
	Int   MB_mean = 0, A = 0;
	Int   y_off;

	for (j = 0; j < MB_SIZE; j++)
	{
		y_off = (y_pos + j) * width;
		for (i = 0; i < MB_SIZE; i++)
		{
			MB_mean += *(curr + x_pos + i + y_off);
		}
	}

	MB_mean /= 256;

	for (j = 0; j < MB_SIZE; j++)
	{
		y_off = (y_pos + j) * width;
		for (i = 0; i < MB_SIZE; i++)
		{
				A += abs( *(curr + x_pos + i + y_off) - MB_mean );
		}
	}

	if (A < (min_SAD - 2*256))
		return 0;
	else
		return 1;
}


/***********************************************************CommentBegin******
 *
 * -- SAD_Macroblock -- obtains the SAD for a Macroblock
 *
 * Purpose :
 *      obtains the SAD for a Macroblock
 *
 * Return values :
 *      sad of the MB
 *
 ***********************************************************CommentEnd********/

Int
SAD_Macroblock(
SInt   *ii,										  /* <-- Pointer to the upper-left pel of first MB */
SInt   *act_block,								  /* <-- Id, second MB (width=16)                  */
UInt   h_length,								  /* <-- Width of first area                       */
Int    Min_FRAME								  /* <-- Minimum prediction error so far           */
)
{
/*	Int    i;
	Int    sad = 0;
	SInt   *kk;
	register Int P_diff;

	kk = act_block;
	i = 16;
	while (i--)
	{
		sad += (DIFF1(ii,kk,0)+DIFF1(ii,kk,1)
			+DIFF1(ii,kk,2)+DIFF1(ii,kk,3)
			+DIFF1(ii,kk,4)+DIFF1(ii,kk,5)
			+DIFF1(ii,kk,6)+DIFF1(ii,kk,7)
			+DIFF1(ii,kk,8)+DIFF1(ii,kk,9)
			+DIFF1(ii,kk,10)+DIFF1(ii,kk,11)
			+DIFF1(ii,kk,12)+DIFF1(ii,kk,13)
			+DIFF1(ii,kk,14)+DIFF1(ii,kk,15)
			);

		ii += h_length;
		kk += 16;
		if (sad > Min_FRAME)
			return MV_MAX_ERROR;
	}

	return sad;
*/
	int i, j;
	int sad = 0;
	SInt *p1 = ii, *p2 = act_block;

	i = 16;
	while (i--) {
		j = 16;
		while (j --)
			sad += abs((int)*(p1++) - (int)*(p2++));
		if (sad > Min_FRAME)
			return MV_MAX_ERROR;
		p1 += h_length - 16;
	}

	return sad;
}


/***********************************************************CommentBegin******
 *
 * -- SAD_Block -- obtains the SAD for a Block
 *
 * Purpose :
 *      obtains the SAD for a Block
 *
 * Return values :
 *      sad of the Block
 *
 ***********************************************************CommentEnd********/

Int
SAD_Block(
SInt   *ii,										  /* <-- First area                      */
SInt   *act_block,								  /* <-- Id. second MB (width=16)        */
UInt   h_length,								  /* <-- Width of first area             */
Int    min_sofar								  /* <-- Minimum prediction error so far */
)
{
/*	Int    i;
	Int    sad = 0;
	SInt   *kk;
	register Int P_diff;

	kk = act_block;
	i = 8;
	while (i--)
	{
		sad += (DIFF1(ii,kk,0)+DIFF1(ii,kk,1)
			+DIFF1(ii,kk,2)+DIFF1(ii,kk,3)
			+DIFF1(ii,kk,4)+DIFF1(ii,kk,5)
			+DIFF1(ii,kk,6)+DIFF1(ii,kk,7)
			);

		ii += h_length;
		kk += 16;
		if (sad > min_sofar)
			return INT_MAX;
	}

	return sad;
*/
	int i, j;
	int sad = 0;
	SInt *p1 = ii, *p2 = act_block;

	i = 8;
	while (i--) {
		j = 8;
		while (j --)
			sad += abs((int)*(p1++) - (int)*(p2++));
		if (sad > min_sofar)
			return INT_MAX;
		p1 += h_length - 8;
		p2 += 16 - 8;
	}

	return sad;
}



/***********************************************************CommentBegin******
 *
 * -- LoadArea -- fills array with a image-data
 *
 * Purpose :
 *      fills array with a image-data
 *
 * Return values :
 *      Pointer to the filled array
 *
 ***********************************************************CommentEnd********/

Void
LoadArea(
SInt	*im,									  /* <-- pointer to image         */
Int		x,										  /* <-- horizontal pos           */
Int		y,										  /* <-- vertical  pos            */
Int		x_size,									  /* <-- width of array           */
Int		y_size,									  /* <-- height of array          */
Int		lx,										  /* <-- width of the image data  */
SInt	*block									  /* <-> pointer to the array     */
)
{
	SInt   *in;
	SInt   *out;
	Int    i = x_size;
	Int    j = y_size;

	in = im + (y*lx) + x;
	out = block;

	while (j--)
	{
		while (i--)
			*out++ = *in++;
		i = x_size;
		in += lx - x_size;
	}

	return;
}


/***********************************************************CommentBegin******
 *
 * -- SetArea -- fills a image-data with an array
 *
 * Purpose :
 *      fills a image-data with an array
 *
 ***********************************************************CommentEnd********/

Void
SetArea(
SInt   *block,									  /* <-- pointer to array         */
Int    x,										  /* <-- horizontal pos in image  */
Int    y,										  /* <-- vertical  pos in image   */
Int    x_size,									  /* <-- width of array           */
Int    y_size,									  /* <-- height of array          */
Int    lx,										  /* <-- width of the image data  */
SInt   *im										  /* --> pointer to image         */
)
{
	SInt   *in;
	SInt   *out;
	Int    i = x_size;
	Int    j = y_size;

	in  = block;
	out = im + (y*lx) + x;

	while (j--)
	{
		while (i--)
			*out++ = *in++;
		i = x_size;
		out += lx - x_size;
	}
}


