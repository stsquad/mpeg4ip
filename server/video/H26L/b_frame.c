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
 * \file b_frame.c
 *
 * \brief
 *    B picture coding
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Byeong-Moon Jeon                <jeonbm@lge.com>
 *    - Yoon-Seong Soh                  <yunsung@lge.com>
 *    - Thomas Stockhammer              <stockhammer@ei.tum.de>
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Guido Heising                   <heising@hhi.de>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *************************************************************************************
 */

#include <stdlib.h>
#include <math.h>
#include <memory.h>

#include "elements.h"
#include "b_frame.h"
#include "refbuf.h"


#ifdef _ADAPT_LAST_GROUP_
extern int *last_P_no;
#endif

/*!
 ************************************************************************
 * \brief
 *    Set reference frame information in global arrays
 *    depending on mode decision. Used for motion vector prediction.
 ************************************************************************
 */
void SetRefFrameInfo_B()
{
  int i,j;
  const int fw_predframe_no = img->mb_data[img->current_mb_nr].ref_frame;

  if(img->imod==B_Direct)
  {
    for (j = 0; j < 4;j++)
    {
      for (i = 0; i < 4;i++)
      {
        fw_refFrArr[img->block_y+j][img->block_x+i] =
            bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
      }
    }
  }
  else
    if (img->imod == B_Forward)
    {
      for (j = 0;j < 4;j++)
      {
        for (i = 0;i < 4;i++)
        {
          fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no;
          bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
        }
      }
    }
    else
      if(img->imod == B_Backward)
      {
        for (j = 0;j < 4;j++)
        {
          for (i = 0;i < 4;i++)
          {
            fw_refFrArr[img->block_y+j][img->block_x+i] = -1;
            bw_refFrArr[img->block_y+j][img->block_x+i] = 0;
          }
        }
      }
      else
        if(img->imod == B_Bidirect)
        {
          for (j = 0;j < 4;j++)
          {
            for (i = 0;i < 4;i++)
            {
              fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no;
              bw_refFrArr[img->block_y+j][img->block_x+i] = 0;
            }
          }
        }
        else // 4x4-, 16x16-intra
        {
          for (j = 0;j < 4;j++)
          {
            for (i = 0;i < 4;i++)
            {
              fw_refFrArr[img->block_y+j][img->block_x+i] =
                bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
            }
          }
        }

}


/*!
 ************************************************************************
 * \brief
 *    Performs DCT, quantization, run/level pre-coding and IDCT
 *    for the MC-compensated MB residue of a B-frame;
 *    current cbp (for LUMA only) is affected
 ************************************************************************
 */
void LumaResidualCoding_B()
{
  int cbp_mask, cbp_blk_mask, sum_cnt_nonz, coeff_cost, nonzero;
  int mb_y, mb_x, block_y, block_x, i, j, pic_pix_y, pic_pix_x, pic_block_x, pic_block_y;
  int ii4, jj4, iii4, jjj4, i2, j2, fw_pred, bw_pred, ref_inx, df_pred, db_pred;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];


  switch(img->imod)
  {
    case B_Forward :
      currMB->cbp     = 0 ;
      currMB->cbp_blk = 0 ;
      sum_cnt_nonz    = 0 ;

      for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
      {
        for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
        {
          cbp_mask=(int)pow(2,(mb_x/8+mb_y/4));
          coeff_cost=0;
          for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
          {
            pic_pix_y=img->pix_y+block_y;
            pic_block_y=pic_pix_y/BLOCK_SIZE;
            for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
            {
              cbp_blk_mask = (block_x>>2)+ block_y ;
              pic_pix_x=img->pix_x+block_x;
              pic_block_x=pic_pix_x/BLOCK_SIZE;

              img->ipredmode[pic_block_x+1][pic_block_y+1]=0;

              if(input->mv_res)
              {
                ii4=(img->pix_x+block_x)*8+tmp_fwMV[0][pic_block_y][pic_block_x+4];
                jj4=(img->pix_y+block_y)*8+tmp_fwMV[1][pic_block_y][pic_block_x+4];

                for (j=0;j<4;j++)
                {
                  j2=j*8;
                  for (i=0;i<4;i++)
                  {
                    i2=i*8;
                    img->mpr[i+block_x][j+block_y]=UMVPelY_18 (mref[img->fw_multframe_no], jj4+j2, ii4+i2); // refbuf
                  }
                }
              }
              else
              {
                ii4=(img->pix_x+block_x)*4+tmp_fwMV[0][pic_block_y][pic_block_x+4];
                jj4=(img->pix_y+block_y)*4+tmp_fwMV[1][pic_block_y][pic_block_x+4];

                for (j=0;j<4;j++)
                {
                  j2=j*4;
                  for (i=0;i<4;i++)
                  {
                    i2=i*4;
                    img->mpr[i+block_x][j+block_y]=UMVPelY_14 (mref[img->fw_multframe_no], jj4+j2, ii4+i2); // refbuf
                  }
                }
              }

              for (j=0; j < BLOCK_SIZE; j++)
              {
                for (i=0; i < BLOCK_SIZE; i++)
                {
                  img->m7[i][j]=
                    imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
                }
              }
              nonzero=dct_luma(block_x,block_y,&coeff_cost);
              if (nonzero)
              {
                currMB->cbp_blk |= 1 << cbp_blk_mask ;            // one bit for every 4x4 block
                currMB->cbp     |= cbp_mask;
              }
            } // block_x
          } // block_y

          if (coeff_cost > 3)
          {
            sum_cnt_nonz += coeff_cost;
          }
          else //discard
          {
            currMB->cbp     &=  (63-cbp_mask);
            currMB->cbp_blk &= ~(51 << (mb_y + (mb_x>>2) )) ;
            for (i=mb_x; i < mb_x+BLOCK_SIZE*2; i++)
            {
              for (j=mb_y; j < mb_y+BLOCK_SIZE*2; j++)
              {
                imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
              }
            }
          }
        } // mb_x
      } // mb_y

      if (sum_cnt_nonz <= 5 )
      {
        currMB->cbp     &= 0xfffff0 ;
        currMB->cbp_blk &= 0xff0000 ;
        for (i=0; i < MB_BLOCK_SIZE; i++)
        {
          for (j=0; j < MB_BLOCK_SIZE; j++)
          {
            imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
          }
        }
      }
      break;

    case B_Backward :
      currMB->cbp     = 0 ;
      currMB->cbp_blk = 0 ;
      sum_cnt_nonz    = 0 ;

      for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
      {
        for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
        {
          cbp_mask=(int)pow(2,(mb_x/8+mb_y/4));
          coeff_cost=0;
          for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
          {
            pic_pix_y=img->pix_y+block_y;
            pic_block_y=pic_pix_y/BLOCK_SIZE;
            for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
            {
              cbp_blk_mask = (block_x>>2)+ block_y ;
              pic_pix_x=img->pix_x+block_x;
              pic_block_x=pic_pix_x/BLOCK_SIZE;

              img->ipredmode[pic_block_x+1][pic_block_y+1]=0;

              if(input->mv_res)
              {
                iii4=(img->pix_x+block_x)*8+tmp_bwMV[0][pic_block_y][pic_block_x+4];
                jjj4=(img->pix_y+block_y)*8+tmp_bwMV[1][pic_block_y][pic_block_x+4];

                for (j=0;j<4;j++)
                {
                  j2=j*8;
                  for (i=0;i<4;i++)
                  {
                    i2=i*8;
                    img->mpr[i+block_x][j+block_y]=UMVPelY_18 (mref_P, jjj4+j2, iii4+i2); // refbuf
                  }
                }
              }
              else
              {
                iii4=(img->pix_x+block_x)*4+tmp_bwMV[0][pic_block_y][pic_block_x+4];
                jjj4=(img->pix_y+block_y)*4+tmp_bwMV[1][pic_block_y][pic_block_x+4];

                for (j=0;j<4;j++)
                {
                  j2=j*4;
                  for (i=0;i<4;i++)
                  {
                    i2=i*4;
                    img->mpr[i+block_x][j+block_y]=UMVPelY_14 (mref_P, jjj4+j2, iii4+i2); // refbuf
                  }
                }
              }

              for (j=0; j < BLOCK_SIZE; j++)
              {
                for (i=0; i < BLOCK_SIZE; i++)
                {
                  img->m7[i][j]=
                    imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
                }
              }
              nonzero=dct_luma(block_x,block_y,&coeff_cost);
              if (nonzero)
              {
                currMB->cbp_blk |= 1 << cbp_blk_mask ;            // one bit for every 4x4 block
                currMB->cbp     |= cbp_mask;
              }
            } // block_x
          } // block_y

          if (coeff_cost > 3)
          {
            sum_cnt_nonz += coeff_cost;
          }
          else //discard
          {
            currMB->cbp     &= (63-cbp_mask);
            currMB->cbp_blk &= ~(51 << (mb_y + (mb_x>>2) )) ;
            for (i=mb_x; i < mb_x+BLOCK_SIZE*2; i++)
            {
              for (j=mb_y; j < mb_y+BLOCK_SIZE*2; j++)
              {
                imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
              }
            }
          }
        } // mb_x
      } // mb_y

      if (sum_cnt_nonz <= 5 )
      {
        currMB->cbp     &= 0xfffff0 ;
        currMB->cbp_blk &= 0xff0000 ;
        for (i=0; i < MB_BLOCK_SIZE; i++)
        {
          for (j=0; j < MB_BLOCK_SIZE; j++)
          {
            imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
          }
        }
      }
      break;

    case B_Bidirect :
      currMB->cbp=0;
      currMB->cbp_blk=0;
      sum_cnt_nonz=0;
      for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
      {
        for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
        {
          cbp_mask=(int)pow(2,(mb_x/8+mb_y/4));
          coeff_cost=0;
          for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
          {
            pic_pix_y=img->pix_y+block_y;
            pic_block_y=pic_pix_y/BLOCK_SIZE;
            for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
            {
              cbp_blk_mask = (block_x>>2)+ block_y ;
              pic_pix_x=img->pix_x+block_x;
              pic_block_x=pic_pix_x/BLOCK_SIZE;

              img->ipredmode[pic_block_x+1][pic_block_y+1]=0;

              if(input->mv_res)
              {
                ii4=(img->pix_x+block_x)*8+tmp_fwMV[0][pic_block_y][pic_block_x+4];
                jj4=(img->pix_y+block_y)*8+tmp_fwMV[1][pic_block_y][pic_block_x+4];
                iii4=(img->pix_x+block_x)*8+tmp_bwMV[0][pic_block_y][pic_block_x+4];
                jjj4=(img->pix_y+block_y)*8+tmp_bwMV[1][pic_block_y][pic_block_x+4];

                for (j=0;j<4;j++)
                {
                  j2=j*8;
                  for (i=0;i<4;i++)
                  {
                    i2=i*8;
                    fw_pred=UMVPelY_18 (mref[img->fw_multframe_no], jj4+j2, ii4+i2);  // refbuf
                    bw_pred=UMVPelY_18 (mref_P, jjj4+j2, iii4+i2);  // refbuf
                    img->mpr[i+block_x][j+block_y]=(int)((fw_pred+bw_pred)/2.0+0.5);
                  }
                }
              }
              else
              {
                ii4=(img->pix_x+block_x)*4+tmp_fwMV[0][pic_block_y][pic_block_x+4];
                jj4=(img->pix_y+block_y)*4+tmp_fwMV[1][pic_block_y][pic_block_x+4];
                iii4=(img->pix_x+block_x)*4+tmp_bwMV[0][pic_block_y][pic_block_x+4];
                jjj4=(img->pix_y+block_y)*4+tmp_bwMV[1][pic_block_y][pic_block_x+4];

                for (j=0;j<4;j++)
                {
                  j2=j*4;
                  for (i=0;i<4;i++)
                  {
                    i2=i*4;
                    fw_pred=UMVPelY_14 (mref[img->fw_multframe_no], jj4+j2, ii4+i2);  // refbuf
                    bw_pred=UMVPelY_14 (mref_P, jjj4+j2, iii4+i2);  // refbuf
                    img->mpr[i+block_x][j+block_y]=(int)((fw_pred+bw_pred)/2.0+0.5);
                  }
                }
              }

              for (j=0; j < BLOCK_SIZE; j++)
              {
                for (i=0; i < BLOCK_SIZE; i++)
                {
                  img->m7[i][j]=
                    imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
                }
              }
              nonzero=dct_luma(block_x,block_y,&coeff_cost);
              if (nonzero)
              {
                currMB->cbp_blk |= 1 << cbp_blk_mask ;            // one bit for every 4x4 block
                currMB->cbp     |= cbp_mask;
              }
            } // block_x
          } // block_y

          if (coeff_cost > 3)
          {
            sum_cnt_nonz += coeff_cost;
          }
          else //discard
          {
            currMB->cbp     &= (63-cbp_mask);
            currMB->cbp_blk &= ~(51 << (mb_y + (mb_x>>2) )) ;
            for (i=mb_x; i < mb_x+BLOCK_SIZE*2; i++)
            {
              for (j=mb_y; j < mb_y+BLOCK_SIZE*2; j++)
              {
                imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
              }
            }
          }
        } // mb_x
      } // mb_y

      if (sum_cnt_nonz <= 5 )
      {
        currMB->cbp     &= 0xfffff0 ;
        currMB->cbp_blk &= 0xff0000 ;
        for (i=0; i < MB_BLOCK_SIZE; i++)
        {
          for (j=0; j < MB_BLOCK_SIZE; j++)
          {
            imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
          }
        }
      }
      break;

    case B_Direct :
      currMB->cbp=0;
      currMB->cbp_blk=0;
      sum_cnt_nonz=0;
      for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
      {
        for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
        {
          cbp_mask=(int)pow(2,(mb_x/8+mb_y/4));
          coeff_cost=0;
          for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
          {
            pic_pix_y=img->pix_y+block_y;
            pic_block_y=pic_pix_y/BLOCK_SIZE;
            for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
            {
              cbp_blk_mask = (block_x>>2)+ block_y ;
              pic_pix_x=img->pix_x+block_x;
              pic_block_x=pic_pix_x/BLOCK_SIZE;

              img->ipredmode[pic_block_x+1][pic_block_y+1]=0;

              if(input->mv_res)
              {
                ii4 =(img->pix_x+block_x)*8+dfMV[0][pic_block_y][pic_block_x+4];
                jj4 =(img->pix_y+block_y)*8+dfMV[1][pic_block_y][pic_block_x+4];
                iii4=(img->pix_x+block_x)*8+dbMV[0][pic_block_y][pic_block_x+4];
                jjj4=(img->pix_y+block_y)*8+dbMV[1][pic_block_y][pic_block_x+4];

                // next P is intra mode
                if(refFrArr[pic_block_y][pic_block_x]==-1)
                  ref_inx=(img->number-1)%img->buf_cycle;
                // next P is skip or inter mode
                else
                  ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%img->buf_cycle;

                for (j=0;j<4;j++)
                {
                  j2=j*8;
                  for (i=0;i<4;i++)
                  {
                    i2=i*8;
                    df_pred=UMVPelY_18 (mref[ref_inx], jj4+j2, ii4+i2);
                    db_pred=UMVPelY_18 (mref_P, jjj4+j2, iii4+i2);

                    img->mpr[i+block_x][j+block_y]=(int)((df_pred+db_pred)/2.0+0.5);
                  }
                }
              }
              else
              {
                ii4 =(img->pix_x+block_x)*4+dfMV[0][pic_block_y][pic_block_x+4];
                jj4 =(img->pix_y+block_y)*4+dfMV[1][pic_block_y][pic_block_x+4];
                iii4=(img->pix_x+block_x)*4+dbMV[0][pic_block_y][pic_block_x+4];
                jjj4=(img->pix_y+block_y)*4+dbMV[1][pic_block_y][pic_block_x+4];

                // next P is intra mode
                if(refFrArr[pic_block_y][pic_block_x]==-1)
                  ref_inx=(img->number-1)%img->buf_cycle;
                // next P is skip or inter mode
                else
                  ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%img->buf_cycle;

                for (j=0;j<4;j++)
                {
                  j2=j*4;
                  for (i=0;i<4;i++)
                  {
                    i2=i*4;
                    df_pred=UMVPelY_14 (mref[ref_inx], jj4+j2, ii4+i2);
                    db_pred=UMVPelY_14 (mref_P, jjj4+j2, iii4+i2);

                    img->mpr[i+block_x][j+block_y]=(int)((df_pred+db_pred)/2.0+0.5);
                  }
                }
              }
              // LG : direct residual coding
              for (j=0; j < BLOCK_SIZE; j++)
              {
                for (i=0; i < BLOCK_SIZE; i++)
                {
                  img->m7[i][j]=
                    imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
                }
              }
              nonzero=dct_luma(block_x,block_y,&coeff_cost);
              if (nonzero)
              {
                currMB->cbp_blk |= 1 << cbp_blk_mask ;            // one bit for every 4x4 block
                currMB->cbp     |= cbp_mask;
              }
            } // block_x
          } // block_y

          // LG : direct residual coding
          if (coeff_cost > 3)
          {
            sum_cnt_nonz += coeff_cost;
          }
          else //discard
          {
            currMB->cbp     &= (63-cbp_mask);
            currMB->cbp_blk &= ~(51 << (mb_y + (mb_x>>2) )) ;
            for (i=mb_x; i < mb_x+BLOCK_SIZE*2; i++)
            {
              for (j=mb_y; j < mb_y+BLOCK_SIZE*2; j++)
              {
                imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
              }
            }
          }
        } // mb_x
      } // mb_y

      // LG : direct residual coding
      if (sum_cnt_nonz <= 5 )
      {
        currMB->cbp     &= 0xfffff0 ;
        currMB->cbp_blk &= 0xff0000 ;
        for (i=0; i < MB_BLOCK_SIZE; i++)
        {
          for (j=0; j < MB_BLOCK_SIZE; j++)
          {
            imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
          }
        }
      }
      break;

    default:
      break;
  } // switch()
}

/*!
 ************************************************************************
 * \brief
 *    Performs DCT, quantization, run/level pre-coding and IDCT
 *    for the chrominance of a B-frame macroblock;
 *    current cbp and cr_cbp are affected
 ************************************************************************
 */
void ChromaCoding_B(int *cr_cbp)
{
  int i, j;
  int uv, ii,jj,ii0,jj0,ii1,jj1,if1,jf1,if0,jf0,f1,f2,f3,f4;
  int pic_block_y, pic_block_x, ref_inx, fw_pred, bw_pred;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  if(input->mv_res)
  {
    f1=16;
    f2=15;
  }
  else
  {
    f1=8;
    f2=7;
  }

  f3=f1*f1;
  f4=f3/2;

  *cr_cbp=0;
  for (uv=0; uv < 2; uv++)
  {
    if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
    {
      intrapred_chroma(img->pix_c_x,img->pix_c_y,uv);
    }
    else if(img->imod == B_Forward)
    {
      for (j=0; j < MB_BLOCK_SIZE/2; j++)
      {
        pic_block_y=(img->pix_c_y+j)/2;
        for (i=0; i < MB_BLOCK_SIZE/2; i++)
        {
          pic_block_x=(img->pix_c_x+i)/2;
          ii=(img->pix_c_x+i)*f1+tmp_fwMV[0][pic_block_y][pic_block_x+4];
          jj=(img->pix_c_y+j)*f1+tmp_fwMV[1][pic_block_y][pic_block_x+4];

          ii0= max (0, min (img->width_cr-1, ii/f1));
          jj0= max (0, min (img->height_cr-1, jj/f1));
          ii1= max (0, min (img->width_cr-1, (ii+f2)/f1));
          jj1= max (0, min (img->height_cr-1, (jj+f2)/f1));

          if1=(ii & f2);
          jf1=(jj & f2);
          if0=f1-if1;
          jf0=f1-jf1;
          img->mpr[i][j]=(if0*jf0*mcef[img->fw_multframe_no][uv][jj0][ii0]+
                  if1*jf0*mcef[img->fw_multframe_no][uv][jj0][ii1]+
                  if0*jf1*mcef[img->fw_multframe_no][uv][jj1][ii0]+
                  if1*jf1*mcef[img->fw_multframe_no][uv][jj1][ii1]+f4)/f3;

          img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j];
        }
      }
    }
    else if(img->imod == B_Backward)
    {
      for (j=0; j < MB_BLOCK_SIZE/2; j++)
      {
        pic_block_y=(img->pix_c_y+j)/2;
        for (i=0; i < MB_BLOCK_SIZE/2; i++)
        {
          pic_block_x=(img->pix_c_x+i)/2;

          ii=(img->pix_c_x+i)*f1+tmp_bwMV[0][pic_block_y][pic_block_x+4];
          jj=(img->pix_c_y+j)*f1+tmp_bwMV[1][pic_block_y][pic_block_x+4];

          ii0= max (0, min (img->width_cr-1, ii/f1));
          jj0= max (0, min (img->height_cr-1, jj/f1));
          ii1= max (0, min (img->width_cr-1, (ii+f2)/f1));
          jj1= max (0, min (img->height_cr-1, (jj+f2)/f1));

          if1=(ii & f2);
          jf1=(jj & f2);
          if0=f1-if1;
          jf0=f1-jf1;
          img->mpr[i][j]=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
                    if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+f4)/f3;

          img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j];
        }
      }
    }
    else if(img->imod == B_Bidirect)
    {
      for (j=0; j < MB_BLOCK_SIZE/2; j++)
      {
        pic_block_y=(img->pix_c_y+j)/2;
        for (i=0; i < MB_BLOCK_SIZE/2; i++)
        {
          pic_block_x=(img->pix_c_x+i)/2;

          ii=(img->pix_c_x+i)*f1+tmp_fwMV[0][pic_block_y][pic_block_x+4];
          jj=(img->pix_c_y+j)*f1+tmp_fwMV[1][pic_block_y][pic_block_x+4];

          ii0= max (0, min (img->width_cr-1, ii/f1));
          jj0= max (0, min (img->height_cr-1, jj/f1));
          ii1= max (0, min (img->width_cr-1, (ii+f2)/f1));
          jj1= max (0, min (img->height_cr-1, (jj+f2)/f1));

          if1=(ii & f2);
          jf1=(jj & f2);
          if0=f1-if1;
          jf0=f1-jf1;
          fw_pred=(if0*jf0*mcef[img->fw_multframe_no][uv][jj0][ii0]+
               if1*jf0*mcef[img->fw_multframe_no][uv][jj0][ii1]+
               if0*jf1*mcef[img->fw_multframe_no][uv][jj1][ii0]+
               if1*jf1*mcef[img->fw_multframe_no][uv][jj1][ii1]+f4)/f3;

          ii=(img->pix_c_x+i)*f1+tmp_bwMV[0][pic_block_y][pic_block_x+4];
          jj=(img->pix_c_y+j)*f1+tmp_bwMV[1][pic_block_y][pic_block_x+4];

          ii0= max (0, min (img->width_cr-1, ii/f1));
          jj0= max (0, min (img->height_cr-1, jj/f1));
          ii1= max (0, min (img->width_cr-1, (ii+f2)/f1));
          jj1= max (0, min (img->height_cr-1, (jj+f2)/f1));

          if1=(ii & f2);
          jf1=(jj & f2);
          if0=f1-if1;
          jf0=f1-jf1;
          bw_pred=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
               if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+f4)/f3;

          img->mpr[i][j]=(int)((fw_pred+bw_pred)/2.0+0.5);
          img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j];
        }
      }
    }
    else // img->imod == B_Direct
    {
      for (j=0; j < MB_BLOCK_SIZE/2; j++)
      {
        pic_block_y=(img->pix_c_y+j)/2;
        for (i=0; i < MB_BLOCK_SIZE/2; i++)
        {
          pic_block_x=(img->pix_c_x+i)/2;

          ii=(img->pix_c_x+i)*f1+dfMV[0][pic_block_y][pic_block_x+4];
          jj=(img->pix_c_y+j)*f1+dfMV[1][pic_block_y][pic_block_x+4];

          ii0= max (0, min (img->width_cr-1, ii/f1));
          jj0= max (0, min (img->height_cr-1, jj/f1));
          ii1= max (0, min (img->width_cr-1, (ii+f2)/f1));
          jj1= max (0, min (img->height_cr-1, (jj+f2)/f1));

          if1=(ii & f2);
          jf1=(jj & f2);
          if0=f1-if1;
          jf0=f1-jf1;

          // next P is intra mode
          if(refFrArr[pic_block_y][pic_block_x]==-1)
            ref_inx=(img->number-1)%img->buf_cycle;
          // next P is skip or inter mode
          else
            ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%img->buf_cycle;

          fw_pred=(if0*jf0*mcef[ref_inx][uv][jj0][ii0]+
               if1*jf0*mcef[ref_inx][uv][jj0][ii1]+
               if0*jf1*mcef[ref_inx][uv][jj1][ii0]+
               if1*jf1*mcef[ref_inx][uv][jj1][ii1]+f4)/f3;

          ii=(img->pix_c_x+i)*f1+dbMV[0][pic_block_y][pic_block_x+4];
          jj=(img->pix_c_y+j)*f1+dbMV[1][pic_block_y][pic_block_x+4];

          ii0= max (0, min (img->width_cr-1, ii/f1));
          jj0= max (0, min (img->height_cr-1, jj/f1));
          ii1= max (0, min (img->width_cr-1, (ii+f2)/f1));
          jj1= max (0, min (img->height_cr-1, (jj+f2)/f1));

          if1=(ii & f2);
          jf1=(jj & f2);
          if0=f1-if1;
          jf0=f1-jf1;

          bw_pred=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
               if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+f4)/f3;

          img->mpr[i][j]=(int)((fw_pred+bw_pred)/2.0+0.5);

          // LG : direct residual coding
          img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j];
        }
      }
    }
    *cr_cbp=dct_chroma(uv, *cr_cbp);
  }
  // LG : direct residual coding
  currMB->cbp += *cr_cbp*16;
}


/*!
 ************************************************************************
 * \brief
 *    Passes for a given MB of a B picture the reference frame
 *    parameter and motion parameters to the NAL
 ************************************************************************
 */
int writeMotionInfo2NAL_Bframe()
{
  int i,j,k,l,m;
  int step_h,step_v;
  int curr_mvd=0, fw_blk_shape=0, bw_blk_shape=0;
  int mb_nr = img->current_mb_nr;
  Macroblock *currMB = &img->mb_data[mb_nr];
  const int fw_predframe_no=currMB->ref_frame;
  SyntaxElement *currSE = &img->MB_SyntaxElements[currMB->currSEnr];
  int *bitCount = currMB->bitcounter;
  Slice *currSlice = img->currentSlice;
  DataPartition *dataPart;
  int *partMap = assignSE2partition[input->partition_mode];
  int no_bits = 0;


  // Write fw_predframe_no(Forward, Bidirect)
  if(img->imod==B_Forward || img->imod==B_Bidirect)
  {
#ifdef _ADDITIONAL_REFERENCE_FRAME_
    if (img->no_multpred > 1 || input->add_ref_frame > 0)
#else
      if (img->no_multpred > 1)
#endif
    {
      currSE->value1 = currMB->ref_frame;
      currSE->type = SE_REFFRAME;
      if (input->symbol_mode == UVLC)
        currSE->mapping = n_linfo2;
      else
        currSE->writing = writeRefFrame2Buffer_CABAC;

      dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
      dataPart->writeSyntaxElement( currSE, dataPart);
      bitCount[BITS_INTER_MB]+=currSE->len;
      no_bits += currSE->len;
#if TRACE
      snprintf(currSE->tracestring, TRACESTRING_SIZE, "B_fw_Reference frame no %3d ", currMB->ref_frame);
#endif
      // proceed to next SE
      currSE++;
      currMB->currSEnr++;
    }
  }

  // Write Blk_size(Bidirect)
  if(img->imod==B_Bidirect)
  {
    // Write blockshape for forward pred
    fw_blk_shape = BlkSize2CodeNumber(img->fw_blc_size_h, img->fw_blc_size_v);

    currSE->value1 = fw_blk_shape;
    currSE->type = SE_BFRAME;
    if (input->symbol_mode == UVLC)
      currSE->mapping = n_linfo2;
    else
      currSE->writing = writeBiDirBlkSize2Buffer_CABAC;

    dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
    dataPart->writeSyntaxElement( currSE, dataPart);
    bitCount[BITS_INTER_MB]+=currSE->len;
    no_bits += currSE->len;
#if TRACE
    snprintf(currSE->tracestring, TRACESTRING_SIZE, "B_bidiret_fw_blk %3d x %3d ", img->fw_blc_size_h, img->fw_blc_size_v);
#endif

    // proceed to next SE
    currSE++;
    currMB->currSEnr++;

    // Write blockshape for backward pred
    bw_blk_shape = BlkSize2CodeNumber(img->bw_blc_size_h, img->bw_blc_size_v);

    currSE->value1 = bw_blk_shape;
    currSE->type = SE_BFRAME;
    if (input->symbol_mode == UVLC)
      currSE->mapping = n_linfo2;
    else
      currSE->writing = writeBiDirBlkSize2Buffer_CABAC;

    dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
    dataPart->writeSyntaxElement( currSE, dataPart);
    bitCount[BITS_INTER_MB]+=currSE->len;
    no_bits += currSE->len;
#if TRACE
    snprintf(currSE->tracestring, TRACESTRING_SIZE, "B_bidiret_bw_blk %3d x %3d ", img->bw_blc_size_h, img->bw_blc_size_v);
#endif

    // proceed to next SE
    currSE++;
    currMB->currSEnr++;

  }

  // Write MVDFW(Forward, Bidirect)
  if(img->imod==B_Forward || img->imod==B_Bidirect)
  {
    step_h=img->fw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize
    step_v=img->fw_blc_size_v/BLOCK_SIZE;  // vertical stepsize

    for (j=0; j < BLOCK_SIZE; j += step_v)
    {
      for (i=0;i < BLOCK_SIZE; i += step_h)
      {
        for (k=0; k < 2; k++)
        {
          if(img->mb_mode==1) // fw 16x16
            curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][k]);
          else
            if(img->mb_mode==3) // bidirectinal
            {
              switch(fw_blk_shape)
              {
                case 0:
                  curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][k]);
                  break;
                case 1:
                  curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][2][k]);
                  break;
                case 2:
                  curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][3][k]);
                  break;
                case 3:
                  curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][4][k]);
                  break;
                case 4:
                  curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][5][k]);
                  break;
                case 5:
                  curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][6][k]);
                  break;
                case 6:
                  curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][7][k]);
                  break;
                default:
                  break;
              }
            }
            else
              curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][(img->mb_mode)/2][k]);

          // store (oversampled) mvd
          for (l=0; l < step_v; l++)
            for (m=0; m < step_h; m++)
              currMB->mvd[0][j+l][i+m][k] = curr_mvd;

          currSE->value1 = curr_mvd;
          currSE->type = SE_MVD;
          if (input->symbol_mode == UVLC)
            currSE->mapping = mvd_linfo2;
          else
          {
            img->subblock_x = i; // position used for context determination
            img->subblock_y = j; // position used for context determination
            currSE->value2 = 2*k; // identifies the component and the direction; only used for context determination
            currSE->writing = writeBiMVD2Buffer_CABAC;
          }
          dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
          dataPart->writeSyntaxElement( currSE, dataPart);
          bitCount[BITS_INTER_MB]+=currSE->len;
          no_bits += currSE->len;
#if TRACE
          snprintf(currSE->tracestring, TRACESTRING_SIZE, " MVD(%d) = %3d",k, curr_mvd);
#endif

          // proceed to next SE
          currSE++;
          currMB->currSEnr++;
        }
      }
    }
  }

  // Write MVDBW(Backward, Bidirect)
  if(img->imod==B_Backward || img->imod==B_Bidirect)
  {
    step_h=img->bw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize
    step_v=img->bw_blc_size_v/BLOCK_SIZE;  // vertical stepsize

    for (j=0; j < BLOCK_SIZE; j += step_v)
    {
      for (i=0;i < BLOCK_SIZE; i += step_h)
      {
        for (k=0; k < 2; k++)
        {
          if(img->mb_mode==2) // bw 16x16
            curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][k]);
          else
            if(img->mb_mode==3) // bidirectional
            {
              switch(bw_blk_shape)
              {
                case 0:
                  curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][k]);
                  break;
                case 1:
                  curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][2][k]);
                  break;
                case 2:
                  curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][3][k]);
                  break;
                case 3:
                  curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][4][k]);
                  break;
                case 4:
                  curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][5][k]);
                  break;
                case 5:
                  curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][6][k]);
                  break;
                case 6:
                  curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][7][k]);
                  break;
                default:
                  break;
              }
            }
            else // other bw
              curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][(img->mb_mode-1)/2][k]);


          // store (oversampled) mvd
          for (l=0; l < step_v; l++)
            for (m=0; m < step_h; m++)
              currMB->mvd[1][j+l][i+m][k] = curr_mvd;

          currSE->value1 = curr_mvd;
          currSE->type = SE_MVD;

          if (input->symbol_mode == UVLC)
            currSE->mapping = mvd_linfo2;
          else
          {
            img->subblock_x = i; // position used for context determination
            img->subblock_y = j; // position used for context determination
            currSE->value2 = 2*k+1; // identifies the component and the direction; only used for context determination
            currSE->writing = writeBiMVD2Buffer_CABAC;
          }

          dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
          dataPart->writeSyntaxElement( currSE, dataPart);
          bitCount[BITS_INTER_MB]+=currSE->len;
          no_bits += currSE->len;
#if TRACE
          snprintf(currSE->tracestring, TRACESTRING_SIZE, " MVD(%d) = %3d",k, curr_mvd);
#endif

          // proceed to next SE
          currSE++;
          currMB->currSEnr++;

        }
      }
    }
  }
  return no_bits;
}

/*!
 ************************************************************************
 * \brief
 *    Passes back the code number given the blocksize width and
 *    height (should be replaced by an appropriate table lookup)
 ************************************************************************
 */
int BlkSize2CodeNumber(int blc_size_h, int blc_size_v)
{

  if(blc_size_h==16 && blc_size_v==16)  // 16x16 : code_number 0
    return 0;
  else
    if(blc_size_h==16 && blc_size_v==8)  // 16x8 : code_number 1
      return 1;
    else
      if(blc_size_h==8 && blc_size_v==16) // 8x16 : code_number 2
        return 2;
      else
        if(blc_size_h==8 && blc_size_v==8) // 8x8 : code_number 3
          return 3;
        else
          if(blc_size_h==8 && blc_size_v==4)  // 8x4 : code_number 4
            return 4;
          else
            if(blc_size_h==4 && blc_size_v==8) // 4x8 : code_number 5
              return 5;
            else  // 4x4 : code_number 6
              return 6;

}

/*!
 ************************************************************************
 * \brief
 *    select intra, forward, backward, bidirectional, direct mode
 ************************************************************************
 */
int motion_search_Bframe(int tot_intra_sad)
{
  int fw_sad, bw_sad, bid_sad, dir_sad;
  int fw_predframe_no;

  fw_predframe_no=get_fwMV(&fw_sad, tot_intra_sad);

  get_bwMV(&bw_sad);

  get_bid(&bid_sad, fw_predframe_no);

  get_dir(&dir_sad);


  compare_sad(tot_intra_sad, fw_sad, bw_sad, bid_sad, dir_sad);

  return fw_predframe_no;
}


void get_bid(int *bid_sad, int fw_predframe_no)
{
  int mb_y,mb_x, block_y, block_x, pic_pix_y, pic_pix_x, pic_block_y, pic_block_x;
  int i, j, ii4, jj4, iii4, jjj4, i2, j2;
  int fw_pred, bw_pred, bid_pred[4][4];
  int code_num, step_h, step_v, mvd_x, mvd_y;

  // consider code number of fw_predframe_no
  *bid_sad = QP2QUANT[img->qp]*min(fw_predframe_no,1)*2;

  // consider bits of fw_blk_size
  if(img->fw_blc_size_h==16 && img->fw_blc_size_v==16)      // 16x16 : blocktype 1
    code_num=0;
  else if(img->fw_blc_size_h==16 && img->fw_blc_size_v==8)   // 16x8 : blocktype 2
    code_num=1;
  else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==16)  // 8x16 : blocktype 3
    code_num=2;
  else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==8)  // 8x8 : blocktype 4
    code_num=3;
  else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==4)   // 8x4 : blocktype 5
    code_num=4;
  else if(img->fw_blc_size_h==4 && img->fw_blc_size_v==8)   // 4x8 : blocktype 6
    code_num=5;
  else        // 4x4 : blocktype 7
    code_num=6;
  *bid_sad += QP2QUANT[img->qp]*img->blk_bituse[code_num];

  // consider bits of bw_blk_size
  if(img->bw_blc_size_h==16 && img->bw_blc_size_v==16)      // 16x16 : blocktype 1
    code_num=0;
  else if(img->bw_blc_size_h==16 && img->bw_blc_size_v==8)   // 16x8 : blocktype 2
    code_num=1;
  else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==16)  // 8x16 : blocktype 3
    code_num=2;
  else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==8)  // 8x8 : blocktype 4
    code_num=3;
  else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==4)   // 8x4 : blocktype 5
    code_num=4;
  else if(img->bw_blc_size_h==4 && img->bw_blc_size_v==8)   // 4x8 : blocktype 6
    code_num=5;
  else        // 4x4 : blocktype 7
    code_num=6;
  *bid_sad += QP2QUANT[img->qp]*img->blk_bituse[code_num];

  // consider bits of mvdfw
  step_h=img->fw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize
  step_v=img->fw_blc_size_v/BLOCK_SIZE;  // vertical stepsize

  for (j=0; j < BLOCK_SIZE; j += step_v)
  {
    for (i=0;i < BLOCK_SIZE; i += step_h)
    {
      if(img->fw_blc_size_h==16 && img->fw_blc_size_v==16)
      {      // 16x16 : blocktype 1
        mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][0];
        mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][1];
      }
      else if(img->fw_blc_size_h==16 && img->fw_blc_size_v==8)
      {  // 16x8 : blocktype 2
        mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][2][0];
        mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][2][1];
      }
      else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==16)
      { // 8x16 : blocktype 3
        mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][3][0];
        mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][3][1];
      }
      else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==8)
      { // 8x8 : blocktype 4
        mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][4][0];
        mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][4][1];
      }
      else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==4)
      {  // 8x4 : blocktype 5
        mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][5][0];
        mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][5][1];
      }
      else if(img->fw_blc_size_h==4 && img->fw_blc_size_v==8)
      {  // 4x8 : blocktype 6
        mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][6][0];
        mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][6][1];
      }
      else
      {                                                      // 4x4 : blocktype 7
        mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][7][0];
        mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][7][1];
      }
      *bid_sad += QP2QUANT[img->qp]*(img->mv_bituse[absm(mvd_x)]+img->mv_bituse[absm(mvd_y)]);
    }
  }

  // consider bits of mvdbw
  step_h=img->bw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize
  step_v=img->bw_blc_size_v/BLOCK_SIZE;  // vertical stepsize

  for (j=0; j < BLOCK_SIZE; j += step_v)
  {
    for (i=0;i < BLOCK_SIZE; i += step_h)
    {
      if(img->bw_blc_size_h==16 && img->bw_blc_size_v==16)
      {      // 16x16 : blocktype 1
        mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][0];
        mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][1];
      }
      else if(img->bw_blc_size_h==16 && img->bw_blc_size_v==8)
      {  // 16x8 : blocktype 2
        mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][2][0];
        mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][2][1];
      }
      else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==16)
      { // 8x16 : blocktype 3
        mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][3][0];
        mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][3][1];
      }
      else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==8)
      { // 8x8 : blocktype 4
        mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][4][0];
        mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][4][1];
      }
      else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==4)
      {  // 8x4 : blocktype 5
        mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][5][0];
        mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][5][1];
      }
      else if(img->bw_blc_size_h==4 && img->bw_blc_size_v==8)
      {  // 4x8 : blocktype 6
        mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][6][0];
        mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][6][1];
      }
      else
      {                                                      // 4x4 : blocktype 7
        mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][7][0];
        mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][7][1];
      }
      *bid_sad += QP2QUANT[img->qp]*(img->mv_bituse[absm(mvd_x)]+img->mv_bituse[absm(mvd_y)]);
    }
  }

  for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
  {
    for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
    {
      for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
      {
        pic_pix_y=img->pix_y+block_y;
        pic_block_y=pic_pix_y/BLOCK_SIZE;

        for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
        {
          pic_pix_x=img->pix_x+block_x;
          pic_block_x=pic_pix_x/BLOCK_SIZE;

          if(input->mv_res)
          {
            ii4=(img->pix_x+block_x)*8+tmp_fwMV[0][pic_block_y][pic_block_x+4];
            jj4=(img->pix_y+block_y)*8+tmp_fwMV[1][pic_block_y][pic_block_x+4];
            iii4=(img->pix_x+block_x)*8+tmp_bwMV[0][pic_block_y][pic_block_x+4];
            jjj4=(img->pix_y+block_y)*8+tmp_bwMV[1][pic_block_y][pic_block_x+4];


            for (j=0;j<4;j++)
            {
              j2=j*8;
              for (i=0;i<4;i++)
              {
                i2=i*8;
                fw_pred = UMVPelY_18 (mref[img->fw_multframe_no],jj4+j2,ii4+i2);
                bw_pred = UMVPelY_18 (mref_P, jjj4+j2, iii4+i2);

                bid_pred[i][j]=(int)((fw_pred+bw_pred)/2.0+0.5);
              }
            }
          }
          else
          {
            ii4=(img->pix_x+block_x)*4+tmp_fwMV[0][pic_block_y][pic_block_x+4];
            jj4=(img->pix_y+block_y)*4+tmp_fwMV[1][pic_block_y][pic_block_x+4];
            iii4=(img->pix_x+block_x)*4+tmp_bwMV[0][pic_block_y][pic_block_x+4];
            jjj4=(img->pix_y+block_y)*4+tmp_bwMV[1][pic_block_y][pic_block_x+4];

            for (j=0;j<4;j++)
            {
              j2=j*4;
              for (i=0;i<4;i++)
              {
                i2=i*4;
                fw_pred=UMVPelY_14 (mref[img->fw_multframe_no], jj4+j2, ii4+i2);
                bw_pred=UMVPelY_14 (mref_P, jjj4+j2, iii4+i2);

                bid_pred[i][j]=(int)((fw_pred+bw_pred)/2.0+0.5);
              }
            }
          }

          for (j=0; j < BLOCK_SIZE; j++)
          {
            for (i=0; i < BLOCK_SIZE; i++)
            {
              img->m7[i][j]=imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-bid_pred[i][j];
            }
          }
          *bid_sad += find_sad(input->hadamard, img->m7);
        }
      }
    }
  }
}

void get_dir(int *dir_sad)
{
  int mb_y,mb_x, block_y, block_x, pic_pix_y, pic_pix_x, pic_block_y, pic_block_x;
  int i, j, ii4, jj4, iii4, jjj4, i2, j2, hv;
  int ref_inx, df_pred, db_pred, dir_pred[4][4];
  int refP_tr, TRb, TRp;

  // initialize with bias value
  *dir_sad=-QP2QUANT[img->qp] * 16;

  // create dfMV, dbMV
  for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
  {
    for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
    {
      for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
      {
        pic_pix_y=img->pix_y+block_y;
        pic_block_y=pic_pix_y/BLOCK_SIZE;

        for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
        {
          pic_pix_x=img->pix_x+block_x;
          pic_block_x=pic_pix_x/BLOCK_SIZE;

          // next P is intra mode
          if(refFrArr[pic_block_y][pic_block_x]==-1)
          {
            for(hv=0; hv<2; hv++)
            {
              dfMV[hv][pic_block_y][pic_block_x+4]=dbMV[hv][pic_block_y][pic_block_x+4]=0;
            }
          }
          // next P is skip or inter mode
          else
          {
#ifdef _ADAPT_LAST_GROUP_
            refP_tr=last_P_no [refFrArr[pic_block_y][pic_block_x]];
#else
            refP_tr=nextP_tr - ((refFrArr[pic_block_y][pic_block_x]+1)*img->p_interval);
#endif
            TRb=img->tr-refP_tr;
            TRp=nextP_tr-refP_tr;
            for(hv=0; hv<2; hv++)
            {
              dfMV[hv][pic_block_y][pic_block_x+4]=TRb*tmp_mv[hv][pic_block_y][pic_block_x+4]/TRp;
              dbMV[hv][pic_block_y][pic_block_x+4]=(TRb-TRp)*tmp_mv[hv][pic_block_y][pic_block_x+4]/TRp;
            }
          }
        }
      }
    }
  }

  // prediction
  for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
  {
    for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
    {
      for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
      {
        pic_pix_y=img->pix_y+block_y;
        pic_block_y=pic_pix_y/BLOCK_SIZE;

        for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
        {
          pic_pix_x=img->pix_x+block_x;
          pic_block_x=pic_pix_x/BLOCK_SIZE;

          if(input->mv_res)
          {
            ii4=(img->pix_x+block_x)*8+dfMV[0][pic_block_y][pic_block_x+4];
            jj4=(img->pix_y+block_y)*8+dfMV[1][pic_block_y][pic_block_x+4];
            iii4=(img->pix_x+block_x)*8+dbMV[0][pic_block_y][pic_block_x+4];
            jjj4=(img->pix_y+block_y)*8+dbMV[1][pic_block_y][pic_block_x+4];

            {
              // next P is intra mode
              if(refFrArr[pic_block_y][pic_block_x]==-1)
                ref_inx=(img->number-1)%img->buf_cycle;
              // next P is skip or inter mode
              else
                ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%img->buf_cycle;

              for (j=0;j<4;j++)
              {
                j2=j*8;
                for (i=0;i<4;i++)
                {
                  i2=i*8;
                  df_pred = UMVPelY_18 (mref[ref_inx], jj4 +j2,  ii4+i2);
                  db_pred = UMVPelY_18 (mref_P,        jjj4+j2, iii4+i2);

                  dir_pred[i][j]=(int)((df_pred+db_pred)/2.0+0.5);
                }
              }

              for (j=0; j < BLOCK_SIZE; j++)
              {
                for (i=0; i < BLOCK_SIZE; i++)
                {
                  img->m7[i][j]=imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-dir_pred[i][j];
                }
              }
              *dir_sad += find_sad(input->hadamard, img->m7);
            }
          }
          else
          {
            ii4=(img->pix_x+block_x)*4+dfMV[0][pic_block_y][pic_block_x+4];
            jj4=(img->pix_y+block_y)*4+dfMV[1][pic_block_y][pic_block_x+4];
            iii4=(img->pix_x+block_x)*4+dbMV[0][pic_block_y][pic_block_x+4];
            jjj4=(img->pix_y+block_y)*4+dbMV[1][pic_block_y][pic_block_x+4];

            {
              // next P is intra mode
              if(refFrArr[pic_block_y][pic_block_x]==-1)
                ref_inx=(img->number-1)%img->buf_cycle;
              // next P is skip or inter mode
              else
                ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%img->buf_cycle;

              for (j=0;j<4;j++)
              {
                j2=j*4;
                for (i=0;i<4;i++)
                {
                  i2=i*4;
                  df_pred=UMVPelY_14 (mref[ref_inx], jj4+j2, ii4+i2);
                  db_pred=UMVPelY_14 (mref_P, jjj4+j2, iii4+i2);

                  dir_pred[i][j]=(int)((df_pred+db_pred)/2.0+0.5);
                }
              }

              for (j=0; j < BLOCK_SIZE; j++)
              {
                for (i=0; i < BLOCK_SIZE; i++)
                {
                  img->m7[i][j]=imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-dir_pred[i][j];
                }
              }
              *dir_sad += find_sad(input->hadamard, img->m7);
            } // else
          } // else

        } // block_x
      } // block_y
    } // mb_x
  } // mb_y
}

void compare_sad(int tot_intra_sad, int fw_sad, int bw_sad, int bid_sad, int dir_sad)
{
  int hv, i, j;
  int InterIntraSave ;

  InterIntraSave = img->mb_data[img->current_mb_nr].intraOrInter ;
  img->mb_data[img->current_mb_nr].intraOrInter = INTER_MB ;

  // LG : dfMV, dbMV reset
  if( (dir_sad<=tot_intra_sad) && (dir_sad<=fw_sad) && (dir_sad<=bw_sad) && (dir_sad<=bid_sad) )
  {
    img->imod = B_Direct;
    img->mb_mode = 0;
    for(hv=0; hv<2; hv++)
      for(i=0; i<4; i++)
        for(j=0; j<4; j++)
          tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=
          tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=0;
  }
  else if( (bw_sad<=tot_intra_sad) && (bw_sad<=fw_sad) && (bw_sad<=bid_sad) && (bw_sad<=dir_sad) )
  {
    img->imod = B_Backward;
    img->mb_mode = img->bw_mb_mode;
    for(hv=0; hv<2; hv++)
      for(i=0; i<4; i++)
        for(j=0; j<4; j++)
          tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=
          dfMV[hv][img->block_y+j][img->block_x+i+4]=
          dbMV[hv][img->block_y+j][img->block_x+i+4]=0;
  }
  else if( (fw_sad<=tot_intra_sad) && (fw_sad<=bw_sad) && (fw_sad<=bid_sad) && (fw_sad<=dir_sad) )
  {
    img->imod = B_Forward;
    img->mb_mode = img->fw_mb_mode;
    for(hv=0; hv<2; hv++)
      for(i=0; i<4; i++)
        for(j=0; j<4; j++)
          tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=
          dfMV[hv][img->block_y+j][img->block_x+i+4]=
          dbMV[hv][img->block_y+j][img->block_x+i+4]=0;
  }
  else if( (bid_sad<=tot_intra_sad) && (bid_sad<=fw_sad) && (bid_sad<=bw_sad) && (bid_sad<=dir_sad) )
  {
    img->imod = B_Bidirect;
    img->mb_mode = 3;
    for(hv=0; hv<2; hv++)
      for(i=0; i<4; i++)
        for(j=0; j<4; j++)
          dfMV[hv][img->block_y+j][img->block_x+i+4]=
          dbMV[hv][img->block_y+j][img->block_x+i+4]=0;
  }
  else if( (tot_intra_sad<=dir_sad) && (tot_intra_sad<=bw_sad) && (tot_intra_sad<=fw_sad) && (tot_intra_sad<=bid_sad) )
  {
    img->mb_mode=img->imod+8*img->type; // img->type=2
    img->mb_data[img->current_mb_nr].intraOrInter  =  InterIntraSave;

    for(hv=0; hv<2; hv++)
      for(i=0; i<4; i++)
        for(j=0; j<4; j++)
          tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=
          tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=
          dfMV[hv][img->block_y+j][img->block_x+i+4]=
          dbMV[hv][img->block_y+j][img->block_x+i+4]=0;
  }
}

