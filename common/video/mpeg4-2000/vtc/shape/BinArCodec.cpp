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

#include <stdio.h>
#include <math.h>
#include "dataStruct.hpp"
//#include "bitpack.hpp"
//#include "ShapeBaseCommon.hpp"
//#include "BinArCodec.hpp"

/***************************/
/*      ArCodec            */
/***************************/


Void CVTCEncoder::
StartArCoder_Still(ArCoder *coder) 
{
    coder -> L = 0;
    coder -> R = HALF-1;
    coder -> bits_to_follow = 0;
    coder -> first_bit = 1;
    coder -> nzeros = MAXHEADING;
    coder -> nonzero = 0;
}

Void CVTCEncoder::
StopArCoder_Still(ArCoder *coder,BSS *bitstream)
{
    Int a = ( coder -> L )              >> ( CODE_BITS - 3 );
    Int b = ( coder -> R + coder -> L ) >> ( CODE_BITS - 3 );

    Int nbits, bits, i;

    if ( b == 0 ) b = 8;

    if ( b-a >= 4 || (b-a == 3 && (a & 1) == 1 ) ) {
        nbits = 2;
        bits  = (a>>1) + 1;
    } else {
        nbits = 3;
        bits  = a + 1;
    }
  
    for ( i=1; i<=nbits; i++ ) {
        BitPlusFollow_Still ( ((bits >> (nbits-i)) & 1), coder, bitstream );
    }
	
    if ( coder -> nzeros < MAXMIDDLE-MAXTRAILING || coder -> nonzero == 0 ) {
        BitPlusFollow_Still( 1, coder, bitstream );
    }

    return;
}

/* ENCODE A BINARY SYMBOL */

Void CVTCEncoder::
ArCodeSymbol_Still(ArCoder *coder,BSS *bitstream,UChar bit,UInt c0)
{
	UInt	c1   = (1<<16) - c0;
  
	UChar	LPS  = (c0 > c1);
	UInt	cLPS = (LPS) ? c1 : c0;
	
	unsigned long rLPS;
	rLPS = ((coder -> R) >> 16) * cLPS;
	if(c0 == 0 || c0==65536) return;
	else if(c0==65537) 
	  errorHandler("Impossible context occured\n");
	if (bit == LPS) {
		coder -> L += coder->R - rLPS;
		coder -> R  = rLPS;
	} else {
		coder -> R -= rLPS;
	}

	EncRenormalize( coder, bitstream );
}

Void CVTCEncoder::
EncRenormalize(ArCoder *coder,BSS *bitstream)
{
    while ( coder -> R < QUARTER ) {
        if( coder -> L >= HALF ) {				 
            BitPlusFollow_Still( 1, coder, bitstream );
            coder -> L -= HALF;
        } else {
            if ( coder -> L + coder -> R <= HALF ) {
                BitPlusFollow_Still( 0, coder, bitstream );
            } else {				 
                coder -> bits_to_follow++;		 
                coder -> L -= QUARTER;		 
            }
        }
        coder -> L += coder -> L;				 
        coder -> R += coder -> R;				 
    }
}

Void CVTCEncoder::
BitByItself_Still(Int bit,ArCoder *coder,BSS *bitstream) 
{
    BitstreamPutBit_Still( bit, bitstream );	/* Output the bit */
    if ( bit == 0 ) {
        coder -> nzeros --;
        if ( coder -> nzeros == 0 ) {
            BitstreamPutBit_Still( 1, bitstream );
            coder -> nonzero = 1;
            coder -> nzeros  = MAXMIDDLE;
        }
    } else {
        coder -> nonzero = 1;
        coder -> nzeros  = MAXMIDDLE;
    }
}

/* OUTPUT BITS PLUS FOLLOWING OPPOSITE BITS */

Void CVTCEncoder::
BitPlusFollow_Still(Int bit,ArCoder *coder,BSS *bitstream)
{
	if ( ! coder -> first_bit )
		BitByItself_Still( bit, coder, bitstream );
	else
		coder -> first_bit = 0;
	while ( (coder -> bits_to_follow) > 0 ) {
		BitByItself_Still( !bit, coder, bitstream );
		coder -> bits_to_follow -= 1;
	}
}


/*  Bitstream operation function in CAE */

Void CVTCEncoder::
BitstreamPutBit_Still(Int bit,BSS *bitstream)
{
	Int		res = bitstream -> res;
	UChar	*p  = bitstream -> ptr;

	*p |= (UChar)( ( bit & 1 ) << ( 7 - res ) );
	res ++;
	if ( res == 8 ) {
		*(++p) = (UChar)0;
		res    = 0;
	}
	bitstream -> ptr = p;
	bitstream -> res = res;
	bitstream -> cnt ++;
}

/***************************/
/*      Decoder            */
/***************************/

Void CVTCDecoder::
StartArDecoder_Still(ArDecoder *decoder) 
{
	Int		i, j;
  
	decoder -> V			= 0;
	decoder -> extrabits	= 0;
	decoder -> nzerosf		= MAXHEADING;

	for ( i=1; i<CODE_BITS; i++ ) {
		j = BitstreamLookBit( i + decoder -> extrabits);
		decoder -> V += decoder -> V + j;
		if ( j == 0 ) {
			decoder -> nzerosf--;
			if (decoder -> nzerosf == 0) {
				decoder -> extrabits ++;
				decoder -> nzerosf   = MAXMIDDLE;
			}
		} else {
			decoder -> nzerosf = MAXMIDDLE;
		}
	}

	decoder -> L				= 0;
	decoder -> R				= HALF - 1;
	decoder -> bits_to_follow	= 0;
	decoder -> arpipe			= decoder -> V;
	decoder -> nzeros			= MAXHEADING;
	decoder -> nonzero			= 0;

	return;
}

  
Void CVTCDecoder::
StopArDecoder_Still(ArDecoder *decoder) 
{
	Int		a = ( decoder -> L >> ( CODE_BITS - 3 ) );
	Int		b = ( decoder -> R + decoder -> L ) >> ( CODE_BITS - 3 );

	Int		nbits, i;

	if (b == 0) {
		b =  8;
	}

	if ( (b-a) >= 4 || ( (b-a) == 3 && (a & 1) == 1 ) )	nbits = 2;
	else												nbits = 3;

	for ( i=1; i<nbits; i++ )
		AddNextInputBit_Still( decoder);	
  
	if ( decoder->nzeros<MAXMIDDLE-MAXTRAILING || decoder->nonzero==0 ) {
		BitstreamFlushBits_Still( 1);
		nbits ++;
	}

	return;
}


UChar CVTCDecoder::
ArDecodeSymbol_Still(ArDecoder *decoder,UInt c0)
{
	Int		bit;
	UInt	c1		= (1<<16) - c0;
	UChar	LPS		= (c0 > c1);
	UInt	cLPS	= LPS ? c1 : c0;
	UInt	rLPS;
	if(c0 == 0) return (1);
	else if(c0==65536) return(0);
	else if(c0==65537) 
	  errorHandler("Impossible context occured\n");
	rLPS = (decoder -> R >> 16) * cLPS;
	if ( (decoder -> V - decoder -> L) >= (decoder -> R - rLPS) ) {
		bit			  = LPS;
		decoder -> L += decoder -> R - rLPS;
		decoder -> R  = rLPS;
	} else {
		bit			  = (1-LPS);
		decoder -> R -= rLPS;
	}
	DecRenormalize( decoder);

	return (bit);
}


Void CVTCDecoder::
AddNextInputBit_Still(ArDecoder *decoder)
{
	Int		i;

	if ( ((decoder -> arpipe >> (CODE_BITS - 2)) & 1) == 0 ) {
		decoder -> nzeros--;
		if ( decoder -> nzeros == 0 ) {
			BitstreamFlushBits_Still( 1);
			decoder -> extrabits--;
			decoder -> nzeros	= MAXMIDDLE;
			decoder -> nonzero	= 1;
		}
	} else {
		decoder -> nzeros	= MAXMIDDLE;
		decoder -> nonzero	= 1;
	}

	BitstreamFlushBits_Still( 1);

	i = BitstreamLookBit( (CODE_BITS-1+decoder->extrabits));

	decoder -> V		+= decoder -> V + i;
	decoder -> arpipe	+= decoder -> arpipe + i;

	if ( i == 0 ) {
		decoder -> nzerosf--;
		if (decoder -> nzerosf == 0) {
			decoder -> nzerosf = MAXMIDDLE;
			decoder -> extrabits++;
		}
	} else {
		decoder -> nzerosf = MAXMIDDLE;
	}
	return;

}

Void CVTCDecoder::
DecRenormalize(ArDecoder *decoder)
{
	while ( decoder -> R < QUARTER ) {
		if (decoder -> L >= HALF) {
			decoder -> V -= HALF;
			decoder -> L -= HALF;
			decoder -> bits_to_follow = 0;
		} else {
			if (decoder -> L + decoder -> R <= HALF) {
				decoder -> bits_to_follow = 0;
			} else {
				decoder -> V -= QUARTER;
				decoder -> L -= QUARTER;
				(decoder -> bits_to_follow)++;
			}
		}
		decoder -> L += decoder -> L;	
		decoder -> R += decoder -> R;
		AddNextInputBit_Still( decoder );
	}
	return;
}

/*  Bitstream Operation Functions */


Void CVTCEncoder::
InitBitstream(Int flag,BSS *bitstream)
{
	bitstream -> ptr = bitstream->bs;
	bitstream -> cnt = 0;
	bitstream -> res = 0;
	if ( flag == 1 )
	*bitstream -> ptr = (UChar)0;
}

Int CVTCEncoder::
ByteAlignmentEnc()
{
  flush_bits();
  return(0);
}

Int CVTCDecoder::
ByteAlignmentDec()
{
  align_byte();
  return(0);
}

Void CVTCEncoder::
PutBitstoStream(Int bits,UInt code,BSS *bitstream)
{
	UChar	*p	= bitstream -> ptr;
	Int		res	= bitstream -> res;
	bitstream -> cnt += bits;
	while ( bits > 0 ) {
		*p |= (UChar) ( ((code >> (bits-1)) & 1) << (7-res) );
		res ++;
		bits--;
		if ( res == 8 ) {
			*(++p) = (UChar)0;
			res    = 0;
		}
	}
	bitstream -> ptr  = p;
	bitstream -> res  = res;

	return;
}
Void CVTCEncoder::
PutBitstoStreamMerge(Int bits,UInt code)
{
  while(bits>16) {
    emit_bits((UShort)(code>>(bits-16)), 16);
    bits-=16;
  }
  
  emit_bits((UShort)code, bits);
  return;
}

Void CVTCDecoder::
BitstreamFlushBits_Still(Int i)
{
  get_X_bits(i);
  return;
}


Int CVTCDecoder::
BitstreamLookBit(Int pos)
{
  return((Int )(LookBitFromStream(pos) &1 ) );
}

UInt CVTCDecoder::
LookBitsFromStream(Int bits)
{
    Int	i;
    UInt code=0;

    for ( i=1; i<=bits; i++ ) {
        code = (code << 1) + ( BitstreamLookBit(i) & 1 );
    }
    return ( code );
}

UInt CVTCDecoder::
GetBitsFromStream(Int bits)
{
  UInt code;
  code = get_X_bits(bits);
  return ( code );
}

Void CVTCEncoder::
BitstreamFlushBitsCopy(Int i,BSS *bitstream)
{
	Int		mdl = 0;
	Int		res = bitstream -> res + i;

	while ( res >= 8 ) {
		mdl ++;
		res -= 8;
	}
	bitstream -> ptr	+= mdl;
	bitstream -> cnt	+= i;
	bitstream -> res	 = res;

	return;
}

Int CVTCEncoder::
BitstreamLookBitCopy(Int pos,BSS *bitstream)
{
	UChar	*ptr = bitstream -> ptr;
	Int		res  = bitstream -> res + pos - 1;

	while ( res >= 8 ) {
		ptr ++;
		res -= 8;
	}

	return( (Int)( ( (*ptr) >> (7 - res) ) & 1 ) );
}

UInt CVTCEncoder::
LookBitsFromStreamCopy(Int bits,BSS *bitstream)
{
    Int	i;
    UInt code=0;

    for ( i=1; i<=bits; i++ ) {
        code = (code << 1) + ( BitstreamLookBitCopy( i, bitstream ) & 1 );
    }
    return ( code );
}

UInt CVTCEncoder::
GetBitsFromStreamCopy(Int bits,BSS *bitstream)
{
    UInt code;
    code = LookBitsFromStreamCopy(bits,bitstream);
    BitstreamFlushBitsCopy(bits,bitstream);
    return ( code );
}

Void CVTCEncoder::
BitStreamCopy(Int cnt,BSS *bitstream1,BSS *bitstream2)
{
    UInt code;
    Int	bits;

    while ( cnt>=32 ) {
        bits  = 32;
        code  = GetBitsFromStreamCopy ( bits, bitstream1 );
                PutBitstoStream   ( bits, code, bitstream2 );
        cnt  -= 32;
    }
    if ( cnt>0 ) {
        code  = GetBitsFromStreamCopy ( cnt, bitstream1 );
                PutBitstoStream   ( cnt, code, bitstream2 );
    }

    return;
}

Void CVTCEncoder::
BitStreamMerge(Int cnt,BSS *bitstream)
{
  UInt code;
  Int	bits;

  while ( cnt>=32 ) {
    bits  = 32;
    code  = GetBitsFromStreamCopy ( bits, bitstream );
    PutBitstoStreamMerge  ( bits, code);
    cnt  -= 32;
  }
  if ( cnt>0 ) {
        code  = GetBitsFromStreamCopy ( cnt, bitstream );
	PutBitstoStreamMerge   ( cnt, code );
  }

  return;
}



