/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
de to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#ifndef	_port_h_
#define	_port_h_

#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#include "all.h"


// wmay - removed static int ch_index(MC_Info *mip, int cpe, int tag);
int check_mc_info(faacDecHandle hDecoder, MC_Info *mip, int new_config);
int chn_config(faacDecHandle hDecoder, int id, int tag,
			   int common_window, MC_Info *mip);
// wmay - removed static void clr_tns(Info *info, TNS_frame_info *tns_frame_info);
__inline void decode_huff_cw(faacDecHandle hDecoder, Huffman *h, int *qp, Hcb *hcb);
__inline int decode_huff_cw_scl(faacDecHandle hDecoder, Huffscl *h);
// wmay - removed static int enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip);
int enter_mc_info(faacDecHandle hDecoder, MC_Info *mip, ProgConfig *pcp);
int get_adif_header(faacDecHandle hDecoder);
int get_adts_header(faacDecHandle hDecoder);
#if 0
 wmay - removed
static int get_ics_info(faacDecHandle hDecoder, byte *win, byte *wshape, byte *group,
				 byte *max_sfb, int *lpflag, int *prstflag,
				 NOK_LT_PRED_STATUS *nok_ltp_status,
				 NOK_LT_PRED_STATUS *nok_ltp_status_right, int stereo_flag);
#endif
int get_prog_config(faacDecHandle hDecoder, ProgConfig *p);
// wmay removed static void get_sign_bits(faacDecHandle hDecoder, int *q, int n);
// wmay removed static int get_tns(faacDecHandle hDecoder, Info *info, TNS_frame_info *tns_frame_info);
// wmay removed static int getescape(faacDecHandle hDecoder, int q);
void getfill(faacDecHandle hDecoder, byte *data);
int getdata(faacDecHandle hDecoder, int *tag, int *dt_cnt, byte *data_bytes);
// wmay - removed static int extension_payload(faacDecHandle hDecoder, int cnt, byte *data);
// wmay - removed static void getgroup(faacDecHandle hDecoder, Info *info, byte *group);
// wmay - removed static int getics(faacDecHandle hDecoder, Info *info, int common_window, byte *win, byte *wshape, byte *group,
//		   byte *max_sfb, int *lpflag, int *prstflag, byte *cb_map, Float *coef,
//		   int *global_gain, int *factors, NOK_LT_PRED_STATUS *nok_ltp_status,
//		   TNS_frame_info *tns);
// wmay - removed static int getmask(faacDecHandle hDecoder, Info *info, byte *group, byte max_sfb, byte *mask);
void huffbookinit(faacDecHandle hDecoder);
// wmay - removed static int huffcb(faacDecHandle hDecoder, byte *sect, int *sectbits, int tot_sfb,
//		   int sfb_per_sbk, byte max_sfb);
int huffdecode(faacDecHandle hDecoder, int id, MC_Info *mip, byte *win,
			   Wnd_Shape *wshape, byte **cb_map, int **factors,
			   byte **group, byte *hasmask, byte **mask, byte *max_sfb,
			   int **lpflag, int **prstflag, NOK_LT_PRED_STATUS **nok_ltp,
			   TNS_frame_info **tns, Float **coef);
// wmay -removed static int hufffac(faacDecHandle hDecoder, Info *info, byte *group, int nsect, byte *sect,
//		int global_gain, int *factors);
// wmay - removed static int huffspec(faacDecHandle hDecoder, Info *info, int nsect,
//			 byte *sect, int *factors, Float *coef);
void hufftab(Hcb *hcb, Huffman *hcw, int dim, int signed_cb);
void infoinit(faacDecHandle hDecoder, SR_Info *sip);
void intensity(MC_Info *mip, Info *info, int widx, int ch, byte *group, byte *cb_map,
			   int *factors, int *lpflag, Float *coef[Chans]);
void map_mask(Info *info, byte *group, byte *mask, byte *cb_map);
void reset_mc_info(faacDecHandle hDecoder, MC_Info *mip);
void reset_bits(void);
void synt(Info *info, byte *group, byte *mask, Float *right, Float *left);
// wmay - removed static void tns_ar_filter(Float *spec, int size, int inc, Float *lpc, int order);
// wmay - removed static void tns_decode_coef(int order, int coef_res, int *coef, Float *a);
void tns_decode_subblock(faacDecHandle hDecoder, Float *spec, int nbands,
						 int *sfb_top, int islong, TNSinfo *tns_info);
void tns_filter_subblock(faacDecHandle hDecoder, Float *spec, int nbands,
						 int *sfb_top, int islong, TNSinfo *tns_info);
void pns(faacDecHandle hDecoder, MC_Info *mip, Info *info, int widx, int ch, byte *group, byte *cb_map,
		 int *factors, int *lpflag, Float *coef[Chans]);
void freq2time_adapt(faacDecHandle hDecoder,byte wnd, Wnd_Shape *wnd_shape,
					 Float *coef, Float *state, Float *data);
void time2freq_adapt(faacDecHandle hDecoder,WINDOW_TYPE blockType, Wnd_Shape *wnd_shape,
					 Float *timeInPtr, Float *ffreqOutPtr);
int predict(faacDecHandle hDecoder, Info* info, int profile, int *lpflag, PRED_STATUS *psp, Float *coef);
void predict_pns_reset(Info* info, PRED_STATUS *psp, byte *cb_map);
void init_pred(faacDecHandle hDecoder, PRED_STATUS **sp_status, int channels);
int predict_reset(faacDecHandle hDecoder, Info* info, int *prstflag, PRED_STATUS **psp,
				  int firstCh, int lastCh, int *last_rstgrp_num);
void reset_pred_state(PRED_STATUS *psp);

#endif	/* _port_h_ */
