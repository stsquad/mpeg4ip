#ifndef _MEM_TRANSFER_H
#define _MEM_TRANSFER_H

// transfer8to16
typedef void (TRANSFER_8TO16COPY)(int16_t * const dst,
					const uint8_t * const src,
					uint32_t stride);
typedef TRANSFER_8TO16COPY* TRANSFER_8TO16COPY_PTR;	
extern TRANSFER_8TO16COPY_PTR transfer_8to16copy;
TRANSFER_8TO16COPY transfer_8to16copy_c;
TRANSFER_8TO16COPY transfer_8to16copy_mmx;

// transfer16to8
typedef void (TRANSFER_16TO8COPY)(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);
typedef TRANSFER_16TO8COPY* TRANSFER_16TO8COPY_PTR;	
extern TRANSFER_16TO8COPY_PTR transfer_16to8copy;
TRANSFER_16TO8COPY transfer_16to8copy_c;
TRANSFER_16TO8COPY transfer_16to8copy_mmx;

// transfer8to16sub
typedef void (TRANSFER_8TO16SUB)(int16_t * const dct,
				uint8_t * const cur,
				const uint8_t * ref,
				const uint32_t stride);
typedef TRANSFER_8TO16SUB* TRANSFER_8TO16SUB_PTR;

extern TRANSFER_8TO16SUB_PTR transfer_8to16sub;
TRANSFER_8TO16SUB transfer_8to16sub_c;
TRANSFER_8TO16SUB transfer_8to16sub_mmx;

// transfer16to8add
typedef void (TRANSFER_16TO8ADD)(uint8_t * const dst,
					const int16_t * const src,
					uint32_t stride);
typedef TRANSFER_16TO8ADD* TRANSFER_16TO8ADD_PTR;	
extern TRANSFER_16TO8ADD_PTR transfer_16to8add;
TRANSFER_16TO8ADD transfer_16to8add_c;
TRANSFER_16TO8ADD transfer_16to8add_mmx;

// transfer8x8_copy
typedef void (TRANSFER8X8_COPY)(uint8_t * const dst,
					const uint8_t * const src,
					const uint32_t stride);
typedef TRANSFER8X8_COPY* TRANSFER8X8_COPY_PTR;	
extern TRANSFER8X8_COPY_PTR transfer8x8_copy;
TRANSFER8X8_COPY transfer8x8_copy_c;
TRANSFER8X8_COPY transfer8x8_copy_mmx;

#endif /* _MEM_TRANSFER_H_ */
