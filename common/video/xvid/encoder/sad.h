#ifndef _ENCODER_SAD_H_
#define _ENCODER_SAD_H_


#include "../portab.h"


typedef uint32_t (sad16Func)(const uint8_t * const cur,
						 const uint8_t * const ref,
						 const uint32_t stride,
						 const uint32_t best_sad);


typedef sad16Func* sad16FuncPtr;	

extern sad16FuncPtr sad16;
sad16Func sad16_c;
sad16Func sad16_mmx;
sad16Func sad16_xmm;

typedef uint32_t (sad8Func)(const uint8_t * const cur,
						const uint8_t * const ref,
						const uint32_t stride);


typedef sad8Func* sad8FuncPtr;	

extern sad8FuncPtr sad8;
sad8Func sad8_c;
sad8Func sad8_mmx;
sad8Func sad8_xmm;


typedef uint32_t (dev16Func)(const uint8_t * const cur,
				const uint32_t stride);


typedef dev16Func *dev16FuncPtr;	

extern dev16FuncPtr dev16;
dev16Func dev16_c;
dev16Func dev16_mmx;
dev16Func dev16_xmm;

/* plain c */
/*

uint32_t sad16(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);

uint32_t dev16(const uint8_t * const cur,
				const uint32_t stride);
*/
/* mmx */
/*

uint32_t sad16_mmx(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8_mmx(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);


uint32_t dev16_mmx(const uint8_t * const cur,
				const uint32_t stride);

*/
/* xmm */
/*
uint32_t sad16_xmm(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride,
				const uint32_t best_sad);

uint32_t sad8_xmm(const uint8_t * const cur,
				const uint8_t * const ref,
				const uint32_t stride);

uint32_t dev16_xmm(const uint8_t * const cur,
				const uint32_t stride);
*/

#endif /* _ENCODER_SAD_H_ */
