

#ifndef _VM_VOP_CODE_H_
#define _VM_VOP_CODE_H_

#include "bitstream.h"
#include "text_code.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct bit_count {
  Char		stats_file[100];
	UInt		vol;
	UInt		vop;
	UInt		syntax;
	UInt		texture;
	UInt		motion;
	UInt		mot_shape_text;
    Bits        text_bits;
};

typedef struct bit_count BitCount;

Void  	VopCode _P_((	Vop *curr,
			Vop *reference,
			Vop *compensated,
			Vop *error,
			Int enable_8x8_mv,
			Float time,
			VolConfig *vol_config
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _VM_VOP_CODE_H_ */

