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
 * \file mv-search.c
 *
 * \brief
 *    Motion Vector Search, unified for B and P Pictures
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *      - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *      - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *      - Jani Lainema                    <jani.lainema@nokia.com>
 *      - Detlev Marpe                    <marpe@hhi.de>
 *      - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *      - Heiko Schwarz                   <hschwarz@hhi.de>
 *
 *************************************************************************************
 * \todo
 *   6. Unite the 1/2, 1/4 and1/8th pel search routines into a single routine       \n
 *   7. Clean up parameters, use sensemaking variable names                         \n
 *   9. Implement fast leaky search, such as UBCs Diamond search
 *
 *************************************************************************************
*/

#include "contributors.h"

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "global.h"
#include "mv-search.h"
#include "refbuf.h"


// Local function declarations

__inline int ByteAbs (int foo);

// These procedure pointers are used by motion_search() and one_eigthpel()
static pel_t (*PelY_18) (pel_t**, int, int);
static pel_t (*PelY_14) (pel_t**, int, int);
static pel_t *(*PelYline_11) (pel_t *, int, int);

//! The Spiral for spiral search
static int SpiralX[6561];
static int SpiralY[6561];

/*! The bit usage for MVs (currently lives in img-> mv_bituse and is used by
    b_frame.c, but should move one day to a more appropriate place */
static int *MVBitUse;

// Statistics, temporary


/*!
 ***********************************************************************
 * \brief
 *    MV Cost returns the initial penalty for the motion vector representation.
 *    Used by the various MV search routines for MV RD optimization
 *
 * \param ParameterResolution
 *    the resolution of the vector parameter, 1, 2, 4, 8, eg 4 means 1/4 pel
 * \param BitstreamResolution
 *    the resolyution in which the bitstream is coded
 * \param ParameterResolution
 *    TargetResolution
 * \param pred_x, pred_y
 *    Predicted vector in BitstreamResolution
 * \param Candidate_x, Candidate_y
 *    Candidate vector in Parameter Resolution
 ***********************************************************************
 */
static int MVCost (int ParameterResolution, int BitstreamResolution,
                   int Blocktype, int qp,
                   int Pred_x, int Pred_y,
                   int Candidate_x, int Candidate_y)
{

  int Factor = BitstreamResolution/ParameterResolution;
  int Penalty;
  int len_x, len_y;

  len_x = Candidate_x * Factor - Pred_x;
  len_y = Candidate_y * Factor - Pred_y;

  Penalty = (QP2QUANT[qp] * ( MVBitUse[absm (len_x)]+MVBitUse[absm(len_y)]) );

  if (img->type != B_IMG && Blocktype == 1 && Candidate_x == 0 && Candidate_y == 0)   // 16x16 and no motion
    Penalty -= QP2QUANT [qp] * 16;
  return Penalty;
}


/*!
 ***********************************************************************
 * \brief
 *    motion vector cost for rate-distortion optimization
 ***********************************************************************
 */
static int MVCostLambda (int shift, double lambda,
       int pred_x, int pred_y, int cand_x, int cand_y)
{
  return (int)(lambda * (MVBitUse[absm((cand_x << shift) - pred_x)] +
       MVBitUse[absm((cand_y << shift) - pred_y)]   ));
}


/*!
 ***********************************************************************
 * \brief
 *    setting the motion vector predictor
 ***********************************************************************
 */
void
SetMotionVectorPredictor (int pred[2], int **refFrArr, int ***tmp_mv,
        int ref_frame, int mb_x, int mb_y, int blockshape_x, int blockshape_y)
{
  int block_x     = mb_x/BLOCK_SIZE;
  int block_y = mb_y/BLOCK_SIZE;
  int pic_block_x = img->block_x + block_x;
  int pic_block_y = img->block_y + block_y;
  int mb_nr = img->current_mb_nr;
  int mb_width = img->width/16;
  int mb_available_up      = (img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width]);
  int mb_available_left    = (img->mb_x == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-1]);
  int mb_available_upleft  = (img->mb_x == 0 || img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width-1]);
  int mb_available_upright = (img->mb_x >= mb_width-1 || img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width+1]);
  int block_available_up, block_available_left, block_available_upright, block_available_upleft;
  int mv_a, mv_b, mv_c, mv_d, pred_vec=0;
  int mvPredType, rFrameL, rFrameU, rFrameUR;
  int hv;

  // D B C
  // A X

  // 1 A, B, D are set to 0 if unavailable
  // 2 If C is not available it is replaced by D
  block_available_up   = mb_available_up   || (mb_y > 0);
  block_available_left = mb_available_left || (mb_x > 0);

  if (mb_y > 0)
    block_available_upright = mb_x+blockshape_x != MB_BLOCK_SIZE ? 1 : 0;
  else if (mb_x+blockshape_x != MB_BLOCK_SIZE)
    block_available_upright = block_available_up;
  else
    block_available_upright = mb_available_upright;

  if (mb_x > 0)
    block_available_upleft = mb_y > 0 ? 1 : block_available_up;
  else if (mb_y > 0)
    block_available_upleft = block_available_left;
  else
    block_available_upleft = mb_available_upleft;

  mvPredType = MVPRED_MEDIAN;

  rFrameL    = block_available_left    ? refFrArr[pic_block_y][pic_block_x-1]   : -1;
  rFrameU    = block_available_up      ? refFrArr[pic_block_y-1][pic_block_x]   : -1;
  rFrameUR   = block_available_upright ? refFrArr[pic_block_y-1][pic_block_x+blockshape_x/4] :
               block_available_upleft  ? refFrArr[pic_block_y-1][pic_block_x-1] : -1;

  /* Prediction if only one of the neighbors uses the reference frame
   * we are checking
   */

  if(rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_L;
  else if(rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_U;
  else if(rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
    mvPredType = MVPRED_UR;

  // Directional predictions

  else if(blockshape_x == 8 && blockshape_y == 16)
  {
    if(mb_x == 0)
    {
      if(rFrameL == ref_frame)
        mvPredType = MVPRED_L;
    }
    else
    {
      if(rFrameUR == ref_frame)
        mvPredType = MVPRED_UR;
    }
  }
  else if(blockshape_x == 16 && blockshape_y == 8)
  {
    if(mb_y == 0)
    {
      if(rFrameU == ref_frame)
        mvPredType = MVPRED_U;
    }
    else
    {
      if(rFrameL == ref_frame)
        mvPredType = MVPRED_L;
    }
  }
  else if(blockshape_x == 8 && blockshape_y == 4 && mb_x == 8)
    mvPredType = MVPRED_L;
  else if(blockshape_x == 4 && blockshape_y == 8 && mb_y == 8)
    mvPredType = MVPRED_U;

  for (hv=0; hv < 2; hv++)
  {
    mv_a = block_available_left    ? tmp_mv[hv][pic_block_y][pic_block_x-1+4]   : 0;
    mv_b = block_available_up      ? tmp_mv[hv][pic_block_y-1][pic_block_x+4]   : 0;
    mv_d = block_available_upleft  ? tmp_mv[hv][pic_block_y-1][pic_block_x-1+4] : 0;
    mv_c = block_available_upright ? tmp_mv[hv][pic_block_y-1][pic_block_x+blockshape_x/4+4] : mv_d;

    switch (mvPredType)
    {
    case MVPRED_MEDIAN:
      if(!(block_available_upleft || block_available_up || block_available_upright))
        pred_vec = mv_a;
      else
        pred_vec = mv_a+mv_b+mv_c-min(mv_a,min(mv_b,mv_c))-max(mv_a,max(mv_b,mv_c));
      break;

    case MVPRED_L:
      pred_vec = mv_a;
      break;
    case MVPRED_U:
      pred_vec = mv_b;
      break;
    case MVPRED_UR:
      pred_vec = mv_c;
      break;
    default:
      break;
    }

    pred[hv] = pred_vec;
  }
}


#ifdef _FAST_FULL_ME_

/*****
 *****  static variables for fast integer motion estimation
 *****
 */
static int  *search_setup_done;  //!< flag if all block SAD's have been calculated yet
static int  *search_center_x;    //!< absolute search center for fast full motion search
static int  *search_center_y;    //!< absolute search center for fast full motion search
static int  *pos_00;             //!< position of (0,0) vector
static int  ****BlockSAD;        //!< SAD for all blocksize, ref. frames and motion vectors


/*!
 ***********************************************************************
 * \brief
 *    function creating arrays for fast integer motion estimation
 ***********************************************************************
 */
void
InitializeFastFullIntegerSearch (int search_range)
{
  int  i, j, k;
  int  max_pos = (2*search_range+1) * (2*search_range+1);

  if ((BlockSAD = (int****)malloc ((img->buf_cycle+1) * sizeof(int***))) == NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
  for (i = 0; i <= img->buf_cycle; i++)
  {
    if ((BlockSAD[i] = (int***)malloc (8 * sizeof(int**))) == NULL)
      no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
    for (j = 1; j < 8; j++)
    {
      if ((BlockSAD[i][j] = (int**)malloc (16 * sizeof(int*))) == NULL)
        no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
      for (k = 0; k < 16; k++)
      {
        if ((BlockSAD[i][j][k] = (int*)malloc (max_pos * sizeof(int))) == NULL)
          no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
      }
    }
  }

  if ((search_setup_done = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: search_setup_done");
  if ((search_center_x = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: search_center_x");
  if ((search_center_y = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: search_center_y");
  if ((pos_00 = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: pos_00");
}


/*!
 ***********************************************************************
 * \brief
 *    function for deleting the arrays for fast integer motion estimation
 ***********************************************************************
 */
void
ClearFastFullIntegerSearch ()
{
  int  i, j, k;

  for (i = 0; i <= img->buf_cycle; i++)
  {
    for (j = 1; j < 8; j++)
    {
      for (k = 0; k < 16; k++)
      {
        free (BlockSAD[i][j][k]);
      }
      free (BlockSAD[i][j]);
    }
    free (BlockSAD[i]);
  }
  free (BlockSAD);

  free (search_setup_done);
  free (search_center_x);
  free (search_center_y);
  free (pos_00);
}


/*!
 ***********************************************************************
 * \brief
 *    function resetting flags for fast integer motion estimation
 *    (have to be called in start_macroblock())
 ***********************************************************************
 */
void
ResetFastFullIntegerSearch ()
{
  int i;
  for (i = 0; i <= img->buf_cycle; i++)
    search_setup_done [i] = 0;
}

/*!
 ***********************************************************************
 * \brief
 *    calculation of SAD for larger blocks on the basis of 4x4 blocks
 ***********************************************************************
 */
void
SetupLargerBlocks (int refindex, int max_pos)
{
#define ADD_UP_BLOCKS()   _o=*_bo; _i=*_bi; _j=*_bj; for(pos=0;pos<max_pos;pos++) *_o++ = *_i++ + *_j++;
#define INCREMENT(inc)    _bo+=inc; _bi+=inc; _bj+=inc;

  int    pos, **_bo, **_bi, **_bj;
  register int *_o,   *_i,   *_j;

  //--- blocktype 6 ---
  _bo = BlockSAD[refindex][6];
  _bi = BlockSAD[refindex][7];
  _bj = _bi + 4;
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(5);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS();

  //--- blocktype 5 ---
  _bo = BlockSAD[refindex][5];
  _bi = BlockSAD[refindex][7];
  _bj = _bi + 1;
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS();

  //--- blocktype 4 ---
  _bo = BlockSAD[refindex][4];
  _bi = BlockSAD[refindex][6];
  _bj = _bi + 1;
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(6);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS();

  //--- blocktype 3 ---
  _bo = BlockSAD[refindex][3];
  _bi = BlockSAD[refindex][4];
  _bj = _bi + 8;
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS();

  //--- blocktype 2 ---
  _bo = BlockSAD[refindex][2];
  _bi = BlockSAD[refindex][4];
  _bj = _bi + 2;
  ADD_UP_BLOCKS(); INCREMENT(8);
  ADD_UP_BLOCKS();

  //--- blocktype 1 ---
  _bo = BlockSAD[refindex][1];
  _bi = BlockSAD[refindex][3];
  _bj = _bi + 2;
  ADD_UP_BLOCKS();
}

/*!
 ***********************************************************************
 * \brief
 *    calculation of all SAD's (for all motion vectors and 4x4 blocks)
 ***********************************************************************
 */
void
SetupFastFullIntegerSearch (int      refframe,
                            int    **refFrArr,
                            int   ***tmp_mv,
                            int *****img_mv,
                            pel_t   *CurrRefPic,
                            int      search_range,
                            int      backward,
                            int      rdopt)
// Note: All Vectors in this function are full pel only.
{
  int     x, y, pos, blky, bindex;
  int Absolute_X, Absolute_Y;
  int LineSadBlk0, LineSadBlk1, LineSadBlk2, LineSadBlk3;
  int     range_partly_outside, offset_x, offset_y, ref_x, ref_y;
  int refindex     = backward ? img->buf_cycle : refframe;
  int max_width    = img->width  - 17;
  int max_height   = img->height - 17;
  int max_pos      = (2*search_range+1) * (2*search_range+1);
  int     mv_mul       = input->mv_res ? 2 : 1;
  int   **block_sad    = BlockSAD[refindex][7];
  pel_t   orig_blocks [256];

  register pel_t  *orgptr = orig_blocks;
  register pel_t  *refptr;


  //===== get search center: predictor of 16x16 block =====
  SetMotionVectorPredictor (img_mv[0][0][refframe][1], refFrArr, tmp_mv,
          refframe, 0, 0, 16, 16);
  search_center_x[refindex] = img_mv[0][0][refframe][1][0] / (mv_mul*4);
  search_center_y[refindex] = img_mv[0][0][refframe][1][1] / (mv_mul*4);
  if (!rdopt)
  {
    // correct center so that (0,0) vector is inside
    search_center_x[refindex] = max(-search_range, min(search_range, search_center_x[refindex]));
    search_center_y[refindex] = max(-search_range, min(search_range, search_center_y[refindex]));
  }
  search_center_x[refindex] += img->pix_x;
  search_center_y[refindex] += img->pix_y;
  offset_x = search_center_x[refindex];
  offset_y = search_center_y[refindex];


  //===== copy original block for fast access =====
  for   (y = img->pix_y; y < img->pix_y+16; y++)
    for (x = img->pix_x; x < img->pix_x+16; x++)
      *orgptr++ = imgY_org [y][x];


  //===== check if whole search range is inside image =====
  if (offset_x >= search_range      &&
      offset_y >= search_range      &&
      offset_x <= max_width  - search_range &&
      offset_y <= max_height - search_range)
  {
    range_partly_outside = 0; PelYline_11 = FastLine16Y_11;
  }
  else
    range_partly_outside = 1;


  //===== determine position of (0,0)-vector =====
  if (!rdopt)
  {
    ref_x = img->pix_x - offset_x;
    ref_y = img->pix_y - offset_y;

    for (pos = 0; pos < max_pos; pos++)
      if (ref_x == SpiralX[pos] &&
          ref_y == SpiralY[pos])
      {
        pos_00[refindex] = pos;
        break;
      }
  }


  //===== loop over search range (spiral search): get blockwise SAD =====
  for (pos = 0; pos < max_pos; pos++)
  {
    Absolute_Y = offset_y + SpiralY[pos];
    Absolute_X = offset_x + SpiralX[pos];

    if (range_partly_outside)
    {
      if (Absolute_Y >= 0 && Absolute_Y <= max_height &&
          Absolute_X >= 0 && Absolute_X <= max_width    )
        PelYline_11 = FastLine16Y_11;
      else
        PelYline_11 = UMVLine16Y_11;
    }

    orgptr = orig_blocks;
    bindex = 0;
    for (blky = 0; blky < 4; blky++)
    {
      LineSadBlk0 = LineSadBlk1 = LineSadBlk2 = LineSadBlk3 = 0;
      for (y = 0; y < 4; y++)
      {
        refptr = PelYline_11 (CurrRefPic, Absolute_Y++, Absolute_X);

        LineSadBlk0 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk0 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk0 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk0 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk1 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk1 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk1 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk1 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk2 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk2 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk2 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk2 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk3 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk3 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk3 += ByteAbs(*refptr++ - *orgptr++);
        LineSadBlk3 += ByteAbs(*refptr++ - *orgptr++);
      }
      block_sad[bindex++][pos] = LineSadBlk0;
      block_sad[bindex++][pos] = LineSadBlk1;
      block_sad[bindex++][pos] = LineSadBlk2;
      block_sad[bindex++][pos] = LineSadBlk3;
    }
  }


  //===== combine SAD's for larger block types =====
  SetupLargerBlocks (refindex, max_pos);


  //===== set flag marking that search setup have been done =====
  search_setup_done[refindex] = 1;
}

/*!
 ***********************************************************************
 * \brief
 *    fast full integer search (SetupFastFullIntegerSearch have to be called before)
 ***********************************************************************
 */
int
FastFullIntegerSearch (int    mv_mul,
                       int    predicted_x,
                       int    predicted_y,
                       int    search_range,
                       int    refindex,
                       int    blocktype,
                       int    mb_x,
                       int    mb_y,
                       int    *center_h2,
                       int    *center_v2,
                       int    pic_pix_x,
                       int    pic_pix_y,
                       double lambda)
{
// Note: All Vectors in this function are full pel only.

  int Candidate_X, Candidate_Y, current_inter_sad;
  int mvres          = mv_mul==1 ? 4 : 8;
  int mvres_l        = mv_mul==1 ? 2 : 3;
  int Offset_X     = search_center_x[refindex] - img->pix_x;
  int Offset_Y       = search_center_y[refindex] - img->pix_y;
  int best_inter_sad = MAX_VALUE;
  int bindex        = mb_y + (mb_x >> 2);

  register int  pos;
  register int  max_pos   = (2*search_range+1)*(2*search_range+1);
  register int *block_sad = BlockSAD[refindex][blocktype][bindex];


  //===== cost for (0,0)-vector: it is done before, because MVCost can be negative =====
  if (!lambda)
  {
    *center_h2     = 0;
    *center_v2     = 0;
    best_inter_sad = block_sad[pos_00[refindex]] + MVCost(1, mvres, blocktype, img->qp, predicted_x, predicted_y, 0, 0);
  }


  //===== loop over search range (spiral search) =====
  if (lambda)
  {
    for (pos = 0; pos < max_pos; pos++, block_sad++)
      if (*block_sad < best_inter_sad)
      {
        Candidate_X        = Offset_X + SpiralX[pos];
        Candidate_Y        = Offset_Y + SpiralY[pos];
        current_inter_sad  = *block_sad;
        current_inter_sad += MVCostLambda (mvres_l, lambda, predicted_x, predicted_y, Candidate_X, Candidate_Y);

        if (current_inter_sad < best_inter_sad)
        {
          *center_h2     = Candidate_X;
          *center_v2     = Candidate_Y;
          best_inter_sad = current_inter_sad;
        }
      }
  }
  else
  {
    for (pos = 0; pos < max_pos; pos++, block_sad++)
      if (*block_sad < best_inter_sad)
      {
        Candidate_X        = Offset_X + SpiralX[pos];
        Candidate_Y        = Offset_Y + SpiralY[pos];
        current_inter_sad  = *block_sad;
        current_inter_sad += MVCost (1, mvres, blocktype, img->qp, predicted_x, predicted_y, Candidate_X, Candidate_Y);

        if (current_inter_sad < best_inter_sad)
        {
          *center_h2     = Candidate_X;
          *center_v2     = Candidate_Y;
          best_inter_sad = current_inter_sad;
        }
      }
  }

  return best_inter_sad;
}



#endif // _FAST_FULL_ME_


/*!
 ***********************************************************************
 * \brief
 *    Integer Spiral Search
 * \par Output:
 *    best_inter_sad
 * \par Output (through Parameter pointers):
 *    center_h2, center_v2 (if better MV was found)
 ***********************************************************************
 */
int IntegerSpiralSearch ( int mv_mul, int center_x, int center_y,
                          int predicted_x, int predicted_y,
                          int blockshape_x, int blockshape_y,
                          int curr_search_range, int blocktype,
                          pel_t mo[16][16], pel_t *FullPelCurrRefPic,
                          int *center_h2, int *center_v2,
                          int pic_pix_x, int pic_pix_y,
                          double lambda)
{
// Note: All Vectors in this function are full pel only.

  int numc, lv;
  int best_inter_sad = MAX_VALUE;
  int abort_search;
  int current_inter_sad;
  int Candidate_X, Candidate_Y, Absolute_X, Absolute_Y;
  int i, x, y;
  int Difference;
  pel_t SingleDimensionMo[16*16];
  pel_t *check;

  numc=(curr_search_range*2+1)*(curr_search_range*2+1);

  // Setup a fast access field for the original
  for (y=0, i=0; y<blockshape_y; y++)
    for (x=0; x<blockshape_x; x++)
      SingleDimensionMo[i++] = mo[y][x];

  for (lv=0; lv < numc; lv++)
  {
    Candidate_X = center_x + SpiralX[lv];
    Candidate_Y = center_y + SpiralY[lv];
    Absolute_X = pic_pix_x + Candidate_X;
    Absolute_Y = pic_pix_y + Candidate_Y;

    if (lambda)
      current_inter_sad = MVCostLambda (mv_mul==1?2:3, lambda, predicted_x, predicted_y, Candidate_X, Candidate_Y);
    else
      current_inter_sad = MVCost (1, (mv_mul==1)?4:8, blocktype, img->qp, predicted_x, predicted_y, Candidate_X,Candidate_Y);


    abort_search=FALSE;


    for (y=0, i=0; y < blockshape_y && !abort_search; y++)
    {
      check = PelYline_11 (FullPelCurrRefPic, Absolute_Y+y, Absolute_X);      // get pointer to line
      for (x=0; x < blockshape_x ;x++)
      {
        Difference = SingleDimensionMo[i++]- *check++;
        current_inter_sad += ByteAbs (Difference);
      }
      if (current_inter_sad >= best_inter_sad)
      {
        abort_search=TRUE;
      }
    }

    if(!abort_search)
    {
      *center_h2=Candidate_X;
      *center_v2=Candidate_Y;
      best_inter_sad=current_inter_sad;
    }
  }
  return best_inter_sad;
}


/*!
 ***********************************************************************
 * \brief
 *    Half Pel Search
 ***********************************************************************
 */
int HalfPelSearch(int pic4_pix_x, int pic4_pix_y, int mv_mul, int blockshape_x, int blockshape_y,
      int *s_pos_x1, int *s_pos_y1, int *s_pos_x2, int *s_pos_y2, int center_h2, int center_v2,
      int predicted_x, int predicted_y, int blocktype, pel_t mo[16][16], pel_t **CurrRefPic, int ***tmp_mv,
      int *****all_mv, int block_x, int block_y, int ref_frame, int pic_block_x, int pic_block_y,
      double lambda)
{
  int best_inter_sad=MAX_VALUE;
  int current_inter_sad;
  int lv, iy0, jy0, vec1_x, vec1_y, vec2_x, vec2_y, i, i22, j;
  int s_pos_x, s_pos_y;
  int  m7[16][16];

  for (lv=0; lv < 9; lv++)
  {
    s_pos_x=center_h2+SpiralX[lv]*2;
    s_pos_y=center_v2+SpiralY[lv]*2;
    iy0=pic4_pix_x+s_pos_x;
    jy0=pic4_pix_y+s_pos_y;

    if (lambda)
      current_inter_sad = MVCostLambda (mv_mul==1?0:1, lambda,
      predicted_x, predicted_y, s_pos_x, s_pos_y);
    else
      current_inter_sad = MVCost (4, (mv_mul==1)?4:8, blocktype, img->qp, predicted_x, predicted_y, s_pos_x, s_pos_y);

    for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4)
    {
      for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          vec2_x=i+vec1_x;
          i22=iy0+vec2_x*4;
          for (j=0; j < BLOCK_SIZE; j++)
          {
            vec2_y=j+vec1_y;
            m7[i][j]=mo[vec2_y][vec2_x]-PelY_14 (CurrRefPic, jy0+vec2_y*4, i22);
          }
        }
        current_inter_sad += find_sad(input->hadamard, m7);
      }
    }
    if (current_inter_sad < best_inter_sad)
    {
      *s_pos_x1=s_pos_x;
      *s_pos_y1=s_pos_y;
      *s_pos_x2=s_pos_x;
      *s_pos_y2=s_pos_y;

      for (i=0; i < blockshape_x/BLOCK_SIZE; i++)
      {
        for (j=0; j < blockshape_y/BLOCK_SIZE; j++)
        {
          all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=mv_mul**s_pos_x1;
          all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=mv_mul**s_pos_y1;

          tmp_mv[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=mv_mul**s_pos_x1;
          tmp_mv[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=mv_mul**s_pos_y1;
        }
      }
      best_inter_sad=current_inter_sad;
    }
  }
  return best_inter_sad;
}


/*!
 ***********************************************************************
 * \brief
 *    Quarter Pel Search
 ***********************************************************************
 */
int QuarterPelSearch(int pic4_pix_x, int pic4_pix_y, int mv_mul, int blockshape_x, int blockshape_y,
        int *s_pos_x1, int *s_pos_y1, int *s_pos_x2, int *s_pos_y2, int center_h2, int center_v2,
        int predicted_x, int predicted_y, int blocktype, pel_t mo[16][16], pel_t **CurrRefPic, int ***tmp_mv,
        int *****all_mv, int block_x, int block_y, int ref_frame,
        int pic_block_x, int pic_block_y, int best_inter_sad,
        double lambda
        )
{
  int current_inter_sad;
  int lv, s_pos_x, s_pos_y, iy0, jy0, vec1_x, vec1_y, vec2_x, vec2_y, i, i22, j;
  int  m7[16][16];

  for (lv=1; lv < 9; lv++)
  {

    s_pos_x=*s_pos_x2+SpiralX[lv];
    s_pos_y=*s_pos_y2+SpiralY[lv];

    iy0=pic4_pix_x+s_pos_x;
    jy0=pic4_pix_y+s_pos_y;

    if (lambda)
      current_inter_sad = MVCostLambda (mv_mul==1?0:1, lambda,
                                        predicted_x, predicted_y, s_pos_x, s_pos_y);
    else
      current_inter_sad = MVCost (4, (mv_mul==1)?4:8, blocktype, img->qp, predicted_x, predicted_y, s_pos_x, s_pos_y);

    for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4)
    {
      for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          vec2_x=i+vec1_x;
          i22=iy0+vec2_x*4;
          for (j=0; j < BLOCK_SIZE; j++)
          {
            vec2_y=j+vec1_y;
            m7[i][j]=mo[vec2_y][vec2_x]-PelY_14 (CurrRefPic, jy0+vec2_y*4, i22);
          }
        }
        current_inter_sad += find_sad(input->hadamard, m7);
      }
    }
    if (current_inter_sad < best_inter_sad)
    {
      // Vectors are saved in all_mv[] to be able to assign correct vectors to each block after mode selection.
      // tmp_mv[] is a 'temporary' assignment of vectors to be used to estimate 'bit cost' in vector prediction.

      *s_pos_x1=s_pos_x;
      *s_pos_y1=s_pos_y;

      for (i=0; i < blockshape_x/BLOCK_SIZE; i++)
      {
        for (j=0; j < blockshape_y/BLOCK_SIZE; j++)
        {
          all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=mv_mul**s_pos_x1;
          all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=mv_mul**s_pos_y1;
          tmp_mv[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=mv_mul**s_pos_x1;
          tmp_mv[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=mv_mul**s_pos_y1;
        }
      }
      best_inter_sad=current_inter_sad;
    }
  }

  *s_pos_x2 = 2 * *s_pos_x1;
  *s_pos_y2 = 2 * *s_pos_y1;

  return best_inter_sad;
}


/*!
 ***********************************************************************
 * \brief
 *    Eighth Pel Search
 ***********************************************************************
 */
int EighthPelSearch(int pic8_pix_x, int pic8_pix_y, int mv_mul, int blockshape_x, int blockshape_y,
       int *s_pos_x1, int *s_pos_y1, int *s_pos_x2, int *s_pos_y2, int center_h2, int center_v2,
       int predicted_x, int predicted_y, int blocktype, pel_t mo[16][16], pel_t **CurrRefPic, int ***tmp_mv,
       int *****all_mv, int block_x, int block_y, int ref_frame,
       int pic_block_x, int pic_block_y, int best_inter_sad,
       double lambda
       )
{
  int current_inter_sad;
  int lv, s_pos_x, s_pos_y, iy0, jy0, vec1_x, vec1_y, vec2_x, vec2_y, i, i22, j;
  int  m7[16][16];

  for (lv=1; lv < 9; lv++)
  {
    s_pos_x=*s_pos_x2+SpiralX[lv];
    s_pos_y=*s_pos_y2+SpiralY[lv];

    iy0=pic8_pix_x+s_pos_x;
    jy0=pic8_pix_y+s_pos_y;

    if (lambda)
      current_inter_sad = MVCostLambda (0, lambda,
      predicted_x, predicted_y, s_pos_x, s_pos_y);
    else
      current_inter_sad = MVCost (8, 8, blocktype, img->qp, predicted_x, predicted_y, s_pos_x, s_pos_y);

    for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4)
    {
      for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          vec2_x=i+vec1_x;
          i22=iy0+vec2_x*8;
          for (j=0; j < BLOCK_SIZE; j++)
          {
            vec2_y=j+vec1_y;
            m7[i][j]=mo[vec2_y][vec2_x]-PelY_18 (CurrRefPic, jy0+vec2_y*8, i22);
          }
        }
        current_inter_sad += find_sad(input->hadamard, m7);
      }
    }
    if (current_inter_sad < best_inter_sad)
    {
      // Vectors are saved in all_mv[] to be able to assign correct vectors to each block after mode selection.
      // tmp_mv[] is a 'temporary' assignment of vectors to be used to estimate 'bit cost' in vector prediction.

      *s_pos_x1=s_pos_x;
      *s_pos_y1=s_pos_y;

      for (i=0; i < blockshape_x/BLOCK_SIZE; i++)
      {
        for (j=0; j < blockshape_y/BLOCK_SIZE; j++)
        {
          all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=*s_pos_x1;
          all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=*s_pos_y1;
          tmp_mv[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=*s_pos_x1;
          tmp_mv[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=*s_pos_y1;
        }
      }
      best_inter_sad=current_inter_sad;
    }
  }
  return best_inter_sad;
}


#define PFRAME    0
#define B_FORWARD 1
#define B_BACKWARD  2

/*!
 ************************************************************************
 * \brief
 *    In this routine motion search (integer pel+1/2 and 1/4 pel) and mode selection
 *    is performed. Since we treat all 4x4 blocks before coding/decoding the
 *    prediction may not be based on decoded pixels (except for some of the blocks).
 *    This will result in too good prediction.  To compensate for this the SAD for
 *    intra(tot_intra_sad) is given a 'handicap' depending on QP.
 *                                                                                     \par
 *    Note that a couple of speed improvements were implemented by StW, which
 *    slightly change the use of some variables.  In particular, the dimensions
 *    of the img->m7 and mo[] variables were changed in order to allow better
 *    cache access.
 *                                                                                     \par
 *    Depending on the MV-resolution 1/4-pel or 1/8-pel motion vectors are estimated
 *                                                                                     \par
 * \par Input:
 *    Best intra SAD value.
 *
 * \par Output:
 *    Reference image.
 *
 * \par Side Effects:
 *    writes refFrArr[ ][ ] to set reference frame ID for each block
 *
 *  \par Bugs and Limitations:
 *    currently the B-frame module does not support UMV in its residual coding.
 *    Hence, the old search range limitation is implemented here for all B frames.
 *    The if()s in question all start in column 1 of the file and are thus
 *    easily idetifyable.
 *
 ************************************************************************
 */

int
UnifiedMotionSearch (int tot_intra_sad, int **refFrArr, int ***tmp_mv,
         int *****img_mv,
         int *img_mb_mode, int *img_blc_size_h, int *img_blc_size_v,
         int *img_multframe_no, int BFrame, int *min_inter_sad, int *****all_mv)
{
  int predframe_no=0;
  int ref_frame,blocktype;
  int tot_inter_sad;
  int ref_inx;

  *min_inter_sad = MAX_VALUE;


  // Loop through all reference frames under consideration
  // P-Frames, B-Forward: All reference frames
  // P-Backward: use only the last reference frame, stored in mref_P
  

#ifdef _ADDITIONAL_REFERENCE_FRAME_
  for (ref_frame=0; ref_frame < img->nb_references; ref_frame++)
    if ((ref_frame <  input->no_multpred) || (ref_frame == input->add_ref_frame))
#else
  for (ref_frame=0; ref_frame < img->nb_references; ref_frame++)
#endif

  {
    ref_inx = (img->number-ref_frame-1)%img->buf_cycle;

    //  Looping through all the chosen block sizes:
    blocktype=1;
    while (input->blc_size[blocktype][0]==0 && blocktype<=7) // skip blocksizes not chosen
      blocktype++;

    for (;blocktype <= 7;)
    {
      tot_inter_sad  = QP2QUANT[img->qp] * min(ref_frame,1) * 2; // start 'handicap '
      tot_inter_sad += SingleUnifiedMotionSearch (ref_frame, blocktype,
        refFrArr, tmp_mv, img_mv,
        BFrame, all_mv, 0.0);

        /*
        Here we update which is the best inter mode. At the end we have the best inter mode.
        predframe_no:  which reference frame to use for prediction
        img->multframe_no:  Index in the mref[] matrix used for prediction
      */

      if (tot_inter_sad <= *min_inter_sad)
      {
        if (BFrame==PFRAME)
          *img_mb_mode=blocktype;
        if (BFrame == B_FORWARD)
        {
          if (blocktype == 1)
            *img_mb_mode = blocktype;
          else
            *img_mb_mode = blocktype * 2;
        }
        if (BFrame == B_BACKWARD)
        {
          if (blocktype == 1)
            *img_mb_mode = blocktype+1;
          else
            *img_mb_mode = blocktype * 2+1;

        }
        *img_blc_size_h=input->blc_size[blocktype][0];
        *img_blc_size_v=input->blc_size[blocktype][1];
        predframe_no=ref_frame;
        *img_multframe_no=ref_inx;
        *min_inter_sad=tot_inter_sad;
      }

      while (input->blc_size[++blocktype][0]==0 && blocktype<=7); // only go through chosen blocksizes
    }
    if (BFrame == B_BACKWARD)
      ref_frame = MAX_VALUE;    // jump out of the loop
  }

  return predframe_no;
}



/*!
 ************************************************************************
 * \brief
 *    unified motion search for a single combination of reference frame and blocksize
 ************************************************************************
 */
int
SingleUnifiedMotionSearch (int    ref_frame,
                           int    blocktype,
                           int  **refFrArr,
                           int ***tmp_mv,
                           int    *****img_mv,
                           int    BFrame,
                           int    *****all_mv,
                           double lambda)
{
  int   j,i;
  int   pic_block_x,pic_block_y,block_x,ref_inx,block_y,pic_pix_y;
  int   pic4_pix_y,pic8_pix_y,pic_pix_x,pic4_pix_x,pic8_pix_x;
  int   ip0,ip1;
  int   center_h2,curr_search_range,center_v2;
  int   s_pos_x1,s_pos_y1,s_pos_x2,s_pos_y2;
  int   mb_y,mb_x;
  int   best_inter_sad, tot_inter_sad = 0;
  pel_t mo[16][16];
  int   blockshape_x,blockshape_y;
  int   mv_mul;
  pel_t **CurrRefPic;
  pel_t *CurrRefPic11;
#ifdef _FAST_FULL_ME_
  int  refindex, full_search_range;
#else
  int    center_x, center_y;
#endif



  /*  curr_search_range is the actual search range used depending on block size and reference frame.
  It may be reduced by 1/2 or 1/4 for smaller blocks and prediction from older frames due to compexity
  */
#ifdef _FULL_SEARCH_RANGE_
  if (input->full_search == 2) // no restrictions at all
  {
    curr_search_range = input->search_range;
  }
  else if (input->full_search == 1) // smaller search range for older reference frames
  {
    curr_search_range = input->search_range /  (min(ref_frame,1)+1);
  }
  else // smaller search range for older reference frames and smaller blocks
#endif
  {
    curr_search_range = input->search_range / ((min(ref_frame,1)+1) * min(2,blocktype));
  }
#ifdef _FAST_FULL_ME_
#ifdef _FULL_SEARCH_RANGE_
  if (input->full_search == 2)
    full_search_range   = input->search_range;
  else
#endif
    full_search_range   = input->search_range / (min(ref_frame,1)+1);
#endif



  // Motion vector scale-factor for 1/8-pel MV-resolution
  if(input->mv_res)
    mv_mul=2;
  else
    mv_mul=1;


  // set reference frame array
  for (j = 0;j < 4;j++)
  {
    for (i = 0;i < 4;i++)
    {
      refFrArr[img->block_y+j][img->block_x+i] = ref_frame;
    }
  }


  // find  reference image
  if (BFrame != B_BACKWARD)
  {
    ref_inx=(img->number-ref_frame-1)%img->buf_cycle;
    CurrRefPic   = mref[ref_inx];
    CurrRefPic11 = Refbuf11[ref_inx];
  }
  else
  {
    CurrRefPic   = mref_P;
    CurrRefPic11 = Refbuf11_P;
  }


#ifdef _FAST_FULL_ME_
  //===== setup fast full integer search: get SAD's for all motion vectors and 4x4 blocks =====
  //      (this is done just once for a macroblock)
  refindex = (BFrame != B_BACKWARD ? ref_frame : img->buf_cycle);

  if (!search_setup_done[refindex])
  {
    SetupFastFullIntegerSearch (ref_frame, refFrArr, tmp_mv, img_mv, CurrRefPic11,
      full_search_range, (BFrame==B_BACKWARD), (lambda != 0));
  }
#endif

  blockshape_x=input->blc_size[blocktype][0];// input->blc_size has information of the 7 blocksizes
  blockshape_y=input->blc_size[blocktype][1];// array iz stores offset inside one MB


  //  Loop through the whole MB with all block sizes
  for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += blockshape_y)
  {
    block_y=mb_y/BLOCK_SIZE;
    pic_block_y=img->block_y+block_y;
    pic_pix_y=img->pix_y+mb_y;
    pic4_pix_y=pic_pix_y*4;

    for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += blockshape_x)
    {
      block_x=mb_x/BLOCK_SIZE;
      pic_block_x=img->block_x+block_x;
      pic_pix_x=img->pix_x+mb_x;
      pic4_pix_x=pic_pix_x*4;


      SetMotionVectorPredictor (img_mv[block_x][block_y][ref_frame][blocktype], refFrArr, tmp_mv,
        ref_frame, mb_x, mb_y, blockshape_x, blockshape_y);


      ip0=img_mv[block_x][block_y][ref_frame][blocktype][0];
      ip1=img_mv[block_x][block_y][ref_frame][blocktype][1];

      //  mo[] is the the original block to be used in the search process
      // Note: dimensions changed to speed up access
      for (i=0; i < blockshape_x; i++)
        for (j=0; j < blockshape_y; j++)
          mo[j][i]=imgY_org[pic_pix_y+j][pic_pix_x+i];

      //  Integer pel search.  center_x,center_y is the 'search center'
#ifndef _FAST_FULL_ME_
      // Note that the predictor in mv[][][][][] is in 1/4 or 1/8th pel resolution
      // The following rounding enforces a "true" integer pel search.
      center_x=ip0/(mv_mul*4);
      center_y=ip1/(mv_mul*4);

      // Limitation of center_x,center_y so that the search wlevel ow contains the (0,0) vector
      center_x=max(-(input->search_range),min(input->search_range,center_x));
      center_y=max(-(input->search_range),min(input->search_range,center_y));

      // The following was deleted to support UMV.  See Remak regarding UMV performance for B
      // in the Readme File (to be provided)
      // Search center corrected to prevent vectors outside the frame, this is not permitted in this model.
      if ((pic_pix_x + center_x > curr_search_range) &&
          (pic_pix_y + center_y > curr_search_range) &&
          (pic_pix_x + center_x < img->width  - 1 - curr_search_range - blockshape_x) &&
          (pic_pix_y + center_y < img->height - 1 - curr_search_range - blockshape_y))
      {
        PelYline_11 = FastLine16Y_11;
      }
      else
      {
        PelYline_11 = UMVLine16Y_11;
      }

      best_inter_sad = IntegerSpiralSearch (mv_mul, center_x, center_y, ip0, ip1,
                                            blockshape_x, blockshape_y, curr_search_range,
                                            blocktype, mo, CurrRefPic11, &center_h2, &center_v2,
                                            pic_pix_x, pic_pix_y,
                                            lambda);

#else // _FAST_FULL_ME_

      best_inter_sad = FastFullIntegerSearch (mv_mul, ip0, ip1, curr_search_range,
                                              refindex, blocktype, mb_x, mb_y,
                                              &center_h2, &center_v2, pic_pix_x, pic_pix_y,
                                              lambda);

#endif // _FAST_FULL_ME_


      // Adjust center to 1/4 pel positions
      center_h2 *=4;
      center_v2 *=4;


      if ((pic4_pix_x + center_h2 > 1) &&
          (pic4_pix_y + center_v2 > 1) &&
          (pic4_pix_x + center_h2 < 4*(img->width  - blockshape_x + 1) - 2) &&
          (pic4_pix_y + center_v2 < 4*(img->height - blockshape_y + 1) - 2)   )
        PelY_14 = FastPelY_14;
      else
        PelY_14 = UMVPelY_14;

      //  1/2 pixel search.  In this version the search is over 9 vector positions.
      best_inter_sad = HalfPelSearch (pic4_pix_x, pic4_pix_y, mv_mul, blockshape_x, blockshape_y,
                                      &s_pos_x1, &s_pos_y1, &s_pos_x2, &s_pos_y2, center_h2, center_v2,
                                      ip0, ip1, blocktype, mo, CurrRefPic, tmp_mv, all_mv, block_x,
                                      block_y, ref_frame, pic_block_x, pic_block_y,
                                      lambda);



      //  1/4 pixel search.
      if ((pic4_pix_x + s_pos_x2 > 0) &&
          (pic4_pix_y + s_pos_y2 > 0) &&
          (pic4_pix_x + s_pos_x2 < 4*(img->width  - blockshape_x + 1) - 1) &&
          (pic4_pix_y + s_pos_y2 < 4*(img->height - blockshape_y + 1) - 1)   )
        PelY_14 = FastPelY_14;
      else
        PelY_14 = UMVPelY_14;

      best_inter_sad = QuarterPelSearch (pic4_pix_x, pic4_pix_y, mv_mul, blockshape_x, blockshape_y,
                                         &s_pos_x1, &s_pos_y1, &s_pos_x2, &s_pos_y2, center_h2, center_v2,
                                         ip0, ip1, blocktype, mo, CurrRefPic, tmp_mv, all_mv, block_x, block_y, ref_frame,
                                         pic_block_x, pic_block_y, best_inter_sad,
                                         lambda);


      //  1/8 pixel search.
      if(input->mv_res)
      {
        pic8_pix_x=2*pic4_pix_x;
        pic8_pix_y=2*pic4_pix_y;

        if ((pic8_pix_x + s_pos_x2 > 0) &&
            (pic8_pix_y + s_pos_y2 > 0) &&
            (pic8_pix_x + s_pos_x2 < 8*(img->width  - blockshape_x + 1) - 2) &&
            (pic8_pix_y + s_pos_y2 < 8*(img->height - blockshape_y + 1) - 2)   )
          PelY_18 = FastPelY_18;
        else
          PelY_18 = UMVPelY_18;

        best_inter_sad = EighthPelSearch(pic8_pix_x, pic8_pix_y, mv_mul, blockshape_x, blockshape_y,
                                        &s_pos_x1, &s_pos_y1, &s_pos_x2, &s_pos_y2, center_h2, center_v2,
                                        ip0, ip1, blocktype, mo, CurrRefPic, tmp_mv, all_mv, block_x, block_y, ref_frame,
                                        pic_block_x, pic_block_y, best_inter_sad,
                                        lambda);

      }

      tot_inter_sad += best_inter_sad;
    }
  }
  return tot_inter_sad;
}



/*!
 ************************************************************************
 * \brief
 *    motion search for P-frames
 ************************************************************************
 */
int motion_search(int tot_intra_sad)
{
  int predframe_no, min_inter_sad, hv, i, j;


  predframe_no = UnifiedMotionSearch(tot_intra_sad, refFrArr, tmp_mv, img->mv, &img->mb_mode,
    &img->blc_size_h, &img->blc_size_v, &img->multframe_no, PFRAME, &min_inter_sad, img->all_mv);

  // tot_intra_sad is now the minimum SAD for intra.  min_inter_sad is the best (min) SAD for inter (see above).
  // Inter/intra is determined depending on which is smallest

  if (tot_intra_sad < min_inter_sad)
  {
    img->mb_mode=img->imod+8*img->type; // set intra mode in inter frame
    for (hv=0; hv < 2; hv++)
      for (i=0; i < 4; i++)
        for (j=0; j < 4; j++)
          tmp_mv[hv][img->block_y+j][img->block_x+i+4]=0;
  }
  else
  {
    img->mb_data[img->current_mb_nr].intraOrInter = INTER_MB;
    img->imod=INTRA_MB_INTER; // Set inter mode
    for (hv=0; hv < 2; hv++)
      for (i=0; i < 4; i++)
        for (j=0; j < 4; j++)
          tmp_mv[hv][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][predframe_no][img->mb_mode][hv];
  }
  return predframe_no;
};



/*!
 ************************************************************************
 * \brief
 *    forward motion search for B-frames
 ************************************************************************
 */
int get_fwMV(int *min_fw_sad, int tot_intra_sad)
{
  int fw_predframe_no, hv, i, j;

  fw_predframe_no = UnifiedMotionSearch(tot_intra_sad, fw_refFrArr, tmp_fwMV,
                                        img->p_fwMV, &img->fw_mb_mode,
                                        &img->fw_blc_size_h, &img->fw_blc_size_v,
                                        &img->fw_multframe_no, B_FORWARD,
                                        min_fw_sad, img->all_mv);

  // save forward MV to tmp_fwMV
  for (hv=0; hv < 2; hv++)
    for (i=0; i < 4; i++)
      for (j=0; j < 4; j++)
      {
        if(img->fw_mb_mode==1)
          tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][fw_predframe_no][1][hv];
        else
          tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][fw_predframe_no][(img->fw_mb_mode)/2][hv];
      }

  return fw_predframe_no;
}



/*!
 ************************************************************************
 * \brief
 *    backward motion search for B-frames
 ************************************************************************
 */
void get_bwMV(int *min_bw_sad)
{
  int bw_predframe_no, hv, i, j;

  bw_predframe_no = UnifiedMotionSearch(MAX_VALUE, bw_refFrArr, tmp_bwMV,
          img->p_bwMV, &img->bw_mb_mode,
          &img->bw_blc_size_h, &img->bw_blc_size_v,
          &img->bw_multframe_no, B_BACKWARD,
          min_bw_sad, img->all_mv);


  // save backward MV to tmp_bwMV
  for (hv=0; hv < 2; hv++)
    for (i=0; i < 4; i++)
      for (j=0; j < 4; j++)
      {
        if(img->bw_mb_mode==2)
          tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][bw_predframe_no][1][hv];
        else
          tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][bw_predframe_no][(img->bw_mb_mode-1)/2][hv];
      }
}


/*!
 ************************************************************************
 * \brief
 *    Do hadamard transform or normal SAD calculation.
 *    If Hadamard=1 4x4 Hadamard transform is performed and SAD of transform
 *    levels is calculated
 *
 * \par Input:
 *    hadamard=0 : normal SAD                                                  \n
 *    hadamard=1 : 4x4 Hadamard transform is performed and SAD of transform
 *                 levels is calculated
 *
 * \par Output:
 *    SAD/hadamard transform value
 *
 * \note
 *    This function was optimized by manual loop unrolling and pointer arithmetic
 ************************************************************************
 */
int find_sad(int hadamard, int m7[16][16])
{
  int i, j,x, y, best_sad,current_sad;
  int m1[BLOCK_SIZE*BLOCK_SIZE], m[BLOCK_SIZE*BLOCK_SIZE], *mp;

#ifdef COMPLEXITY
    SadCalls++;
#endif

  current_sad=0;
  if (hadamard != 0)
  {
    best_sad=0;

// Copy img->m7[][] into local variable, to avoid costly address arithmetic and cache misses

    mp=m;
    for (y=0; y<BLOCK_SIZE; y++)
      for (x=0; x<BLOCK_SIZE; x++)
        *mp++= m7[y][x];


    m1[0] = m[0] + m[12];
    m1[4] = m[4] + m[8];
    m1[8] = m[4] - m[8];
    m1[12]= m[0] - m[12];

    m1[1] = m[1] + m[13];
    m1[5] = m[5] + m[9];
    m1[9] = m[5] - m[9];
    m1[13]= m[1] - m[13];

    m1[2] = m[2] + m[14];
    m1[6] = m[6] + m[10];
    m1[10]= m[6] - m[10];
    m1[14]= m[2] - m[14];

    m1[3] = m[3] + m[15];
    m1[7] = m[7] + m[11];
    m1[11]= m[7] - m[11];
    m1[15]= m[3] - m[15];


    m[0] = m1[0] + m1[4];
    m[8] = m1[0] - m1[4];
    m[4] = m1[8] + m1[12];
    m[12]= m1[12]- m1[8];

    m[1] = m1[1] + m1[5];
    m[9] = m1[1] - m1[5];
    m[5] = m1[9] + m1[13];
    m[13]= m1[13]- m1[9];

    m[2] = m1[2] + m1[6];
    m[10] = m1[2] - m1[6];
    m[6] = m1[10] + m1[14];
    m[14]= m1[14]- m1[10];

    m[3] = m1[3] + m1[7];
    m[11]= m1[3] - m1[7];
    m[7] = m1[11]+ m1[15];
    m[15]= m1[15]- m1[11];



    m1[0] = m[0] + m[3];
    m1[1] = m[1] + m[2];
    m1[2] = m[1] - m[2];
    m1[3] = m[0] - m[3];

    m1[4] = m[4] + m[7];
    m1[5] = m[5] + m[6];
    m1[6] = m[5] - m[6];
    m1[7] = m[4] - m[7];

    m1[8] = m[8] + m[11];
    m1[9] = m[9] + m[10];
    m1[10]= m[9] - m[10];
    m1[11]= m[8] - m[11];

    m1[12]= m[12]+ m[15];
    m1[13]= m[13]+ m[14];
    m1[14]= m[13]- m[14];
    m1[15]= m[12]- m[15];


    m[0] = m1[0] + m1[1];
    m[1] = m1[0] - m1[1];
    m[2] = m1[2] + m1[3];
    m[3] = m1[3] - m1[2];

    m[4] = m1[4] + m1[5];
    m[5] = m1[4] - m1[5];
    m[6] = m1[6] + m1[7];
    m[7] = m1[7] - m1[6];

    m[8] = m1[8] + m1[9];
    m[9] = m1[8] - m1[9];
    m[10]= m1[10]+ m1[11];
    m[11]= m1[11]- m1[10];

    m[12]= m1[12]+ m1[13];
    m[13]= m1[12]- m1[13];
    m[14]= m1[14]+ m1[15];
    m[15]= m1[15]- m1[14];


    // Note: we cannot use ByteAbs() here because the absolute values can be bigger
    // than +- 256.  WOuld either need to enhance ByteAbs to WordAbs (or something)
    // or leave it as is (use of absm*( macro)

    for (j=0; j< BLOCK_SIZE * BLOCK_SIZE; j++)
      best_sad += absm (m[j]);

      /*
    Calculation of normalized Hadamard transforms would require divison by 4 here.
    However,we flevel  that divison by 2 is better (assuming we give the same penalty for
    bituse for Hadamard=0 and 1)
    */

    current_sad += best_sad/2;
  }
  else
  {
    register int *p;
    {
      for (i=0;i<4;i++)
      {
        p = m7[i];
        for (j=0;j<4;j++)
          current_sad+=ByteAbs(*p++);
      }
    }
  }
  return current_sad;
}



/*!
 ************************************************************************
 * \brief
 *    control the sign of a with b
 ************************************************************************
 */
int sign(int a,int b)
{
  int x;
  x=absm(a);
  if (b >= 0)
    return x;
  else
    return -x;
}

static int ByteAbsLookup[] = {
  255,254,253,252,251,250,249,248,247,246,245,244,243,242,241,240,
  239,238,237,236,235,234,233,232,231,230,229,228,227,226,225,224,
  223,222,221,220,219,218,217,216,215,214,213,212,211,210,209,208,
  207,206,205,204,203,202,201,200,199,198,197,196,195,194,193,192,
  191,190,189,188,187,186,185,184,183,182,181,180,179,178,177,176,
  175,174,173,172,171,170,169,168,167,166,165,164,163,162,161,160,
  159,158,157,156,155,154,153,152,151,150,149,148,147,146,145,144,
  143,142,141,140,139,138,137,136,135,134,133,132,131,130,129,128,
  127,126,125,124,123,122,121,120,119,118,117,116,115,114,113,112,
  111,110,109,108,107,106,105,104,103,102,101,100, 99, 98, 97, 96,
   95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80,
   79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64,
   63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48,
   47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
   31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16,
   15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
    1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
   17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
   33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
   49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
   65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
   81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
   97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,112,
  113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
  129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,
  145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,
  161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,
  177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,
  193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,
  209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,
  225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,
  241,242,243,244,245,246,247,248,249,250,251,252,253,254,255 };

__inline int ByteAbs (int foo)
{
  return ByteAbsLookup [foo+255];
}


void InitMotionVectorSearchModule ()
{
  int i,j,k,l,l2;

  // Set up the static pointer for MV_bituse;
  MVBitUse = img->mv_bituse;

  // initialize Spiral Search statics

  k=1;
  for (l=1; l < 41; l++)
  {
    l2=l+l;
    for (i=-l+1; i < l; i++)
    {
      for (j=-l; j < l+1; j += l2)
      {
        SpiralX[k] = i;
        SpiralY[k] = j;
        k++;
      }
    }

    for (j=-l; j <= l; j++)
    {
      for (i=-l;i <= l; i += l2)
      {
        SpiralX[k] = i;
        SpiralY[k] = j;
        k++;
      }
    }
  }
}
