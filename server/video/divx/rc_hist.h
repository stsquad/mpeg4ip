
#ifndef _rc_hist_H
#define _rc_hist_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void  	rch_init _P_((	RC_HIST *rch,
			Int size
	));
Void  	rch_free _P_((	RC_HIST *rch
	));
void  	rch_store_before _P_((	RC_HIST *rch,
			Int pixels,
			Double mad,
			Int qp
	));
void  	rch_store_after _P_((	RC_HIST *rch,
			Int bits_text,
			Int bits_vop,
			Double dist
	));
Int  	rch_get_last_qp _P_((	RC_HIST *rch
	));
Int  	rch_get_last_bits_vop _P_((	RC_HIST *rch
	));
Double  	rch_get_last_mad_text _P_((	RC_HIST *rch
	));
Double  	rch_get_plast_mad_text _P_((	RC_HIST *rch
	));
Int  	rch_inc_mod _P_((	RC_HIST *rch,
			Int i
	));
Int  	rch_dec_mod _P_((	RC_HIST *rch,
			Int i
	));

#ifdef __cplusplus
}
#endif /* __cplusplus  */
 
#endif /* _rc_hist_H  */ 
