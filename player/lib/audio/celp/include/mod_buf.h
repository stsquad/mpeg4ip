/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 

AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by


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
Copyright(c)1999.
 *                                                                           *
 ****************************************************************************/

#ifndef __MOD_BUF_H
#define __MOD_BUF_H

#if 0
typedef  unsigned int UINT ;
typedef  float FLOAT;
typedef  unsigned char  BOOL ;

typedef struct{
    UINT  size;
    UINT  current_position;
    UINT  readPosition;
    FLOAT *paBuffer;
    int  numVal;
}MODULO_BUFFER;


typedef MODULO_BUFFER *HANDLE_MODULO_BUFFER;

HANDLE_MODULO_BUFFER CreateFloatModuloBuffer(UINT size);
#else
HANDLE_MODULO_BUFFER CreateFloatModuloBuffer(unsigned int size);
#endif
void                 DeleteFloatModuloBuffer(HANDLE_MODULO_BUFFER hModuloBuffer);
unsigned char        AddFloatModuloBufferValues(HANDLE_MODULO_BUFFER hModuloBuffer, const float *values, unsigned int n);
unsigned char        ZeroFloatModuloBuffer(HANDLE_MODULO_BUFFER hModuloBuffer, unsigned int n);
unsigned char        GetFloatModuloBufferValues(HANDLE_MODULO_BUFFER hModuloBuffer, float *values, unsigned int n, unsigned int age);
unsigned char        ReadFloatModuloBufferValues(HANDLE_MODULO_BUFFER hModuloBuffer, float *values, unsigned int n);
int                  GetNumVal ( HANDLE_MODULO_BUFFER hModuloBuffer );
#endif
