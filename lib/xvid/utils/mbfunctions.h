/**************************************************************************
 *
 *  Modifications:
 *
 *  29.03.2002 removed MBFieldToFrame - no longer used (transfers instead)
 *  26.03.2002 interlacing support
 *  02.12.2001 motion estimation/compensation split
 *  16.11.2001 const/uint32_t changes to MBMotionEstComp()
 *  26.08.2001 added inter4v_mode parameter to MBMotionEstComp()
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCORE_BLOCK_H
#define _ENCORE_BLOCK_H

#include "../encoder.h"
#include "../bitstream/bitstream.h"



/** MotionEstimation **/

bool MotionEstimation(
	MACROBLOCK * const pMBs,
	MBParam * const pParam,
	const IMAGE * const pRef,
	const IMAGE * const pRefH,
	const IMAGE * const pRefV,
	const IMAGE * const pRefHV,
	IMAGE * const pCurrent, 
	const uint32_t iLimit);


/** MBMotionCompensation **/
void MBMotionCompensation(
	MACROBLOCK * const pMB,
	const uint32_t j,
	const uint32_t i,
	const IMAGE * const pRef,
	const IMAGE * const pRefH,
	const IMAGE * const pRefV,
	const IMAGE * const pRefHV,
	IMAGE * const pCurrent,
	int16_t dct_codes[6*64],
	const uint32_t width, 
	const uint32_t height,
	const uint32_t edged_width,
	const uint32_t rounding);


/** MBTransQuant.c **/


void MBTransQuantIntra(const MBParam *pParam,	 
		       MACROBLOCK * pMB,
		       const uint32_t x_pos,     /* <-- The x position of the MB to be searched */ 
		       const uint32_t y_pos,     /* <-- The y position of the MB to be searched */ 
		       int16_t data[6*64],       /* <-> the data of the MB to be coded */ 
		       int16_t qcoeff[6*64],     /* <-> the quantized DCT coefficients */ 
		       IMAGE * const pCurrent    /* <-> the reconstructed image */
                                                 /*     (function will update one MB in it with data from data[])  */
);


uint8_t MBTransQuantInter(const MBParam *pParam, /* <-- the parameter for DCT transformation 
						    and Quantization */ 
			  MACROBLOCK * pMB,
			  const uint32_t x_pos,  /* <-- The x position of the MB to be searched */ 
			  const uint32_t y_pos,  /* <-- The y position of the MB to be searched */
			  int16_t data[6*64],    /* <-> the data of the MB to be coded */ 
			  int16_t qcoeff[6*64],  /* <-> the quantized DCT coefficients */ 
			  IMAGE * const pCurrent /* <-> the reconstructed image ( function will
						    update one MB in it with data from data[] ) */
);


/** interlacing **/

uint32_t MBDecideFieldDCT(int16_t data[6*64]); /* <- decide whether to use field-based DCT
						  for interlacing */

void MBFrameToField(int16_t data[6*64]);       /* de-interlace vertical Y blocks */


/** MBCoding.c **/

void MBCoding(const MBParam *pParam, /* <-- the parameter for coding of the bitstream */
	      MACROBLOCK *pMB,       /* <-- Info of the MB to be coded */ 
	      int16_t qcoeff[6*64],  /* <-- the quantized DCT coefficients */ 
	      Bitstream * bs,        /* <-> the bitstream */ 
	      Statistics * pStat     /* <-> statistical data collected for current frame */
    );

#endif
