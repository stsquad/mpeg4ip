
#ifndef _MOT_EST_H_
#define _MOT_EST_H_

#include "vm_common_defs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Void  	MotionEstimationCompensation _P_((	Vop *curr_vop,
			Vop *prev_rec_vop,
			Int enable_8x8_mv,
			Int edge,
			Int f_code,
			Vop *curr_comp_vop,
			Float *mad,
			Image **mot_x,
			Image **mot_y,
			Image **mode
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _MOT_EST_H_ */

