/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
Bernd Edler and Hendrik Fuchs, University of Hannover in the course of
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7,
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
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
/*
 * $Id: monopred.h,v 1.2 2002/01/11 00:55:17 wmaycisco Exp $
 */

#ifndef _monopred_h_
#define _monopred_h_

#define MAX_PGRAD   2
#define MINVAR      1
#define Q_ZERO      0x0000
#define Q_ONE       0x3F80

/* Predictor status information for mono predictor */
typedef struct {
    short   r[MAX_PGRAD];       /* contents of delay elements */
    short   kor[MAX_PGRAD];         /* estimates of correlations */
    short   var[MAX_PGRAD];         /* estimates of variances */
} PRED_STATUS;

typedef struct {
    float   r[MAX_PGRAD];       /* contents of delay elements */
    float   kor[MAX_PGRAD];         /* estimates of correlations */
    float   var[MAX_PGRAD];         /* estimates of variances */
} TMP_PRED_STATUS;

#endif /* _monopred_h_ */
