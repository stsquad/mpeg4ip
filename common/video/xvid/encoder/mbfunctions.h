/**************************************************************************
 *
 *  Modifications:
 *
 *  02.12.2001 motion estimation/compensation split
 *  16.11.2001 const/uint32_t changes to MBMotionEstComp()
 *  26.08.2001 added inter4v_mode parameter to MBMotionEstComp()
 *
 *  Michael Militzer <isibaar@videocoding.de>
 *
 **************************************************************************/

#ifndef _ENCORE_BLOCK_H
#define _ENCORE_BLOCK_H

#include "encoder.h"
#include "bitwriter.h"



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
		    int16_t dct_codes[][64],
			const uint32_t width, 
			const uint32_t height,
			const uint32_t edged_width,
			const uint32_t rounding);


/** MBTransQuant.c **/


void MBTransQuantIntra(const MBParam *pParam,	 
		       const uint32_t x_pos, 		 /* <-- The x position of the MB to be searched */ 
		       const uint32_t y_pos, 		 /* <-- The y position of the MB to be searched */ 
		       int16_t data[][64],	 /* <-> the data of the MB to be coded */ 
		       int16_t qcoeff[][64], 	 /* <-> the quantized DCT coefficients */ 
		       IMAGE * const pCurrent         /* <-> the reconstructed image ( function will update one
   	         							    MB in it with data from data[] ) */
);


uint8_t MBTransQuantInter(const MBParam *pParam, /* <-- the parameter for DCT transformation 
						 						   and Quantization */ 
			   const uint32_t x_pos, 	 /* <-- The x position of the MB to be searched */ 
			   const uint32_t y_pos,	 /* <-- The y position of the MB to be searched */
			   int16_t data[][64], 	 /* <-> the data of the MB to be coded */ 
			   int16_t qcoeff[][64], /* <-> the quantized DCT coefficients */ 
			   IMAGE * const pCurrent		 /* <-> the reconstructed image ( function will
								    update one MB in it with data from data[] ) */
);


/** MBPrediction.c **/

void MBPrediction(MBParam *pParam,	 /* <-- the parameter for ACDC and MV prediction */
		  uint32_t x_pos,		 /* <-- The x position of the MB to be searched */
		  uint32_t y_pos, 		 /* <-- The y position of the MB to be searched */ 
		  uint32_t x_dim,		 /* <-- Number of macroblocks in a row */
		  int16_t qcoeff[][64], 	 /* <-> The quantized DCT coefficients */ 
		  MACROBLOCK *MB_array		 /* <-> the array of all the MB Infomations */
    );

/** MBCoding.c **/

void MBCoding(const MBParam *pParam,	 	 /* <-- the parameter for coding of the bitstream */
	      const MACROBLOCK *pMB, 		 /* <-- Info of the MB to be coded */ 
	      int16_t qcoeff[][64], 		 /* <-- the quantized DCT coefficients */ 
	      Bitstream * bs, 			 /* <-> the bitstream */ 
	      Statistics * pStat		 /* <-> statistical data collected for current frame */
    );

#endif
