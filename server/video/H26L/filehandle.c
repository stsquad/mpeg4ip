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
 **************************************************************************************
 * \file
 *    filehandle.c
 * \brief
 *    Handles the operations how to write
 *    the generated symbols on the interim file format or some
 *    other specified output formats
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Thomas Stockhammer            <stockhammer@ei.tum.de>
 *      - Detlev Marpe                  <marpe@hhi.de>
 ***************************************************************************************
 */

#include "contributors.h"

#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "global.h"
#include "header.h"
#include "rtp.h"


// Global Varibales

static FILE *out;   //!< output file

/*
   The implemented solution for a unified picture header
*/

/*!
 ************************************************************************
 * \brief
 *    Error handling procedure. Print error message to stderr and exit
 *    with supplied code.
 * \param text
 *    Error message
 * \param code
 *    Exit code
 ************************************************************************
 */
void error(char *text, int code)
{
  fprintf(stderr, "%s\n", text);
  exit(code);
}


/*!
 ************************************************************************
 * \brief
 *    This function opens the output files and generates the
 *    appropriate sequence header
 ************************************************************************
 */
int start_sequence()
{
  int len;

  switch(input->of_mode)
  {
    case PAR_OF_26L:
#ifndef H26L_LIB
      if ((out=fopen(input->outfile,"wb"))==NULL)
      {
          snprintf(errortext, ET_SIZE, "Error open file %s  \n",input->outfile);
          error(errortext,1);
      }
#endif
      len = SequenceHeader(out);
      return 0;
    case PAR_OF_RTP:
      if ((out=fopen(input->outfile,"wb"))==NULL)
      {
          snprintf(errortext, ET_SIZE, "Error open file %s  \n",input->outfile);
          error(errortext,1);
      }
      len = SequenceHeader(out);
      return len;
      
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}

/*!
 ************************************************************************
 * \brief
 *     This function terminates the sequence and closes the
 *     output files
 ************************************************************************
 */
int terminate_sequence()
{
  Bitstream *currStream;

  // Mainly flushing of everything
  // Add termination symbol, etc.

  switch(input->of_mode)
  {
    case PAR_OF_26L:
      currStream = ((img->currentSlice)->partArr[0]).bitstream;
      if (input->symbol_mode == UVLC)
      {

        // Current TML File Format
        // Get the trailing bits of the last slice
        currStream->bits_to_go  = currStream->stored_bits_to_go;
        currStream->byte_pos    = currStream->stored_byte_pos;
        currStream->byte_buf    = currStream->stored_byte_buf;

        if (currStream->bits_to_go < 8)   // there are bits left in the last byte
          currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
        // Write all remaining bits to output bitstream file
#ifdef H26L_LIB
        memcpy(&memout[memoutlength], currStream->streamBuffer, 
			currStream->byte_pos);
		memoutlength += currStream->byte_pos;
#else
        fwrite (currStream->streamBuffer, 1, currStream->byte_pos, out);
        fclose(out);
#endif
      }
      else
      {
        // CABAC File Format
        fclose(out);
      }
      return 0;
    case PAR_OF_RTP:
      fclose (out);
      return 0;
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}

/*!
 ************************************************************************
 *  \brief
 *     This function generates the appropriate slice
 *     header
 *
 *  \return number of bits used for the picture header, including the PSC.
 *
 *  \par Side effects:
 *      Adds picture header symbols to the symbol buffer
 *
 *  \par Remarks:
 *      THIS IS AN INTERIM SOLUTION FOR A PICTURE HEADER, see VCEG-M79
 *                                                                               \par
 *      Generates the Picture Header out of the information in img_par and inp-par,
 *      and writes it to the symbol buffer.  The structure of the Picture Header
 *      is discussed in VCEG-M79.  It is implemented as discussed there.  In addition,
 *      it is preceeded by a picture start code, a UVLC-codeword of LEN=31 and INFO = 0.
 *      It would have been possible to put information into this codeword, similar
 *      to designs earlier than TML 5.9.  But it is deemed that the waste of 15
 *      data bits is acceptable considering the advantages of being able to search
 *      for a picture header quickly and easily, by looking for 30 consecutive, byte-
 *      aligned zero bits.
 *                                                                               \par
 *      The accounting of the header length (variable len) relies on the side effect
 *      of writeUVLCSymbol() that sets len and info in the symbol variable parameter
 ************************************************************************
*/

int start_slice(SyntaxElement *sym)
{
  EncodingEnvironmentPtr eep;
  Slice *currSlice = img->currentSlice;
  Bitstream *currStream;
  int header_len;
  int i;

  switch(input->of_mode)
  {
    case PAR_OF_26L:
      if (input->symbol_mode == UVLC)
      {
        currStream = (currSlice->partArr[0]).bitstream;
        // the information given in the SliceHeader is far not enought if we want to be able
        // to do some errorconcealment.
        // So write PictureHeader and SliceHeader in any case.
       header_len = PictureHeader();   // Picture Header
       header_len += SliceHeader (0);  // Slice Header without Start Code
       
        return header_len;
      }
      else
      {                   // H.26: CABAC File Format
        eep = &((currSlice->partArr[0]).ee_cabac);
        currStream = (currSlice->partArr[0]).bitstream;

        assert (currStream->bits_to_go == 8);
        assert (currStream->byte_buf == 0);
        assert (currStream->byte_pos == 0);
        memset(currStream->streamBuffer, 0, 12);    // fill first 12 bytes with zeros (debug only)

        // the information given in the SliceHeader is far not enought if we want to be able
        // to do some errorconcealment.
        // So write PictureHeader and SliceHeader in any case.
        header_len = PictureHeader();   // Picture Header
        header_len += SliceHeader (0);  // Slice Header without Start Code
        
        // Note that PictureHeader() and SliceHeader() set the buffer pointers as a side effect
        // Hence no need for adjusting it manually (and preserving space to be patched later

        // reserve bits for d_MB_Nr
        currStream->header_len = header_len;
        currStream->header_byte_buffer = currStream->byte_buf;

        currStream->byte_pos += ((31-currStream->bits_to_go)/8);
        if ((31-currStream->bits_to_go)%8 != 0)
          currStream->byte_pos++;
        currStream->bits_to_go = 8;
        currStream->byte_pos++;

        // If there is an absolute need to communicate the partition size, this would be
        // the space to insert it
        arienco_start_encoding(eep, currStream->streamBuffer, &(currStream->byte_pos));
        // initialize context models
        init_contexts_MotionInfo(currSlice->mot_ctx, 1);
        init_contexts_TextureInfo(currSlice->tex_ctx, 1);

        return header_len;

      }
    
    case PAR_OF_RTP:
      if (img->current_mb_nr == 0)                       // new picture
        RTPUpdateTimestamp (img->tr);

      if (input->symbol_mode == UVLC)
      {
        header_len=RTPSliceHeader();                      // generate the slice header
        currStream = currSlice->partArr[0].bitstream;
        
        if (header_len % 8 != 0)
        {
          currStream->byte_buf <<= currStream->bits_to_go;
          currStream->bits_to_go=8;
          currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
          currStream->byte_buf=0;
        }
        return header_len;
      }
      else
      {                   // RTP: CABAC 
        eep = &((currSlice->partArr[0]).ee_cabac);
        currStream = (currSlice->partArr[0]).bitstream;

        assert (currStream->bits_to_go == 8);
        assert (currStream->byte_buf == 0);


        header_len=RTPSliceHeader();                      // generate the slice header

        // reserve bits for d_MB_Nr
        currStream->header_len = header_len;
        currStream->header_byte_buffer = currStream->byte_buf;

        currStream->byte_pos += ((31-currStream->bits_to_go)/8);
        if ((31-currStream->bits_to_go)%8 != 0)
          currStream->byte_pos++;
        currStream->bits_to_go = 8;
        currStream->byte_pos++;

        arienco_start_encoding(eep, currStream->streamBuffer, &(currStream->byte_pos));
        
        if(input->partition_mode != PAR_DP_1)
        {
          for (i=1; i<currSlice->max_part_nr; i++)
          {
            eep = &((currSlice->partArr[0]).ee_cabac);
            currStream = (currSlice->partArr[0]).bitstream;
  
            assert (currStream->bits_to_go == 8);
            assert (currStream->byte_buf == 0);
            assert (currStream->byte_pos == 0);

            arienco_start_encoding(eep, currStream->streamBuffer, &(currStream->byte_pos));
          }
        }
        // initialize context models
        init_contexts_MotionInfo(currSlice->mot_ctx, 1);
        init_contexts_TextureInfo(currSlice->tex_ctx, 1);

        return header_len;
      }
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}


/*!
 ************************************************************************
 * \brief
 *    This function terminates a slice
 * \return
 *    0 if OK,                                                         \n
 *    1 in case of error
 ************************************************************************
 */
int terminate_slice()
{
  int bytes_written;
  Bitstream *currStream;
  Slice *currSlice = img->currentSlice;
  EncodingEnvironmentPtr eep;
  int i;
  int byte_pos, bits_to_go, start_data;
  byte buffer;
  int LastPartition;
  int cabac_byte_pos=0, uvlc_byte_pos=0, empty_bytes=0;
  int rtp_bytes_written;

  // Mainly flushing of everything
  // Add termination symbol, etc.
  switch(input->of_mode)
  {
    case PAR_OF_26L:


      if (input->symbol_mode == UVLC)
      {
        // Current TML File Format
        // Enforce byte alignment of next header: zero bit stuffing
        currStream = (currSlice->partArr[0]).bitstream;

        if (currStream->bits_to_go < 8)
        { // trailing bits to process
          currStream->byte_buf <<= currStream->bits_to_go;
          currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
          currStream->bits_to_go = 8;
        }

        bytes_written = currStream->byte_pos;
        stat->bit_ctr += 8*bytes_written;     // actually written bits
#ifdef H26L_LIB
        memcpy(&memout[memoutlength], currStream->streamBuffer, bytes_written);
		memoutlength += bytes_written;
#else
        fwrite (currStream->streamBuffer, 1, bytes_written, out);
#endif

        currStream->stored_bits_to_go = 8; // store bits_to_go
        currStream->stored_byte_buf   = currStream->byte_buf;   // store current byte
        currStream->stored_byte_pos   = 0; // reset byte position

      }
      else
      {

        // CABAC File Format
        eep = &((currSlice->partArr[0]).ee_cabac);
        currStream = (currSlice->partArr[0]).bitstream;

        // terminate the arithmetic code
        arienco_done_encoding(eep);

        // Add Number of MBs of this slice to the header
        // Save current state of Bitstream
        currStream = (currSlice->partArr[0]).bitstream;
        byte_pos = currStream->byte_pos;
        bits_to_go = currStream->bits_to_go;
        buffer = currStream->byte_buf;

        // Go to the reserved bits
        currStream->byte_pos = (currStream->header_len)/8;
        currStream->bits_to_go = 8-(currStream->header_len)%8;
        currStream->byte_buf = currStream->header_byte_buffer;

        // Ad Info about last MB
        LastMBInSlice();

        // And write the header to the output
        bytes_written = currStream->byte_pos;
        if (currStream->bits_to_go < 8) // trailing bits to process
        {
            currStream->byte_buf <<= currStream->bits_to_go;
            currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
            bytes_written++;
            currStream->bits_to_go = 8;
        }
#ifdef H26L_LIB
        memcpy(&memout[memoutlength], currStream->streamBuffer, bytes_written);
		memoutlength += bytes_written;
#else
        fwrite (currStream->streamBuffer, 1, bytes_written, out);
#endif
        stat->bit_ctr += 8*bytes_written;

        // Go back to the end of the stream
        currStream->byte_pos = byte_pos;
        currStream->bits_to_go = bits_to_go;
        currStream->byte_buf = buffer;


        // Find startposition of databitstream
        start_data = (currStream->header_len+31)/8;
        if ((currStream->header_len+31)%8 != 0)
          start_data++;
        bytes_written = currStream->byte_pos - start_data; // number of written bytes

        stat->bit_ctr += 8*bytes_written;     // actually written bits
#ifdef H26L_LIB
        memcpy(&memout[memoutlength], currStream->streamBuffer+start_data, 
			bytes_written);
		memoutlength += bytes_written;
#else
        fwrite ((currStream->streamBuffer+start_data), 1, bytes_written, out);
#endif
      }
      return 0;

    case PAR_OF_RTP:

      for (i=0; i<currSlice->max_part_nr; i++)
      {
        if (input->symbol_mode == CABAC)
        {
          eep = &((currSlice->partArr[i]).ee_cabac);
          arienco_done_encoding(eep);
        }
        currStream = (currSlice->partArr[i]).bitstream;
        
        if (currStream->bits_to_go < 8) // trailing bits to process
        {
          currStream->byte_buf <<= currStream->bits_to_go;
          currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
          currStream->bits_to_go = 8;
        }
        bytes_written = currStream->byte_pos; // number of written bytes

        assert (bytes_written != 0);
        if (bytes_written != 0)     // at least one bit in the partition -- something to do
          {
            int Marker;
            int FirstBytePacketType;

            // Determine First Byte of RTP payload
            switch (input->partition_mode)
            {
              case PAR_DP_1:
                FirstBytePacketType = 0;
                break;
              case PAR_DP_3:
                FirstBytePacketType = i+1;      // See VCEG-N72
                break;
              default:
                assert (0==1);
            }

            if (input->symbol_mode == CABAC && i == 0)
            {
              //! Add Number of MBs of this slice to the header
              //! Save current state of Bitstream
              byte_pos = currStream->byte_pos; //last byte in the stream

              //! Go to the reserved bits
              currStream->byte_pos = cabac_byte_pos = (currStream->header_len)/8;
              currStream->bits_to_go = 8-(currStream->header_len)%8;
              currStream->byte_buf = currStream->header_byte_buffer;

              cabac_byte_pos += ((31-currStream->bits_to_go)/8);
              if ((31-currStream->bits_to_go)%8 != 0)
                cabac_byte_pos++;
              cabac_byte_pos++; //! that's the position where we started to write CABAC code

              //! Ad Info about last MB 
              LastMBInSlice();

              if (currStream->bits_to_go < 8) //! trailing bits to process
              {
                currStream->byte_buf <<= currStream->bits_to_go;
                currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
              }

              uvlc_byte_pos = currStream->byte_pos; //! that's the first byte after the UVLC header data
              currStream->byte_pos = byte_pos;  //! where we were before this last_MB thing
              empty_bytes = cabac_byte_pos - uvlc_byte_pos; //! These bytes contain no information
                                                            //! They were reserved for writing the last_MB information but were not used

              for(byte_pos=uvlc_byte_pos; byte_pos<=currStream->byte_pos-empty_bytes; byte_pos++)
                currStream->streamBuffer[byte_pos]=currStream->streamBuffer[byte_pos+empty_bytes]; //shift the bitstreams
              bytes_written = byte_pos;
              //! TO 02.11.2001 I'm sure this can be done much more elegant, but that's the way it works. 
              //! If anybody understands what happens here please feel free to change this!
          }  

          // Calculate RTP payload's First Byte
          if (input->partition_mode==PAR_DP_1)
            FirstBytePacketType = 0;
          else
            FirstBytePacketType = i+1;

          assert (FirstBytePacketType == 0);    //! This has to go as soon as data partitioning is to be tested

          // Calculate the Marker bit
          LastPartition=0;
          if (input->partition_mode == PAR_DP_3)
          {
            if (currSlice->partArr[1].bitstream->byte_pos > 0)  // something in partition 1
              LastPartition=1;
            if (currSlice->partArr[2].bitstream->byte_pos > 0)  // something in partition 2
              LastPartition=2;
          }
          Marker = 0;
          if (img->current_mb_nr == img->total_number_mb)   // last MB is in this slice
            if ((input->partition_mode==PAR_DP_1) ||          // Single SLice
                (input->partition_mode==PAR_DP_3 && i == LastPartition ))   // Last partition containing bits
              Marker = 1;
          // and write the RTP packet
          rtp_bytes_written = RTPWriteBits (Marker, FirstBytePacketType, currStream->streamBuffer, bytes_written, out);
        }
        stat->bit_ctr += 8*bytes_written;
        // Provide the next partition with a 'fresh' buffer
        currStream->stored_bits_to_go = 8;
        currStream->stored_byte_buf   = 0;
        currStream->stored_byte_pos   = 0;
        currStream->bits_to_go = 8;
        currStream->byte_buf   = 0;
        currStream->byte_pos   = 0;

      }
    return 0;


    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}
