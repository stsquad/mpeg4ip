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
 *
 * \file rtp.h
 *
 * \brief
 *    Definition of structures and functions to handle RTP headers.  For a
 *    description of RTP see RFC1889 on http://www.ietf.org
 *
 * \date
 *    30 September 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 **************************************************************************/

#ifndef _RTP_H_
#define _RTP_H_

#include <stdio.h>
#include "global.h"

#define MAXRTPPAYLOADLEN  (65536 - 40)    //!< Maximum payload size of an RTP packet
#define MAXRTPPACKETSIZE  (65536 - 28)    //!< Maximum size of an RTP packet incl. header
#define H26LPAYLOADTYPE 105               //!< RTP paylaod type fixed here for simplicity
#define H26LSSRC 0x12345678               //!< SSRC, chosen to simplify debugging
#define RTP_TR_TIMESTAMP_MULT 1000        //!< should be something like 27 Mhz / 29.97 Hz

typedef struct 
{
  unsigned int v;          //!< Version, 2 bits, MUST be 0x2
  unsigned int p;          //!< Padding bit, Padding MUST NOT be used
  unsigned int x;          //!< Extension, MUST be zero */
  unsigned int cc;         /*!< CSRC count, normally 0 in the absence 
                                of RTP mixers */
  unsigned int m;          //!< Marker bit
  unsigned int pt;         //!< 7 bits, Payload Type, dynamically established
  unsigned int seq;        /*!< RTP sequence number, incremented by one for 
                                each sent packet */
  unsigned int timestamp;  //!< timestamp, 27 MHz for H.26L
  unsigned int ssrc;       //!< Synchronization Source, chosen randomly
  byte *       payload;    //!< the payload including payload headers
  unsigned int paylen;     //!< length of payload in bytes
  byte *       packet;     //!< complete packet including header and payload
  unsigned int packlen;    //!< length of packet, typically paylen+12
} RTPpacket_t;


int  RTPSequenceHeader (FILE *out);

int  ComposeRTPPacket (RTPpacket_t *p);
int  DecomposeRTPpacket (RTPpacket_t *p);
int  WriteRTPPacket (RTPpacket_t *p, FILE *f);
void DumpRTPHeader (RTPpacket_t *p);
int  RTPSequenceHeader(FILE *);
void RTPUpdateTimestamp (int tr);
int  RTPSliceHeader();
int  RTPWriteBits (int Marker, int PacketType, void * bitstream, 
                   int BitStreamLenInByte, FILE *out);

#endif