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
 *****************************************************************************
 *
 * \file rtp.c
 *
 * \brief
 *    Functions to handle RTP headers and packets per RFC1889 and RTP NAL spec
 *    Functions support little endian systems only (Intel, not Motorola/Sparc)
 *
 * \date
 *    30 September 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <memory.h>

#include "rtp.h"
#include "elements.h"
#include "defines.h"

#include "global.h"


// A little trick to avoid those horrible #if TRACE all over the source code
#if TRACE
#define SYMTRACESTRING(s) strncpy(sym.tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // to nothing
#endif


int CurrentRTPTimestamp = 0;      //! The RTP timestamp of the current packet,
                                  //! incremented with all P and I frames
int CurrentRTPSequenceNumber = 0; //! The RTP sequence number of the current packet
                                  //! incremented by one for each sent packet

/*!
 *****************************************************************************
 *
 * \brief 
 *    ComposeRTPpacket composes the complete RTP packet using the various
 *    structure members of the RTPpacket_t structure
 *
 * \return
 *    0 in case of success
 *    negative error code in case of failure
 *
 * \para Parameters
 *    Caller is responsible to allocate enough memory for the generated packet
 *    in parameter->packet. Typically a malloc of 12+paylen bytes is sufficient
 *
 * \para Side effects
 *    none
 *
 * \para Other Notes
 *    Function contains assert() tests for debug purposes (consistency checks
 *    for RTP header fields
 *
 * \date
 *    30 Spetember 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int ComposeRTPPacket (RTPpacket_t *p)

{
  // Consistency checks through assert, only used for debug purposes
  assert (p->v == 2);
  assert (p->p == 0);
  assert (p->x == 0);
  assert (p->cc == 0);    // mixer designers need to change this one
  assert (p->m == 0 || p->m == 1);
  assert (p->pt < 128);
  assert (p->seq < 65536);
  assert (p->payload != NULL);
  assert (p->paylen < 65536 - 40);  // 2**16 -40 for IP/UDP/RTP header
  assert (p->packet != NULL);

  // Compose RTP header, little endian

  p->packet[0] = (   (p->v)
                  |  (p->p << 2)
                  |  (p->x << 3)
                  |  (p->cc << 4) );
  p->packet[1] = (   (p->m)
                  |  (p->pt << 1) );
  p->packet[2] = p->seq & 0xff;
  p->packet[3] = (p->seq >> 8) & 0xff;

  memcpy (&p->packet[4], &p->timestamp, 4);  // change to shifts for unified byte sex
  memcpy (&p->packet[8], &p->ssrc, 4);// change to shifts for unified byte sex

  // Copy payload 

  memcpy (&p->packet[12], p->payload, p->paylen);
  p->packlen = p->paylen+12;
  return 0;
}



/*!
 *****************************************************************************
 *
 * \brief 
 *    WriteRTPPacket writes the supplied RTP packet to the output file
 *
 * \return
 *    0 in case of access
 *    <0 in case of write failure (typically fatal)
 *
 * \para Parameters
 *    p: the RTP packet to be written (after ComposeRTPPacket() )
 *    f: output file
 *
 * \para Side effects
 *    none
 *
 * \date
 *    October 23, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

int WriteRTPPacket (RTPpacket_t *p, FILE *f)

{
  int intime = -1;

  assert (f != NULL);
  assert (p != NULL);


  if (1 != fwrite (&p->packlen, 4, 1, f))
    return -1;
  if (1 != fwrite (&intime, 4, 1, f))
    return -1;
  if (1 != fwrite (p->packet, p->packlen, 1, f))
    return -1;
  return 0;
}


/*!
 *****************************************************************************
 *
 * \brief 
 *    ComposeHeaderPacketPayload generates the payload for a header packet
 *    using the inp-> structure contents.  The header packet contains 
 *    definitions for a single parameter set 0, which is used for all 
 *    slices of the picture
 *
 * \return
 *    len of the genberated payload in bytes
 *    <0 in case of failure (typically fatal)
 *
 * \para Parameters
 *    p: the payload of the RTP packet to be written
 *
 * \para Side effects
 *    none
 *
 * \note
 *    This function should be revisited and checked in case of additional
 *    bit stream parameters that affect the picture header (or higher
 *    entitries).  Typical examples are more entropy coding schemes, other
 *    motion vector resolutiuon, and optional elements
 *
 * \date
 *    October 23, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int ComposeHeaderPacketPayload (byte *payload)
{
  int slen=0;
  int multpred;

  assert (img->width%16 == 0);
  assert (img->height%16 == 0);

#ifdef _ADDITIONAL_REFERENCE_FRAME_
  if (input->no_multpred <= 1 && input->add_ref_frame == 0)
#else
  if (input->no_multpred <= 1)
#endif
    multpred=FALSE;
  else
    multpred=TRUE;               // multiple reference frames in motion search

  
  slen = snprintf (payload, MAXRTPPAYLOADLEN, 
             "a=H26L (0) MaxPicID %d\
            \na=H26L (0) UseMultpred %d\
            \na=H26L (0) BufCycle %d\
            \na=H26L (0) PixAspectRatioX 1\
            \na=H26L (0) PixAspectRatioY 1\
            \na=H26L (0) DisplayWindowOffsetTop 0\
            \na=H26L (0) DisplayWindowOffsetBottom 0\
            \na=H26L (0) DisplayWindowOffsetRight 0\
            \na=H26L (0) DisplayWindowOffsetLeft 0\
            \na=H26L (0) XSizeMB %d\
            \na=H26L (0) YSizeMB %d\
            \na=H26L (0) EntropyCoding %s\
            \na=H26L (0) MotionResolution %s\
            \na=H26L (0) PartitioningType %s\
            \na=H26L (0) IntraPredictionType %s\
            \na=H26L (0) HRCParameters 0\
            \
            \na=H26L (-1) FramesToBeEncoded %d\
            \na=H26L (-1) FrameSkip %d\
            \na=H26L (-1) SequenceFileName %s\
            %c%c",

            256,
            multpred==TRUE?1:0,
            input->no_multpred,
            input->img_width/16,
            input->img_height/16,
            input->symbol_mode==UVLC?"UVLC":"CABAC",
            input->mv_res==0?"quater":"eigth",
            input->partition_mode==0?"one":"three",
            input->UseConstrainedIntraPred==0?"InterPredicted":"NotInterPredicted",

            input->no_frames,
            input->jumpd,
            input->infile,
            4,     // Unix Control D EOF symbol
            26);  // MS-DOS/Windows Control Z EOF Symbol
  return slen;
}



/*!
 *****************************************************************************
 *
 * \brief 
 *    void RTPSequenceHeader (FILE *out) write the RTP Sequence header in the
 *       form of a header packet into the outfile.  It is assumed that the
 *       input-> structure is already filled (config file is parsed)
 *
 * \return
 *    length of sequence header
 *
 * \para Parameters
 *    out: fiel pointer of the output file
 *
 * \para Side effects
 *    header written, statistics are updated, RTP sequence number updated
 *
 * \para Note
 *    This function uses alloca() to reserve memry on the stack -- no freeing and
 *    no return value check necessary, error lead to stack overflows
 *
 * \date
 *    October 24, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

int RTPSequenceHeader (FILE *out)
{
  RTPpacket_t *p;

  assert (out != NULL);

  // Set RTP structure elements and alloca() memory foor the buffers
  p = (RTPpacket_t *) alloca (sizeof (RTPpacket_t));
  p->packet=alloca (MAXRTPPACKETSIZE);
  p->payload=alloca (MAXRTPPACKETSIZE);
  p->v=2;
  p->p=0;
  p->x=0;
  p->cc=0;
  p->m=1;
  p->pt=H26LPAYLOADTYPE;
  p->seq=CurrentRTPSequenceNumber++;
  p->timestamp=CurrentRTPTimestamp;
  p->ssrc=H26LSSRC;

  // Set First Byte of RTP payload, see VCEG-N72r1
  p->payload[0] = 
    6 |               // Parameter Update Packet
    0 |               // no know error
    0;                // reserved

  // Generate the Parameter update SDP syntax, donm't overwrite First Byte
  p->paylen = 1 + ComposeHeaderPacketPayload (&p->payload[1]);

  // Generate complete RTP packet
  if (ComposeRTPPacket (p) < 0)
  {
    printf ("Cannot compose RTP packet, exit\n");
    exit (-1);
  }

  if (WriteRTPPacket (p, out) < 0)
  {
    printf ("Cannot write %d bytes of RTP packet to outfile, exit\n", p->packlen);
    exit (-1);
  }
  return (p->packlen);
}



/*!
 *****************************************************************************
 *
 * \brief 
 *    int RTPSliceHeader () write the Slice header for the RTP NAL      
 *
 * \return
 *    Number of bits used by the slice header
 *
 * \para Parameters
 *    none
 *
 * \para Side effects
 *    Slice header as per VCEG-N72r2 is written into the appropriate partition
 *    bit buffer
 *
 * \para Limitations/Shortcomings/Tweaks
 *    The current code does not support the change of picture parameters within
 *    one coded sequence, hence there is only one parameter set necessary.  This
 *    is hard coded to zero.
 *    As per Email deiscussions on 10/24/01 we decided to include the SliceID
 *    into both the Full Slice pakcet and Partition A packet
 *   
 * \date
 *    October 24, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int RTPSliceHeader()
{
  int dP_nr = assignSE2partition[input->partition_mode][SE_HEADER];
  Bitstream *currStream = ((img->currentSlice)->partArr[dP_nr]).bitstream;
  DataPartition *partition = &((img->currentSlice)->partArr[dP_nr]);
  SyntaxElement sym;

  int len = 0;
  int RTPSliceType;

  assert (input->of_mode == PAR_OF_RTP);
  sym.type = SE_HEADER;         // This will be true for all symbols generated here
  sym.mapping = n_linfo2;       // Mapping rule: Simple code number to len/info


  SYMTRACESTRING("RTP-SH: Parameter Set");
  sym.value1 = 0;
  len += writeSyntaxElement_UVLC (&sym, partition);

  SYMTRACESTRING("RTP-SH: Picture ID");
  sym.value1 = img->currentSlice->picture_id;
// printf ("Put PictureId %d\n", img->currentSlice->picture_id);

  len += writeSyntaxElement_UVLC (&sym, partition);

  /*! StW:
  // Note: we currently do not support mixed slice types in the encoder, hence
  // we use the picture type here (although there is no such thing as a picture
  // type any more)
  //
  // This needs to be double checked against the document.  Email inquiery on
  // 10/24 evening: is there a need for distinguishing the multpred P from regular
  // P?  if so, this has to be revisited! 
  //
  // StW bug fix: write Slice type according to document
  */

  switch (img->type)
  {
    case INTER_IMG:
      if (img->types==SP_IMG)
        RTPSliceType = 2;
      else
        RTPSliceType = 0;
      break;
    case INTRA_IMG:
      RTPSliceType = 3;
      break;
    case B_IMG:
      RTPSliceType = 1;
      break;
    case SP_IMG:
      // That's what I would expect to be the case. But img->type contains INTER_IMG 
      // here (see handling above)
      //! \todo make one variable from img->type and img->types
      RTPSliceType = 2;
      break;
    default:
      printf ("Panic: unknown picture type %d, exit\n", img->type);
      exit (-1);
  }


  SYMTRACESTRING("RTP-SH: Slice Type");
  sym.value1 = RTPSliceType;
  len += writeSyntaxElement_UVLC (&sym, partition);

  SYMTRACESTRING("RTP-SH: FirstMBInSliceX");
  sym.value1 = img->mb_x;
  len += writeSyntaxElement_UVLC (&sym, partition);

  SYMTRACESTRING("RTP-SH: FirstMBinSlinceY");
  sym.value1 = img->mb_y;
  len += writeSyntaxElement_UVLC (&sym, partition);

  SYMTRACESTRING("RTP-SH: InitialQP");
  sym.value1 = 31-img->qp;
  len += writeSyntaxElement_UVLC (&sym, partition);

  if (img->types==SP_IMG)
  {
    SYMTRACESTRING("RTP-SH: SP InitialQP");
    sym.value1 = 31-img->qpsp;
    len += writeSyntaxElement_UVLC (&sym, partition);
  }


  /*! StW:
  // Note: VCEG-N72 says that the slice type UVLC is not present in single
  // slice packets.  Generally, the NAL allows to mix Single Slice and Partitioning,
  // however, the software does not.  Hence it is sufficient here to express
  // the dependency by checkling the input parameter
  */

  if (input->partition_mode == PAR_DP_3)
  {
    SYMTRACESTRING("RTP-SH: Slice ID");
    sym.value1 = img->current_slice_nr;
    len += writeSyntaxElement_UVLC (&sym, partition);
  }

  // After this, and when in CABAC mode, terminateSlice() may insert one more
  // UVLC codeword for the number of coded MBs

  return len;
}


/*!
 *****************************************************************************
 *
 * \brief 
 *    int RTPWriteBits (int marker) write the Slice header for the RTP NAL      
 *
 * \return
 *    Number of bytes written to output file
 *
 * \para Parameters
 *    marker: markber bit,
 *
 * \para Side effects
 *    Packet written, RTPSequenceNumber and RTPTimestamp updated
 *   
 * \date
 *    October 24, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int RTPWriteBits (int Marker, int PacketType, void * bitstream, 
                    int BitStreamLenInByte, FILE *out)
{
  RTPpacket_t *p;

  assert (out != NULL);
  assert (BitStreamLenInByte < 65000);
  assert (bitstream != NULL);
  assert (PacketType < 4);

  // Set RTP structure elements and alloca() memory foor the buffers
  p = (RTPpacket_t *) alloca (sizeof (RTPpacket_t));
  p->packet=alloca (MAXRTPPACKETSIZE);
  p->payload=alloca (MAXRTPPACKETSIZE);
  p->v=2;
  p->p=0;
  p->x=0;
  p->cc=0;
  p->m=Marker&1;
  p->pt=H26LPAYLOADTYPE;
  p->seq=CurrentRTPSequenceNumber++;
  p->timestamp=CurrentRTPTimestamp;
  p->ssrc=H26LSSRC;

  p->payload[0] = PacketType;

  p->paylen = 1 + BitStreamLenInByte;


  memcpy (&p->payload[1], bitstream, BitStreamLenInByte);

  // Generate complete RTP packet
  if (ComposeRTPPacket (p) < 0)
  {
    printf ("Cannot compose RTP packet, exit\n");
    exit (-1);
  }

  if (WriteRTPPacket (p, out) < 0)
  {
    printf ("Cannot write %d bytes of RTP packet to outfile, exit\n", p->packlen);
    exit (-1);
  }
  return (p->packlen);

}


static int oldtr = -1;

void RTPUpdateTimestamp (int tr)
{
  int delta;

  if (oldtr == -1)            // First invocation
  {
    CurrentRTPTimestamp = 0;  //! This is a violation of the security req. of
                              //! RTP (random timestamp), but easier to debug
    oldtr = 0;
    return;
  }

  /*! The following code assumes a wrap around of TR at 256, and
      needs to be changed as soon as this is no more true.
      
      The support for B frames is a bit tricky, because it is not easy to distinguish
      between a natural wrap-around of the tr, and the intentional going back of the
      tr because of a B frame.  It is solved here by a heuristic means: It is assumed that
      B frames are never "older" than 10 tr ticks.  Everything higher than 10 is considered
      a wrap around.
      I'm a bit at loss whether this is a fundamental problem of B frames.  Does a decoder
      have to change its TR interpretation depenent  on the picture type?  Horrible -- we
      would have a fundamental problem when mixing slices.  This needs careful review.
  */

  delta = tr - oldtr;

  if (delta < -10)        // wrap-around
    delta+=256;

  CurrentRTPTimestamp += delta * RTP_TR_TIMESTAMP_MULT;
  oldtr = tr;
}

