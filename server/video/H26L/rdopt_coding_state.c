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
 * \file rdopt_coding_state.c
 *
 * \brief
 *    Storing/restoring coding state for
 *    Rate-Distortion optimized mode decision
 *
 * \author
 *    Heiko Schwarz
 *
 * \date
 *    17. April 2001
 **************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "rdopt_coding_state.h"

//===== static variable: structure for storing the coding state =====
RDCodingState  *cs = 0;



/*!
 ************************************************************************
 * \brief
 *    delete structure for storing coding state
 ************************************************************************
 */
void
clear_coding_state ()
{
  if (cs != NULL)
    {
      //=== structures of data partition array ===
      if (cs->encenv    != NULL)   free (cs->encenv);
      if (cs->bitstream != NULL)   free (cs->bitstream);

      //=== contexts for binary arithmetic coding ===
      delete_contexts_MotionInfo  (cs->mot_ctx);
      delete_contexts_TextureInfo (cs->tex_ctx);

      //=== coding state structure ===
      free (cs);
      cs=NULL;
    }
}



/*!
 ************************************************************************
 * \brief
 *    create structure for storing coding state
 ************************************************************************
 */
void
init_coding_state ()
{
  //=== coding state structure ===
  if ((cs = (RDCodingState*) calloc (1, sizeof(RDCodingState))) == NULL)
    no_mem_exit("init_coding_state: cs");

  //=== important variables of data partition array ===
  cs->no_part = img->currentSlice->max_part_nr;
  if (input->symbol_mode == CABAC)
  {
    if ((cs->encenv = (EncodingEnvironment*) calloc (cs->no_part, sizeof(EncodingEnvironment))) == NULL)
      no_mem_exit("init_coding_state: cs->encenv");
  }
  else
  {
    cs->encenv = NULL;
  }
  if ((cs->bitstream = (Bitstream*) calloc (cs->no_part, sizeof(Bitstream))) == NULL)
    no_mem_exit("init_coding_state: cs->bitstream");

  //=== context for binary arithmetic coding ===
  cs->symbol_mode = input->symbol_mode;
  if (cs->symbol_mode == CABAC)
  {
    cs->mot_ctx = create_contexts_MotionInfo ();
    cs->tex_ctx = create_contexts_TextureInfo();
  }
  else
  {
    cs->mot_ctx = NULL;
    cs->tex_ctx = NULL;
  }
}



/*!
 ************************************************************************
 * \brief
 *    store coding state (for rd-optimized mode decision)
 ************************************************************************
 */
void
store_coding_state ()
{
  int  i, j, k;

  EncodingEnvironment  *ee_src, *ee_dest;
  Bitstream            *bs_src, *bs_dest;

  MotionInfoContexts   *mc_src  = img->currentSlice->mot_ctx;
  TextureInfoContexts  *tc_src  = img->currentSlice->tex_ctx;
  MotionInfoContexts   *mc_dest = cs->mot_ctx;
  TextureInfoContexts  *tc_dest = cs->tex_ctx;
  Macroblock           *currMB  = &(img->mb_data [img->current_mb_nr]);


  //=== important variables of data partition array ===
  for (i = 0; i < cs->no_part; i++)
  {
    ee_src  = &(img->currentSlice->partArr[i].ee_cabac);
    bs_src  =   img->currentSlice->partArr[i].bitstream;
    ee_dest = &(cs->encenv   [i]);
    bs_dest = &(cs->bitstream[i]);

    //--- parameters of encoding environments ---
    if (cs->symbol_mode == CABAC)
    {
      ee_dest->Elow            = ee_src->Elow;
      ee_dest->Ehigh           = ee_src->Ehigh;
      ee_dest->Ebuffer         = ee_src->Ebuffer;
      ee_dest->Ebits_to_go     = ee_src->Ebits_to_go;
      ee_dest->Ebits_to_follow = ee_src->Ebits_to_follow;
    }

    //--- bitstream parameters ---
    bs_dest->byte_pos          = bs_src->byte_pos;
    bs_dest->bits_to_go        = bs_src->bits_to_go;
    bs_dest->byte_buf          = bs_src->byte_buf;
    bs_dest->stored_byte_pos   = bs_src->stored_byte_pos;
    bs_dest->stored_bits_to_go = bs_src->stored_bits_to_go;
    bs_dest->stored_byte_buf   = bs_src->stored_byte_buf;
  }


  //=== contexts for binary arithmetic coding ===
  if (cs->symbol_mode == CABAC)
  {
    //--- motion coding contexts ---
    for (i = 0; i < 2; i++)
    {
      for (j = 0; j < NUM_MB_TYPE_CTX; j++)
        biari_copy_context (&(mc_src->mb_type_contexts[i][j]), &(mc_dest->mb_type_contexts[i][j]));
      for (j = 0; j < NUM_MV_RES_CTX; j++)
        biari_copy_context (&(mc_src->mv_res_contexts[i][j]), &(mc_dest->mv_res_contexts[i][j]));
    }
    for (i = 0; i < NUM_REF_NO_CTX; i++)
      biari_copy_context (&(mc_src->ref_no_contexts[i]), &(mc_dest->ref_no_contexts[i]));

    //--- texture coding contexts ---
    for (i = 0; i < 6; i++)
      for (j = 0; j < NUM_IPR_CTX; j++)
        biari_copy_context (&(tc_src->ipr_contexts[i][j]), &(tc_dest->ipr_contexts[i][j]));
    for (i = 0; i < 2; i++)
      for (j = 0; j < 3; j++)
        for (k = 0; k < NUM_CBP_CTX; k++)
          biari_copy_context (&(tc_src->cbp_contexts[i][j][k]), &(tc_dest->cbp_contexts[i][j][k]));
    for (i = 0; i < NUM_TRANS_TYPE; i++)
    {
      for (j = 0; j < NUM_LEVEL_CTX; j++)
        biari_copy_context (&(tc_src->level_context[i][j]), &(tc_dest->level_context[i][j]));
      for (j = 0; j < NUM_RUN_CTX; j++)
        biari_copy_context (&(tc_src->run_context[i][j]), &(tc_dest->run_context[i][j]));
    }
  }


  //=== syntax element number and bitcounters ===
  cs->currSEnr = currMB->currSEnr;
  for (i = 0; i < MAX_BITCOUNTER_MB; i++)
    cs->bitcounter[i] = currMB->bitcounter[i];

  //=== elements of current macroblock ===
  memcpy (cs->mvd, currMB->mvd, 2*2*BLOCK_MULTIPLE*BLOCK_MULTIPLE*sizeof(int));
}



/*!
 ************************************************************************
 * \brief
 *    restore coding state (for rd-optimized mode decision)
 ************************************************************************
 */
void
restore_coding_state ()
{
  int  i, j, k;

  EncodingEnvironment  *ee_src, *ee_dest;
  Bitstream            *bs_src, *bs_dest;

  MotionInfoContexts   *mc_dest = img->currentSlice->mot_ctx;
  TextureInfoContexts  *tc_dest = img->currentSlice->tex_ctx;
  MotionInfoContexts   *mc_src  = cs->mot_ctx;
  TextureInfoContexts  *tc_src  = cs->tex_ctx;
  Macroblock           *currMB  = &(img->mb_data [img->current_mb_nr]);


  //=== important variables of data partition array ===
  for (i = 0; i < cs->no_part; i++)
  {
    ee_dest = &(img->currentSlice->partArr[i].ee_cabac);
    bs_dest =   img->currentSlice->partArr[i].bitstream;
    ee_src  = &(cs->encenv   [i]);
    bs_src  = &(cs->bitstream[i]);

    //--- parameters of encoding environments ---
    if (cs->symbol_mode == CABAC)
    {
      ee_dest->Elow            = ee_src->Elow;
      ee_dest->Ehigh           = ee_src->Ehigh;
      ee_dest->Ebuffer         = ee_src->Ebuffer;
      ee_dest->Ebits_to_go     = ee_src->Ebits_to_go;
      ee_dest->Ebits_to_follow = ee_src->Ebits_to_follow;
    }

    //--- bitstream parameters ---
    bs_dest->byte_pos          = bs_src->byte_pos;
    bs_dest->bits_to_go        = bs_src->bits_to_go;
    bs_dest->byte_buf          = bs_src->byte_buf;
    bs_dest->stored_byte_pos   = bs_src->stored_byte_pos;
    bs_dest->stored_bits_to_go = bs_src->stored_bits_to_go;
    bs_dest->stored_byte_buf   = bs_src->stored_byte_buf;
  }


  //=== contexts for binary arithmetic coding ===
  if (cs->symbol_mode == CABAC)
  {
    //--- motion coding contexts ---
    for (i = 0; i < 2; i++)
    {
      for (j = 0; j < NUM_MB_TYPE_CTX; j++)
        biari_copy_context (&(mc_src->mb_type_contexts[i][j]), &(mc_dest->mb_type_contexts[i][j]));
      for (j = 0; j < NUM_MV_RES_CTX; j++)
        biari_copy_context (&(mc_src->mv_res_contexts[i][j]), &(mc_dest->mv_res_contexts[i][j]));
    }
    for (i = 0; i < NUM_REF_NO_CTX; i++)
      biari_copy_context (&(mc_src->ref_no_contexts[i]), &(mc_dest->ref_no_contexts[i]));

    //--- texture coding contexts ---
    for (i = 0; i < 6; i++)
      for (j = 0; j < NUM_IPR_CTX; j++)
        biari_copy_context (&(tc_src->ipr_contexts[i][j]), &(tc_dest->ipr_contexts[i][j]));
      for (i = 0; i < 2; i++)
        for (j = 0; j < 3; j++)
          for (k = 0; k < NUM_CBP_CTX; k++)
            biari_copy_context (&(tc_src->cbp_contexts[i][j][k]), &(tc_dest->cbp_contexts[i][j][k]));
      for (i = 0; i < NUM_TRANS_TYPE; i++)
      {
        for (j = 0; j < NUM_LEVEL_CTX; j++)
          biari_copy_context (&(tc_src->level_context[i][j]), &(tc_dest->level_context[i][j]));
        for (j = 0; j < NUM_RUN_CTX; j++)
          biari_copy_context (&(tc_src->run_context[i][j]), &(tc_dest->run_context[i][j]));
      }
  }


  //=== syntax element number and bitcounters ===
  currMB->currSEnr = cs->currSEnr;
  for (i = 0; i < MAX_BITCOUNTER_MB; i++)
    currMB->bitcounter[i] = cs->bitcounter[i];

  //=== elements of current macroblock ===
  memcpy (currMB->mvd, cs->mvd, 2*2*BLOCK_MULTIPLE*BLOCK_MULTIPLE*sizeof(int));
}
