

/* mot_code.h */

#ifndef _MOT_CODE_H_
#define _MOT_CODE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Int  	Bits_CountMB_Motion _P_((	Image *mot_h,
			Image *mot_v,
			Image *alpha,
			Image *modes,
			Int h,
			Int v,
			Int f_code,
			Int quarter_pel,
			Image *bs,
			Int error_res_disable,
			Int after_marker,
			Int **slice_nb,
			Int arbitrary_shape
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _MOT_CODE_H_ */



