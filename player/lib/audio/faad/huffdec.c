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
/*
 * $Id: huffdec.c,v 1.7 2003/02/18 18:51:31 wmaycisco Exp $
 */

#include "all.h"
#include "port.h"
#include "bits.h"
#include "util.h"


// wmay - add statics
static int extension_payload(faacDecHandle hDecoder, int cnt, byte *data);
static int getescape(faacDecHandle hDecoder, int q);
static void getgroup(faacDecHandle hDecoder, Info *info, byte *group);
static int getics(faacDecHandle hDecoder, Info *info, int common_window, byte *win, byte *wshape,
   byte *group, byte *max_sfb, int *lpflag, int *prstflag,
   byte *cb_map, Float *coef, int *global_gain,
   int *factors,
   NOK_LT_PRED_STATUS *nok_ltp_status,
  TNS_frame_info *tns);
static int get_ics_info(faacDecHandle hDecoder, byte *win, byte *wshape,
byte *group, byte *max_sfb,
int *lpflag, int *prstflag,
NOK_LT_PRED_STATUS *nok_ltp_status,
NOK_LT_PRED_STATUS *nok_ltp_status_right,
int stereo_flag);
static int getmask(faacDecHandle hDecoder, Info *info, byte *group, byte max_sfb, byte *mask);
static void get_sign_bits(faacDecHandle hDecoder, int *q, int n);
static int huffcb(faacDecHandle hDecoder, byte *sect, int *sectbits,
  int tot_sfb, int sfb_per_sbk, byte max_sfb);
static int hufffac(faacDecHandle hDecoder, Info *info, byte *group, int nsect, byte *sect,
   int global_gain, int *factors);
static int huffspec(faacDecHandle hDecoder, Info *info, int nsect, byte *sect,
    int *factors, Float *coef);

void getfill(faacDecHandle hDecoder, byte *data)
{
    int cnt;

    if ((cnt = faad_getbits(&hDecoder->ld, LEN_F_CNT)) == (1<<LEN_F_CNT)-1)
        cnt +=  faad_getbits(&hDecoder->ld, LEN_F_ESC) - 1;

    while (cnt > 0)
        cnt -= extension_payload(hDecoder, cnt, data);
}

int getdata(faacDecHandle hDecoder, int *tag, int *dt_cnt, byte *data_bytes)
{
    int i, align_flag, cnt;

    *tag = faad_getbits(&hDecoder->ld, LEN_TAG);
    align_flag = faad_getbits(&hDecoder->ld, LEN_D_ALIGN);

    if ((cnt = faad_getbits(&hDecoder->ld, LEN_D_CNT)) == (1<<LEN_D_CNT)-1)
        cnt +=  faad_getbits(&hDecoder->ld, LEN_D_ESC);

    *dt_cnt = cnt;

    if (align_flag)
        faad_byte_align(&hDecoder->ld);

    for (i=0; i<cnt; i++)
        data_bytes[i] = faad_getbits(&hDecoder->ld, LEN_BYTE);

    return 0;
}

static int extension_payload(faacDecHandle hDecoder, int cnt, byte *data)
{
    int type, i;

    /* fill bytes should not emulate any EX types below! */
    type = faad_getbits(&hDecoder->ld, LEN_EX_TYPE);

    switch(type) {
    case EX_FILL_DATA:
        faad_getbits(&hDecoder->ld, LEN_NIBBLE);
        for (i=0; i<cnt-1; i++)
            data[i] = faad_getbits(&hDecoder->ld, LEN_BYTE);
        return cnt;
    default:
        faad_getbits(&hDecoder->ld, LEN_NIBBLE);
        for (i=0; i<cnt-1; i++)
            faad_getbits(&hDecoder->ld, LEN_BYTE);
        return cnt;
    }
}

/*
 * read and decode the data for the next 1024 output samples
 * return -1 if there was an error
 */
int huffdecode(faacDecHandle hDecoder, int id, MC_Info *mip, byte *win,
               Wnd_Shape *wshape,
               byte **cb_map, int **factors,
               byte **group, byte *hasmask, byte **mask, byte *max_sfb,
               int **lpflag, int **prstflag,
               NOK_LT_PRED_STATUS **nok_ltp_status,
               TNS_frame_info **tns, Float **coef)
{
    int i, tag, common_window, ch, widx, first=0, last=0;

    int global_gain;  /* not used in this routine */
    Info info;

    tag = faad_getbits(&hDecoder->ld, LEN_TAG);

    switch(id) {
    case ID_SCE:
        common_window = 0;
        break;
    case ID_CPE:
        common_window = faad_get1bit(&hDecoder->ld); /* common_window */
        break;
    default:
        /* CommonWarning("Unknown id"); */
        return(-1);
    }

    if ((ch = chn_config(hDecoder, id, tag, common_window, mip)) < 0)
        return -1;

    switch(id) {
    case ID_SCE:
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
            if (!get_ics_info(hDecoder, &win[widx], &wshape[widx].this_bk, group[widx],
                &max_sfb[widx], lpflag[widx], prstflag[widx],
                nok_ltp_status[widx],nok_ltp_status[mip->ch_info[ch].paired_ch], common_window))
                return -1;

            hasmask[widx] = getmask(hDecoder, hDecoder->winmap[win[widx]], group[widx],
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
        SetMemory(coef[i], 0, LN2*sizeof(Float));

        if(!getics(hDecoder, &info, common_window, &win[widx], &wshape[widx].this_bk,
            group[widx], &max_sfb[widx], lpflag[widx], prstflag[widx],
            cb_map[i], coef[i], &global_gain, factors[i], nok_ltp_status[widx],
            tns[i]))
            return -1;
    }

    return 0;
}

static int get_ics_info(faacDecHandle hDecoder, byte *win, byte *wshape, byte *group, byte *max_sfb,
                 int *lpflag, int *prstflag, NOK_LT_PRED_STATUS *nok_ltp_status,
                 NOK_LT_PRED_STATUS *nok_ltp_status_right, int stereo_flag)
{
    Info *info;
    int i, j;

    int max_pred_sfb = pred_max_bands(hDecoder);

    faad_get1bit(&hDecoder->ld); /* reserved bit */
    *win = (unsigned char)faad_getbits(&hDecoder->ld, LEN_WIN_SEQ);
    *wshape = (unsigned char)faad_get1bit(&hDecoder->ld); /* window shape */
    if ((info = hDecoder->winmap[*win]) == NULL)
        /* CommonExit(1, "bad window code"); */
        return 0;

    /*
     * max scale factor, scale factor grouping and prediction flags
     */
    prstflag[0] = 0;
    if (info->islong) {
        *max_sfb = (unsigned char)faad_getbits(&hDecoder->ld, LEN_MAX_SFBL);
        group[0] = 1;

        if (hDecoder->mc_info.object_type != AACLTP) {
            if ((lpflag[0] = faad_getbits(&hDecoder->ld, LEN_PRED_PRES))) {
                if ((prstflag[0] = faad_getbits(&hDecoder->ld, LEN_PRED_RST))) {
                    for(i=1; i<LEN_PRED_RSTGRP+1; i++)
                        prstflag[i] = faad_getbits(&hDecoder->ld, LEN_PRED_RST);
                }
                j = ( (*max_sfb < max_pred_sfb) ?
                    *max_sfb : max_pred_sfb ) + 1;
                for (i = 1; i < j; i++)
                    lpflag[i] = faad_getbits(&hDecoder->ld, LEN_PRED_ENAB);
                for ( ; i < max_pred_sfb+1; i++)
                    lpflag[i] = 0;
            }
        } else { /* AAC LTP */
            if(faad_get1bit(&hDecoder->ld))
            {
                nok_lt_decode(hDecoder, *max_sfb, nok_ltp_status->sbk_prediction_used,
                    nok_ltp_status->sfb_prediction_used,
                    &nok_ltp_status->weight,
                    nok_ltp_status->delay);

                if(stereo_flag)
                    nok_lt_decode(hDecoder, *max_sfb, nok_ltp_status_right->sbk_prediction_used,
                        nok_ltp_status_right->sfb_prediction_used,
                        &nok_ltp_status_right->weight,
                        nok_ltp_status_right->delay);
            } else {
                nok_ltp_status->sbk_prediction_used[0] = 0;
                if(stereo_flag)
                    nok_ltp_status_right->sbk_prediction_used[0] = 0;
            }
        }
    } else {
        *max_sfb = (unsigned char)faad_getbits(&hDecoder->ld, LEN_MAX_SFBS);
        getgroup(hDecoder, info, group);
        lpflag[0] = 0;
        nok_ltp_status->sbk_prediction_used[0] = 0;
        if(stereo_flag)
            nok_ltp_status_right->sbk_prediction_used[0] = 0;
    }

    return 1;
}

static void deinterleave(int inptr[], int outptr[], int ngroups,
                  int nsubgroups[], int ncells[], int cellsize[])
{
    int i, j, k, l;
    int *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
    int cell_inc, subgroup_inc;

    start_subgroup_ptr = outptr;

    for (i = 0; i < ngroups; i++)
    {
        cell_inc = 0;
        start_inptr = inptr;

        /* Compute the increment size for the subgroup pointer */
        subgroup_inc = 0;
        for (j = 0; j < ncells[i]; j++) {
            subgroup_inc += cellsize[j];
        }

        /* Perform the deinterleaving across all subgroups in a group */
        for (j = 0; j < ncells[i]; j++)
        {
            subgroup_ptr = start_subgroup_ptr;

            for (k = 0; k < nsubgroups[i]; k++) {
                outptr = subgroup_ptr + cell_inc;
                for (l = 0; l < cellsize[j]; l++) {
                    *outptr++ = *inptr++;
                }
                subgroup_ptr += subgroup_inc;
            }
            cell_inc += cellsize[j];
        }
        start_subgroup_ptr += (inptr - start_inptr);
    }
}

static void calc_gsfb_table(Info *info, byte *group)
{
    int group_offset;
    int group_idx;
    int offset;
    int *group_offset_p;
    int sfb,len;

    /* first calc the group length*/
    if (info->islong){
        return;
    } else {
        group_offset = 0;
        group_idx =0;
        do  {
            info->group_len[group_idx] = group[group_idx]-group_offset;
            group_offset=group[group_idx];
            group_idx++;
        } while (group_offset<8);

        info->num_groups=group_idx;
        group_offset_p = info->bk_sfb_top;
        offset=0;

        for (group_idx=0;group_idx<info->num_groups;group_idx++){
            len = info->group_len[group_idx];
            for (sfb=0;sfb<info->sfb_per_sbk[group_idx];sfb++){
                offset += info->sfb_width_128[sfb] * len;
                *group_offset_p++ = offset;
            }
        }
    }
}

static void getgroup(faacDecHandle hDecoder, Info *info, byte *group)
{
    int i, j, first_short;

    first_short=1;
    for (i = 0; i < info->nsbk; i++) {
        if (info->bins_per_sbk[i] > SN2) {
            /* non-short windows are always their own group */
            *group++ = i+1;
        } else {
            /* only short-window sequences are grouped! */
            if (first_short) {
                /* first short window is always a new group */
                first_short=0;
            } else {
                if((j = faad_get1bit(&hDecoder->ld)) == 0) {
                    *group++ = i;
                }
            }
        }
    }
    *group = i;
}

/*
 * read a synthesis mask
 *  uses EXTENDED_MS_MASK
 *  and grouped mask
 */
static int getmask(faacDecHandle hDecoder, Info *info, byte *group, byte max_sfb, byte *mask)
{
    int b, i, mp;

    mp = faad_getbits(&hDecoder->ld, LEN_MASK_PRES);

    /* special EXTENDED_MS_MASK cases */
    if(mp == 0) { /* no ms at all */
        return 0;
    }
    if(mp == 2) {/* MS for whole spectrum on, mask bits set to 1 */
        for(b = 0; b < info->nsbk; b = *group++)
            for(i = 0; i < info->sfb_per_sbk[b]; i ++)
                *mask++ = 1;
            return 2;
    }

    /* otherwise get mask */
    for(b = 0; b < info->nsbk; b = *group++){
        for(i = 0; i < max_sfb; i ++) {
            *mask = (byte)faad_get1bit(&hDecoder->ld);
            mask++;
        }
        for( ; i < info->sfb_per_sbk[b]; i++){
            *mask = 0;
            mask++;
        }
    }
    return 1;
}

static void clr_tns( Info *info, TNS_frame_info *tns_frame_info )
{
    int s;

    tns_frame_info->n_subblocks = info->nsbk;
    for (s=0; s<tns_frame_info->n_subblocks; s++)
        tns_frame_info->info[s].n_filt = 0;
}

static int get_tns(faacDecHandle hDecoder, Info *info, TNS_frame_info *tns_frame_info)
{
    int f, t, top, res, res2, compress;
    int short_flag, s;
    int *sp, tmp, s_mask, n_mask;
    TNSfilt *tns_filt;
    TNSinfo *tns_info;
    static int sgn_mask[] = { 0x2, 0x4, 0x8 };
    static int neg_mask[] = { 0xfffc, 0xfff8, 0xfff0 };


    short_flag = (!info->islong);
    tns_frame_info->n_subblocks = info->nsbk;

    for (s=0; s<tns_frame_info->n_subblocks; s++) {
        tns_info = &tns_frame_info->info[s];

        if (!(tns_info->n_filt = faad_getbits(&hDecoder->ld, short_flag ? 1 : 2)))
            continue;

        tns_info -> coef_res = res = faad_get1bit(&hDecoder->ld) + 3;
        top = info->sfb_per_sbk[s];
        tns_filt = &tns_info->filt[ 0 ];
        for (f=tns_info->n_filt; f>0; f--)  {
            tns_filt->stop_band = top;
            top = tns_filt->start_band = top - faad_getbits(&hDecoder->ld, short_flag ? 4 : 6);
            tns_filt->order = faad_getbits(&hDecoder->ld, short_flag ? 3 : 5);

            if (tns_filt->order)  {
                tns_filt->direction = faad_get1bit(&hDecoder->ld);
                compress = faad_get1bit(&hDecoder->ld);

                res2 = res - compress;
                s_mask = sgn_mask[ res2 - 2 ];
                n_mask = neg_mask[ res2 - 2 ];

                sp = tns_filt->coef;
                for (t=tns_filt->order; t>0; t--)  {
                    tmp = (short)faad_getbits(&hDecoder->ld, res2);
                    *sp++ = (tmp & s_mask) ? (short)(tmp | n_mask) : (short)tmp;
                }
            }
            tns_filt++;
        }
    }   /* subblock loop */

    return 1;
}

static void get_pulse_nc(faacDecHandle hDecoder, struct Pulse_Info *pulse_info)
{
    int i;

    pulse_info->number_pulse = faad_getbits(&hDecoder->ld, LEN_NPULSE);
    pulse_info->pulse_start_sfb = faad_getbits(&hDecoder->ld, LEN_PULSE_ST_SFB);

    for(i = 0; i < pulse_info->number_pulse + 1; i++) {
        pulse_info->pulse_offset[i] = faad_getbits(&hDecoder->ld, LEN_POFF);
        pulse_info->pulse_amp[i] = faad_getbits(&hDecoder->ld, LEN_PAMP);
    }
}

static void pulse_nc(faacDecHandle hDecoder, int *coef, struct Pulse_Info *pulse_info)
{
    int i, k;

    k = hDecoder->only_long_info.sbk_sfb_top[0][pulse_info->pulse_start_sfb];

    for(i = 0; i <= pulse_info->number_pulse; i++) {
        k += pulse_info->pulse_offset[i];
        if (coef[k]>0)
            coef[k] += pulse_info->pulse_amp[i];
        else
            coef[k] -= pulse_info->pulse_amp[i];
    }
}

static int getics(faacDecHandle hDecoder, Info *info, int common_window, byte *win, byte *wshape,
           byte *group, byte *max_sfb, int *lpflag, int *prstflag,
           byte *cb_map, Float *coef, int *global_gain,
           int *factors,
           NOK_LT_PRED_STATUS *nok_ltp_status,
           TNS_frame_info *tns)
{
    int nsect, i, cb, top, bot, tot_sfb;
    byte sect[ 2*(MAXBANDS+1) ];

    memset(sect, 0, sizeof(sect));

    /*
    * global gain
    */
    *global_gain = (short)faad_getbits(&hDecoder->ld, LEN_SCL_PCM);

    if (!common_window) {
        if (!get_ics_info(hDecoder, win, wshape, group, max_sfb, lpflag, prstflag,
            nok_ltp_status, NULL, 0))
            return 0;
    }

    CopyMemory(info, hDecoder->winmap[*win], sizeof(Info));

    /* calculate total number of sfb for this grouping */
    if (*max_sfb == 0) {
        tot_sfb = 0;
    }
    else {
        i=0;
        tot_sfb = info->sfb_per_sbk[0];
        while (group[i++] < info->nsbk) {
            tot_sfb += info->sfb_per_sbk[0];
        }
    }

    /*
    * section data
    */
    nsect = huffcb(hDecoder, sect, info->sectbits, tot_sfb, info->sfb_per_sbk[0], *max_sfb);
    if(nsect==0 && *max_sfb>0)
        return 0;

        /* generate "linear" description from section info
        * stored as codebook for each scalefactor band and group
    */
    if (nsect) {
        bot = 0;
        for (i=0; i<nsect; i++) {
            cb = sect[2*i];
            top = sect[2*i + 1];
            for (; bot<top; bot++)
                *cb_map++ = cb;
            bot = top;
        }
    }  else {
        for (i=0; i<MAXBANDS; i++)
            cb_map[i] = 0;
    }

    /* calculate band offsets
    * (because of grouping and interleaving this cannot be
    * a constant: store it in info.bk_sfb_top)
    */
    calc_gsfb_table(info, group);

    /*
    * scale factor data
    */
    if(!hufffac(hDecoder, info, group, nsect, sect, *global_gain, factors))
        return 0;

    /*
     *  Pulse coding
     */
    if ((hDecoder->pulse_info.pulse_data_present = faad_get1bit(&hDecoder->ld))) { /* pulse data present */
        if (info->islong) {
            get_pulse_nc(hDecoder, &hDecoder->pulse_info);
        } else {
            /* CommonExit(1,"Pulse data not allowed for short blocks"); */
            return 0;
        }
    }

    /*
    * tns data
    */
    if (faad_get1bit(&hDecoder->ld)) { /* tns present */
        get_tns(hDecoder, info, tns);
    }
    else {
        clr_tns(info, tns);
    }

    if (faad_get1bit(&hDecoder->ld)) { /* gain control present */
        /* CommonExit(1, "Gain control not implemented"); */
        return 0;
    }

    return huffspec(hDecoder, info, nsect, sect, factors, coef);
}

/*
 * read the codebook and boundaries
 */
static int huffcb(faacDecHandle hDecoder, byte *sect, int *sectbits,
           int tot_sfb, int sfb_per_sbk, byte max_sfb)
{
    int nsect, n, base, bits, len;

    bits = sectbits[0];
    len = (1 << bits) - 1;
    nsect = 0;

    for(base = 0; base < tot_sfb && nsect < tot_sfb; ) {
        *sect++ = (byte)faad_getbits(&hDecoder->ld, LEN_CB);

        n = faad_getbits(&hDecoder->ld, bits);
        while(n == len && base < tot_sfb) {
            base += len;
            n = faad_getbits(&hDecoder->ld, bits);
        }
        base += n;
        *sect++ = base;
        nsect++;

        /* insert a zero section for regions above max_sfb for each group */
        if ((sect[-1] % sfb_per_sbk) == max_sfb) {
            base += (sfb_per_sbk - max_sfb);
            *sect++ = 0;
            *sect++ = base;
            nsect++;
        }
    }

    if(base != tot_sfb || nsect > tot_sfb)
        return 0;

    return nsect;
}

/*
 * get scale factors
 */
static int hufffac(faacDecHandle hDecoder, Info *info, byte *group, int nsect, byte *sect,
        int global_gain, int *factors)
{
    Huffscl *hcw;
    int i, b, bb, t, n, sfb, top, fac, is_pos;
    int factor_transmitted[MAXBANDS], *fac_trans;
    int noise_pcm_flag = 1;
    int noise_nrg;

    /* clear array for the case of max_sfb == 0 */
    SetMemory(factor_transmitted, 0, MAXBANDS*sizeof(int));
    SetMemory(factors, 0, MAXBANDS*sizeof(int));

    sfb = 0;
    fac_trans = factor_transmitted;
    for(i = 0; i < nsect; i++){
        top = sect[1];      /* top of section in sfb */
        t = sect[0];        /* codebook for this section */
        sect += 2;
        for(; sfb < top; sfb++) {
            fac_trans[sfb] = t;
        }
    }

    /* scale factors are dpcm relative to global gain
    * intensity positions are dpcm relative to zero
    */
    fac = global_gain;
    is_pos = 0;
    noise_nrg = global_gain - NOISE_OFFSET;

    /* get scale factors */
    hcw = bookscl;
    bb = 0;
    for(b = 0; b < info->nsbk; ){
        n = info->sfb_per_sbk[b];
        b = *group++;
        for(i = 0; i < n; i++){
            switch (fac_trans[i]) {
            case ZERO_HCB:      /* zero book */
                break;
            default:            /* spectral books */
                /* decode scale factor */
                t = decode_huff_cw_scl(hDecoder, hcw);
                fac += t - MIDFAC;    /* 1.5 dB */
                if(fac >= 2*TEXP || fac < 0)
                    return 0;
                factors[i] = fac;
                break;
            case BOOKSCL:       /* invalid books */
                return 0;
            case INTENSITY_HCB:     /* intensity books */
            case INTENSITY_HCB2:
                /* decode intensity position */
                t = decode_huff_cw_scl(hDecoder, hcw);
                is_pos += t - MIDFAC;
                factors[i] = is_pos;
                break;
            case NOISE_HCB:     /* noise books */
                /* decode noise energy */
                if (noise_pcm_flag) {
                    noise_pcm_flag = 0;
                    t = faad_getbits(&hDecoder->ld, NOISE_PCM_BITS) - NOISE_PCM_OFFSET;
                } else
                    t = decode_huff_cw_scl(hDecoder, hcw) - MIDFAC;
                noise_nrg += t;
                factors[i] = noise_nrg;
                break;
            }
        }

        /* expand short block grouping */
        if (!(info->islong)) {
            for(bb++; bb < b; bb++) {
                for (i=0; i<n; i++) {
                    factors[i+n] = factors[i];
                }
                factors += n;
            }
        }
        fac_trans += n;
        factors += n;
    }
    return 1;
}

__inline float esc_iquant(faacDecHandle hDecoder, int q)
{
    if (q > 0) {
        if (q < MAX_IQ_TBL) {
            return((float)hDecoder->iq_exp_tbl[q]);
        } else {
            return((float)pow(q, 4./3.));
        }
    } else {
        q = -q;
        if (q < MAX_IQ_TBL) {
            return((float)(-hDecoder->iq_exp_tbl[q]));
        } else {
            return((float)(-pow(q, 4./3.)));
        }
    }
}

static int huffspec(faacDecHandle hDecoder, Info *info, int nsect, byte *sect,
             int *factors, Float *coef)
{
    Hcb *hcb;
    Huffman *hcw;
    int i, j, k, table, step, stop, bottom, top;
    int *bands, *bandp;
    int *quant, *qp;
    int *tmp_spec;
    int *quantPtr;
    int *tmp_specPtr;

    quant = AllocMemory(LN2*sizeof(int));
    tmp_spec = AllocMemory(LN2*sizeof(int));

    quantPtr = quant;
    tmp_specPtr = tmp_spec;

#ifndef _WIN32
    SetMemory(quant, 0, LN2*sizeof(int));
#endif

    bands = info->bk_sfb_top;
    bottom = 0;
    k = 0;
    bandp = bands;
    for(i = nsect; i; i--) {
        table = sect[0];
        top = sect[1];
        sect += 2;
        if( (table == 0) || (table == NOISE_HCB) ||
            (table == INTENSITY_HCB) || (table == INTENSITY_HCB2) ) {
            bandp = bands+top;
            k = bandp[-1];
            bottom = top;
            continue;
        }
        if(table < BY4BOOKS+1) {
            step = 4;
        } else {
            step = 2;
        }
        hcb = &book[table];
        hcw = hcb->hcw;
        qp = quant+k;
        for(j = bottom; j < top; j++) {
            stop = *bandp++;
            while(k < stop) {
                decode_huff_cw(hDecoder, hcw, qp, hcb);

                if (!hcb->signed_cb)
                    get_sign_bits(hDecoder, qp, step);
                if(table == ESCBOOK){
                    qp[0] = getescape(hDecoder, qp[0]);
                    qp[1] = getescape(hDecoder, qp[1]);
                }
                qp += step;
                k += step;
            }
        }
        bottom = top;
    }

    /* pulse coding reconstruction */
    if ((info->islong) && (hDecoder->pulse_info.pulse_data_present))
        pulse_nc(hDecoder, quant, &hDecoder->pulse_info);

    if (!info->islong) {
        deinterleave (quant,tmp_spec,
            (short)info->num_groups,
            info->group_len,
            info->sfb_per_sbk,
            info->sfb_width_128);

        for (i = LN2/16 - 1; i >= 0; --i)
        {
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
            *quantPtr++ = *tmp_specPtr++; *quantPtr++ = *tmp_specPtr++;
        }
    }

    /* inverse quantization */
    for (i=0; i<info->bins_per_bk; i++) {
        coef[i] = esc_iquant(hDecoder, quant[i]);
    }

    /* rescaling */
    {
        int sbk, nsbk, sfb, nsfb, fac, top;
        Float *fp, scale;

        i = 0;
        fp = coef;
        nsbk = info->nsbk;
        for (sbk=0; sbk<nsbk; sbk++) {
            nsfb = info->sfb_per_sbk[sbk];
            k=0;
            for (sfb=0; sfb<nsfb; sfb++) {
                top = info->sbk_sfb_top[sbk][sfb];
                fac = factors[i++]-SF_OFFSET;

                if (fac >= 0 && fac < TEXP) {
                    scale = hDecoder->exptable[fac];
                }
                else {
                    if (fac == -SF_OFFSET) {
                        scale = 0;
                    }
                    else {
                        scale = (float)pow(2.0,  0.25*fac);
                    }
                }
                for ( ; k<top; k++) {
                    *fp++ *= scale;
                }
            }
        }
    }

    if (quant) FreeMemory(quant);
    if (tmp_spec) FreeMemory(tmp_spec);

    return 1;
}


/*
 * initialize the Hcb structure and sort the Huffman
 * codewords by length, shortest (most probable) first
 */
void hufftab(Hcb *hcb, Huffman *hcw, int dim, int signed_cb)
{
    hcb->dim = dim;
    hcb->signed_cb = signed_cb;
    hcb->hcw = hcw;
}

__inline void decode_huff_cw(faacDecHandle hDecoder, Huffman *h, int *qp, Hcb *hcb)
{
    int i, j;
    unsigned long cw;

    i = h->len;
    cw = faad_getbits_fast(&hDecoder->ld, i);

    while (cw != h->cw)
    {
        h++;
        j = h->len-i;
        if (j!=0) {
            i += j;
            while (j--)
                cw = (cw<<1)|faad_get1bit(&hDecoder->ld);
        }
    }

    if(hcb->dim == 4)
    {
        qp[0] = h->x;
        qp[1] = h->y;
        qp[2] = h->v;
        qp[3] = h->w;
    } else {
        qp[0] = h->x;
        qp[1] = h->y;
    }
}

__inline int decode_huff_cw_scl(faacDecHandle hDecoder, Huffscl *h)
{
    int i, j;
    long cw;

    i = h->len;
    cw = faad_getbits_fast(&hDecoder->ld, i);

    while ((unsigned long)cw != h->cw)
    {
        h++;
        j = h->len-i;
        if (j!=0) {
            i += j;
            while (j--)
                cw = (cw<<1)|faad_get1bit(&hDecoder->ld);
        }
    }

    return h->scl;
}

/* get sign bits */
static void get_sign_bits(faacDecHandle hDecoder, int *q, int n)
{
    int i;

    for(i=0; i < n; i++)
    {
        if(q[i])
        {
            if(faad_get1bit(&hDecoder->ld) & 1)
            {
                /* 1 signals negative, as in 2's complement */
                q[i] = -q[i];
            }
        }
    }
}

static int getescape(faacDecHandle hDecoder, int q)
{
    int i, off, neg;

    if(q < 0){
        if(q != -16)
            return q;
        neg = 1;
    } else{
        if(q != +16)
            return q;
        neg = 0;
    }

    for(i=4;; i++){
        if(faad_get1bit(&hDecoder->ld) == 0)
            break;
    }

    if(i > 16){
        off = faad_getbits(&hDecoder->ld, i-16) << 16;
        off |= faad_getbits(&hDecoder->ld, 16);
    } else
        off = faad_getbits(&hDecoder->ld, i);

    i = off + (1<<i);
    if(neg)
        i = -i;
    return i;
}
