/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Ralph Sperschneider (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1998.
 *                                                                           *
 ****************************************************************************/
#ifndef _buffers_h_
#define _buffers_h_

#include "bitstreamHandle.h"     /* handler */

HANDLE_BUFFER  CreateBuffer ( unsigned long bufferSize );


unsigned long  GetReadBitCnt( HANDLE_BUFFER handle );

long           GetBits ( CODE_TABLE        code,
                         int               n,
                         HANDLE_RESILIENCE hResilience,
                         HANDLE_BUFFER     hVm,
                         HANDLE_EP_INFO    hEPInfo );


void           ResetReadBitCnt ( HANDLE_BUFFER handle );

void           ResetWriteBitCnt ( HANDLE_BUFFER handle );

void           RestoreBufferPointer ( HANDLE_BUFFER handle );

void           StoreBufferPointer ( HANDLE_BUFFER handle );

void           StoreDataInBuffer ( HANDLE_BUFFER  fromHandle,
                                   HANDLE_BUFFER  toHandle,
                                   unsigned short nrOfBits );


void           setHuffdec2BitBuffer ( BsBitStream* fixed_stream );

void            byte_align ( void ); /* make it void */

#endif /* _buffers_h_  */
