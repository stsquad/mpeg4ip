
#ifndef _TEXT_CODE_MB_H_
#define _TEXT_CODE_MB_H_

#include <math.h>
#include "momusys.h"
#include "text_defs.h"
//#include "text_util.h"
//#include "text_quant.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Void 	CodeMB _P_((	Vop *curr,
			Vop *rec_curr,
			Vop *comp,
			Int x_pos,
			Int y_pos,
			UInt width,
			Int QP,
			Int Mode,
			Int *qcoeff
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _TEXT_CODE_MB_H_ */
