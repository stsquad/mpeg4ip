/*
This software module was originally developed by
Naoya Tanaka (Matsushita Communication Industrial Co., Ltd.)
and edited by
FN2 LN2 (CN2), FN3 LN3 (CN3),
in the course of development of the
MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 NBC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 NBC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 NBC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
/*----------------------------------------------------------------------*
 *    MPEG-4 Audio Verification Model (VM)                              * 
 *                                                                      *
 *	CELP based coder                                                *
 * 	  Module: pan_mv_data.c                                         *
 *                                                                      *
 *  Last modified: Sep. 25, 1996                                        *
 *----------------------------------------------------------------------*/

#include <stdio.h>

#include "buffersHandle.h"       /* handler, defines, enums */

#include "bitstream.h"
#include "pan_celp_proto.h"

/* move an array to another */
void pan_mv_cdata(char out[], char in[], long num)
{
	long i;

	for(i=0;i<num;i++) out[i] = in[i];

}

void pan_mv_sdata(short out[], short in[], long num)
{
	long i;

	for(i=0;i<num;i++) out[i] = in[i];

}

void pan_mv_ldata(long out[], long in[], long num)
{
	long i;

	for(i=0;i<num;i++) out[i] = in[i];

}

void pan_mv_fdata(float out[], float in[], long num)
{
	long i;

	for(i=0;i<num;i++) out[i] = in[i];

}

void pan_mv_ddata(double out[], double in[], long num)
{
	long i;

	for(i=0;i<num;i++) out[i] = in[i];

}

