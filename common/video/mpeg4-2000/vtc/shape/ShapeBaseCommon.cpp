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
/*                                                                          */
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

#include <stdio.h>
#include "dataStruct.hpp"
//#include "ShapeUtil.h"
//#include "ShapeBaseCodec.h"
//#include "ShapeBaseCommon.h"
/*
extern Void
AddBorderToBAB (Int x,Int y,Int blkn,Int cr,Int MAX_blkx,UChar **BABin,UChar **BABout,Char **shape);
extern Void
AdaptiveUpSampling_Still(UChar **BABdown,UChar **BABresult,Int sub_size);
*/
/* global variables for checking the boundary of
object */
//Int ObjectWidth, ObjectHeight; 

Void CVTCCommon::
UpSampling_Still(Int x,Int y,Int blkn,Int cr,Int blkx,UChar **buff,UChar**data,UChar **shape)
{
    Int	i, j;

    Int b_size=blkn/cr;

    if ( b_size==16 ) {
        for ( j=0; j<b_size; j++ ) {
        for ( i=0; i<b_size; i++ ) {
            data[j][i] = buff[j][i];
        }
        }
    } else if ( b_size==8 ) {
        UChar **down_tmp = (UChar**) malloc_2d_Char( blkn + 4, blkn + 4);
        AddBorderToBAB(x,y,blkn,cr,blkx,buff,down_tmp,shape,0);
        AdaptiveUpSampling_Still(data,down_tmp,8);
        free(down_tmp);
    } else if ( b_size==4 ) {
        UChar **down_tmp = (UChar **)malloc_2d_Char( blkn + 4, blkn + 4);
        UChar **up_tmp   = (UChar **)malloc_2d_Char( blkn    , blkn    );
        AddBorderToBAB(x,y,blkn,cr,blkx,buff,down_tmp,shape,0);
        AdaptiveUpSampling_Still(down_tmp,up_tmp,4);
        AddBorderToBAB(x,y,blkn,cr/2,blkx,up_tmp,down_tmp,shape,0);
        AdaptiveUpSampling_Still(down_tmp,data,8);
        free(down_tmp);
        free(up_tmp);
    } else {
        fprintf(stderr,"UpSampling():Wrong UpSampling Size (%d->16)\n",b_size);
        exit( 0 );
    }
}

Int	CVTCCommon::
GetContext ( Char a, Char b, Char c, Char d, Char e, Char f, Char g, Char h )
{
	return (Int) ( a + (b<<1) + (c<<2) + (d<<3) + (e<<4) + (f<<5) + (g<<6) + (h<<7) );
}

Char CVTCCommon::
	GetShapeVL ( Char a, Char b, Char c, Char d, Char e, Char f, 
									   Char g, Char h, Char i, Char j, Char k, Char l, Int t )
{
	return (Char) ( ( (Int)(a<<2) + (Int)((b+c+d)<<1) + (Int)(e+f+g+h+i+j+k+l) ) > t );
}



Void CVTCCommon::
AdaptiveUpSampling_Still(UChar **BABdown,UChar **BABresult,Int sub_size)
{
  static Int
    Th[256] = {
      3, 6, 6, 7, 4, 7, 7, 8, 6, 7, 5, 8, 7, 8, 8, 9,
      6, 5, 5, 8, 5, 6, 8, 9, 7, 6, 8, 9, 8, 7, 9,10,
      6, 7, 7, 8, 7, 8, 8, 9, 7,10, 8, 9, 8, 9, 9,10,
      7, 8, 6, 9, 6, 9, 9,10, 8, 9, 9,10,11,10,10,11,
      6, 9, 5, 8, 5, 6, 8, 9, 7,10,10, 9, 8, 7, 9,10,
      7, 6, 8, 9, 8, 7, 7,10, 8, 9, 9,10, 9, 8,10, 9,
      7, 8, 8, 9, 6, 9, 9,10, 8, 9, 9,10, 9,10,10, 9,
      8, 9,11,10, 7,10,10,11, 9,12,10,11,10,11,11,12,
      6, 7, 5, 8, 5, 6, 8, 9, 5, 6, 6, 9, 8, 9, 9,10,
      5, 8, 8, 9, 6, 7, 9,10, 6, 7, 9,10, 9,10,10,11,
      7, 8, 6, 9, 8, 9, 9,10, 8, 7, 9,10, 9,10,10,11,
      8, 9, 7,10, 9,10, 8,11, 9,10,10,11,10,11, 9,12,
      7, 8, 6, 9, 8, 9, 9,10,10, 9, 7,10, 9,10,10,11,
      8, 7, 7,10, 7, 8, 8, 9, 9,10,10,11,10,11,11,12,
      8, 9, 9,10, 9,10,10, 9, 9,10,10,11,10,11,11,12,
      9,10,10,11,10,11,11,12,10,11,11,12,11,12,12,13 };
  /* --  E  F -- */
  /*  L  A  B  G */
  /*  K  D  C  H */
  /* --  J  I -- */
  
  /*   A   B   C   D   E   F   G   H   I   J   K   L	*/
  static Int	xx [12] = { -1, +0, +0, -1, -1, +0, +1, +1, +0, -1, -2, -2, };
  static Int	yy [12] = { -1, -1, +0, +0, -2, -2, -1, +0, +1, +1, +0, -1, };
  
  register Int	i, j, k, l, m;
  
  Char		val[12];
  Int			blk_size  = sub_size;
  Int			context;
  Int			iup, jup;
  Int			i2, j2;
  Int			ks, ke, ls, le;
  
  
  for	( j=0; j<=blk_size; j++ ) {
    for	( i=0; i<=blk_size; i++ ) {
      
      for( m=0; m<12; m++ )
	val[m] = ( BABdown [yy[m]+j+2] [xx[m]+i+2] != 0 );
      
      if ( i == 0        ) ks =  0;
      else                 ks = -1;
      if ( i == blk_size ) ke =  0; 
      else                 ke =  1;
      
      if ( j == 0        ) ls =  0;
      else                 ls = -1;
      if ( j == blk_size ) le =  0; 
      else                 le =  1;
      
      i2 = i << 1;
      j2 = j << 1;
      
      for ( l=ls; l<le; l++ ) {
	for ( k=ks; k<ke; k++ ) {
	  
	  iup = i2 + k;
	  jup = j2 + l;
	  
	  if( (iup & 1) == 1 && (jup & 1) == 1 ) { /* P1 case : */
	    
	    context = GetContext (	val[5],val[4],val[11],val[10],val[9],val[8],val[7],val[6] );
	    
	    BABresult [jup][iup]
	      = GetShapeVL (	val[0],
				val[1],val[2],val[3],
				val[4],val[5],val[6],val[7],val[8],val[9],val[10],val[11],
				Th [context] );
	    
	  } else
	    if( (iup & 1) == 0 && (jup & 1) == 1 ) { /* P2 case : */
	      
	      context	= GetContext (	val[7],val[6],val[5],val[4],val[11],val[10],val[9],val[8] );
	      
	      BABresult [jup][iup]
		= GetShapeVL (	val[1],
				val[0],val[2],val[3],
				val[4],val[5],val[6],val[7],val[8],val[9],val[10],val[11],
				Th [context] );
	      
	    } else
	      if( (iup & 1) == 1 && (jup & 1) == 1 ) { /* P3 case : */
		
		context	= GetContext  (	val[9],val[8],val[7],val[6],val[5],val[4],val[11],val[10] );
		
		BABresult [jup][iup]
		  = GetShapeVL (	val[2],
					val[0],val[1],val[3],
					val[4],val[5],val[6],val[7],val[8],val[9],val[10],val[11],
					Th [context] );
		
	      } else {								/* P4 case : */
		
		context = GetContext  (	val[11],val[10],val[9],val[8],val[7],val[6],val[5],val[4] );
		
		BABresult [jup][iup]
		  = GetShapeVL (	val[3],
					val[0],val[1],val[2],
					val[4],val[5],val[6],val[7],val[8],val[9],val[10],val[11],
					Th [context] );
		
	      }
	}
      }
    }
  }
  return;
}


Void CVTCCommon::
DownSampling_Still(UChar **buff,UChar **data,Int b_size,Int s_size)
{
    Int	i, j, k, l;
    Int	dat;
 
    if ( s_size == 1 ) {;
        for ( j=0; j<b_size; j++ ) {
        for ( i=0; i<b_size; i++ ) {
            data[j][i] = buff[j][i];
        }
        }
    } else {
        for ( j=0; j<b_size; j++ ) {
        for ( i=0; i<b_size; i++ ) {
            dat = 0;
            for ( l=0; l<s_size; l++ ) {
            for ( k=0; k<s_size; k++ ) {
                dat += buff[j*s_size+l][i*s_size+k];
            }
            }
            data[j][i] = ( (dat*2)<(s_size*s_size) ) ? 0 : 1;
        }
        }
    }
    return;
}

Void CVTCCommon::
AddBorderToBAB(Int blkx,Int blky,Int blksize,Int cr_size,Int MAX_blkx,UChar **BABinput,UChar **BABresult,UChar **shape, Int flag )
{
    Int	blkn = blksize/cr_size;
    Int	i, j, k, tmp;

    for ( j=0; j<blkn+4; j++ )
    for ( i=0; i<blkn+4; i++ )
        BABresult[j][i] = 0;

    for ( j=0; j<blkn; j++ )
    for ( i=0; i<blkn; i++ )
        BABresult[j+2][i+2] = BABinput[j][i];

	/*  Up-left  */
    if ( blkx!=0 && blky!=0 ) {
        Int	X = blkx * blksize - 2;
        Int	Y = blky * blksize - 2;
        for ( j=0; j<2; j++ ) {
        for ( i=0; i<2; i++ ) {
	  if(Y+j < ObjectHeight && X+i < ObjectWidth)
            BABresult[j][i] = ( shape[Y+j][X+i]!=0 );
        }
        }
    }

	/*  up  */
    if ( blky!=0 ) {
        Int	X = blkx * blksize;
        Int	Y = blky * blksize - 2;
        for ( j=0; j<2   ; j++ ) {
        for ( i=0; i<blkn; i++ ) {
            tmp = 0;
            for ( k=0; k<cr_size; k++ ) 
	      if(Y+j < ObjectHeight && X+i*cr_size+k < ObjectWidth)
                tmp += ( shape[Y+j][X+i*cr_size+k]!=0 );
	    
            BABresult[j][i+2] = ( (tmp*2 < cr_size) ? 0 : 1 );
        }
        }
    }

	/*  left  */
    if ( blkx!=0 ) {
        Int	X = blkx * blksize - 2;
        Int	Y = blky * blksize;
        for ( j=0; j<blkn; j++ ) {
        for ( i=0; i<2   ; i++ ) {
            tmp = 0;
            for ( k=0; k<cr_size; k++ ) 
	      if(Y+j*cr_size+k < ObjectHeight && X+i < ObjectWidth)
                tmp += ( shape[Y+j*cr_size+k][X+i]!=0 );
            BABresult[j+2][i] = ( (tmp*2 < cr_size) ? 0 : 1 );
        }
        }
    }

	/*  up-right  */
    if ( blky!=0 && blkx<MAX_blkx-1 ) {
        Int	X = (blkx + 1) * blksize;
        Int	Y = blky * blksize - 2;
        for ( j=0; j<2; j++ ) {
        for ( i=0; i<2; i++ ) {
	  if(Y+j < ObjectHeight && X+i < ObjectWidth)
            BABresult[j][blkn+2+i] = ( shape[Y+j][X+i]!=0 );
        }
        }
    }

	/* padding right & bottom border pixels unknown at decoding time. */
	if ( flag != 2 ) {		/*  for up sampling & encoding */

		for	( i=0; i<blkn; i++ ) {
			BABresult [i+2][blkn+3] =
			BABresult [i+2][blkn+2] = BABresult [i+2][blkn+1];
			BABresult [blkn+3][i+2] =
			BABresult [blkn+2][i+2] = BABresult [blkn+1][i+2];
		}

	}

	if ( flag == 0 ) {		/* for up sampling */

		for	( j=0; j<2; j++ ) {
			BABresult [blkn+j+2][0] =
			BABresult [blkn+j+2][1] = BABresult [blkn+j+2][2];
			BABresult [blkn+j+2][blkn+3] =
			BABresult [blkn+j+2][blkn+2] = BABresult [blkn+j+2][blkn+1];
		}

	} else {

		for	( i=0; i<2; i++ ) {
			BABresult [blkn+3][i] =
			BABresult [blkn+2][i] = BABresult [blkn+1][i];
			BABresult [blkn+i+2][blkn+3] =
			BABresult [blkn+i+2][blkn+2] = 0;
		}

	}

    return;
}
