/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by
Yoshiaki Oikawa (Sony Corporaion),
Mitsuyuki Hatanaka (Sony Corporation),
in the course of development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7,
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
code to a third party and to inhibit third party from using the code for non
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works."
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
/*
 * $Id: port.h,v 1.5 2002/01/11 00:55:17 wmaycisco Exp $
 */

#ifndef _port_h_
#define _port_h_

#ifndef min
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#include "all.h"


int check_mc_info(faacDecHandle hDecoder, MC_Info *mip, int new_config);
int chn_config(faacDecHandle hDecoder, int id, int tag,
               int common_window, MC_Info *mip);
__inline void decode_huff_cw(faacDecHandle hDecoder, Huffman *h, int *qp, Hcb *hcb);
__inline int decode_huff_cw_scl(faacDecHandle hDecoder, Huffscl *h);
int enter_mc_info(faacDecHandle hDecoder, MC_Info *mip, ProgConfig *pcp);
int get_adif_header(faacDecHandle hDecoder);
int get_adts_header(faacDecHandle hDecoder);
int get_prog_config(faacDecHandle hDecoder, ProgConfig *p);
void getfill(faacDecHandle hDecoder, byte *data);
int getdata(faacDecHandle hDecoder, int *tag, int *dt_cnt, byte *data_bytes);
void huffbookinit(faacDecHandle hDecoder);
int huffdecode(faacDecHandle hDecoder, int id, MC_Info *mip, byte *win,
               Wnd_Shape *wshape, byte **cb_map, int **factors,
               byte **group, byte *hasmask, byte **mask, byte *max_sfb,
               int **lpflag, int **prstflag, NOK_LT_PRED_STATUS **nok_ltp,
               TNS_frame_info **tns, Float **coef);
void hufftab(Hcb *hcb, Huffman *hcw, int dim, int signed_cb);
void infoinit(faacDecHandle hDecoder, SR_Info *sip);
void intensity(MC_Info *mip, Info *info, int widx, int ch, byte *group, byte *cb_map,
               int *factors, int *lpflag, Float *coef[Chans]);
void map_mask(Info *info, byte *group, byte *mask, byte *cb_map);
void reset_mc_info(faacDecHandle hDecoder, MC_Info *mip);
void reset_bits(void);
void synt(Info *info, byte *group, byte *mask, Float *right, Float *left);
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
int get_sr_index(unsigned int sampleRate);

#endif  /* _port_h_ */
