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
//#include "ShapeBaseCodec.h"
//#include "ShapeBaseCommon.h"
//#include "BinArCodec.h"
#include "shape_def.hpp"
//#include "ShapeUtil.h"
//#include "bitpack.h"   /* new */

#define Error -1

//extern   Int ObjectWidth, ObjectHeight;



Int CVTCDecoder::
ShapeBaseDeCoding(UChar *OutShape, Int object_width, Int object_height,
		  Int	change_CR_disable)
{
   /*  Int	alphaTH		  = alphaTH;  */

  Int	cols		  = object_width;
  Int	rows		  = object_height;
  Int	blkn		= 16;
  Int	blkx		= (cols+15)/16;
  Int	blky		= (rows+15)/16;
  SBI	ShapeInf, *infor;
  
  UChar **BAB		= malloc_2d_Char ( blkn, blkn );
  UChar **BABdown	= malloc_2d_Char ( blkn, blkn );
  UChar **BAB_ext	= malloc_2d_Char ( blkn+4, blkn+4 );
  UChar **ip, **shape;

    Int	i, j, k, l;
    Int	smode;
    Int	bsize;
    Int	CR;

    ObjectWidth = object_width;
    ObjectHeight = object_height;

    shape = (UChar **)malloc(sizeof(UChar *)*object_height);
    if(shape == NULL) {
      fprintf(stderr,"Memory allocation failed\n");
      return Error;
    }
    for(i=0;i<object_height;i++) shape[i] = OutShape + i*object_width;

    infor = &ShapeInf;
    infor -> shape_mode = malloc_2d_Int(blky, blkx);
    infor -> CR = malloc_2d_Int(blky, blkx);
    infor -> ST = malloc_2d_Int(blky, blkx);

    infor -> change_CR_disable	= change_CR_disable; 
   /*  infor -> alphaTH			= alphaTH;  */
    infor -> block_size			= blkn;
    infor -> block_num_hor		= blkx;
    infor -> block_num_ver		= blky;

    for ( j=0; j<blky; j++ ) {
    for ( i=0; i<blkx; i++ ) {
        if ( ShapeBaseHeaderDecode ( i, j, blkx, infor ) == Error ) {
            fprintf(stderr,"\n CAE arithmetic decoding Error !\n");
            return  Error;
        }

        smode = infor -> shape_mode[j][i];

        if ( smode == ALL0 || smode == ALL255 ) {

            for ( l=0; l<blkn; l++ ) {
            for ( k=0; k<blkn; k++ ) {
	      if(j*blkn+l < ObjectHeight && i*blkn+k < ObjectWidth)
                shape[j*blkn+l][i*blkn+k] = (Char)( smode == ALL0 ? 0 : 1 );
            }
            }

        } else {

            CR    = infor -> CR[j][i];
            bsize = blkn / CR;

            for ( l=0; l<bsize; l++ ) {
            for ( k=0; k<bsize; k++ ) {
                BABdown [l][k] = (Char)0;
            }
            }

			/* Add surrounding pixels for CAE. */
            AddBorderToBAB ( i, j, blkn, CR, blkx, BABdown, BAB_ext, shape, 2 );

            if ( ShapeBaseContentDecode ( i, j, bsize, BAB_ext, infor ) == Error ) {
                fprintf ( stderr, "\n CAE arithmetic decoding Error !\n" );
                return  Error;
            }

			/* make reconstructed 16x16 shape by BAB */
            for ( l=0; l<bsize; l++ ) {
            for ( k=0; k<bsize; k++ ) {
                BABdown [l][k] = BAB_ext [l+2][k+2];
            }
            }

            if ( CR != 1 ) {
				/* Reconstruct shape from BAB */
                UpSampling_Still  ( i, j, blkn, CR, blkx, BABdown, BAB, shape );
            } else {
                ip = BAB;
                BAB = BABdown;
                BABdown = ip;
                ip = (UChar **)NULL;
            }

			/* store result BAB.  */
            for ( l=0; l<blkn; l++ ) {
            for ( k=0; k<blkn; k++ ) {
	      if(j*blkn+l < ObjectHeight && i*blkn+k < ObjectWidth)
                shape[j*blkn+l][i*blkn+k] = ( BAB[l][k] == 0 ? 0 : 1 );
            }
            }

        }
    }
    }


    free_2d_Char ( BAB, blkn );
    free_2d_Char ( BABdown, blkn );
    free_2d_Char ( BAB_ext, blkn+4 );
    free_2d_Int  (infor->shape_mode, blky); 
    free_2d_Int  (infor->CR, blky); 
    free_2d_Int  (infor->ST, blky); 
    /* ByteAlignmentDec(); */
    free(shape);
    return ( 0 );
}

Int CVTCDecoder::
ShapeBaseContentDecode(Int i,Int j,Int bsize,UChar **BAB,SBI *infor)
{
    Int	k, l;
    Int	dir = infor -> ST[j][i];
    Int	context;
    Int	ctx_max = 1023;
    ArDecoder decoder;

    StartArDecoder_Still( &decoder);

/*    if ( dir == HOR ) { */
    if ( dir == 0 ) {

        for ( l=0; l<bsize; l++ ) {
            for ( k=0; k<bsize; k++ ) {
                context = ( BAB [l+2+(+0)][k+2+(-1)] << 0 ) +
                          ( BAB [l+2+(+0)][k+2+(-2)] << 1 ) +
                          ( BAB [l+2+(-1)][k+2+(+2)] << 2 ) +
                          ( BAB [l+2+(-1)][k+2+(+1)] << 3 ) +
                          ( BAB [l+2+(-1)][k+2+(+0)] << 4 ) +
                          ( BAB [l+2+(-1)][k+2+(-1)] << 5 ) +
                          ( BAB [l+2+(-1)][k+2+(-2)] << 6 ) +
                          ( BAB [l+2+(-2)][k+2+(+1)] << 7 ) +
                          ( BAB [l+2+(-2)][k+2+(+0)] << 8 ) +
                          ( BAB [l+2+(-2)][k+2+(-1)] << 9 );

                if ( context > ctx_max ) {
                    fprintf ( stderr, "\n Shape context Error !\n" );
                    return  Error;
                }
                BAB [l+2][k+2] = ArDecodeSymbol_Still( &decoder, intra_prob[context] );
            }
            BAB [l+2][bsize+2] = BAB [l+2][bsize+3] = BAB [l+2][bsize+1];
        }

    } else {

        for ( k=0; k<bsize; k++ ) {
            for ( l=0; l<bsize; l++ ) {
                context = ( BAB [l+2+(-1)][k+2+(+0)] << 0 ) +
                          ( BAB [l+2+(-2)][k+2+(+0)] << 1 ) +
                          ( BAB [l+2+(+2)][k+2+(-1)] << 2 ) +
                          ( BAB [l+2+(+1)][k+2+(-1)] << 3 ) +
                          ( BAB [l+2+(+0)][k+2+(-1)] << 4 ) +
                          ( BAB [l+2+(-1)][k+2+(-1)] << 5 ) +
                          ( BAB [l+2+(-2)][k+2+(-1)] << 6 ) +
                          ( BAB [l+2+(+1)][k+2+(-2)] << 7 ) +
                          ( BAB [l+2+(+0)][k+2+(-2)] << 8 ) +
                          ( BAB [l+2+(-1)][k+2+(-2)] << 9 );

                if ( context > ctx_max ) {
                    fprintf ( stderr, "\n Shape context Error !\n" );
                    return  Error;
                }
                BAB [l+2][k+2] = ArDecodeSymbol_Still( &decoder, intra_prob[context] );
            }
            BAB [bsize+2][k+2] = BAB [bsize+3][k+2] = BAB [bsize+1][k+2];
        }

    }

    StopArDecoder_Still ( &decoder);

    return ( 0 );
}

Int CVTCDecoder::
ShapeBaseHeaderDecode(Int i,Int j,Int blkx,SBI *infor)
{
    Int	dis_CR = infor -> change_CR_disable;
    Int	UL= ( (i == 0      || j == 0) ? 0 : infor -> shape_mode[j-1][i-1] );
    Int	UR= ( (i == blkx-1 || j == 0) ? 0 : infor -> shape_mode[j-1][i+1] );
    Int	U = ( (j == 0) ? 0 : infor -> shape_mode[j-1][i] );
    Int	L = ( (i == 0) ? 0 : infor -> shape_mode[j][i-1] );
    Int	index = ( 27 * UL + 9 * U + 3 * UR + L );

    Int	mode = 0;
    Int	CR = 1;
    UInt code = 0;
    Int	bits;

    do {
        bits = LMMR_first_shape_code_I [index][mode];
        code = LookBitsFromStream ( bits);

    } while ( code != CMMR_first_shape_code_I [index][mode] && (++mode) <= 2 );
	
    if ( code == CMMR_first_shape_code_I [index][mode] ) {

        BitstreamFlushBits_Still( bits);

        if ( mode == BORDER ) {

            if ( dis_CR == 0 ) {

                do {
                    bits = LCR [CR];
                    code = LookBitsFromStream ( bits );

                } while ( code != CCR[CR] && (CR=(CR<<1)) <= 4 );
	
                if ( code == CCR [CR] ) {
                    BitstreamFlushBits_Still( bits);
                } else {
                    fprintf(stderr,"\n Decode shape information CR Error at [%d, %d] \n", i, j );
                    return  Error;
                }

            }

            bits = LST;
            code = GetBitsFromStream ( bits);

        }
        infor -> shape_mode[j][i] = mode;
        infor -> CR        [j][i] = CR;
        infor -> ST        [j][i] = (Int)code;

    } else {
        fprintf ( stderr, "\n Decode shape mode Error at [%d, %d] \n", i, j );
        return  Error;
    }

    return ( 0 );
}
