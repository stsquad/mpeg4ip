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
code to a third party and to inhibit third party from using the code for non
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works."
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
/*
 * $Id: config.c,v 1.11 2003/04/09 00:44:38 wmaycisco Exp $
 */

#include "all.h"
#include "bits.h"
#include "util.h"

static int ch_index(MC_Info *mip, int cpe, int tag);
static int enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip);

/*
 * adts_header
 */
int get_adts_header(faacDecHandle hDecoder)
{
    int sync = 0;
    ADTS_Header *p = &hDecoder->adts_header;

    faad_byte_align(&hDecoder->ld);

    sync = faad_showbits(&hDecoder->ld, 12);
    while (sync != 4096 - 1) {
      faad_flushbits(&hDecoder->ld, 8);
      if (faad_bits_done(&hDecoder->ld) != 0) {
	return -1;
      }
      sync = faad_showbits(&hDecoder->ld, 12);
    }
    faad_flushbits(&hDecoder->ld, 12);

    if (hDecoder->frameNum) {
        faad_getbits(&hDecoder->ld, 16);
#if 0
        if (p->fixed.ID == 0) /* MPEG2 AAC doesn't have this */
            faad_getbits(&hDecoder->ld, 2);
#endif
    } else {
        /* Syncword found, proceed to read in the fixed ADTS header */
        p->fixed.ID = faad_get1bit(&hDecoder->ld); /* 0 -> MPEG4, 1 -> MPEG2 */
        hDecoder->isMpeg4 = !p->fixed.ID;
        p->fixed.layer = faad_getbits(&hDecoder->ld, 2);
        p->fixed.protection_absent = faad_get1bit(&hDecoder->ld);
        hDecoder->mc_info.object_type = p->fixed.object_type = faad_getbits(&hDecoder->ld, 2);
        hDecoder->mc_info.sampling_rate_idx = p->fixed.sampling_rate_idx = faad_getbits(&hDecoder->ld, 4);
        p->fixed.private_bit = faad_get1bit(&hDecoder->ld);
        p->fixed.channel_configuration = faad_getbits(&hDecoder->ld, 3);
        p->fixed.original_copy = faad_get1bit(&hDecoder->ld);
        p->fixed.home = faad_get1bit(&hDecoder->ld);
#if 0
        if (p->fixed.ID == 0) /* MPEG2 AAC doesn't have this */
            p->fixed.emphasis = faad_getbits(&hDecoder->ld, 2);
#endif
    }

    /* ...and the variable ADTS header */
    p->variable.copy_id_bit = faad_get1bit(&hDecoder->ld);
    p->variable.copy_id_start = faad_get1bit(&hDecoder->ld);
    p->variable.frame_length = faad_getbits(&hDecoder->ld, 13);
    p->variable.buffer_fullness = faad_getbits(&hDecoder->ld, 11);
    p->variable.raw_blocks = faad_getbits(&hDecoder->ld, 2);

    /* ADTS error check (ignored) */
    if(!p->fixed.protection_absent)
        faad_getbits(&hDecoder->ld, 16);

    return 0;
}

/*
 * adif_header
 */
int get_adif_header(faacDecHandle hDecoder)
{
    int i, n, tag, select_status;
    ProgConfig tmp_config;
    ADIF_Header *p = &hDecoder->adif_header;

    /* adif header */
    for (i=0; i<LEN_ADIF_ID; i++)
        p->adif_id[i] = (char)faad_getbits(&hDecoder->ld, LEN_BYTE);
    p->adif_id[i] = 0;      /* null terminated string */
    /* copyright string */
    if ((p->copy_id_present = faad_getbits(&hDecoder->ld, LEN_COPYRT_PRES)) == 1) {
        for (i=0; i<LEN_COPYRT_ID; i++)
            p->copy_id[i] = (char)faad_getbits(&hDecoder->ld, LEN_BYTE);
        p->copy_id[i] = 0;  /* null terminated string */
    }
    p->original_copy = faad_getbits(&hDecoder->ld, LEN_ORIG);
    p->home = faad_getbits(&hDecoder->ld, LEN_HOME);
    p->bitstream_type = faad_getbits(&hDecoder->ld, LEN_BS_TYPE);
    p->bitrate = faad_getbits(&hDecoder->ld, LEN_BIT_RATE);

    /* program config elements */
    select_status = -1;
    n = faad_getbits(&hDecoder->ld, LEN_NUM_PCE) + 1;
    for (i=0; i<n; i++) {
        tmp_config.buffer_fullness =
            (p->bitstream_type == 0) ? faad_getbits(&hDecoder->ld, LEN_ADIF_BF) : 0;
        tag = get_prog_config(hDecoder, &tmp_config);
        if (hDecoder->current_program < 0)
            hDecoder->current_program = tag;        /* default is first prog */
        if (hDecoder->current_program == tag) {
            CopyMemory(&hDecoder->prog_config, &tmp_config, sizeof(hDecoder->prog_config));
            select_status = 1;
        }
    }

    return select_status;
}


/*
 * program configuration element
 */
static void get_ele_list(faacDecHandle hDecoder, EleList *p, int enable_cpe)
{
    int i, j;
    for (i=0, j=p->num_ele; i<j; i++) {
        if (enable_cpe)
            p->ele_is_cpe[i] = faad_getbits(&hDecoder->ld, LEN_ELE_IS_CPE);
        else
            p->ele_is_cpe[i] = 0; /* sdb */
        p->ele_tag[i] = faad_getbits(&hDecoder->ld, LEN_TAG);
    }
}

int get_prog_config(faacDecHandle hDecoder, ProgConfig *p)
{
    int i, j, tag;

    tag = faad_getbits(&hDecoder->ld, LEN_TAG);

    p->object_type = faad_getbits(&hDecoder->ld, LEN_OBJECTTYPE);
    p->sampling_rate_idx = faad_getbits(&hDecoder->ld, LEN_SAMP_IDX);
    p->front.num_ele = faad_getbits(&hDecoder->ld, LEN_NUM_ELE);
    p->side.num_ele = faad_getbits(&hDecoder->ld, LEN_NUM_ELE);
    p->back.num_ele = faad_getbits(&hDecoder->ld, LEN_NUM_ELE);
    p->lfe.num_ele = faad_getbits(&hDecoder->ld, LEN_NUM_LFE);
    p->data.num_ele = faad_getbits(&hDecoder->ld, LEN_NUM_DAT);
    p->coupling.num_ele = faad_getbits(&hDecoder->ld, LEN_NUM_CCE);
    if ((p->mono_mix.present = faad_getbits(&hDecoder->ld, LEN_MIX_PRES)) == 1)
        p->mono_mix.ele_tag = faad_getbits(&hDecoder->ld, LEN_TAG);
    if ((p->stereo_mix.present = faad_getbits(&hDecoder->ld, LEN_MIX_PRES)) == 1)
        p->stereo_mix.ele_tag = faad_getbits(&hDecoder->ld, LEN_TAG);
    if ((p->matrix_mix.present = faad_getbits(&hDecoder->ld, LEN_MIX_PRES)) == 1) {
        p->matrix_mix.ele_tag = faad_getbits(&hDecoder->ld, LEN_MMIX_IDX);
        p->matrix_mix.pseudo_enab = faad_getbits(&hDecoder->ld, LEN_PSUR_ENAB);
    }
    get_ele_list(hDecoder, &p->front, 1);
    get_ele_list(hDecoder, &p->side, 1);
    get_ele_list(hDecoder, &p->back, 1);
    get_ele_list(hDecoder, &p->lfe, 0);
    get_ele_list(hDecoder, &p->data, 0);
    get_ele_list(hDecoder, &p->coupling, 1);

    /*
     * if this is a MPEG4 file and the PCE is inside a raw_data_block()
     * this should be aligned to beginning of raw_data_block() boundary,
     * not byte boundary (FIXME!!!)
     */
    faad_byte_align(&hDecoder->ld);

    j = faad_getbits(&hDecoder->ld, LEN_COMMENT_BYTES);
    for (i=0; i<j; i++)
        p->comments[i] = (char)faad_getbits(&hDecoder->ld, LEN_BYTE);
    p->comments[i] = 0; /* null terminator for string */

    /* activate new program configuration if appropriate */
    if (hDecoder->current_program < 0)
        hDecoder->current_program = tag;        /* always select new program */
    if (tag == hDecoder->current_program) {
        /* enter configuration into MC_Info structure */
        if ((hDecoder->pceChannels = enter_mc_info(hDecoder, &hDecoder->mc_info, p)) < 0)
            return -1;
        /* inhibit default configuration */
        hDecoder->default_config = 0;
    }

    return tag;
}

/* enter program configuration into MC_Info structure
 *  only configures for channels specified in all.h
 */
int enter_mc_info(faacDecHandle hDecoder, MC_Info *mip, ProgConfig *pcp)
{
    int i, j, cpe, tag, cw;
    EleList *elp;
    MIXdown *mxp;
    int channels = 0;

    /* reset channel counts */
    mip->nch = 0;
    mip->nfch = 0;
    mip->nfsce = 0;
    mip->nsch = 0;
    mip->nbch = 0;
    mip->nlch = 0;
    mip->ncch = 0;

    /*  object type and sampling rate
     *  re-configure if new sampling rate
     */
    mip->object_type = pcp->object_type;
    if (mip->sampling_rate_idx != pcp->sampling_rate_idx) {
        mip->sampling_rate_idx = pcp->sampling_rate_idx;
        infoinit(hDecoder, &samp_rate_info[mip->sampling_rate_idx]);
    }

    cw = 0;             /* changed later */

    /* front elements, center out */
    elp = &pcp->front;
    /* count number of leading SCE's */
    for (i=0, j=elp->num_ele; i<j; i++) {
        if (elp->ele_is_cpe[i])
            break;
        mip->nfsce++;
    }
    for (i=0, j=elp->num_ele; i<j; i++) {
        cpe = elp->ele_is_cpe[i];
        tag = elp->ele_tag[i];
        if (enter_chn(cpe, tag, 'f', cw, mip) < 0)
            return(-1);
        if (cpe) channels+=2;
        else channels++;
    }

    /* side elements, left to right then front to back */
    elp = &pcp->side;
    for (i=0, j=elp->num_ele; i<j; i++) {
        cpe = elp->ele_is_cpe[i];
        tag = elp->ele_tag[i];
        if (enter_chn(cpe, tag, 's', cw, mip) < 0)
            return(-1);
        if (cpe) channels+=2;
        else channels++;
    }

    /* back elements, outside to center */
    elp = &pcp->back;
    for (i=0, j=elp->num_ele; i<j; i++) {
        cpe = elp->ele_is_cpe[i];
        tag = elp->ele_tag[i];
        if (enter_chn(cpe, tag, 'b', cw, mip) < 0)
            return(-1);
        if (cpe) channels+=2;
        else channels++;
    }

    /* lfe elements */
    elp = &pcp->lfe;
    for (i=0, j=elp->num_ele; i<j; i++) {
        cpe = elp->ele_is_cpe[i];
        tag = elp->ele_tag[i];
        if (enter_chn(cpe, tag, 'l', cw, mip) < 0)
            return(-1);
        if (cpe) channels+=2;
        else channels++;
    }

    /* coupling channel elements */
    elp = &pcp->coupling;
    for (i=0, j=elp->num_ele; i<j; i++)
        mip->cch_tag[i] = elp->ele_tag[i];
    mip->ncch = j;
    channels += j;

    /* mono mixdown elements */
    mxp = &pcp->mono_mix;
    if (mxp->present) {
/*      CommonWarning("Unanticipated mono mixdown channel"); */
        return(-1);
    }

    /* stereo mixdown elements */
    mxp = &pcp->stereo_mix;
    if (mxp->present) {
/*      CommonWarning("Unanticipated stereo mixdown channel"); */
        return(-1);
    }

    /* matrix mixdown elements */
    mxp = &pcp->matrix_mix;
    if (mxp->present) {
/*      CommonWarning("Unanticipated matrix mixdown channel"); */
        return(-1);
    }

    /* save to check future consistency */
    if (!check_mc_info(hDecoder, mip, 1))
        return -1;

    return channels;
}

/* translate prog config or default config
 * into to multi-channel info structure
 *  returns index of channel in MC_Info
 */
static int enter_chn(int cpe, int tag, char position, int common_window, MC_Info *mip)
{
    int nch, cidx=0;
    Ch_Info *cip;

    nch = (cpe == 1) ? 2 : 1;

    switch (position) {
    /* use configuration already in MC_Info, but now common_window is valid */
    case 0:
    cidx = ch_index(mip, cpe, tag);
    if (common_window) {
        mip->ch_info[cidx].widx = cidx; /* window info is left */
        mip->ch_info[cidx+1].widx = cidx;
    } else {
        mip->ch_info[cidx].widx = cidx; /* each has window info */
        mip->ch_info[cidx+1].widx = cidx+1;
    }
    return cidx;

    /* build configuration */
    case 'f':
    if (mip->nfch + nch > FChans) {
/*      CommonWarning("Unanticipated front channel"); */
        return -1;
    }
    if (mip->nfch == 0) {
        /* consider case of center channel */
        if (FCenter) {
        if (cpe) {
            /* has center speaker but center channel missing */
            cidx = 0 + 1;
            mip->nfch = 1 + nch;
        }
        else {
            if (mip->nfsce & 1) {
            /* has center speaker and this is center channel */
            /* odd number of leading SCE's */
            cidx = 0;
            mip->nfch = nch;
            }
            else {
            /* has center speaker but center channel missing */
            /* even number of leading SCE's */
            /* (Note that in implicit congiguration
             * channel to speaker mapping may be wrong
             * for first block while count of SCE's prior
             * to first CPE is being make. However first block
             * is not written so it doesn't matter.
             * Second block will be correct.
             */
            cidx = 0 + 1;
            mip->nfch = 1 + nch;
            }
        }
        }
        else {
        if (cpe) {
            /* no center speaker and center channel missing */
            cidx = 0;
            mip->nfch = nch;
        }
        else {
            /* no center speaker so this is left channel */
            cidx = 0;
            mip->nfch = nch;
        }
        }
    }
    else {
        cidx = mip->nfch;
        mip->nfch += nch;
    }
    break;
    case 's':
    if (mip->nsch + nch > SChans)  {
/*      CommonWarning("Unanticipated side channel"); */
        return -1;
    }
    cidx = FChans + mip->nsch;
    mip->nsch += nch;
    break;
    case 'b':
    if (mip->nbch + nch > BChans) {
/*      CommonWarning("Unanticipated back channel"); */
        return -1;
    }
    cidx = FChans + SChans + mip->nbch;
    mip->nbch += nch;
    break;
    case 'l':
    if (mip->nlch + nch > LChans) {
/*      CommonWarning("Unanticipated LFE channel"); */
        return -1;
    }
    cidx = FChans + SChans + BChans + mip->nlch; /* sdb */
    mip->nlch += nch;
    break;
    default:
/*      CommonExit(1,"2020: Error in channel configuration"); */
        return -1;
    }
    mip->nch += nch;

    if (cpe == 0) {
    /* SCE */
    cip = &mip->ch_info[cidx];
    cip->present = 1;
    cip->tag = tag;
    cip->cpe = 0;
    cip->common_window = common_window;
    cip->widx = cidx;
    mip->nch = cidx + 1;
    }
    else {
        /* CPE */
    /* left */
    cip = &mip->ch_info[cidx];
    cip->present = 1;
    cip->tag = tag;
    cip->cpe = 1;
    cip->common_window = common_window;
    cip->ch_is_left = 1;
    cip->paired_ch = cidx+1;
    /* right */
    cip = &mip->ch_info[cidx+1];
    cip->present = 1;
    cip->tag = tag;
    cip->cpe = 1;
    cip->common_window = common_window;
    cip->ch_is_left = 0;
    cip->paired_ch = cidx;
    if (common_window) {
        mip->ch_info[cidx].widx = cidx; /* window info is left */
        mip->ch_info[cidx+1].widx = cidx;
    }
    else {
        mip->ch_info[cidx].widx = cidx; /* each has window info */
        mip->ch_info[cidx+1].widx = cidx+1;
    }
    mip->nch = cidx + 2;
    }

    return cidx;
}

static char default_position(faacDecHandle hDecoder, MC_Info *mip, int id)
{
    if (mip->nch < FChans)
    {
        if (id == ID_CPE)   /* first CPE */
            hDecoder->first_cpe = 1;
        else if ((hDecoder->frameNum==0) && !hDecoder->first_cpe)
            /* count number of SCE prior to first CPE in first block */
            mip->nfsce++;

        return('f'); /* front */
    }
    else if  (mip->nch < FChans+SChans)
        return('s'); /* side */
    else if  (mip->nch < FChans+SChans+BChans)
        return('b'); /* back */

    return 0;
}

/* retrieve appropriate channel index for the program
 * and decoder configuration
 */
int chn_config(faacDecHandle hDecoder, int id, int tag, int common_window, MC_Info *mip)
{
    int cpe, cidx=0;
    char position;

    /* channel index to position mapping for 5.1 configuration is
     *  0   center
     *  1   left  front
     *  2   right front
     *  3   left surround
     *  4   right surround
     *  5   lfe
     */

    cpe = (id == ID_CPE) ? 1 : 0;

    if (hDecoder->default_config)
    {
        switch (id)
        {
            case ID_SCE:
            case ID_CPE:
                if ((position = default_position(hDecoder, mip, id)) == 0)
                {
/*                  CommonExit(1,"2021: Unanticipated channel"); */
                    return (-1);
                }
                cidx = enter_chn(cpe, tag, position, common_window, mip);
            break;
        }
    }
    else
        cidx = enter_chn(cpe, tag, 0, common_window, mip);


    return cidx;        /* index of chn in mc_info */
}

/*
 * check continuity of configuration from one
 * block to next
 */
void reset_mc_info(faacDecHandle hDecoder, MC_Info *mip)
{
    int i;
    Ch_Info *p;

    /* only reset for implicit configuration */
    if (hDecoder->default_config)
    {
        /* reset channel counts */
        mip->nch = 0;
        mip->nfch = 0;
        mip->nsch = 0;
        mip->nbch = 0;
        mip->nlch = 0;
        mip->ncch = 0;

        if (hDecoder->frameNum == 0)
            /* reset prior to first block scan only! */
            mip->nfsce = 0;

        for (i=0; i<Chans; i++)
        {
            p = &mip->ch_info[i];
            p->present = 0;
            p->cpe = 0;
            p->ch_is_left = 0;
            p->paired_ch = 0;
            p->is_present = 0;
            p->widx = 0;
            p->ncch = 0;
        }
    }
}


int check_mc_info(faacDecHandle hDecoder, MC_Info *mip, int new_config)
{
    int i, nch, err;
    Ch_Info *s, *p;

    nch = mip->nch;
    if (new_config) {
        /* enter valid configuration */
        for (i=0; i<nch; i++) {
            s = &hDecoder->save_mc_info.ch_info[i];
            p = &mip->ch_info[i];
            s->present = p->present;
            s->cpe = p->cpe;
            s->ch_is_left = p->ch_is_left;
            s->paired_ch = p->paired_ch;
        }
    } else {
        /* check this block's configuration */
        err = 0;
        for (i=0; i<nch; i++) {
            s = &hDecoder->save_mc_info.ch_info[i];
            p = &mip->ch_info[i];
            if (s->present != p->present) err=1;
            if (!s->present) continue; /* sdb */
            if (s->cpe != p->cpe) err=1;
            if (s->ch_is_left != p->ch_is_left) err=1;
            if (s->paired_ch != p->paired_ch) err=1;
        }
        if (err) {
            /* CommonExit(1,"Channel configuration inconsistency"); */
            return 0;
        }
    }

    return 1;
}


/* given cpe and tag,
 *  returns channel index of SCE or left chn in CPE
 */
static int ch_index(MC_Info *mip, int cpe, int tag)
{
    int ch;

    for (ch=0; ch<mip->nch; ch++) {

    if (!(mip->ch_info[ch].present))
        continue;

    if ( (mip->ch_info[ch].cpe == cpe) &&
        (mip->ch_info[ch].tag == tag) )
        return ch;
    }

    /* no match, so channel is not in this program
     *  dummy up the ch_info structure so rest of chn will parse
     */
    if (XChans > 0) {
    ch = Chans - XChans;    /* left scratch channel */
    mip->ch_info[ch].cpe = cpe;
    mip->ch_info[ch].ch_is_left = 1;
    mip->ch_info[ch].widx = ch;
    if (cpe) {
        mip->ch_info[ch].paired_ch = ch+1;
        mip->ch_info[ch+1].ch_is_left = 0;
        mip->ch_info[ch+1].paired_ch = ch;
    }
    }
    else {
    ch = -1;        /* error, no scratch space */
    }

    return ch;
}
