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
 ***************************************************************************
 * \file rdopt.c
 *
 * \brief
 *    Rate-Distortion optimized mode decision
 *
 * \author
 *    Heiko Schwarz <hschwarz@hhi.de>
 *
 * \date
 *    12. April 2001
 **************************************************************************
 */

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "rdopt.h"
#include "refbuf.h"

extern const byte PRED_IPRED[7][7][6];
extern       int  QP2QUANT  [32];

//=== static parameters ===
RDOpt  *rdopt     = NULL;


int    *forward_me_done[9];
int    *tot_for_sad[9];


/*!
 ************************************************************************
 * \brief
 *    delete structure for RD-optimized mode decision
 ************************************************************************
 */
void clear_rdopt ()
{
  int i;

  // rd-optimization structure
  if (rdopt != NULL)
  {
    free (rdopt);
    rdopt = NULL;

    for (i=0; i<9; i++)
    {
      free (forward_me_done[i]);
      free (tot_for_sad    [i]);
    }
  }

  // structure for saving the coding state
  clear_coding_state ();
}


/*!
 ************************************************************************
 * \brief
 *    create structure for RD-optimized mode decision
 ************************************************************************
 */
void
init_rdopt ()
{
  int i;

  if (!input->rdopt)
    return;

  clear_rdopt ();

  // rd-optimization structure
  if ((rdopt = (RDOpt*) calloc (1, sizeof(RDOpt))) == NULL)
  {
    no_mem_exit ("init_rdopt: rdopt");
  }

  for (i=0; i<9; i++)
  {
    if ((forward_me_done[i] = (int*)malloc (img->buf_cycle*sizeof(int))) == NULL) no_mem_exit ("init_rdopt: forward_me_done");
    if ((tot_for_sad    [i] = (int*)malloc (img->buf_cycle*sizeof(int))) == NULL) no_mem_exit ("init_rdopt: tot_for_sad");
  }


  // structure for saving the coding state
  init_coding_state ();
}



/*!
 ************************************************************************
 * \brief
 *    set Rate-Distortion cost of 4x4 Intra modes
 ************************************************************************
 */
void
RDCost_Intra4x4_Block (int block_x,
                       int block_y,
                       int ipmode)
{
  int             dummy, left_ipmode, upper_ipmode, curr_ipmode, x, y;
  int             i            = block_x >> 2;
  int             j            = block_y >> 2;
  int             pic_block_x  = (img->pix_x+block_x) / BLOCK_SIZE;
  int             pic_block_y  = (img->pix_y+block_y) / BLOCK_SIZE;
  int             even_block   = ((i & 1) == 1);
  Slice          *currSlice    =  img->currentSlice;
  Macroblock     *currMB       = &img->mb_data[img->current_mb_nr];
  SyntaxElement  *currSE       = &img->MB_SyntaxElements[currMB->currSEnr];
  int            *partMap      = assignSE2partition[input->partition_mode];
  DataPartition  *dataPart;


  //===== perform DCT, Q, IQ, IDCT =====
  dct_luma (block_x, block_y, &dummy);

  //===== get distortion (SSD) of 4x4 block =====
  for   (y=0; y < BLOCK_SIZE; y++)
    for (x=0; x < BLOCK_SIZE; x++)
    {
      dummy = imgY_org[img->pix_y+block_y+y][img->pix_x+block_x+x] - img->m7[x][y];
      rdopt->distortion_4x4[j][i][ipmode] += img->quad[abs(dummy)];
    }
  rdopt->rdcost_4x4[j][i][ipmode] = (double)rdopt->distortion_4x4[j][i][ipmode];
  if (rdopt->rdcost_4x4[j][i][ipmode] >= rdopt->min_rdcost_4x4[j][i])
    return;

  //===== store the coding state =====
  store_coding_state ();

  //===== get (estimate) rate for intra prediction mode =====
  //--- set or estimate the values of syntax element ---
  if (even_block)
    // EVEN BLOCK: the values the of syntax element can be set correctly
  {
    // value of last block
    upper_ipmode   =  img->ipredmode [pic_block_x  ][pic_block_y  ];
    left_ipmode    =  img->ipredmode [pic_block_x-1][pic_block_y+1];
    curr_ipmode    =  img->ipredmode [pic_block_x  ][pic_block_y+1];
    currSE->value1 =  PRED_IPRED [upper_ipmode+1][left_ipmode+1][curr_ipmode];
    // value of this block
    upper_ipmode   =  img->ipredmode [pic_block_x+1][pic_block_y  ];
    currSE->value2 =  PRED_IPRED [upper_ipmode+1][curr_ipmode+1][ipmode];
  }
  else
  /* ODD BLOCK:  the second value of the syntax element cannot be set, since
   *             it will be known only after the next step. For estimating
   *             the rate the corresponding prediction mode is set to zero
   *             (minimum rate). */
  {
    upper_ipmode   =  img->ipredmode [pic_block_x+1][pic_block_y  ];
    left_ipmode    =  img->ipredmode [pic_block_x  ][pic_block_y+1];
    currSE->value1 =  PRED_IPRED [upper_ipmode+1][left_ipmode+1][ipmode];
    currSE->value2 =  0; // value with maximum probability
  }
  //--- set function pointers for syntax element ---
  if (input->symbol_mode == UVLC)   currSE->mapping = intrapred_linfo;
  else                currSE->writing = writeIntraPredMode2Buffer_CABAC;
  currSE->type = SE_INTRAPREDMODE;
  //--- choose the appropriate data partition ---
  if (img->type != B_IMG)     dataPart = &(currSlice->partArr[partMap[SE_INTRAPREDMODE]]);
  else                        dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
  //--- get length of syntax element ---
  dataPart->writeSyntaxElement (currSE, dataPart);
  rdopt->rate_imode_4x4[j][i][ipmode] = currSE->len;
  rdopt->rdcost_4x4[j][i][ipmode] += rdopt->lambda_intra*(double)rdopt->rate_imode_4x4[j][i][ipmode];
  if (rdopt->rdcost_4x4[j][i][ipmode] >= rdopt->min_rdcost_4x4[j][i])
  {
    restore_coding_state ();
    return;
  }
  currSE++;
  currMB->currSEnr++;

  //===== get rate for luminance coefficients =====
  rdopt->rate_luma_4x4[j][i][ipmode] = writeMB_bits_for_4x4_luma (i, j, 0);
  rdopt->rdcost_4x4[j][i][ipmode] += rdopt->lambda_intra*(double)rdopt->rate_luma_4x4[j][i][ipmode];

  //===== restore the saved coding state =====
  restore_coding_state ();

  //===== update best mode =====
  if (rdopt->rdcost_4x4[j][i][ipmode] < rdopt->min_rdcost_4x4[j][i])
  {
    rdopt->best_mode_4x4 [j][i] = ipmode;
    rdopt->min_rdcost_4x4[j][i] = rdopt->rdcost_4x4[j][i][ipmode];
  }
}


/*!
 ************************************************************************
 * \brief
 *    Get best 4x4 Intra mode by rate-dist-based decision
 ************************************************************************
 */
int
RD_Intra4x4_Mode_Decision (int block_x,
                           int block_y)
{
  int  ipmode, i, j;
  int  index_y     = block_y >> 2;
  int  index_x     = block_x >> 2;
  int  pic_pix_y   = img->pix_y + block_y;
  int  pic_pix_x   = img->pix_x + block_x;
  int  pic_block_y = pic_pix_y / BLOCK_SIZE;
  int  pic_block_x = pic_pix_x / BLOCK_SIZE;

  typedef int (*WRITE_SE)(SyntaxElement*, struct datapartition*);
  WRITE_SE     *writeSE=0;
  int           symbol_mode;



  /*=== set symbol mode to UVLC (CABAC does not work correctly -> wrong coding order) ===*
   *=== and store the original symbol mode and function pointers                      ===*/
  symbol_mode = input->symbol_mode;
  if (input->symbol_mode != UVLC)
  {
    input->symbol_mode = UVLC;
    if ((writeSE = (WRITE_SE*) calloc (img->currentSlice->max_part_nr, sizeof (WRITE_SE))) == NULL)
    {
      no_mem_exit ("RD_Intra4x4_Mode_Decision: writeSE");
    }
    for (i=0; i<img->currentSlice->max_part_nr; i++)
    {
      writeSE[i] = img->currentSlice->partArr[i].writeSyntaxElement;
      img->currentSlice->partArr[i].writeSyntaxElement = writeSyntaxElement_UVLC;
    }
  }


  //=== LOOP OVER ALL 4x4 INTRA PREDICTION MODES ===
  rdopt->best_mode_4x4 [index_y][index_x] = -1;
  rdopt->min_rdcost_4x4[index_y][index_x] =  1e30;
  for (ipmode=0; ipmode < NO_INTRA_PMODE; ipmode++)
  {
    rdopt->rdcost_4x4    [index_y][index_x][ipmode] = -1;
    rdopt->distortion_4x4[index_y][index_x][ipmode] =  0;
    rdopt->rate_imode_4x4[index_y][index_x][ipmode] =  0;
    rdopt->rate_luma_4x4 [index_y][index_x][ipmode] =  0;

    // Horizontal pred from Y neighbour pix , vertical use X pix, diagonal needs both
    if (ipmode==DC_PRED || ipmode==HOR_PRED || img->ipredmode[pic_block_x+1][pic_block_y] >= 0)
      // DC or vert pred or hor edge
    {
      if (ipmode==DC_PRED || ipmode==VERT_PRED || img->ipredmode[pic_block_x][pic_block_y+1] >= 0)
        // DC or hor pred or vert edge
      {
        // find diff
        for   (j=0; j < BLOCK_SIZE; j++)
          for (i=0; i < BLOCK_SIZE; i++)
            img->m7[i][j] = imgY_org[pic_pix_y+j][pic_pix_x+i] - img->mprr[ipmode][j][i];
          // copy intra prediction block
          for   (j=0; j < BLOCK_SIZE; j++)
            for (i=0; i < BLOCK_SIZE; i++)
              img->mpr[i+block_x][j+block_y] = img->mprr[ipmode][j][i];
          // get lagrangian cost
          RDCost_Intra4x4_Block (block_x, block_y, ipmode);
      }
    }
  }

  //=== restore the original symbol mode and function pointers ===
  if (symbol_mode != UVLC)
  {
    input->symbol_mode = symbol_mode;
    for (i=0; i<img->currentSlice->max_part_nr; i++)
    {
      img->currentSlice->partArr[i].writeSyntaxElement = writeSE[i];
    }
    free (writeSE);
  }

  return rdopt->best_mode_4x4[index_y][index_x];
}


/*!
 ************************************************************************
 * \brief
 *    Rate-Distortion opt. mode decision for 4x4 intra blocks
 ************************************************************************
 */
void
Intra4x4_Mode_Decision ()
{
  int i,j;
  int block_x, block_y;
  int best_ipmode;
  int coeff_cost; // not used
  int pic_pix_y,pic_pix_x,pic_block_y,pic_block_x;
  int last_ipred=0;           // keeps last chosen intra prediction mode for 4x4 intra pred
  int nonzero;                          // keep track of nonzero coefficients
  int cbp_mask;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  // start making 4x4 intra prediction
  currMB->cbp=0;
  img->mb_data[img->current_mb_nr].intraOrInter = INTRA_MB_4x4;

  for(block_y = 0 ; block_y < MB_BLOCK_SIZE ; block_y += BLOCK_MULTIPLE)
  {
    pic_pix_y=img->pix_y+block_y;
    pic_block_y=pic_pix_y/BLOCK_SIZE;

    for(block_x = 0 ; block_x < MB_BLOCK_SIZE  ; block_x += BLOCK_MULTIPLE)
    {
      cbp_mask=(1<<(2*(block_y/8)+block_x/8));
      pic_pix_x=img->pix_x+block_x;
      pic_block_x=pic_pix_x/BLOCK_SIZE;

      /*
      intrapred_luma() makes and returns 4x4 blocks with all 5 intra prediction modes.
      Notice that some modes are not possible at frame edges.
      */
      intrapred_luma (pic_pix_x,pic_pix_y);

      //=== rate-constrained mode decision ===
      best_ipmode = RD_Intra4x4_Mode_Decision (block_x, block_y);

      img->ipredmode[pic_block_x+1][pic_block_y+1]=best_ipmode;

      if ((pic_block_x & 1) == 1) // just even blocks, two and two predmodes are sent together
      {
        currMB->intra_pred_modes[block_x/4+block_y]=PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][best_ipmode];
        currMB->intra_pred_modes[block_x/4-1+block_y]=last_ipred;
      }
      last_ipred=PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][best_ipmode];

      //  Make difference from prediction to be transformed
      for (j=0; j < BLOCK_SIZE; j++)
        for (i=0; i < BLOCK_SIZE; i++)
        {
          img->mpr[i+block_x][j+block_y]=img->mprr[best_ipmode][j][i];
          img->m7[i][j] =imgY_org[pic_pix_y+j][pic_pix_x+i] - img->mprr[best_ipmode][j][i];
        }

        nonzero = dct_luma(block_x,block_y,&coeff_cost);

        if (nonzero)
        {
          currMB->cbp |= cbp_mask;// set coded block pattern if there are nonzero coeffs
        }
    }
  }
}



/*!
 ************************************************************************
 * \brief
 *    Rate-Distortion cost of a macroblock
 ************************************************************************
 * \note
 * the following parameters have to be set before using this function:  \n
 * -------------------------------------------------------------------  \n
 *   for INTRA4x4  : img->imod,
 *                   img->cof,
 *                   currMB->cbp,
 *                   currMB->intra_pred_modes,
 *                   currMB->intraOrInter
 *                                                                      \n
 *   for INTRA16x16: img->imod,
 *                   img->mprr_2
 *                   currMB->intraOrInter
 *                                                                      \n
 *   for INTER     : img->imod,
 *                   img->mv,   (predictors)
 *                   tmp_mv,    (motion vectors)
 *                   currMB->intraOrInter
 *                                                                      \n
 *   for FORWARD   : img->imod,
 *                   img->p_fwMV,
 *                   tmp_fwMV,
 *                   currMB->intraOrInter
 *                                                                      \n
 *   for BACKWARD  : img->imod,
 *                   img->p_bwMV,
 *                   tmp_bwMV,
 *                   currMB->intraOrInter
 *                                                                      \n
 *   for BIDIRECT  : img->imod,
 *                   img->p_fwMV,
 *                   img->p_bwMV,
 *                   tmp_fwMV,
 *                   tmp_bwMV,
 *                   currMB->intraOrInter
 *                                                                      \n
 *   for DIRECT    : img->imod,
 *                   dfMV,
 *                   dbMV,
 *                   currMB->intraOrInter
 *                                                                      \n
 *   for COPY      : currMB->intraOrInter
 *
 ************************************************************************
 */
int
RDCost_Macroblock (RateDistortion  *rd,
                   int              mode,
                   int              ref_or_i16mode,
                   int              blocktype,
                   int              blocktype_back)
{
  Slice          *currSlice =  img->currentSlice;
  Macroblock     *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement  *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int            *partMap   = assignSE2partition[input->partition_mode];
  DataPartition  *dataPart;

  int  i, j, refidx=0, cr_cbp, mb_mode;
  int  bframe       =  (img->type == B_IMG);
  int  copy         =  (mode == MBMODE_COPY);
  int  intra4       =  (mode == MBMODE_INTRA4x4);
  int  intra16      =  (mode == MBMODE_INTRA16x16);
  int  inter        =  (mode >= MBMODE_INTER16x16    && mode <= MBMODE_INTER4x4    && !bframe);
  int  inter_for    =  (mode >= MBMODE_INTER16x16    && mode <= MBMODE_INTER4x4    &&  bframe);
  int  inter_back   =  (mode >= MBMODE_BACKWARD16x16 && mode <= MBMODE_BACKWARD4x4);
  int  inter_bidir  =  (mode == MBMODE_BIDIRECTIONAL);
  int  inter_direct =  (mode == MBMODE_DIRECT);
  int  inter_bframe =  (inter_for || inter_back || inter_bidir || inter_direct);


  //=== init rd-values ===
  rd->rdcost        = 0.0;
  rd->distortion    = 0;
  rd->distortion_UV = 0;
  rd->rate_mode     = 0;
  rd->rate_motion   = 0;
  rd->rate_cbp      = 0;
  rd->rate_luma     = 0;
  rd->rate_chroma   = 0;


  /*======================================================*
   *=====                                            =====*
   *=====   DCT, QUANT, DEQUANT, IDCT OF LUMINANCE   =====*
   *=====                                            =====*
   *======================================================*/
  /*
   * note: - for intra4x4, dct have to be done already
   *       - for copy_mb,  no dct coefficients are needed
   */
  if (intra16)
  {
    //===== INTRA 16x16 MACROBLOCK =====
    dct_luma2 (ref_or_i16mode);
    currMB->cbp = 0;
  }
  else if (inter)
  {
    //===== INTER MACROBLOCK =====
    img->multframe_no = (img->number-ref_or_i16mode-1) % img->buf_cycle;
    LumaResidualCoding_P ();
  }
  else if (inter_bframe)
  {
    //===== INTER MACROBLOCK in B-FRAME =====
    img->fw_multframe_no = (img->number-ref_or_i16mode-1) % img->buf_cycle;
    LumaResidualCoding_B ();
  }


  /*==============================================================*
   *=====   PREDICTION, DCT, QUANT, DEQUANT OF CHROMINANCE   =====*
   *==============================================================*/
  if (!copy)
  {
    if (bframe)  ChromaCoding_B (&cr_cbp);
    else         ChromaCoding_P (&cr_cbp);
  }




  /*=========================================================*
   *=====                                               =====*
   *=====   GET DISTORTION OF LUMINANCE AND INIT COST   =====*
   *=====                                               =====*
   *=========================================================*/
  if (input->rdopt==2)
  {
    int k;
    for (k=0; k < input->NoOfDecoders ;k++)
    {
      decode_one_macroblock(k, mode, ref_or_i16mode);
      for (j=img->pix_y; j<img->pix_y+MB_BLOCK_SIZE; j++)
        for (i=img->pix_x; i<img->pix_x+MB_BLOCK_SIZE; i++)
        {
          rd->distortion += img->quad[abs (imgY_org[j][i] - decY[k][j][i])];
        }
    }
    rd->distortion /= input->NoOfDecoders;
    refidx = (img->number-1) % img->buf_cycle; /* reference frame index */
  }
  else
  {
    if (copy)
    {
      refidx = (img->number-1) % img->buf_cycle; // reference frame index
      for   (j=img->pix_y; j<img->pix_y+MB_BLOCK_SIZE; j++)
        for (i=img->pix_x; i<img->pix_x+MB_BLOCK_SIZE; i++)
          rd->distortion += img->quad [abs (imgY_org[j][i] - FastPelY_14(mref[refidx], j<<2, i<<2))];
    }
    else
    {
      for   (j=img->pix_y; j<img->pix_y+MB_BLOCK_SIZE; j++)
        for (i=img->pix_x; i<img->pix_x+MB_BLOCK_SIZE; i++)
          rd->distortion += img->quad [abs (imgY_org[j][i] - imgY[j][i])];
    }
  }
  //=== init and check rate-distortion cost ===
  rd->rdcost = (double)rd->distortion;
  if (rd->rdcost >= rdopt->min_rdcost)
    return 0;


  /*=============================================================*
   *=====                                                   =====*
   *=====   GET DISTORTION OF CHROMINANCE AND UPDATE COST   =====*
   *=====                                                   =====*
   *=============================================================*/
  if (copy)
  {
    for   (j=img->pix_c_y; j<img->pix_c_y+MB_BLOCK_SIZE/2; j++)
      for (i=img->pix_c_x; i<img->pix_c_x+MB_BLOCK_SIZE/2; i++)
      {
        rd->distortion_UV += img->quad[abs (imgUV_org[0][j][i] - mcef[refidx][0][j][i])];
        rd->distortion_UV += img->quad[abs (imgUV_org[1][j][i] - mcef[refidx][1][j][i])];
      }
  }
  else
  {
    for   (j=img->pix_c_y; j<img->pix_c_y+MB_BLOCK_SIZE/2; j++)
      for (i=img->pix_c_x; i<img->pix_c_x+MB_BLOCK_SIZE/2; i++)
      {
        rd->distortion_UV += img->quad[abs (imgUV_org[0][j][i] - imgUV[0][j][i])];
        rd->distortion_UV += img->quad[abs (imgUV_org[1][j][i] - imgUV[1][j][i])];
      }
  }
  //=== init and check rate-distortion cost ===
  rd->rdcost += (double)rd->distortion_UV;
  if (rd->rdcost >= rdopt->min_rdcost)
    return 0;


  /*================================================*
   *=====                                      =====*
   *=====   GET CODING RATES AND UPDATE COST   =====*
   *=====                                      =====*
   *================================================*/

  //=== store encoding environment ===
  store_coding_state ();


  /*===================================================*
   *=====   GET RATE OF MB-MODE AND UPDATE COST   =====*
   *===================================================*/
  //--- set mode constants ---
  if      (copy || inter_direct)  mb_mode = 0;
  else if (inter)     mb_mode = blocktype;
  else if (intra4)      mb_mode = (img->type<<3);
  else if (intra16)     mb_mode = (img->type<<3) + (cr_cbp<<2) + 12*img->kac + 1 + ref_or_i16mode;
  else if (inter_bidir)     mb_mode = 3;
  else if (inter_for)     mb_mode = (blocktype     ==1 ? blocktype        : (blocktype     <<1));
  else /*  inter_back */    mb_mode = (blocktype_back==1 ? blocktype_back+1 : (blocktype_back<<1)+1);
  currSE->value1 = mb_mode;
  //--- set symbol type and function pointers ---
  if (input->symbol_mode == UVLC)     currSE->mapping = n_linfo2;
  else          currSE->writing = writeMB_typeInfo2Buffer_CABAC;
  currSE->type = SE_MBTYPE;
  //--- choose data partition ---
  if (bframe)   dataPart = &(currSlice->partArr[partMap[SE_MBTYPE]]);
  else          dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
  //--- encode and set rate ---
  dataPart->writeSyntaxElement (currSE, dataPart);
  rd->rate_mode = currSE->len;
  currSE++;
  currMB->currSEnr++;
  //=== encode intra prediction modes ===
  if (intra4)
    for (i=0; i < (MB_BLOCK_SIZE>>1); i++)
    {
      currSE->value1 = currMB->intra_pred_modes[ i<<1   ];
      currSE->value2 = currMB->intra_pred_modes[(i<<1)+1];
      //--- set symbol type and function pointers ---
      if (input->symbol_mode == UVLC)   currSE->mapping = intrapred_linfo;
      else                currSE->writing = writeIntraPredMode2Buffer_CABAC;
      currSE->type = SE_INTRAPREDMODE;
      //--- choose data partition ---
      if (bframe)   dataPart = &(currSlice->partArr[partMap[SE_INTRAPREDMODE]]);
      else          dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
      //--- encode and update rate ---
      dataPart->writeSyntaxElement (currSE, dataPart);
      rd->rate_mode += currSE->len;
      currSE++;
      currMB->currSEnr++;
    }
    //=== update and check rate-distortion cost ===
    rd->rdcost += rdopt->lambda_mode * (double)rd->rate_mode;
    if (rd->rdcost >= rdopt->min_rdcost)
    {
      restore_coding_state ();
      return 0;
    }


  /*=======================================================*
   *=====   GET RATE OF MOTION INFO AND UPDATE COST   =====*
   *=======================================================*/
  if (inter)
  {
    //=== set some parameters ===
    currMB->ref_frame = ref_or_i16mode;
    currMB->mb_type   = mb_mode;
    img->mb_mode      = mb_mode;
    img->blc_size_h   = input->blc_size[blocktype][0];
    img->blc_size_v   = input->blc_size[blocktype][1];
    //=== get rate ===
    rd->rate_motion = writeMotionInfo2NAL_Pframe ();
    //=== update and check rate-distortion cost ===
    rd->rdcost += rdopt->lambda_mode * (double)rd->rate_motion;
    if (rd->rdcost >= rdopt->min_rdcost)
    {
      restore_coding_state ();
      return 0;
    }
  }
  else if (inter_bframe && !inter_direct)
  {
    //=== set some parameters ===
    currMB->ref_frame  = ref_or_i16mode;
    currMB->mb_type    = mb_mode;
    img->mb_mode       = mb_mode;
    img->fw_blc_size_h = input->blc_size[blocktype     ][0];
    img->fw_blc_size_v = input->blc_size[blocktype     ][1];
    img->bw_blc_size_h = input->blc_size[blocktype_back][0];
    img->bw_blc_size_v = input->blc_size[blocktype_back][1];
    //=== get rate ===
    rd->rate_motion = writeMotionInfo2NAL_Bframe ();
    //=== update and check rate-distortion cost ===
    rd->rdcost += rdopt->lambda_mode * (double)rd->rate_motion;
    if (rd->rdcost >= rdopt->min_rdcost)
    {
      restore_coding_state ();
      return 0;
    }
  }



  //===== GET RATE FOR CBP, LUMA and CHROMA =====
  if (intra16)
  {
  /*========================================================*
   *=====   GET RATE OF LUMA (16x16) AND UPDATE COST   =====*
   *========================================================*/
    rd->rate_luma = writeMB_bits_for_16x16_luma ();
    //=== update and check rate-distortion cost ===
    rd->rdcost += rdopt->lambda_mode * (double)rd->rate_luma;
    if (rd->rdcost >= rdopt->min_rdcost)
    {
      restore_coding_state ();
      return 0;
    }
  }
  else if (!copy)
  {
  /*===============================================*
   *=====   GET RATE OF CBP AND UPDATE COST   =====*
   *===============================================*/
    rd->rate_cbp = writeMB_bits_for_CBP ();
    //=== update and check rate-distortion cost ===
    rd->rdcost += rdopt->lambda_mode * (double)rd->rate_cbp;
    if (rd->rdcost >= rdopt->min_rdcost)
    {
      restore_coding_state ();
      return 0;
    }

    /*=========================================================*
     *=====   GET RATE OF LUMA (16x(4x4)) AND UPDATE COST   =====*
     *=========================================================*/
    rd->rate_luma = writeMB_bits_for_luma (0);
    //=== update and check rate-distortion cost ===
    rd->rdcost += rdopt->lambda_mode * (double)rd->rate_luma;
    if (rd->rdcost >= rdopt->min_rdcost)
    {
      restore_coding_state ();
      return 0;
    }
  }


  if (!copy)
  {
  /*==================================================*
   *=====   GET RATE OF CHROMA AND UPDATE COST   =====*
   *==================================================*/
    rd->rate_chroma  = writeMB_bits_for_DC_chroma (0);
    rd->rate_chroma += writeMB_bits_for_AC_chroma (0);
    //=== update and check rate-distortion cost ===
    rd->rdcost += rdopt->lambda_mode * (double)rd->rate_chroma;
    if (rd->rdcost >= rdopt->min_rdcost)
    {
      restore_coding_state ();
      return 0;
    }
  }


  //=== restore encoding environment ===
  restore_coding_state ();


  /*=================================================*
   *=====                                       =====*
   *=====   STORE PARAMETERS OF NEW BEST MODE   =====*
   *=====                                       =====*
   *=================================================*/
  rdopt->min_rdcost     = rd->rdcost;
  rdopt->best_mode      = mode;
  rdopt->ref_or_i16mode = ref_or_i16mode;
  rdopt->blocktype      = blocktype;
  rdopt->blocktype_back = blocktype_back;

  //=== LUMINANCE OF RECONSTRUCTED MACROBLOCK ===
  if (copy)
    for   (j=0; j<MB_BLOCK_SIZE; j++)
      for (i=0; i<MB_BLOCK_SIZE; i++)
        rdopt->rec_mb_Y[j][i] = FastPelY_14(mref[refidx], (img->pix_y+j)<<2, (img->pix_x+i)<<2);
  else
    for   (j=0; j<MB_BLOCK_SIZE; j++)
      for (i=0; i<MB_BLOCK_SIZE; i++)
        rdopt->rec_mb_Y[j][i] = imgY[img->pix_y+j][img->pix_x+i];

  //=== CHROMINANCE OF RECONSTRUCTED MACROBLOCK ===
  if (copy)
    for   (j=0; j<(BLOCK_SIZE<<1); j++)
      for (i=0; i<(BLOCK_SIZE<<1); i++)
      {
        rdopt->rec_mb_U[j][i] = mcef[refidx][0][img->pix_c_y+j][img->pix_c_x+i];
        rdopt->rec_mb_V[j][i] = mcef[refidx][1][img->pix_c_y+j][img->pix_c_x+i];
      }
  else
    for   (j=0; j<(BLOCK_SIZE<<1); j++)
      for (i=0; i<(BLOCK_SIZE<<1); i++)
      {
        rdopt->rec_mb_U[j][i] = imgUV[0][img->pix_c_y+j][img->pix_c_x+i];
        rdopt->rec_mb_V[j][i] = imgUV[1][img->pix_c_y+j][img->pix_c_x+i];
      }

  //=== CBP, KAC, DCT-COEFFICIENTS ===
  if (!copy)
  {
    rdopt->cbp         = currMB->cbp;
    rdopt->cbp_blk     = currMB->cbp_blk;
    rdopt->kac         = img->kac;
    memcpy (rdopt->cof,  img->cof,  1728*sizeof(int));
    memcpy (rdopt->cofu, img->cofu,   20*sizeof(int));
  }
  else
      rdopt->cbp_blk     = 0 ;


  //=== INTRA PREDICTION MODES ===
  if (intra4)
  {
    memcpy (rdopt->ipredmode, currMB->intra_pred_modes, 16*sizeof(int));
  }

  //=== MOTION VECTORS ===
  if (inter)
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        rdopt->   mv[j][i][0] = tmp_mv  [0][img->block_y+j][img->block_x+4+i];
        rdopt->   mv[j][i][1] = tmp_mv  [1][img->block_y+j][img->block_x+4+i];
      }
  else if (inter_for)
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        rdopt->   mv[j][i][0] = tmp_fwMV[0][img->block_y+j][img->block_x+4+i];
        rdopt->   mv[j][i][1] = tmp_fwMV[1][img->block_y+j][img->block_x+4+i];
      }
  else if (inter_back)
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        rdopt->bw_mv[j][i][0] = tmp_bwMV[0][img->block_y+j][img->block_x+4+i];
        rdopt->bw_mv[j][i][1] = tmp_bwMV[1][img->block_y+j][img->block_x+4+i];
      }
  else if (inter_bidir)
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        rdopt->   mv[j][i][0] = tmp_fwMV[0][img->block_y+j][img->block_x+4+i];
        rdopt->   mv[j][i][1] = tmp_fwMV[1][img->block_y+j][img->block_x+4+i];
        rdopt->bw_mv[j][i][0] = tmp_bwMV[0][img->block_y+j][img->block_x+4+i];
        rdopt->bw_mv[j][i][1] = tmp_bwMV[1][img->block_y+j][img->block_x+4+i];
      }

  if (input->rdopt==2)
  {
    int k;
    for (k=0; k<input->NoOfDecoders; k++)
    {
      for (j=img->pix_y; j<img->pix_y+MB_BLOCK_SIZE; j++)
        for (i=img->pix_x; i<img->pix_x+MB_BLOCK_SIZE; i++)
        {
          /* Keep the decoded values of each MB for updating the ref frames*/
          decY_best[k][j][i] = decY[k][j][i];
        }
    }
  }

  return 1; // new best mode
}



#define PFRAME    0
#define B_FORWARD 1
#define B_BACKWARD  2

/*!
 ************************************************************************
 * \brief
 *    Rate-Distortion optimized mode decision
 ************************************************************************
 */
void
RD_Mode_Decision ()
{
  int             i, j, k, k0 = 0, dummy;
  int             mode, intra16mode, refframe, blocktype, blocktype_back;
  int             valid_mode [NO_MAX_MBMODES], valid_intra16mode[4];
  int             bframe, intra_macroblock, any_inter_mode = 0;
  int             tot_inter_sad, min_inter_sad;
  int             backward_me_done[9];
  double          forward_rdcost[9], backward_rdcost[9];
  double          min_rdcost;
  int             max_ref_frame, best_refframe[9];
  RateDistortion  rd;
  int             k1 = 1;
  int             mb_nr           = img->current_mb_nr;
  Macroblock     *currMB          = &img->mb_data[mb_nr];


  /*==============================================*
   *===  INIT PARAMETERS OF RD-OPT. STRUCTURE  ===*
   *==============================================*/

  //--- set lagrange parameters ---
  double qp = (double)img->qp;

  rdopt->lambda_mode = 5.0 * exp (0.1 * qp) * (qp + 5.0) / (34.0 - qp);
  
  if (img->type == B_IMG || img->types == SP_IMG)
    rdopt->lambda_mode *= 4;

  rdopt->lambda_motion = sqrt (rdopt->lambda_mode);
  rdopt->lambda_intra  = rdopt->lambda_mode;

  if (input->rdopt == 2)
  {
    rdopt->lambda_mode=(1.0-input->LossRate/100.0)*rdopt->lambda_mode;
    rdopt->lambda_motion = sqrt (1.0-input->LossRate/100.0) * rdopt->lambda_motion;
  }

  //--- cost values ---
  rdopt->best_mode   = -1;
  rdopt->min_rdcost  =  1e30;
  for (i = 0; i < 9; i++)
  {
    forward_rdcost  [i] = backward_rdcost [i] = -1.0;
    backward_me_done[i] =  0;
    for (j = 0; j < img->buf_cycle; j++)
    {
      forward_me_done[i][j] = 0;
    }
  }


  /*========================*
   *===  SET SOME FLAGS  ===*
   *========================*/
  bframe           = (img->type==B_IMG);
  intra_macroblock = (img->mb_y==img->mb_y_upd && img->mb_y_upd!=img->mb_y_intra) || img->type==INTRA_IMG;
  max_ref_frame    = min (img->number, img->buf_cycle);

  /*========================================*
   *===  MARK POSSIBLE MACROBLOCK MODES  ===*
   *========================================*/
  //--- reset arrays ---
  memset (valid_mode,        0, sizeof(int)*NO_MAX_MBMODES);
  memset (valid_intra16mode, 0, sizeof(int)*4);
  //--- set intra modes ---
  valid_mode [MBMODE_INTRA4x4  ] = 1;
  valid_mode [MBMODE_INTRA16x16] = 1;
  //--- set inter modes ---
  if (!intra_macroblock || bframe)
  {
    for (i=MBMODE_INTER16x16; i <= MBMODE_INTER4x4; i++)
      if (input->blc_size [i][0] > 0)
        any_inter_mode = valid_mode [i] = 1;
      if (bframe)
      {
        memcpy (valid_mode+MBMODE_BACKWARD16x16,
                valid_mode+MBMODE_INTER16x16,   NO_INTER_MBMODES*sizeof(int));
        valid_mode[MBMODE_BIDIRECTIONAL] = valid_mode[MBMODE_DIRECT] = any_inter_mode;
      }
      else
      {
        valid_mode[MBMODE_COPY] = valid_mode[MBMODE_INTER16x16];
      }
  }

  if (img->type==INTER_IMG && img->types== SP_IMG)
    valid_mode[MBMODE_COPY] =0;

  //===== LOOP OVER ALL MACROBLOCK MODES =====
  for (mode = 0; mode < NO_MAX_MBMODES; mode++)
    if (valid_mode[mode])
    {
    /*==================================================================*
     *===  MOTION ESTIMATION, INTRA PREDICTION and COST CALCULATION  ===*
     *==================================================================*/
       if (mode == MBMODE_INTRA4x4)
       {
       /*======================*
        *=== INTRA 4x4 MODE ===*
        *======================*/
         currMB->intraOrInter = INTRA_MB_4x4;
         img->imod            = INTRA_MB_OLD;
         Intra4x4_Mode_Decision ();   // intra 4x4 prediction, dct, etc.
         RDCost_Macroblock (&rd, mode, 0, 0, 0);
       }
       else if (mode == MBMODE_INTRA16x16)
       {
       /*========================*
        *=== INTRA 16x16 MODE ===*
        *========================*/
         currMB->intraOrInter = INTRA_MB_16x16;
         img->imod            = INTRA_MB_NEW;
         intrapred_luma_2 ();   // make intra pred for all 4 new modes
         find_sad2 (&k);        // get best new intra mode
         RDCost_Macroblock (&rd, mode, k, 0, 0);
       }
       else if (mode == MBMODE_COPY)
       {
       /*=======================*
        *=== COPY MACROBLOCK ===*
        *=======================*/
         currMB->intraOrInter = INTER_MB;
         img->imod            = INTRA_MB_INTER;
         RDCost_Macroblock (&rd, mode, 0, 0, 0);
       }
       else if (!bframe && (mode >= MBMODE_INTER16x16 && mode <= MBMODE_INTER4x4))
       {
       /*===================================*
        *=== INTER MACROBLOCK in P-FRAME ===*
        *===================================*/
         currMB->intraOrInter = INTER_MB;
         img->imod            = INTRA_MB_INTER;

         min_inter_sad = MAX_VALUE;
         for (k=0; k<max_ref_frame; k++)
#ifdef _ADDITIONAL_REFERENCE_FRAME_
           if (k <  input->no_multpred ||
               k == input->add_ref_frame)
#endif
           {
             tot_inter_sad  = (int)floor(rdopt->lambda_motion * (1+2*floor(log(k+1)/log(2)+1e-10)));
             tot_inter_sad += SingleUnifiedMotionSearch (k, mode,
                            refFrArr, tmp_mv, img->mv, PFRAME, img->all_mv,
                            rdopt->lambda_motion);
             if (tot_inter_sad <  min_inter_sad)
             {
               k0 = k;
               min_inter_sad = tot_inter_sad;
             }
           }
         for (  i=0; i < 4; i++)
           for (j=0; j < 4; j++)
           {
             tmp_mv[0][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][k0][mode][0];
             tmp_mv[1][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][k0][mode][1];
           }
         RDCost_Macroblock (&rd, mode, k0, mode, 0);
       }
       else if (bframe && (mode >= MBMODE_INTER16x16 && mode <= MBMODE_INTER4x4))
       {
        /*===============================================*
         *=== FORWARD PREDICTED MACROBLOCK in B-FRAME ===*
         *===============================================*/
         if (forward_rdcost[mode] < 0.0)
         {
           currMB->intraOrInter = INTER_MB;
           img->imod            = B_Forward;

           min_inter_sad = MAX_VALUE;
           for (k=0; k<max_ref_frame; k++)
#ifdef _ADDITIONAL_REFERENCE_FRAME_
             if (k <  input->no_multpred ||
                 k == input->add_ref_frame)
#endif
             {
               if (!forward_me_done[mode][k])
               {
                 tot_for_sad[mode][k]  = (int)floor(rdopt->lambda_motion * (1+2*floor(log(k+1)/log(2)+1e-10)));
                 tot_for_sad[mode][k] += SingleUnifiedMotionSearch (k, mode,
                                         fw_refFrArr, tmp_fwMV, img->p_fwMV,
                                         B_FORWARD, img->all_mv,
                                         rdopt->lambda_motion);
                                         forward_me_done[mode][k] = 1;
               }
               if (tot_for_sad[mode][k] < min_inter_sad)
               {
                 k0 = k;
                 min_inter_sad = tot_for_sad[mode][k];
               }
             }
           for (  i=0; i < 4; i++)
             for (j=0; j < 4; j++)
             {
               tmp_fwMV[0][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][k0][mode][0];
               tmp_fwMV[1][img->block_y+j][img->block_x+i+4]=img->all_mv[i][j][k0][mode][1];
             }
           RDCost_Macroblock (&rd, mode, k0, mode, 0);
           forward_rdcost[mode] = rd.rdcost;
           best_refframe [mode] = k0;
         }
       }
       else if (mode >= MBMODE_BACKWARD16x16 && mode <= MBMODE_BACKWARD4x4)
       {
        /*================================================*
         *=== BACKWARD PREDICTED MACROBLOCK in B-FRAME ===*
         *================================================*/
         if (backward_rdcost [mode-NO_INTER_MBMODES] < 0.0)
         {
           currMB->intraOrInter = INTER_MB;
           img->imod            = B_Backward;
           if (!backward_me_done[mode-NO_INTER_MBMODES])
           {
             SingleUnifiedMotionSearch (0, mode-NO_INTER_MBMODES,
               bw_refFrArr, tmp_bwMV, img->p_bwMV, B_BACKWARD, img->all_bmv,
               rdopt->lambda_motion);
             backward_me_done[mode-NO_INTER_MBMODES]=1;
           }
           else
             for (j = 0; j < 4; j++)
               for (i = 0; i < 4; i++)
               {
                 tmp_bwMV[0][img->block_y+j][img->block_x+i+4]=
                   img->all_bmv[i][j][0][mode-NO_INTER_MBMODES][0];
                 tmp_bwMV[1][img->block_y+j][img->block_x+i+4]=
                   img->all_bmv[i][j][0][mode-NO_INTER_MBMODES][1];
               }
           RDCost_Macroblock (&rd, mode, 0, 0, mode-NO_INTER_MBMODES);
           backward_rdcost[mode-NO_INTER_MBMODES] = rd.rdcost;
         }
       }
       else if (mode == MBMODE_DIRECT)
       {
       /*==============================================*
        *=== DIRECT PREDICTED MACROBLOCK in B-FRAME ===*
        *==============================================*/
         currMB->intraOrInter = INTER_MB;
         img->imod            = B_Direct;
         get_dir (&dummy);
         if (dummy != 100000000 /*MAX_DIR_SAD*/)
         {
           RDCost_Macroblock (&rd, mode, 0, 0, 0);
         }
       }
       else
       {
        /*=====================================================*
         *=== BIDIRECTIONAL PREDICTED MACROBLOCK in B-FRAME ===*
         *=====================================================*/
        //--- get best backward prediction ---
         min_rdcost = 1e30;
         for (blocktype = 1; blocktype < 8; blocktype++)
           if (valid_mode[blocktype+NO_INTER_MBMODES])
           {
             if (backward_rdcost[blocktype] < 0.0)
             {
               //--- get rd-cost's ---
               currMB->intraOrInter = INTER_MB;
               img->imod            = B_Backward;
               if (!backward_me_done[blocktype])
               {
                 SingleUnifiedMotionSearch (0, blocktype,
                   bw_refFrArr, tmp_bwMV, img->p_bwMV, B_BACKWARD, img->all_bmv,
                   rdopt->lambda_motion);
                 backward_me_done[mode]=1;
               }
               else
                 for (j = 0; j < 4; j++)
                   for (i = 0; i < 4; i++)
                   {
                     tmp_bwMV[0][img->block_y+j][img->block_x+i+4]=
                       img->all_bmv[i][j][0][blocktype][0];
                     tmp_bwMV[1][img->block_y+j][img->block_x+i+4]=
                       img->all_bmv[i][j][0][blocktype][1];
                   }
               RDCost_Macroblock (&rd, blocktype+NO_INTER_MBMODES, 0, 0, blocktype);
               backward_rdcost[blocktype] = rd.rdcost;
             }
             if (backward_rdcost[blocktype] < min_rdcost)
             {
               min_rdcost = backward_rdcost[blocktype];
               k0         = blocktype;
             }
           }
         blocktype_back = k0;

         //--- get best forward prediction ---
         min_rdcost = 1e30;
         for (blocktype = 1; blocktype < 8; blocktype++)
           if (valid_mode[blocktype])
           {
             if (forward_rdcost[blocktype] < 0.0)
             {
               //--- get rd-cost's ---
               currMB->intraOrInter = INTER_MB;
               img->imod            = B_Forward;
               min_inter_sad        = MAX_VALUE;
               for (k=0; k<max_ref_frame; k++)
#ifdef _ADDITIONAL_REFERENCE_FRAME_
               if (k <  input->no_multpred ||
                   k == input->add_ref_frame)
#endif
               {
                 if (!forward_me_done[blocktype][k])
                 {
                   tot_for_sad[blocktype][k]  = (int)floor(rdopt->lambda_motion * (1+2*floor(log(k+1)/log(2)+1e-10)));
                   tot_for_sad[blocktype][k] += SingleUnifiedMotionSearch (k, blocktype,
                     fw_refFrArr, tmp_fwMV, img->p_fwMV,
                     B_FORWARD, img->all_mv,
                     rdopt->lambda_motion);
                   forward_me_done[blocktype][k] = 1;
                 }
                 if (tot_for_sad[blocktype][k] < min_inter_sad)
                 {
                   k0 = k;
                   min_inter_sad = tot_for_sad[blocktype][k];
                 }
               }
               for (  i=0; i < 4; i++)
                 for (j=0; j < 4; j++)
                 {
                   tmp_fwMV[0][img->block_y+j][img->block_x+i+4]=
                     img->all_mv[i][j][k0][blocktype][0];
                   tmp_fwMV[1][img->block_y+j][img->block_x+i+4]=
                     img->all_mv[i][j][k0][blocktype][1];
                 }
               RDCost_Macroblock (&rd, blocktype, k0, blocktype, 0);
               forward_rdcost[blocktype] = rd.rdcost;
               best_refframe [blocktype] = k0;
             }
             if (forward_rdcost[blocktype] < min_rdcost)
             {
               min_rdcost = forward_rdcost[blocktype];
               k1         = blocktype;
             }
           }
         blocktype = k1;
         k0        = best_refframe[blocktype];

         //===== COST CALCULATION =====
         for (j = 0; j < 4; j++)
           for (i = 0; i < 4; i++)
           {
             tmp_bwMV[0][img->block_y+j][img->block_x+i+4]=
               img->all_bmv[i][j][0][blocktype_back][0];
             tmp_bwMV[1][img->block_y+j][img->block_x+i+4]=
               img->all_bmv[i][j][0][blocktype_back][1];
             tmp_fwMV[0][img->block_y+j][img->block_x+i+4]=
               img->all_mv[i][j][k0][blocktype][0];
             tmp_fwMV[1][img->block_y+j][img->block_x+i+4]=
               img->all_mv[i][j][k0][blocktype][1];
           }
         img->fw_blc_size_h = input->blc_size[blocktype][0];
         img->fw_blc_size_v = input->blc_size[blocktype][1];
         img->bw_blc_size_h = input->blc_size[blocktype_back][0];
         img->bw_blc_size_v = input->blc_size[blocktype_back][1];
         currMB->intraOrInter = INTER_MB;
         img->imod            = B_Bidirect;
         RDCost_Macroblock (&rd, mode, k0, blocktype, blocktype_back);
       }
   }



  /*======================================*
   *===  SET PARAMETERS FOR BEST MODE  ===*
   *======================================*/
  mode           = rdopt->best_mode;
  refframe       = rdopt->ref_or_i16mode;
  intra16mode    = rdopt->ref_or_i16mode;
  blocktype      = rdopt->blocktype;
  blocktype_back = rdopt->blocktype_back;


  //=== RECONSTRUCTED MACROBLOCK ===
  for   (j=0; j<MB_BLOCK_SIZE; j++)
    for (i=0; i<MB_BLOCK_SIZE; i++)
      imgY[img->pix_y+j][img->pix_x+i] = rdopt->rec_mb_Y[j][i];
  for   (j=0; j<(BLOCK_SIZE<<1); j++)
    for (i=0; i<(BLOCK_SIZE<<1); i++)
    {
      imgUV[0][img->pix_c_y+j][img->pix_c_x+i] = rdopt->rec_mb_U[j][i];
      imgUV[1][img->pix_c_y+j][img->pix_c_x+i] = rdopt->rec_mb_V[j][i];
    }

  //=== INTRAORINTER and IMOD ===
  if (mode == MBMODE_INTRA4x4)
  {
    currMB->intraOrInter = INTRA_MB_4x4;
    img->imod            = INTRA_MB_OLD;
  }
  else if (mode == MBMODE_INTRA16x16)
  {
    currMB->intraOrInter = INTRA_MB_16x16;
    img->imod            = INTRA_MB_NEW;
  }
  else if (!bframe && (mode >= MBMODE_COPY && mode <= MBMODE_INTER4x4))
  {
    currMB->intraOrInter = INTER_MB;
    img->imod            = INTRA_MB_INTER;
  }
  else if (mode >= MBMODE_INTER16x16 && mode <= MBMODE_INTER4x4)
  {
    currMB->intraOrInter = INTER_MB;
    img->imod            = B_Forward;
  }
  else if (mode >= MBMODE_BACKWARD16x16 && mode <= MBMODE_BACKWARD4x4)
  {
    currMB->intraOrInter = INTER_MB;
    img->imod            = B_Backward;
  }
  else if (mode == MBMODE_BIDIRECTIONAL)
  {
    currMB->intraOrInter = INTER_MB;
    img->imod            = B_Bidirect;
  }
  else // mode == MBMODE_DIRECT
  {
    currMB->intraOrInter = INTER_MB;
    img->imod            = B_Direct;
  }

  //=== DCT-COEFFICIENTS, CBP and KAC ===
  if (mode != MBMODE_COPY)
  {
    memcpy (img->cof,  rdopt->cof,  1728*sizeof(int));
    memcpy (img->cofu, rdopt->cofu,   20*sizeof(int));
    currMB->cbp     = rdopt->cbp;
    currMB->cbp_blk = rdopt->cbp_blk;
    img->kac        = rdopt->kac;

    if (img->imod==INTRA_MB_NEW)
      currMB->cbp += 15*img->kac; // correct cbp for new intra modes: needed by CABAC
  }
  else
  {
    memset (img->cof,  0, 1728*sizeof(int));
    memset (img->cofu, 0,   20*sizeof(int));
    currMB->cbp     = 0;
    currMB->cbp_blk = 0;
    img->kac        = 0;
  }

  //=== INTRA PREDICTION MODES ===
  if (mode == MBMODE_INTRA4x4)
    memcpy (currMB->intra_pred_modes, rdopt->ipredmode, 16*sizeof(int));
  else
  {
    for (i=0;i<4;i++)
      for (j=0;j<4;j++)
        img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;
  }

  //=== MOTION VECTORS, REFERENCE FRAME and BLOCK DIMENSIONS ===
  //--- init all motion vectors with (0,0) ---
  currMB->ref_frame = 0;
  if (bframe)
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        tmp_fwMV[0][img->block_y+j][img->block_x+4+i] = 0;
        tmp_fwMV[1][img->block_y+j][img->block_x+4+i] = 0;
        tmp_bwMV[0][img->block_y+j][img->block_x+4+i] = 0;
        tmp_bwMV[1][img->block_y+j][img->block_x+4+i] = 0;
      }
  else
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        tmp_mv  [0][img->block_y+j][img->block_x+4+i] = 0;
        tmp_mv  [1][img->block_y+j][img->block_x+4+i] = 0;
      }
  //--- set used motion vectors ---
  if (!bframe && (mode >= MBMODE_INTER16x16 && mode <= MBMODE_INTER4x4))
  {
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        tmp_mv[0][img->block_y+j][img->block_x+4+i] = rdopt->mv[j][i][0];
        tmp_mv[1][img->block_y+j][img->block_x+4+i] = rdopt->mv[j][i][1];
      }
    currMB->ref_frame = refframe;
    img->blc_size_h   = input->blc_size[blocktype][0];
    img->blc_size_v   = input->blc_size[blocktype][1];
  }
  else if (mode >= MBMODE_INTER16x16 && mode <= MBMODE_INTER4x4)
  {
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        tmp_fwMV[0][img->block_y+j][img->block_x+4+i] = rdopt->mv[j][i][0];
        tmp_fwMV[1][img->block_y+j][img->block_x+4+i] = rdopt->mv[j][i][1];
        dfMV    [0][img->block_y+j][img->block_x+4+i] = 0;
        dfMV    [1][img->block_y+j][img->block_x+4+i] = 0;
        dbMV    [0][img->block_y+j][img->block_x+4+i] = 0;
        dbMV    [1][img->block_y+j][img->block_x+4+i] = 0;
      }
    currMB->ref_frame  = refframe;
    img->fw_blc_size_h = input->blc_size[blocktype][0];
    img->fw_blc_size_v = input->blc_size[blocktype][1];
  }
  else if (mode >= MBMODE_BACKWARD16x16 && mode <= MBMODE_BACKWARD4x4)
  {
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        tmp_bwMV[0][img->block_y+j][img->block_x+4+i] = rdopt->bw_mv[j][i][0];
        tmp_bwMV[1][img->block_y+j][img->block_x+4+i] = rdopt->bw_mv[j][i][1];
        dfMV    [0][img->block_y+j][img->block_x+4+i] = 0;
        dfMV    [1][img->block_y+j][img->block_x+4+i] = 0;
        dbMV    [0][img->block_y+j][img->block_x+4+i] = 0;
        dbMV    [1][img->block_y+j][img->block_x+4+i] = 0;
      }
    currMB->ref_frame  = refframe;
    img->bw_blc_size_h = input->blc_size[blocktype_back][0];
    img->bw_blc_size_v = input->blc_size[blocktype_back][1];
  }
  else if (mode == MBMODE_BIDIRECTIONAL)
  {
    for   (j=0; j<BLOCK_SIZE; j++)
      for (i=0; i<BLOCK_SIZE; i++)
      {
        tmp_fwMV[0][img->block_y+j][img->block_x+4+i] = rdopt->   mv[j][i][0];
        tmp_fwMV[1][img->block_y+j][img->block_x+4+i] = rdopt->   mv[j][i][1];
        tmp_bwMV[0][img->block_y+j][img->block_x+4+i] = rdopt->bw_mv[j][i][0];
        tmp_bwMV[1][img->block_y+j][img->block_x+4+i] = rdopt->bw_mv[j][i][1];
        dfMV    [0][img->block_y+j][img->block_x+4+i] = 0;
        dfMV    [1][img->block_y+j][img->block_x+4+i] = 0;
        dbMV    [0][img->block_y+j][img->block_x+4+i] = 0;
        dbMV    [1][img->block_y+j][img->block_x+4+i] = 0;
      }
    currMB->ref_frame  = refframe;
    img->fw_blc_size_h = input->blc_size[blocktype     ][0];
    img->fw_blc_size_v = input->blc_size[blocktype     ][1];
    img->bw_blc_size_h = input->blc_size[blocktype_back][0];
    img->bw_blc_size_v = input->blc_size[blocktype_back][1];
  }

  //=== MACROBLOCK MODE ===
  if      (mode == MBMODE_COPY || mode == MBMODE_DIRECT)
    img->mb_mode = 0;
  else if (mode == MBMODE_INTRA4x4)
    img->mb_mode = 8*img->type + img->imod;
  else if (mode == MBMODE_INTRA16x16)
    img->mb_mode = 8*img->type + img->imod + intra16mode + 12*img->kac + 4*((currMB->cbp&0xF0)>>4);
  else if (mode == MBMODE_BIDIRECTIONAL)
    img->mb_mode = 3;
  else if (mode >= MBMODE_BACKWARD16x16 && mode <= MBMODE_BACKWARD4x4)
    img->mb_mode = (blocktype_back == 1 ? blocktype_back+1 : 2*blocktype_back+1);
  else if (mode >= MBMODE_INTER16x16 && mode <= MBMODE_INTER4x4 && bframe)
    img->mb_mode = (blocktype == 1 ? blocktype : 2*blocktype);
  else if (blocktype                         == M16x16_MB &&
     currMB->cbp                             == 0         &&
     currMB->ref_frame                       == 0         &&
     tmp_mv[0][img->block_y][img->block_x+4] == 0         &&
     tmp_mv[1][img->block_y][img->block_x+4] == 0)
    img->mb_mode = 0;
  else
    img->mb_mode = blocktype;
  currMB->mb_type = img->mb_mode;


  /*===========================================================*
   *===  SET LOOP FILTER STRENGTH and REFERENCE FRAME INFO  ===*
   *===========================================================*/
  if (!bframe)
  {
    // Set reference frame information for motion vector prediction of future MBs
    SetRefFrameInfo_P();
  }
  else
  {
    // Set reference frame information for motion vector prediction of future MBs
    SetRefFrameInfo_B();
  }
  currMB->qp       = img->qp ;    // this should (or has to be) done somewere else, but where???
  DeblockMb(img, imgY, imgUV) ;   // filter this macroblock ( pixels to the right and above the MB are affected )
  if (input->rdopt == 2)
  {
    for (j=0 ; j<input->NoOfDecoders ; j++)
      DeblockMb(img, decY_best[j],NULL);
  }

  if (img->current_mb_nr==0)
    intras=0;
  if (img->number && (mode==MBMODE_INTRA4x4 || mode==MBMODE_INTRA16x16))
    intras++;

}

