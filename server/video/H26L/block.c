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
 * \file block.c
 *
 * \brief
 *    Process one block
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *    - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *    - Jani Lainema                    <jani.lainema@nokia.com>
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *    - Ragip Kurceren                  <ragip.kurceren@nokia.com>
 *************************************************************************************
 */

#include "contributors.h"


#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "block.h"
#include "refbuf.h"


/*!
 ************************************************************************
 * \brief
 *    Make intra 4x4 prediction according to all 6 prediction modes.
 *    The routine uses left and upper neighbouring points from
 *    previous coded blocks to do this (if available). Notice that
 *    inaccessible neighbouring points are signalled with a negative
 *    value i the predmode array .
 *
 *  \para Input:
 *     Starting point of current 4x4 block image posision
 *
 *  \para Output:
 *      none
 ************************************************************************
 */
void intrapred_luma(int img_x,int img_y)
{
  int i,j,s0=0,s1,s2,ia[7][3],s[4][2];

  int block_available_up = (img->ipredmode[img_x/BLOCK_SIZE+1][img_y/BLOCK_SIZE] >=0);
  int block_available_left = (img->ipredmode[img_x/BLOCK_SIZE][img_y/BLOCK_SIZE+1] >=0);

  s1=0;
  s2=0;

  // make DC prediction
  for (i=0; i < BLOCK_SIZE; i++)
  {
    if (block_available_up)
      s1 += imgY[img_y-1][img_x+i];    // sum hor pix
    if (block_available_left)
      s2 += imgY[img_y+i][img_x-1];    // sum vert pix
  }
  if (block_available_up && block_available_left)
    s0=(s1+s2+4)/(2*BLOCK_SIZE);      // no edge
  if (!block_available_up && block_available_left)
    s0=(s2+2)/BLOCK_SIZE;             // upper edge
  if (block_available_up && !block_available_left)
    s0=(s1+2)/BLOCK_SIZE;             // left edge
  if (!block_available_up && !block_available_left)
    s0=128;                           // top left corner, nothing to predict from

  for (i=0; i < BLOCK_SIZE; i++)
  {
    // vertical prediction
    if (block_available_up)
      s[i][0]=imgY[img_y-1][img_x+i];
    // horizontal prediction
    if (block_available_left)
      s[i][1]=imgY[img_y+i][img_x-1];
  }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      img->mprr[DC_PRED][i][j]=s0;      // store DC prediction
      img->mprr[VERT_PRED][i][j]=s[j][0]; // store vertical prediction
      img->mprr[HOR_PRED][i][j]=s[i][1]; // store horizontal prediction
    }
  }

  //  Prediction according to 'diagonal' modes
  if (block_available_up && block_available_left)
  {
    int A = imgY[img_y-1][img_x];
    int B = imgY[img_y-1][img_x+1];
    int C = imgY[img_y-1][img_x+2];
    int D = imgY[img_y-1][img_x+3];
    int E = imgY[img_y  ][img_x-1];
    int F = imgY[img_y+1][img_x-1];
    int G = imgY[img_y+2][img_x-1];
    int H = imgY[img_y+3][img_x-1];
    int I = imgY[img_y-1][img_x-1];
    ia[0][0]=(H+2*G+F+2)/4;
    ia[1][0]=(G+2*F+E+2)/4;
    ia[2][0]=(F+2*E+I+2)/4;
    ia[3][0]=(E+2*I+A+2)/4;
    ia[4][0]=(I+2*A+B+2)/4;
    ia[5][0]=(A+2*B+C+2)/4;
    ia[6][0]=(B+2*C+D+2)/4;
    for (i=0;i<4;i++)
      for (j=0;j<4;j++)
        img->mprr[DIAG_PRED_LR_45][i][j]=ia[j-i+3][0];
  }
  if (block_available_up)
  { // Do prediction 1
    int A = imgY[img_y-1][img_x+0];
    int B = imgY[img_y-1][img_x+1];
    int C = imgY[img_y-1][img_x+2];
    int D = imgY[img_y-1][img_x+3];

    img->mprr[DIAG_PRED_RL][0][0] = (A+B)/2; // a
    img->mprr[DIAG_PRED_RL][1][0] = B;       // e
    img->mprr[DIAG_PRED_RL][0][1] = img->mprr[DIAG_PRED_RL][2][0] = (B+C)/2; // b i
    img->mprr[DIAG_PRED_RL][1][1] = img->mprr[DIAG_PRED_RL][3][0] = C;       // f m
    img->mprr[DIAG_PRED_RL][0][2] = img->mprr[DIAG_PRED_RL][2][1] = (C+D)/2; // c j
    img->mprr[DIAG_PRED_RL][3][1] =
        img->mprr[DIAG_PRED_RL][1][2] =
            img->mprr[DIAG_PRED_RL][2][2] =
                img->mprr[DIAG_PRED_RL][3][2] =
                    img->mprr[DIAG_PRED_RL][0][3] =
                        img->mprr[DIAG_PRED_RL][1][3] =
                            img->mprr[DIAG_PRED_RL][2][3] =
                                img->mprr[DIAG_PRED_RL][3][3] = D; // d g h k l n o p
  }

  if (block_available_left)
  { // Do prediction 5
    int E = imgY[img_y+0][img_x-1];
    int F = imgY[img_y+1][img_x-1];
    int G = imgY[img_y+2][img_x-1];
    int H = imgY[img_y+3][img_x-1];

    img->mprr[DIAG_PRED_LR][0][0] = (E+F)/2; // a
    img->mprr[DIAG_PRED_LR][0][1] = F;       // b
    img->mprr[DIAG_PRED_LR][1][0] = img->mprr[DIAG_PRED_LR][0][2] = (F+G)/2; // e c
    img->mprr[DIAG_PRED_LR][1][1] = img->mprr[DIAG_PRED_LR][0][3] = G;       // f d
    img->mprr[DIAG_PRED_LR][2][0] = img->mprr[DIAG_PRED_LR][1][2] = (G+H)/2; // i g

    img->mprr[DIAG_PRED_LR][1][3] =
        img->mprr[DIAG_PRED_LR][2][1] =
            img->mprr[DIAG_PRED_LR][2][2] =
                img->mprr[DIAG_PRED_LR][2][3] =
                    img->mprr[DIAG_PRED_LR][3][0] =
                        img->mprr[DIAG_PRED_LR][3][1] =
                            img->mprr[DIAG_PRED_LR][3][2] =
                                img->mprr[DIAG_PRED_LR][3][3] = H;
  }
}

/*!
 ************************************************************************
 * \brief
 *    16x16 based luma prediction
 *
 * \para Input:
 *    Image parameters
 *
 * \para Output:
 *    none
 ************************************************************************
 */
void intrapred_luma_2()
{
  int s0=0,s1,s2;
  int i,j;
  int s[16][2];

  int ih,iv;
  int ib,ic,iaa;

  int mb_nr = img->current_mb_nr;
  int mb_width = img->width/16;
  int mb_available_up = (img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width]);
  int mb_available_left = (img->mb_x == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-1]);

  if(input->UseConstrainedIntraPred)
  {
    if (mb_available_up && (img->intra_mb[mb_nr-mb_width] ==0))
      mb_available_up = 0;
    if (mb_available_left && (img->intra_mb[mb_nr-1] ==0))
      mb_available_left = 0;
  }

  s1=s2=0;
  // make DC prediction
  for (i=0; i < MB_BLOCK_SIZE; i++)
  {
    if (mb_available_up)
      s1 += imgY[img->pix_y-1][img->pix_x+i];    // sum hor pix
    if (mb_available_left)
      s2 += imgY[img->pix_y+i][img->pix_x-1];    // sum vert pix
  }
  if (mb_available_up && mb_available_left)
    s0=(s1+s2+16)/(2*MB_BLOCK_SIZE);             // no edge
  if (!mb_available_up && mb_available_left)
    s0=(s2+8)/MB_BLOCK_SIZE;                     // upper edge
  if (mb_available_up && !mb_available_left)
    s0=(s1+8)/MB_BLOCK_SIZE;                     // left edge
  if (!mb_available_up && !mb_available_left)
    s0=128;                                      // top left corner, nothing to predict from

  for (i=0; i < MB_BLOCK_SIZE; i++)
  {
    // vertical prediction
    if (mb_available_up)
      s[i][0]=imgY[img->pix_y-1][img->pix_x+i];
    // horizontal prediction
    if (mb_available_left)
      s[i][1]=imgY[img->pix_y+i][img->pix_x-1];
  }

  for (j=0; j < MB_BLOCK_SIZE; j++)
  {
    for (i=0; i < MB_BLOCK_SIZE; i++)
    {
      img->mprr_2[VERT_PRED_16][j][i]=s[i][0]; // store vertical prediction
      img->mprr_2[HOR_PRED_16 ][j][i]=s[j][1]; // store horizontal prediction
      img->mprr_2[DC_PRED_16  ][j][i]=s0;      // store DC prediction
    }
  }
  if (!mb_available_up || !mb_available_left) // edge
    return;

  // 16 bit integer plan pred

  ih=0;
  iv=0;
  for (i=1;i<9;i++)
  {
    ih += i*(imgY[img->pix_y-1][img->pix_x+7+i] - imgY[img->pix_y-1][img->pix_x+7-i]);
    iv += i*(imgY[img->pix_y+7+i][img->pix_x-1] - imgY[img->pix_y+7-i][img->pix_x-1]);
  }
  ib=5*(ih/4)/16;
  ic=5*(iv/4)/16;

  iaa=16*(imgY[img->pix_y-1][img->pix_x+15]+imgY[img->pix_y+15][img->pix_x-1]);
  for (j=0;j< MB_BLOCK_SIZE;j++)
  {
    for (i=0;i< MB_BLOCK_SIZE;i++)
    {
      img->mprr_2[PLANE_16][j][i]=max(0,min(255,(iaa+(i-7)*ib +(j-7)*ic + 16)/32));/* store plane prediction */                              
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    For new intra pred routines
 *
 * \para Input:
 *    Image par, 16x16 based intra mode
 *
 * \para Output:
 *    none
 ************************************************************************
 */
void dct_luma2(int new_intra_mode)
{
#ifndef NO_RDQUANT
  int jq0;
#endif
#ifdef NO_RDQUANT
  int qp_const;
#endif
  int i,j;
  int ii,jj;
  int i1,j1;

  int M1[16][16];
  int M4[4][4];
  int M5[4],M6[4];
  int M0[4][4][4][4];
#ifndef NO_RDQUANT
  int coeff[16];
#endif
  int quant_set,run,scan_pos,coeff_ctr,level;

#ifndef NO_RDQUANT
  jq0=JQQ3;
#endif
#ifdef NO_RDQUANT
  qp_const = JQQ3;
#endif

  for (j=0;j<16;j++)
  {
    for (i=0;i<16;i++)
    {
      M1[i][j]=imgY_org[img->pix_y+j][img->pix_x+i]-img->mprr_2[new_intra_mode][j][i];
      M0[i%4][i/4][j%4][j/4]=M1[i][j];
    }
  }

  for (jj=0;jj<4;jj++)
  {
    for (ii=0;ii<4;ii++)
    {
      for (j=0;j<4;j++)
      {
        for (i=0;i<2;i++)
        {
          i1=3-i;
          M5[i]=  M0[i][ii][j][jj]+M0[i1][ii][j][jj];
          M5[i1]= M0[i][ii][j][jj]-M0[i1][ii][j][jj];
        }
        M0[0][ii][j][jj]=(M5[0]+M5[1])*13;
        M0[2][ii][j][jj]=(M5[0]-M5[1])*13;
        M0[1][ii][j][jj]=M5[3]*17+M5[2]*7;
        M0[3][ii][j][jj]=M5[3]*7-M5[2]*17;
      }
      // vertical
      for (i=0;i<4;i++)
      {
        for (j=0;j<2;j++)
        {
          j1=3-j;
          M5[j] = M0[i][ii][j][jj]+M0[i][ii][j1][jj];
          M5[j1]= M0[i][ii][j][jj]-M0[i][ii][j1][jj];
        }
        M0[i][ii][0][jj]=(M5[0]+M5[1])*13;
        M0[i][ii][2][jj]=(M5[0]-M5[1])*13;
        M0[i][ii][1][jj]= M5[3]*17+M5[2]*7;
        M0[i][ii][3][jj]= M5[3]*7 -M5[2]*17;
      }
    }
  }

  // pick out DC coeff

  for (j=0;j<4;j++)
    for (i=0;i<4;i++)
      M4[i][j]= 49 * M0[0][i][0][j]/32768;

  for (j=0;j<4;j++)
  {
    for (i=0;i<2;i++)
    {
      i1=3-i;
      M5[i]= M4[i][j]+M4[i1][j];
      M5[i1]=M4[i][j]-M4[i1][j];
    }
    M4[0][j]=(M5[0]+M5[1])*13;
    M4[2][j]=(M5[0]-M5[1])*13;
    M4[1][j]= M5[3]*17+M5[2]*7;
    M4[3][j]= M5[3]*7 -M5[2]*17;
  }

  // vertical

  for (i=0;i<4;i++)
  {
    for (j=0;j<2;j++)
    {
      j1=3-j;
      M5[j]= M4[i][j]+M4[i][j1];
      M5[j1]=M4[i][j]-M4[i][j1];
    }
    M4[i][0]=(M5[0]+M5[1])*13;
    M4[i][2]=(M5[0]-M5[1])*13;
    M4[i][1]= M5[3]*17+M5[2]*7;
    M4[i][3]= M5[3]*7 -M5[2]*17;
  }

  // quant

  quant_set=img->qp;
  run=-1;
  scan_pos=0;
#ifndef NO_RDQUANT

  for (coeff_ctr=0;coeff_ctr<16;coeff_ctr++)
  {
    i=SNGL_SCAN[coeff_ctr][0];
    j=SNGL_SCAN[coeff_ctr][1];
    coeff[coeff_ctr]=M4[i][j];
  }
  rd_quant(QUANT_LUMA_SNG,coeff);

  for (coeff_ctr=0;coeff_ctr<16;coeff_ctr++)
  {
    i=SNGL_SCAN[coeff_ctr][0];
    j=SNGL_SCAN[coeff_ctr][1];

    run++;

    level=abs(coeff[coeff_ctr]);

    if (level != 0)
    {
      img->cof[0][0][scan_pos][0][1]=sign(level,M4[i][j]);
      img->cof[0][0][scan_pos][1][1]=run;
      ++scan_pos;
      run=-1;
    }
#endif
#ifdef NO_RDQUANT

  for (coeff_ctr=0;coeff_ctr<16;coeff_ctr++)
  {
    i=SNGL_SCAN[coeff_ctr][0];
    j=SNGL_SCAN[coeff_ctr][1];

    run++;

    level= (abs(M4[i][j]) * JQ[quant_set][0]+qp_const)/JQQ1;

    if (level != 0)
    {
      img->cof[0][0][scan_pos][0][1]=sign(level,M4[i][j]);
      img->cof[0][0][scan_pos][1][1]=run;
      ++scan_pos;
      run=-1;
    }
#endif
    M4[i][j]=sign(level,M4[i][j]);
  }
  img->cof[0][0][scan_pos][0][1]=0;

  // invers DC transform

  for (j=0;j<4;j++)
  {
    for (i=0;i<4;i++)
      M5[i]=M4[i][j];

    M6[0]=(M5[0]+M5[2])*13;
    M6[1]=(M5[0]-M5[2])*13;
    M6[2]= M5[1]*7 -M5[3]*17;
    M6[3]= M5[1]*17+M5[3]*7;

    for (i=0;i<2;i++)
    {
      i1=3-i;
      M4[i][j]= M6[i]+M6[i1];
      M4[i1][j]=M6[i]-M6[i1];
    }
  }

  for (i=0;i<4;i++)
  {
    for (j=0;j<4;j++)
      M5[j]=M4[i][j];

    M6[0]=(M5[0]+M5[2])*13;
    M6[1]=(M5[0]-M5[2])*13;
    M6[2]= M5[1]*7 -M5[3]*17;
    M6[3]= M5[1]*17+M5[3]*7;

    for (j=0;j<2;j++)
    {
      j1=3-j;
      M0[0][i][0][j] = ((M6[j]+M6[j1])/8) *JQ[quant_set][1];
      M0[0][i][0][j1]= ((M6[j]-M6[j1])/8) *JQ[quant_set][1];
    }
  }
  for (j=0;j<4;j++)
  {
    for (i=0;i<4;i++)
    {
      M0[0][i][0][j] = 3 * M0[0][i][0][j]/256;
    }
  }

  // AC invers trans/quant for MB
  img->kac=0;
  for (jj=0;jj<4;jj++)
  {
    for (ii=0;ii<4;ii++)
    {
      run=-1;
      scan_pos=0;
#ifndef NO_RDQUANT
      for (coeff_ctr=1;coeff_ctr<16;coeff_ctr++) // set in AC coeff
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        coeff[coeff_ctr-1]=M0[i][ii][j][jj];
      }
      rd_quant(QUANT_LUMA_AC,coeff);

      for (coeff_ctr=1;coeff_ctr<16;coeff_ctr++) // set in AC coeff
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        run++;

        level=abs(coeff[coeff_ctr-1]);

        if (level != 0)
        {
          img->kac=1;
          img->cof[ii][jj][scan_pos][0][0]=sign(level,M0[i][ii][j][jj]);
          img->cof[ii][jj][scan_pos][1][0]=run;
          ++scan_pos;
          run=-1;
        }
        M0[i][ii][j][jj]=sign(level*JQ[quant_set][1],M0[i][ii][j][jj]);
      }
      img->cof[ii][jj][scan_pos][0][0]=0;
#endif
#ifdef NO_RDQUANT
      for (coeff_ctr=1;coeff_ctr<16;coeff_ctr++) // set in AC coeff
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        run++;

        level= ( abs( M0[i][ii][j][jj]) * JQ[quant_set][0]+qp_const)/JQQ1;

        if (level != 0)
        {
          img->kac=1;
          img->cof[ii][jj][scan_pos][0][0]=sign(level,M0[i][ii][j][jj]);
          img->cof[ii][jj][scan_pos][1][0]=run;
          ++scan_pos;
          run=-1;
        }
        M0[i][ii][j][jj]=sign(level*JQ[quant_set][1],M0[i][ii][j][jj]);
      }
      img->cof[ii][jj][scan_pos][0][0]=0;
#endif


      // IDCT horizontal

      for (j=0;j<4;j++)
      {
        for (i=0;i<4;i++)
        {
          M5[i]=M0[i][ii][j][jj];
        }

        M6[0]=(M5[0]+M5[2])*13;
        M6[1]=(M5[0]-M5[2])*13;
        M6[2]=M5[1]*7 -M5[3]*17;
        M6[3]=M5[1]*17+M5[3]*7;

        for (i=0;i<2;i++)
        {
          i1=3-i;
          M0[i][ii][j][jj] =M6[i]+M6[i1];
          M0[i1][ii][j][jj]=M6[i]-M6[i1];
        }
      }

      // vert
      for (i=0;i<4;i++)
      {
        for (j=0;j<4;j++)
          M5[j]=M0[i][ii][j][jj];

        M6[0]=(M5[0]+M5[2])*13;
        M6[1]=(M5[0]-M5[2])*13;
        M6[2]=M5[1]*7 -M5[3]*17;
        M6[3]=M5[1]*17+M5[3]*7;

        for (j=0;j<2;j++)
        {
          j1=3-j;
          M0[i][ii][ j][jj]=M6[j]+M6[j1];
          M0[i][ii][j1][jj]=M6[j]-M6[j1];

        }
      }

    }
  }

  for (j=0;j<16;j++)
  {
    for (i=0;i<16;i++)
    {
      M1[i][j]=M0[i%4][i/4][j%4][j/4];
    }
  }

  for (j=0;j<16;j++)
    for (i=0;i<16;i++)
      imgY[img->pix_y+j][img->pix_x+i]=min(255,max(0,(M1[i][j]+img->mprr_2[new_intra_mode][j][i]*JQQ1+JQQ2)/JQQ1));

}


/*!
 ************************************************************************
 * \brief
 *    Intra prediction for chroma.  There is only one prediction mode,
 *    corresponding to 'DC prediction' for luma. However,since 2x2 transform
 *    of DC levels are used,all predictions are made from neighbouring MBs.
 *    Prediction also depends on whether the block is at a frame edge.
 *
 *  \para Input:
 *     Starting point of current chroma macro block image posision
 *
 *  \para Output:
 *     8x8 array with DC intra chroma prediction and diff array
 ************************************************************************
 */
void intrapred_chroma(int img_c_x,int img_c_y,int uv)
{
  int s[2][2],s0,s1,s2,s3;
  int i,j;

  int mb_nr = img->current_mb_nr;
  int mb_width = img->width/16;
  int mb_available_up = (img_c_y/BLOCK_SIZE == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width]);
  int mb_available_left = (img_c_x/BLOCK_SIZE == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-1]);
  if(input->UseConstrainedIntraPred)
  {
    if (mb_available_up && (img->intra_mb[mb_nr-mb_width] ==0))
      mb_available_up = 0;
    if (mb_available_left && (img->intra_mb[mb_nr-1] ==0))
      mb_available_left = 0;
  }
  s0=s1=s2=s3=0;          // reset counters

  for (i=0; i < BLOCK_SIZE; i++)
  {
    if(mb_available_up)
    {
      s0 += imgUV[uv][img_c_y-1][img_c_x+i];
      s1 += imgUV[uv][img_c_y-1][img_c_x+i+BLOCK_SIZE];
    }
    if(mb_available_left)
    {
      s2 += imgUV[uv][img_c_y+i][img_c_x-1];
      s3 += imgUV[uv][img_c_y+i+BLOCK_SIZE][img_c_x-1];
    }
  }

  if(mb_available_up && mb_available_left)
  {
    s[0][0]=(s0+s2+4)/(2*BLOCK_SIZE);
    s[1][0]=(s1+2)/BLOCK_SIZE;
    s[0][1]=(s3+2)/BLOCK_SIZE;
    s[1][1]=(s1+s3+4)/(2*BLOCK_SIZE);
  }
  else
    if(mb_available_up && !mb_available_left)
    {
      s[0][0]=(s0+2)/BLOCK_SIZE;
      s[1][0]=(s1+2)/BLOCK_SIZE;
      s[0][1]=(s0+2)/BLOCK_SIZE;
      s[1][1]=(s1+2)/BLOCK_SIZE;
    }
    else
      if(!mb_available_up && mb_available_left)
      {
        s[0][0]=(s2+2)/BLOCK_SIZE;
        s[1][0]=(s2+2)/BLOCK_SIZE;
        s[0][1]=(s3+2)/BLOCK_SIZE;
        s[1][1]=(s3+2)/BLOCK_SIZE;
      }
      else
        if(!mb_available_up && !mb_available_left)
        {
          s[0][0]=128;
          s[1][0]=128;
          s[0][1]=128;
          s[1][1]=128;
        }
  for (j=0; j < MB_BLOCK_SIZE/2; j++)
  {
    for (i=0; i < MB_BLOCK_SIZE/2; i++)
    {
      img->mpr[i][j]=s[i/BLOCK_SIZE][j/BLOCK_SIZE];
      img->m7[i][j]=imgUV_org[uv][img_c_y+j][img_c_x+i]-img->mpr[i][j];
    }
  }
}


/*!
 ************************************************************************
 * \brief
 *    The routine performs transform,quantization,inverse transform, adds the diff.
 *    to the prediction and writes the result to the decoded luma frame. Includes the
 *    RD constrained quantization also.
 *
 * \para Input:
 *    block_x,block_y: Block position inside a macro block (0,4,8,12).
 *
 * \para Output_
 *    nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels.             \n
 *    coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
 ************************************************************************
 */
int dct_luma(int block_x,int block_y,int *coeff_cost)
{
  int sign(int a,int b);

  int i,j,i1,j1,ilev,m5[4],m6[4],coeff_ctr,scan_loop_ctr;
  int qp_const,pos_x,pos_y,quant_set,level,scan_pos,run;
  int nonzero;
  int idx;

  int scan_mode;
  int loop_rep;
#ifndef NO_RDQUANT
  int coeff[16];
#endif

  if (img->type == INTRA_IMG)
    qp_const=JQQ3;    // intra
  else
    qp_const=JQQ4;    // inter

  pos_x=block_x/BLOCK_SIZE;
  pos_y=block_y/BLOCK_SIZE;

  //  Horizontal transform

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=img->m7[i][j]+img->m7[i1][j];
      m5[i1]=img->m7[i][j]-img->m7[i1][j];
    }
    img->m7[0][j]=(m5[0]+m5[1])*13;
    img->m7[2][j]=(m5[0]-m5[1])*13;
    img->m7[1][j]=m5[3]*17+m5[2]*7;
    img->m7[3][j]=m5[3]*7-m5[2]*17;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=img->m7[i][j]+img->m7[i][j1];
      m5[j1]=img->m7[i][j]-img->m7[i][j1];
    }
    img->m7[i][0]=(m5[0]+m5[1])*13;
    img->m7[i][2]=(m5[0]-m5[1])*13;
    img->m7[i][1]=m5[3]*17+m5[2]*7;
    img->m7[i][3]=m5[3]*7-m5[2]*17;
  }

  // Quant

  quant_set=img->qp;
  nonzero=FALSE;

  if (img->imod == INTRA_MB_OLD && img->qp < 24)
  {
    scan_mode=DOUBLE_SCAN;
    loop_rep=2;
    idx=1;
  }
  else
  {
    scan_mode=SINGLE_SCAN;
    loop_rep=1;
    idx=0;
  }

#ifndef NO_RDQUANT
  for(scan_loop_ctr=0;scan_loop_ctr<loop_rep;scan_loop_ctr++) // 2 times if double scan, 1 normal scan
  {
    for (coeff_ctr=0;coeff_ctr < 16/loop_rep;coeff_ctr++)     // 8 times if double scan, 16 normal scan
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }
      coeff[coeff_ctr]=img->m7[i][j];
    }
    if (scan_mode==DOUBLE_SCAN)
      rd_quant(QUANT_LUMA_DBL,coeff);
    else
      rd_quant(QUANT_LUMA_SNG,coeff);

    run=-1;
    scan_pos=scan_loop_ctr*9;   // for double scan; set first or second scan posision
    for (coeff_ctr=0; coeff_ctr<16/loop_rep; coeff_ctr++)
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }
      run++;
      ilev=0;

      level= absm(coeff[coeff_ctr]);
      if (level != 0)
      {
        nonzero=TRUE;
        if (level > 1)
          *coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
        else
          *coeff_cost += COEFF_COST[run];
        img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=sign(level,img->m7[i][j]);
        img->cof[pos_x][pos_y][scan_pos][1][scan_mode]=run;
        ++scan_pos;
        run=-1;                     // reset zero level counter
        ilev=level*JQ[quant_set][1];
      }
      img->m7[i][j]=sign(ilev,img->m7[i][j]);
    }
    img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=0;  // end of block
  }
#endif

#ifdef NO_RDQUANT
  for(scan_loop_ctr=0;scan_loop_ctr<loop_rep;scan_loop_ctr++) // 2 times if double scan, 1 normal scan
  {
  run=-1;
  scan_pos=scan_loop_ctr*9;

    for (coeff_ctr=0;coeff_ctr < 16/loop_rep;coeff_ctr++)     // 8 times if double scan, 16 normal scan
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }

      run++;
      ilev=0;
      level = (abs (img->m7[i][j]) * JQ[quant_set][0] +qp_const) / JQQ1;

      if (level != 0)
      {
        nonzero=TRUE;
        if (level > 1)
          *coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
        else
          *coeff_cost += COEFF_COST[run];
        img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=sign(level,img->m7[i][j]);
        img->cof[pos_x][pos_y][scan_pos][1][scan_mode]=run;
        ++scan_pos;
        run=-1;                     // reset zero level counter
        ilev=level*JQ[quant_set][1];
      }
      img->m7[i][j]=sign(ilev,img->m7[i][j]);
    }
    img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=0;  // end of block
  }
#endif




  //     IDCT.
  //     horizontal

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (i=0; i < 2; i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }

  //  vertical

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < BLOCK_SIZE; j++)
    {
      m5[j]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (j=0; j < 2; j++)
    {
      j1=3-j;
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+img->mpr[i+block_x][j+block_y] *JQQ1+JQQ2)/JQQ1));
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+img->mpr[i+block_x][j1+block_y] *JQQ1+JQQ2)/JQQ1));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];


  return nonzero;
}

/*!
 ************************************************************************
 * \brief
 *    Transform,quantization,inverse transform for chroma.
 *    The main reason why this is done in a separate routine is the
 *    additional 2x2 transform of DC-coeffs. This routine is called
 *    ones for each of the chroma components.
 *
 * \para Input:
 *    uv    : Make difference between the U and V chroma component  \n
 *    cr_cbp: chroma coded block pattern
 *
 * \para Output:
 *    cr_cbp: Updated chroma coded block pattern.
 ************************************************************************
 */
#ifndef NO_RDQUANT

int dct_chroma(int uv,int cr_cbp)
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,coeff_ctr,qp_const,pos_x,pos_y,quant_set,level ,scan_pos,run;
  int m1[BLOCK_SIZE],m5[BLOCK_SIZE],m6[BLOCK_SIZE];
  int coeff[16];

  if (img->type == INTRA_IMG)
    qp_const=JQQ3;
  else
    qp_const=JQQ4;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {

      //  Horizontal transform.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++)
        {
          i1=3-i;
          m5[i]=img->m7[i+n1][mb_y]+img->m7[i1+n1][mb_y];
          m5[i1]=img->m7[i+n1][mb_y]-img->m7[i1+n1][mb_y];
        }
        img->m7[n1][mb_y]=(m5[0]+m5[1])*13;
        img->m7[n1+2][mb_y]=(m5[0]-m5[1])*13;
        img->m7[n1+1][mb_y]=m5[3]*17+m5[2]*7;
        img->m7[n1+3][mb_y]=m5[3]*7-m5[2]*17;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=img->m7[j1][n2+j]+img->m7[j1][n2+j2];
          m5[j2]=img->m7[j1][n2+j]-img->m7[j1][n2+j2];
        }
        img->m7[j1][n2+0]=(m5[0]+m5[1])*13;
        img->m7[j1][n2+2]=(m5[0]-m5[1])*13;
        img->m7[j1][n2+1]=m5[3]*17+m5[2]*7;
        img->m7[j1][n2+3]=m5[3]*7-m5[2]*17;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  m1[0]=(img->m7[0][0]+img->m7[4][0]+img->m7[0][4]+img->m7[4][4])/2;
  m1[1]=(img->m7[0][0]-img->m7[4][0]+img->m7[0][4]-img->m7[4][4])/2;
  m1[2]=(img->m7[0][0]+img->m7[4][0]-img->m7[0][4]-img->m7[4][4])/2;
  m1[3]=(img->m7[0][0]-img->m7[4][0]-img->m7[0][4]+img->m7[4][4])/2;

  //     Quant of chroma 2X2 coeffs.
  quant_set=QP_SCALE_CR[img->qp];
  run=-1;
  scan_pos=0;

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
    coeff[coeff_ctr]=m1[coeff_ctr];

  rd_quant(QUANT_CHROMA_DC,coeff);

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
  {
    run++;
    ilev=0;
    level =0;

    level =(absm(coeff[coeff_ctr]));
    if (level  != 0)
    {
      currMB->cbp_blk |= 0xf0000 << (uv << 2) ;  // if one of the 2x2-DC levels is != 0 the coded-bit
      cr_cbp=max(1,cr_cbp);                      // for all 4 4x4 blocks is set (bit 16-19 or 20-23)
      img->cofu[scan_pos][0][uv]=sign(level ,m1[coeff_ctr]);
      img->cofu[scan_pos][1][uv]=run;
      scan_pos++;
      run=-1;
      ilev=level*JQ[quant_set][1];
    }
    m1[coeff_ctr]=sign(ilev,m1[coeff_ctr]);
  }

  img->cofu[scan_pos][0][uv]=0;

  //  Invers transform of 2x2 DC levels

  img->m7[0][0]=(m1[0]+m1[1]+m1[2]+m1[3])/2;
  img->m7[4][0]=(m1[0]-m1[1]+m1[2]-m1[3])/2;
  img->m7[0][4]=(m1[0]+m1[1]-m1[2]-m1[3])/2;
  img->m7[4][4]=(m1[0]-m1[1]-m1[2]+m1[3])/2;

  //     Quant of chroma AC-coeffs.

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      pos_x=n1/BLOCK_SIZE + 2*uv;
      pos_y=n2/BLOCK_SIZE + BLOCK_SIZE;
      run=-1;
      scan_pos=0;

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        coeff[coeff_ctr-1]=img->m7[n1+i][n2+j];
      }
      rd_quant(QUANT_CHROMA_AC,coeff);

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        ++run;
        ilev=0;

        level=absm(coeff[coeff_ctr-1]);
        if (level  != 0)
        {
          currMB->cbp_blk |=  1 << (16 + (uv << 2) + ((n2 >> 1) + (n1 >> 2))) ;
          cr_cbp = 2;
          img->cof[pos_x][pos_y][scan_pos][0][0]=sign(level,img->m7[n1+i][n2+j]);
          img->cof[pos_x][pos_y][scan_pos][1][0]=run;
          ++scan_pos;
          run=-1;
          ilev=level*JQ[quant_set][1];
        }
        img->m7[n1+i][n2+j]=sign(ilev,img->m7[n1+i][n2+j]); // for use in IDCT
      }
      img->cof[pos_x][pos_y][scan_pos][0][0]=0; // EOB


      //     IDCT.

      //     Horizontal.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          m5[i]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (i=0; i < 2; i++)
        {
          i1=3-i;
          img->m7[n1+i][n2+j]=m6[i]+m6[i1];
          img->m7[n1+i1][n2+j]=m6[i]-m6[i1];
        }
      }

      //     Vertical.
      for (i=0; i < BLOCK_SIZE; i++)
      {
        for (j=0; j < BLOCK_SIZE; j++)
        {
          m5[j]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (j=0; j < 2; j++)
        {
          j2=3-j;
          img->m7[n1+i][n2+j]=min(255,max(0,(m6[j]+m6[j2]+img->mpr[n1+i][n2+j]*JQQ1+JQQ2)/JQQ1));
          img->m7[n1+i][n2+j2]=min(255,max(0,(m6[j]-m6[j2]+img->mpr[n1+i][n2+j2]*JQQ1+JQQ2)/JQQ1));
        }
      }
    }
  }

  //  Decoded block moved to memory
  for (j=0; j < BLOCK_SIZE*2; j++)
    for (i=0; i < BLOCK_SIZE*2; i++)
    {
      imgUV[uv][img->pix_c_y+j][img->pix_c_x+i]= img->m7[i][j];
    }

  return cr_cbp;
}
#endif

//************************************************************************

#ifdef NO_RDQUANT
int dct_chroma(int uv,int cr_cbp)
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,coeff_ctr,qp_const,pos_x,pos_y,quant_set,level ,scan_pos,run;
  int m1[BLOCK_SIZE],m5[BLOCK_SIZE],m6[BLOCK_SIZE];
// int coeff[16];
  int coeff_cost;
  int cr_cbp_tmp;
  int nn0,nn1;
  int DCcoded=0 ;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  if (img->type == INTRA_IMG)
    qp_const=JQQ3;
  else
    qp_const=JQQ4;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {

      //  Horizontal transform.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++)
        {
          i1=3-i;
          m5[i]=img->m7[i+n1][mb_y]+img->m7[i1+n1][mb_y];
          m5[i1]=img->m7[i+n1][mb_y]-img->m7[i1+n1][mb_y];
        }
        img->m7[n1][mb_y]=(m5[0]+m5[1])*13;
        img->m7[n1+2][mb_y]=(m5[0]-m5[1])*13;
        img->m7[n1+1][mb_y]=m5[3]*17+m5[2]*7;
        img->m7[n1+3][mb_y]=m5[3]*7-m5[2]*17;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=img->m7[j1][n2+j]+img->m7[j1][n2+j2];
          m5[j2]=img->m7[j1][n2+j]-img->m7[j1][n2+j2];
        }
        img->m7[j1][n2+0]=(m5[0]+m5[1])*13;
        img->m7[j1][n2+2]=(m5[0]-m5[1])*13;
        img->m7[j1][n2+1]=m5[3]*17+m5[2]*7;
        img->m7[j1][n2+3]=m5[3]*7-m5[2]*17;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  m1[0]=(img->m7[0][0]+img->m7[4][0]+img->m7[0][4]+img->m7[4][4])/2;
  m1[1]=(img->m7[0][0]-img->m7[4][0]+img->m7[0][4]-img->m7[4][4])/2;
  m1[2]=(img->m7[0][0]+img->m7[4][0]-img->m7[0][4]-img->m7[4][4])/2;
  m1[3]=(img->m7[0][0]-img->m7[4][0]-img->m7[0][4]+img->m7[4][4])/2;

//     Quant of chroma 2X2 coeffs.
  quant_set=QP_SCALE_CR[img->qp];
  run=-1;
  scan_pos=0;

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
  {
    run++;
    ilev=0;

    level =(abs(m1[coeff_ctr])*JQ[quant_set][0]+qp_const)/JQQ1;// CHANGE rd_quant removed
    if (level  != 0)
    {
      currMB->cbp_blk |= 0xf0000 << (uv << 2) ;    // if one of the 2x2-DC levels is != 0 set the
      cr_cbp=max(1,cr_cbp);                     // coded-bit all 4 4x4 blocks (bit 16-19 or 20-23)
      DCcoded = 1 ;
      img->cofu[scan_pos][0][uv]=sign(level ,m1[coeff_ctr]);
      img->cofu[scan_pos][1][uv]=run;
      scan_pos++;
      run=-1;
      ilev=level*JQ[quant_set][1];
    }
    m1[coeff_ctr]=sign(ilev,m1[coeff_ctr]);
  }
  img->cofu[scan_pos][0][uv]=0;

  //  Invers transform of 2x2 DC levels

  img->m7[0][0]=(m1[0]+m1[1]+m1[2]+m1[3])/2;
  img->m7[4][0]=(m1[0]-m1[1]+m1[2]-m1[3])/2;
  img->m7[0][4]=(m1[0]+m1[1]-m1[2]-m1[3])/2;
  img->m7[4][4]=(m1[0]-m1[1]-m1[2]+m1[3])/2;

  //     Quant of chroma AC-coeffs.
  coeff_cost=0;
  cr_cbp_tmp=0;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      pos_x=n1/BLOCK_SIZE + 2*uv;
      pos_y=n2/BLOCK_SIZE + BLOCK_SIZE;
      run=-1;
      scan_pos=0;

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)// start change rd_quant
      {
        i = SNGL_SCAN[coeff_ctr][0];
        j = SNGL_SCAN[coeff_ctr][1];
        ++run;
        ilev=0;

        level=(abs(img->m7[n1+i][n2+j])*JQ[quant_set][0]+qp_const)/JQQ1;// CHANGE rd_quant removed
        if (level  != 0)
        {
          currMB->cbp_blk |= 1 << (16 + (uv << 2) + ((n2 >> 1) + (n1 >> 2))) ;
          if (level > 1)
            coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
          else
            coeff_cost += COEFF_COST[run];

          cr_cbp_tmp=2;
          img->cof[pos_x][pos_y][scan_pos][0][0]=sign(level,img->m7[n1+i][n2+j]);
          img->cof[pos_x][pos_y][scan_pos][1][0]=run;
          ++scan_pos;
          run=-1;
          ilev=level*JQ[quant_set][1];
        }
        img->m7[n1+i][n2+j]=sign(ilev,img->m7[n1+i][n2+j]); // for use in IDCT
      }
      img->cof[pos_x][pos_y][scan_pos][0][0]=0; // EOB
    }
  }

  // * reset chroma coeffs
  if(coeff_cost<7)
  {
    cr_cbp_tmp = 0 ;
    for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
    {
      for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
      {
        if( DCcoded == 0)                                   // if no chroma DC's: then reset
         currMB->cbp_blk &= ~(0xf0000 << (uv << 2)) ; // coded-bits of this chroma subblock
        nn0 = (n1>>2) + (uv<<1);
        nn1 = 4 + (n2>>2) ;
        img->cof[nn0][nn1][0][0][0] = 0;// dc coeff
        for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)// ac coeff
        {
          i=SNGL_SCAN[coeff_ctr][0];
          j=SNGL_SCAN[coeff_ctr][1];
          img->m7[n1+i][n2+j]=0;
          img->cof[nn0][nn1][coeff_ctr][0][0]=0;
        }
      }
    }
  }
  if(cr_cbp_tmp==2)
      cr_cbp = 2;
  //     IDCT.

      //     Horizontal.
  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      for (j=0; j < BLOCK_SIZE; j++)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          m5[i]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (i=0; i < 2; i++)
        {
          i1=3-i;
          img->m7[n1+i][n2+j]=m6[i]+m6[i1];
          img->m7[n1+i1][n2+j]=m6[i]-m6[i1];
        }
      }

      //     Vertical.
      for (i=0; i < BLOCK_SIZE; i++)
      {
        for (j=0; j < BLOCK_SIZE; j++)
        {
          m5[j]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (j=0; j < 2; j++)
        {
          j2=3-j;
          img->m7[n1+i][n2+j]=min(255,max(0,(m6[j]+m6[j2]+img->mpr[n1+i][n2+j]*JQQ1+JQQ2)/JQQ1));
          img->m7[n1+i][n2+j2]=min(255,max(0,(m6[j]-m6[j2]+img->mpr[n1+i][n2+j2]*JQQ1+JQQ2)/JQQ1));
        }
      }
    }
  }

  //  Decoded block moved to memory
  for (j=0; j < BLOCK_SIZE*2; j++)
    for (i=0; i < BLOCK_SIZE*2; i++)
      imgUV[uv][img->pix_c_y+j][img->pix_c_x+i]= img->m7[i][j];

  return cr_cbp;
}


#endif




void rd_quant(int scan_type,int *coeff)
{
  int idx,coeff_ctr;
  int qp_const,intra_add;

  int dbl_coeff_ctr;
  int level0,level1;
  int snr0;
  int dbl_coeff,k,k1,k2,rd_best,best_coeff_comb,rd_curr,snr1;
  int k0;
  int quant_set;
  int ilev,run,level;
  int no_coeff;

  if (img->type == INTRA_IMG)
  {
    qp_const=JQQ3;
    intra_add=2;
  }
  else
  {
    qp_const=JQQ4;
    intra_add=0;
  }
  quant_set=img->qp;

  switch (scan_type)
  {
  case  QUANT_LUMA_SNG:
    quant_set=img->qp;
    idx=2;
    no_coeff=16;
    break;
  case  QUANT_LUMA_AC:
    quant_set=img->qp;
    idx=2;
    no_coeff=15;
    break;
  case QUANT_LUMA_DBL:
    quant_set=img->qp;
    idx=1;
    no_coeff=8;
    break;
  case QUANT_CHROMA_DC:
    quant_set=QP_SCALE_CR[img->qp];
    idx=0;
    no_coeff=4;
    break;
  case QUANT_CHROMA_AC:
    quant_set=QP_SCALE_CR[img->qp];
    idx=2;
    no_coeff=15;
    break;
  default:
    error("rd_quant: unsupported scan_type", 600);
    break;
  }

  dbl_coeff_ctr=0;
  for (coeff_ctr=0;coeff_ctr < no_coeff ;coeff_ctr++)
  {
    k0=coeff[coeff_ctr];
    k1=abs(k0);

    if (dbl_coeff_ctr < MAX_TWO_LEVEL_COEFF)  // limit the number of 'twin' levels
    {
      level0 = (k0*JQ[quant_set][0])/J20;
      level1 = (k1*JQ[quant_set][0]+JQ4)/J20; // make positive summation
      level1 = sign(level1,k0);// set back sign on level
    }
    else
    {
      level0 = (k1*JQ[quant_set][0]+qp_const)/J20;
      level0 = sign(level0,k0);
      level1 = level0;
    }

    if (level0 != level1)
    {
      dbl_coeff = TRUE;     // decision is still open
      dbl_coeff_ctr++;      // count number of coefficients with 2 possible levels
    }
    else
      dbl_coeff = FALSE;    // level is decided


    snr0 = (12+intra_add)*level0*(64*level0 - (JQ[quant_set][0]*coeff[coeff_ctr])/J13); // find SNR improvement

    level_arr[coeff_ctr][MTLC_POW]=0; // indicates that all coefficients are decided

    for (k=0; k< MTLC_POW; k++)
    {
      level_arr[coeff_ctr][k]=level0;
      snr_arr[coeff_ctr][k]=snr0;
    }
    if (dbl_coeff)
    {
      snr1 = (12+intra_add)*level1*(64*level1 - (JQ[quant_set][0]*coeff[coeff_ctr])/J13);
      ilev= (int)pow(2,dbl_coeff_ctr-1);
      for (k1=ilev; k1<MTLC_POW; k1+=ilev*2)
      {
        for (k2=k1; k2<k1+ilev; k2++)
        {
          level_arr[coeff_ctr][k2]=level1;
          snr_arr[coeff_ctr][k2]=snr1;
        }
      }
    }
  }

  rd_best=0;
  best_coeff_comb= MTLC_POW;      // initial setting, used if no double decision coefficients
  for (k=0; k < pow(2,dbl_coeff_ctr);k++) // go through all combinations
  {
    rd_curr=0;
    run=-1;
    for (coeff_ctr=0;coeff_ctr < no_coeff;coeff_ctr++)
    {
      run++;
      level=min(16,absm(level_arr[coeff_ctr][k]));
      if (level != 0)
      {
        rd_curr += 64*COEFF_BIT_COST[idx][run][level-1]+snr_arr[coeff_ctr][k];
        run = -1;
      }
    }
    if (rd_curr < rd_best)
    {
      rd_best=rd_curr;
      best_coeff_comb=k;
    }
  }
  for (coeff_ctr=0;coeff_ctr < no_coeff ;coeff_ctr++)
    coeff[coeff_ctr]=level_arr[coeff_ctr][best_coeff_comb];

  return;
}

/*!
 ************************************************************************
 * \brief
 *    The routine performs transform,quantization,inverse transform, adds the diff.
 *    to the prediction and writes the result to the decoded luma frame. Includes the
 *    RD constrained quantization also.
 *
 * \para Input:
 *    block_x,block_y: Block position inside a macro block (0,4,8,12).
 *
 * \para Output:
 *    nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels.              \n
 *    coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
 *
 *
************************************************************************/
#ifndef NO_RDQUANT
int dct_luma_sp(int block_x,int block_y,int *coeff_cost)
{
  int sign(int a,int b);

  int i,j,i1,j1,ilev,m5[4],m6[4],coeff_ctr,scan_loop_ctr;
  int pos_x,pos_y,quant_set,level,scan_pos,run;
  int nonzero;
  int idx;

  int scan_mode;
  int loop_rep;
  int predicted_block[BLOCK_SIZE][BLOCK_SIZE],alpha,quant_set1,Fq1q2;
  int coeff[16],coeff2[16];

  pos_x=block_x/BLOCK_SIZE;
  pos_y=block_y/BLOCK_SIZE;

  //  Horizontal transform
  for (j=0; j< BLOCK_SIZE; j++)
    for (i=0; i< BLOCK_SIZE; i++)
    {
      img->m7[i][j]+=img->mpr[i+block_x][j+block_y];
      predicted_block[i][j]=img->mpr[i+block_x][j+block_y];
    }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=img->m7[i][j]+img->m7[i1][j];
      m5[i1]=img->m7[i][j]-img->m7[i1][j];
    }
    img->m7[0][j]=(m5[0]+m5[1])*13;
    img->m7[2][j]=(m5[0]-m5[1])*13;
    img->m7[1][j]=m5[3]*17+m5[2]*7;
    img->m7[3][j]=m5[3]*7-m5[2]*17;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=img->m7[i][j]+img->m7[i][j1];
      m5[j1]=img->m7[i][j]-img->m7[i][j1];
    }
    img->m7[i][0]=(m5[0]+m5[1])*13;
    img->m7[i][2]=(m5[0]-m5[1])*13;
    img->m7[i][1]=m5[3]*17+m5[2]*7;
    img->m7[i][3]=m5[3]*7-m5[2]*17;
  }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=predicted_block[i][j]+predicted_block[i1][j];
      m5[i1]=predicted_block[i][j]-predicted_block[i1][j];
    }
    predicted_block[0][j]=(m5[0]+m5[1])*13;
    predicted_block[2][j]=(m5[0]-m5[1])*13;
    predicted_block[1][j]=m5[3]*17+m5[2]*7;
    predicted_block[3][j]=m5[3]*7-m5[2]*17;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=predicted_block[i][j]+predicted_block[i][j1];
      m5[j1]=predicted_block[i][j]-predicted_block[i][j1];
    }
    predicted_block[i][0]=(m5[0]+m5[1])*13;
    predicted_block[i][2]=(m5[0]-m5[1])*13;
    predicted_block[i][1]=m5[3]*17+m5[2]*7;
    predicted_block[i][3]=m5[3]*7-m5[2]*17;
  }

  // Quant

  quant_set=img->qp;
  quant_set1=img->qpsp;
  alpha=(JQQ1+JQ[quant_set1][0]/2)/JQ[quant_set1][0];
  Fq1q2=(JQQ1*JQ[quant_set1][0]+JQ[quant_set][0]/2)/JQ[quant_set][0];
  nonzero=FALSE;

    scan_mode=SINGLE_SCAN;
    loop_rep=1;
    idx=0;

  for(scan_loop_ctr=0;scan_loop_ctr<loop_rep;scan_loop_ctr++) // 2 times if double scan, 1 normal scan
  {
    for (coeff_ctr=0;coeff_ctr < 16/loop_rep;coeff_ctr++)     // 8 times if double scan, 16 normal scan
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }
      coeff[coeff_ctr]=img->m7[i][j];
      coeff2[coeff_ctr]=(img->m7[i][j]-sign(((abs (predicted_block[i][j]) * JQ[quant_set1][0] +JQQ2) / JQQ1),predicted_block[i][j])*alpha);
    }
    rd_quant(QUANT_LUMA_SNG,coeff2);

    run=-1;
    scan_pos=scan_loop_ctr*9;   // for double scan; set first or second scan posision
    for (coeff_ctr=0; coeff_ctr<16/loop_rep; coeff_ctr++)
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }
      run++;
      ilev=0;

      level= absm(coeff2[coeff_ctr]);
      if (level != 0)
      {
        nonzero=TRUE;
        if (level > 1)
          *coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
        else
          *coeff_cost += COEFF_COST[run];
        img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=sign(level,coeff2[coeff_ctr]);
        img->cof[pos_x][pos_y][scan_pos][1][scan_mode]=run;
        ++scan_pos;
        run=-1;                     // reset zero level counter
        ilev=level;
      }
      ilev=coeff2[coeff_ctr]*Fq1q2+predicted_block[i][j]*JQ[quant_set1][0];
      img->m7[i][j]=sign((abs(ilev)+JQQ2)/ JQQ1,ilev)*JQ[quant_set1][1];
    }
    img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=0;  // end of block
  }

  //     IDCT.
  //     horizontal

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (i=0; i < 2; i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }

  //  vertical

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < BLOCK_SIZE; j++)
    {
      m5[j]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (j=0; j < 2; j++)
    {
      j1=3-j;
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+JQQ2)/JQQ1));
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+JQQ2)/JQQ1));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];


  return nonzero;
}
#endif
#ifdef NO_RDQUANT
int dct_luma_sp(int block_x,int block_y,int *coeff_cost)
{
  int sign(int a,int b);

  int i,j,i1,j1,ilev,m5[4],m6[4],coeff_ctr,scan_loop_ctr;
  int qp_const,pos_x,pos_y,quant_set,level,scan_pos,run;
  int nonzero;
  int idx;

  int scan_mode;
  int loop_rep;
  int predicted_block[BLOCK_SIZE][BLOCK_SIZE],alpha,quant_set1,Fq1q2,c_err;

  qp_const=JQQ4;    // inter

  pos_x=block_x/BLOCK_SIZE;
  pos_y=block_y/BLOCK_SIZE;

  //  Horizontal transform
  for (j=0; j< BLOCK_SIZE; j++)
    for (i=0; i< BLOCK_SIZE; i++)
    {
      img->m7[i][j]+=img->mpr[i+block_x][j+block_y];
      predicted_block[i][j]=img->mpr[i+block_x][j+block_y];
    }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=img->m7[i][j]+img->m7[i1][j];
      m5[i1]=img->m7[i][j]-img->m7[i1][j];
    }
    img->m7[0][j]=(m5[0]+m5[1])*13;
    img->m7[2][j]=(m5[0]-m5[1])*13;
    img->m7[1][j]=m5[3]*17+m5[2]*7;
    img->m7[3][j]=m5[3]*7-m5[2]*17;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=img->m7[i][j]+img->m7[i][j1];
      m5[j1]=img->m7[i][j]-img->m7[i][j1];
    }
    img->m7[i][0]=(m5[0]+m5[1])*13;
    img->m7[i][2]=(m5[0]-m5[1])*13;
    img->m7[i][1]=m5[3]*17+m5[2]*7;
    img->m7[i][3]=m5[3]*7-m5[2]*17;
  }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=predicted_block[i][j]+predicted_block[i1][j];
      m5[i1]=predicted_block[i][j]-predicted_block[i1][j];
    }
    predicted_block[0][j]=(m5[0]+m5[1])*13;
    predicted_block[2][j]=(m5[0]-m5[1])*13;
    predicted_block[1][j]=m5[3]*17+m5[2]*7;
    predicted_block[3][j]=m5[3]*7-m5[2]*17;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=predicted_block[i][j]+predicted_block[i][j1];
      m5[j1]=predicted_block[i][j]-predicted_block[i][j1];
    }
    predicted_block[i][0]=(m5[0]+m5[1])*13;
    predicted_block[i][2]=(m5[0]-m5[1])*13;
    predicted_block[i][1]=m5[3]*17+m5[2]*7;
    predicted_block[i][3]=m5[3]*7-m5[2]*17;
  }

  // Quant

  quant_set=img->qp;
  quant_set1=img->qpsp;
  alpha=(JQQ1+JQ[quant_set1][0]/2)/JQ[quant_set1][0];
  Fq1q2=(JQQ1*JQ[quant_set1][0]+JQ[quant_set][0]/2)/JQ[quant_set][0];
  nonzero=FALSE;

    scan_mode=SINGLE_SCAN;
    loop_rep=1;
    idx=0;

  for(scan_loop_ctr=0;scan_loop_ctr<loop_rep;scan_loop_ctr++) // 2 times if double scan, 1 normal scan
  {
  run=-1;
  scan_pos=scan_loop_ctr*9;

    for (coeff_ctr=0;coeff_ctr < 16/loop_rep;coeff_ctr++)     // 8 times if double scan, 16 normal scan
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }

      run++;
      ilev=0;

      c_err=img->m7[i][j]-alpha*sign(((abs (predicted_block[i][j]) * JQ[quant_set1][0] +JQQ2) / JQQ1),predicted_block[i][j]);
      level = (abs (c_err) * JQ[quant_set][0] +qp_const) / JQQ1;
      if (level != 0)
      {
        nonzero=TRUE;
        if (level > 1)
          *coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
        else
          *coeff_cost += COEFF_COST[run];
        img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=sign(level,c_err);
        img->cof[pos_x][pos_y][scan_pos][1][scan_mode]=run;
        ++scan_pos;
        run=-1;                     // reset zero level counter
        ilev=level;
      }
      ilev=sign(ilev,c_err)*Fq1q2+predicted_block[i][j]*JQ[quant_set1][0];
      img->m7[i][j]=sign((abs(ilev)+JQQ2)/ JQQ1*JQ[quant_set1][1],ilev);
    }
    img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=0;  // end of block
  }


  //     IDCT.
  //     horizontal

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (i=0; i < 2; i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }

  //  vertical

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < BLOCK_SIZE; j++)
    {
      m5[j]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (j=0; j < 2; j++)
    {
      j1=3-j;
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+JQQ2)/JQQ1));
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+JQQ2)/JQQ1));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];


  return nonzero;
}
#endif

/*!
 ************************************************************************
 * \brief
 *    Transform,quantization,inverse transform for chroma.
 *    The main reason why this is done in a separate routine is the
 *    additional 2x2 transform of DC-coeffs. This routine is called
 *    ones for each of the chroma components.
 *
 * \para Input:
 *    uv    : Make difference between the U and V chroma component               \n
 *    cr_cbp: chroma coded block pattern
 *
 * \para Output:
 *    cr_cbp: Updated chroma coded block pattern.
 ************************************************************************
 */
#ifndef NO_RDQUANT
int dct_chroma_sp(int uv,int cr_cbp)
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,coeff_ctr,qp_const,pos_x,pos_y,quant_set,level ,scan_pos,run;
  int m1[BLOCK_SIZE],m5[BLOCK_SIZE],m6[BLOCK_SIZE];
  int coeff[16];
  int predicted_chroma_block[MB_BLOCK_SIZE/2][MB_BLOCK_SIZE/2],alpha,Fq1q2,mp1[BLOCK_SIZE],quant_set1;

  qp_const=JQQ4;

  for (j=0; j < MB_BLOCK_SIZE/2; j++)
    for (i=0; i < MB_BLOCK_SIZE/2; i++)
    {
      img->m7[i][j]+=img->mpr[i][j];
      predicted_chroma_block[i][j]=img->mpr[i][j];
    }

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {

      //  Horizontal transform.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++)
        {
          i1=3-i;
          m5[i]=img->m7[i+n1][mb_y]+img->m7[i1+n1][mb_y];
          m5[i1]=img->m7[i+n1][mb_y]-img->m7[i1+n1][mb_y];
        }
        img->m7[n1][mb_y]=(m5[0]+m5[1])*13;
        img->m7[n1+2][mb_y]=(m5[0]-m5[1])*13;
        img->m7[n1+1][mb_y]=m5[3]*17+m5[2]*7;
        img->m7[n1+3][mb_y]=m5[3]*7-m5[2]*17;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=img->m7[j1][n2+j]+img->m7[j1][n2+j2];
          m5[j2]=img->m7[j1][n2+j]-img->m7[j1][n2+j2];
        }
        img->m7[j1][n2+0]=(m5[0]+m5[1])*13;
        img->m7[j1][n2+2]=(m5[0]-m5[1])*13;
        img->m7[j1][n2+1]=m5[3]*17+m5[2]*7;
        img->m7[j1][n2+3]=m5[3]*7-m5[2]*17;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  m1[0]=(img->m7[0][0]+img->m7[4][0]+img->m7[0][4]+img->m7[4][4])/2;
  m1[1]=(img->m7[0][0]-img->m7[4][0]+img->m7[0][4]-img->m7[4][4])/2;
  m1[2]=(img->m7[0][0]+img->m7[4][0]-img->m7[0][4]-img->m7[4][4])/2;
  m1[3]=(img->m7[0][0]-img->m7[4][0]-img->m7[0][4]+img->m7[4][4])/2;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {

      //  Horizontal transform.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++)
        {
          i1=3-i;
          m5[i]=predicted_chroma_block[i+n1][mb_y]+predicted_chroma_block[i1+n1][mb_y];
          m5[i1]=predicted_chroma_block[i+n1][mb_y]-predicted_chroma_block[i1+n1][mb_y];
        }
        predicted_chroma_block[n1][mb_y]=(m5[0]+m5[1])*13;
        predicted_chroma_block[n1+2][mb_y]=(m5[0]-m5[1])*13;
        predicted_chroma_block[n1+1][mb_y]=m5[3]*17+m5[2]*7;
        predicted_chroma_block[n1+3][mb_y]=m5[3]*7-m5[2]*17;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=predicted_chroma_block[j1][n2+j]+predicted_chroma_block[j1][n2+j2];
          m5[j2]=predicted_chroma_block[j1][n2+j]-predicted_chroma_block[j1][n2+j2];
        }
        predicted_chroma_block[j1][n2+0]=(m5[0]+m5[1])*13;
        predicted_chroma_block[j1][n2+2]=(m5[0]-m5[1])*13;
        predicted_chroma_block[j1][n2+1]=m5[3]*17+m5[2]*7;
        predicted_chroma_block[j1][n2+3]=m5[3]*7-m5[2]*17;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  mp1[0]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]+predicted_chroma_block[0][4]+predicted_chroma_block[4][4])/2;
  mp1[1]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]+predicted_chroma_block[0][4]-predicted_chroma_block[4][4])/2;
  mp1[2]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]-predicted_chroma_block[0][4]-predicted_chroma_block[4][4])/2;
  mp1[3]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]-predicted_chroma_block[0][4]+predicted_chroma_block[4][4])/2;

  //     Quant of chroma 2X2 coeffs.
  quant_set=QP_SCALE_CR[img->qp];
  quant_set1=QP_SCALE_CR[img->qpsp];
  alpha=(JQQ1+JQ[quant_set1][0]/2)/JQ[quant_set1][0];
  Fq1q2=(JQQ1*JQ[quant_set1][0]+JQ[quant_set][0]/2)/JQ[quant_set][0];

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
    coeff[coeff_ctr]=m1[coeff_ctr]-alpha*sign((abs (mp1[coeff_ctr]) * JQ[quant_set1][0] +JQQ2) / JQQ1,mp1[coeff_ctr]);
  rd_quant(QUANT_CHROMA_DC,coeff);

  run=-1;
  scan_pos=0;

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
  {
    run++;
    ilev=0;
    level =0;

    level =(absm(coeff[coeff_ctr]));
    if (level  != 0)
    {
      cr_cbp=max(1,cr_cbp);
      img->cofu[scan_pos][0][uv]=sign(level ,coeff[coeff_ctr]);
      img->cofu[scan_pos][1][uv]=run;
      scan_pos++;
      run=-1;
      ilev=level;
    }
      ilev=coeff[coeff_ctr]*Fq1q2+mp1[coeff_ctr]*JQ[quant_set1][0];
      m1[coeff_ctr]=sign((abs(ilev)+JQQ2)/ JQQ1,ilev)*JQ[quant_set1][1];
  }
  img->cofu[scan_pos][0][uv]=0;

  //  Invers transform of 2x2 DC levels

  img->m7[0][0]=(m1[0]+m1[1]+m1[2]+m1[3])/2;
  img->m7[4][0]=(m1[0]-m1[1]+m1[2]-m1[3])/2;
  img->m7[0][4]=(m1[0]+m1[1]-m1[2]-m1[3])/2;
  img->m7[4][4]=(m1[0]-m1[1]-m1[2]+m1[3])/2;
  //     Quant of chroma AC-coeffs.

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      pos_x=n1/BLOCK_SIZE + 2*uv;
      pos_y=n2/BLOCK_SIZE + BLOCK_SIZE;
      run=-1;
      scan_pos=0;

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        coeff[coeff_ctr-1]=img->m7[n1+i][n2+j]-alpha*sign((abs (predicted_chroma_block[n1+i][n2+j]) * JQ[quant_set1][0] +JQQ2) / JQQ1,predicted_chroma_block[n1+i][n2+j]);
      }
      rd_quant(QUANT_CHROMA_AC,coeff);

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        ++run;
        ilev=0;

        level=absm(coeff[coeff_ctr-1]);
        if (level  != 0)
        {
          cr_cbp=2;
          img->cof[pos_x][pos_y][scan_pos][0][0]=sign(level,coeff[coeff_ctr-1]);
          img->cof[pos_x][pos_y][scan_pos][1][0]=run;
          ++scan_pos;
          run=-1;
          ilev=level;
        }
        ilev=sign(ilev,coeff[coeff_ctr-1])*Fq1q2+predicted_chroma_block[n1+i][n2+j]*JQ[quant_set1][0];
        img->m7[n1+i][n2+j]=sign((abs(ilev)+JQQ2)/ JQQ1,ilev)*JQ[quant_set1][1];
      }
      img->cof[pos_x][pos_y][scan_pos][0][0]=0; // EOB

      //     IDCT.

      //     Horizontal.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          m5[i]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (i=0; i < 2; i++)
        {
          i1=3-i;
          img->m7[n1+i][n2+j]=m6[i]+m6[i1];
          img->m7[n1+i1][n2+j]=m6[i]-m6[i1];
        }
      }

      //     Vertical.
      for (i=0; i < BLOCK_SIZE; i++)
      {
        for (j=0; j < BLOCK_SIZE; j++)
        {
          m5[j]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (j=0; j < 2; j++)
        {
          j2=3-j;
          img->m7[n1+i][n2+j]=min(255,max(0,(m6[j]+m6[j2]+JQQ2)/JQQ1));
          img->m7[n1+i][n2+j2]=min(255,max(0,(m6[j]-m6[j2]+JQQ2)/JQQ1));
        }
      }
    }
  }

  //  Decoded block moved to memory
  for (j=0; j < BLOCK_SIZE*2; j++)
    for (i=0; i < BLOCK_SIZE*2; i++)
    {
      imgUV[uv][img->pix_c_y+j][img->pix_c_x+i]= img->m7[i][j];
    }

  return cr_cbp;
}
#endif
#ifdef NO_RDQUANT
int dct_chroma_sp(int uv,int cr_cbp)
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,coeff_ctr,qp_const,pos_x,pos_y,quant_set,quant_set1,c_err,level ,scan_pos,run;
  int m1[BLOCK_SIZE],m5[BLOCK_SIZE],m6[BLOCK_SIZE];
// int coeff[16];
  int coeff_cost;
  int cr_cbp_tmp;
  int predicted_chroma_block[MB_BLOCK_SIZE/2][MB_BLOCK_SIZE/2],alpha,Fq1q2,mp1[BLOCK_SIZE];
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];


  qp_const=JQQ4;

  for (j=0; j < MB_BLOCK_SIZE/2; j++)
        for (i=0; i < MB_BLOCK_SIZE/2; i++)
        {
          img->m7[i][j]+=img->mpr[i][j];
          predicted_chroma_block[i][j]=img->mpr[i][j];
        }

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {

      //  Horizontal transform.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++)
        {
          i1=3-i;
          m5[i]=img->m7[i+n1][mb_y]+img->m7[i1+n1][mb_y];
          m5[i1]=img->m7[i+n1][mb_y]-img->m7[i1+n1][mb_y];
        }
        img->m7[n1][mb_y]=(m5[0]+m5[1])*13;
        img->m7[n1+2][mb_y]=(m5[0]-m5[1])*13;
        img->m7[n1+1][mb_y]=m5[3]*17+m5[2]*7;
        img->m7[n1+3][mb_y]=m5[3]*7-m5[2]*17;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=img->m7[j1][n2+j]+img->m7[j1][n2+j2];
          m5[j2]=img->m7[j1][n2+j]-img->m7[j1][n2+j2];
        }
        img->m7[j1][n2+0]=(m5[0]+m5[1])*13;
        img->m7[j1][n2+2]=(m5[0]-m5[1])*13;
        img->m7[j1][n2+1]=m5[3]*17+m5[2]*7;
        img->m7[j1][n2+3]=m5[3]*7-m5[2]*17;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  m1[0]=(img->m7[0][0]+img->m7[4][0]+img->m7[0][4]+img->m7[4][4])/2;
  m1[1]=(img->m7[0][0]-img->m7[4][0]+img->m7[0][4]-img->m7[4][4])/2;
  m1[2]=(img->m7[0][0]+img->m7[4][0]-img->m7[0][4]-img->m7[4][4])/2;
  m1[3]=(img->m7[0][0]-img->m7[4][0]-img->m7[0][4]+img->m7[4][4])/2;

for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {

      //  Horizontal transform.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++)
        {
          i1=3-i;
          m5[i]=predicted_chroma_block[i+n1][mb_y]+predicted_chroma_block[i1+n1][mb_y];
          m5[i1]=predicted_chroma_block[i+n1][mb_y]-predicted_chroma_block[i1+n1][mb_y];
        }
        predicted_chroma_block[n1][mb_y]=(m5[0]+m5[1])*13;
        predicted_chroma_block[n1+2][mb_y]=(m5[0]-m5[1])*13;
        predicted_chroma_block[n1+1][mb_y]=m5[3]*17+m5[2]*7;
        predicted_chroma_block[n1+3][mb_y]=m5[3]*7-m5[2]*17;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=predicted_chroma_block[j1][n2+j]+predicted_chroma_block[j1][n2+j2];
          m5[j2]=predicted_chroma_block[j1][n2+j]-predicted_chroma_block[j1][n2+j2];
        }
        predicted_chroma_block[j1][n2+0]=(m5[0]+m5[1])*13;
        predicted_chroma_block[j1][n2+2]=(m5[0]-m5[1])*13;
        predicted_chroma_block[j1][n2+1]=m5[3]*17+m5[2]*7;
        predicted_chroma_block[j1][n2+3]=m5[3]*7-m5[2]*17;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  mp1[0]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]+predicted_chroma_block[0][4]+predicted_chroma_block[4][4])/2;
  mp1[1]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]+predicted_chroma_block[0][4]-predicted_chroma_block[4][4])/2;
  mp1[2]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]-predicted_chroma_block[0][4]-predicted_chroma_block[4][4])/2;
  mp1[3]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]-predicted_chroma_block[0][4]+predicted_chroma_block[4][4])/2;

//     Quant of chroma 2X2 coeffs.
  quant_set=QP_SCALE_CR[img->qp];
  quant_set1=QP_SCALE_CR[img->qpsp];
  alpha=(JQQ1+JQ[quant_set1][0]/2)/JQ[quant_set1][0];
  Fq1q2=(JQQ1*JQ[quant_set1][0]+JQ[quant_set][0]/2)/JQ[quant_set][0];

  run=-1;
  scan_pos=0;

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
  {
    run++;
    ilev=0;

    c_err=m1[coeff_ctr]-alpha*sign((abs (mp1[coeff_ctr]) * JQ[quant_set1][0] +JQQ2) / JQQ1,mp1[coeff_ctr]);
    level =(abs(c_err)*JQ[quant_set][0]+qp_const)/JQQ1;
    if (level  != 0)
    {
      currMB->cbp_blk |= 0xf0000 << (uv << 2) ;  // if one of the 2x2-DC levels is != 0 the coded-bit
      cr_cbp=max(1,cr_cbp);
      img->cofu[scan_pos][0][uv]=sign(level ,c_err);
      img->cofu[scan_pos][1][uv]=run;
      scan_pos++;
      run=-1;
      ilev=level;
    }
    ilev=sign(level,c_err)*Fq1q2+mp1[coeff_ctr]*JQ[quant_set1][0];
    m1[coeff_ctr]=sign((abs(ilev)+JQQ2)/ JQQ1,ilev)*JQ[quant_set1][1];
  }
  img->cofu[scan_pos][0][uv]=0;

  //  Invers transform of 2x2 DC levels

  img->m7[0][0]=(m1[0]+m1[1]+m1[2]+m1[3])/2;
  img->m7[4][0]=(m1[0]-m1[1]+m1[2]-m1[3])/2;
  img->m7[0][4]=(m1[0]+m1[1]-m1[2]-m1[3])/2;
  img->m7[4][4]=(m1[0]-m1[1]-m1[2]+m1[3])/2;

  //     Quant of chroma AC-coeffs.
  coeff_cost=0;
  cr_cbp_tmp=0;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      pos_x=n1/BLOCK_SIZE + 2*uv;
      pos_y=n2/BLOCK_SIZE + BLOCK_SIZE;
      run=-1;
      scan_pos=0;

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)// start change rd_quant
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        ++run;
        ilev=0;

        c_err=img->m7[n1+i][n2+j]-alpha*sign((abs (predicted_chroma_block[n1+i][n2+j]) * JQ[quant_set1][0] +JQQ2) / JQQ1,predicted_chroma_block[n1+i][n2+j]);
        level= (abs(c_err)*JQ[quant_set][0]+qp_const)/JQQ1;

        if (level  != 0)
        {
          currMB->cbp_blk |=  1 << (16 + (uv << 2) + ((n2 >> 1) + (n1 >> 2))) ;
          if (level > 1)
            coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
          else
            coeff_cost += COEFF_COST[run];

          cr_cbp_tmp=2;
          img->cof[pos_x][pos_y][scan_pos][0][0]=sign(level,c_err);
          img->cof[pos_x][pos_y][scan_pos][1][0]=run;
          ++scan_pos;
          run=-1;
          ilev=level;
        }
        ilev=sign(ilev,c_err)*Fq1q2+predicted_chroma_block[n1+i][n2+j]*JQ[quant_set1][0];
        img->m7[n1+i][n2+j]=sign((abs(ilev)+JQQ2)/ JQQ1,ilev)*JQ[quant_set1][1];
      }
      img->cof[pos_x][pos_y][scan_pos][0][0]=0; // EOB
    }
  }

  // * reset chroma coeffs

  if(cr_cbp_tmp==2)
      cr_cbp=2;
  //     IDCT.

      //     Horizontal.
  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      for (j=0; j < BLOCK_SIZE; j++)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          m5[i]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (i=0; i < 2; i++)
        {
          i1=3-i;
          img->m7[n1+i][n2+j]=m6[i]+m6[i1];
          img->m7[n1+i1][n2+j]=m6[i]-m6[i1];
        }
      }

      //     Vertical.
      for (i=0; i < BLOCK_SIZE; i++)
      {
        for (j=0; j < BLOCK_SIZE; j++)
        {
          m5[j]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;

        for (j=0; j < 2; j++)
        {
          j2=3-j;
          img->m7[n1+i][n2+j]=min(255,max(0,(m6[j]+m6[j2]+JQQ2)/JQQ1));
          img->m7[n1+i][n2+j2]=min(255,max(0,(m6[j]-m6[j2]+JQQ2)/JQQ1));
        }
      }
    }
  }

  //  Decoded block moved to memory
  for (j=0; j < BLOCK_SIZE*2; j++)
    for (i=0; i < BLOCK_SIZE*2; i++)
    {
      imgUV[uv][img->pix_c_y+j][img->pix_c_x+i]= img->m7[i][j];
    }

  return cr_cbp;
}


#endif

/*!
 ************************************************************************
 * \brief
 *    The routine performs transform,quantization,inverse transform, adds the diff.
 *    to the prediction and writes the result to the decoded luma frame. Includes the
 *    RD constrained quantization also.
 *
 * \para Input:
 *    block_x,block_y: Block position inside a macro block (0,4,8,12).
 *
 * \para Output:
 *    nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels.            \n
 *    coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
 ************************************************************************
 */
int copyblock_sp(int block_x,int block_y)
{
  int sign(int a,int b);

  int i,j,i1,j1,m5[4],m6[4],coeff_ctr;

  int predicted_block[BLOCK_SIZE][BLOCK_SIZE],quant_set1;

  //  Horizontal transform
  for (j=0; j< BLOCK_SIZE; j++)
    for (i=0; i< BLOCK_SIZE; i++)
    {
      predicted_block[i][j]=img->mpr[i+block_x][j+block_y];
    }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=predicted_block[i][j]+predicted_block[i1][j];
      m5[i1]=predicted_block[i][j]-predicted_block[i1][j];
    }
    predicted_block[0][j]=(m5[0]+m5[1])*13;
    predicted_block[2][j]=(m5[0]-m5[1])*13;
    predicted_block[1][j]=m5[3]*17+m5[2]*7;
    predicted_block[3][j]=m5[3]*7-m5[2]*17;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=predicted_block[i][j]+predicted_block[i][j1];
      m5[j1]=predicted_block[i][j]-predicted_block[i][j1];
    }
    predicted_block[i][0]=(m5[0]+m5[1])*13;
    predicted_block[i][2]=(m5[0]-m5[1])*13;
    predicted_block[i][1]=m5[3]*17+m5[2]*7;
    predicted_block[i][3]=m5[3]*7-m5[2]*17;
  }

  // Quant
  quant_set1=img->qpsp;

    for (coeff_ctr=0;coeff_ctr < 16;coeff_ctr++)     // 8 times if double scan, 16 normal scan
    {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        img->m7[i][j]=sign((abs(predicted_block[i][j])*JQ[quant_set1][0]+JQQ2)/ JQQ1,predicted_block[i][j])*JQ[quant_set1][1];
    }

  //     IDCT.
  //     horizontal

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (i=0; i < 2; i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }

  //  vertical

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < BLOCK_SIZE; j++)
    {
      m5[j]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;

    for (j=0; j < 2; j++)
    {
      j1=3-j;
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+JQQ2)/JQQ1));
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+JQQ2)/JQQ1));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];


  return 1;
}
