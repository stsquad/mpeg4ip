/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by
Yoshiaki Oikawa (Sony Corporation),
Mitsuyuki Hatanaka (Sony Corporation)
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
#include "faad_all.h"
#include "port.h"

/*
 * read and decode the data for the next 1024 output samples
 * return -1 if there was an error
 */
int huffdecode(int id, MC_Info *mip, byte *win, Wnd_Shape *wshape, 
			   byte **cb_map, short **factors, 
			   byte **group, byte *hasmask, byte **mask, byte *max_sfb, 
			   int **lpflag, int **prstflag,
			   NOK_LT_PRED_STATUS **nok_ltp_status, 
			   TNS_frame_info **tns, Float **coef)
{
    int i, tag, common_window, ch, widx, first=0, last=0;

    short global_gain;  /* not used in this routine */
    Info info;

    tag = getbits(LEN_TAG);

    switch(id) {
	case ID_SCE:
        case ID_LFE:
		common_window = 0;
		break;
	case ID_CPE:
		common_window = getbits(LEN_COM_WIN);
		break;
	default:
		//CommonWarning("Unknown id");
		return(-1);
    }

    if ((ch = chn_config(id, tag, common_window, mip)) < 0)
		return -1;
    
    switch(id) {
	case ID_SCE:
	case ID_LFE:
		widx = mip->ch_info[ch].widx;
		first = ch;
		last = ch;
		hasmask[widx] = 0;
		break;
	case ID_CPE:
		first = ch;
		last = mip->ch_info[ch].paired_ch;
		if (common_window) {
			widx = mip->ch_info[ch].widx;
			get_ics_info(&win[widx], &wshape[widx].this_bk, group[widx],
				&max_sfb[widx], lpflag[widx], prstflag[widx],nok_ltp_status[first]);
			nok_ltp_status[last]->sbk_prediction_used[0] = 0;

			hasmask[widx] = getmask(winmap[win[widx]], group[widx], 
				max_sfb[widx], mask[widx]);
		}
		else { 
			hasmask[mip->ch_info[first].widx] = 0;
			hasmask[mip->ch_info[last].widx] = 0;
		}
		break;
    }

    for (i=first; i<=last; i++) {
		widx = mip->ch_info[i].widx;
		memset(coef[i], 0, LN2*sizeof(*coef[i]));

		if(!getics(&info, common_window, &win[widx], &wshape[widx].this_bk,
			group[widx], &max_sfb[widx], lpflag[widx], prstflag[widx], 
			cb_map[i], coef[i], &global_gain, factors[i],
			nok_ltp_status[i],
			tns[i]
			))
			return -1;
    }
    return 0;
}

void get_ics_info(byte *win, byte *wshape, byte *group,
				  byte *max_sfb, int *lpflag, int *prstflag
				  , NOK_LT_PRED_STATUS *nok_ltp_status
				  )
{
    Info *info;

    getbits(LEN_ICS_RESERV);	    /* reserved bit */
    *win = (byte)getbits(LEN_WIN_SEQ);
    *wshape = (byte)getbits(LEN_WIN_SH);
    if ((info = winmap[*win]) == NULL)
        CommonExit(1,"2027: Illegal window code");
	
	/*
	* max scale factor, scale factor grouping and prediction flags
	*/
    prstflag[0] = 0;
    if (info->islong) {
		*max_sfb = (byte)getbits(LEN_MAX_SFBL);
		group[0] = 1;
		nok_lt_decode((WINDOW_TYPE)*win, *max_sfb,
			nok_ltp_status->sbk_prediction_used,
			nok_ltp_status->sfb_prediction_used,
			&nok_ltp_status->weight,
			nok_ltp_status->delay);
    }
    else {
		*max_sfb = (byte)getbits(LEN_MAX_SFBS);
		getgroup(info, group);
		lpflag[0] = 0;
		nok_ltp_status->sbk_prediction_used[0] = 0;
    }
}

