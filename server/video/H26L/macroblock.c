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
 * \file macroblock.c
 *
 * \brief
 *    Process one macroblock
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *    - Jani Lainema                    <jani.lainema@nokia.com>
 *    - Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *    - Ragip Kurceren                  <ragip.kurceren@nokia.com>
 *************************************************************************************
 */
#include "contributors.h"

#include <math.h>
#include <stdlib.h>

#include "elements.h"
#include "macroblock.h"
#include "refbuf.h"


/*!
 ************************************************************************
 * \brief
 *    updates the coordinates and statistics parameter for the
 *    next macroblock
 ************************************************************************
 */
void proceed2nextMacroblock()
{
#if TRACE
  int use_bitstream_backing = (input->slice_mode == FIXED_RATE || input->slice_mode == CALLBACK);
#endif
  const int number_mb_per_row = img->width / MB_BLOCK_SIZE ;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

#if TRACE
  int i;
  if (p_trace)
  {
    if(use_bitstream_backing)
      fprintf(p_trace, "\n*********** Pic: %i (I/P) MB: %i Slice: %i **********\n\n", frame_no, img->current_mb_nr, img->current_slice_nr);
   // Write out the tracestring for each symbol
     for (i=0; i<currMB->currSEnr; i++)
       trace2out(&(img->MB_SyntaxElements[i]));
  }
#endif

  // Update the statistics
  stat->bit_use_mb_type[img->type]      += currMB->bitcounter[BITS_MB_MODE];
  stat->bit_use_coeffY[img->type]       += currMB->bitcounter[BITS_COEFF_Y_MB] ;
  stat->bit_use_mode_inter[img->mb_mode]+= currMB->bitcounter[BITS_INTER_MB];
  stat->tmp_bit_use_cbp[img->type]      += currMB->bitcounter[BITS_CBP_MB];
  stat->bit_use_coeffC[img->type]       += currMB->bitcounter[BITS_COEFF_UV_MB];
  stat->bit_use_delta_quant[img->type]  += currMB->bitcounter[BITS_DELTA_QUANT_MB];

/*  if (input->symbol_mode == UVLC)
    stat->bit_ctr += currMB->bitcounter[BITS_TOTAL_MB]; */
  if (img->type==INTRA_IMG)
    ++stat->mode_use_intra[img->mb_mode];
  else
    if (img->type != B_IMG)
      ++stat->mode_use_inter[img->mb_mode];
    else
      ++stat->mode_use_Bframe[img->mb_mode];

  // Update coordinates of macroblock
  img->mb_x++;
  if (img->mb_x == number_mb_per_row) // next row of MBs
  {
    img->mb_x = 0; // start processing of next row
    img->mb_y++;
  }
  img->current_mb_nr++;


  // Define vertical positions
  img->block_y = img->mb_y * BLOCK_SIZE;      // vertical luma block position
  img->pix_y   = img->mb_y * MB_BLOCK_SIZE;   // vertical luma macroblock position
  img->pix_c_y = img->mb_y * MB_BLOCK_SIZE/2; // vertical chroma macroblock position

  if (img->type != B_IMG)
  {
    if (input->intra_upd > 0 && img->mb_y <= img->mb_y_intra)
      img->height_err=(img->mb_y_intra*16)+15;     // for extra intra MB
    else
      img->height_err=img->height-1;
  }

  // Define horizontal positions
  img->block_x = img->mb_x * BLOCK_SIZE;        // luma block
  img->pix_x   = img->mb_x * MB_BLOCK_SIZE;     // luma pixel
  img->block_c_x = img->mb_x * BLOCK_SIZE/2;    // chroma block
  img->pix_c_x   = img->mb_x * MB_BLOCK_SIZE/2; // chroma pixel

  // Statistics
  if ((img->type == INTER_IMG)||(img->types==SP_IMG) )
  {
    ++stat->quant0;
    stat->quant1 += img->qp;      // to find average quant for inter frames
  }
}

/*!
 ************************************************************************
 * \brief
 *    initializes the current macroblock
 ************************************************************************
 */
void start_macroblock()
{
  int i,j,k,l;
  int x=0, y=0 ;
  int use_bitstream_backing = (input->slice_mode == FIXED_RATE || input->slice_mode == CALLBACK);
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];
  Slice *curr_slice = img->currentSlice;
  DataPartition *dataPart;
  Bitstream *currStream;
  EncodingEnvironmentPtr eep;

  if(use_bitstream_backing)
  {
     // Save image to allow recoding if necessary
     for(y=0; y<img->height; y++)
       for(x=0; x<img->width; x++)
         imgY_tmp[y][x] = imgY[y][x];
     for(i=0; i<2; i++)
       for(y=0; y<img->height_cr; y++)
         for(x=0; x<img->width_cr; x++)
           imgUV_tmp[i][y][x] = imgUV[i][y][x];

    // Keep the current state of the bitstreams
    if(!img->cod_counter)
      for (i=0; i<curr_slice->max_part_nr; i++)
      {
        dataPart = &(curr_slice->partArr[i]);
        currStream = dataPart->bitstream;
        currStream->stored_bits_to_go   = currStream->bits_to_go;
        currStream->stored_byte_pos   = currStream->byte_pos;
        currStream->stored_byte_buf   = currStream->byte_buf;

        if (input->symbol_mode ==CABAC)
        {
          eep = &(dataPart->ee_cabac);
          eep->ElowS            = eep->Elow;
          eep->EhighS           = eep->Ehigh;
          eep->EbufferS         = eep->Ebuffer;
          eep->Ebits_to_goS     = eep->Ebits_to_go;
          eep->Ebits_to_followS = eep->Ebits_to_follow;
          eep->EcodestrmS       = eep->Ecodestrm;
          eep->Ecodestrm_lenS   = eep->Ecodestrm_len;
        }
      }
  }

  // Save the slice number of this macroblock. When the macroblock below
  // is coded it will use this to decide if prediction for above is possible
  img->slice_numbers[img->current_mb_nr] = img->current_slice_nr;

  // Save the slice and macroblock number of the current MB
  currMB->slice_nr = img->current_slice_nr;

    // Initialize delta qp change from last macroblock. Feature may be used for future rate control
  currMB->delta_qp=0;

  // Initialize counter for MB symbols
  currMB->currSEnr=0;

  // If MB is next to a slice boundary, mark neighboring blocks unavailable for prediction
  CheckAvailabilityOfNeighbors(img);

  // Reset vectors before doing motion search in motion_search().
  if (img->type != B_IMG)
  {
    for (k=0; k < 2; k++)
    {
      for (j=0; j < BLOCK_MULTIPLE; j++)
        for (i=0; i < BLOCK_MULTIPLE; i++)
          tmp_mv[k][img->block_y+j][img->block_x+i+4]=0;
    }
  }

  // Reset syntax element entries in MB struct
  currMB->ref_frame = 0;
  currMB->mb_type   = 0;
  currMB->cbp_blk   = 0;
  currMB->cbp       = 0;

  for (l=0; l < 2; l++)
    for (j=0; j < BLOCK_MULTIPLE; j++)
      for (i=0; i < BLOCK_MULTIPLE; i++)
        for (k=0; k < 2; k++)
          currMB->mvd[l][j][i][k] = 0;

  for (i=0; i < (BLOCK_MULTIPLE*BLOCK_MULTIPLE); i++)
    currMB->intra_pred_modes[i] = 0;


  // Initialize bitcounters for this macroblock
  if(img->current_mb_nr == 0) // No slice header to account for
  {
    currMB->bitcounter[BITS_HEADER] = 0;
  }
  else if (img->slice_numbers[img->current_mb_nr] == img->slice_numbers[img->current_mb_nr-1]) // current MB belongs to the
  // same slice as the last MB
  {
    currMB->bitcounter[BITS_HEADER] = 0;
  }

  currMB->bitcounter[BITS_MB_MODE] = 0;
  currMB->bitcounter[BITS_COEFF_Y_MB] = 0;
  currMB->bitcounter[BITS_INTER_MB] = 0;
  currMB->bitcounter[BITS_CBP_MB] = 0;
  currMB->bitcounter[BITS_DELTA_QUANT_MB] = 0;
  currMB->bitcounter[BITS_COEFF_UV_MB] = 0;

#ifdef _FAST_FULL_ME_
  ResetFastFullIntegerSearch ();
#endif
}

/*!
 ************************************************************************
 * \brief
 *    terminates processing of the current macroblock depending
 *    on the chosen slice mode
 ************************************************************************
 */
void terminate_macroblock(Boolean *end_of_slice, Boolean *recode_macroblock)
{
  int i,x=0, y=0 ;
  Slice *currSlice = img->currentSlice;
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int *partMap = assignSE2partition[input->partition_mode];
  DataPartition *dataPart;
  Bitstream *currStream;
  int rlc_bits=0;
  EncodingEnvironmentPtr eep;
  int use_bitstream_backing = (input->slice_mode == FIXED_RATE || input->slice_mode == CALLBACK);
  int new_slice = ( (img->current_mb_nr == 0) || img->mb_data[img->current_mb_nr-1].slice_nr != img->current_slice_nr);
  static int skip = FALSE;

  switch(input->slice_mode)
  {
  case NO_SLICES:
    *recode_macroblock = FALSE;
    if ((img->current_mb_nr+1) == img->total_number_mb) // maximum number of MBs
      *end_of_slice = TRUE;
    break;
  case FIXED_MB:
    // For slice mode one, check if a new slice boundary follows
    *recode_macroblock = FALSE;
    if ( ((img->current_mb_nr+1) % input->slice_argument == 0) || ((img->current_mb_nr+1) == img->total_number_mb) )
    {
      *end_of_slice = TRUE;
    }
    break;

    // For slice modes two and three, check if coding of this macroblock
    // resulted in too many bits for this slice. If so, indicate slice
    // boundary before this macroblock and code the macroblock again
  case FIXED_RATE:
     // in case of skip MBs check if there is a slice boundary
     // only for UVLC (img->cod_counter is always 0 in case of CABAC)
     if(img->cod_counter)
     {
       // write out the skip MBs to know how many bits we need for the RLC
       dataPart = &(currSlice->partArr[partMap[SE_MBTYPE]]);
       currSE->value1 = img->cod_counter;
       currSE->mapping = n_linfo2;
       currSE->type = SE_MBTYPE;
       dataPart->writeSyntaxElement(  currSE, dataPart);
       rlc_bits=currSE->len;

       currStream = dataPart->bitstream;
       // save the bitstream as it would be if we write the skip MBs
       currStream->bits_to_go_skip  = currStream->bits_to_go;
       currStream->byte_pos_skip    = currStream->byte_pos;
       currStream->byte_buf_skip    = currStream->byte_buf;
       // restore the bitstream
       currStream->bits_to_go = currStream->stored_bits_to_go;
       currStream->byte_pos = currStream->stored_byte_pos;
       currStream->byte_buf = currStream->stored_byte_buf;
       skip = TRUE;
     }
     //! Check if the last coded macroblock fits into the size of the slice
     //! But only if this is not the first macroblock of this slice
     if (!new_slice)
     {
       if(slice_too_big(rlc_bits))
       {
         *recode_macroblock = TRUE;
         *end_of_slice = TRUE;
       }
       else if(!img->cod_counter)
         skip = FALSE;
     }
     // maximum number of MBs
     if ((*recode_macroblock == FALSE) && ((img->current_mb_nr+1) == img->total_number_mb)) 
     {
       *end_of_slice = TRUE;
       if(!img->cod_counter)
         skip = FALSE;
     }
   
     //! (first MB OR first MB in a slice) AND bigger that maximum size of slice
     if (new_slice && slice_too_big(rlc_bits))
     {
       *end_of_slice = TRUE;
       if(!img->cod_counter)
         skip = FALSE;
     }
     break;

  case  CALLBACK:
    if (img->current_mb_nr > 0 && !new_slice)
    {
      if (currSlice->slice_too_big(rlc_bits))
      {
        *recode_macroblock = TRUE;
        *end_of_slice = TRUE;
      }
    }
    if ( (*recode_macroblock == FALSE) && ((img->current_mb_nr+1) == img->total_number_mb) ) // maximum number of MBs
      *end_of_slice = TRUE;
    break;
  default:
    snprintf(errortext, ET_SIZE, "Slice Mode %d not supported", input->slice_mode);
    error(errortext, 600);
  }

  if(*recode_macroblock == TRUE)
  {
    // Restore everything
    for (i=0; i<currSlice->max_part_nr; i++)
    {
      dataPart = &(currSlice->partArr[i]);
      currStream = dataPart->bitstream;
      currStream->bits_to_go = currStream->stored_bits_to_go;
      currStream->byte_pos  = currStream->stored_byte_pos;
      currStream->byte_buf  = currStream->stored_byte_buf;
      if (input->symbol_mode == CABAC)
      {
        eep = &(dataPart->ee_cabac);
        eep->Elow            = eep->ElowS;
        eep->Ehigh           = eep->EhighS;
        eep->Ebuffer         = eep->EbufferS;
        eep->Ebits_to_go     = eep->Ebits_to_goS;
        eep->Ebits_to_follow = eep->Ebits_to_followS;
        eep->Ecodestrm       = eep->EcodestrmS;
        eep->Ecodestrm_len   = eep->Ecodestrm_lenS;
      }
    }
    // Restore image to avoid DeblockMB to operate twice
    // Note that this can be simplified! The copy range!
    for(y=0; y<img->height; y++)
      for(x=0; x<img->width; x++)
        imgY[y][x] = imgY_tmp[y][x];
    for(i=0; i<2; i++)
      for(y=0; y<img->height_cr; y++)
        for(x=0; x<img->width_cr; x++)
          imgUV[i][y][x] = imgUV_tmp[i][y][x];
  }

  if(*end_of_slice == TRUE  && skip == TRUE) //! TO 4.11.2001 Skip MBs at the end of this slice
  { 
    //! only for Slice Mode 2 or 3
    // If we still have to write the skip, let's do it!
    if(img->cod_counter && *recode_macroblock == TRUE) //! MB that did not fit in this slice
    { 
      // If recoding is true and we have had skip, 
      // we have to reduce the counter in case of recoding
      img->cod_counter--;
      if(img->cod_counter)
      {
        dataPart = &(currSlice->partArr[partMap[SE_MBTYPE]]);
        currSE->value1 = img->cod_counter;
        currSE->mapping = n_linfo2;
        currSE->type = SE_MBTYPE;
        dataPart->writeSyntaxElement(  currSE, dataPart);
        rlc_bits=currSE->len;
        currMB->bitcounter[BITS_MB_MODE]+=rlc_bits;
        img->cod_counter = 0;
      }
    }
    else //! MB that did not fit in this slice anymore is not a Skip MB
    {
      for (i=0; i<currSlice->max_part_nr; i++)
      {
        dataPart = &(currSlice->partArr[i]);
        currStream = dataPart->bitstream;
        // update the bitstream
        currStream->bits_to_go = currStream->bits_to_go_skip;
        currStream->byte_pos  = currStream->byte_pos_skip;
        currStream->byte_buf  = currStream->byte_buf_skip;
      }
      // update the statistics
      img->cod_counter = 0;
      skip = FALSE;
    }
  }
  
  //! TO 4.11.2001 Skip MBs at the end of this slice for Slice Mode 0 or 1
  if(*end_of_slice == TRUE && img->cod_counter && !use_bitstream_backing)
  {
    dataPart = &(currSlice->partArr[partMap[SE_MBTYPE]]);
    currSE->value1 = img->cod_counter;
    currSE->mapping = n_linfo2;
    currSE->type = SE_MBTYPE;
    dataPart->writeSyntaxElement(  currSE, dataPart);
    rlc_bits=currSE->len;
    currMB->bitcounter[BITS_MB_MODE]+=rlc_bits;
    img->cod_counter = 0;
  }
}

/*!
 *****************************************************************************
 *
 * \brief 
 *    For Slice Mode 2: Checks if one partition of one slice exceeds the 
 *    allowed size
 * 
 * \return
 *    FALSE if all Partitions of this slice are smaller than the allowed size
 *    TRUE is at least one Partition exceeds the limit
 *
 * \para Parameters
 *    
 *    
 *
 * \para Side effects
 *    none
 *
 * \para Other Notes
 *    
 *    
 *
 * \date
 *    4 November 2001
 *
 * \author
 *    Tobias Oelbaum      drehvial@gmx.net
 *****************************************************************************/
 
 int slice_too_big(int rlc_bits)
 {
   Slice *currSlice = img->currentSlice;
   DataPartition *dataPart;
   Bitstream *currStream;
   EncodingEnvironmentPtr eep;
   int i;
   int size_in_bytes;
   
   //! UVLC
   if (input->symbol_mode == UVLC)
   {
     for (i=0; i<currSlice->max_part_nr; i++)
     {
       dataPart = &(currSlice->partArr[i]);
       currStream = dataPart->bitstream;
       size_in_bytes = currStream->byte_pos;
       if (currStream->bits_to_go < 8)
         size_in_bytes++;
       if (currStream->bits_to_go < rlc_bits)
         size_in_bytes++;
       if(size_in_bytes > input->slice_argument)
         return TRUE;
     }
   }
    
   //! CABAC
   if (input->symbol_mode ==CABAC)
   {
     for (i=0; i<currSlice->max_part_nr; i++)
     {
        dataPart= &(currSlice->partArr[i]);
        eep = &(dataPart->ee_cabac);
        if(arienco_bits_written(eep) > (input->slice_argument*8))
          return TRUE;
     }
   }
   return FALSE;
 }
/*!
 ************************************************************************
 * \brief
 *    Checks the availability of neighboring macroblocks of
 *    the current macroblock for prediction and context determination;
 *    marks the unavailable MBs for intra prediction in the
 *    ipredmode-array by -1. Only neighboring MBs in the causal
 *    past of the current MB are checked.
 ************************************************************************
 */
void CheckAvailabilityOfNeighbors()
{
  int i,j;
  const int mb_width = img->width/MB_BLOCK_SIZE;
  const int mb_nr = img->current_mb_nr;
  Macroblock *currMB = &img->mb_data[mb_nr];

  // mark all neighbors as unavailable
  for (i=0; i<3; i++)
    for (j=0; j<3; j++)
      img->mb_data[mb_nr].mb_available[i][j]=NULL;
  img->mb_data[mb_nr].mb_available[1][1]=currMB; // current MB

  // Check MB to the left
  if(img->pix_x >= MB_BLOCK_SIZE)
  {
    int remove_prediction = currMB->slice_nr != img->mb_data[mb_nr-1].slice_nr;
    if(input->UseConstrainedIntraPred)
      remove_prediction = (remove_prediction || img->intra_mb[mb_nr-1] ==0);
    if(remove_prediction)
    {
      img->ipredmode[img->block_x][img->block_y+1] = -1;
      img->ipredmode[img->block_x][img->block_y+2] = -1;
      img->ipredmode[img->block_x][img->block_y+3] = -1;
      img->ipredmode[img->block_x][img->block_y+4] = -1;
    } else
      currMB->mb_available[1][0]=&(img->mb_data[mb_nr-1]);
  }


  // Check MB above
  if(img->pix_y >= MB_BLOCK_SIZE)
  {
    int remove_prediction = currMB->slice_nr != img->mb_data[mb_nr-mb_width].slice_nr;
    if(input->UseConstrainedIntraPred)
      remove_prediction = (remove_prediction || img->intra_mb[mb_nr-mb_width] ==0);
    if(remove_prediction)
    {
      img->ipredmode[img->block_x+1][img->block_y] = -1;
      img->ipredmode[img->block_x+2][img->block_y] = -1;
      img->ipredmode[img->block_x+3][img->block_y] = -1;
      img->ipredmode[img->block_x+4][img->block_y] = -1;
    } else
      currMB->mb_available[0][1]=&(img->mb_data[mb_nr-mb_width]);
  }

  // Check MB left above
  if(img->pix_x >= MB_BLOCK_SIZE && img->pix_y  >= MB_BLOCK_SIZE )
  {
    if(currMB->slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr)
      img->mb_data[mb_nr].mb_available[0][0]=&(img->mb_data[mb_nr-mb_width-1]);
  }

  // Check MB right above
  if(img->pix_y >= MB_BLOCK_SIZE && img->pix_x < (img->width-MB_BLOCK_SIZE ))
  {
    if(currMB->slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr)
      // currMB->mb_available[0][1]=&(img->mb_data[mb_nr-mb_width+1]);
      currMB->mb_available[0][2]=&(img->mb_data[mb_nr-mb_width+1]);
  }
}

/*!
 ************************************************************************
 * \brief
 *    Performs 4x4 and 16x16 intra prediction and transform coding
 *    of the prediction residue. The routine returns the best cost;
 *    current cbp (for LUMA only) and intra pred modes are affected
 ************************************************************************
 */
int MakeIntraPrediction(int *intra_pred_mode_2)
{

  int i,j;
  int block_x, block_y;
  int best_ipmode=0;
  int tot_intra_sad, tot_intra_sad2, best_intra_sad, current_intra_sad;
  int coeff_cost; // not used
  int pic_pix_y,pic_pix_x,pic_block_y,pic_block_x;
  int last_ipred=0;                       // keeps last chosen intra prediction mode for 4x4 intra pred
  int ipmode;                           // intra pred mode
  int cbp_mask;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  // start making 4x4 intra prediction
  currMB->cbp     = 0;
  currMB->intraOrInter = INTRA_MB_4x4;

  tot_intra_sad=QP2QUANT[img->qp]*24;// sum of intra sad values, start with a 'handicap'

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
      intrapred_luma(pic_pix_x,pic_pix_y);

      best_intra_sad=MAX_VALUE; // initial value, will be modified below
      img->imod = INTRA_MB_OLD;  // for now mode set to intra, may be changed in motion_search()
      // DM: has to be removed

      for (ipmode=0; ipmode < NO_INTRA_PMODE; ipmode++)   // all intra prediction modes
      {
        // Horizontal pred from Y neighbour pix , vertical use X pix, diagonal needs both
        if (ipmode==DC_PRED||ipmode==HOR_PRED||img->ipredmode[pic_block_x+1][pic_block_y] >= 0)// DC or vert pred or hor edge
        {
          if (ipmode==DC_PRED||ipmode==VERT_PRED||img->ipredmode[pic_block_x][pic_block_y+1] >= 0)// DC or hor pred or vert edge
          {
            for (j=0; j < BLOCK_SIZE; j++)
            {
              for (i=0; i < BLOCK_SIZE; i++)
                img->m7[i][j]=imgY_org[pic_pix_y+j][pic_pix_x+i]-img->mprr[ipmode][j][i]; // find diff
            }
            current_intra_sad=QP2QUANT[img->qp]*PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][ipmode]*2;

            current_intra_sad += find_sad(input->hadamard, img->m7); // add the start 'handicap' and the computed SAD

            if (current_intra_sad < best_intra_sad)
            {
              best_intra_sad=current_intra_sad;
              best_ipmode=ipmode;

              for (j=0; j < BLOCK_SIZE; j++)
                for (i=0; i < BLOCK_SIZE; i++)
                  img->mpr[i+block_x][j+block_y]=img->mprr[ipmode][j][i];       // store the currently best intra prediction block
            }
          }
        }
      }
      tot_intra_sad += best_intra_sad;

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
          img->m7[i][j] =imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i] - img->mpr[i+block_x][j+block_y];

        if( dct_luma(block_x,block_y,&coeff_cost) )          // if non zero coefficients
          currMB->cbp     |= cbp_mask;      // set coded block pattern if nonzero coeffs
    }
  }

  // 16x16 intra prediction
  intrapred_luma_2(img);                        // make intra pred for the new 4 modes
  tot_intra_sad2 = find_sad2(intra_pred_mode_2);        // find best SAD for new modes

  if (tot_intra_sad2<tot_intra_sad)
  {
    currMB->cbp     = 0;              // cbp for 16x16 LUMA is signaled by the MB-mode
    tot_intra_sad   = tot_intra_sad2;            // update best intra sad if necessary
    img->imod = INTRA_MB_NEW;                          // one of the new modes is used
    currMB->intraOrInter = INTRA_MB_16x16;
    dct_luma2(*intra_pred_mode_2);
    for (i=0;i<4;i++)
      for (j=0;j<4;j++)
        img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;
  }
  return tot_intra_sad;
}

/*!
 ************************************************************************
 * \brief
 *    Performs DCT, R-D constrained quantization, run/level
 *    pre-coding and IDCT for the MC-compensated MB residue
 *    of P-frame; current cbp (for LUMA only) is affected
 ************************************************************************
 */
void LumaResidualCoding_P()
{

  int i,j;
  int block_x, block_y;
  int pic_pix_y,pic_pix_x,pic_block_y,pic_block_x;
  int ii4,i2,jj4,j2;
  int sum_cnt_nonz;
  int mb_x, mb_y;
  int cbp_mask, cbp_blk_mask ;
  int coeff_cost;
  int nonzero;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];


  currMB->cbp     = 0 ;
  currMB->cbp_blk = 0 ;
  sum_cnt_nonz    = 0 ;

  for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
  {
    for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
    {
      cbp_mask   = (1<<(mb_x/8+mb_y/4));
      coeff_cost = 0;
      for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
      {
        pic_pix_y=img->pix_y+block_y;
        pic_block_y=pic_pix_y/BLOCK_SIZE;

        for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
        {
          pic_pix_x    = img->pix_x+block_x;
          pic_block_x  = pic_pix_x/BLOCK_SIZE;
          cbp_blk_mask = (block_x>>2)+ block_y ;

          img->ipredmode[pic_block_x+1][pic_block_y+1]=0;

          if(input->mv_res)

          {
            ii4=(img->pix_x+block_x)*8+tmp_mv[0][pic_block_y][pic_block_x+4];
            jj4=(img->pix_y+block_y)*8+tmp_mv[1][pic_block_y][pic_block_x+4];
            for (j=0;j<4;j++)
            {
              j2=j*8;
              for (i=0;i<4;i++)
              {
                i2=i*8;
                img->mpr[i+block_x][j+block_y]=UMVPelY_18 (mref[img->multframe_no], jj4+j2, ii4+i2);
              }
            }

          }
          else
          {
            ii4=(img->pix_x+block_x)*4+tmp_mv[0][pic_block_y][pic_block_x+4];
            jj4=(img->pix_y+block_y)*4+tmp_mv[1][pic_block_y][pic_block_x+4];
            for (j=0;j<4;j++)
            {
              j2 = jj4+j*4;
              for (i=0;i<4;i++)
              {
                i2 = ii4+i*4;
                img->mpr[i+block_x][j+block_y]=UMVPelY_14 (mref[img->multframe_no], j2, i2);

              }
            }
          }

          for (j=0; j < BLOCK_SIZE; j++)
          {
            for (i=0; i < BLOCK_SIZE; i++)
            {
              img->m7[i][j] =imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i] - img->mpr[i+block_x][j+block_y];
            }
          }
          if (img->types!=SP_IMG)
            nonzero=dct_luma(block_x,block_y,&coeff_cost);
          else nonzero=dct_luma_sp(block_x,block_y,&coeff_cost);

          if (nonzero)
          {
            currMB->cbp_blk |= 1 << cbp_blk_mask ;            // one bit for every 4x4 block
            currMB->cbp     |= cbp_mask;       // one bit for the 4x4 blocks of an 8x8 block
          }
        }
      }

      /*
      The purpose of the action below is to prevent that single or 'expensive' coefficients are coded.
      With 4x4 transform there is larger chance that a single coefficient in a 8x8 or 16x16 block may be nonzero.
      A single small (level=1) coefficient in a 8x8 block will cost: 3 or more bits for the coefficient,
      4 bits for EOBs for the 4x4 blocks,possibly also more bits for CBP.  Hence the total 'cost' of that single
      coefficient will typically be 10-12 bits which in a RD consideration is too much to justify the distortion improvement.
      The action below is to watch such 'single' coefficients and set the reconstructed block equal to the prediction according
      to a given criterium.  The action is taken only for inter luma blocks.

        Notice that this is a pure encoder issue and hence does not have any implication on the standard.
        coeff_cost is a parameter set in dct_luma() and accumulated for each 8x8 block.  If level=1 for a coefficient,
        coeff_cost is increased by a number depending on RUN for that coefficient.The numbers are (see also dct_luma()): 3,2,2,1,1,1,0,0,...
        when RUN equals 0,1,2,3,4,5,6, etc.
        If level >1 coeff_cost is increased by 9 (or any number above 3). The threshold is set to 3. This means for example:
        1: If there is one coefficient with (RUN,level)=(0,1) in a 8x8 block this coefficient is discarded.
        2: If there are two coefficients with (RUN,level)=(1,1) and (4,1) the coefficients are also discarded
        sum_cnt_nonz is the accumulation of coeff_cost over a whole macro block.  If sum_cnt_nonz is 5 or less for the whole MB,
        all nonzero coefficients are discarded for the MB and the reconstructed block is set equal to the prediction.
      */

      if (coeff_cost > 3)
      {
        sum_cnt_nonz += coeff_cost;
      }
      else //discard
      {
        currMB->cbp     &=  (63  -  cbp_mask ) ;
        currMB->cbp_blk &= ~(51 << (mb_y + (mb_x>>2) )) ;
        for (i=mb_x; i < mb_x+BLOCK_SIZE*2; i++)
        {
          for (j=mb_y; j < mb_y+BLOCK_SIZE*2; j++)
          {
            imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
          }
        }
        if (img->types==SP_IMG)
          for (i=mb_x; i < mb_x+BLOCK_SIZE*2; i+=BLOCK_SIZE)
            for (j=mb_y; j < mb_y+BLOCK_SIZE*2; j+=BLOCK_SIZE)
              copyblock_sp(i,j);
      }
    }
  }

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
     if (img->types==SP_IMG)
       for (i=0; i < MB_BLOCK_SIZE; i+=BLOCK_SIZE)
         for (j=0; j < MB_BLOCK_SIZE; j+=BLOCK_SIZE)
           copyblock_sp(i,j);
   }
}

/*!
 ************************************************************************
 * \brief
 *    Performs DCT, quantization, run/level pre-coding and IDCT
 *    for the chrominance of a I- of P-frame macroblock;
 *    current cbp and cr_cbp are affected
 ************************************************************************
 */
void ChromaCoding_P(int *cr_cbp)
{
  int i, j;
  int uv, ii,jj,ii0,jj0,ii1,jj1,if1,jf1,if0,jf0,f1,f2,f3,f4;
  int pic_block_y, pic_block_x;
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
    else
    {
      for (j=0; j < MB_BLOCK_SIZE/2; j++)
      {
        pic_block_y=(img->pix_c_y+j)/2;
        for (i=0; i < MB_BLOCK_SIZE/2; i++)
        {
          pic_block_x=(img->pix_c_x+i)/2;
          ii=(img->pix_c_x+i)*f1+tmp_mv[0][pic_block_y][pic_block_x+4];
          jj=(img->pix_c_y+j)*f1+tmp_mv[1][pic_block_y][pic_block_x+4];

          ii0 = max (0, min (img->width_cr-1,ii/f1));
          jj0 = max (0, min (img->height_cr-1,jj/f1));
          ii1 = max (0, min (img->width_cr-1,(ii+f2)/f1));
          jj1 = max (0, min (img->height_cr-1,(jj+f2)/f1));

          if1=(ii & f2);
          jf1=(jj & f2);
          if0=f1-if1;
          jf0=f1-jf1;
          img->mpr[i][j]=(if0*jf0*mcef[img->multframe_no][uv][jj0][ii0]+
            if1*jf0*mcef[img->multframe_no][uv][jj0][ii1]+
            if0*jf1*mcef[img->multframe_no][uv][jj1][ii0]+
            if1*jf1*mcef[img->multframe_no][uv][jj1][ii1]+f4)/f3;

          img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j];
        }
      }
    }
    if (img->types!=SP_IMG || (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW) )
      *cr_cbp=dct_chroma(uv,*cr_cbp);
    else *cr_cbp=dct_chroma_sp(uv,*cr_cbp);
  }
  currMB->cbp += *cr_cbp*16;
}

/*!
 ************************************************************************
 * \brief
 *    Set reference frame information in global arrays
 *    depending on mode decision. Used for motion vector prediction.
 ************************************************************************
 */
void SetRefFrameInfo_P()
{
  int i,j;

  if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
  {
    // Set the reference frame information for motion vector prediction as unavailable
    for (j = 0;j < BLOCK_MULTIPLE;j++)
    {
      for (i = 0;i < BLOCK_MULTIPLE;i++)
      {
        refFrArr[img->block_y+j][img->block_x+i] = -1;
      }
    }
  }
  else
  {
    // Set the reference frame information for motion vector prediction
    for (j = 0;j < BLOCK_MULTIPLE;j++)
    {
      for (i = 0;i < BLOCK_MULTIPLE;i++)
      {
        refFrArr[img->block_y+j][img->block_x+i] =  img->mb_data[img->current_mb_nr].ref_frame;
      }
    }
  }
}


/*!
 ************************************************************************
 * \brief
 *    Encode one macroblock depending on chosen picture type
 ************************************************************************
 */
void encode_one_macroblock()
{
  int cr_cbp;             // chroma coded block pattern
  int tot_intra_sad;
  int intra_pred_mode_2;  // best 16x16 intra mode

  if (input->rdopt)
  {
    RD_Mode_Decision ();
  }
  else
  {
    Macroblock *currMB = &img->mb_data[img->current_mb_nr];

    tot_intra_sad = MakeIntraPrediction(&intra_pred_mode_2);     // Intra Prediction
    if (img->type != B_IMG)         // I- or P-frame
    {
      if ((img->mb_y == img->mb_y_upd && img->mb_y_upd != img->mb_y_intra) || img->type == INTRA_IMG)
      {
        img->mb_mode=8*img->type+img->imod;  // Intra mode: set if intra image or if intra GOB for error robustness
      }
      else
      {
        currMB->ref_frame = motion_search(tot_intra_sad);      // P-frames MV-search
      }
    }
    else                          // B-frame
      currMB->ref_frame = motion_search_Bframe(tot_intra_sad); // B-frames MV-search

    if (img->type == B_IMG)         // B-frame
    {

      LumaResidualCoding_B(img);           // Residual coding of Luma (B-modes only)
      ChromaCoding_B(&cr_cbp);                              // Coding of Chrominance


      SetRefFrameInfo_B();     // Set ref-frame info for mv-prediction of future MBs
    }
    else                  // I- or P-frame
    {
      if (currMB->intraOrInter == INTER_MB)
        LumaResidualCoding_P();    // Residual coding of Luma (only for inter modes)
      // Coding of Luma in intra mode is done implicitly in MakeIntraPredicition
      ChromaCoding_P(&cr_cbp);                              // Coding of Chrominance

      // Set reference frame information for motion vector prediction of future MBs
      SetRefFrameInfo_P();

      //  Check if a MB is skipped (no coeffs. only 0-vectors and prediction from the most recent frame)
      if(   (currMB->ref_frame                      == 0) && (currMB->intraOrInter == INTER_MB)
        && (tmp_mv[0][img->block_y][img->block_x+4]== 0) && (img->mb_mode         == M16x16_MB)
        && (tmp_mv[1][img->block_y][img->block_x+4]== 0) && (currMB->cbp          == 0) )
        img->mb_mode=COPY_MB;
    }

    currMB->qp = img->qp; // this should (or has to be) done somewere else. where?
    DeblockMb(img, imgY, imgUV) ; // Deblock this MB ( pixels to the right and above are affected)

    if (img->imod==INTRA_MB_NEW)        // Set 16x16 intra mode and make "intra CBP"
    {
      img->mb_mode += intra_pred_mode_2 + 4*cr_cbp + 12*img->kac;
      currMB->cbp += 15*img->kac; //GB
    }


    if ((((img->type == INTER_IMG)||(img->types==SP_IMG))  && ((img->imod==INTRA_MB_NEW) || (img->imod==INTRA_MB_OLD)))
      || (img->type == B_IMG && (img->imod==B_Backward || img->imod==B_Direct || img->imod==INTRA_MB_NEW || img->imod==INTRA_MB_OLD)))// gb b-frames too
      currMB->ref_frame = 0;

  }
}

/*!
 ************************************************************************
 * \brief
 *    Passes the chosen syntax elements to the NAL
 ************************************************************************
 */
void write_one_macroblock()
{
  int i;
  int mb_nr = img->current_mb_nr;
  SyntaxElement *currSE = img->MB_SyntaxElements;
  Macroblock *currMB = &img->mb_data[mb_nr];
  int *bitCount = currMB->bitcounter;
  Slice *currSlice = img->currentSlice;
  DataPartition *dataPart;
  int *partMap = assignSE2partition[input->partition_mode];

  // Store imod for further use
  currMB->mb_imode = img->imod;

  // Store mb_mode for further use
  currMB->mb_type = (currSE->value1 = img->mb_mode);

  // choose the appropriate data partition
  if (img->type == B_IMG)
    dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
  else
    dataPart = &(currSlice->partArr[partMap[SE_MBTYPE]]);

  //  Bits for mode
  if(img->type == INTRA_IMG || img->types == SP_IMG || input->symbol_mode == CABAC)
  {
    if (input->symbol_mode == UVLC)
      currSE->mapping = n_linfo2;
    else
      currSE->writing = writeMB_typeInfo2Buffer_CABAC;
    currSE->type = SE_MBTYPE;

#if TRACE
    if (img->type == B_IMG)
      snprintf(currSE->tracestring, TRACESTRING_SIZE, "B_MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y, img->mb_mode);
    else
      snprintf(currSE->tracestring, TRACESTRING_SIZE, "MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y,img->mb_mode);
#endif
    dataPart->writeSyntaxElement( currSE, dataPart);

    bitCount[BITS_MB_MODE]+=currSE->len;
    currSE++;
    currMB->currSEnr++;

  }
  else
  {
    if (img->mb_mode != COPY_MB || currMB->intraOrInter != INTER_MB || (img->type == B_IMG && currMB->cbp != 0))
  {
      // Macroblock is coded, put out run and mbmode
      currSE->value1 = img->cod_counter;
      currSE->mapping = n_linfo2;
      currSE->type = SE_MBTYPE;
#if TRACE
      snprintf(currSE->tracestring, TRACESTRING_SIZE, "MB runlength = %3d",img->cod_counter);
#endif
      dataPart->writeSyntaxElement( currSE, dataPart);
      bitCount[BITS_MB_MODE]+=currSE->len;
      currSE++;
      currMB->currSEnr++;
    // Reset cod counter
      img->cod_counter = 0;

      // Put out mb mode
      currSE->value1 = img->mb_mode;
      if(img->type != B_IMG)
        currSE->value1--;
      currSE->mapping = n_linfo2;
      currSE->type = SE_MBTYPE;
#if TRACE
      if (img->type == B_IMG)
        snprintf(currSE->tracestring, TRACESTRING_SIZE, "B_MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y, img->mb_mode);
      else
        snprintf(currSE->tracestring, TRACESTRING_SIZE, "MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y,img->mb_mode);
#endif
      dataPart->writeSyntaxElement( currSE, dataPart);
      bitCount[BITS_MB_MODE]+=currSE->len;
      currSE++;
      currMB->currSEnr++;
    }
  else
  {
      // Macroblock is skipped, increase cod_counter
      img->cod_counter++;

      if(img->current_mb_nr == img->total_number_mb)
      {
        // Put out run
        currSE->value1 = img->cod_counter;
        currSE->mapping = n_linfo2;
        currSE->type = SE_MBTYPE;
#if TRACE
        snprintf(currSE->tracestring, TRACESTRING_SIZE, "MB runlength = %3d",img->cod_counter);
#endif
        dataPart->writeSyntaxElement( currSE, dataPart);
        bitCount[BITS_MB_MODE]+=currSE->len;
        currSE++;
        currMB->currSEnr++;

        // Reset cod counter
        img->cod_counter = 0;
      }
    }
  }

  if(input->UseConstrainedIntraPred)
  {
    if (img->type==INTER_IMG && img->types != SP_IMG)
      if(currMB->intraOrInter == INTER_MB)
        img->intra_mb[img->current_mb_nr] = 0;
  }


  //  Do nothing more if copy and inter mode
  if (img->mb_mode != COPY_MB || currMB->intraOrInter != INTER_MB ||
     (img->type == B_IMG && input->symbol_mode == CABAC) ||
     (img->type == B_IMG && input->symbol_mode == UVLC &&  currMB->cbp != 0))
  {

    //  Bits for intra prediction modes
    if (img->imod == INTRA_MB_OLD)
    {
      for (i=0; i < MB_BLOCK_SIZE/2; i++)
      {
        currSE->value1 = currMB->intra_pred_modes[2*i];
        currSE->value2 = currMB->intra_pred_modes[2*i+1];
        if (input->symbol_mode == UVLC)
          currSE->mapping = intrapred_linfo;
        else
          currSE->writing = writeIntraPredMode2Buffer_CABAC;
        currSE->type = SE_INTRAPREDMODE;

        // choose the appropriate data partition
        if (img->type != B_IMG)
        {
#if TRACE
          snprintf(currSE->tracestring, TRACESTRING_SIZE, "Intra mode     = %3d",IPRED_ORDER[currSE->value1][currSE->value2]);
#endif
          dataPart = &(currSlice->partArr[partMap[SE_INTRAPREDMODE]]);

        }
        else
        {
#if TRACE
          snprintf(currSE->tracestring, TRACESTRING_SIZE, "B_Intra mode = %3d\t",IPRED_ORDER[currSE->value1][currSE->value2]);
#endif
          dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

        }
        dataPart->writeSyntaxElement( currSE, dataPart);
        bitCount[BITS_COEFF_Y_MB]+=currSE->len;

        // proceed to next SE
        currSE++;
        currMB->currSEnr++;
      }
    }
    //  Bits for vector data
    if (img->type != B_IMG)
    {
      if (currMB->intraOrInter == INTER_MB) // inter
        writeMotionInfo2NAL_Pframe();
    }
    else
    {
      if(img->imod != B_Direct)
        writeMotionInfo2NAL_Bframe();
    }

    // Bits for CBP and Coefficients
    writeCBPandCoeffs2NAL();
  }
  bitCount[BITS_TOTAL_MB] = bitCount[BITS_MB_MODE] + bitCount[BITS_COEFF_Y_MB]  + bitCount[BITS_INTER_MB]
    + bitCount[BITS_CBP_MB] + bitCount[BITS_DELTA_QUANT_MB] + bitCount[BITS_COEFF_UV_MB];
  stat->bit_slice += bitCount[BITS_TOTAL_MB];
}

/*!
 ************************************************************************
 * \brief
 *    Passes for a given MB of a P picture the reference frame
 *    parameter and the motion vectors to the NAL
 ************************************************************************
 */
int writeMotionInfo2NAL_Pframe()
{
  int i,j,k,l,m;
  int step_h,step_v;
  int curr_mvd;
  int mb_nr = img->current_mb_nr;
  Macroblock *currMB = &img->mb_data[mb_nr];
  SyntaxElement *currSE = &img->MB_SyntaxElements[currMB->currSEnr];
  int *bitCount = currMB->bitcounter;
  Slice *currSlice = img->currentSlice;
  DataPartition *dataPart;
  int *partMap = assignSE2partition[input->partition_mode];
  int no_bits = 0;

  //  If multiple ref. frames, write reference frame for the MB
#ifdef _ADDITIONAL_REFERENCE_FRAME_
  if (input->no_multpred > 1 || input->add_ref_frame > 0)
#else
    if (input->no_multpred > 1)
#endif
    {

      currSE->value1 = currMB->ref_frame ;
      currSE->type = SE_REFFRAME;
      if (input->symbol_mode == UVLC)
        currSE->mapping = n_linfo2;
      else
        currSE->writing = writeRefFrame2Buffer_CABAC;
      dataPart = &(currSlice->partArr[partMap[currSE->type]]);
      dataPart->writeSyntaxElement( currSE, dataPart);
      bitCount[BITS_INTER_MB]+=currSE->len;
      no_bits += currSE->len;
#if TRACE
      snprintf(currSE->tracestring, TRACESTRING_SIZE, "Reference frame no %d", currMB->ref_frame);
#endif

      // proceed to next SE
      currSE++;
      currMB->currSEnr++;
    }

    // Write motion vectors
    step_h=img->blc_size_h/BLOCK_SIZE;      // horizontal stepsize
    step_v=img->blc_size_v/BLOCK_SIZE;      // vertical stepsize

    for (j=0; j < BLOCK_SIZE; j += step_v)
    {
      for (i=0;i < BLOCK_SIZE; i += step_h)
      {
        for (k=0; k < 2; k++)
        {
          curr_mvd = tmp_mv[k][img->block_y+j][img->block_x+i+4]-img->mv[i][j][currMB->ref_frame][img->mb_mode][k];

          img->subblock_x = i; // position used for context determination
          img->subblock_y = j; // position used for context determination
          currSE->value1 = curr_mvd;
          // store (oversampled) mvd
          for (l=0; l < step_v; l++)
            for (m=0; m < step_h; m++)
              currMB->mvd[0][j+l][i+m][k] =  curr_mvd;
          currSE->value2 = k; // identifies the component; only used for context determination
          currSE->type = SE_MVD;
          if (input->symbol_mode == UVLC)
            currSE->mapping = mvd_linfo2;
          else
            currSE->writing = writeMVD2Buffer_CABAC;
          dataPart = &(currSlice->partArr[partMap[currSE->type]]);
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
    return no_bits;
}

/*!
 ************************************************************************
 * \brief
 *    Passes coded block pattern and coefficients (run/level)
 *    to the NAL
 ************************************************************************
 */
void
writeCBPandCoeffs2NAL ()
{
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  if (img->imod != INTRA_MB_NEW)
  {
    // Bits for CBP
    writeMB_bits_for_CBP  ();
    // Bits for Delta quant
    if (currMB->cbp != 0)
      writeMB_bits_for_Dquant  ();
    // Bits for luma coefficients
    writeMB_bits_for_luma (1);
  }
  else // 16x16 based intra modes
  {
    writeMB_bits_for_Dquant  ();
    writeMB_bits_for_16x16_luma ();
  }
  // Bits for chroma 2x2 DC transform coefficients
  writeMB_bits_for_DC_chroma (1);
  // Bits for chroma AC-coeffs.
  writeMB_bits_for_AC_chroma (1);
}



int
writeMB_bits_for_CBP ()
{
  int           no_bits    = 0;
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int           *bitCount  = currMB->bitcounter;
  DataPartition *dataPart;
  int           *partMap   = assignSE2partition[input->partition_mode];

  currSE->value1 = currMB->cbp;

#if TRACE
  snprintf(currSE->tracestring, TRACESTRING_SIZE, "CBP (%2d,%2d) = %3d",img->mb_x, img->mb_y, currMB->cbp);
#endif

  if (img->imod == INTRA_MB_OLD)
  {
    if (input->symbol_mode == UVLC)
      currSE->mapping = cbp_linfo_intra;
    currSE->type = SE_CBP_INTRA;
  }
  else
  {
    if (input->symbol_mode == UVLC)
      currSE->mapping = cbp_linfo_inter;
    currSE->type = SE_CBP_INTER;
  }

  if (input->symbol_mode == CABAC)
    currSE->writing = writeCBP2Buffer_CABAC;

  // choose the appropriate data partition
  if (img->type != B_IMG)
    dataPart = &(img->currentSlice->partArr[partMap[currSE->type]]);
  else
    dataPart = &(img->currentSlice->partArr[partMap[SE_BFRAME]]);

  dataPart->writeSyntaxElement(currSE, dataPart);
  bitCount[BITS_CBP_MB]+=currSE->len;
  no_bits              +=currSE->len;

  // proceed to next SE
  currSE++;
  currMB->currSEnr++;

  return no_bits;
}

int
writeMB_bits_for_Dquant ()
{
  int           no_bits    = 0;
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int           *bitCount  = currMB->bitcounter;
  DataPartition *dataPart;
  int           *partMap   = assignSE2partition[input->partition_mode];


  currSE->value1 = currMB->delta_qp;
#if TRACE
  snprintf(currSE->tracestring, TRACESTRING_SIZE, "Delta QP (%2d,%2d) = %3d",img->mb_x, img->mb_y, currMB->delta_qp);
#endif
  if (input->symbol_mode == UVLC)
     currSE->mapping = dquant_linfo;
  else if (input->symbol_mode == CABAC)
     currSE->writing = writeDquant_CABAC;// writeMVD2Buffer_CABAC;

   currSE->type = SE_DELTA_QUANT;

   // choose the appropriate data partition

   if (img->type != B_IMG)
     dataPart = &(img->currentSlice->partArr[partMap[currSE->type]]);
   else
     dataPart = &(img->currentSlice->partArr[partMap[SE_BFRAME]]);

   dataPart->writeSyntaxElement(  currSE, dataPart);
   bitCount[BITS_DELTA_QUANT_MB]+=currSE->len;
   no_bits              +=currSE->len;

    // proceed to next SE
   currSE++;
   currMB->currSEnr++;

   return no_bits;
}

int
writeMB_bits_for_luma (int  filtering)
{
  int no_bits = 0;
  int cbp     = img->mb_data [img->current_mb_nr].cbp;
  int mb_y, mb_x, i, j, ii, jj, bits;

  for (mb_y=0; mb_y < 4; mb_y += 2)
  {
    for (mb_x=0; mb_x < 4; mb_x += 2)
    {
      for (j=mb_y; j < mb_y+2; j++)
      {
        jj=j/2;
        for (i=mb_x; i < mb_x+2; i++)
        {
          ii=i/2;
          if ((cbp & (1<<(ii+jj*2))) != 0)        // check for any coefficients
          {
            no_bits += (bits = writeMB_bits_for_4x4_luma (i, j, filtering));
          }
          else bits = 0;
#ifdef _RD_DEBUG_I4MODE_
          rcdebug_set_luma_rate_4x4 (i, j, bits);
#endif
        }
      }
    }
  }
  return no_bits;
}

int
writeMB_bits_for_4x4_luma (int i, int j, int  filtering)
{
  int           no_bits    = 0;
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int           *bitCount  = currMB->bitcounter;
  Slice         *currSlice = img->currentSlice;
  DataPartition *dataPart;
  int           *partMap   = assignSE2partition[input->partition_mode];

  int kk,kbeg,kend;
  int level, run;
  int k;


  if (img->imod == INTRA_MB_OLD && img->qp < 24)  // double scan
  {

    for(kk=0;kk<2;kk++)
    {
      kbeg=kk*9;
      kend=kbeg+8;
      level=1; // get inside loop

      for(k=kbeg;k <= kend && level !=0; k++)
      {
        level = currSE->value1 = img->cof[i][j][k][0][DOUBLE_SCAN]; // level
        run   = currSE->value2 = img->cof[i][j][k][1][DOUBLE_SCAN]; // run

        if (input->symbol_mode == UVLC)
          currSE->mapping = levrun_linfo_intra;
        else
        {
          currSE->context = 0; // for choosing context model
          currSE->writing = writeRunLevel2Buffer_CABAC;
        }

        if (k == kbeg)
        {
          currSE->type  = SE_LUM_DC_INTRA; // element is of type DC

          // choose the appropriate data partition
          if (img->type != B_IMG)
            dataPart = &(currSlice->partArr[partMap[SE_LUM_DC_INTRA]]);
          else
            dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
        }
        else
        {
          currSE->type  = SE_LUM_AC_INTRA;   // element is of type AC

          // choose the appropriate data partition
          if (img->type != B_IMG)
            dataPart = &(currSlice->partArr[partMap[SE_LUM_AC_INTRA]]);
          else
            dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
        }
        dataPart->writeSyntaxElement (currSE, dataPart);
        bitCount[BITS_COEFF_Y_MB]+=currSE->len;
        no_bits                  +=currSE->len;
#if TRACE
        snprintf(currSE->tracestring, TRACESTRING_SIZE, "Luma dbl(%2d,%2d)  level=%3d Run=%2d",kk,k,level,run);
#endif
        // proceed to next SE
        currSE++;
        currMB->currSEnr++;
      }
    }
  }
  else     // single scan
  {
    level=1; // get inside loop
    for(k=0;k<=16 && level !=0; k++)
    {
      level = currSE->value1 = img->cof[i][j][k][0][SINGLE_SCAN]; // level
      run   = currSE->value2 = img->cof[i][j][k][1][SINGLE_SCAN]; // run

      if (input->symbol_mode == UVLC)
        currSE->mapping = levrun_linfo_inter;
      else
        currSE->writing = writeRunLevel2Buffer_CABAC;

      if (k == 0)
      {
        if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
        {
          currSE->context = 2; // for choosing context model
          currSE->type  = SE_LUM_DC_INTRA;
        }
        else
        {
          currSE->context = 1; // for choosing context model
          currSE->type  = SE_LUM_DC_INTER;
        }
      }
      else
      {
        if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
        {
          currSE->context = 2; // for choosing context model
          currSE->type  = SE_LUM_AC_INTRA;
        }
        else
        {
          currSE->context = 1; // for choosing context model
          currSE->type  = SE_LUM_AC_INTER;
        }
      }

      // choose the appropriate data partition
      if (img->type != B_IMG)
        dataPart = &(currSlice->partArr[partMap[currSE->type]]);
      else
        dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

      dataPart->writeSyntaxElement (currSE, dataPart);
      bitCount[BITS_COEFF_Y_MB]+=currSE->len;
      no_bits                  +=currSE->len;
#if TRACE
      snprintf(currSE->tracestring, TRACESTRING_SIZE, "Luma sng(%2d) level =%3d run =%2d", k, level,run);
#endif
      // proceed to next SE
      currSE++;
      currMB->currSEnr++;

    }
  }

  return no_bits;
}





int
writeMB_bits_for_16x16_luma ()
{
  int           no_bits    = 0;
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int           *bitCount  = currMB->bitcounter;
  Slice         *currSlice = img->currentSlice;
  DataPartition *dataPart;
  int           *partMap   = assignSE2partition[input->partition_mode];

  int level, run;
  int i, j, k, mb_x, mb_y;


  // DC coeffs
  level=1; // get inside loop
  for (k=0;k<=16 && level !=0;k++)
  {
    level = currSE->value1 = img->cof[0][0][k][0][1]; // level
    run   = currSE->value2 = img->cof[0][0][k][1][1]; // run

    if (input->symbol_mode == UVLC)
      currSE->mapping = levrun_linfo_inter;
    else
    {
      currSE->context = 3; // for choosing context model
      currSE->writing = writeRunLevel2Buffer_CABAC;
    }
    currSE->type  = SE_LUM_DC_INTRA;   // element is of type DC

    // choose the appropriate data partition
    if (img->type != B_IMG)
      dataPart = &(currSlice->partArr[partMap[currSE->type]]);
    else
      dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

    dataPart->writeSyntaxElement (currSE, dataPart);
    bitCount[BITS_COEFF_Y_MB]+=currSE->len;
    no_bits                  +=currSE->len;
#if TRACE
    snprintf(currSE->tracestring, TRACESTRING_SIZE, "DC luma 16x16 sng(%2d) level =%3d run =%2d", k, level, run);
#endif

    // proceed to next SE
    currSE++;
    currMB->currSEnr++;
  }


  // AC coeffs
  if (img->kac==1)
  {
    for (mb_y=0; mb_y < 4; mb_y += 2)
    {
      for (mb_x=0; mb_x < 4; mb_x += 2)
      {
        for (j=mb_y; j < mb_y+2; j++)
        {
          for (i=mb_x; i < mb_x+2; i++)
          {
            level=1; // get inside loop
            for (k=0;k<16 && level !=0;k++)
            {
              level = currSE->value1 = img->cof[i][j][k][0][SINGLE_SCAN]; // level
              run   = currSE->value2 = img->cof[i][j][k][1][SINGLE_SCAN]; // run

              if (input->symbol_mode == UVLC)
                currSE->mapping = levrun_linfo_inter;
              else
              {
                currSE->context = 4; // for choosing context model
                currSE->writing = writeRunLevel2Buffer_CABAC;
              }
              currSE->type  = SE_LUM_AC_INTRA;   // element is of type AC

              // choose the appropriate data partition
              if (img->type != B_IMG)
                dataPart = &(currSlice->partArr[partMap[currSE->type]]);
              else
                dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

              dataPart->writeSyntaxElement (currSE, dataPart);
              bitCount[BITS_COEFF_Y_MB]+=currSE->len;
              no_bits                  +=currSE->len;
#if TRACE
              snprintf(currSE->tracestring, TRACESTRING_SIZE, "AC luma 16x16 sng(%2d) level =%3d run =%2d", k, level, run);
#endif
              // proceed to next SE
              currSE++;
              currMB->currSEnr++;
            }
          }
        }
      }
    }
  }

  return no_bits;
}





int
writeMB_bits_for_DC_chroma (int filtering)
{
  int           no_bits    = 0;
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int           *bitCount  = currMB->bitcounter;
  Slice         *currSlice = img->currentSlice;
  DataPartition *dataPart;
  int           *partMap   = assignSE2partition[input->partition_mode];

  int cbp = img->mb_data [img->current_mb_nr].cbp;

  int level, run;
  int k, uv;


  if (cbp > 15)  // check if any chroma bits in coded block pattern is set
  {
    for (uv=0; uv < 2; uv++)
    {
      level=1;
      for (k=0; k < 5 && level != 0; ++k)
      {
        level = currSE->value1 = img->cofu[k][0][uv]; // level
        run   = currSE->value2 = img->cofu[k][1][uv]; // run

        if (input->symbol_mode == UVLC)
          currSE->mapping = levrun_linfo_c2x2;
        else
          currSE->writing = writeRunLevel2Buffer_CABAC;

        if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
        {
          currSE->context = 6; // for choosing context model
          currSE->type  = SE_CHR_DC_INTRA;
        }
        else
        {
          currSE->context = 5; // for choosing context model
          currSE->type  = SE_CHR_DC_INTER;
        }

        // choose the appropriate data partition
        if (img->type != B_IMG)
          dataPart = &(currSlice->partArr[partMap[currSE->type]]);
        else
          dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

        dataPart->writeSyntaxElement (currSE, dataPart);
        bitCount[BITS_COEFF_UV_MB]+=currSE->len;
        no_bits                   +=currSE->len;
#if TRACE
        snprintf(currSE->tracestring, TRACESTRING_SIZE, "2x2 DC Chroma %2d: level =%3d run =%2d",k, level, run);
#endif

        // proceed to next SE
        currSE++;
        currMB->currSEnr++;
      }
    }
  }

  return no_bits;
}

int
writeMB_bits_for_AC_chroma (int  filtering)
{
  int           no_bits    = 0;
  Macroblock    *currMB    = &img->mb_data[img->current_mb_nr];
  SyntaxElement *currSE    = &img->MB_SyntaxElements[currMB->currSEnr];
  int           *bitCount  = currMB->bitcounter;
  Slice         *currSlice = img->currentSlice;
  DataPartition *dataPart;
  int           *partMap   = assignSE2partition[input->partition_mode];

  int cbp = img->mb_data [img->current_mb_nr].cbp;

  int level, run;
  int i, j, k, mb_x, mb_y, i1, ii, j1, jj;


  if (cbp >> 4 == 2) // check if chroma bits in coded block pattern = 10b
  {
    for (mb_y=4; mb_y < 6; mb_y += 2)
    {
      for (mb_x=0; mb_x < 4; mb_x += 2)
      {
        for (j=mb_y; j < mb_y+2; j++)
        {
          jj=j/2;
          j1=j-4;
          for (i=mb_x; i < mb_x+2; i++)
          {
            ii=i/2;
            i1=i%2;
            level=1;
            for (k=0; k < 16 && level != 0; k++)
            {
              level = currSE->value1 = img->cof[i][j][k][0][0]; // level
              run   = currSE->value2 = img->cof[i][j][k][1][0]; // run

              if (input->symbol_mode == UVLC)
                currSE->mapping = levrun_linfo_inter;
              else
                currSE->writing = writeRunLevel2Buffer_CABAC;

              if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
              {
                currSE->context = 8; // for choosing context model
                currSE->type  = SE_CHR_AC_INTRA;
              }
              else
              {
                currSE->context = 7; // for choosing context model
                currSE->type  = SE_CHR_AC_INTER;
              }
              // choose the appropriate data partition
              if (img->type != B_IMG)
                dataPart = &(currSlice->partArr[partMap[currSE->type]]);
              else
                dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

              dataPart->writeSyntaxElement (currSE, dataPart);
              bitCount[BITS_COEFF_UV_MB]+=currSE->len;
              no_bits                   +=currSE->len;
#if TRACE
              snprintf(currSE->tracestring, TRACESTRING_SIZE, "AC Chroma %2d: level =%3d run =%2d",k, level, run);
#endif
              // proceed to next SE
              currSE++;
              currMB->currSEnr++;
            }
          }
        }
      }
    }
  }

  return no_bits;
}

/*!
 ************************************************************************
 * \brief
 *    Find best 16x16 based intra mode
 *
 * \par Input:
 *    Image parameters, pointer to best 16x16 intra mode
 *
 * \par Output:
 *    best 16x16 based SAD
 ************************************************************************/
int find_sad2(int *intra_mode)
{
  int current_intra_sad_2,best_intra_sad2;
  int M1[16][16],M0[4][4][4][4],M3[4],M4[4][4];

  int i,j,k;
  int ii,jj;

  best_intra_sad2=MAX_VALUE;

  for (k=0;k<4;k++)
  {
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
    //check if there are neighbours to predict from
    if ((k==0 && !mb_available_up) || (k==1 && !mb_available_left) || (k==3 && (!mb_available_left || !mb_available_up)))
    {
      ; // edge, do nothing
    }
    else
    {
      for (j=0;j<16;j++)
      {
        for (i=0;i<16;i++)
        {
          M1[i][j]=imgY_org[img->pix_y+j][img->pix_x+i]-img->mprr_2[k][j][i];
          M0[i%4][i/4][j%4][j/4]=M1[i][j];
        }
      }
      current_intra_sad_2=0;              // no SAD start handicap here
      for (jj=0;jj<4;jj++)
      {
        for (ii=0;ii<4;ii++)
        {
          for (j=0;j<4;j++)
          {
            M3[0]=M0[0][ii][j][jj]+M0[3][ii][j][jj];
            M3[1]=M0[1][ii][j][jj]+M0[2][ii][j][jj];
            M3[2]=M0[1][ii][j][jj]-M0[2][ii][j][jj];
            M3[3]=M0[0][ii][j][jj]-M0[3][ii][j][jj];

            M0[0][ii][j][jj]=M3[0]+M3[1];
            M0[2][ii][j][jj]=M3[0]-M3[1];
            M0[1][ii][j][jj]=M3[2]+M3[3];
            M0[3][ii][j][jj]=M3[3]-M3[2];
          }

          for (i=0;i<4;i++)
          {
            M3[0]=M0[i][ii][0][jj]+M0[i][ii][3][jj];
            M3[1]=M0[i][ii][1][jj]+M0[i][ii][2][jj];
            M3[2]=M0[i][ii][1][jj]-M0[i][ii][2][jj];
            M3[3]=M0[i][ii][0][jj]-M0[i][ii][3][jj];

            M0[i][ii][0][jj]=M3[0]+M3[1];
            M0[i][ii][2][jj]=M3[0]-M3[1];
            M0[i][ii][1][jj]=M3[2]+M3[3];
            M0[i][ii][3][jj]=M3[3]-M3[2];
            for (j=0;j<4;j++)
              if ((i+j)!=0)
                current_intra_sad_2 += abs(M0[i][ii][j][jj]);
          }
        }
      }

      for (j=0;j<4;j++)
        for (i=0;i<4;i++)
          M4[i][j]=M0[0][i][0][j]/4;

        // Hadamard of DC koeff
        for (j=0;j<4;j++)
        {
          M3[0]=M4[0][j]+M4[3][j];
          M3[1]=M4[1][j]+M4[2][j];
          M3[2]=M4[1][j]-M4[2][j];
          M3[3]=M4[0][j]-M4[3][j];

          M4[0][j]=M3[0]+M3[1];
          M4[2][j]=M3[0]-M3[1];
          M4[1][j]=M3[2]+M3[3];
          M4[3][j]=M3[3]-M3[2];
        }

        for (i=0;i<4;i++)
        {
          M3[0]=M4[i][0]+M4[i][3];
          M3[1]=M4[i][1]+M4[i][2];
          M3[2]=M4[i][1]-M4[i][2];
          M3[3]=M4[i][0]-M4[i][3];

          M4[i][0]=M3[0]+M3[1];
          M4[i][2]=M3[0]-M3[1];
          M4[i][1]=M3[2]+M3[3];
          M4[i][3]=M3[3]-M3[2];

          for (j=0;j<4;j++)
            current_intra_sad_2 += abs(M4[i][j]);
        }
        if(current_intra_sad_2 < best_intra_sad2)
        {
          best_intra_sad2=current_intra_sad_2;
          *intra_mode = k; // update best intra mode

        }
    }
  }
  best_intra_sad2 = best_intra_sad2/2;

  return best_intra_sad2;

}
