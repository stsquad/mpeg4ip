
#ifndef _MOT_EST_MB_H
#define _MOT_EST_MB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Void  	MBMotionEstimation _P_((	Vop *curr_vop,
			SInt *prev,
			Int br_x,
			Int br_y,
			Int br_width,
			Int i,
			Int j,
			Int prev_x,
			Int prev_y,
			Int vop_width,
			Int vop_height,
			Int enable_8x8_mv,
			Int edge,
			Int f_code,
			Int sr,
			Float hint_mv_w,
			Float hint_mv_h,
			Float *mv16_w,
			Float *mv16_h,
			Float *mv8_w,
			Float *mv8_h,
			Int *min_error,
			SInt *flags
	));
Void  	FindSubPel _P_((	Int x,
			Int y,
			SInt *prev,
			SInt *curr,
			Int bs_x,
			Int bs_y,
			Int comp,
			Int rel_x,
			Int rel_y,
			Int pels,
			Int lines,
			Int edge,
			SInt *flags,
			SInt *corr_comp_mb,
			Float *mvx,
			Float *mvy,
			Int *min_error
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _MOT_EST_MB_H  */ 
