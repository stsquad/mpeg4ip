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
 ***********************************************************************
 *  \mainpage
 *     This is the H.26L encoder reference software. For detailed documentation
 *     see the comments in each file.
 *
 *  \author
 *     The main contributors are listed in contributors.h
 *
 *  \version
 *     TML 9.4
 *
 *  \note
 *     tags are used for document system "doxygen"
 *     available at http://www.doxygen.org
 */
/*!
 *  \file
 *     lencod.c
 *  \brief
 *     TML encoder project main
 *  \author
 *   Main contributors (see contributors.h for copyright, address and affiliation details)
 *   - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *   - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *   - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *   - Jani Lainema                    <jani.lainema@nokia.com>
 *   - Byeong-Moon Jeon                <jeonbm@lge.com>
 *   - Yoon-Seong Soh                  <yunsung@lge.com>
 *   - Thomas Stockhammer              <stockhammer@ei.tum.de>
 *   - Detlev Marpe                    <marpe@hhi.de>
 *   - Guido Heising                   <heising@hhi.de>
 *
 ***********************************************************************
 */

#include "contributors.h"

#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdlib.h>
#if defined WIN32
  #include <conio.h>
#endif
#include "global.h"
#include "configfile.h"
#include "leaky_bucket.h"

#define TML     "9"
#define VERSION "9.40"

InputParameters inputs, *input = &inputs;
ImageParameters images, *img   = &images;
StatParameters  stats,  *h26lstat  = &stats;
SNRParameters   snrs,   *snr   = &snrs;

#ifdef _ADAPT_LAST_GROUP_
int initial_Bframes = 0;
#endif

/*!
 ***********************************************************************
 * \brief
 *    Main function for encoder.
 * \param argc
 *    number of command line arguments
 * \param argv
 *    command line arguments
 * \return
 *    exit code
 ***********************************************************************
 */
int main(int argc,char **argv)
{
  int image_type;
  
  p_dec = p_dec_u = p_dec_v = p_stat = p_log = p_datpart = p_trace = NULL;

  Configure (argc, argv);

  // Initialize Image Parameters
  init_img();

  // Allocate Slice data struct
  malloc_slice();

  // create and init structure for rd-opt. mode decision
  init_rdopt ();

#ifdef _FAST_FULL_ME_
  // create arrays for fast full motion estimation
  InitializeFastFullIntegerSearch (input->search_range);
#endif

  // Initialize Statistic Parameters
  init_stat();

  // allocate memory for frame buffers
  get_mem4global_buffers();

  // Just some information which goes to the standard output
  information_init();

  // Write sequence header; open bitstream files
  h26lstat->bit_slice = start_sequence();
  // B pictures
  Bframe_ctr=0;
  img->refPicID=-1;  //WYK: Oct. 16, 2001
  tot_time=0;                 // time for total encoding session

#ifdef _ADAPT_LAST_GROUP_
  if (input->last_frame > 0)
    input->no_frames = 1 + (input->last_frame + input->jumpd) / (input->jumpd + 1);
  initial_Bframes = input->successive_Bframe;
#endif

  for (img->number=0; img->number < input->no_frames; img->number++)
  {
    if (input->intra_period == 0)
    {
      if (img->number == 0)
      {
        img->type = INTRA_IMG;        // set image type for first image to I-frame
      }
      else
      {
        img->type = INTER_IMG;        // P-frame
        if (input->sp_periodicity)
        {
          if ((img->number % input->sp_periodicity) ==0)
          {
            img->types=SP_IMG;
          }
          else img->types=INTER_IMG;
        }
      }
    }
    else
    {
      if ((img->number%input->intra_period) == 0)
      {
        img->type = INTRA_IMG;
      }
      else
      {
        img->type = INTER_IMG;        // P-frame
        if (input->sp_periodicity)
        {
          if ((img->number % input->sp_periodicity) ==0)
              img->types=SP_IMG;
          else img->types=INTER_IMG;
        }
      }
    }

#ifdef _ADAPT_LAST_GROUP_
    if (input->successive_Bframe && input->last_frame && img->number+1 == input->no_frames)
      {
        int bi = (int)((float)(input->jumpd+1)/(input->successive_Bframe+1.0)+0.499999);
        input->successive_Bframe = (input->last_frame-(img->number-1)*(input->jumpd+1))/bi-1;
      }
#endif

    image_type = img->type;
    
    encode_one_frame(); // encode one I- or P-frame
    if ((input->successive_Bframe != 0) && (img->number > 0)) // B-frame(s) to encode
    {
      img->type = B_IMG;            // set image type to B-frame
      img->types= INTER_IMG;
      for(img->b_frame_to_code=1; img->b_frame_to_code<=input->successive_Bframe; img->b_frame_to_code++)
        encode_one_frame();  // encode one B-frame
    }
    
    if (image_type == INTRA_IMG || img->types==SP_IMG)
    {
        img->nb_references = 1;
    }
    else
    {
       img->nb_references += 1;
       img->nb_references = min(img->nb_references, img->buf_cycle);
    }
  }

    // terminate sequence
  terminate_sequence();

#ifdef _FAST_FULL_ME_
  // free arrays for fast full motion estimation
  ClearFastFullIntegerSearch ();
#endif

  // free structure for rd-opt. mode decision
  clear_rdopt ();

#ifdef _LEAKYBUCKET_
  calc_buffer();
#endif

  // report everything
  report();

  free_slice();

  // free allocated memory for frame buffers
  free_mem4global_buffers();

  // free image mem
  free_img ();

  return 0;
}

/*!
 ***********************************************************************
 * \brief
 *    Initializes the Image structure with appropriate parameters.
 * \par Input:
 *    Input Parameters struct inp_par *inp
 * \par  Output:
 *    Image Parameters struct img_par *img
 ***********************************************************************
 */
void init_img()
{
  int i,j;

  img->no_multpred=input->no_multpred;
#ifdef _ADDITIONAL_REFERENCE_FRAME_
  img->buf_cycle = max (input->no_multpred, input->add_ref_frame+1);
#else
  img->buf_cycle = input->no_multpred;
#endif
  img->framerate=INIT_FRAME_RATE;   // The basic frame rate (of the original sequence)

  get_mem_mv (&(img->mv));
  get_mem_mv (&(img->p_fwMV));
  get_mem_mv (&(img->p_bwMV));
  get_mem_mv (&(img->all_mv));
  get_mem_mv (&(img->all_bmv));

  img->width    = input->img_width;
  img->height   = input->img_height;
  img->width_cr = input->img_width/2;
  img->height_cr= input->img_height/2;

  if(((img->mb_data) = (Macroblock *) calloc((img->width/MB_BLOCK_SIZE) * (img->height/MB_BLOCK_SIZE),sizeof(Macroblock))) == NULL)
    no_mem_exit("init_img: img->mb_data");

  if(((img->slice_numbers) = (int *) calloc((img->width/MB_BLOCK_SIZE) * (img->height/MB_BLOCK_SIZE),sizeof(int))) == NULL)
    no_mem_exit("init_img: img->slice_numbers");

  if(input->UseConstrainedIntraPred)
  {
    if(((img->intra_mb) = (int *) calloc((img->width/MB_BLOCK_SIZE) * (img->height/MB_BLOCK_SIZE),sizeof(int))) == NULL)
      no_mem_exit("init_img: img->intra_mb");
  }

  init(img);

  // allocate memory for intra pred mode buffer for each block: img->ipredmode
  // int  img->ipredmode[90][74];
  get_mem2Dint(&(img->ipredmode), img->width/BLOCK_SIZE+2, img->height/BLOCK_SIZE+2);

  // Prediction mode is set to -1 outside the frame, indicating that no prediction can be made from this part
  for (i=0; i < img->width/BLOCK_SIZE+1; i++)
  {
    img->ipredmode[i+1][0]=-1;
  }
  for (j=0; j < img->height/BLOCK_SIZE+1; j++)
  {
    img->ipredmode[0][j+1]=-1;
  }

  img->mb_y_upd=0;

}

/*!
 ***********************************************************************
 * \brief
 *    Free the Image structures
 * \par Input:
 *    Image Parameters struct img_par *img
 ***********************************************************************
 */
void free_img ()
{
  free_mem_mv (img->mv);
  free_mem_mv (img->p_fwMV);
  free_mem_mv (img->p_bwMV);
  free_mem_mv (img->all_mv);
  free_mem_mv (img->all_bmv);
}


/*!
 ************************************************************************
 * \brief
 *    Allocates the slice structure along with its dependent
 *    data structures
 * \par Input:
 *    Input Parameters struct inp_par *inp,  struct img_par *img
 ************************************************************************
 */
void malloc_slice()
{
  int i;
  DataPartition *dataPart;
  Slice *currSlice;
  const int buffer_size = (img->width * img->height * 3)/2; // DM 070301: The only assumption here is that we
                                // do not consider the case of data expansion.
                                // So this is a strictly conservative estimation
                                // of the size of the bitstream buffer for one frame */                                                       /* ~DM

  switch(input->of_mode) // init depending on NAL mode
  {
    case PAR_OF_26L:
      // Current File Format
      img->currentSlice = (Slice *) calloc(1, sizeof(Slice));
      if ( (currSlice = img->currentSlice) == NULL)
      {
        snprintf (errortext, ET_SIZE, "Memory allocation for Slice datastruct in NAL-mode %d failed", input->of_mode);
        error(errortext, 600);
      }
      if (input->symbol_mode == CABAC)
      {
        // create all context models
        currSlice->mot_ctx = create_contexts_MotionInfo();
        currSlice->tex_ctx = create_contexts_TextureInfo();
      }

      switch(input->partition_mode)
      {
      case PAR_DP_1:
        currSlice->max_part_nr = 1;
        break;
      case PAR_DP_3:
        error("Data Partitioning not supported with bit stream file format",600);
        break;
      default:
        error("Data Partitioning Mode not supported!",600);
        break;
      }


      currSlice->partArr = (DataPartition *) calloc(currSlice->max_part_nr, sizeof(DataPartition));
      if (currSlice->partArr == NULL)
      {
        snprintf(errortext, ET_SIZE, "Memory allocation for Data Partition datastruct in NAL-mode %d failed", input->of_mode);
        error(errortext, 100);
      }
      for (i=0; i<currSlice->max_part_nr; i++) // loop over all data partitions
      {
        dataPart = &(currSlice->partArr[i]);
        dataPart->bitstream = (Bitstream *) calloc(1, sizeof(Bitstream));
        if (dataPart->bitstream == NULL)
        {
          snprintf(errortext, ET_SIZE, "Memory allocation for Bitstream datastruct in NAL-mode %d failed", input->of_mode);
          error (errortext, 100);
        }
        dataPart->bitstream->streamBuffer = (byte *) calloc(buffer_size, sizeof(byte));
        if (dataPart->bitstream->streamBuffer == NULL)
        {
          snprintf(errortext, ET_SIZE, "Memory allocation for bitstream buffer in NAL-mode %d failed", input->of_mode);
          error (errortext, 100);
        }
        // Initialize storage of bitstream parameters
        dataPart->bitstream->stored_bits_to_go = 8;
        dataPart->bitstream->stored_byte_pos = 0;
        dataPart->bitstream->stored_byte_buf = 0;
      }
      return;
    case PAR_OF_RTP:
      // RTP packet file format
      img->currentSlice = (Slice *) calloc(1, sizeof(Slice));
      if ( (currSlice = img->currentSlice) == NULL)
      {
        snprintf(errortext, ET_SIZE, "Memory allocation for Slice datastruct in NAL-mode %d failed", input->of_mode);
        error(errortext, 100);
      }
      if (input->symbol_mode == CABAC)
      {
        // create all context models
        currSlice->mot_ctx = create_contexts_MotionInfo();
        currSlice->tex_ctx = create_contexts_TextureInfo();
      }
      switch(input->partition_mode)
      {
      case PAR_DP_1:
        currSlice->max_part_nr = 1;
        break;
      case PAR_DP_3:
        currSlice->max_part_nr = 3;
        break;
      default:
        error("Data Partitioning Mode not supported!",600);
        break;
      }

      currSlice->partArr = (DataPartition *) calloc(currSlice->max_part_nr, sizeof(DataPartition));
      if (currSlice->partArr == NULL)
      {
        snprintf(errortext, ET_SIZE, "Memory allocation for Data Partition datastruct in NAL-mode %d failed", input->of_mode);
        error(errortext, 100);
      }

      for (i=0; i<currSlice->max_part_nr; i++) // loop over all data partitions
      {
        dataPart = &(currSlice->partArr[i]);
        dataPart->bitstream = (Bitstream *) calloc(1, sizeof(Bitstream));
        if (dataPart->bitstream == NULL)
        {
          snprintf(errortext, ET_SIZE, "Memory allocation for Bitstream datastruct in NAL-mode %d failed", input->of_mode);
          error(errortext, 100);
        }
        dataPart->bitstream->streamBuffer = (byte *) calloc(buffer_size, sizeof(byte));
        if (dataPart->bitstream->streamBuffer == NULL)
        {
          snprintf(errortext, ET_SIZE, "Memory allocation for bitstream buffer in NAL-mode %d failed", input->of_mode);
          error(errortext, 100);
        }
        // Initialize storage of bitstream parameters
        dataPart->bitstream->stored_bits_to_go = 8;
        dataPart->bitstream->stored_byte_pos = 0;
        dataPart->bitstream->stored_byte_buf = 0;
      }
      return;

    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext, 600);
  }

}

/*!
 ************************************************************************
 * \brief
 *    Memory frees of the Slice structure and of its dependent
 *    data structures
 * \par Input:
 *    Input Parameters struct inp_par *inp,  struct img_par *img
 ************************************************************************
 */
void free_slice()
{
  int i;
  DataPartition *dataPart;
  Slice *currSlice = img->currentSlice;

  for (i=0; i<currSlice->max_part_nr; i++) // loop over all data partitions
  {
    dataPart = &(currSlice->partArr[i]);
    if (dataPart->bitstream->streamBuffer != NULL)
      free(dataPart->bitstream->streamBuffer);
    if (dataPart->bitstream != NULL)
      free(dataPart->bitstream);
  }
  if (currSlice->partArr != NULL)
    free(currSlice->partArr);
  if (input->symbol_mode == CABAC)
  {
    // delete all context models
    delete_contexts_MotionInfo(currSlice->mot_ctx);
    delete_contexts_TextureInfo(currSlice->tex_ctx);
  }
  if (currSlice != NULL)
    free(img->currentSlice);
}


/*!
 ************************************************************************
 * \brief
 *    Initializes the Statistics structure with appropriate parameters
 * \par Input:
 *    Statistic Parameters struct stat_par *stat
 * \par Output:
 *    Statistic Parameters struct stat_par *stat
 ************************************************************************
 */
void init_stat()
{
  int i;
  if ((h26lstat->mode_use_Bframe = (int *)malloc(sizeof(int)*41))==NULL)
    no_mem_exit("init_stat: h26lstat->mode_use_Bframe");
  if ((h26lstat->bit_use_mode_Bframe =  (int *)malloc(sizeof(int)*41))==NULL)
    no_mem_exit("init_stat: h26lstat->bit_use_mode_Bframe");
  for(i=0; i<41; i++)
    h26lstat->mode_use_Bframe[i]=h26lstat->bit_use_mode_Bframe[i]=0;
}

/*!
 ************************************************************************
 * \brief
 *    Reports the gathered information to appropriate outputs
 * \par Input:
 *    struct inp_par *inp,                                            \n
 *    struct img_par *img,                                            \n
 *    struct stat_par *stat,                                          \n
 *    struct stat_par *stat                                           \n
 *
 * \par Output:
 *    None
 ************************************************************************
 */
void report()
{
  int bit_use[2][2] ;
  int i,j;
  char name[20];
  int bit_use_Bframe=0;
  int total_bits;
  float frame_rate;

#ifndef WIN32
  time_t now;
  struct tm *l_time;
  char string[1000];
#else
  char timebuf[128];
#endif
  bit_use[0][0]=1;
  bit_use[1][0]=max(1,input->no_frames-1);

  //  Accumulate bit usage for inter and intra frames
  bit_use[0][1]=bit_use[1][1]=0;

  for (i=0; i < 9; i++)
    bit_use[1][1] += h26lstat->bit_use_mode_inter[i];

  for (j=0;j<2;j++)
  {
    bit_use[j][1]+=h26lstat->bit_use_header[j];
    bit_use[j][1]+=h26lstat->bit_use_mb_type[j];
    bit_use[j][1]+=h26lstat->tmp_bit_use_cbp[j];
    bit_use[j][1]+=h26lstat->bit_use_coeffY[j];
    bit_use[j][1]+=h26lstat->bit_use_coeffC[j];
    bit_use[j][1]+=h26lstat->bit_use_delta_quant[j];
  }

  // B pictures
  if(Bframe_ctr!=0)
  {
    bit_use_Bframe=0;
    for(i=0; i<41; i++)
      bit_use_Bframe += h26lstat->bit_use_mode_Bframe[i]; // fw_predframe_no, blk_size
    bit_use_Bframe += h26lstat->bit_use_header[2];
    bit_use_Bframe += h26lstat->tmp_bit_use_cbp[2];
    bit_use_Bframe += h26lstat->bit_use_coeffY[2];
    bit_use_Bframe += h26lstat->bit_use_coeffC[2];
    bit_use_Bframe += h26lstat->bit_use_delta_quant[2];

    h26lstat->bitrate_P=(h26lstat->bit_ctr_0+h26lstat->bit_ctr_P)*(float)(img->framerate/(input->jumpd+1))/input->no_frames;
#ifdef _ADAPT_LAST_GROUP_
    h26lstat->bitrate_B=(h26lstat->bit_ctr_B)*(float)(img->framerate/(input->jumpd+1))*initial_Bframes/Bframe_ctr;
#else
    h26lstat->bitrate_B=(h26lstat->bit_ctr_B)*(float)(img->framerate/(input->jumpd+1))*input->successive_Bframe/Bframe_ctr;
#endif
  }
  else
  {
    if (input->no_frames > 1)
    {
      h26lstat->bitrate=(bit_use[0][1]+bit_use[1][1])*(float)img->framerate/(input->no_frames*(input->jumpd+1));
    }
  }

  fprintf(stdout,"--------------------------------------------------------------------------\n");
  fprintf(stdout,   " Freq. for encoded bitstream       : %1.0f\n",(float)img->framerate/(float)(input->jumpd+1));
  if(input->hadamard)
    fprintf(stdout," Hadamard transform                : Used\n");
  else
    fprintf(stdout," Hadamard transform                : Not used\n");

  fprintf(stdout," Image format                      : %dx%d\n",input->img_width,input->img_height);

  if(input->intra_upd)
    fprintf(stdout," Error robustness                  : On\n");
  else
    fprintf(stdout," Error robustness                  : Off\n");
  fprintf(stdout,    " Search range                      : %d\n",input->search_range);

  if(input->mv_res)
    fprintf(stdout," MV resolution                     : 1/8-pel\n");
  else
    fprintf(stdout," MV resolution                     : 1/4-pel\n");

#ifdef _ADDITIONAL_REFERENCE_FRAME_
  if (input->add_ref_frame >= input->no_multpred)
  {
      fprintf(stdout,   " No of ref. frames used in P pred  : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
      if(input->successive_Bframe != 0)
        fprintf(stdout,   " No of ref. frames used in B pred  : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
  }
  else
#endif
  {
    fprintf(stdout,   " No of ref. frames used in P pred  : %d\n",input->no_multpred);
    if(input->successive_Bframe != 0)
      fprintf(stdout,   " No of ref. frames used in B pred  : %d\n",input->no_multpred);
  }
  fprintf(stdout,   " Total encoding time for the seq.  : %.3f sec \n",tot_time*0.001);

  // B pictures
  fprintf(stdout, " Sequence type                     :" );
  if(input->successive_Bframe==1)   fprintf(stdout, " IBPBP (QP: I %d, P %d, B %d) \n",
    input->qp0, input->qpN, input->qpB);
  else if(input->successive_Bframe==2) fprintf(stdout, " IBBPBBP (QP: I %d, P %d, B %d) \n",
    input->qp0, input->qpN, input->qpB);
  else if(input->successive_Bframe==0 && input->sp_periodicity==0) fprintf(stdout, " IPPP (QP: I %d, P %d) \n",   input->qp0, input->qpN);
  else fprintf(stdout, " I-P-P-SP-P (QP: I %d, P %d, SP (%d, %d)) \n",  input->qp0, input->qpN, input->qpsp, input->qpsp_pred);

  // report on entropy coding  method
  if (input->symbol_mode == UVLC)
    fprintf(stdout," Entropy coding method             : UVLC\n");
  else
    fprintf(stdout," Entropy coding method             : CABAC\n");

#ifdef _FULL_SEARCH_RANGE_
  if (input->full_search == 2)
    fprintf(stdout," Search range restrictions         : none\n");
  else if (input->full_search == 1)
    fprintf(stdout," Search range restrictions         : older reference frames\n");
  else
    fprintf(stdout," Search range restrictions         : smaller blocks and older reference frames\n");
#endif

  if (input->rdopt)
    fprintf(stdout," RD-optimized mode decision        : used\n");
  else
    fprintf(stdout," RD-optimized mode decision        : not used\n");

  switch(input->partition_mode)
    {
    case PAR_DP_1:
      fprintf(stdout," Data Partitioning Mode            : 1 partition \n");
      break;
    case PAR_DP_3:
      fprintf(stdout," Data Partitioning Mode            : 3 partitions \n");
      break;
    default:
      fprintf(stdout," Data Partitioning Mode            : not supported\n");
      break;
    }


    switch(input->of_mode)
    {
    case PAR_OF_26L:
      fprintf(stdout," Output File Format                : H.26L Bit Stream File Format \n");
      break;
    case PAR_OF_RTP:
      fprintf(stdout," Output File Format                : RTP Packet File Format \n");
      break;
    default:
      fprintf(stdout," Output File Format                : not supported\n");
      break;
    }




  fprintf(stdout,"------------------ Average data all frames  ------------------------------\n");
  fprintf(stdout," SNR Y(dB)                         : %5.2f\n",snr->snr_ya);
  fprintf(stdout," SNR U(dB)                         : %5.2f\n",snr->snr_ua);
  fprintf(stdout," SNR V(dB)                         : %5.2f\n",snr->snr_va);

  if(Bframe_ctr!=0)
  {

    fprintf(stdout, " Total bits                        : %d (I %5d, P %5d, B %d) \n",
            total_bits=h26lstat->bit_ctr_P + h26lstat->bit_ctr_0 + h26lstat->bit_ctr_B, h26lstat->bit_ctr_0, h26lstat->bit_ctr_P, h26lstat->bit_ctr_B);

    frame_rate = (float)img->framerate / ( (float) (input->jumpd - input->successive_Bframe + 1) );
    h26lstat->bitrate= ((float) total_bits * frame_rate)/((float) (input->no_frames + Bframe_ctr));

    fprintf(stdout, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, h26lstat->bitrate/1000);

  }
  else if (input->sp_periodicity==0)
  {
    fprintf(stdout, " Total bits                        : %d (I %5d, P %5d) \n",
    total_bits=h26lstat->bit_ctr_P + h26lstat->bit_ctr_0 , h26lstat->bit_ctr_0, h26lstat->bit_ctr_P);

    frame_rate = (float)img->framerate / ( (float) (input->jumpd + 1) );
    h26lstat->bitrate= ((float) total_bits * frame_rate)/((float) input->no_frames );

    fprintf(stdout, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, h26lstat->bitrate/1000);
  }else
  {
    fprintf(stdout, " Total bits                        : %d (I %5d, P %5d) \n",
    total_bits=h26lstat->bit_ctr_P + h26lstat->bit_ctr_0 , h26lstat->bit_ctr_0, h26lstat->bit_ctr_P);

    frame_rate = (float)img->framerate / ( (float) (input->jumpd + 1) );
    h26lstat->bitrate= ((float) total_bits * frame_rate)/((float) input->no_frames );

    fprintf(stdout, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, h26lstat->bitrate/1000);
  }

  fprintf(stdout,"--------------------------------------------------------------------------\n");
  fprintf(stdout,"Exit TML %s encoder ver %s\n", TML, VERSION);

  // status file
  if ((p_stat=fopen("stat.dat","wb"))==0)
  {
    snprintf(errortext, ET_SIZE, "Error open file %s", "stat.dat");
    error(errortext, 500);
  }
  fprintf(p_stat," -------------------------------------------------------------- \n");
  fprintf(p_stat,"  This file contains statistics for the last encoded sequence   \n");
  fprintf(p_stat," -------------------------------------------------------------- \n");
  fprintf(p_stat,   " Sequence                     : %s\n",input->infile);
  fprintf(p_stat,   " No.of coded pictures         : %d\n",input->no_frames+Bframe_ctr);
  fprintf(p_stat,   " Freq. for encoded bitstream  : %3.0f\n",frame_rate);

  // B pictures
  if(input->successive_Bframe != 0)
  {
    fprintf(p_stat,   " BaseLayer Bitrate(kb/s)      : %6.2f\n", h26lstat->bitrate_P/1000);
    fprintf(p_stat,   " EnhancedLyaer Bitrate(kb/s)  : %6.2f\n", h26lstat->bitrate_B/1000);
  }
  else
    fprintf(p_stat,   " Bitrate(kb/s)                : %6.2f\n", h26lstat->bitrate/1000);

  if(input->hadamard)
    fprintf(p_stat," Hadamard transform           : Used\n");
  else
    fprintf(p_stat," Hadamard transform           : Not used\n");

  fprintf(p_stat," Image format                 : %dx%d\n",input->img_width,input->img_height);

  if(input->intra_upd)
    fprintf(p_stat," Error robustness             : On\n");
  else
    fprintf(p_stat," Error robustness             : Off\n");

  fprintf(p_stat,    " Search range                 : %d\n",input->search_range);

  if(input->mv_res)
    fprintf(p_stat," MV resolution                : 1/8-pel\n");
  else
    fprintf(p_stat," MV resolution                : 1/4-pel\n");

#ifdef _ADDITIONAL_REFERENCE_FRAME_
  if (input->add_ref_frame >= input->no_multpred)
    {
      fprintf(p_stat,   " No of frame used in P pred   : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
      if(input->successive_Bframe != 0)
        fprintf(p_stat,   " No of frame used in B pred   : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
    }
  else
#endif
  {
    fprintf(p_stat,   " No of frame used in P pred   : %d\n",input->no_multpred);
    if(input->successive_Bframe != 0)
      fprintf(p_stat,   " No of frame used in B pred   : %d\n",input->no_multpred);
  }
  if (input->symbol_mode == UVLC)
    fprintf(p_stat,   " Entropy coding method        : UVLC\n");
  else
    fprintf(p_stat,   " Entropy coding method        : CABAC\n");

#ifdef _FULL_SEARCH_RANGE_
  if (input->full_search == 2)
    fprintf(p_stat," Search range restrictions    : none\n");
  else if (input->full_search == 1)
    fprintf(p_stat," Search range restrictions    : older reference frames\n");
  else
    fprintf(p_stat," Search range restrictions    : smaller blocks and older reference frames\n");
#endif
  if (input->rdopt)
    fprintf(p_stat," RD-optimized mode decision   : used\n");
  else
    fprintf(p_stat," RD-optimized mode decision   : not used\n");

  fprintf(p_stat," -------------------|---------------|---------------|\n");
  fprintf(p_stat,"     Item           |     Intra     |   All frames  |\n");
  fprintf(p_stat," -------------------|---------------|---------------|\n");
  fprintf(p_stat," SNR Y(dB)          |");
  fprintf(p_stat," %5.2f         |",snr->snr_y1);
  fprintf(p_stat," %5.2f         |\n",snr->snr_ya);
  fprintf(p_stat," SNR U/V (dB)       |");
  fprintf(p_stat," %5.2f/%5.2f   |",snr->snr_u1,snr->snr_v1);
  fprintf(p_stat," %5.2f/%5.2f   |\n",snr->snr_ua,snr->snr_va);

  // QUANT.
  fprintf(p_stat," Average quant      |");
  fprintf(p_stat," %5d         |",absm(input->qp0));
  fprintf(p_stat," %5.2f         |\n",(float)h26lstat->quant1/max(1.0,(float)h26lstat->quant0));

  // MODE
  fprintf(p_stat,"\n -------------------|---------------|\n");
  fprintf(p_stat,"   Intra            |   Mode used   |\n");
  fprintf(p_stat," -------------------|---------------|\n");

  fprintf(p_stat," Mode 0  intra old\t| %5d         |\n",h26lstat->mode_use_intra[0]);
  for (i=1;i<24;i++)
    fprintf(p_stat," Mode %d intra new\t| %5d         |\n",i,h26lstat->mode_use_intra[i]);

  fprintf(p_stat,"\n -------------------|---------------|---------------|\n");
  fprintf(p_stat,"   Inter            |   Mode used   | Vector bit use|\n");
  fprintf(p_stat," -------------------|---------------|---------------|");

  fprintf(p_stat,"\n Mode 0  (copy)  \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[0],(float)h26lstat->bit_use_mode_inter[0]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 1  (16x16) \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[1],(float)h26lstat->bit_use_mode_inter[1]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 2  (16x8)  \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[2],(float)h26lstat->bit_use_mode_inter[2]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 3  (8x16)  \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[3],(float)h26lstat->bit_use_mode_inter[3]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 4  (8x8)   \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[4],(float)h26lstat->bit_use_mode_inter[4]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 5  (8x4)   \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[5],(float)h26lstat->bit_use_mode_inter[5]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 6  (4x8)   \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[6],(float)h26lstat->bit_use_mode_inter[6]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 7  (4x4)   \t| %5d         | %5.0f         |",h26lstat->mode_use_inter[7],(float)h26lstat->bit_use_mode_inter[7]/(float)bit_use[1][0]);
  fprintf(p_stat,"\n Mode 8 intra old\t| %5d         | --------------|",h26lstat->mode_use_inter[8]);
  for (i=9;i<33;i++)
    fprintf(p_stat,"\n Mode %d intr.new \t| %5d         |",i,h26lstat->mode_use_inter[i]);

  // B pictures
  if(input->successive_Bframe!=0 && Bframe_ctr!=0)
  {
    fprintf(p_stat,"\n\n -------------------|---------------|\n");
    fprintf(p_stat,"   B frame          |   Mode used   |\n");
    fprintf(p_stat," -------------------|---------------|");

    for(i=0; i<16; i++)
      fprintf(p_stat,"\n Mode %d   \t| %5d         |", i, h26lstat->mode_use_Bframe[i]);
    fprintf(p_stat,"\n Mode %d intra old\t| %5d     |", 16, h26lstat->mode_use_Bframe[16]);
    for (i=17; i<41; i++)
      fprintf(p_stat,"\n Mode %d intr.new \t| %5d     |",i, h26lstat->mode_use_Bframe[i]);
  }

  fprintf(p_stat,"\n\n -------------------|---------------|---------------|---------------|\n");
  fprintf(p_stat,"  Bit usage:        |     Intra     |    Inter      |   B frame     |\n");
  fprintf(p_stat," -------------------|---------------|---------------|---------------|\n");

  fprintf(p_stat," Header+modeinfo    |");
  fprintf(p_stat," %7d       |",h26lstat->bit_use_header[0]);
  fprintf(p_stat," %7d       |",h26lstat->bit_use_header[1]/bit_use[1][0]);
  if(input->successive_Bframe!=0 && Bframe_ctr!=0)
    fprintf(p_stat," %7d       |",h26lstat->bit_use_header[2]/Bframe_ctr);
  else fprintf(p_stat," %7d       |", 0);
  fprintf(p_stat,"\n");

  fprintf(p_stat," CBP Y/C            |");
  for (j=0; j < 2; j++)
  {
    fprintf(p_stat," %7.2f       |", (float)h26lstat->tmp_bit_use_cbp[j]/bit_use[j][0]);
  }
  if(input->successive_Bframe!=0 && Bframe_ctr!=0)
    fprintf(p_stat," %7.2f       |", (float)h26lstat->tmp_bit_use_cbp[2]/Bframe_ctr);
  else fprintf(p_stat," %7.2f       |", 0.);
  fprintf(p_stat,"\n");

  if(input->successive_Bframe!=0 && Bframe_ctr!=0)
    fprintf(p_stat," Coeffs. Y          | %7.2f      | %7.2f       | %7.2f       |\n",
      (float)h26lstat->bit_use_coeffY[0]/bit_use[0][0], (float)h26lstat->bit_use_coeffY[1]/bit_use[1][0], (float)h26lstat->bit_use_coeffY[2]/Bframe_ctr);
  else
    fprintf(p_stat," Coeffs. Y          | %7.2f      | %7.2f       | %7.2f       |\n",
      (float)h26lstat->bit_use_coeffY[0]/bit_use[0][0], (float)h26lstat->bit_use_coeffY[1]/bit_use[1][0], 0.);

  if(input->successive_Bframe!=0 && Bframe_ctr!=0)
    fprintf(p_stat," Coeffs. C          | %7.2f       | %7.2f       | %7.2f       |\n",
      (float)h26lstat->bit_use_coeffC[0]/bit_use[0][0], (float)h26lstat->bit_use_coeffC[1]/bit_use[1][0], (float)h26lstat->bit_use_coeffC[2]/Bframe_ctr);
  else
    fprintf(p_stat," Coeffs. C          | %7.2f       | %7.2f       | %7.2f       |\n",
      (float)h26lstat->bit_use_coeffC[0]/bit_use[0][0], (float)h26lstat->bit_use_coeffC[1]/bit_use[1][0], 0.);

  if(input->successive_Bframe!=0 && Bframe_ctr!=0)
    fprintf(p_stat," Delta quant        | %7.2f       | %7.2f       | %7.2f       |\n",
      (float)h26lstat->bit_use_delta_quant[0]/bit_use[0][0], (float)h26lstat->bit_use_delta_quant[1]/bit_use[1][0], (float)h26lstat->bit_use_delta_quant[2]/Bframe_ctr);
  else
    fprintf(p_stat," Delta quant        | %7.2f       | %7.2f       | %7.2f       |\n",
      (float)h26lstat->bit_use_delta_quant[0]/bit_use[0][0], (float)h26lstat->bit_use_delta_quant[1]/bit_use[1][0], 0.);


  fprintf(p_stat," -------------------|---------------|---------------|---------------|\n");

  fprintf(p_stat," average bits/frame |");
  for (i=0; i < 2; i++)
  {
    fprintf(p_stat," %7d       |",bit_use[i][1]/bit_use[i][0]);
  }
  if(input->successive_Bframe!=0 && Bframe_ctr!=0)
    fprintf(p_stat," %7d       |", bit_use_Bframe/Bframe_ctr);
  else fprintf(p_stat," %7d       |", 0);

  fprintf(p_stat,"\n");
  fprintf(p_stat," -------------------|---------------|---------------|---------------|\n");

  // write to log file
  if (fopen("log.dat","r")==0)                      // check if file exist
  {
    if ((p_log=fopen("log.dat","a"))==NULL)            // append new statistic at the end
    {
      snprintf(errortext, ET_SIZE, "Error open file %s  \n","log.dat");
      error(errortext, 500);
    }
    else                                            // Create header for new log file
    {
      fprintf(p_log," ---------------------------------------------------------------------------------------------------------------------------------------------------------------- \n");
      fprintf(p_log,"|            Encoder statistics. This file is generated during first encoding session, new sessions will be appended                                               |\n");
      fprintf(p_log," ---------------------------------------------------------------------------------------------------------------------------------------------------------------- \n");
      fprintf(p_log,"| Date  | Time  |    Sequence        |#Img|Quant1|QuantN|Format|Hadamard|Search r|#Ref |Freq |Intra upd|SNRY 1|SNRU 1|SNRV 1|SNRY N|SNRU N|SNRV N|#Bitr P|#Bitr B|\n");
      fprintf(p_log," ---------------------------------------------------------------------------------------------------------------------------------------------------------------- \n");
    }
  }
  else
    if ((p_log=fopen("log.dat","a"))==NULL)            // File exist,just open for appending
    {
      snprintf(errortext, ET_SIZE, "Error open file %s  \n","log.dat");
      error(errortext, 500);
    }

#ifdef WIN32
  _strdate( timebuf );
  fprintf(p_log,"| %1.5s |",timebuf );

  _strtime( timebuf);
  fprintf(p_log," % 1.5s |",timebuf);
#else
  now = time ((time_t *) NULL); // Get the system time and put it into 'now' as 'calender time'
  time (&now);
  l_time = localtime (&now);
  strftime (string, sizeof string, "%d-%b-%Y", l_time);
  fprintf(p_log,"| %1.5s |",string );

  strftime (string, sizeof string, "%H:%M:%S", l_time);
  fprintf(p_log," %1.5s |",string );
#endif

  for (i=0;i<20;i++)
    name[i]=input->infile[i+max(0,strlen(input->infile)-20)]; // write last part of path, max 20 chars
  fprintf(p_log,"%20.20s|",name);

  fprintf(p_log,"%3d |",input->no_frames);
  fprintf(p_log,"  %2d  |",input->qp0);
  fprintf(p_log,"  %2d  |",input->qpN);


  fprintf(p_log,"%dx%d|",input->img_width,input->img_height);


  if (input->hadamard==1)
    fprintf(p_log,"   ON   |");
  else
    fprintf(p_log,"   OFF  |");

  fprintf(p_log,"   %2d   |",input->search_range );

  fprintf(p_log," %2d  |",input->no_multpred);

  fprintf(p_log," %2d  |",img->framerate/(input->jumpd+1));

  if (input->intra_upd==1)
    fprintf(p_log,"   ON    |");
  else
    fprintf(p_log,"   OFF   |");

  fprintf(p_log,"%5.3f|",snr->snr_y1);
  fprintf(p_log,"%5.3f|",snr->snr_u1);
  fprintf(p_log,"%5.3f|",snr->snr_v1);
  fprintf(p_log,"%5.3f|",snr->snr_ya);
  fprintf(p_log,"%5.3f|",snr->snr_ua);
  fprintf(p_log,"%5.3f|",snr->snr_va);
  if(input->successive_Bframe != 0)
  {
    fprintf(p_log,"%7.0f|",h26lstat->bitrate_P);
    fprintf(p_log,"%7.0f|\n",h26lstat->bitrate_B);
  }
  else
  {
    fprintf(p_log,"%7.0f|",h26lstat->bitrate);
    fprintf(p_log,"%7.0f|\n",0.0);
  }

  fclose(p_log);

  p_log=fopen("data.txt","a");

  if(input->successive_Bframe != 0 && Bframe_ctr != 0) // B picture used
  {
    fprintf(p_log, "%3d %2d %2d %2.2f %2.2f %2.2f %5d "
          "%2.2f %2.2f %2.2f %5d "
        "%2.2f %2.2f %2.2f %5d %5d %.3f\n",
        input->no_frames, input->qp0, input->qpN,
        snr->snr_y1,
        snr->snr_u1,
        snr->snr_v1,
        h26lstat->bit_ctr_0,
        0.0,
        0.0,
        0.0,
        0,
        snr->snr_ya,
        snr->snr_ua,
        snr->snr_va,
        (h26lstat->bit_ctr_0+h26lstat->bit_ctr)/input->no_frames,
        h26lstat->bit_ctr_B/Bframe_ctr,
        (double)0.001*tot_time/(input->no_frames+Bframe_ctr));
  }
  else
  {
    fprintf(p_log, "%3d %2d %2d %2.2f %2.2f %2.2f %5d "
          "%2.2f %2.2f %2.2f %5d "
        "%2.2f %2.2f %2.2f %5d %5d %.3f\n",
        input->no_frames, input->qp0, input->qpN,
        snr->snr_y1,
        snr->snr_u1,
        snr->snr_v1,
        h26lstat->bit_ctr_0,
        0.0,
        0.0,
        0.0,
        0,
        snr->snr_ya,
        snr->snr_ua,
        snr->snr_va,
        (h26lstat->bit_ctr_0+h26lstat->bit_ctr)/input->no_frames,
        0,
        (double)0.001*tot_time/input->no_frames);
  }

  fclose(p_log);

  // B pictures
  free(h26lstat->mode_use_Bframe);
  free(h26lstat->bit_use_mode_Bframe);
}

/*!
 ************************************************************************
 * \brief
 *    Some matrices are initilized here.
 * \par Input:
 *    none
 * \par Output:
 *    none
 ************************************************************************
 */
void init()
{
  int i, ii, ind, j, i2;

  InitMotionVectorSearchModule();

  /*  img->mv_bituse[] is the number of bits used for motion vectors.
  It is used to add a portion to the SAD depending on bit usage in the motion search
  */

  img->mv_bituse[0]=1;
  ind=0;
  for (i=0; i < 9; i++)
  {
    ii= 2*i + 3;
    for (j=1; j <= (int)pow(2,i); j++)
    {
      ind++;
      img->mv_bituse[ind]=ii;
    }
  }

  // quad(0:255) SNR quad array
  for (i=0; i < 256; ++i) // fix from TML1 / TML2 sw, truncation removed
  {
    i2=i*i;
    img->quad[i]=i2;
  }

  // B pictures : img->blk_bitsue[] is used when getting bidirection SAD
  for (i=0; i < 7; i++)
  {
    if(i==0) img->blk_bituse[i]=1;
    else if(i==1 || i==2) img->blk_bituse[i]=3;
    else img->blk_bituse[i]=5;
  }
}

/*!
 ************************************************************************
 * \brief
 *    Prints the header of the protocol.
 * \par Input:
 *    struct inp_par *inp
 * \par Output:
 *    none
 ************************************************************************
 */
void information_init()
{
  printf("--------------------------------------------------------------------------\n");
  printf(" Input YUV file                    : %s \n",input->infile);
  printf(" Output H.26L bitstream            : %s \n",input->outfile);
  if (p_dec != NULL)
    printf(" Output YUV file(debug)            : %s \n",input->ReconFile);
  printf(" Output log file                   : log.dat \n");
  printf(" Output statistics file            : stat.dat \n");
  printf("--------------------------------------------------------------------------\n");


  printf(" Frame   Bit/pic   QP   SnrY    SnrU    SnrV    Time(ms) IntraMBs\n");
}


/*!
 ************************************************************************
 * \brief
 *    Dynamic memory allocation of frame size related global buffers
 *    buffers are defined in global.h, allocated memory must be freed in
 *    void free_mem4global_buffers()
 * \par Input:
 *    Input Parameters struct inp_par *inp,                            \n
 *    Image Parameters struct img_par *img
 * \return Number of allocated bytes
 ************************************************************************
 */
int get_mem4global_buffers()
{
  int j,memory_size=0;
#ifdef _ADAPT_LAST_GROUP_
  extern int *last_P_no;

  if ((last_P_no = (int*)malloc(img->buf_cycle*sizeof(int))) == NULL)
    no_mem_exit("get_mem4global_buffers: last_P_no");
#endif

  // allocate memory for encoding frame buffers: imgY, imgUV
  // byte imgY[288][352];
  // byte imgUV[2][144][176];
  memory_size += get_mem2D(&imgY, img->height, img->width);
  memory_size += get_mem3D(&imgUV, 2, img->height_cr, img->width_cr);

  // allocate memory for reference frame buffers: imgY_org, imgUV_org
  // byte imgY_org[288][352];
  // byte imgUV_org[2][144][176];
  memory_size += get_mem2D(&imgY_org, img->height, img->width);
  memory_size += get_mem3D(&imgUV_org, 2, img->height_cr, img->width_cr);

  // allocate memory for post filter frame buffers: imgY_pf, imgUV_pf
  // byte imgY_pf[288][352];
  // byte imgUV_pf[2][144][176];
  memory_size += get_mem2D(&imgY_pf, img->height, img->width);
  memory_size += get_mem3D(&imgUV_pf, 2, img->height_cr, img->width_cr);

  // allocate memory for B frame coding: nextP_imgY, nextP_imgUV
  // byte nextP_imgY[288][352];
  // byte nextP_imgUV[2][144][176];
  memory_size += get_mem2D(&nextP_imgY, img->height, img->width);
  memory_size += get_mem3D(&nextP_imgUV, 2, img->height_cr, img->width_cr);

  // allocate memory for multiple ref. frame buffers: mref, mcref
  //byte mref[MAX_MULT_PRED][1152][1408];  */   /* 1/4 pix luma
  //byte mcef[MAX_MULT_PRED][2][352][288]; */   /* pix chroma
  // rows and cols for croma component mcef[ref][croma][4x][4y] are switched
  // compared to luma mref[ref][4y][4x] for whatever reason
  // number of reference frames increased by one for next P-frame
  memory_size += get_mem3D(&mref, img->buf_cycle+1, (img->height+2*IMG_PAD_SIZE)*4, (img->width+2*IMG_PAD_SIZE)*4);
  if((mcef = (byte****)calloc(img->buf_cycle+1,sizeof(byte***))) == NULL)
    no_mem_exit("get_mem4global_buffers: mcef");
  for(j=0;j<img->buf_cycle+1;j++)
  {
    memory_size += get_mem3D(&(mcef[j]), 2, img->height_cr, img->width_cr);
  }

  InitRefbuf (Refbuf11, Refbuf11_P);


  // allocate memory for B-frame coding (last upsampled P-frame): mref_P, mcref_P
  // is currently also used in oneforthpix_1() and _2() for coding without B-frames
  //byte mref[1152][1408];  */   /* 1/4 pix luma
  //byte mcef[2][352][288]; */   /* pix chroma
  memory_size += get_mem2D(&mref_P, (img->height+2*IMG_PAD_SIZE)*4, (img->width+2*IMG_PAD_SIZE)*4);
  memory_size += get_mem3D(&mcef_P, 2, img->height_cr, img->width_cr);

  if(input->successive_Bframe!=0)
  {
    // allocate memory for temp B-frame motion vector buffer: tmp_fwMV, tmp_bwMV, dfMV, dbMV
    // int ...MV[2][72][92];  ([2][72][88] should be enough)
    memory_size += get_mem3Dint(&tmp_fwMV, 2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
    memory_size += get_mem3Dint(&tmp_bwMV, 2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
    memory_size += get_mem3Dint(&dfMV,     2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
    memory_size += get_mem3Dint(&dbMV,     2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);

    // allocate memory for temp B-frame motion vector buffer: fw_refFrArr, bw_refFrArr
    // int ...refFrArr[72][88];
    memory_size += get_mem2Dint(&fw_refFrArr, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE);
    memory_size += get_mem2Dint(&bw_refFrArr, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE);
  }

  // allocate memory for temp P and B-frame motion vector buffer: tmp_mv, temp_mv_block
  // int tmp_mv[2][72][92];  ([2][72][88] should be enough)
  memory_size += get_mem3Dint(&tmp_mv, 2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);

  // allocate memory for temp quarter pel luma frame buffer: img4Y_tmp
  // int img4Y_tmp[576][704];  (previously int imgY_tmp in global.h)
  memory_size += get_mem2Dint(&img4Y_tmp, img->height+2*IMG_PAD_SIZE, (img->width+2*IMG_PAD_SIZE)*4);

  // allocate memory for tmp loop filter frame buffers: imgY_tmp, imgUV_tmp
  // byte imgY_tmp[288][352];
  memory_size += get_mem2D(&imgY_tmp, img->height, img->width);
  memory_size += get_mem3D(&imgUV_tmp, 2, img->height_cr, img->width_cr);

  // allocate memory for reference frames of each block: refFrArr
  // int  refFrArr[72][88];
  memory_size += get_mem2Dint(&refFrArr, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE);

  if (input->rdopt==2)
  {
    memory_size += get_mem2Dint(&resY, MB_BLOCK_SIZE, MB_BLOCK_SIZE);
    if ((decref = (byte****) calloc(input->NoOfDecoders,sizeof(byte***))) == NULL) 
      no_mem_exit("get_mem4global_buffers: decref");
    for (j=0 ; j<input->NoOfDecoders; j++)
    {
      memory_size += get_mem3D(&decref[j], img->buf_cycle+1, img->height, img->width);
    }
    memory_size += get_mem2D(&RefBlock, BLOCK_SIZE,BLOCK_SIZE);
    memory_size += get_mem3D(&decY, input->NoOfDecoders, img->height, img->width);
    memory_size += get_mem3D(&decY_best, input->NoOfDecoders, img->height, img->width);
    memory_size += get_mem2D(&status_map, img->height/MB_BLOCK_SIZE,img->width/MB_BLOCK_SIZE);
  }

  return (memory_size);
}

/*!
 ************************************************************************
 * \brief
 *    Free allocated memory of frame size related global buffers
 *    buffers are defined in global.h, allocated memory is allocated in
 *    int get_mem4global_buffers()
 * \par Input:
 *    Input Parameters struct inp_par *inp,                             \n
 *    Image Parameters struct img_par *img
 * \par Output:
 *    none
 ************************************************************************
 */
void free_mem4global_buffers()
{
  int  i,j;

#ifdef _ADAPT_LAST_GROUP_
  extern int *last_P_no;
  free (last_P_no);
#endif

  free(imgY[0]);
  free(imgY);
  free(imgUV[0][0]);
  free(imgUV[1][0]);
  free(imgUV[0]);
  free(imgUV[1]);
  free(imgUV);
  free(imgY_org[0]);    // free ref frame buffers
  free(imgY_org);
  free(imgUV_org[0][0]);
  free(imgUV_org[1][0]);
  free(imgUV_org[0]);
  free(imgUV_org[1]);
  free(imgUV_org);
  free(imgY_pf[0]);  // free post filtering frame buffers
  free(imgY_pf);
  free(imgUV_pf[0][0]);
  free(imgUV_pf[1][0]);
  free(imgUV_pf[0]);
  free(imgUV_pf[1]);
  free(imgUV_pf);

  free(nextP_imgY[0]);    // free next frame buffers (for B frames)
  free(nextP_imgY);
  free(nextP_imgUV[0][0]);
  free(nextP_imgUV[1][0]);
  free(nextP_imgUV[0]);
  free(nextP_imgUV[1]);
  free(nextP_imgUV);


  free (Refbuf11_P);
  for (j=0; j<img->buf_cycle; j++)
    free (Refbuf11[j]);
  free (Refbuf11);

  // free multiple ref frame buffers
  // number of reference frames increased by one for next P-frame
  for(j=0;j<=img->buf_cycle;j++)
  {
    free(mref[j][0]);
    free(mref[j]);
    free(mcef[j][0][0]);
    free(mcef[j][1][0]);
    free(mcef[j][0]);
    free(mcef[j][1]);
    free(mcef[j]);
  }

  free(mref);
  free(mcef);
  free(mref_P[0]);
  free(mref_P);
  free(mcef_P[0][0]);
  free(mcef_P[1][0]);
  free(mcef_P[0]);
  free(mcef_P[1]);
  free(mcef_P);

  if(input->successive_Bframe!=0)
  {
    // free last P-frame buffers for B-frame coding
    for(j=0;j<2;j++)
    {
      free(tmp_fwMV[j][0]);
      free(tmp_bwMV[j][0]);
      free(dfMV[j][0]);
      free(dbMV[j][0]);
      free(tmp_fwMV[j]);
      free(tmp_bwMV[j]);
      free(dfMV[j]);
      free(dbMV[j]);
    }
    free(tmp_fwMV);
    free(tmp_bwMV);
    free(dfMV);
    free(dbMV);
    free(fw_refFrArr[0]);
    free(bw_refFrArr[0]);
    free(fw_refFrArr);
    free(bw_refFrArr);
  } // end if B frame


  free(tmp_mv[0][0]);
  free(tmp_mv[0]);
  free(tmp_mv[1][0]);
  free(tmp_mv[1]);
  free(tmp_mv);
  free(img4Y_tmp[0]);    // free temp quarter pel frame buffer
  free(img4Y_tmp);
  free(imgY_tmp[0]);    // free temp loop filter frame buffer
  free(imgY_tmp);
  free(imgUV_tmp[0][0]);
  free(imgUV_tmp[1][0]);
  free(imgUV_tmp[0]);
  free(imgUV_tmp[1]);
  free(imgUV_tmp);
  free(refFrArr[0]);
  free(refFrArr);

  // free mem, allocated in init_img()
  // free intra pred mode buffer for blocks
  free(img->ipredmode[0]);
  free(img->ipredmode);
  free(img->mb_data);
  free(img->slice_numbers);
  if(input->UseConstrainedIntraPred)
  {
    free(img->intra_mb);
  }

  if (input->rdopt==2)
  {
    free(resY[0]);
    free(resY);
    free(RefBlock[0]);
    free(RefBlock);
    for (j=0; j<input->NoOfDecoders; j++)
    {
      free(decY[j][0]);
      free(decY[j]);
      free(decY_best[j][0]);
      free(decY_best[j]);
      for (i=0; i<img->buf_cycle+1; i++)
      {
        free(decref[j][i][0]);
        free(decref[j][i]);
      }
      free(decref[j]);
    }
    free(decY);
    free(decY_best);
    free(decref);
    free(status_map[0]);
    free(status_map);
  }
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 2D memory array -> unsigned char array2D[rows][columns]
 * \par Input:
 *    none
 * \return memory size in bytes
 ************************************************************************
 */
// Change 9-Aug-2001 P. List: dont allocate independant row arrays anymore
// but one complete array and move row-pointers to array. Now you can step
// to the next line with an offset of img->width
int get_mem2D(byte ***array2D, int rows, int columns)
{
  int i;

  if((*array2D      = (byte**)calloc(rows,        sizeof(byte*))) == NULL)
    no_mem_exit("get_mem2D: array2D");
  if(((*array2D)[0] = (byte* )calloc(columns*rows,sizeof(byte ))) == NULL)
    no_mem_exit("get_mem2D: array2D");

  for(i=1;i<rows;i++)
    (*array2D)[i] = (*array2D)[i-1] + columns ;

  return rows*columns;
}


/*!
 ************************************************************************
 * \brief
 *    Allocate memory for mv
 * \par Input:
 *    Image Parameters struct img_par *img                             \n
 *    int****** mv
 * \return memory size in bytes
 ************************************************************************
 */
int get_mem_mv (int****** mv)
{
  int i, j, k, l;

  if ((*mv = (int*****)calloc(4,sizeof(int****))) == NULL)
    no_mem_exit ("get_mem_mv: mv");
  for (i=0; i<4; i++)
  {
    if (((*mv)[i] = (int****)calloc(4,sizeof(int***))) == NULL)
      no_mem_exit ("get_mem_mv: mv");
    for (j=0; j<4; j++)
    {
      if (((*mv)[i][j] = (int***)calloc(img->buf_cycle,sizeof(int**))) == NULL)
        no_mem_exit ("get_mem_mv: mv");
      for (k=0; k<img->buf_cycle; k++)
      {
        if (((*mv)[i][j][k] = (int**)calloc(9,sizeof(int*))) == NULL)
          no_mem_exit ("get_mem_mv: mv");
        for (l=0; l<9; l++)
          if (((*mv)[i][j][k][l] = (int*)calloc(2,sizeof(int))) == NULL)
            no_mem_exit ("get_mem_mv: mv");
      }
    }
  }
  return 4*4*img->buf_cycle*9*2*sizeof(int);
}

/*!
 ************************************************************************
 * \brief
 *    Free memory from mv
 * \par Input:
 *    int****** mv
 ************************************************************************
 */
void free_mem_mv (int***** mv)
{
  int i, j, k, l;

  for (i=0; i<4; i++)
  {
    for (j=0; j<4; j++)
    {
      for (k=0; k<img->buf_cycle; k++)
      {
        for (l=0; l<9; l++)
          free (mv[i][j][k][l]);
        free (mv[i][j][k]);
      }
      free (mv[i][j]);
    }
    free (mv[i]);
  }
  free (mv);
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 2D memory array -> int array2D[rows][columns]
 * \param rows
 *    number of rows
 * \param columns
 *    number of columns
 * \par Output:
 *    int ***array2D allocated memory
 * \return memory size in bytes
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem2Dint(int ***array2D, int rows, int columns)
{
  int i;

  if((*array2D      = (int**)calloc(rows,        sizeof(int*))) == NULL)
    no_mem_exit("get_mem2Dint:array2D");
  if(((*array2D)[0] = (int* )calloc(rows*columns,sizeof(int ))) == NULL)
    no_mem_exit("get_mem2Dint:array2D");

  for(i=1 ; i<rows ; i++)
    (*array2D)[i] =  (*array2D)[i-1] + columns  ;

  return rows*columns*sizeof(int);
}


/*!
 ************************************************************************
 * \brief
 *   Allocate 3D memory array -> unsigned char array3D[frames][rows][columns]
 * \param frames
 *    number of frames
 * \param rows
 *    number of rows
 * \param columns
 *    number of columns
 * \par Output:
 *    byte ****array3D allocated memory
 * \return memory size in bytes
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem3D(byte ****array3D, int frames, int rows, int columns)
{
  int  j;

  if(((*array3D) = (byte***)calloc(frames,sizeof(byte**))) == NULL)
    no_mem_exit("get_mem3D: array3D");

  for(j=0;j<frames;j++)
    get_mem2D( (*array3D)+j, rows, columns ) ;

  return frames*rows*columns;
}

/*!
 ************************************************************************
 * \brief
 *    Allocate 3D memory array -> int array3D[frames][rows][columns]
 * \param frames
 *    number of frames
 * \param rows
 *    number of rows
 * \param columns
 *    number of columns
 * \par Output:
 *    int ****array3D allocated memory
 * \return memory size in bytes
 ************************************************************************
 */
// same change as in get_mem2Dint
int get_mem3Dint(int ****array3D, int frames, int rows, int columns)
{
  int  j;

  if(((*array3D) = (int***)calloc(frames,sizeof(int**))) == NULL)
    no_mem_exit("get_mem3Dint: array3D");

  for(j=0;j<frames;j++)
    get_mem2Dint( (*array3D)+j, rows, columns ) ;


  return frames*rows*columns*sizeof(int);
}
/*!
 ************************************************************************
 * \brief
 *    Exit program if memory allocation failed (using error())
 * \param where
 *    string indicating which memory allocation failed
 ************************************************************************
 */
void no_mem_exit(char *where)
{
   snprintf(errortext, ET_SIZE, "Could not allocate memory: %s",where);
   error (errortext, 100);
}

