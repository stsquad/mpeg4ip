/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*   Scalable Shape Coding was provided by:                                 */
/*         Shipeng Li (Sarnoff Corporation),                                */
/*         Dae-Sung Cho (Samsung AIT),					    */
/*         Se Hoon Son (Samsung AIT)	                                    */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/
/************************************************************************/
/*                                               			*/
/* This software module was originally developed by              	*/
/*                                                               	*/
/* Dae-Sung Cho (Samsung AIT),						*/
/* Se Hoon Son (Samsung AIT)						*/
/*                                                               	*/
/* and edited by					              	*/
/* Dae-Sung Cho (Samsung AIT)						*/
/*                                                               	*/
/* in the course of development of the MPEG-4 Video(ISO/IEC 14496-2).  	*/
/* This software module is an implementation of a part of one or 	*/
/* more MPEG-4 Video tools as specified by the                	 	*/
/* MPEG-4 Video(ISO/IEC 14496-2). 					*/
/*									*/
/* ISO/IEC gives users of the MPEG-4 Video(ISO/IEC 14496-2)   	 	*/
/* free license to this software module or modifications thereof 	*/
/* for use in hardware or software products claiming conformance 	*/
/* to the MPEG-4 Video(ISO/IEC 14496-2). 				*/
/*									*/
/* Those intending to use this software  				*/
/* module in hardware or software products are advised that its  	*/
/* use may infringe existing patents. The original developer of  	*/
/* this software module and his/her company, the subsequent      	*/
/* editors and their companies, and ISO/IEC have no liability    	*/
/* for use of this software module or modifications thereof in   	*/
/* an implementation. Copyright is not released for non          	*/
/* MPEG-4 Video(ISO/IEC 14496-2) conforming products. 			*/
/*									*/
/* Samsung AIT (SAIT) retain full right to use the code for 		*/
/* their own purposes, assign or donate the code to a third party 	*/
/* and to inhibit third parties from using the code for non		*/
/* MPEG-4 Video(ISO/IEC 14496-2) conforming products. 			*/
/*									*/
/* This copyright notice must be included in all copies or 		*/
/* derivative works.                             			*/
/* Copyright (c) 1997, 1998                                            	*/
/*                                                               	*/
/************************************************************************/


/***********************************************************HeaderBegin*******
 *                                                                         
 * File:	ShapeEnhCommon.c
 *
 * Author:	Dae-Sung Cho (Samsung AIT)
 * Created:	09-Feb-98
 * Modified by: Shipeng Li (Sarnoff Corporation), Dec 2, 1998.
 * Description: Contains functions used to implement scan interleaving
 *                               coding of spatial scalable binary alpha blocks.
 *
 *
 *                                 
 ***********************************************************HeaderEnd*********/

#include <stdio.h>
//#include <malloc.h>
#include <math.h>

#include "dataStruct.hpp"
#include "ShapeEnhDef.hpp"
//#include "ShapeEnhCommon.h"
                   
Int CVTCCommon::
GetContextEnhBAB_XOR(UChar *curr_bab_data,
		Int x2, 
		Int y2,
		Int width2,
		Int pixel_type)	/* pixel type : 0-P1, 1-P2/P3 */
{
   Int		t=0;
   UChar	*p;

   if(pixel_type==0)	/* P1 pixel */
   {
	p=curr_bab_data + (y2-2) * width2 + x2-1;

	t <<= 1; t |= *p; p++;			/* c6 */
	t <<= 1; t |= *p; p++;			/* c5 */
	t <<= 1; t |= *p; p+=((width2<<1)-2);	/* c4 */  
	t <<= 1; t |= *p; p+=2;			/* c3 */
	t <<= 1; t |= *p; p+=((width2<<1)-2);	/* c2 */
	t <<= 1; t |= *p; p+=2;			/* c1 */
	t <<= 1; t |= *p;			/* c0 */
   } 
   else 		/* P2/P3 band */
   {
	p=curr_bab_data + (y2-1) * width2 + x2-1;

	t <<= 1; t |= *p; p++;			/* c6 */
	t <<= 1; t |= *p; p++;			/* c5 */
	t <<= 1; t |= *p; p+=(width2-2);	/* c4 */  
	t <<= 1; t |= *p; p+=width2;		/* c3 */
	t <<= 1; t |= *p; p++;			/* c2 */ 
	t <<= 1; t |= *p; p++;			/* c1 */ 
	t <<= 1; t |= *p;			/* c0 */
   } 
   return t;
}

Int CVTCCommon::
GetContextEnhBAB_FULL(UChar *lower_bab_data,
		UChar *curr_bab_data,
		Int x2, 
		Int y2,
		Int width,
		Int width2)
{
   Int		t=0;
   Int		x = x2 >> 1, 
		y = y2 >> 1;
   UChar	*p, *q;

   p = lower_bab_data + y*width + x;
   q = curr_bab_data + (y2-1) * width2 + (x2-1);

   t <<= 1; t |= *p; p++;			/* c7 */
   t <<= 1; t |= *p; p+= width-1;		/* c6 */
   t <<= 1; t |= *p; p++;			/* c5 */
   t <<= 1; t |= *p; 				/* c4 */

   t <<= 1; t |= *q; q++;			/* c3 */
   t <<= 1; t |= *q; q++;			/* c2 */
   t <<= 1; t |= *q; q+=(width2-2);		/* c1 */
   t <<= 1; t |= *q; 				/* c0 */

   return t;
}

// SAIT_PDAM BEGIN - ADDED by Dae-Sung Cho (Samsung AIT) 
Int CVTCCommon::DecideScanOrder(UChar *bordered_lower_bab, Int mbsize)
{
        Int scan_order;
        Int border = BORDER;
        Int bsize = mbsize >> 1;
        Int bsize_ext = bsize+(border<<1);
        Int i,j;
        Int num_right=0, num_bottom=0;

        UChar   *lower_bab_data;

        lower_bab_data = bordered_lower_bab + border * bsize_ext + border;

        for(j=0;j<bsize;j++){
			for(i=0;i<bsize;i++){
				if( *(lower_bab_data+j*bsize_ext + i) != *(lower_bab_data+j*bsize_ext + i+1))
                                num_right++;
                
				if( *(lower_bab_data+j*bsize_ext + i) != *(lower_bab_data+(j+1)*bsize_ext + i))
                                num_bottom++;
			}
        }

        if(num_bottom <= num_right)     scan_order = 0;
        else                            scan_order = 1;

        return(scan_order);
}
// SAIT_PDAM END - ADDED by Dae-Sung Cho (Samsung AIT) 

Void  CVTCCommon::      
AddBorderToBABs(UChar *LowShape, 
		UChar *HalfShape,
		UChar *CurShape,
		UChar *lower_bab, 
		UChar *half_bab,
		UChar *curr_bab,
		UChar *bordered_lower_bab, 
		UChar *bordered_half_bab,
		UChar *bordered_curr_bab,
		Int object_width, 	/* object_width in the current layer */
		Int object_height,	/* object_height in the current layer */
		Int blkx, 
		Int blky, 
		Int mblksize, 
		Int max_blkx)

{
    Int i, j, i2, j2, x, y, x2, y2, k, l;

    Int mborder		= MBORDER,
	mblksize_ext	= mblksize+(mborder<<1);/* bordered bab size in the current layer: 20 */
    Int border		= BORDER,
	blksize		= mblksize>>1,		/* bab size in the lower layer : 8 */
	blksize_ext	= blksize+(border<<1);	/* bordered bab size in the lower layer: 8 */
    Int width2 = object_width,
	height2 = object_height,
	width = width2 >> 1,
	height = height2 >> 1;
 
    /*-- Initialize bordered babs -----------------------------------*/
    for ( i=k=0; i<blksize_ext; i++)
	for( j=0; j<blksize_ext; j++, k++)
		bordered_lower_bab[k] = 0;

    for ( i=k=0; i<mblksize_ext; i++)
	for( j=0; j<blksize_ext; j++, k++)
		bordered_half_bab[k] = 0;

    for ( i=k=0; i<mblksize_ext; i++)
	for( j=0; j<mblksize_ext; j++, k++)
		bordered_curr_bab[k] = 0;
	
    /*-- Copy original babs ----------------------------------------*/
    for ( j=k=0, l=border*blksize_ext; j<blksize; j++, l+=blksize_ext ) 
        for ( i=0; i<blksize; i++, k++ ) 
	   bordered_lower_bab[l+i+border] = lower_bab[k];

    for ( j=k=0, l=mborder*blksize_ext; j<mblksize; j++, l+=blksize_ext ) 
        for ( i=0; i<blksize; i++, k++ ) 
	   bordered_half_bab[l+i+border] = half_bab[k];

    for ( j=k=0, l=mborder*mblksize_ext; j<mblksize; j++, l+=mblksize_ext ) 
        for ( i=0; i<mblksize; i++, k++ ) 
	   bordered_curr_bab[l+i+mborder] = curr_bab[k];

    /*----- Top border -----------------------------------------------*/

    /*-- Top border for lower  layer --*/
    x = blkx * blksize - border;
    y = blky * blksize - border;
    for( j=l=0, k=y*width; j<border; j++, k+=width, l+=blksize_ext ) 
    {
          for( i=0; i<blksize_ext; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) 
	    	bordered_lower_bab[l+i] = ( LowShape[k+x+i] != 0);
	  }
    }

   /*-- Top border for half  layer --*/
    x = blkx * blksize - border;
    y = blky * mblksize - mborder;
    for( j=l=0, k=y*width; j<mborder; j++, k+=width, l+=blksize_ext ) 
    {
          for( i=0; i<blksize_ext; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height2) && (0 <= x+i && x+i < width) ) 
	    	bordered_half_bab[l+i] = ( HalfShape[k+x+i] != 0);
	  }
    }

    /*-- Top border for current layer --*/
    x2 = blkx * mblksize - mborder;
    y2 = blky * mblksize - mborder;
    for( j2=l=0, k=y2*width2; j2<mborder; j2++, k+=width2, l+=mblksize_ext ) 
    {
          for( i2=0; i2<mblksize_ext; i2++ ) 
	  {
	    if( (0 <= y2+j2 && y2+j2 < height2) && (0 <= x2+i2 && x2+i2 < width2) ) 
	    	bordered_curr_bab[l+i2] = ( CurShape[k+x2+i2] != 0);
	  }
    }

    /*----- Left border ----------------------------------------------*/

    /*--  Left border for the lower layer --*/
    x = blkx * blksize - border;
    y = blky * blksize;
    for( j=0, l=border*blksize_ext, k=y*width; j<blksize; j++, k+=width, l+=blksize_ext) 
    {
          for( i=0; i<border; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) 
	    	bordered_lower_bab[l+i] = ( LowShape[k+x+i] != 0);
	  }
    }

   /*--  Left border for the half higher layer --*/
    x = blkx * blksize - border;
    y = blky * mblksize;
    for( j=0, l=mborder*blksize_ext, k=y*width; j<mblksize; j++, k+=width, l+=blksize_ext) 
    {
          for( i=0; i<border; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height2) && (0 <= x+i && x+i < width) ) 
	    	bordered_half_bab[l+i] = ( HalfShape[k+x+i] != 0);
	  }
    }

   /*--  Left border for the current layer --*/
    x2 = blkx * mblksize - mborder;
    y2 = blky * mblksize;
    for( j2=0, l=mborder*mblksize_ext, k=y2*width2; j2<mblksize; 
					j2++, k+=width2, l+=mblksize_ext) 
    {
          for( i2=0; i2<mborder; i2++ ) 
	  {
	    if( (0 <= y2+j2 && y2+j2 < height2) && (0 <= x2+i2 && x2+i2 < width2) ) 
	    	bordered_curr_bab[l+i2] = ( CurShape[k+x2+i2] != 0);
	  }
    }

    /*----- Right border ---------------------------------------------*/

    x = (blkx + 1) * blksize;
    y = blky * blksize;
    x2 = (blkx + 1) * mblksize;
    y2 = blky * mblksize;

    /*-- Right border for the lower layer --*/
    for ( j=0, l=border*blksize_ext, k=y*width; j<blksize; j++, k+=width, l+=blksize_ext) 
    {
          for ( i=0; i<border; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) 
	    	bordered_lower_bab[l+i+blksize+border] = ( LowShape[k+x+i] != 0);
          }
    }
   /*----- Right border ---------------------------------------------*/

    x = (blkx + 1) * blksize;
    y = blky * blksize;
    x2 = (blkx + 1) * mblksize;
    y2 = blky * mblksize;

    /*-- Right border for the half layer --*/
    for ( j2=0, l=mborder*blksize_ext; j2<mblksize; j2++, l+=blksize_ext) 
    {
	  j = j2>>1;
	  k = (y+j)*width;

          for ( i=0; i<border; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) {
	    	bordered_half_bab[l+i+blksize+border] = ( LowShape[k+x+i] != 0);
		if(/* LowShape[k+x+i-1] == 0 && */ y+j+1 < height && j2%2==1) { /* avoid the odd subsampling mishap */
		  bordered_half_bab[l+i+blksize+border] =  ( LowShape[k+x+i] != 0) | ( LowShape[k+width+x+i] != 0);
		}
	    }
          }
    }

    /*-- Right border for the currnet layer --*/
    for ( j2=0, l=mborder*mblksize_ext; j2<mblksize; j2++, l+=mblksize_ext) 
    {
	  j = j2>>1;
	  k = (y+j)*width;

          for ( i2=0; i2<mborder; i2++ ) 
	  {
	    i = i2 >> 1;
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) 
	    	bordered_curr_bab[l+i2+mblksize+mborder] = ( LowShape[k+x+i] != 0);
          }
    }
   
    /*----- Bottom border --------------------------------------------*/

    x = blkx * blksize - border;
    y = (blky + 1) * blksize;
    x2 = blkx * mblksize - mborder;
    y2 = (blky + 1) * mblksize;

    /*-- Bottom border for the lower layer --*/
    for ( j=0, l=(blksize+border)*blksize_ext, k=y*width; j<border; 
						j++, k+=width, l+=blksize_ext) 
    {
          for ( i=0; i<blksize_ext; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) 
	    	bordered_lower_bab[l+i] = ( LowShape[k+x+i] != 0);
          }
    }

    /*-- Bottom border for the half layer --*/
    for ( j2=0, l=(mblksize+mborder)*blksize_ext; j2<mborder; j2++, l+=blksize_ext) 
    {
	  j = j2>>1;
	  k = (y+j)*width;
	
          for ( i=0; i<blksize_ext; i++ ) 
	  {
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) 
	    	bordered_half_bab[l+i] = ( LowShape[k+x+i] != 0);
          }
    }

   /*-- Bottom border for the current layer --*/
    for ( j2=0, l=(mblksize+mborder)*mblksize_ext; j2<mborder; j2++, l+=mblksize_ext) 
    {
	  j = j2>>1;
	  k = (y+j)*width;
	
          for ( i2=0; i2<mblksize_ext; i2++ ) 
	  {
	    i = i2>>1;
	    if( (0 <= y+j && y+j < height) && (0 <= x+i && x+i < width) ) 
	    	bordered_curr_bab[l+i2] = ( LowShape[k+x+i] != 0);
          }
    }

    return;
}
