/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
 "This software module was originally developed by 
 Fraunhofer Gesellschaft IIS / University of Erlangen (UER) in the course of 
 development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
 14496-1,2 and 3. This software module is an implementation of a part of one or more 
 MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
 Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
 standards free license to this software module or modifications thereof for use in 
 hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
 Audio standards. Those intending to use this software module in hardware or 
 software products are advised that this use may infringe existing patents. 
 The original developer of this software module and his/her company, the subsequent 
 editors and their companies, and ISO/IEC have no liability for use of this software 
 module or modifications thereof in an implementation. Copyright is not released for 
 non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
 retains full right to use the code for his/her  own purpose, assign or donate the 
 code to a third party and to inhibit third party from using the code for non 
 MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
 be included in all copies or derivative works." 
 Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

/* CREATED BY :  Eric Allamanche -- 990112  */


/* ADIF header Parser for AAC raw bitstreams (only basic functionalities) */
/* Note: only front channel elements are decoded */

#include <stdio.h>
#include <string.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstreamStruct.h"     /* structs */

#include "adif.h"
#include "common_m4a.h"

static const int samplingRateTable[] = { 96000,
                                         88200,
                                         64000,
                                         48000,
                                         44100,
                                         32000,
                                         24000,
                                         22050,
                                         16000,
                                         12000,
                                         11025,
                                          8000};

static int  GetSamplingRate( int index )
{
  if ( (index < 0) || 
       (index > (int) (sizeof(samplingRateTable)/sizeof(samplingRateTable[0]))) )
    return 0;
  else
    return samplingRateTable[index];
}



static int CheckProfile( int profile )
{
  if ( (profile < 0) || (profile > 2))
    return 1;
  else
    return 0;
}


static void GetProgConfig( BsBitStream* bitstream, ADIF_INFO* adifInfo )
{
  
  unsigned long data;
  int i, tmp;


  /* Instance Tag */
  BsGetBit( bitstream, &data, 4 );
 
 
  /* profile */
  BsGetBit( bitstream, &data, 2 );
  tmp = (int)data;
  if ( CheckProfile(tmp) )
    CommonExit(1,"Illegal profile");

  adifInfo->profile = tmp;


  /* Sampling frequency index */
  BsGetBit( bitstream, &data, 4 );
  tmp = GetSamplingRate((int)data); 
  if ( !tmp )
    CommonExit(1,"Illegal frequency index");

  adifInfo->samplingRate = tmp;


  /* number of front channel elements: 
     only one element (SCE or CPE) is supported */
  BsGetBit( bitstream, &data, 4 );
  if ( data != 1)
    CommonExit(1,"Unsupported number of front channels");

  /* number of side channel elements (not supported) */
  BsGetBit( bitstream, &data, 4 );
  if ( data )
    CommonExit(1,"Unsupported channel element");    

  /* number of back channel elements (not supported) */
  BsGetBit( bitstream, &data, 4 );
  if ( data )
    CommonExit(1,"Unsupported channel element");    
  
  /* number of LFE channel elements (not supported) */
  BsGetBit( bitstream, &data, 2 );
  if ( data )
    CommonExit(1,"Unsupported channel element");    
  
  /* number of data elements (not supported) */
  BsGetBit( bitstream, &data, 3 );
  if ( data )
    CommonExit(1,"Unsupported channel element");    
  
  /* number of coupling channel elements (not supported) */
  BsGetBit( bitstream, &data, 4 );
  if ( data )
    CommonExit(1,"Unsupported channel element");    
  

  /* mono mixdown present */
  BsGetBit( bitstream, &data, 1 );
  if ( data )
    BsGetBit( bitstream, &data, 4 );

  /* stereo mixdown present */
  BsGetBit( bitstream, &data, 1 );
  if ( data )
    BsGetBit( bitstream, &data, 4 );

  /* matrix mixdown index present */
  BsGetBit( bitstream, &data, 1 );
  if ( data ) {
    BsGetBit( bitstream, &data, 2 );
    BsGetBit( bitstream, &data, 1 );
  }

  /* we only have to parse one front channel element */
  BsGetBit( bitstream, &data, 1 );
  if ( !data )
    adifInfo->numChannels = 1; /* single channel */
  else 
    adifInfo->numChannels = 2; /* channel pair */

  /* instance tag */
  BsGetBit( bitstream, &data, 4 );
  adifInfo->elementTag = (int)data;

  /* The other channel elements should be parsed here !! */

  /* byte allignment */
  data = BsCurrentBit( bitstream );
  tmp = ((int)data)%8;
  if ( tmp )
    BsGetSkip( bitstream, 8 - tmp );

  /* Comment field */
  BsGetBit( bitstream, &data, 8 );
  tmp = (int)data;

  for (i=0; i<tmp; i++) {
    BsGetBit( bitstream, &data, 8 );
    adifInfo->commentField[i] = (char)data;
  }
  adifInfo->commentField[i] = '\0';

}


int GetAdifHeader( BsBitStream* bitstream, ADIF_INFO* adifInfo )
{

  int i;
  int ADIF_present = 0;
  unsigned long data;
  BsBitBuffer* bitBuffer;

  const char ADIF_ID[] = "ADIF";
  const int ADIF_ID_LEN = 32;

  bitBuffer = BsAllocBuffer( ADIF_ID_LEN );

  /* Check if ADIF ID is present */
  BsGetBufferAhead( bitstream, bitBuffer, ADIF_ID_LEN );

  if ( !strncmp( (char*)bitBuffer->data, ADIF_ID, strlen(ADIF_ID))) {

    /* skip ADIF_ID from bitstream */
    BsGetSkip( bitstream, ADIF_ID_LEN );

    /* copyright_id_present bit */
    BsGetBit( bitstream, &data, 1);
    i = 0;
    if( data ) {
      while (i<9) {
        BsGetBit( bitstream, &data, 8 );
        adifInfo->copyrightId[i++] = (char)data;
      }
    }
    
    adifInfo->copyrightId[i] = '\0';

    /* original_copy bit */
    BsGetBit( bitstream, &data, 1);
    adifInfo->origCopy = (int)data;
    
    /* home bit */
    BsGetBit( bitstream, &data, 1);
    adifInfo->home = (int)data;

    /* bitstream_type bit (0: constant rate, 1: variable rate) */
    BsGetBit( bitstream, &data, 1);
    adifInfo->varRateOn = (int)data;

    /* bitrate */
    BsGetBit( bitstream, &data, 23 );
    adifInfo->bitrate = (int)data;

    /* num_pce's */
    BsGetBit( bitstream, &data, 4 );

    if ( data )
      CommonExit(1, "Only one PCE supported");

    /* buffer fullness */
    if ( !adifInfo->varRateOn ) {
      BsGetBit( bitstream, &data, 20);
      adifInfo->bitresFullnes = (int)data;
    }

    /* parse program config element */
    GetProgConfig( bitstream, adifInfo );

    ADIF_present = 1;
  }

  BsFreeBuffer( bitBuffer );

  return ADIF_present;
}

