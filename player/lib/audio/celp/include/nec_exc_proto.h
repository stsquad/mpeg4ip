/*
This software module was originally developed by
Toshiyuki Nomura (NEC Corporation)
and edited by

in the course of development of the
MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 14496-1,2 and 3.
This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 Audio
standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio standards
free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 NBC/
MPEG-4 Audio  standards. Those intending to use this software module in
hardware or software products are advised that this use may infringe
existing patents. The original developer of this software module and
his/her company, the subsequent editors and their companies, and ISO/IEC
have no liability for use of this software module or modifications
thereof in an implementation. Copyright is not released for non
MPEG-2 NBC/MPEG-4 Audio conforming products. The original developer
retains full right to use the code for his/her  own purpose, assign or
donate the code to a third party and to inhibit third party from using
the code for non MPEG-2 NBC/MPEG-4 Audio conforming products.
This copyright notice must be included in all copies or derivative works.
Copyright (c)1996.
*/
#ifndef nec_exc_proto_h_
#define nec_exc_proto_h_

#ifdef __cplusplus
extern "C" {
#endif

void nec_mode_decision(
		       float	input_signal[],	/* input */
		       float	wn_alpha[],	/* input */
		       float	wd_alpha[],	/* input */
		       long	lpc_order,	/* configuration input */
		       long	frame_size,	/* configuration input */
		       long	sbfrm_size,	/* configuration input */
		       long	lag_idx_cand[],	/* output */
		       long	*vu_flag,	/* output */
                       long SampleRateMode);      /*input */

void nec_enc_rms(
		 float	xnorm[],	/* input */
		 float	qxnorm[],	/* output */
		 long	n_subframes,	/* configuration input */
		 float	rms_max,	/* configuration input */
		 float	mu_law,		/* configuration input */
		 long	rmsbit,		/* configuration input */
		 long	*rms_index );	/* output */

void nec_dec_rms(
		 float	qxnorm[],	/* output */
		 long	n_subframes,	/* configuration input */
		 float	rms_max,	/* configuration input */
		 float	mu_law,		/* configuration input */
		 long	rmsbit,		/* configuration input */
		 long	rms_index );	/* input */

void nec_bws_rms_dec(
		 float	qxnorm[],	/* output */
		 long	n_subframes,	/* configuration input */
		 float	rms_max,	/* configuration input */
		 float	mu_law,		/* configuration input */
		 long	rmsbit,		/* configuration input */
		 long	rms_index		/* input */
		 );

void nec_enc_acb(
		 float	xw[],		/* input */
		 float	*og_ac,		/* output */
		 float	ac[],		/* output */
		 float	syn_ac[],	/* output */
		 long	op_idx,		/* configuration input */
		 long	st_idx,		/* configuration input */
		 long	ed_idx,		/* configuration input */
		 long	*ac_idx_opt,	/* output */
		 long	lpc_order,	/* configuration input */
		 long	len_sf,		/* configuration input */
		 long	lagbit,		/* configuration input */
		 float	alpha[],	/* input */
		 float	g_den[],	/* input */
		 float	g_num[],	/* input */
		 float	mem_past_exc[],	/* input */
		 long	*int_part,	/* output */
                 long SampleRateMode);    /* configuration input */

void nec_dec_acb(
		 float	ac[],		/* output */
		 long	ac_idx_opt,	/* input */
		 long	len_sf,		/* configuration input */
		 long	lagbit,		/* configuration input */
		 float	mem_past_exc[],	/* input */
		 long	*int_part,	/* output */
                 long SampleRateMode);    /* configuration input */

void nec_bws_acb_dec(
		 float	ac[],		/* output */
		 long	ac_idx_opt,	/* input */
		 long	len_sf,		/* configuration input */
		 long	lagbit,		/* configuration input */
		 float	mem_past_exc[],	/* input */
		 long	*int_part);	/* output */

void nec_bws_acb_enc(
		 float	xw[],		/* input */
		 float	*og_ac,		/* output */
		 float	ac[],		/* output */
		 float	syn_ac[],	/* output */
		 long	op_idx,		/* configuration input */
		 long	st_idx,		/* configuration input */
		 long	ed_idx,		/* configuration input */
		 long	*ac_idx_opt,	/* output */
		 long	lpc_order,	/* configuration input */
		 long	len_sf,		/* configuration input */
		 long	lagbit,		/* configuration input */
		 float	alpha[],	/* input */
		 float	g_den[],	/* input */
		 float	g_num[],	/* input */
		 float	mem_past_exc[],	/* input */
		 long	*int_part	/* output */
);

void nec_pitch_enhancement(
			   float exc[],		/* input */
			   float enh_exc[],	/* output */
			   float mem_pitch[],	/* input */
			   long  vu_flag,	/* input */
			   long  idx,		/* input */
			   long  len_sf, 
			   long SampleRateMode );  /* configuration input */

void nec_bws_pitch_enhancement(
			   float exc[],		/* input */
			   float enh_exc[],	/* output */
			   float mem_pitch[],	/* input */
			   long  vu_flag,	/* input */
			   long  idx,		/* input */
			   long  len_sf); 	/* configuration input */

void nec_enc_mp(
		long	vu_flag,	/* input */
		float	z[],		/* input */
		float	syn_ac[],	/* input */
		float	og_ac,		/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float	qxnorm,		/* input */
		long	I_part,		/* input */
		long	*pos_idx,	/* output */
		long	*sgn_idx,	/* output */
		float	comb_exc[],	/* output */
		float	ac[],		/* input */
		float	alpha[],	/* input */
		float	g_den[],	/* input */
		float	g_num[],	/* input */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	*ga_idx );	/* output */

void nec_dec_mp(
		long	vu_flag,	/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float	qxnorm,		/* input */
		float	alpha[],	/* input */
		long	I_part,		/* input */
		long	pos_idx,	/* input */
		long	sgn_idx,	/* input */
		float	comb_exc[],	/* output */
		float	ac[],		/* input */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	ga_idx );	/* input */

void nec_bws_mp_enc(
		long	vu_flag,	/* input */
		float	z[],		/* input */
		float	syn_ac[],	/* input */
		float	og_ac,		/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float   *g_mp8,
		float	qxnorm,		/* input */
		long	I_part,		/* input */
		long	*pos_idx,	/* output */
		long	*sgn_idx,	/* output */
		float	comb_exc[],	/* output */
		float	ac[],		/* input */
		float   mpexc_8[],
		float	alpha[],	/* input */
		float	g_den[],	/* input */
		float	g_num[],	/* input */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	*ga_idx
	         );	

void nec_bws_mp_dec(
		long	vu_flag,	/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float   *g_mp8,
		float	qxnorm,		/* input */
		float	alpha[],	/* input */
		long	I_part,		/* input */
		long	pos_idx,	/* input */
		long	sgn_idx,	/* input */
		float	comb_exc[],	/* output */
		float	ac[],		/* input */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	ga_idx ); 	/* input */

void nec_enc_gain(
		  long	vu_flag,	/* input */
		  long	pul_loc[],	/* input */
		  float	pul_amp[],	/* input */
		  long	num_pulse,	/* configuration input */
		  long	I_part,		/* input */
		  long	cand_gain,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec,		/* output */
		  float	qxnorm,		/* input */
		  float	z[],		/* input */
		  float	ac[],		/* input */
		  float	facx[],		/* input */
		  float	alpha[],	/* input */
		  float	g_den[],	/* input */
		  float	g_num[],	/* input */
		  long	lpc_order,	/* configuration input */
		  long	len_sf,		/* configuration input */
		  long	gainbit,	/* configuration input */
		  long	*ga_idx );	/* output */

void nec_dec_gain(
		  long	vu_flag,	/* input */
		  float	qxnorm,		/* input */
		  float	alpha[],	/* input */
		  float	ac[],		/* input */
		  float	comb_exc[],	/* input */
		  long	len_sf,		/* configuration input */
		  long	ga_idx,		/* input */
		  long	lpc_order,	/* configuration input */
		  long	gainbit,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec );	/* output */

void nec_bws_gain_enc(
		  long	vu_flag,	/* input */
		  long	pul_loc[],	/* input */
		  float	pul_amp[],	/* input */
		  long	num_pulse,	/* configuration input */
		  long	I_part,		/* input */
		  long	cand_gain,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec,		/* output */
		  float	*g_mp8,		/* output */
		  float	qxnorm,		/* input */
		  float	z[],		/* input */
		  float	ac[],		/* input */
		  float	facx[],		/* input */
		  float	syn_mp8[],	/* input */
		  float	alpha[],	/* input */
		  float	g_den[],	/* input */
		  float	g_num[],	/* input */
		  long	lpc_order,	/* configuration input */
		  long	len_sf,		/* configuration input */
		  long	gainbit,	/* configuration input */
		  long	*ga_idx		/* output */
);

void nec_bws_gain_dec(
		  long	vu_flag,	/* input */
		  float	qxnorm,		/* input */
		  float	alpha[],	/* input */
		  float	ac[],		/* input */
		  float	comb_exc[],	/* input */
		  long	len_sf,		/* configuration input */
		  long	ga_idx,		/* input */
		  long	lpc_order,	/* configuration input */
		  long	gainbit,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec,		/* output */
		  float	*g_mp8 );		/* output */

void nec_zero_filt(
		   float	x[], 		/* input */
		   float	y[], 		/* output */
		   float	alpha[],	/* input */
		   float	g_den[],	/* input */
		   float	g_num[],	/* input */
		   long		order,		/* configuration input */
		   long		len );		/* configuration input */

void nec_pw_imprs(
		  float	y[],		/* output */
		  float	a[],		/* input */
		  long	m,		/* configuration input */
		  float	g_den[],	/* input */
		  float	g_num[],	/* input */
		  long n );		/* configuration input */

void nec_comb_filt(
		   float	exc[],		/* input */
		   float	comb_exc[],	/* output */
		   long		len_sf,		/* configuration input */
		   long		I_part,		/* input */
		   long		flag );		/* input */

void nec_syn_filt(
		  float	di[],	/* input */
		  float	a[],	/* input */
		  float	pm[],	/* input/output */
		  float	xr[],	/* output */
		  long	np,	/* configuration input */
		  long	n );	/* configuration input */

void nec_pw_filt(
		 float		y[],	/* output */
		 float		x[],	/* input */
		 long		m,	/* configuration input */
		 float		gd[],	/* input */
		 float		gn[],	/* input */
		 float		pmem1[],/* input/output */
		 float		pmem2[],/* input/output */
		 long		n );	/* configuration input */

void nec_mp_position(
			    long	len,     /* input */
			    long	num,     /* input */
			    long	bit[],   /* output */
			    long	pos[] ); /* output */

void nec_lpc2par( float a[], float rc[], long m );

long nec_acb_generation(long idx, long len_sf, float mem_ac[],
			       float exci[], float exco[],
			       float ga, long type ,long SampleRateMode);

long nec_acb_generation_16(long idx, long len_sf, float mem_ac[],
			       float exci[], float exco[],
			       float ga, long type );

void nec_enh_mp_position(
			 long	len,		/* input */
			 long	num[],		/* input */
			 long	idx[],		/* input */
			 long	num_enh,	/* input */
			 long	bit[],		/* output */
			 long	pos[] );	/* output */

void nec_enh_mp_enc(
		long	vu_flag,	/* input */
		float	z[],		/* input */
		float	syn_ac[],	/* input */
		float	og_ac,		/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float	qxnorm,		/* input */
		long	I_part,		/* input */
		long	*pos_idx,	/* output */
		long	*sgn_idx,	/* output */
		float	comb_exc[],	/* output */
		float	ac[],		/* input */
		float	alpha[],	/* input */
		float	g_den[],	/* input */
		float	g_num[],	/* input */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse[],	/* configuration input */
		long	pre_indices[],	/* configuration input */
		long	num_enh,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	*ga_idx );	/* output */

void nec_enh_mp_dec(
		long	vu_flag,	/* input */
		float	*g_ac,		/* output */
		float	*g_ec,		/* output */
		float	qxnorm,		/* input */
		float	alpha[],	/* input */
		long	I_part,		/* input */
		long	pos_idx,	/* input */
		long	sgn_idx,	/* input */
		float	comb_exc[],	/* output */
		long	lpc_order,	/* configuration input */
		long	len_sf,		/* configuration input */
		long	num_pulse[],	/* configuration input */
		long	pre_indices[],	/* configuration input */
		long	num_enh,	/* configuration input */
		long	gainbit,	/* configuration input */
		long	ga_idx );	/* input */

void nec_enh_gain_enc(
		      long	vu_flag,	/* input */
		      long	pul_loc[],	/* input */
		      float	pul_amp[],	/* input */
		      long	num_pulse,	/* configuration input */
		      long	I_part,		/* input */
		      long	cand_gain,	/* configuration input */
		      float	*g_ac,		/* output */
		      float	*g_ec,		/* output */
		      float	qxnorm,		/* input */
		      float	z[],		/* input */
		      float	ac[],		/* input */
		      float	facx[],		/* input */
		      float	alpha[],	/* input */
		      float	g_den[],	/* input */
		      float	g_num[],	/* input */
		      long	lpc_order,	/* configuration input */
		      long	len_sf,		/* configuration input */
		      long	gainbit,	/* configuration input */
		      long	*ga_idx );	/* output */

void nec_enh_gain_dec(
		  long	vu_flag,	/* input */
		  float	qxnorm,		/* input */
		  float	alpha[],	/* input */
		  float	comb_exc[],	/* input */
		  long	len_sf,		/* configuration input */
		  long	ga_idx,		/* input */
		  long	lpc_order,	/* configuration input */
		  long	gainbit,	/* configuration input */
		  float	*g_ac,		/* output */
		  float	*g_ec );	/* output */

void nec_mk_target(
		   float InputSignal[],
		   float target[],
		   long  sbfrm_size,
		   long  lpc_order,
		   float int_lpc_coefficients[],
		   float Wden_coeff[],
		   float Wnum_coeff[],
		   float mem_past_in[],
		   float mem_past_win[],
		   float mem_past_syn[],
		   float mem_past_wsyn[]
		   );

#ifdef __cplusplus
}
#endif

#endif
