/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of
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
 * $Id: block.h,v 1.6 2003/05/23 18:55:19 wmaycisco Exp $
 */

#ifndef BLOCK_H
#define BLOCK_H 1

#define IN_DATATYPE  double
#define OUT_DATATYPE double

#ifndef BLOCK_LEN_LONG
#define BLOCK_LEN_LONG   1024
#endif
#define BLOCK_LEN_SHORT  128


#define NWINLONG    (BLOCK_LEN_LONG)
#define ALFALONG    4.0
#define NWINSHORT   (BLOCK_LEN_SHORT)
#define ALFASHORT   7.0

#define NWINFLAT    (NWINLONG)                  /* flat params */
#define NWINADV     (NWINLONG-NWINSHORT)        /* Advanced flat params */
#define NFLAT       ((NWINFLAT-NWINSHORT)/2)
#define NADV0       ((NWINADV-NWINSHORT)/2)

#define START_OFFSET 0
#define SHORT_IN_START_OFFSET 5
#define STOP_OFFSET 3
#define SHORT_IN_STOP_OFFSET 0

#define WEAVE_START 0       /* type parameter for start blocks */
#define WEAVE_STOP  1       /* type paremeter for stop blocks */



typedef enum {                  /* ADVanced transform types */
    LONG_BLOCK,
    START_BLOCK,
    SHORT_BLOCK,
    STOP_BLOCK,
    START_ADV_BLOCK,
    STOP_ADV_BLOCK,
    START_FLAT_BLOCK,
    STOP_FLAT_BLOCK,
    N_BLOCK_TYPES
}
BLOCK_TYPE;


void unfold (Float *data_in, Float *data_out, int inLeng);
void InitBlock(faacDecHandle hDecoder);
void EndBlock(faacDecHandle hDecoder);
void ITransformBlock(faacDecHandle hDecoder,Float* dataPtr, BLOCK_TYPE wT, Wnd_Shape *ws, Float *state);
void TransformBlock(faacDecHandle hDecoder,Float* dataPtr, BLOCK_TYPE bT, Wnd_Shape *wnd_shape);

#endif  /*  BLOCK_H */
