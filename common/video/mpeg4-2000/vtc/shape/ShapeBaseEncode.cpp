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
#include <memory.h>
#include <string.h>
#include "dataStruct.hpp"
//#include "ShapeBaseCodec.h"
//#include "ShapeBaseCommon.h"
//#include "BinArCodec.h"
#include "shape_def.hpp"
//#include "ShapeUtil.h"
//#include "bitpack.h"


#define Error -1



Int CVTCEncoder::
ShapeBaseEnCoding(UChar *InShape, Int object_width, Int object_height, Int alphaTH, Int change_CR_disable)
{
    Int	blks		= 4;
    Int	blkn		= 1<<blks;
    Int	blkx		= (object_width+15)/16;
    Int	blky		= (object_height+15)/16;
    SBI	ShapeInf, *infor;

    UChar **BAB		= malloc_2d_Char(blkn,blkn);
    UChar **BABdown	= malloc_2d_Char(blkn,blkn);
    UChar **BAB_ext	= malloc_2d_Char(blkn+4,blkn+4);
    UChar **shape;
    Int	i, j, k, l, status;
    Int	bsize;
    Int	CR;
	
    ObjectWidth = object_width;
    ObjectHeight = object_height;
	ShapeBitstream=NULL;
	ShapeBitstreamLength=0;

    shape = (UChar **)malloc(sizeof(UChar *)*object_height);
    if(shape == NULL) {
      fprintf(stderr,"Memory allocation failed\n");
      return Error;
    }
    for(i=0;i<object_height;i++) shape[i] = InShape + i*object_width;

    infor = &ShapeInf;
    infor->BitStream = ShapeBitstream = (BSS *)malloc(sizeof(BSS));
    if(infor->BitStream==NULL) {
      fprintf(stderr,"Memory allocation failed\n");
      return Error;
    }
    infor->BitStream->bs = (UChar *)malloc(sizeof(UChar)*object_width*object_height);
    if(infor->BitStream->bs==NULL) {
      fprintf(stderr,"Memory allocation failed\n");
      return Error;
    }
	/* clear the output buffer */
    memset(infor->BitStream->bs, (UChar )0, object_width*object_height);
    infor -> change_CR_disable	= change_CR_disable; 
    infor -> alphaTH			= alphaTH; 
    infor -> block_size			= blkn;
    infor -> block_num_hor		= blkx;
    infor -> block_num_ver		= blky;
    infor -> shape_mode = malloc_2d_Int(blky, blkx);
    infor -> CR = malloc_2d_Int(blky, blkx);
    infor -> ST = malloc_2d_Int(blky, blkx);
    InitBitstream(1,infor->BitStream);
    for ( j=0; j<blky; j++ ) {
      for ( i=0; i<blkx; i++ ) {
	/* get BAB. */
    	for ( l=0; l<blkn; l++ ) {
	  for ( k=0; k<blkn; k++ ) {
            BAB[l][k] = ((((j<<blks)+l)<ObjectHeight)
			 && (((i<<blks)+k) <ObjectWidth))?
	      ( shape[(j<<blks)+l][(i<<blks)+k]!=0 ):0;
	  }
    	}
	/* check content of BAB. */
        status = decide_CR(i,j,blkn,blkx,BAB,BABdown,change_CR_disable,alphaTH,
                           shape);
        if ( status==ALL0 || status==ALL255 ) {
	  
			/* All 0 or 255, hence no CAE is needed.  */
            infor -> shape_mode[j][i] = status;
            infor -> CR        [j][i] = 1;
            infor -> ST        [j][i] = 0;

            if ( ShapeBaseHeaderEncode(i,j,blkx,infor)==Error ) {
                fprintf(stderr,"\n CAE arithmetic coding Error !\n");
                return  Error;
            }

            for ( l=0; l<blkn; l++ ) {
            for ( k=0; k<blkn; k++ ) {
	      if((j<<blks)+l < ObjectHeight && (i<<blks)+k < ObjectWidth)
                shape[(j<<blks)+l][(i<<blks)+k] = ( status==ALL0 ? 0 : 1 );
            }
            }

        } else {

            infor -> shape_mode[j][i] = BORDER;
            infor -> CR        [j][i] = CR = ( 1<<(status-CR1_1) );

            if ( ShapeBaseHeaderEncode(i,j,blkx,infor) == Error ) {
                fprintf(stderr,"\n CAE arithmetic coding Error !\n");
                return  Error;
            }

			/* Add surrounding pixels for CAE. */
            AddBorderToBAB(i,j,blkn,CR,blkx,BABdown,BAB_ext,shape,1);	/* modified by Z. Wu @ OKI	*/

            bsize = blkn/CR;
            if ( ShapeBaseContentEncode(i,j,bsize,BAB_ext,infor) == Error ) {
                fprintf(stderr,"\n CAE arithmetic coding Error !\n");
                return  Error;
            }

            if ( CR!=1 ) {
				/* Reconstruct shape from BAB */
                UpSampling_Still(i,j,blkn,CR,blkx,BABdown,BAB,shape);
            }

            for ( l=0; l<blkn; l++ ) {
            for ( k=0; k<blkn; k++ ) {
	      if((j<<blks)+l < ObjectHeight && (i<<blks)+k < ObjectWidth)
                shape[(j<<blks)+l][(i<<blks)+k] = ( BAB[l][k]==0 ? 0 : 1 );
            }
            }

        }
    }
    }

    ShapeBitstreamLength=infor -> BitStream_Length	= infor -> BitStream -> cnt;

    free_2d_Char ( BAB, blkn );
    free_2d_Char ( BABdown, blkn );
    free_2d_Char ( BAB_ext, blkn+4 );
    free_2d_Int  (infor->shape_mode, blky); 
    free_2d_Int  (infor->CR, blky); 
    free_2d_Int  (infor->ST, blky); 
/*   printf("%x\n", *(infor->BitStream->bs)); */
   
    free(shape);
    return ( infor->BitStream_Length );
}

/* Merge Shape Bitstream Into main bit stream */
Void CVTCEncoder::
MergeShapeBaseBitstream()
{
  if(ShapeBitstream==NULL) {
    fprintf(stderr,"ShapeBitStream Not Available\n");
    exit(1);
  }
  InitBitstream(0,ShapeBitstream);
  BitStreamMerge(ShapeBitstreamLength, ShapeBitstream);
 /*  ByteAlignmentEnc(); */
/*   printf("%x\n", *(ShapeBitstream->bs)); */

  free(ShapeBitstream->bs);
  free(ShapeBitstream);
  ShapeBitstream = NULL;
}

 Int CVTCEncoder::
CheckBABstatus(Int blkn,UChar **BAB1,UChar **BAB2,Int alphaTH)
{
    Int	i, j, k, l;
    Int	SAD;
    Int	all0   = 0;
    Int	all255 = 0;

    for ( j=0; j<blkn; j+=4 ) {
    for ( i=0; i<blkn; i+=4 ) {

        SAD = 0;
        if ( BAB2==(UChar **)NULL ) {

            for ( l=0; l<4; l++ )
            for ( k=0; k<4; k++ )
                SAD += ( BAB1[j+l][i+k]!=0 );

            if ( 16 *     SAD  > alphaTH ) all0   = 1;
            if ( 16 * (16-SAD) > alphaTH ) all255 = 1;

            if ( all0==1 && all255==1 )
                return (BORDER);

        } else {

            for ( l=0; l<4; l++ )
            for ( k=0; k<4; k++ )
                SAD += ( BAB1[j+l][i+k]!=BAB2[j+l][i+k] );

            if ( 16 * SAD > alphaTH )
                return (BORDER);

        }
    }
    }
    if ( all0==0 ) return ( ALL0   );
    else           return ( ALL255 );
}


 Int CVTCEncoder::
decide_CR(Int x,Int y,Int blkn,Int blkx, UChar **BAB_org,UChar **BAB_dwn,Int change_CR_disable,Int alphaTH,UChar **shape)
{
    Int		status;
    Int		i, j;

	/* estimation of all_0 or all_255 or gray at 16x16 first. */
    if ( (status=CheckBABstatus(blkn,BAB_org,(UChar **)NULL,alphaTH))!=BORDER )
        return (status);

    if ( change_CR_disable == 0 ) {
        UChar	**BAB_up = malloc_2d_Char ( blkn, blkn );

		/* estimation of CR = 1/4 conversion error. */
        DownSampling_Still(BAB_org,BAB_dwn,blkn/4,4);
    	UpSampling_Still(x,y,blkn,4,blkx,BAB_dwn,BAB_up,shape);

        if ( CheckBABstatus(blkn,BAB_org,BAB_up,alphaTH)!=BORDER ) {
            free(BAB_up);
            return (CR1_4 );
        }

		/* estimation of CR = 1/2 conversion error. */
        DownSampling_Still ( BAB_org, BAB_dwn, blkn/2, 2 );
        UpSampling_Still   ( x, y, blkn, 2, blkx, BAB_dwn, BAB_up, shape );

        if ( CheckBABstatus ( blkn, BAB_org, BAB_up, alphaTH ) != BORDER ) {
            free ( BAB_up );
            return (CR1_2 );
        }
    }
    for ( j=0; j<blkn; j++ ) {
    for ( i=0; i<blkn; i++ ) {
        BAB_dwn[j][i] = BAB_org[j][i];
    }
    }
    return (CR1_1);
}

 Int CVTCEncoder::
ShapeBaseContentEncode(Int i,Int j,Int bsize,UChar **BAB,SBI *infor)
{
    Int		k, l;
    Int		dir, dirMin=0;
    Int		len, lenMin = INT_MAX;
    Int		context;
    Char	target;
    Int		ctx_max = 1023;
    BSS		*bitstream[2];
    ArCoder	coder;


    for ( dir=0; dir<2; dir++ ) {

        bitstream[dir]		  = (BSS *)malloc( sizeof( BSS ) );
        bitstream[dir] -> bs  = (UChar *) malloc(bsize*bsize*sizeof(UChar));

	memset( bitstream[dir] -> bs, (UChar)0, bsize*bsize); 

        InitBitstream ( 1, bitstream[dir] );

/*        if ( dir == HOR ) { */
        if ( dir == 0 ) {

            StartArCoder_Still ( &coder );

            for ( l=0; l<bsize; l++ ) {
            for ( k=0; k<bsize; k++ ) {
                target  = BAB [l+2][k+2];
                context =	( BAB [l+2+(+0)][k+2+(-1)] << 0 ) +
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
                ArCodeSymbol_Still( &coder, bitstream[dir], target, intra_prob[context] );	
            }
            }
            StopArCoder_Still( &coder, bitstream[dir] );
	    
            if ( (len  = bitstream[dir] -> cnt) < lenMin ) {
                lenMin = len;
                dirMin = dir;
            }

        } else {

            StartArCoder_Still ( &coder );

            for ( k=0; k<bsize; k++ ) {
            for ( l=0; l<bsize; l++ ) {
                target  = BAB [l+2][k+2];
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
                ArCodeSymbol_Still( &coder, bitstream[dir], target, intra_prob[context] );	

            }
            }
            StopArCoder_Still( &coder, bitstream[dir] );

            if ( (len  = bitstream[dir] -> cnt) < lenMin ) {
                lenMin = len;
                dirMin = dir;
            }

        }

    }

    infor -> ST[j][i] = dirMin;
	
    PutBitstoStream ( LST, (UInt)dirMin, infor -> BitStream );

    InitBitstream ( 0, bitstream[dirMin] );

    BitStreamCopy ( lenMin, bitstream[dirMin], infor -> BitStream );

    for ( dir=0; dir<2; dir++ ) {
        free ( bitstream[dir] -> bs );
        free ( bitstream[dir] );
    }

    return ( 0 );
}

Int CVTCEncoder::
ShapeBaseHeaderEncode(Int i,Int j,Int blkx,SBI *infor)
{
    Int	mode		= infor -> shape_mode[j][i];
    Int	dis_CR		= infor -> change_CR_disable;
    BSS	*bitstream	= infor -> BitStream;

    Int	UL = ( (i == 0      || j == 0) ? 0 : infor -> shape_mode[j-1][i-1] );
    Int	UR = ( (i == blkx-1 || j == 0) ? 0 : infor -> shape_mode[j-1][i+1] );
    Int	U  = ( (j == 0) ? 0 : infor -> shape_mode[j-1][i] );
    Int	L  = ( (i == 0) ? 0 : infor -> shape_mode[j][i-1] );
    Int		index	= ( 27 * UL + 9 * U + 3 * UR + L );
    Int		bits;
    UInt	code;

    bits = LMMR_first_shape_code_I [index][mode];
    code = CMMR_first_shape_code_I [index][mode];

    PutBitstoStream ( bits, code, bitstream );

    if ( mode == BORDER ) {
        if ( dis_CR == 0 ) { 
            bits = LCR [infor -> CR[j][i]];
            code = CCR [infor -> CR[j][i]];

            PutBitstoStream ( bits, code, bitstream );
        }
    }

    return ( 0 );
}

