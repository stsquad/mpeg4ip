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
 * \file cabac.c
 *
 * \brief
 *    CABAC entropy coding routines
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Detlev Marpe                    <marpe@hhi.de>
 **************************************************************************************
 */

#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "cabac.h"

/*!
 ************************************************************************
 * \brief
 *    Allocation of contexts models for the motion info
 *    used for arithmetic encoding
 ************************************************************************
 */
MotionInfoContexts* create_contexts_MotionInfo(void)
{

  int j;
  MotionInfoContexts *enco_ctx;


  enco_ctx = (MotionInfoContexts*) calloc(1, sizeof(MotionInfoContexts) );
  if( enco_ctx == NULL )
    no_mem_exit("create_contexts_MotionInfo: enco_ctx");

  for (j=0; j<2; j++)
  {
    enco_ctx->mb_type_contexts[j] = (BiContextTypePtr) malloc(NUM_MB_TYPE_CTX  * sizeof( BiContextType ) );

    if( enco_ctx->mb_type_contexts[j] == NULL )
      no_mem_exit("create_contexts_MotionInfo: enco_ctx->mb_type_contexts");

    enco_ctx->mv_res_contexts[j] = (BiContextTypePtr) malloc(NUM_MV_RES_CTX  * sizeof( BiContextType ) );

    if( enco_ctx->mv_res_contexts[j] == NULL )
      no_mem_exit("create_contexts_MotionInfo: enco_ctx->mv_res_contexts");
  }

  enco_ctx->ref_no_contexts = (BiContextTypePtr) malloc(NUM_REF_NO_CTX * sizeof( BiContextType ) );

  if( enco_ctx->ref_no_contexts == NULL )
    no_mem_exit("create_contexts_MotionInfo: enco_ctx->ref_no_contexts");

  enco_ctx->delta_qp_contexts = (BiContextTypePtr) malloc(NUM_DELTA_QP_CTX * sizeof( BiContextType ) );

  if( enco_ctx->delta_qp_contexts == NULL )
    no_mem_exit("create_contexts_MotionInfo: enco_ctx->delta_qp_contexts");

  return enco_ctx;
}


/*!
 ************************************************************************
 * \brief
 *    Allocates of contexts models for the texture info
 *    used for arithmetic encoding
 ************************************************************************
 */
TextureInfoContexts* create_contexts_TextureInfo(void)
{

  int j,k;
  TextureInfoContexts *enco_ctx;

  enco_ctx = (TextureInfoContexts*) calloc(1, sizeof(TextureInfoContexts) );
  if( enco_ctx == NULL )
    no_mem_exit("create_contexts_TextureInfo: enco_ctx");

  for (j=0; j < 6; j++)
  {

    enco_ctx->ipr_contexts[j] = (BiContextTypePtr) malloc(NUM_IPR_CTX  * sizeof( BiContextType ) );

    if( enco_ctx->ipr_contexts[j] == NULL )
      no_mem_exit("create_contexts_TextureInfo: enco_ctx->ipr_contexts");
  }

  for (k=0; k<2; k++)
    for (j=0; j<3; j++)
    {

      enco_ctx->cbp_contexts[k][j] = (BiContextTypePtr) malloc(NUM_CBP_CTX  * sizeof( BiContextType ) );

      if( enco_ctx->cbp_contexts[k][j] == NULL )
        no_mem_exit("create_contexts_TextureInfo: enco_ctx->cbp_contexts");
    }


  for (j=0; j < NUM_TRANS_TYPE; j++)
  {

    enco_ctx->level_context[j] = (BiContextTypePtr) malloc(NUM_LEVEL_CTX  * sizeof( BiContextType ) );

    if( enco_ctx->level_context[j] == NULL )
      no_mem_exit("create_contexts_TextureInfo: enco_ctx->level_context");


    enco_ctx->run_context[j] = (BiContextTypePtr) malloc(NUM_RUN_CTX  * sizeof( BiContextType ) );

    if( enco_ctx->run_context[j] == NULL )
      no_mem_exit("create_contexts_TextureInfo: enco_ctx->run_context");

  }
  return enco_ctx;
}


/*!
 ************************************************************************
 * \brief
 *    Initializes an array of contexts models with some pre-defined
 *    counts (ini_flag = 1) or with a flat histogram (ini_flag = 0)
 ************************************************************************
 */
void init_contexts_MotionInfo(MotionInfoContexts *enco_ctx, int ini_flag)
{

  int i,j;
  int scale_factor;
  int qp_factor;
  int ini[3];

  if ( (img->width*img->height) <=  (IMG_WIDTH * IMG_HEIGHT) ) //  format <= QCIF
    scale_factor=1;
  else
    scale_factor=2;

  if(img->qp <= 10)
    qp_factor=0;
  else
    qp_factor=img->qp-10;

  for (j=0; j<2; j++)
  {
    if (ini_flag)
    {
      for (i=0; i < NUM_MB_TYPE_CTX; i++)
      {
        ini[0] = MB_TYPE_Ini[j][i][0]+(MB_TYPE_Ini[j][i][3]*qp_factor)/10;
        ini[1] = MB_TYPE_Ini[j][i][1]+(MB_TYPE_Ini[j][i][4]*qp_factor)/10;
        ini[2] = MB_TYPE_Ini[j][i][2]*scale_factor;
        biari_init_context(enco_ctx->mb_type_contexts[j] + i,ini[0],ini[1],ini[2]);
      }
    }
    else
    {
      for (i=0; i < NUM_MB_TYPE_CTX; i++)
        biari_init_context(enco_ctx->mb_type_contexts[j] + i,1,1,100);
    }

    if (ini_flag)
    {
      for (i=0; i < NUM_MV_RES_CTX; i++)
        biari_init_context(enco_ctx->mv_res_contexts[j] + i,MV_RES_Ini[j][i][0]*scale_factor,MV_RES_Ini[j][i][1]*scale_factor,MV_RES_Ini[j][i][2]*scale_factor);
    }
    else
    {
      for (i=0; i < NUM_MV_RES_CTX; i++)
        biari_init_context(enco_ctx->mv_res_contexts[j] + i,1,1,1000);
    }
  }

  if (ini_flag)
  {
    for (i=0; i < NUM_REF_NO_CTX; i++)
      biari_init_context(enco_ctx->ref_no_contexts + i,REF_NO_Ini[i][0]*scale_factor,REF_NO_Ini[i][1]*scale_factor,REF_NO_Ini[i][2]*scale_factor);
  }
  else
  {
    for (i=0; i < NUM_REF_NO_CTX; i++)
      biari_init_context(enco_ctx->ref_no_contexts + i,1,1,1000);
  }

  if (ini_flag)
  {
    for (i=0; i < NUM_DELTA_QP_CTX; i++)
      biari_init_context(enco_ctx->delta_qp_contexts + i,DELTA_QP_Ini[i][0]*scale_factor,DELTA_QP_Ini[i][1]*scale_factor,DELTA_QP_Ini[i][2]*scale_factor);
  }
  else
  {
    for (i=0; i < NUM_DELTA_QP_CTX; i++)
      biari_init_context(enco_ctx->delta_qp_contexts + i,1,1,1000);
  }
}

/*!
 ************************************************************************
 * \brief
 *    Initializes an array of contexts models with some pre-defined
 *    counts (ini_flag = 1) or with a flat histogram (ini_flag = 0)
 ************************************************************************
 */
void init_contexts_TextureInfo(TextureInfoContexts *enco_ctx, int ini_flag)
{

  int i,j,k;
  int scale_factor;
  int qp_factor;
  int ini[3];

  if ( (img->width*img->height) <=  (IMG_WIDTH * IMG_HEIGHT) ) //  format <= QCIF
   scale_factor=1;
  else
   scale_factor=2;

  if(img->qp <= 10)
    qp_factor=0;
  else
    qp_factor=img->qp-10;

  for (j=0; j < 6; j++)
  {
    if (ini_flag)
    {
      for (i=0; i < NUM_IPR_CTX; i++)
        biari_init_context(enco_ctx->ipr_contexts[j] + i,IPR_Ini[j][i][0]*scale_factor,IPR_Ini[j][i][1]*scale_factor,IPR_Ini[j][i][2]*scale_factor);
    }
    else
    {
      for (i=0; i < NUM_IPR_CTX; i++)
        biari_init_context(enco_ctx->ipr_contexts[j] + i,2,1,50);
    }
  }

  for (k=0; k<2; k++)
    for (j=0; j<3; j++)
    {
      if (ini_flag)
      {
        for (i=0; i < NUM_CBP_CTX; i++)
        {
          ini[0] = CBP_Ini[k][j][i][0]+(CBP_Ini[k][j][i][3]*qp_factor)/10;
          ini[1] = CBP_Ini[k][j][i][1]+(CBP_Ini[k][j][i][4]*qp_factor)/10;
          ini[2] = CBP_Ini[k][j][i][2]*scale_factor;
          biari_init_context(enco_ctx->cbp_contexts[k][j] + i,ini[0],ini[1],ini[2]);
        }
      }
      else
      {
        for (i=0; i < NUM_CBP_CTX; i++)
          biari_init_context(enco_ctx->cbp_contexts[k][j] + i,1,1,100);
      }
    }

  for (j=0; j < NUM_TRANS_TYPE; j++)
  {

    if (ini_flag)
    {
      for (i=0; i < NUM_LEVEL_CTX; i++)
      {
        ini[0] = (Level_Ini[j][i][0]+(Level_Ini[j][i][3]*qp_factor)/10)*scale_factor;
        ini[1] = (Level_Ini[j][i][1]+(Level_Ini[j][i][4]*qp_factor)/10)*scale_factor;
        ini[2] = Level_Ini[j][i][2]*scale_factor;
        biari_init_context(enco_ctx->level_context[j] + i,ini[0],ini[1],ini[2]);
      }
    }
    else
    {
      for (i=0; i < NUM_LEVEL_CTX; i++)
        biari_init_context(enco_ctx->level_context[j] + i,1,1,100);
    }

    if (ini_flag)
    {
      for (i=0; i < NUM_RUN_CTX; i++)
      {
        ini[0] = Run_Ini[j][i][0]*scale_factor;
        ini[1] = Run_Ini[j][i][1]*scale_factor;
        ini[2] = Run_Ini[j][i][2]*scale_factor;
        biari_init_context(enco_ctx->run_context[j] + i,ini[0],ini[1],ini[2]);
      }
    }
    else
    {
      for (i=0; i < NUM_RUN_CTX; i++)
        biari_init_context(enco_ctx->run_context[j] + i,1,1,100);
    }
  }
}


/*!
 ************************************************************************
 * \brief
 *    Frees the memory of the contexts models
 *    used for arithmetic encoding of the motion info.
 ************************************************************************
 */
void delete_contexts_MotionInfo(MotionInfoContexts *enco_ctx)
{

  int j;

  if( enco_ctx == NULL )
    return;

  for (j=0; j<2; j++)
  {
    if (enco_ctx->mb_type_contexts[j] != NULL)
      free(enco_ctx->mb_type_contexts[j] );

    if (enco_ctx->mv_res_contexts[j]  != NULL)
      free(enco_ctx->mv_res_contexts[j] );
  }

  if (enco_ctx->ref_no_contexts != NULL)
    free(enco_ctx->ref_no_contexts);

  if (enco_ctx->delta_qp_contexts != NULL)
    free(enco_ctx->delta_qp_contexts);


  free( enco_ctx );

  return;

}

/*!
 ************************************************************************
 * \brief
 *    Frees the memory of the contexts models
 *    used for arithmetic encoding of the texture info.
 ************************************************************************
 */
void delete_contexts_TextureInfo(TextureInfoContexts *enco_ctx)
{

  int j,k;

  if( enco_ctx == NULL )
    return;

  for (j=0; j < 6; j++)
  {
    if (enco_ctx->ipr_contexts[j] != NULL)
      free(enco_ctx->ipr_contexts[j]);
  }

  for (k=0; k<2; k++)
    for (j=0; j<3; j++)
    {
      if (enco_ctx->cbp_contexts[k][j] != NULL)
        free(enco_ctx->cbp_contexts[k][j]);
    }

  for (j=0; j < NUM_TRANS_TYPE; j++)
  {
    if (enco_ctx->level_context[j] != NULL)
      free(enco_ctx->level_context[j]);

    if (enco_ctx->run_context[j] != NULL)
      free(enco_ctx->run_context[j]);
  }
  free( enco_ctx );

  return;

}

/*!
 **************************************************************************
 * \brief
 *    generates arithmetic code and passes the code to the buffer
 **************************************************************************
 */
int writeSyntaxElement_CABAC(SyntaxElement *se, DataPartition *this_dataPart)
{
  int curr_len;
  EncodingEnvironmentPtr eep_dp = &(this_dataPart->ee_cabac);

  curr_len = arienco_bits_written(eep_dp);

  // perform the actual coding by calling the appropriate method
  se->writing(se, eep_dp);

  return (se->len = (arienco_bits_written(eep_dp) - curr_len));
}

/*!
 ***************************************************************************
 * \brief
 *    This function is used to arithmetically encode the macroblock
 *    type info of a given MB.
 ***************************************************************************
 */
void writeMB_typeInfo2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  int l;
  int a, b;
  int act_ctx;
  int act_sym;
  int log_sym;
  int mode_sym=0;
  int mask;
  int mode16x16;

  MotionInfoContexts *ctx = (img->currentSlice)->mot_ctx;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];
  int curr_mb_type = se->value1;

  if(img->type == INTRA_IMG)  // INTRA-frame
  {
    if (currMB->mb_available[0][1] == NULL)
      b = 0;
    else
      b = (( (currMB->mb_available[0][1])->mb_type != 0) ? 1 : 0 );
    if (currMB->mb_available[1][0] == NULL)
      a = 0;
    else
      a = (( (currMB->mb_available[1][0])->mb_type != 0) ? 1 : 0 );

    act_ctx = a + b;
    act_sym = curr_mb_type;
    se->context = act_ctx; // store context

    if (act_sym==0) // 4x4 Intra
      biari_encode_symbol(eep_dp, 0, ctx->mb_type_contexts[0] + act_ctx );
    else // 16x16 Intra
    {
      biari_encode_symbol(eep_dp, 1, ctx->mb_type_contexts[0] + act_ctx );
      mode_sym=act_sym-1; // Values in the range of 0...23
      act_ctx = 4;
      act_sym = mode_sym/12;
      biari_encode_symbol(eep_dp, (unsigned char) act_sym, ctx->mb_type_contexts[0] + act_ctx ); // coding of AC/no AC
      mode_sym = mode_sym % 12;
      act_sym = mode_sym / 4; // coding of cbp: 0,1,2
      act_ctx = 5;
      if (act_sym==0)
      {
        biari_encode_symbol(eep_dp, 0, ctx->mb_type_contexts[0] + act_ctx );
      }
      else
      {
        biari_encode_symbol(eep_dp, 1, ctx->mb_type_contexts[0] + act_ctx );
        act_ctx=6;
        if (act_sym==1)
        {
          biari_encode_symbol(eep_dp, 0, ctx->mb_type_contexts[0] + act_ctx );
        }
        else
        {

          biari_encode_symbol(eep_dp, 1, ctx->mb_type_contexts[0] + act_ctx );
        }

      }
      mode_sym = mode_sym % 4; // coding of I pred-mode: 0,1,2,3
      act_sym = mode_sym/2;
      act_ctx = 7;
      biari_encode_symbol(eep_dp, (unsigned char) act_sym, ctx->mb_type_contexts[0] + act_ctx );
      act_ctx = 8;
      act_sym = mode_sym%2;
      biari_encode_symbol(eep_dp, (unsigned char) act_sym, ctx->mb_type_contexts[0] + act_ctx );
    }
  }
  else
  {

    if (currMB->mb_available[0][1] == NULL)
      b = 0;
    else
      b = (( (currMB->mb_available[0][1])->mb_type != 0) ? 1 : 0 );
    if (currMB->mb_available[1][0] == NULL)
      a = 0;
    else
      a = (( (currMB->mb_available[1][0])->mb_type != 0) ? 1 : 0 );


    act_sym = curr_mb_type;

    if(act_sym>=(mode16x16=(8*img->type+1))) // 16x16 Intra-mode: mode16x16=9 (P-frame) mode16x16=17 (B-frame)
    {
      mode_sym=act_sym-mode16x16;
      act_sym=mode16x16; // 16x16 mode info
    }

    act_sym++;

    for (log_sym = 0; (1<<log_sym) <= act_sym; log_sym++);
    log_sym--;

    act_ctx = a + b;
    se->context = act_ctx; // store context

    if (log_sym==0)
    {
      biari_encode_symbol(eep_dp, 0, &ctx->mb_type_contexts[1][act_ctx] );

    }
    else
    {
      // code unary part
      biari_encode_symbol(eep_dp, 1, &ctx->mb_type_contexts[1][act_ctx] );
      act_ctx=4;
      if (log_sym==1)
      {
        biari_encode_symbol(eep_dp, 0, &ctx->mb_type_contexts[1][act_ctx ] );
      }
      else
      {
        for(l=0;l<log_sym-1;l++)
        {
          biari_encode_symbol(eep_dp, 1, &ctx->mb_type_contexts[1][act_ctx] );
          if (l==0) act_ctx=5;
        }
        if ( log_sym < (3+((img->type == B_IMG)?1:0)) ) // maximum mode no. is 9 (P-frame) or 17 (B-frame)
          biari_encode_symbol(eep_dp, 0, &ctx->mb_type_contexts[1][act_ctx ] );
      }

      // code binary part
      act_ctx=6;
      if (log_sym==(3+((img->type == B_IMG)?1:0)) ) log_sym=2; // only 2 LSBs are actually set for mode 7-9 (P-frame) or 15-17 (B-frame)
      mask = (1<<log_sym); // MSB
      for(l=0;l<log_sym;l++)
      {
        mask >>=1;
        biari_encode_symbol(eep_dp, (unsigned char) (act_sym & mask), &ctx->mb_type_contexts[1][act_ctx] );
      }
    }


    if(act_sym==(mode16x16+1)) // additional info for 16x16 Intra-mode
    {
      act_ctx = 7;
      act_sym = mode_sym/12;
      biari_encode_symbol(eep_dp, (unsigned char) act_sym, ctx->mb_type_contexts[1] + act_ctx ); // coding of AC/no AC
      mode_sym = mode_sym % 12;

      act_sym = mode_sym / 4; // coding of cbp: 0,1,2
      act_ctx = 8;
      if (act_sym==0)
      {
        biari_encode_symbol(eep_dp, 0, ctx->mb_type_contexts[1] + act_ctx );
      }
      else
      {
        biari_encode_symbol(eep_dp, 1, ctx->mb_type_contexts[1] + act_ctx );
        if (act_sym==1)
        {
          biari_encode_symbol(eep_dp, 0, ctx->mb_type_contexts[1] + act_ctx );
        }
        else
        {

          biari_encode_symbol(eep_dp, 1, ctx->mb_type_contexts[1] + act_ctx );
        }

      }

      mode_sym = mode_sym % 4; // coding of I pred-mode: 0,1,2,3
      act_ctx = 9;

      act_sym = mode_sym/2;
      biari_encode_symbol(eep_dp, (unsigned char) act_sym, ctx->mb_type_contexts[1] + act_ctx );
      act_sym = mode_sym%2;
      biari_encode_symbol(eep_dp, (unsigned char) act_sym, ctx->mb_type_contexts[1] + act_ctx );

    }
  }
}

/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode a pair of
 *    intra prediction modes of a given MB.
 ****************************************************************************
 */

void writeIntraPredMode2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  static int prev_sym = 0;
  static int count = 0; // to detect a new row of intra prediction modes

  TextureInfoContexts *ctx = img->currentSlice->tex_ctx;

  if (count % 2 == 0)
    prev_sym = 0;

  unary_bin_max_encode(eep_dp,(unsigned int) se->value1,ctx->ipr_contexts[prev_sym],1,5);
  prev_sym = se->value1;
  unary_bin_max_encode(eep_dp,(unsigned int) se->value2,ctx->ipr_contexts[prev_sym],1,5);
  prev_sym = se->value2;


  if(++count == MB_BLOCK_SIZE/2) // all modes of one MB have been processed
    count=0;
}

/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode the reference
 *    parameter of a given MB.
 ****************************************************************************
 */
void writeRefFrame2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  MotionInfoContexts *ctx = img->currentSlice->mot_ctx;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  int a, b;
  int act_ctx;
  int act_sym;

  if (currMB->mb_available[0][1] == NULL)
    b = 0;
  else
    b = ( ((currMB->mb_available[0][1])->ref_frame != 0) ? 1 : 0);
  if (currMB->mb_available[1][0] == NULL)
    a = 0;
  else
    a = ( ((currMB->mb_available[1][0])->ref_frame != 0) ? 1 : 0);

  act_ctx = a + 2*b;
  se->context = act_ctx; // store context
  act_sym = se->value1;

  if (act_sym==0)
  {
    biari_encode_symbol(eep_dp, 0, ctx->ref_no_contexts + act_ctx );
  }
  else
  {
    biari_encode_symbol(eep_dp, 1, ctx->ref_no_contexts + act_ctx);
    act_sym--;
    act_ctx=4;
    unary_bin_encode(eep_dp, act_sym,ctx->ref_no_contexts+act_ctx,1);
  }
}

/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode the motion
 *    vector data of a given MB.
 ****************************************************************************
 */
void writeMVD2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  int step_h, step_v;
  int i = img->subblock_x;
  int j = img->subblock_y;
  int a, b;
  int act_ctx;
  int act_sym;
  int mv_pred_res;
  int mv_local_err;
  int mv_sign;
  int k = se->value2; // MVD component

  MotionInfoContexts *ctx = img->currentSlice->mot_ctx;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];
  int curr_mb_type = currMB->mb_type;


  step_h=input->blc_size[curr_mb_type][0]/BLOCK_SIZE;
  step_v=input->blc_size[curr_mb_type][1]/BLOCK_SIZE;

  if (j==0)
  {
    if (currMB->mb_available[0][1] == NULL)
      b = 0;
    else
      b = absm((currMB->mb_available[0][1])->mvd[0][BLOCK_SIZE-1][i][k]);
  }
  else
    b = absm(currMB->mvd[0][j-step_v][i][k]);

  if (i==0)
  {
    if (currMB->mb_available[1][0] == NULL)
      a = 0;
    else
      a = absm((currMB->mb_available[1][0])->mvd[0][j][BLOCK_SIZE-1][k]);
  }
  else
    a = absm(currMB->mvd[0][j][i-step_h][k]);

  if ((mv_local_err=a+b)<3)
    act_ctx = 5*k;
  else
  {
    if (mv_local_err>32)
      act_ctx=5*k+3;
    else
      act_ctx=5*k+2;
  }
  mv_pred_res = se->value1;
  se->context = act_ctx;

  act_sym = absm(mv_pred_res);

  if (act_sym == 0)
    biari_encode_symbol(eep_dp, 0, &ctx->mv_res_contexts[0][act_ctx] );
  else
  {
    biari_encode_symbol(eep_dp, 1, &ctx->mv_res_contexts[0][act_ctx] );
    mv_sign = (mv_pred_res<0) ? 1: 0;
    act_ctx=5*k+4;
    biari_encode_symbol(eep_dp, (unsigned char) mv_sign, &ctx->mv_res_contexts[1][act_ctx] );
    act_sym--;
    act_ctx=5*k;
    unary_mv_encode(eep_dp,act_sym,ctx->mv_res_contexts[1]+act_ctx,3);
  }
}

/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode the coded
 *    block pattern of a given delta quant.
 ****************************************************************************
 */
void writeDquant_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  MotionInfoContexts *ctx = img->currentSlice->mot_ctx;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  int act_ctx;
  int act_sym;
  int dquant = se->value1;
  int sign=0;

  if (dquant <= 0)
    sign = 1;
  act_sym = abs(dquant) << 1;

  act_sym += sign;
  act_sym --;

  if (currMB->mb_available[1][0] == NULL)
    act_ctx = 0;
  else
    act_ctx = ( ((currMB->mb_available[1][0])->delta_qp != 0) ? 1 : 0);

  if (act_sym==0)
  {
    biari_encode_symbol(eep_dp, 0, ctx->delta_qp_contexts + act_ctx );
  }
  else
  {
    biari_encode_symbol(eep_dp, 1, ctx->delta_qp_contexts + act_ctx);
    act_ctx=2;
    act_sym--;
    unary_bin_encode(eep_dp, act_sym,ctx->delta_qp_contexts+act_ctx,1);
  }
}

/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode the motion
 *    vector data of a B-frame MB.
 ****************************************************************************
 */
void writeBiMVD2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  int step_h, step_v;
  int i = img->subblock_x;
  int j = img->subblock_y;
  int a, b;
  int act_ctx;
  int act_sym;
  int mv_pred_res;
  int mv_local_err;
  int mv_sign;
  int backward = se->value2 & 0x01;
  int k = (se->value2>>1); // MVD component

  MotionInfoContexts *ctx = img->currentSlice->mot_ctx;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  if(backward == 0)
  {
    step_h=img->fw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize
    step_v=img->fw_blc_size_v/BLOCK_SIZE;  // vertical stepsize
  }
  else
  {
    step_h=img->bw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize
    step_v=img->bw_blc_size_v/BLOCK_SIZE;  // vertical stepsize
  }

  if (j==0)
  {
    if (currMB->mb_available[0][1] == NULL)
      b = 0;
    else
      b = absm((currMB->mb_available[0][1])->mvd[backward][BLOCK_SIZE-1][i][k]);
  }
  else
    b = absm(currMB->mvd[backward][j-step_v][i][k]);

  if (i==0)
  {
    if (currMB->mb_available[1][0] == NULL)
      a = 0;
    else
      a = absm((currMB->mb_available[1][0])->mvd[backward][j][BLOCK_SIZE-1][k]);
  }
  else
    a = absm(currMB->mvd[backward][j][i-step_h][k]);

  if ((mv_local_err=a+b)<3)
    act_ctx = 5*k;
  else
  {
    if (mv_local_err>32)
      act_ctx=5*k+3;
    else
      act_ctx=5*k+2;
  }
  mv_pred_res = se->value1;
  se->context = act_ctx;

  act_sym = absm(mv_pred_res);

  if (act_sym == 0)
    biari_encode_symbol(eep_dp, 0, &ctx->mv_res_contexts[0][act_ctx] );
  else
  {
    biari_encode_symbol(eep_dp, 1, &ctx->mv_res_contexts[0][act_ctx] );
    mv_sign = (mv_pred_res<0) ? 1: 0;
    act_ctx=5*k+4;
    biari_encode_symbol(eep_dp, (unsigned char) mv_sign, &ctx->mv_res_contexts[1][act_ctx] );
    act_sym--;
    act_ctx=5*k;
    unary_mv_encode(eep_dp,act_sym,ctx->mv_res_contexts[1]+act_ctx,3);
  }
}


/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode the forward
 *    or backward bidirectional blocksize (for B frames only)
 ****************************************************************************
 */
void writeBiDirBlkSize2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  int act_ctx;
  int act_sym;

  MotionInfoContexts *ctx = img->currentSlice->mot_ctx;

  act_sym = se->value1; // maximum is 6
  act_ctx=4;

  // using the context models of mb_type
  unary_bin_max_encode(eep_dp,(unsigned int) act_sym,ctx->mb_type_contexts[1]+act_ctx,1,6);
}


/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode the coded
 *    block pattern of a given MB.
 ****************************************************************************
 */
void writeCBP2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  TextureInfoContexts *ctx = img->currentSlice->tex_ctx;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  int mb_x, mb_y;
  int a, b;
  int curr_cbp_ctx, curr_cbp_idx;
  int cbp = se->value1; // symbol to encode
  int mask;
  int cbp_bit;

  if ( se->type == SE_CBP_INTRA )
    curr_cbp_idx = 0;
  else
    curr_cbp_idx = 1;

  //  coding of luma part (bit by bit)
  for (mb_y=0; mb_y < 4; mb_y += 2)
  {
    for (mb_x=0; mb_x < 4; mb_x += 2)
    {

      if (mb_y == 0)
      {
        if (currMB->mb_available[0][1] == NULL)
          b = 0;
        else
          b = (( ((currMB->mb_available[0][1])->cbp & (1<<(2+mb_x/2))) == 0) ? 1 : 0);
      }
      else
        b = ( ((cbp & (1<<(mb_x/2))) == 0) ? 1: 0);

      if (mb_x == 0)
      {
        if (currMB->mb_available[1][0] == NULL)
          a = 0;
        else
          a = (( ((currMB->mb_available[1][0])->cbp & (1<<(mb_y+1))) == 0) ? 1 : 0);
      }
      else
        a = ( ((cbp & (1<<mb_y)) == 0) ? 1: 0);

      curr_cbp_ctx = a+2*b;
      mask = (1<<(mb_y+mb_x/2));
      cbp_bit = cbp & mask;
      biari_encode_symbol(eep_dp, (unsigned char) cbp_bit, ctx->cbp_contexts[curr_cbp_idx][0] + curr_cbp_ctx );
    }
  }

  // coding of chroma part
  b = 0;
  if (currMB->mb_available[0][1] != NULL)
    b = ((currMB->mb_available[0][1])->cbp > 15) ? 1 : 0;

  a = 0;
  if (currMB->mb_available[1][0] != NULL)
    a = ((currMB->mb_available[1][0])->cbp > 15) ? 1 : 0;

  curr_cbp_ctx = a+2*b;
  cbp_bit = (cbp > 15 ) ? 1 : 0;
  biari_encode_symbol(eep_dp, (unsigned char) cbp_bit, ctx->cbp_contexts[curr_cbp_idx][1] + curr_cbp_ctx );

  if (cbp > 15)
  {
    b = 0;
    if (currMB->mb_available[0][1] != NULL)
      if ((currMB->mb_available[0][1])->cbp > 15)
        b = (( ((currMB->mb_available[0][1])->cbp >> 4) == 2) ? 1 : 0);

    a = 0;
    if (currMB->mb_available[1][0] != NULL)
      if ((currMB->mb_available[1][0])->cbp > 15)
        a = (( ((currMB->mb_available[1][0])->cbp >> 4) == 2) ? 1 : 0);

    curr_cbp_ctx = a+2*b;
    cbp_bit = ((cbp>>4) == 2) ? 1 : 0;
    biari_encode_symbol(eep_dp, (unsigned char) cbp_bit, ctx->cbp_contexts[curr_cbp_idx][2] + curr_cbp_ctx );
  }
}


/*!
 ****************************************************************************
 * \brief
 *    This function is used to arithmetically encode level and
 *    run of a given MB.
 ****************************************************************************
 */
void writeRunLevel2Buffer_CABAC(SyntaxElement *se, EncodingEnvironmentPtr eep_dp)
{
  const int level = se->value1;
  const int run = se->value2;
  const int curr_ctx_idx = se->context;
  int curr_level_ctx;
  int sign_of_level;
  int max_run;

  TextureInfoContexts *ctx = img->currentSlice->tex_ctx;

  unary_level_encode(eep_dp,(unsigned int) absm(level),ctx->level_context[curr_ctx_idx]);

  if (level!=0)
  {
    sign_of_level = ((level < 0) ? 1 : 0);
    curr_level_ctx = 3;
    biari_encode_symbol(eep_dp, (unsigned char) sign_of_level, ctx->level_context[curr_ctx_idx] + curr_level_ctx );
    if (curr_ctx_idx != 0 && curr_ctx_idx != 6 && curr_ctx_idx != 5) // not double scan and not DC-chroma
        unary_bin_encode(eep_dp,(unsigned int) run,ctx->run_context[curr_ctx_idx],1);
    else
    {
      max_run =  (curr_ctx_idx == 0) ? 7 : 3;  // if double scan max_run = 7; if DC-chroma max_run = 3;
      unary_bin_max_encode(eep_dp,(unsigned int) run,ctx->run_context[curr_ctx_idx],1,max_run);
    }
  }

}
/*!
 ************************************************************************
 * \brief
 *    Unary binarization and encoding of a symbol by using
 *    one or two distinct models for the first two and all
 *    remaining bins
*
************************************************************************/
void unary_bin_encode(EncodingEnvironmentPtr eep_dp,
                      unsigned int symbol,
                      BiContextTypePtr ctx,
                      int ctx_offset)
{
  unsigned int l;
  BiContextTypePtr ictx;

  if (symbol==0)
  {
    biari_encode_symbol(eep_dp, 0, ctx );
    return;
  }
  else
  {
    biari_encode_symbol(eep_dp, 1, ctx );
    l=symbol;
    ictx=ctx+ctx_offset;
    while ((--l)>0)
      biari_encode_symbol(eep_dp, 1, ictx);
    biari_encode_symbol(eep_dp, 0, ictx);
  }
  return;
}

/*!
 ************************************************************************
 * \brief
 *    Unary binarization and encoding of a symbol by using
 *    one or two distinct models for the first two and all
 *    remaining bins; no terminating "0" for max_symbol
 *    (finite symbol alphabet)
 ************************************************************************
 */
void unary_bin_max_encode(EncodingEnvironmentPtr eep_dp,
                          unsigned int symbol,
                          BiContextTypePtr ctx,
                          int ctx_offset,
                          unsigned int max_symbol)
{
  unsigned int l;
  BiContextTypePtr ictx;

  if (symbol==0)
  {
    biari_encode_symbol(eep_dp, 0, ctx );
    return;
  }
  else
  {
    biari_encode_symbol(eep_dp, 1, ctx );
    l=symbol;
    ictx=ctx+ctx_offset;
    while ((--l)>0)
      biari_encode_symbol(eep_dp, 1, ictx);
    if (symbol<max_symbol)
        biari_encode_symbol(eep_dp, 0, ictx);
  }
  return;
}

/*!
 ************************************************************************
 * \brief
 *    Unary binarization and encoding of a symbol by using
 *    three distinct models for the first, the second and all
 *    remaining bins
 ************************************************************************
 */
void unary_level_encode(EncodingEnvironmentPtr eep_dp,
                        unsigned int symbol,
                        BiContextTypePtr ctx)
{
  unsigned int l;
  int bin=1;
  BiContextTypePtr ictx=ctx;

  if (symbol==0)
  {
    biari_encode_symbol(eep_dp, 0, ictx );
    return;
  }
  else
  {
    biari_encode_symbol(eep_dp, 1, ictx );
    l=symbol;
    ictx++;
    while ((--l)>0)
    {
      biari_encode_symbol(eep_dp, 1, ictx  );
      if ((++bin)==2) ictx++;
    }
    biari_encode_symbol(eep_dp, 0, ictx  );
  }
  return;
}

/*!
 ************************************************************************
 * \brief
 *    Unary binarization and encoding of a symbol by using
 *    four distinct models for the first, the second, intermediate
 *    and all remaining bins
 ************************************************************************
 */
void unary_mv_encode(EncodingEnvironmentPtr eep_dp,
                        unsigned int symbol,
                        BiContextTypePtr ctx,
                        unsigned int max_bin)
{
  unsigned int l;
  unsigned int bin=1;
  BiContextTypePtr ictx=ctx;

  if (symbol==0)
  {
    biari_encode_symbol(eep_dp, 0, ictx );
    return;
  }
  else
  {
    biari_encode_symbol(eep_dp, 1, ictx );
    l=symbol;
    ictx++;
    while ((--l)>0)
    {
      biari_encode_symbol(eep_dp, 1, ictx  );
      if ((++bin)==2) ictx++;
      if (bin==max_bin) ictx++;
    }
    biari_encode_symbol(eep_dp, 0, ictx  );
  }
  return;
}

