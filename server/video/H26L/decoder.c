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
 * \file decoder.c
 *
 * \brief
 *    Contains functions that implement the "decoders in the encoder" concept for the
 *    rate-distortion optimization with losses.
 * \date
 *    October 22nd, 2001
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and 
 *    affiliation details)
 *    - Dimitrios Kontopodis                    <dkonto@eikon.tum.de>
 *************************************************************************************
 */

#include <stdlib.h>
#include <memory.h>

#include "global.h"
#include "refbuf.h"
#include "rdopt.h"

/*! 
 *************************************************************************************
 * \brief
 *    decodes one macroblock at one simulated decoder 
 *
 * \param decoder
 *    The id of the decoder
 *
 * \param mode
 *    encoding mode of the MB
 *
 * \param ref
 *    reference frame index
 *
 * \note
 *    Gives the expected value in the decoder of one MB. This is done based on the 
 *    stored reconstructed residue resY[][], the reconstructed values imgY[][]
 *    and the motion vectors. The decoded MB is moved to decY[][].
 *************************************************************************************
 */
void decode_one_macroblock(int decoder, int mode, int ref)
{
  int i,j,block_y,block_x;
  int ref_inx;
  int mv[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE];
  int resY_tmp[MB_BLOCK_SIZE][MB_BLOCK_SIZE];

  int inter = (mode >= MBMODE_INTER16x16    && mode <= MBMODE_INTER4x4    && img->type!=B_IMG);

  if (img->number==0)
  {
    for(i=0;i<MB_BLOCK_SIZE;i++)
      for(j=0;j<MB_BLOCK_SIZE;j++)
        decY[decoder][img->pix_y+j][img->pix_x+i]=imgY[img->pix_y+j][img->pix_x+i];
  }
  else
  {
    if (mode==MBMODE_COPY)
    {
      for(i=0;i<MB_BLOCK_SIZE;i++)
        for(j=0;j<MB_BLOCK_SIZE;j++)
          resY_tmp[j][i]=0;

      /* Set motion vectors to zero */
      for (block_y=0; block_y<BLOCK_MULTIPLE; block_y++)
        for (block_x=0; block_x<BLOCK_MULTIPLE; block_x++)
          for (i=0;i<2;i++)
            mv[i][block_y][block_x]=0;

    }
    else
    {
    /* Copy motion vectors and residues to local arrays mv, resY_tmp, resUV_tmp */
      for (block_y=0; block_y<BLOCK_MULTIPLE; block_y++)
        for (block_x=0; block_x<BLOCK_MULTIPLE; block_x++)
          for (i=0;i<2;i++)
            mv[i][block_y][block_x]=tmp_mv[i][img->block_y+block_y][img->block_x+block_x+4];
          
      for(i=0;i<MB_BLOCK_SIZE;i++)
        for(j=0;j<MB_BLOCK_SIZE;j++)
          resY_tmp[j][i]=resY[j][i];
    }
  
    
    /* Decode Luminance */
    if (inter || mode==MBMODE_COPY)  
    {
      for (block_y=img->block_y ; block_y < img->block_y+BLOCK_MULTIPLE ; block_y++)
        for (block_x=img->block_x ; block_x < img->block_x+BLOCK_MULTIPLE ; block_x++)
        {

          ref_inx = (img->number-ref-1)%img->no_multpred;
          
          Get_Reference_Block(decref[decoder][ref_inx],
                                block_y, block_x,
                                mv[0][block_y-img->block_y][block_x-img->block_x],
                                mv[1][block_y-img->block_y][block_x-img->block_x], 
                                RefBlock);
          for (j=0;j<BLOCK_SIZE;j++)
            for (i=0;i<BLOCK_SIZE;i++)
            {
              if (RefBlock[j][i] != UMVPelY_14 (mref[ref_inx],
                                                (block_y*4+j)*4+mv[1][block_y-img->block_y][block_x-img->block_x],
                                                (block_x*4+i)*4+mv[0][block_y-img->block_y][block_x-img->block_x]))
              ref_inx = (img->number-ref-1)%img->no_multpred;
              decY[decoder][block_y*BLOCK_SIZE + j][block_x*BLOCK_SIZE + i] = 
                                  resY_tmp[(block_y-img->block_y)*BLOCK_SIZE + j]
                                          [(block_x-img->block_x)*BLOCK_SIZE + i]
                                  + RefBlock[j][i];
            }
        }
    }
    else 
    {
      /* Intra Refresh - Assume no spatial prediction */
      for (j=0;j<MB_BLOCK_SIZE;j++)
        for (i=0;i<MB_BLOCK_SIZE;i++)
          decY[decoder][img->pix_y + j][img->pix_x + i] = imgY[img->pix_y + j][img->pix_x + i];
    }
  }
}


/*! 
 *************************************************************************************
 * \brief
 *    Finds the reference MB given the decoded reference frame
 * \note
 *    This is based on the function UnifiedOneForthPix, only it is modified to
 *    be used at the "many decoders in the encoder" RD optimization. In this case
 *    we dont want to keep full upsampled reference frames for all decoders, so
 *    we just upsample when it is necessary.
 * \param imY
 *    The frame to be upsampled
 * \param block_y
 *    The row of the block, whose prediction we want to find
 * \param block_x
 *    The column of the block, whose prediction we want to track
 * \param mvhor
 *    Motion vector, horizontal part
 * \param mvver
 *    Motion vector, vertical part
 * \param out
 *    Output: The prediction for the block (block_y, block_x)
 *************************************************************************************
 */
void Get_Reference_Block(byte **imY, 
                         int block_y, 
                         int block_x, 
                         int mvhor, 
                         int mvver, 
                         byte **out)
{
  int i,j,y,x;

  y = block_y * BLOCK_SIZE * 4 + mvver;
  x = block_x * BLOCK_SIZE * 4 + mvhor;

  for (j=0; j<BLOCK_SIZE; j++)
    for (i=0; i<BLOCK_SIZE; i++)
      out[j][i] = Get_Reference_Pixel(imY, 
                  max(0,min(img->mvert, y+j*4)), 
                  max(0,min(img->mhor, x+i*4)));
}

/*! 
 *************************************************************************************
 * \brief
 *    Finds a pixel (y,x) of the upsampled reference frame
 * \note
 *    This is based on the function UnifiedOneForthPix, only it is modified to
 *    be used at the "many decoders in the encoder" RD optimization. In this case
 *    we dont want to keep full upsampled reference frames for all decoders, so
 *    we just upsample when it is necessary.
 *************************************************************************************
 */
byte Get_Reference_Pixel(byte **imY, int y_pos, int x_pos)
{

  int dx, x;
  int dy, y;
  int maxold_x,maxold_y;

  int result = 0, result1, result2;
  int pres_x;
  int pres_y; 

  int tmp_res[6];

  static const int COEF[6] = {
    1, -5, 20, 20, -5, 1
  };


  dx = x_pos&3;
  dy = y_pos&3;
  x_pos = (x_pos-dx)/4;
  y_pos = (y_pos-dy)/4;
  maxold_x = img->width-1;
  maxold_y = img->height-1;

  if (dx == 0 && dy == 0) { /* fullpel position */
    result = imY[max(0,min(maxold_y,y_pos))][max(0,min(maxold_x,x_pos))];
  }
  else if (dx == 3 && dy == 3) { /* funny position */
    result = (imY[max(0,min(maxold_y,y_pos))  ][max(0,min(maxold_x,x_pos))  ]+
              imY[max(0,min(maxold_y,y_pos))  ][max(0,min(maxold_x,x_pos+1))]+
              imY[max(0,min(maxold_y,y_pos+1))][max(0,min(maxold_x,x_pos+1))]+
              imY[max(0,min(maxold_y,y_pos+1))][max(0,min(maxold_x,x_pos))  ]+2)/4;
  }
  else { /* other positions */

    if (dy == 0) {

      pres_y = max(0,min(maxold_y,y_pos));
      for(x=-2;x<4;x++) {
        pres_x = max(0,min(maxold_x,x_pos+x));
        result += imY[pres_y][pres_x]*COEF[x+2];
      }

      result = max(0, min(255, (result+16)/32));

      if (dx == 1) {
        result = (result + imY[pres_y][max(0,min(maxold_x,x_pos))])/2;
      }
      else if (dx == 3) {
        result = (result + imY[pres_y][max(0,min(maxold_x,x_pos+1))])/2;
      }
    }
    else if (dx == 0) {

      pres_x = max(0,min(maxold_x,x_pos));
      for(y=-2;y<4;y++) {
        pres_y = max(0,min(maxold_y,y_pos+y));
        result += imY[pres_y][pres_x]*COEF[y+2];
      }

      result = max(0, min(255, (result+16)/32));

      if (dy == 1) {
        result = (result + imY[max(0,min(maxold_y,y_pos))][pres_x])/2;
      }
      else if (dy == 3) {
        result = (result + imY[max(0,min(maxold_y,y_pos+1))][pres_x])/2;
      }
    }
    else if (dx == 2) {

      for(y=-2;y<4;y++) {
        result = 0;
        pres_y = max(0,min(maxold_y,y_pos+y));
        for(x=-2;x<4;x++) {
          pres_x = max(0,min(maxold_x,x_pos+x));
          result += imY[pres_y][pres_x]*COEF[x+2];
        }
        tmp_res[y+2] = result;
      }

      result = 0;
      for(y=-2;y<4;y++) {
        result += tmp_res[y+2]*COEF[y+2];
      }

      result = max(0, min(255, (result+512)/1024));

      if (dy == 1) {
        result = (result + max(0, min(255, (tmp_res[2]+16)/32)))/2;
      }
      else if (dy == 3) {
        result = (result + max(0, min(255, (tmp_res[3]+16)/32)))/2;
      }
    }
    else if (dy == 2) {

      for(x=-2;x<4;x++) {
        result = 0;
        pres_x = max(0,min(maxold_x,x_pos+x));
        for(y=-2;y<4;y++) {
          pres_y = max(0,min(maxold_y,y_pos+y));
          result += imY[pres_y][pres_x]*COEF[y+2];
        }
        tmp_res[x+2] = result;
      }

      result = 0;
      for(x=-2;x<4;x++) {
        result += tmp_res[x+2]*COEF[x+2];
      }

      result = max(0, min(255, (result+512)/1024));

      if (dx == 1) {
        result = (result + max(0, min(255, (tmp_res[2]+16)/32)))/2;
      }
      else {
        result = (result + max(0, min(255, (tmp_res[3]+16)/32)))/2;
      }
    }
    else {

      result = 0;
      pres_y = dy == 1 ? y_pos : y_pos+1;
      pres_y = max(0,min(maxold_y,pres_y));

      for(x=-2;x<4;x++) {
        pres_x = max(0,min(maxold_x,x_pos+x));
        result += imY[pres_y][pres_x]*COEF[x+2];
      }

      result1 = max(0, min(255, (result+16)/32));

      result = 0;
      pres_x = dx == 1 ? x_pos : x_pos+1;
      pres_x = max(0,min(maxold_x,pres_x));

      for(y=-2;y<4;y++) {
        pres_y = max(0,min(maxold_y,y_pos+y));
        result += imY[pres_y][pres_x]*COEF[y+2];
      }

      result2 = max(0, min(255, (result+16)/32));
      result = (result1+result2)/2;
    }
  }

  return result;
}
  
/*! 
 *************************************************************************************
 * \brief
 *    Performs the simulation of the packet losses, calls the error concealment funcs
 *    and copies the decoded images to the reference frame buffers of the decoders 
 *
 *************************************************************************************
 */
void UpdateDecoders()
{
  int k;
  for (k=0; k<input->NoOfDecoders; k++)
  {
    Build_Status_Map(status_map); // simulates the packet losses
    Error_Concealment(decY_best[k], status_map, decref[k]); // for the moment error concealment is just a "copy"
    // Move decoded frames to reference buffers: (at the decoders this is done 
    // without interpolation (upsampling) - upsampling is done while decoding
    DecOneForthPix(decY_best[k], decref[k]); 
  }
}
/*! 
 *************************************************************************************
 * \brief
 *    Copies one (reconstructed) image to the respective reference frame buffer
 *
 * \note
 *    This is used at the "many decoders in the encoder"
 * \param dY
 *    The reconstructed image
 * \param dref
 *    The reference buffer
 *************************************************************************************
 */
void DecOneForthPix(byte **dY, byte ***dref)
{
  int j, ref=img->number%img->buf_cycle;

  for (j=0; j<img->height; j++)
    memcpy(dref[ref][j], dY[j], img->width);
}

/*! 
 *************************************************************************************
 * \brief
 *    Gives the prediction residue for this MB.
 *
 * \param mode
 *    The encoding mode of this MB.
 *************************************************************************************
 */
void compute_residue(int mode)
{
  int i,j;
  if (mode == MBMODE_INTRA16x16)  /* Intra 16 MB*/
    for (i=0; i<MB_BLOCK_SIZE; i++)
      for (j=0; j<MB_BLOCK_SIZE; j++)
        resY[j][i] = imgY[img->pix_y+j][img->pix_x+i]-img->mprr_2[mode][j][i]; /* SOS: Is this correct? or maybe mprr_2[][i][j] ??*/
  else
    for (i=0; i<MB_BLOCK_SIZE; i++)  /* Inter MB */
      for (j=0; j<MB_BLOCK_SIZE; j++)
        resY[j][i] = imgY[img->pix_y+j][img->pix_x+i]-img->mpr[i][j];
}

/*! 
 *************************************************************************************
 * \brief
 *    Builds a random status map showing whether each MB is received or lost, based
 *    on the packet loss rate and the slice structure.
 *
 * \param s_map
 *    The status map to be filled
 *************************************************************************************
 */
void Build_Status_Map(byte **s_map)
{
  int i,j,slice=-1,mb=0,jj,ii,packet_lost;

  jj = img->height/MB_BLOCK_SIZE;
  ii = img->width/MB_BLOCK_SIZE;

  for (j=0 ; j<jj; j++)
    for (i=0 ; i<ii; i++)
    {
      if (!input->slice_mode || img->mb_data[mb].slice_nr != slice) /* new slice */
      {
        if ((double)rand()/(double)RAND_MAX*100 < input->LossRate)
          packet_lost=1;
        else
          packet_lost=0;
        
        slice++;
      }
      if (packet_lost)
        s_map[j][i]=0;
      else
        s_map[j][i]=1;
      mb++;
    }
}

/*! 
 *************************************************************************************
 * \brief
 *    Performs some sort of error concealment for the areas that are lost according
 *    to the status_map
 *    
 * \param inY
 *    Error concealment is performed on this frame imY[][]
 * \param s_map
 *    The status map shows which areas are lost.
 * \param refY
 *    The set of reference frames - may be used for the error concealment.
 *************************************************************************************
 */
void Error_Concealment(byte **inY, byte **s_map, byte ***refY)
{
  int mb_y, mb_x, mb_h, mb_w;
  mb_h = img->height/MB_BLOCK_SIZE;
  mb_w = img->width/MB_BLOCK_SIZE;
  
  for (mb_y=0; mb_y < mb_h; mb_y++)
    for (mb_x=0; mb_x < mb_w; mb_x++)
      if (!s_map[mb_y][mb_x])
        Conceal_Error(inY, mb_y, mb_x, refY);
}

/*! 
 *************************************************************************************
 * \brief
 *    Copies a certain MB (mb_y,mb_x) of the frame inY[][] from the previous frame.
 *    For the time there is no better EC...
 *************************************************************************************
 */
void Conceal_Error(byte **inY, int mb_y, int mb_x, byte ***refY)
{
  int i,j;
  int ref_inx = (img->number-1)%img->no_multpred;
  int pos_y = mb_y*MB_BLOCK_SIZE, pos_x = mb_x*MB_BLOCK_SIZE;
  if (img->number)
  {
    for (j=0;j<MB_BLOCK_SIZE;j++)
      for (i=0;i<MB_BLOCK_SIZE;i++)
        inY[pos_y+j][pos_x+i] = refY[ref_inx][pos_y+j][pos_x+i];
  }
  else
  {
    for (j=0;j<MB_BLOCK_SIZE;j++)
      for (i=0;i<MB_BLOCK_SIZE;i++)
        inY[pos_y+j][pos_x+i] = 127;
  }
}
