
#ifndef _TEXT_CODE_H_
#define _TEXT_CODE_H_


#include "momusys.h"
#include "mom_structs.h"
#include "text_defs.h"
#include "text_bits.h"
//#include "text_util.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* vm_enc/src/text_code.c */
Void  	VopCodeShapeTextIntraCom _P_((	Vop *curr,
			Vop *rec_curr,
			Image *mottext_bitstream
	));

Void  	VopShapeMotText _P_((	Vop *curr,
			Vop *comp,
			Image *MB_decisions,
			Image *mot_x,
			Image *mot_y,
			Int f_code_for,
			Int intra_acdc_pred_disable,
			Vop *rec_curr,
			Image *mottext_bitstream
	));
Int  	cal_dc_scaler _P_((	Int QP,
			Int type
	));


#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _TEXT_CODE_H_ */
