/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

/*!
 *************************************************************************************
 * \file loopFilter.c
 *
 * \brief
 *    Filter to reduce blocking artifacts on a macroblock level.
 *    The filter strengh is QP dependent.
 *
 * \author
 *    Contributors:
 *    - Peter List      Peter.List@t-systems.de:  Original code                             (13.8.2001)
 *    - Jani Lainema    Jani.Lainema@nokia.com:   Some bug fixing, removal of recusiveness  (16.8.2001)
 *************************************************************************************
 */

#include <stdlib.h>
#include <string.h>
#include "global.h"


#define  Clip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

int ALPHA_TABLE[32]  = {128,128,128,128,128,128,128,128,128,128,122, 96, 75, 59, 47, 37,
                         29, 23, 18, 15, 13, 11,  9,  8,  7,  6,  5,  4,  3,  3,  2,  2 } ;
int  BETA_TABLE[32]  = {  0,  0,  0,  0,  0,  0,  0,  0,  3,  3,  3,  4,  4,  4,  6,  6,
                          6,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14  } ;
byte CLIP_TBL[3][32] = {{0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0},
                        {0,0,0,0,0,0,0,0, 0,0,0,1,1,1,1,1, 1,1,1,1,1,2,2,2, 2,3,3,3,3,4,5,5},
                        {0,0,0,0,0,0,0,0, 0,1,1,1,1,1,1,1, 1,2,2,2,2,3,3,3, 4,4,5,5,5,7,8,9}} ;
byte MASK_L[2][16]   = {{3,0,1,2,  7,4,5,6,  11,8,9,10,  15,12,13,14}, {12,13,14,15,  0,1,2,3, 4,5,6,7, 8,9,10,11}} ;
byte MASK_C[2][ 4]   = {{1,0,3,2}, {2,3,0,1}} ;


/*!
 *****************************************************************************************
 * \brief
 *    Filters one edge of 4 (luma) or 2 (chroma) pel
 *****************************************************************************************
 */
void EdgeLoop( byte* ptrOut, int* ptrIn, Macroblock *MbP, Macroblock *MbQ, int VecDif, int dir, int CbpMaskP, int CbpMaskQ, int widthOut, int widthIn, int luma )
{
  int      StrengthP, StrengthQ ;
  int      alpha, beta  ;

  int      pel, ap, aq;
  int      incIn, incIn2, incIn3, incIn4 ;
  int      incOut, incOut2, incOut3 ;
  int      C0, Cq, Cp, c0, n, delta, strong, dif ;
  int      lastPel = luma ? 4 : 2;


  StrengthQ = ((MbQ->intraOrInter != INTER_MB) || (img->types == SP_IMG))? 2: ((MbQ->cbp_blk & CbpMaskQ) != 0)? 1:0 ;  // if not INTRA: has this
  StrengthP = ((MbP->intraOrInter != INTER_MB) || (img->types == SP_IMG))? 2: ((MbP->cbp_blk & CbpMaskP) != 0)? 1:0 ;       // 4x4 block coeffs?

  if( StrengthP || StrengthQ || VecDif )
  {
    alpha   = ALPHA_TABLE[ MbQ->qp ] ;
    beta    = BETA_TABLE [ MbQ->qp ] ;
    Cq      = CLIP_TBL[ StrengthQ ][ MbQ->qp ] ;
    Cp      = CLIP_TBL[ StrengthP ][ MbQ->qp ] ;
    C0      = Cq + Cp ;
    incIn   = dir ?  widthIn : 1 ;                     // vertical filtering increment to next pixel is 1 else width
    incIn2  = incIn<<1 ;
    incIn3  = incIn + incIn2 ;
    incIn4  = incIn<<2 ;
    incOut  = dir ?  widthOut : 1 ;                    // vertical filtering increment to next pixel is 1 else width
    incOut2 = incOut<<1 ;
    incOut3 = incOut + incOut2 ;
    strong  = (MbP != MbQ ) && ((StrengthQ == 2) || (StrengthP == 2)) ; // stronger filtering possible if Macroblock
                                                                                // boundary and one of MB's is Intra
    for( pel=0 ; pel<lastPel ; pel++ )
    {
      delta = abs( ptrIn[0] - ptrIn[-incIn] ) ;
      n     = min( 3, 4-(delta*alpha >> 7) ) ;

      for( aq=1 ; aq<n ; aq++ )
        if( abs(ptrIn[     0] - ptrIn[     aq*incIn]) > beta)
          break ;
      for( ap=1 ; ap<n  ; ap++ )
        if( abs(ptrIn[-incIn] - ptrIn[(-1-ap)*incIn]) > beta)
          break ;

      if( strong & (ap+aq == 6) & (delta < MbQ->qp>>2) & (delta >= 2) )                   // INTRA strong filtering
      {
        ptrOut[ -incOut]   = (25*( ptrIn[-incIn3] +  ptrIn[  incIn]) + 26*( ptrIn[-incIn2] + ptrIn[-incIn ] +  ptrIn[      0 ]) + 64) >> 7 ;
        ptrOut[-incOut2]   = (25*( ptrIn[-incIn4] +  ptrIn[      0]) + 26*( ptrIn[-incIn3] + ptrIn[-incIn2] + ptrOut[ -incOut]) + 64) >> 7 ;
        ptrOut[       0]   = (25*( ptrIn[-incIn2] +  ptrIn[ incIn2]) + 26*( ptrIn[ -incIn] + ptrIn[      0] +  ptrIn[  incIn ]) + 64) >> 7 ;
        ptrOut[  incOut]   = (25*( ptrIn[-incIn ] +  ptrIn[ incIn3]) + 26*(ptrOut[      0] + ptrIn[ incIn ] +  ptrIn[  incIn2]) + 64) >> 7 ;

        if( luma )                                                                  // For luma do two more pixels
        {
          ptrOut[-incOut3] = (25*( ptrIn[-incIn3] + ptrOut[-incOut]) + 26*( ptrIn[-incIn4] + ptrIn[-incIn3] + ptrOut[-incOut2]) + 64) >> 7 ;
          ptrOut[ incOut2] = (25*(ptrOut[      0] +  ptrIn[ incIn2]) + 26*(ptrOut[ incOut] + ptrIn[ incIn2] +  ptrIn[  incIn3]) + 64) >> 7 ;
        }
      }
      else
      {
        if( (ap > 1) && (aq > 1) )                                                             // normal filtering
        {
          c0                 = (C0 + ap + aq) >> 1 ;
          dif                = Clip( -c0,  c0, (((ptrIn[0] - ptrIn[-incIn]) << 2) + (ptrIn[-incIn2] - ptrIn[incIn]) + 4) >> 3 ) ;
          ptrOut[-incOut ]   = Clip(   0, 255, ptrIn[-incIn] + dif ) ;
          ptrOut[      0 ]   = Clip(   0, 255, ptrIn[    0 ] - dif ) ;

          if( ap == 3)
          {
            dif              = Clip( -Cp,  Cp, (ptrIn[-incIn3] + ptrIn[-incIn] - (ptrIn[-incIn2]<<1)) >> 1 ) ;
            ptrOut[-incOut2] = ptrIn[-incIn2] + dif;
          }
          if( aq == 3)
          {
            dif              = Clip( -Cq,  Cq, (ptrIn[ incIn2] + ptrIn[     0] - (ptrIn[  incIn]<<1)) >> 1 ) ;
            ptrOut[  incOut] = ptrIn[  incIn] + dif;
          }
        }
      }

      ptrOut += dir?  1:widthOut ;      // Increment to next set of pixel
      ptrIn  += dir?  1:widthIn  ;
    }
  }
}


/*!
 *****************************************************************************************
 * \brief
 *    returns VecDiff for different Frame types
 *****************************************************************************************
 */
int GetVecDif( ImageParameters *img, int dir, int blk_y, int blk_x )
{

  if( img->type == B_IMG )
  {
    if( img->imod != B_Direct )
      return ( abs( tmp_fwMV[0][blk_y][blk_x+4] - tmp_fwMV[0][blk_y-dir][blk_x+4-!dir]) >= 4) |
             ( abs( tmp_fwMV[1][blk_y][blk_x+4] - tmp_fwMV[1][blk_y-dir][blk_x+4-!dir]) >= 4) |
             ( abs( tmp_bwMV[0][blk_y][blk_x+4] - tmp_bwMV[0][blk_y-dir][blk_x+4-!dir]) >= 4) |
             ( abs( tmp_bwMV[1][blk_y][blk_x+4] - tmp_bwMV[1][blk_y-dir][blk_x+4-!dir]) >= 4)  ;
    else
      return ( abs(     dfMV[0][blk_y][blk_x+4] -     dfMV[0][blk_y-dir][blk_x+4-!dir]) >= 4) |
             ( abs(     dfMV[1][blk_y][blk_x+4] -     dfMV[1][blk_y-dir][blk_x+4-!dir]) >= 4) |
             ( abs(     dbMV[0][blk_y][blk_x+4] -     dbMV[0][blk_y-dir][blk_x+4-!dir]) >= 4) |
             ( abs(     dbMV[1][blk_y][blk_x+4] -     dbMV[1][blk_y-dir][blk_x+4-!dir]) >= 4)  ;
  }
  else
    return   ( abs(   tmp_mv[0][blk_y][blk_x+4] -   tmp_mv[0][blk_y-dir][blk_x+4-!dir]) >= 4 ) |
             ( abs(   tmp_mv[1][blk_y][blk_x+4] -   tmp_mv[1][blk_y-dir][blk_x+4-!dir]) >= 4 ) |
             (         refFrArr[blk_y][blk_x  ]!=    refFrArr[blk_y-dir][blk_x  -!dir]);
}


/*!
 *****************************************************************************************
 * \brief
 *    The main MB-filtering function
 *****************************************************************************************
 */
void DeblockMb(ImageParameters *img, byte **imgY, byte ***imgUV)
{
  int           ofs_x, ofs_y ;
  int           blk_x, blk_y ;
  int           x, y, EdgeX, EdgeY ;
  int           VecDif ;                                    // TRUE if x or ydifference to neighboring mv is >= 4
  int           dir ;                                                         // horizontal or vertical filtering
  int           CbpMaskQ ;
  Macroblock    *MbP, *MbQ ;                                        // the current (MbQ) and neighboring (MbP) mb
  int           bufferY[20*20],  bufferU[12*12], bufferV[12*12];
  int xFirst =  img->block_x? -4:0;
  int yFirst =  img->block_y? -4:0;



  for( dir=0 ; dir<2 ; dir++ )                                           // vertical edges, than horicontal edges
  {
    for( y = yFirst ; y < 16 ; y++ )                        // Copy the original data to be filtered into buffers
      for( x = xFirst ; x < 16 ; x++ )
        bufferY[20*y+x+84] = imgY[img->pix_y+y][img->pix_x+x];

    if (imgUV!=NULL)
      for( y = yFirst ; y < 8 ; y++ )
        for( x = xFirst ; x < 8 ; x++ )
        {
          bufferU[12*y+x+52] = imgUV[0][img->pix_c_y+y][img->pix_c_x+x];
          bufferV[12*y+x+52] = imgUV[1][img->pix_c_y+y][img->pix_c_x+x];
        }

    EdgeX = ( dir || img->block_x)? 0:1 ;
    EdgeY = (!dir || img->block_y)? 0:1 ;


    for( ofs_y=EdgeY ; ofs_y<4 ; ofs_y++ )             // go  vertically through 4x4 blocks
      for( ofs_x=EdgeX ; ofs_x<4 ; ofs_x++ )          // go horicontally through 4x4 blocks
      {
        blk_y    = img->block_y + ofs_y ;                                                 // absolute 4x4 address
        blk_x    = img->block_x + ofs_x ;
        MbQ      = MbP = &img->mb_data[ img->current_mb_nr ] ;                                      // current Mb
        MbP     -= ( !ofs_x && !dir)? 1 : ((!ofs_y && dir)? (img->width>>4) : 0) ;              // neighboring Mb
        VecDif   = GetVecDif( img, dir, blk_y, blk_x ) ;                 // Get Vecdiff for different frame types
        CbpMaskQ = (ofs_y<<2) + ofs_x ;
        EdgeLoop( imgY[blk_y<<2] + (blk_x<<2), bufferY + ((20*ofs_y+ofs_x)<<2) + 84, MbP, MbQ, VecDif, dir, 1<<MASK_L[dir][CbpMaskQ], 1<<CbpMaskQ, img->width, 20, 1 ) ;

        if (imgUV!=NULL)
        {
          if( (!dir && !(ofs_x & 1)) || (dir && !(ofs_y & 1)) )                      // do the same for chrominance
          {
            CbpMaskQ = (ofs_y & 0xfe) + (ofs_x>>1) ;
            EdgeLoop( imgUV[0][blk_y<<1] + ((blk_x)<<1), bufferU + ((12*ofs_y+ofs_x)<<1) + 52, MbP, MbQ, VecDif, dir, 0x010000<<MASK_C[dir][CbpMaskQ], 0x010000<<CbpMaskQ, img->width_cr, 12, 0 ) ;
            EdgeLoop( imgUV[1][blk_y<<1] + ((blk_x)<<1), bufferV + ((12*ofs_y+ofs_x)<<1) + 52, MbP, MbQ, VecDif, dir, 0x100000<<MASK_C[dir][CbpMaskQ], 0x100000<<CbpMaskQ, img->width_cr, 12, 0 ) ;
          }
        }
      }
  }
}
