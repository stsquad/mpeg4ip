/**********************************************************************
MPEG-4 Audio VM
Encoder core (lpc-based)



This software module was originally developed by

Heiko Purnhagen (University of Hannover)

and edited by

Toshiyuki Nomura (NEC Corporation)

in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard
ISO/IEC 13818-7, 14496-1,2 and 3. This software module is an
implementation of a part of one or more MPEG-2 NBC/MPEG-4 Audio tools
as specified by the MPEG-2 NBC/MPEG-4 Audio standard. ISO/IEC gives
users of the MPEG-2 NBC/MPEG-4 Audio standards free license to this
software module or modifications thereof for use in hardware or
software products claiming conformance to the MPEG-2 NBC/ MPEG-4 Audio
standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing
patents. The original developer of this software module and his/her
company, the subsequent editors and their companies, and ISO/IEC have
no liability for use of this software module or modifications thereof
in an implementation. Copyright is not released for non MPEG-2
NBC/MPEG-4 Audio conforming products. The original developer retains
full right to use the code for his/her own purpose, assign or donate
the code to a third party and to inhibit third party from using the
code for non MPEG-2 NBC/MPEG-4 Audio conforming products. This
copyright notice must be included in all copies or derivative works.

Copyright (c) 1996.



Header file: mp4_lpc.h

$Id: mp4_lpc.h,v 1.1 2002/05/13 18:57:42 wmaycisco Exp $

Authors:
HP    Heiko Purnhagen, Uni Hannover <purnhage@tnt.uni-hannover.de>
TN    Toshiyuki Nomura, NEC <t-nomura@dsp.cl.nec.co.jp>

Changes:
09-aug-96   HP    first version
26-aug-96   HP    CVS
28-oct-96   HP    added MP4ModeLpc
07-Feb-97   TN    adapted to Rate Control functionality
**********************************************************************/


#ifndef _mp4_lpc_h_
#define _mp4_lpc_h_


/* ---------- declarations ---------- */

/* parametric core mode */

enum MP4ModeLpc {MODELPC_UNDEF, MODELPC_PANNEC8, MODELPC_PHI16, MODELPC_NUM};

static char *MP4ModeLpcName[MODELPC_NUM] = {"undefined", "pannec_8", "phi_16"};


#endif	/* #ifndef _mp4_lpc_h_ */

/* end of mp4_lpc.h */

