/* $Id: ac.cpp,v 1.1 2003/05/05 21:24:05 wmaycisco Exp $ */

/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*                                                                          */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/

/************************************************************/
/*  Filename: ac.c                                          */
/*  Author: B.S. Srinivas                                   */
/*  Date Modified: April 23, 1998                           */
/*                                                          */
/*  Descriptions:                                           */
/*    This file contains routines for Integer arithmetic    */
/*    coding, which is based on the ac.c file from the SOL  */
/*    package.                                              */
/*                                                          */
/*    The following routines are modified or created for    */
/*    the latest VTC package:                               */
/*      static Void mzte_output_bit()                       */
/*      Void mzte_ac_encoder_init()                         */
/*      Int mzte_ac_encoder_done()                          */
/*      static Int mzte_input_bit()                         */
/*      Void mzte_ac_decoder_init()                         */
/*      Void mzte_ac_decoder_done()                         */
/*                                                          */
/************************************************************/

//#include <stdio.h>
//#include <stdlib.h>
//#include <assert.h>
#include "dataStruct.hpp"
#include "ac.hpp"
#include "bitpack.hpp"
#include "errorHandler.hpp"
#include "msg.hpp"

#define codeValueBits 16
#define peakValue (((long)1<<codeValueBits)-1)
#define firstQtr  (peakValue/4+1)
#define Half      (2*firstQtr)
#define thirdQtr  (3*firstQtr)

#define BYTETYPE	UChar
#define SHORTTYPE	UShort
#define MIN(a,b)  (((a)<(b)) ? (a) : (b))

//Changed by Sarnoff for error resilience, 3/5/99
//#define STUFFING_CNT 22
static Int STUFFING_CNT=22;  //setting for error resi case
//End changed by Sarnoff for error resilience, 3/5/99

#define MIXED 		2

/* static function prototypes */

//static Void mzte_output_bit(ac_encoder *,Int);
//static Void mzte_bit_plus_follow(ac_encoder *,Int);
//static Void mzte_update_model(ac_model *,ac_model *,Int);
//static Int mzte_input_bit(ac_decoder *);


static Int zeroStrLen=0;

/************************************************************************/
/*              Error Checking and Handling Macros                      */
/************************************************************************/

#define error(m)                                           \
do {                                                      \
  fflush(stdout);                                         \
  fprIntf(stderr, "%s:%d: error: ", __File__, __LINE__);  \
  fprIntf(stderr, m);                                     \
  fprIntf(stderr, "\n");                                  \
  exit(1);                                                \
} while (0)

#define check(b,m)                                         \
do { 													   \
  if (b)                                                   \
	 error(m);                                            \
} while (0)

/************************************************************************/
/*                           Bit Output                                 */
/************************************************************************/

/**************************************************/
/*  Added bit stuffing to prevent start code      */
/*  emulation, i.e., add a "1" bit after every 22 */
/*  consecutive "0" bits in the bit stream        */
/*                                                */
/*  Modified to use a fixed buffer and write to   */
/*  file directly after the buffer is full. So the*/
/*  ace->bitstreamLength now only has the # of 	  */
/*  bytes in current buffer. Total bits (bitCount)*/
/*  will indicate the total # bits for arithmetic */
/*  coding part.								  */
/**************************************************/

Void CVTCEncoder::mzte_output_bit(ac_encoder *ace,Int bit)
{
	register int flag=0;

	ace->buffer *= 2;
	ace->buffer |= (bit)?0x01:0;
	(ace->bitsLeft)--;
	(ace->bitCount)++;
	if (!(ace->bitsLeft)) {
		if (!(ace->bitstream))
			errorHandler("Failure to allocate space for array Bitstream " \
                            "in ac_encoder structure");
		switch (flag=(ace->bitstreamLength>=MAX_BUFFER)) {
			case 1:
				write_to_bitstream(ace->bitstream,MAX_BUFFER<<3);
				ace->bitstreamLength=0;
				break;
			default:
				break;
		}
		ace->bitstream[(ace->bitstreamLength)++] = ace->buffer;
		ace->bitsLeft = 8;
	}
	/* Dealing with bit stuffing when 0's are encountered */
	zeroStrLen+=(!bit)?1:-zeroStrLen;
	if (zeroStrLen==STUFFING_CNT) {
		mzte_output_bit(ace,1);
		zeroStrLen=0;
	}
	return;
}

Void CVTCEncoder::mzte_bit_plus_follow(ac_encoder *ace,Int bit)
{
	register long followBits;

	followBits = ace->followBits;
	mzte_output_bit(ace,bit);
	while (followBits) {
		mzte_output_bit(ace,!bit);
		--followBits;
	}
	ace->followBits = followBits;
	return;
}

/************************************************************************/
/*                             Encoder                                  */
/************************************************************************/

Void CVTCEncoder::mzte_ac_encoder_init(ac_encoder *ace)
{
	ace->low = 0;
	ace->high = peakValue;
	ace->followBits = 0;
	ace->bitsLeft = 8;
	ace->buffer = 0;
	ace->bitCount = 0;
	ace->bitstreamLength = 0;
	ace->bitstream=(BYTETYPE *)malloc((MAX_BUFFER+10)*sizeof(BYTETYPE));
	if (ace->bitstream == NULL)
		errorHandler("can't allocate memory for ace->bitstream");
//	assert((ace->bitstream=(BYTETYPE *)malloc((MAX_BUFFER+10)*sizeof(BYTETYPE))));
	zeroStrLen=0;

	//Added by Sarnoff for error resilience, 3/5/99
	if(!mzte_codec.m_usErrResiDisable)
		STUFFING_CNT=15;
	//End Added by Sarnoff for error resilience, 3/5/99

	/* always start arithmetic coding bitstream with a 1 bit. */
	emit_bits(1,1);
	return;
}


/***************************************************************/
/* Added stuffing bits to prevent start code emulation.        */
/***************************************************************/

Int CVTCEncoder::mzte_ac_encoder_done(ac_encoder *ace)
{
	register long bitCount;
	register Int bitsLeft;
	register Int bitsToWrite;
	register long streamLength;
	register int flag;

	++(ace->followBits);
	flag = (ace->low >= firstQtr);
	mzte_bit_plus_follow(ace,flag);
	bitsLeft = ace->bitsLeft;
	bitCount = ace->bitCount;
	streamLength = ace->bitstreamLength;
	if (bitsLeft != 8) {
		ace->bitstream[streamLength++] = (ace->buffer << bitsLeft);
		if (!(ace->bitstream[streamLength-1]&(1<<bitsLeft))) {
	  		ace->bitstream[streamLength-1] += (1<<bitsLeft)-1;
			++bitCount;
		}
	}
	bitsToWrite = (streamLength > MAX_BUFFER)?(MAX_BUFFER<<3):0;
	bitsToWrite += (bitCount % (MAX_BUFFER<<3));
	if ((bitsToWrite==0) && (streamLength==MAX_BUFFER))
		bitsToWrite=(MAX_BUFFER<<3);
	write_to_bitstream(ace->bitstream,bitsToWrite);
	if ((bitsLeft==8) && (!(ace->bitstream[streamLength-1]&1))) {
		/* stuffing bits to prevent start code emulation */
		emit_bits(1,1);
		++bitCount;
	}
	ace->bitstreamLength=streamLength;
	ace->bitCount=bitCount;
	free(ace->bitstream);
	return(ace->bitCount);
}

Int CVTCEncoder::mzte_ac_encode_symbol(ac_encoder *ace, ac_model *acm, Int sym)
{
  register long low,high;
  register long range;
  register long bitCount;
  //static int count=0;
  if (sym<0 || sym>=acm->nsym)
    errorHandler("Invalid symbol passed to mzte_ac_encode_symbol " \
		 "(sym=%d while nsym=%d)", 
		 sym,acm->nsym);

  low = ace->low;
  high = ace->high;
  range = (long) (high - low) + 1;
 // printf("%d %d %d %d\n", count++, sym, acm->cfreq[0], acm->cfreq[1]);
  high=low+(range*(Int)acm->cfreq[sym])/(Int)acm->cfreq[0] - 1;
  low=low+(range*(Int)acm->cfreq[sym+1])/(Int)acm->cfreq[0];

  bitCount = ace->bitCount;
  while (1) {
    if (high < Half)  {
      mzte_bit_plus_follow(ace,0);
    }
    else if (low >= Half)  {
      mzte_bit_plus_follow(ace,1);
      low -= Half;
      high -= Half;
    }
    else if ((low >= firstQtr) && (high < thirdQtr)) {
      ++(ace->followBits);
      low -= firstQtr;
      high -= firstQtr;
    }
    else
      break;
    low = 2*low;
    high = 2*high + 1;
  }
  ace->low = low;
  ace->high = high;

  if (acm->adapt)
    mzte_update_model(acm,sym);

  return ace->bitCount - bitCount;
}


/************************************************************************/
/*                            Bit Input                                 */
/************************************************************************/

/*********************************************************/
/* Modified to be consistant with the functions in       */
/* bitpack.c, i.e., using nextinputbit() to get the new  */
/* bits from the bit stream.                             */
/*                                                       */
/* Included remove stuffing bits, refer to 				 */
/* mzte_output_bit() for more details.                   */
/*********************************************************/

Int CVTCDecoder::mzte_input_bit(ac_decoder *acd)
{
	register Int t;

	if (!(acd->bitsLeft))
		acd->bitsLeft = 8;

	t = nextinputbit();
	--(acd->bitsLeft);
	++(acd->bitCount);

	/* remove stuffing bits */
	zeroStrLen+=(!t)?1:-zeroStrLen;
	if(zeroStrLen==STUFFING_CNT) {
		if (!mzte_input_bit(acd))
			errorHandler("Error in decoding stuffing bits " \
                         "(must be 1 after %d 0's)",STUFFING_CNT);
		zeroStrLen=0;
	}
	return(t);
}


/************************************************************************/
/*                             Decoder                                  */
/************************************************************************/

Void CVTCDecoder::mzte_ac_decoder_init(ac_decoder *acd)
{
	register long i,value=0;

	//Added by Sarnoff for error resilience, 3/5/99
	if(!mzte_codec.m_usErrResiDisable)
		STUFFING_CNT=15;
	//End Added by Sarnoff for error resilience, 3/5/99

	/* remove first stuffing bit */
	if(!get_X_bits(1))
		errorHandler("Error in extracting the stuffing bit at the\n"\
                        "beginning of arithmetic decoding"\
                        "refer mzte_encoder_init in ac.c)");
	
	zeroStrLen=0;
	i = codeValueBits;
	do {
		value <<= 1;
		value += mzte_input_bit(acd);
	} while (--i);
	acd->value = value;
	acd->low = 0;
	acd->high = peakValue;
	acd->bitCount = 0;
	acd->bitsLeft = 0;
	return;
}


/*******************************************************/
/* Added restore_arithmetic_offset() called to recover */
/* the extra bits read in by decoder. This routine is  */
/* defined in bitpack.c                                */
/*******************************************************/
Void CVTCDecoder::mzte_ac_decoder_done(ac_decoder *acd)
{
	restore_arithmetic_offset(acd->bitsLeft);	
	acd->bitCount += acd->bitsLeft;
	if ((acd->bitCount)%8)
		errorHandler("Did not get alignment in arithmetic decoding");
}

Int CVTCDecoder::mzte_ac_decode_symbol(ac_decoder *acd,ac_model *acm)
{
  register Int high,low,value;
  register long range;
  register Int cum;
  Int sym;
  Int modify=0;
 // static int count=0;
  high=acd->high; low=acd->low; value=acd->value;
  range = (long)(high-low)+1;
  
  cum = (((long)(value-low)+1)*(Int)(acm->cfreq[0])-1)/range;
  for (sym=0; (Int)(acm->cfreq[sym+1])>cum; sym++)
    /* do nothing */ ;
  high = low + (range*(Int)(acm->cfreq[sym]))/(Int)(acm->cfreq[0])-1;
  low  = low + (range*(Int)(acm->cfreq[sym+1]))/(Int)(acm->cfreq[0]);
  modify = acm->adapt; 
 // printf("%d %d %d %d\n", count++,sym, acm->cfreq[0], acm->cfreq[1]);
  while (1) {
    if (high < Half) {
      /* do nothing */
    } else if (low >= Half) {
      value -= Half;
      low -= Half;
      high -= Half;
    }
    else if ((low >= firstQtr) && (high < thirdQtr))  {
      value -= firstQtr;
      low -= firstQtr;
      high -= firstQtr;
    }
    else
      break;
    low <<= 1;
    high = (high<<1)+1;
    value = (value<<1) + mzte_input_bit(acd);
  }
  acd->high = high;
  acd->low = low;
  acd->value = value;
  if (modify)
    mzte_update_model(acm,sym);
  
  return sym;
}


/************************************************************************/
/*                       Probability Model                              */
/************************************************************************/

Void CVTCCommon::mzte_ac_model_init(ac_model *acm,Int nsym,SHORTTYPE *ifreq,Int adapt,
Int inc)
{
	register Int i;
	register UShort tmpFreq=0;

	acm->inc = inc;
	acm->nsym = nsym;
	acm->adapt = adapt;

  if ((acm->freq=(UShort *)malloc(nsym*sizeof(UShort)))==NULL)
    errorHandler("Can't allocate %d bytes for acm->freq in " \
		 "mzte_ac_model_init.",
		 nsym*sizeof(UShort));

  if  ((acm->cfreq=(UShort *) malloc((nsym+1)*sizeof(UShort)))==NULL)
    errorHandler("Can't allocate %d bytes for acm->cfreq in " \
		 "mzte_ac_model_init.",
		 (nsym+1)*sizeof(UShort));
  
	if (ifreq) {
		acm->cfreq[nsym] = 0;
		for (i=nsym-1; i>=0; i--) {
			acm->freq[i] = ifreq[i];
			acm->cfreq[i] = tmpFreq + ifreq[i];
			tmpFreq = acm->cfreq[i];
		}
		/* NOTE: This check won't always work for mixture of models */
		if (acm->cfreq[0] > acm->Max_frequency) {
			register Int cum=0;
			acm->cfreq[nsym] = 0;
			for (i=nsym-1; i>=0; i--)  {
				acm->freq[i] = ((Int) ifreq[i] + 1)/2;
				cum += (ifreq[i] + 1)/2;
				acm->cfreq[i] = cum;
			}
		}
		if (acm->cfreq[0] > acm->Max_frequency)
			errorHandler("error in acm->cfreq[0]");
	}
	else {
		for (i=0; i < nsym; i++) {
			acm->freq[i] = 1;
			acm->cfreq[i] = nsym - i;
		}
		acm->cfreq[nsym] = 0;
	}
}

Void CVTCCommon::mzte_ac_model_done(ac_model *acm)
{
	acm->nsym = 0;
	free(acm->freq);
	acm->freq = NULL;
	free(acm->cfreq);
	acm->cfreq = NULL;
}

Void CVTCCommon::mzte_update_model(ac_model *acm,Int sym)
{
  register SHORTTYPE *freq,*cfreq;
  register Int i;
  register Int inc;

  freq = acm->freq;
  cfreq = acm->cfreq;
  /* scale freq count down */
  if (cfreq[0] == acm->Max_frequency) {
    register Int cum=0,nsym;
    nsym = acm->nsym;
    cfreq[nsym] = 0;
    for (i=nsym-1; i>=0; i--) {
      freq[i] = ((Int)freq[i] + 1)/2;
      cum += freq[i];
      cfreq[i] = cum;
    }
  }
  inc = acm->inc;
  freq[sym] += inc;
  for (i=sym; i>=0; i--)
    cfreq[i] += inc;
}

