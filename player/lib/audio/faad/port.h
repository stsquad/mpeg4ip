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

#ifndef	_port_h_
#define	_port_h_

#ifdef __unix__
#define min(a,b) ( (a) < (b) ? (a) : (b) )
#endif

int		byte_align(void);
int		ch_index(MC_Info *mip, int cpe, int tag);
void		check_mc_info(MC_Info *mip, int new_config);
int		chn_config(int id, int tag, int common_window, MC_Info *mip);
void		clr_tns( Info *info, TNS_frame_info *tns_frame_info );
#if (CChans > 0)
void		coupling(Info *info, MC_Info *mip, Float **coef, Float **cc_coef, Float *cc_gain[CChans][Chans], int ch, int cc_dom, int cc_dep);
#endif
int		decode_huff_cw(Huffman *h);
int		enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip);
int		enter_mc_info(MC_Info *mip, ProgConfig *pcp);
Float		esc_iquant(int q);
int		get_adif_header(void);
void	get_ics_info(byte *win, byte *wshape, byte *group,byte *max_sfb, int *lpflag, int *prstflag
				, NOK_LT_PRED_STATUS *nok_ltp_status);
int		get_prog_config(ProgConfig *p);
void		get_sign_bits(int *q, int n);
int		get_tns( Info *info, TNS_frame_info *tns_frame_info );
#ifdef _MSC_VER
extern
#endif
// getshort eliminated
//long getshort(void);
__inline long getbits(int n);
#if (CChans > 0)
int		getcc(MC_Info *mip, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float **cc_coef,Float *cc_gain[CChans][Chans]);
#endif
int		getescape(int q);
void		getfill(void);
void		getgroup(Info *info, byte *group);
int		getics(Info *info, int common_window, byte *win, byte *wshape, byte *group, byte *max_sfb, int *lpflag, int *prstflag, byte *cb_map, Float *coef, short *global_gain     , short *factors,
			NOK_LT_PRED_STATUS *nok_ltp_status, TNS_frame_info *tns);
int		getmask(Info *info, byte *group, byte max_sfb, byte *mask);
long		getshort(void);
void		huffbookinit(void);
int		huffcb(byte *sect, int *sectbits, int tot_sfb, int sfb_per_sbk, byte max_sfb);
int		huffdecode(int id, MC_Info *mip, byte *win, Wnd_Shape *wshape, byte **cb_map, short **factors,  byte **group, byte *hasmask, byte **mask, byte *max_sfb, int **lpflag, int **prstflag,
NOK_LT_PRED_STATUS **nok_ltp,
TNS_frame_info **tns, Float **coef);
int		hufffac(Info *info, byte *group, int nsect, byte *sect,short global_gain, short *factors);
int		huffspec(Info *info, int nsect, byte *sect, short *factors, Float *coef);
void		hufftab(Hcb *hcb, Huffman *hcw, int dim, int lav, int signed_cb);
void		infoinit(SR_Info *sip);
void		do_init(void);
void		init_cc(void);
int		initio(char *filename);
#if (CChans > 0)
void		ind_coupling(MC_Info *mip, byte *wnd, Wnd_Shape *wnd_shape, byte *cc_wnd, Wnd_Shape *cc_wnd_shape, Float *cc_coef[CChans], Float *cc_state[CChans]);
#endif
void		intensity(MC_Info *mip, Info *info, int widx, int ch, byte *group, byte *cb_map, short *factors, int *lpflag, Float *coef[Chans]);
int		max_dep_cc(int nch);
int		max_indep_cc(int nch);
int		max_lfe_chn(int nch);
void            map_mask(Info *info, byte *group, byte *mask, byte *cb_map);
void		predinit(void);
void		reset_mc_info(MC_Info *mip);
void		reset_bits(void);
void		startblock(void);
void		synt(Info *info, byte *group, byte *mask, Float *right, Float *left);
void		tns_ar_filter( Float *spec, int size, int inc, Float *lpc, int order );
void		tns_decode_coef( int order, int coef_res, short *coef, Float *a );
void		tns_decode_subblock(Float *spec, int nbands, short *sfb_top, int islong, TNSinfo *tns_info);
int		tns_max_bands(int win);
int		tns_max_order(int islong);
void		unpack_idx(int *qp, int idx, Hcb *hcb);
void            pns(MC_Info *mip, Info *info, int widx, int ch, byte *group, byte *cb_map, short *factors, int *lpflag, Float *coef[Chans] );

#endif	/* _port_h_ */
