/*************************************************************************

This software module was adopted from MPEG4 VM7.0 text, modified by 

	Wei-ge Chen (wchen@microsoft.com), Microsoft Corporation
	(date: April, 1997)

and also edited by
	Yoshihiro Kikuchi (TOSHIBA CORPORATION)
	Takeshi Nagai (TOSHIBA CORPORATION)
	Toshiaki Watanabe (TOSHIBA CORPORATION)
	Noboru Yamaguchi (TOSHIBA CORPORATION)

in the course of development of the MPEG-4 Video (ISO/IEC 14496-2). 
This software module is an implementation of a part of one or more MPEG-4 Video tools 
as specified by the MPEG-4 Video. 
ISO/IEC gives users of the MPEG-4 Video free license to this software module or modifications 
thereof for use in hardware or software products claiming conformance to the MPEG-4 Video. 
Those intending to use this software module in hardware or software products are advised that its use may infringe existing patents. 
The original developer of this software module and his/her company, 
the subsequent editors and their companies, 
and ISO/IEC have no liability for use of this software module or modifications thereof in an implementation. 
Copyright is not released for non MPEG-4 Video conforming products. 
Microsoft retains full right to use the code for his/her own purpose, 
assign or donate the code to a third party and to inhibit third parties from using the code for non <MPEG standard> conforming products. 
This copyright notice must be included in all copies or derivative works. 

Copyright (c) 1996, 1997.

Module Name:

	cae.c

Abstract:

	Context-based arithmatic coding

Revision History:

*************************************************************************/

#include "typeapi.h"
#include "basic.hpp"
#include "global.hpp" //	Added for error resilient mode by Toshiba(1997-11-14)
#include "bitstrm.hpp"
#include "cae.h"

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_


/* START ENCODING A STREAM OF SYMBOLS */

Void StartArCoder(ArCoder *coder) 
{
  coder->L = 0;
  coder->R = HALF-1;
  coder->bits_to_follow = 0;
  coder->first_bit = 1;
  coder->nzeros = g_iMaxHeading;	//	Modified for error resilient mode by Toshiba(1997-11-14)
  coder->nonzero = 0;
  coder->nBits = 0;			//added by wchen
}

/* FINISH ENCODING THE STREAM */

Int StopArCoder(ArCoder *coder, COutBitStream* bitstream) 
{

  Int a = (coder->L) >> (CODE_BITS-3);
  Int b = (coder->R + coder->L) >> (CODE_BITS-3);
  Int nbits, bits, i;

  if (b == 0)
    b = 8;

  if (b - a >= 4 || (b - a == 3 && (a & 1))) {
    nbits = 2;
    bits = (a>>1) + 1;
  }
  else {
    nbits = 3;
    bits = a + 1;
  }
  
  for (i = 1; i <= nbits; i++)
    BitPlusFollow (((bits >> (nbits - i)) & 1), coder, bitstream);
  
  if (coder->nzeros < g_iMaxMiddle-g_iMaxTrailing || coder->nonzero == 0) {	//	Modified for error resilient mode by Toshiba(1997-11-14)
    BitPlusFollow (1, coder, bitstream);
  }
//  return;
  return 0;
}

Void BitByItself(Int bit, ArCoder *coder, COutBitStream* bitstream) {
//BitstreamPutBit(bitstream,bit);	/* Output the bit */
  if (bitstream != NULL)
	bitstream->putBits (bit, 1, "MB_CAE_Bit");
  coder->nBits++;
  if (bit == 0) {
    coder->nzeros--;
    if (coder->nzeros == 0) {
//    BitstreamPutBit(bitstream,1);
	  if (bitstream != NULL)
		bitstream->putBits (1, (UInt) 1, "MB_CAE_Bit");
	  coder->nBits++;
      coder->nonzero = 1;
      coder->nzeros = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
    }
  }
  else {
    coder->nonzero = 1;
    coder->nzeros = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
  }
}

/* OUTPUT BITS PLUS FOLLOWING OPPOSITE BITS */

Void BitPlusFollow(Int bit, ArCoder *coder, COutBitStream *bitstream)
{
	if (!coder->first_bit)
		BitByItself(bit, coder, bitstream);
	else
		coder->first_bit = 0;
	while ((coder->bits_to_follow) > 0) {
		BitByItself(!bit, coder, bitstream);
		coder->bits_to_follow -= 1;
	}
}


/* ENCODE A BINARY SYMBOL */

Void ArCodeSymbol(Int bit, USInt c0, ArCoder *coder, COutBitStream *bitstream) 
{
  //  Int bits = 0;
  USInt c1 = (1<<16) - c0;
  
  Int LPS = (c0 > c1);
  USInt cLPS = (LPS) ? c1 : c0;
  assert (cLPS != 0);
  
  unsigned long rLPS;
  
  rLPS = ((coder->R) >> 16) * cLPS;
  
  if (bit == LPS) {
    coder->L += coder->R - rLPS;
    coder->R = rLPS;
  }
  else
    coder->R -= rLPS;
  
  ENCODE_RENORMALISE(coder,bitstream);
}


Void ENCODE_RENORMALISE(ArCoder *coder, COutBitStream* bitstream) 
{
	while (coder->R < QUARTER) {					 
		if (coder->L >= HALF) {				 
			BitPlusFollow(1,coder,bitstream);		 
			coder->L -= HALF;			 
		}				 
		else 
			if (coder->L + coder->R <= HALF) 
				BitPlusFollow(0,coder,bitstream);
			else {				 
				coder->bits_to_follow++;		 
				coder->L -= QUARTER;		 
			}				 
		coder->L += coder->L;				 
		coder->R += coder->R;				 
	}
}

//Decoder part
Void StartArDecoder(ArDecoder *decoder, CInBitStream *bitstream) 
{
  Int			i,j;
  
  decoder->V = 0;  
  decoder->nzerosf = g_iMaxHeading;	//	Modified for error resilient mode by Toshiba(1997-11-14)
  decoder->extrabits = 0;

  for (i = 1; i<CODE_BITS; i++) {
//    j = BitstreamLookBit(bitstream,i+decoder->extrabits);
	  j = bitstream->peekOneBit (i + decoder->extrabits);
    decoder->V += decoder->V + j;
    if (j == 0) {
      decoder->nzerosf--;
      if (decoder->nzerosf == 0) {
			decoder->extrabits++;
			decoder->nzerosf = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
      }
    }
    else
      decoder->nzerosf = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
  }

  decoder->L = 0;
  decoder->R = HALF - 1;
  decoder->bits_to_follow = 0;
  decoder->arpipe = decoder->V;
  decoder->nzeros = g_iMaxHeading;	//	Modified for error resilient mode by Toshiba(1997-11-14)
  decoder->nonzero = 0;
}

  
Void StopArDecoder(ArDecoder *decoder, CInBitStream *bitstream) 
{
  Int a = decoder->L >> (CODE_BITS - 3);
  Int b = (decoder->R + decoder->L) >> (CODE_BITS - 3);

  Int nbits,i;

  if (b == 0) {
    b = 8;
  }
  if (b - a >= 4 || (b - a == 3 && a&1))
    nbits = 2;
  else
    nbits = 3;

  for (i = 1; i <= nbits - 1; i++)
    AddNextInputBit (bitstream, decoder);	
  
  if (decoder->nzeros < g_iMaxMiddle-g_iMaxTrailing || decoder->nonzero == 0) {	//	Modified for error resilient mode by Toshiba(1997-11-14)
    BitstreamFlushBits(bitstream,1);
  }
}


Void AddNextInputBit(CInBitStream *bitstream, ArDecoder *decoder) 
{
	Int i;
	if (((decoder->arpipe >> (CODE_BITS-2)) & 1) == 0) {
		decoder->nzeros--;
		if (decoder->nzeros == 0) {
			BitstreamFlushBits(bitstream,1);
			decoder->extrabits--;
			decoder->nzeros = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
			decoder->nonzero = 1;
		}
	}
	else {
		decoder->nzeros = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
		decoder->nonzero = 1;
	}

	BitstreamFlushBits(bitstream,	1);
//	i = (Int)BitstreamLookBit(bitstream, CODE_BITS-1+decoder->extrabits);
	i = bitstream->peekOneBit (CODE_BITS-1+decoder->extrabits);
	decoder->V += decoder->V + i;
	decoder->arpipe += decoder->arpipe + i;

	if (i == 0) {
		decoder->nzerosf--;
		if (decoder->nzerosf == 0) {
			decoder->nzerosf = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
			decoder->extrabits++;
		}
	}
	else
		decoder->nzerosf = g_iMaxMiddle;	//	Modified for error resilient mode by Toshiba(1997-11-14)
}


Int ArDecodeSymbol(USInt c0, ArDecoder *decoder, CInBitStream *bitstream) 
{
	Int bit;
	Int c1 = (1<<16) - c0;
	Int LPS = c0 > c1;
	Int cLPS = LPS ? c1 : c0;
	unsigned long rLPS;

	rLPS = ((decoder->R) >> 16) * cLPS;
	if ((decoder->V - decoder->L) >= (decoder->R - rLPS)) 	{
		bit = LPS;
		decoder->L += decoder->R - rLPS;
		decoder->R = rLPS;
	}
	else 	{
		bit = (1-LPS);
		decoder->R -= rLPS;
	}

	DECODE_RENORMALISE(decoder,bitstream);
	return(bit);
}

Void DECODE_RENORMALISE(ArDecoder *decoder,  CInBitStream *bitstream) 
{
	while (decoder->R < QUARTER) 	{
		if (decoder->L >= HALF) 	{
			decoder->V -= HALF;
			decoder->L -= HALF;
			decoder->bits_to_follow = 0;
		}
		else
			if (decoder->L + decoder->R <= HALF) 
				decoder->bits_to_follow = 0;
			else	{
				decoder->V -= QUARTER;
				decoder->L -= QUARTER;
				(decoder->bits_to_follow)++;
			}
		decoder->L += decoder->L;	
		decoder->R += decoder->R;
		AddNextInputBit(bitstream, decoder);
	}
}

Void  BitstreamFlushBits(CInBitStream *bitstream, Int nBits)
{
	assert (nBits >= 0);
	bitstream->getBits (nBits);
}


/*
	//decoding
	pf = fopen (TEST_FILENAME, "r");
	ac = (ArDecoder*) malloc (sizeof (ArDecoder));
	StartArDecoder (ac, pf);
	for (i = 0; i < NUM_SYMBOL; i++)	{	
		j = ArDecodeSymbol (intra_prob [i], ac, pf);		//context is simply "i"
		assert (j == i % 2);							//symbol should be 0, 1 alternatively
	}
	StopArDecoder(ac, pf);
	free (ac);
	fclose (pf);
*/
