
#ifndef _rc_q2_H
#define _rc_q2_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void  	RCQ2_init _P_((VolConfig *, int
	));
void  	RCQ2_Free _P_((
	));
Int  	RCQ2_QuantAdjust _P_((
			Double mad,
			UInt num_pels_vop,
			Int frame_type
	));
void  	RCQ2_Update2OrderModel _P_((	
			Int vopbits,
			Int frame_type
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _rc_q2_H  */ 
