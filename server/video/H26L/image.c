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
 * \file image.c
 *
 * \brief
 *    Code one image/slice
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *     - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *     - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *     - Jani Lainema                    <jani.lainema@nokia.com>
 *     - Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *     - Byeong-Moon Jeon                <jeonbm@lge.com>
 *     - Yoon-Seong Soh                  <yunsung@lge.com>
 *     - Thomas Stockhammer              <stockhammer@ei.tum.de>
 *     - Detlev Marpe                    <marpe@hhi.de>
 *     - Guido Heising                   <heising@hhi.de>
 *     - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *     - Ragip Kurceren                  <ragip.kurceren@nokia.com>
 *     - Antti Hallapuro                 <antti.hallapuro@nokia.com>
 *************************************************************************************
 */
#include "contributors.h"

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include "global.h"
#include "image.h"
#include "refbuf.h"

#ifdef _ADAPT_LAST_GROUP_
int *last_P_no;
#endif

//! The followibng two variables are used for debug purposes.  They store
//! the status of the currStream data structure elements after the header
//! writing, and are used whether any MB bits were written during the
//! macroblock coding stage.  We need to worry about empty slices!
static int Byte_Pos_After_Header;
static int Bits_To_Go_After_Header;

static void UnifiedOneForthPix (pel_t **imgY, pel_t** imgU, pel_t **imgV,
        pel_t **out4Y, pel_t **outU, pel_t **outV, pel_t *ref11);

/*!
 ************************************************************************
 * \brief
 *    Encodes one frame
 ************************************************************************
 */
int encode_one_frame()
{
#ifdef _LEAKYBUCKET_
  extern long Bit_Buffer[10000];
  extern unsigned long total_frame_buffer;
#endif
  Boolean end_of_frame = FALSE;
  SyntaxElement sym;

  time_t ltime1;   // for time measurement
  time_t ltime2;

#ifdef WIN32
  struct _timeb tstruct1;
  struct _timeb tstruct2;
#else
  struct timeval tstruct1;
  struct timeval tstruct2;
#endif

  int tmp_time;

#ifdef WIN32
  _ftime( &tstruct1 );    // start time ms
#else
  gettimeofday(&tstruct1, NULL);
#endif
  time( &ltime1 );        // start time s


  // Initialize frame with all stat and img variables
  img->total_number_mb = (img->width * img->height)/(MB_BLOCK_SIZE*MB_BLOCK_SIZE);
  init_frame();

  // Read one new frame
#ifndef H26L_LIB
  read_one_new_frame();
#endif

  if (img->type == B_IMG)
    Bframe_ctr++;

  while (end_of_frame == FALSE) // loop over slices
  {
    // Encode the current slice
    encode_one_slice(&sym);

    // Proceed to next slice
    img->current_slice_nr++;
    h26lstat->bit_slice = 0;
    if (img->current_mb_nr == img->total_number_mb) // end of frame reached?
      end_of_frame = TRUE;
  }

  // in future only one call of oneforthpix() for all frame tyoes will be necessary, because
  // mref buffer will be increased by one frame to store also the next P-frame. Then mref_P
  // will not be used any more
  if (img->type != B_IMG) //all I- and P-frames
  {
    if (input->successive_Bframe == 0 || img->number == 0)
      interpolate_frame(); // I- and P-frames:loop-filtered imgY, imgUV -> mref[][][], mcef[][][][]
    else
      interpolate_frame_2();    // I- and P-frames prior a B-frame:loop-filtered imgY, imgUV -> mref_P[][][], mcef_P[][][][]
                                 // I- and P-frames prior a B-frame:loop-filtered imgY, imgUV -> mref_P[][][], mcef_P[][][][]
  }
  else
    if (img->b_frame_to_code == input->successive_Bframe)
      copy2mref(img);          // last successive B-frame: mref_P[][][], mcef_P[][][][] (loop-filtered imgY, imgUV)-> mref[][][], mcef[][][][]

  if (input->rdopt==2)
    UpdateDecoders(); // simulate packet losses and move decoded image to reference buffers

  find_snr(snr,img);

  time(&ltime2);       // end time sec
#ifdef WIN32
  _ftime(&tstruct2);   // end time ms
  tmp_time=(ltime2*1000+tstruct2.millitm) - (ltime1*1000+tstruct1.millitm);
#else
  gettimeofday(&tstruct2, NULL);    // end time ms
  tmp_time=(ltime2*1000+tstruct2.tv_usec/1000) - (ltime1*1000+(tstruct1.tv_usec / 1000));
#endif
  tot_time=tot_time + tmp_time;

#ifndef H26L_LIB
  // Write reconstructed images
  write_reconstructed_image();
#endif

#ifdef _LEAKYBUCKET_
  // Store bits used for this frame and increment counter of no. of coded frames
  Bit_Buffer[total_frame_buffer] = h26lstat->bit_ctr - h26lstat->bit_ctr_n;
  total_frame_buffer++;
#endif
  if(img->number == 0)
  {
    printf("%3d(I)  %8d %4d %7.4f %7.4f %7.4f  %5d \n",
        frame_no, h26lstat->bit_ctr-h26lstat->bit_ctr_n,
        img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time);
    h26lstat->bitr0=h26lstat->bitr;
    h26lstat->bit_ctr_0=h26lstat->bit_ctr;
    h26lstat->bit_ctr=0;
  }
  else
  {
    if (img->type == INTRA_IMG)
    {
      h26lstat->bit_ctr_P += h26lstat->bit_ctr-h26lstat->bit_ctr_n;
      printf("%3d(I)  %8d %4d %7.4f %7.4f %7.4f  %5d \n",
        frame_no, h26lstat->bit_ctr-h26lstat->bit_ctr_n,
        img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time);
    }
    else if (img->type != B_IMG)
    {
      h26lstat->bit_ctr_P += h26lstat->bit_ctr-h26lstat->bit_ctr_n;
      if(img->types == SP_IMG)
        printf("%3d(SP) %8d %4d %7.4f %7.4f %7.4f  %5d    %3d\n",
          frame_no, h26lstat->bit_ctr-h26lstat->bit_ctr_n,
          img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, intras);
      else
        printf("%3d(P)  %8d %4d %7.4f %7.4f %7.4f  %5d    %3d\n",
          frame_no, h26lstat->bit_ctr-h26lstat->bit_ctr_n,
          img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time, intras);
    }
    else
    {
      h26lstat->bit_ctr_B += h26lstat->bit_ctr-h26lstat->bit_ctr_n;
      printf("%3d(B)  %8d %4d %7.4f %7.4f %7.4f  %5d \n",
        frame_no, h26lstat->bit_ctr-h26lstat->bit_ctr_n, img->qp,
        snr->snr_y, snr->snr_u, snr->snr_v, tmp_time);
    }
  }
  h26lstat->bit_ctr_n=h26lstat->bit_ctr;


  if(img->number == 0)
    return 0;
  else
    return 1;
}


/*!
 ************************************************************************
 * \brief
 *    Encodes one slice
 ************************************************************************
 */
void encode_one_slice(SyntaxElement *sym)
{
  Boolean end_of_slice = FALSE;
  Boolean recode_macroblock;
  int len;

  img->cod_counter=0;

  // Initializes the parameters of the current slice
  init_slice();

  // Write slice or picture header
  len = start_slice(sym);

  Byte_Pos_After_Header = img->currentSlice->partArr[0].bitstream->byte_pos;
  Bits_To_Go_After_Header = img->currentSlice->partArr[0].bitstream->bits_to_go;

  if (input->of_mode==PAR_OF_RTP)
  {
    assert (Byte_Pos_After_Header > 0);     // there must be a header
    assert (Bits_To_Go_After_Header == 8);  // byte alignment must have been established
  }

  // Update statistics
  h26lstat->bit_slice += len;
  h26lstat->bit_use_header[img->type] += len;

  while (end_of_slice == FALSE) // loop over macroblocks
  {
    // recode_macroblock is used in slice mode two and three where
    // backing of one macroblock in the bitstream is possible
    recode_macroblock = FALSE;

    // Initializes the current macroblock
    start_macroblock();

    // Encode one macroblock
    encode_one_macroblock();

    // Pass the generated syntax elements to the NAL
    write_one_macroblock();

    // Terminate processing of the current macroblock
    terminate_macroblock(&end_of_slice, &recode_macroblock);

    if (recode_macroblock == FALSE)         // The final processing of the macroblock has been done
      proceed2nextMacroblock(); // Go to next macroblock

  }
  terminate_slice();
}


/*!
 ************************************************************************
 * \brief
 *    Initializes the parameters for a new frame
 ************************************************************************
 */
void init_frame()
{
  int i,j,k;
  int prevP_no, nextP_no;

  img->current_mb_nr=0;
  img->current_slice_nr=0;
  h26lstat->bit_slice = 0;

  if(input->UseConstrainedIntraPred)
  {
    for (i=0; i<img->total_number_mb; i++)
      img->intra_mb[i] = 1; // default 1 = intra mb
  }

  img->mhor  = img->width *4-1;
  img->mvert = img->height*4-1;

  img->mb_y = img->mb_x = 0;
  img->block_y = img->pix_y = img->pix_c_y = 0;   // define vertical positions
  img->block_x = img->pix_x = img->block_c_x = img->pix_c_x = 0; // define horizontal positions

  if (input->intra_upd > 0 && img->mb_y <= img->mb_y_intra)
    img->height_err=(img->mb_y_intra*16)+15;     // for extra intra MB
  else
    img->height_err=img->height-1;

  if(img->type != B_IMG)
  {
    img->refPicID ++;
    img->tr=img->number*(input->jumpd+1);

#ifdef _ADAPT_LAST_GROUP_
    if (input->last_frame && img->number+1 == input->no_frames)
      img->tr=input->last_frame;
#endif
    if(img->number!=0 && input->successive_Bframe != 0)   // B pictures to encode
      nextP_tr=img->tr;

    if (img->type == INTRA_IMG)
      img->qp = input->qp0;         // set quant. parameter for I-frame
    else
    {
#ifdef _CHANGE_QP_
        if (input->qp2start > 0 && img->tr >= input->qp2start)
          img->qp = input->qpN2;
        else
#endif
          img->qp = input->qpN;
      if (img->types==SP_IMG)
      {
        img->qp = input->qpsp;
        img->qpsp = input->qpsp_pred;
      }

    }

    img->mb_y_intra=img->mb_y_upd;   //  img->mb_y_intra indicates which GOB to intra code for this frame

    if (input->intra_upd > 0)          // if error robustness, find next GOB to update
    {
      img->mb_y_upd=(img->number/input->intra_upd) % (img->height/MB_BLOCK_SIZE);
    }
  }
  else
  {
    img->p_interval = input->jumpd+1;
    prevP_no = (img->number-1)*img->p_interval;
    nextP_no = img->number*img->p_interval;
#ifdef _ADAPT_LAST_GROUP_
    last_P_no[0] = prevP_no; for (i=1; i<img->buf_cycle; i++) last_P_no[i] = last_P_no[i-1]-img->p_interval;

    if (input->last_frame && img->number+1 == input->no_frames)
      {
        nextP_no        =input->last_frame;
        img->p_interval =nextP_no - prevP_no;
      }
#endif


    img->b_interval = (int)((float)(input->jumpd+1)/(input->successive_Bframe+1.0)+0.49999);

    img->tr= prevP_no+img->b_interval*img->b_frame_to_code; // from prev_P
    if(img->tr >= nextP_no)
      img->tr=nextP_no-1; // ?????

#ifdef _CHANGE_QP_
    if (input->qp2start > 0 && img->tr >= input->qp2start)
      img->qp = input->qpB2;
    else
#endif
      img->qp = input->qpB;

    // initialize arrays
    for(k=0; k<2; k++)
      for(i=0; i<img->height/BLOCK_SIZE; i++)
        for(j=0; j<img->width/BLOCK_SIZE+4; j++)
        {
          tmp_fwMV[k][i][j]=0;
          tmp_bwMV[k][i][j]=0;
          dfMV[k][i][j]=0;
          dbMV[k][i][j]=0;
        }
    for(i=0; i<img->height/BLOCK_SIZE; i++)
      for(j=0; j<img->width/BLOCK_SIZE; j++)
      {
        fw_refFrArr[i][j]=bw_refFrArr[i][j]=-1;
      }

  }
}

/*!
 ************************************************************************
 * \brief
 *    Initializes the parameters for a new slice
 ************************************************************************
 */
void init_slice()
{

  int i;
  Slice *curr_slice = img->currentSlice;
  DataPartition *dataPart;
  Bitstream *currStream;

  curr_slice->picture_id = img->tr%256;
  curr_slice->slice_nr = img->current_slice_nr;
  curr_slice->qp = img->qp;
  curr_slice->start_mb_nr = img->current_mb_nr;
  curr_slice->dp_mode = input->partition_mode;
  curr_slice->slice_too_big = dummy_slice_too_big;

  for (i=0; i<curr_slice->max_part_nr; i++)
  {
    dataPart = &(curr_slice->partArr[i]);

    // in priciple it is possible to assign to each partition
    // a different entropy coding method
    if (input->symbol_mode == UVLC)
      dataPart->writeSyntaxElement = writeSyntaxElement_UVLC;
    else
      dataPart->writeSyntaxElement = writeSyntaxElement_CABAC;

    // A little hack until CABAC can handle non-byte aligned start positions   StW!
    // For UVLC, the stored_ positions in the bit buffer are necessary.  For CABAC,
    // the buffer is initialized to start at zero.


    if (input->symbol_mode == UVLC && input->of_mode == PAR_OF_26L)    // Stw: added PAR_OF_26L check
    {
      currStream = dataPart->bitstream;
      currStream->bits_to_go  = currStream->stored_bits_to_go;
      currStream->byte_pos    = currStream->stored_byte_pos;
      currStream->byte_buf    = currStream->stored_byte_buf;
    } 
    else // anything but UVLC and PAR_OF_26L
    {   
      currStream = dataPart->bitstream;
      currStream->bits_to_go  = 8;
      currStream->byte_pos    = 0;
      currStream->byte_buf    = 0;
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    Reads new frame from file and sets frame_no
 ************************************************************************
 */
void read_one_new_frame()
{
  int i, j, uv;
  int status; // frame_no;
  int frame_size = img->height*img->width*3/2;

  if(img->type == B_IMG)
    frame_no = (img->number-1)*(input->jumpd+1)+img->b_interval*img->b_frame_to_code;
  else
  {
    frame_no = img->number*(input->jumpd+1);
#ifdef _ADAPT_LAST_GROUP_
      if (input->last_frame && img->number+1 == input->no_frames)
        frame_no=input->last_frame;
#endif
  }

  rewind (p_in);

  status  = fseek (p_in, frame_no * frame_size + input->infile_header, 0);

  if (status != 0)
  {
    snprintf(errortext, ET_SIZE, "Error in seeking frame no: %d\n", frame_no);
    error(errortext,1);
  }

  for (j=0; j < img->height; j++)
    for (i=0; i < img->width; i++)
      imgY_org[j][i]=fgetc(p_in);
  for (uv=0; uv < 2; uv++)
    for (j=0; j < img->height_cr ; j++)
      for (i=0; i < img->width_cr; i++)
        imgUV_org[uv][j][i]=fgetc(p_in);
}

/*!
 ************************************************************************
 * \brief
 *     Writes reconstructed image(s) to file
 *     This can be done more elegant!
 ************************************************************************
 */
void write_reconstructed_image()
{
  int i, j, k;

  if (p_dec != NULL)
  {
    if(img->type != B_IMG)
    {
      // write reconstructed image (IPPP)
      if(input->successive_Bframe==0)
      {
        for (i=0; i < img->height; i++)
          for (j=0; j < img->width; j++)
            fputc(min(imgY[i][j],255),p_dec);

        for (k=0; k < 2; ++k)
          for (i=0; i < img->height/2; i++)
            for (j=0; j < img->width/2; j++)
              fputc(min(imgUV[k][i][j],255),p_dec);
      }

      // write reconstructed image (IBPBP) : only intra written
      else if (img->number==0 && input->successive_Bframe!=0)
      {
        for (i=0; i < img->height; i++)
          for (j=0; j < img->width; j++)
            fputc(min(imgY[i][j],255),p_dec);

        for (k=0; k < 2; ++k)
          for (i=0; i < img->height/2; i++)
            for (j=0; j < img->width/2; j++)
              fputc(min(imgUV[k][i][j],255),p_dec);
      }

      // next P picture. This is saved with recon B picture after B picture coding
      if (img->number!=0 && input->successive_Bframe!=0)
      {
        for (i=0; i < img->height; i++)
          for (j=0; j < img->width; j++)
            nextP_imgY[i][j]=imgY[i][j];
        for (k=0; k < 2; ++k)
          for (i=0; i < img->height/2; i++)
            for (j=0; j < img->width/2; j++)
              nextP_imgUV[k][i][j]=imgUV[k][i][j];
      }
    }
    else
    {
      for (i=0; i < img->height; i++)
        for (j=0; j < img->width; j++)
          fputc(min(imgY[i][j],255),p_dec);
      for (k=0; k < 2; ++k)
        for (i=0; i < img->height/2; i++)
          for (j=0; j < img->width/2; j++)
            fputc(min(imgUV[k][i][j],255),p_dec);

      // If this is last B frame also store P frame
      if(img->b_frame_to_code == input->successive_Bframe)
      {
        // save P picture
        for (i=0; i < img->height; i++)
          for (j=0; j < img->width; j++)
            fputc(min(nextP_imgY[i][j],255),p_dec);
        for (k=0; k < 2; ++k)
          for (i=0; i < img->height/2; i++)
            for (j=0; j < img->width/2; j++)
              fputc(min(nextP_imgUV[k][i][j],255),p_dec);
      }
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    Choose interpolation method depending on MV-resolution
 ************************************************************************
 */
void interpolate_frame()
{                 // write to mref[]
  int rpic;

  rpic = img->frame_cycle = img->number % img->buf_cycle;

  if(input->mv_res)
    oneeighthpix(0);
  else
    UnifiedOneForthPix(imgY, imgUV[0], imgUV[1],
               mref[rpic], mcef[rpic][0], mcef[rpic][1],
               Refbuf11[rpic]);
}

/*!
 ************************************************************************
 * \brief
 *    Choose interpolation method depending on MV-resolution
 ************************************************************************
 */
void interpolate_frame_2()      // write to mref_P
{
  if(input->mv_res)
    oneeighthpix(1);
  else
    UnifiedOneForthPix(imgY, imgUV[0], imgUV[1], mref_P, mcef_P[0], mcef_P[1], Refbuf11_P);

}




static void GenerateFullPelRepresentation (pel_t **Fourthpel, pel_t *Fullpel, int xsize, int ysize)
{

  int x, y;

  for (y=0; y<ysize; y++)
    for (x=0; x<xsize; x++)
      PutPel_11 (Fullpel, y, x, FastPelY_14 (Fourthpel, y*4, x*4));
}


/*!
 ************************************************************************
 * \brief
 *    Upsample 4 times, store them in out4x.  Color is simply copied
 *
 * \par Input:
 *    srcy, srcu, srcv, out4y, out4u, out4v
 *
 * \par Side Effects_
 *    Uses (writes) img4Y_tmp.  This should be moved to a static variable
 *    in this module
 ************************************************************************/
void UnifiedOneForthPix (pel_t **imgY, pel_t** imgU, pel_t **imgV,
                         pel_t **out4Y, pel_t **outU, pel_t **outV,
                         pel_t *ref11)
{
  int is;
  int i,j,j4;
  int ie2,je2,jj,maxy;


  for (j=-IMG_PAD_SIZE; j < img->height+IMG_PAD_SIZE; j++)
  {
    for (i=-IMG_PAD_SIZE; i < img->width+IMG_PAD_SIZE; i++)
    {
      jj = max(0,min(img->height-1,j));
      is=(ONE_FOURTH_TAP[0][0]*(imgY[jj][max(0,min(img->width-1,i  ))]+imgY[jj][max(0,min(img->width-1,i+1))])+
          ONE_FOURTH_TAP[1][0]*(imgY[jj][max(0,min(img->width-1,i-1))]+imgY[jj][max(0,min(img->width-1,i+2))])+
          ONE_FOURTH_TAP[2][0]*(imgY[jj][max(0,min(img->width-1,i-2))]+imgY[jj][max(0,min(img->width-1,i+3))]));
      img4Y_tmp[j+IMG_PAD_SIZE][(i+IMG_PAD_SIZE)*2  ]=imgY[jj][max(0,min(img->width-1,i))]*1024;    // 1/1 pix pos
      img4Y_tmp[j+IMG_PAD_SIZE][(i+IMG_PAD_SIZE)*2+1]=is*32;              // 1/2 pix pos
    }
  }
  
  for (i=0; i < (img->width+2*IMG_PAD_SIZE)*2; i++)
  {
    for (j=0; j < img->height+2*IMG_PAD_SIZE; j++)
    {
      j4=j*4;
      maxy = img->height+2*IMG_PAD_SIZE-1;
      // change for TML4, use 6 TAP vertical filter
      is=(ONE_FOURTH_TAP[0][0]*(img4Y_tmp[j         ][i]+img4Y_tmp[min(maxy,j+1)][i])+
          ONE_FOURTH_TAP[1][0]*(img4Y_tmp[max(0,j-1)][i]+img4Y_tmp[min(maxy,j+2)][i])+
          ONE_FOURTH_TAP[2][0]*(img4Y_tmp[max(0,j-2)][i]+img4Y_tmp[min(maxy,j+3)][i]))/32;

      PutPel_14 (out4Y, (j-IMG_PAD_SIZE)*4,   (i-IMG_PAD_SIZE*2)*2,(pel_t) max(0,min(255,(int)((img4Y_tmp[j][i]+512)/1024))));     // 1/2 pix
      PutPel_14 (out4Y, (j-IMG_PAD_SIZE)*4+2, (i-IMG_PAD_SIZE*2)*2,(pel_t) max(0,min(255,(int)((is+512)/1024)))); // 1/2 pix
    }
  }

  /* 1/4 pix */
  /* luma */
  ie2=(img->width+2*IMG_PAD_SIZE-1)*4;
  je2=(img->height+2*IMG_PAD_SIZE-1)*4;

  for (j=0;j<je2+4;j+=2)
    for (i=0;i<ie2+3;i+=2) {
      /*  '-'  */
      PutPel_14 (out4Y, j-IMG_PAD_SIZE*4, i-IMG_PAD_SIZE*4+1, (pel_t) (max(0,min(255,(int)(FastPelY_14(out4Y, j-IMG_PAD_SIZE*4, i-IMG_PAD_SIZE*4)+FastPelY_14(out4Y, j-IMG_PAD_SIZE*4, min(ie2+2,i+2)-IMG_PAD_SIZE*4))/2))));
    }
  for (i=0;i<ie2+4;i++)
  {
    for (j=0;j<je2+3;j+=2)
    {
      if( i%2 == 0 ) {
        /*  '|'  */
        PutPel_14 (out4Y, j-IMG_PAD_SIZE*4+1, i-IMG_PAD_SIZE*4, (pel_t)(max(0,min(255,(int)(FastPelY_14(out4Y, j-IMG_PAD_SIZE*4, i-IMG_PAD_SIZE*4)+FastPelY_14(out4Y, min(je2+2,j+2)-IMG_PAD_SIZE*4, i-IMG_PAD_SIZE*4))/2))));
      }
      else if( ((i&3) == 3)&&(((j+1)&3) == 3))
      {
        /* "funny posision" */
        PutPel_14 (out4Y, j-IMG_PAD_SIZE*4+1, i-IMG_PAD_SIZE*4, (pel_t) ((
          FastPelY_14 (out4Y, j-IMG_PAD_SIZE*4-2, i-IMG_PAD_SIZE*4-3) +
          FastPelY_14 (out4Y, min(je2,j+2)-IMG_PAD_SIZE*4, i-IMG_PAD_SIZE*4-3) +
          FastPelY_14 (out4Y, j-IMG_PAD_SIZE*4-2, min(ie2,i+1)-IMG_PAD_SIZE*4) +
          FastPelY_14 (out4Y, min(je2,j+2)-IMG_PAD_SIZE*4, min(ie2,i+1)-IMG_PAD_SIZE*4)
          + 2 )/4));
      }
      else if ((j%4 == 0 && i%4 == 1) || (j%4 == 2 && i%4 == 3)) {
        /*  '/'  */
        PutPel_14 (out4Y, j-IMG_PAD_SIZE*4+1, i-IMG_PAD_SIZE*4, (pel_t)(max(0,min(255,(int)(
                   FastPelY_14 (out4Y, j-IMG_PAD_SIZE*4, min(ie2+2,i+1)-IMG_PAD_SIZE*4) +
                   FastPelY_14(out4Y, min(je2+2,j+2)-IMG_PAD_SIZE*4, i-IMG_PAD_SIZE*4-1))/2))));
      }
      else {
        /*  '\'  */
        PutPel_14 (out4Y, j-IMG_PAD_SIZE*4+1, i-IMG_PAD_SIZE*4, (pel_t)(max(0,min(255,(int)(
          FastPelY_14 (out4Y, j-IMG_PAD_SIZE*4, i-IMG_PAD_SIZE*4-1) +
          FastPelY_14(out4Y, min(je2+2,j+2)-IMG_PAD_SIZE*4, min(ie2+2,i+1)-IMG_PAD_SIZE*4))/2))));
      }
    }
  }

  /*  Chroma: */
  for (j=0; j < img->height_cr; j++) {
    memcpy(outU[j],imgUV[0][j],img->width_cr); // just copy 1/1 pix, interpolate "online" 
    memcpy(outV[j],imgUV[1][j],img->width_cr);
  }

  // Generate 1/1th pel representation (used for integer pel MV search)
  GenerateFullPelRepresentation (out4Y, ref11, img->width, img->height);

}

/*!
 ************************************************************************
 * \brief
 *    Upsample 4 times for 1/8-pel estimation and store in buffer
 *    for multiple reference frames. 1/8-pel resolution is calculated
 *    during the motion estimation on the fly with bilinear interpolation.
 *
 ************************************************************************
 */
void oneeighthpix(int prior_B_frame)
{
  static int h1[8] = {  -3, 12, -37, 229,  71, -21,  6, -1 };  
  static int h2[8] = {  -3, 12, -39, 158, 158, -39, 12, -3 };  
  static int h3[8] = {  -1,  6, -21,  71, 229, -37, 12, -3 };  

  int uv,x,y,y1,x4,y4,x4p;

  int nx_out, ny_out, nx_1, ny_1, maxy;
  int i0,i1,i2,i3;

  img->frame_cycle=img->number % img->buf_cycle;  /*GH input->no_multpred used insteadof MAX_MULT_PRED
                                                  frame buffer size = input->no_multpred+1*/
  nx_out = 4*img->width;
  ny_out = 4*img->height;
  nx_1   = img->width-1;
  ny_1   = img->height-1;


  //horizontal filtering filtering
  for(y=-IMG_PAD_SIZE;y<img->height+IMG_PAD_SIZE;y++)
  {
    for(x=-IMG_PAD_SIZE;x<img->width+IMG_PAD_SIZE;x++)
    {
      y1 = max(0,min(ny_1,y));

      i0=(256*imgY[y1][max(0,min(nx_1,x))]);
      
      i1=(
        h1[0]* imgY[y1][max(0,min(nx_1,x-3))]  +
        h1[1]* imgY[y1][max(0,min(nx_1,x-2))]  +
        h1[2]* imgY[y1][max(0,min(nx_1,x-1))]  +
        h1[3]* imgY[y1][max(0,min(nx_1,x  ))]  +
        h1[4]* imgY[y1][max(0,min(nx_1,x+1))]  +
        h1[5]* imgY[y1][max(0,min(nx_1,x+2))]  +
        h1[6]* imgY[y1][max(0,min(nx_1,x+3))]  +                         
        h1[7]* imgY[y1][max(0,min(nx_1,x+4))] );
      
      
      i2=(
        h2[0]* imgY[y1][max(0,min(nx_1,x-3))]  +
        h2[1]* imgY[y1][max(0,min(nx_1,x-2))]  +
        h2[2]* imgY[y1][max(0,min(nx_1,x-1))]  +
        h2[3]* imgY[y1][max(0,min(nx_1,x  ))]  +
        h2[4]* imgY[y1][max(0,min(nx_1,x+1))]  +
        h2[5]* imgY[y1][max(0,min(nx_1,x+2))]  +
        h2[6]* imgY[y1][max(0,min(nx_1,x+3))]  +                         
        h2[7]* imgY[y1][max(0,min(nx_1,x+4))] );
      
      
      i3=(
        h3[0]* imgY[y1][max(0,min(nx_1,x-3))]  +
        h3[1]* imgY[y1][max(0,min(nx_1,x-2))]  +
        h3[2]* imgY[y1][max(0,min(nx_1,x-1))]  +
        h3[3]* imgY[y1][max(0,min(nx_1,x  ))]  +
        h3[4]* imgY[y1][max(0,min(nx_1,x+1))]  +
        h3[5]* imgY[y1][max(0,min(nx_1,x+2))]  +
        h3[6]* imgY[y1][max(0,min(nx_1,x+3))]  +                         
        h3[7]* imgY[y1][max(0,min(nx_1,x+4))] );
      
      x4=(x+IMG_PAD_SIZE)*4;

      img4Y_tmp[y+IMG_PAD_SIZE][x4  ] = i0;
      img4Y_tmp[y+IMG_PAD_SIZE][x4+1] = i1;
      img4Y_tmp[y+IMG_PAD_SIZE][x4+2] = i2;
      img4Y_tmp[y+IMG_PAD_SIZE][x4+3] = i3;
    }
  }

  maxy = img->height+2*IMG_PAD_SIZE-1;

  for(x4=0;x4<nx_out+2*IMG_PAD_SIZE*4;x4++)
  {
    for(y=0;y<=maxy;y++)
    {
      i0=(long int)(img4Y_tmp[y][x4]+256/2)/256;
      
      i1=(long int)( 
        h1[0]* img4Y_tmp[max(0   ,y-3)][x4]+
        h1[1]* img4Y_tmp[max(0   ,y-2)][x4]+
        h1[2]* img4Y_tmp[max(0   ,y-1)][x4]+
        h1[3]* img4Y_tmp[y][x4]            +
        h1[4]* img4Y_tmp[min(maxy,y+1)][x4]+
        h1[5]* img4Y_tmp[min(maxy,y+2)][x4]+
        h1[6]* img4Y_tmp[min(maxy,y+3)][x4]+ 
        h1[7]* img4Y_tmp[min(maxy,y+4)][x4]+ 256*256/2 ) / (256*256);
      
      i2=(long int)( 
        h2[0]* img4Y_tmp[max(0   ,y-3)][x4]+
        h2[1]* img4Y_tmp[max(0   ,y-2)][x4]+
        h2[2]* img4Y_tmp[max(0   ,y-1)][x4]+
        h2[3]* img4Y_tmp[y][x4]            +
        h2[4]* img4Y_tmp[min(maxy,y+1)][x4]+
        h2[5]* img4Y_tmp[min(maxy,y+2)][x4]+
        h2[6]* img4Y_tmp[min(maxy,y+3)][x4]+ 
        h2[7]* img4Y_tmp[min(maxy,y+4)][x4]+ 256*256/2 ) / (256*256);
      
      i3=(long int)( 
        h3[0]* img4Y_tmp[max(0   ,y-3)][x4]+
        h3[1]* img4Y_tmp[max(0   ,y-2)][x4]+
        h3[2]* img4Y_tmp[max(0   ,y-1)][x4]+
        h3[3]* img4Y_tmp[y][x4]            +
        h3[4]* img4Y_tmp[min(maxy,y+1)][x4]+
        h3[5]* img4Y_tmp[min(maxy,y+2)][x4]+
        h3[6]* img4Y_tmp[min(maxy,y+3)][x4]+ 
        h3[7]* img4Y_tmp[min(maxy,y+4)][x4]+ 256*256/2 ) / (256*256);
      
      y4  = (y-IMG_PAD_SIZE)*4;
      x4p = x4-IMG_PAD_SIZE*4;
  
      if(prior_B_frame)
      {
        PutPel_14 (mref_P, y4,   x4p, (pel_t) max(0,min(255,i0)));   
        PutPel_14 (mref_P, y4+1, x4p, (pel_t) max(0,min(255,i1)));   
        PutPel_14 (mref_P, y4+2, x4p, (pel_t) max(0,min(255,i2)));   
        PutPel_14 (mref_P, y4+3, x4p, (pel_t) max(0,min(255,i3)));   
      }
      else
      {
        PutPel_14 (mref[img->frame_cycle], y4,   x4p, (pel_t) max(0,min(255,i0)));   
        PutPel_14 (mref[img->frame_cycle], y4+1, x4p, (pel_t) max(0,min(255,i1)));   
        PutPel_14 (mref[img->frame_cycle], y4+2, x4p, (pel_t) max(0,min(255,i2)));
        PutPel_14 (mref[img->frame_cycle], y4+3, x4p, (pel_t) max(0,min(255,i3)));   
      }

    }
  }

  if(!prior_B_frame)
  {
    for(y=0;y<img->height;y++)
      for(x=0;x<img->width;x++)
        PutPel_11 (Refbuf11[img->frame_cycle], y, x, FastPelY_14 (mref[img->frame_cycle], y*4, x*4));
  }

  // Chroma and full pel representation:
  if(prior_B_frame)
  {
    for (uv=0; uv < 2; uv++)
      for (y=0; y < img->height_cr; y++)
        memcpy(mcef_P[uv][y],imgUV[uv][y],img->width_cr); // just copy 1/1 pix, interpolate "online"
    GenerateFullPelRepresentation (mref_P, Refbuf11_P, img->width, img->height);

  }
  else
  {
    for (uv=0; uv < 2; uv++)
      for (y=0; y < img->height_cr; y++)
        memcpy(mcef[img->frame_cycle][uv][y],imgUV[uv][y],img->width_cr); // just copy 1/1 pix, interpolate "online"
    GenerateFullPelRepresentation (mref[img->frame_cycle], Refbuf11[img->frame_cycle], img->width, img->height);
  }
    // Generate 1/1th pel representation (used for integer pel MV search)

}

/*!
 ************************************************************************
 * \brief
 *    Find SNR for all three components
 ************************************************************************
 */
void find_snr()
{
  int i,j;
  int diff_y,diff_u,diff_v;
  int impix;

  //  Calculate  PSNR for Y, U and V.

  //     Luma.
  impix = img->height*img->width;

  diff_y=0;
  for (i=0; i < img->width; ++i)
  {
    for (j=0; j < img->height; ++j)
    {
      diff_y += img->quad[abs(imgY_org[j][i]-imgY[j][i])];
    }
  }

  //     Chroma.

  diff_u=0;
  diff_v=0;

  for (i=0; i < img->width_cr; i++)
  {
    for (j=0; j < img->height_cr; j++)
    {
      diff_u += img->quad[abs(imgUV_org[0][j][i]-imgUV[0][j][i])];
      diff_v += img->quad[abs(imgUV_org[1][j][i]-imgUV[1][j][i])];
    }
  }

  //  Collecting SNR statistics
  if (diff_y != 0)
  {
    snr->snr_y=(float)(10*log10(65025*(float)impix/(float)diff_y));        // luma snr for current frame
    snr->snr_u=(float)(10*log10(65025*(float)impix/(float)(4*diff_u)));    // u croma snr for current frame, 1/4 of luma samples
    snr->snr_v=(float)(10*log10(65025*(float)impix/(float)(4*diff_v)));    // v croma snr for current frame, 1/4 of luma samples
  }

  if (img->number == 0)
  {
    snr->snr_y1=(float)(10*log10(65025*(float)impix/(float)diff_y));       // keep luma snr for first frame
    snr->snr_u1=(float)(10*log10(65025*(float)impix/(float)(4*diff_u)));   // keep croma u snr for first frame
    snr->snr_v1=(float)(10*log10(65025*(float)impix/(float)(4*diff_v)));   // keep croma v snr for first frame
    snr->snr_ya=snr->snr_y1;
    snr->snr_ua=snr->snr_u1;
    snr->snr_va=snr->snr_v1;
  }
  // B pictures
  else
  {
    snr->snr_ya=(float)(snr->snr_ya*(img->number+Bframe_ctr)+snr->snr_y)/(img->number+Bframe_ctr+1);   // average snr lume for all frames inc. first
    snr->snr_ua=(float)(snr->snr_ua*(img->number+Bframe_ctr)+snr->snr_u)/(img->number+Bframe_ctr+1);   // average snr u croma for all frames inc. first
    snr->snr_va=(float)(snr->snr_va*(img->number+Bframe_ctr)+snr->snr_v)/(img->number+Bframe_ctr+1);   // average snr v croma for all frames inc. first
  }
}


/*!
 ************************************************************************
 * \brief
 *    Just a placebo
 ************************************************************************
 */
Boolean dummy_slice_too_big(int bits_slice)
{
  return FALSE;
}
