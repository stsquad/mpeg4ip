/**************************************************************************
 *                                                                        *
 * This code has been developed by John Funnell. This software is an      *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * John Funnell
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
**/
// postprocess.h //

/* Currently this contains only the deblocking filter.  The vertial    */
/* deblocking filter operates over eight pixel-wide columns at once.  The  */
/* horizontal deblocking filter works on four horizontals row at a time. */

/* Picture height must be multiple of 8, width a multiple of 16 */



#ifndef POSTPROCESS_H
#define POSTPROCESS_H



#include "portab.h"


/**** Compile-time options ****/

/* the following parameters allow for some tuning of the postprocessor */
#define DEBLOCK_HORIZ_USEDC_THR    (28 -  8)
#define DEBLOCK_VERT_USEDC_THR     (56 - 16)

/* SHOWDECISIONS(_H/_V) enables you to see where the deblocking filter has used DC filtering (black) and default filtering (white) */
//#define SHOWDECISIONS_H
//#define SHOWDECISIONS_V

/* When defined, PP_SELF_CHECK causes the postfilter to double check every */
/* computation it makes.  For development use. */
//#define PP_SELF_CHECK

/* Type to use for QP. This may depend on the decoder's QP store implementation */
//#define TSINGHUA

#define QP_STORE_T int

#ifdef TSINGHUA
#define QP_STORE_T int16_t
#endif

/*** decore parameter mask ***/
#define PP_DEBLOCK_Y_H_MASK		0x00ff0000
#define PP_DEBLOCK_Y_V_MASK		0x0000ff00
#define PP_DERING_Y_MASK			0x000000ff

/**** Function prototype - entry point for postprocessing ****/
void postprocess(unsigned char * src[], int src_stride,
                 unsigned char * dst[], int dst_stride, 
                 int horizontal_size,   int vertical_size, 
                 QP_STORE_T *QP_store,  int QP_stride,
		 					   int mode);

/**** mode flags to control postprocessing actions ****/
#define PP_DEBLOCK_Y_H  0x00000001  /* Luma horizontal deblocking   */
#define PP_DEBLOCK_Y_V  0x00000002  /* Luma vertical deblocking     */
#define PP_DEBLOCK_C_H  0x00000004  /* Chroma horizontal deblocking */
#define PP_DEBLOCK_C_V  0x00000008  /* Chroma vertical deblocking   */
#define PP_DERING_Y     0x00000010  /* Luma deringing               */
#define PP_DERING_C     0x00000020  /* Chroma deringing             */
#define PP_DONT_COPY    0x10000000  /* Postprocessor will not copy src -> dst */
                                    /* instead, it will operate on dst only   */





#endif
