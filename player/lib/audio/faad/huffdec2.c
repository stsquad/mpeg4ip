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

#include <math.h>
#include "all.h"
#include "port.h"

Info only_long_info;

typedef int DINT_DATATYPE ;

static void
deinterleave(DINT_DATATYPE inptr[], DINT_DATATYPE outptr[], short ngroups,
    short nsubgroups[], int ncells[], short cellsize[])
{
    int i, j, k, l;
    DINT_DATATYPE *start_inptr, *start_subgroup_ptr, *subgroup_ptr;
    short cell_inc, subgroup_inc;

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

	for (j = 0; j < ncells[i]; j++) {
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

static void 
calc_gsfb_table(Info *info, byte *group)
{
    int group_offset;
    int group_idx;
    int offset;
    short * group_offset_p;
    int sfb,len;
    /* first calc the group length*/
    if (info->islong){
	return;
    } else {
	group_offset = 0;
	group_idx =0;
	do  {
	    info->group_len[group_idx]=group[group_idx]-group_offset;
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

void
getgroup(Info *info, byte *group)
{
    int i, j, first_short;

    first_short=1;
    for (i = 0; i < info->nsbk; i++) {
	if (info->bins_per_sbk[i] > SN2) {
	    /* non-short windows are always their own group */
	    *group++ = i+1;
	}
	else {
	    /* only short-window sequences are grouped! */
	    if (first_short) {
		/* first short window is always a new group */
		first_short=0;
	    }
	    else {
		if((j = getbits(1)) == 0) {
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
int
getmask(Info *info, byte *group, byte max_sfb, byte *mask)
{
    int b, i, mp;

    mp = getbits(LEN_MASK_PRES);

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
	    *mask = (byte)getbits(LEN_MASK);
	    mask++;
	}
	for( ; i < info->sfb_per_sbk[b]; i++){
	    *mask = 0;
	    mask++;
	}
    }
    return 1;
}

void
clr_tns( Info *info, TNS_frame_info *tns_frame_info )
{
    int s;

    tns_frame_info->n_subblocks = info->nsbk;
    for (s=0; s<tns_frame_info->n_subblocks; s++)
	tns_frame_info->info[s].n_filt = 0;
}

int
get_tns( Info *info, TNS_frame_info *tns_frame_info )
{
    int                       f, t, top, res, res2, compress;
    int                       short_flag, s;
    short                     *sp, tmp, s_mask, n_mask;
    TNSfilt                   *tns_filt;
    TNSinfo                   *tns_info;
    static int                sgn_mask[] = { 0x2, 0x4, 0x8 };
    static int                neg_mask[] = { 0xfffc, 0xfff8, 0xfff0 };


    short_flag = (!info->islong);
    tns_frame_info->n_subblocks = info->nsbk;

    for (s=0; s<tns_frame_info->n_subblocks; s++) {
	tns_info = &tns_frame_info->info[s];

	if (!(tns_info->n_filt = getbits( short_flag ? 1 : 2 )))
	    continue;
	    
	tns_info -> coef_res = res = getbits( 1 ) + 3;
	top = info->sfb_per_sbk[s];
	tns_filt = &tns_info->filt[ 0 ];
	for (f=tns_info->n_filt; f>0; f--)  {
	    tns_filt->stop_band = top;
	    top = tns_filt->start_band = top - getbits( short_flag ? 4 : 6 );
	    tns_filt->order = getbits( short_flag ? 3 : 5 );

	    if (tns_filt->order)  {
		tns_filt->direction = getbits( 1 );
		compress = getbits( 1 );

		res2 = res - compress;
		s_mask = sgn_mask[ res2 - 2 ];
		n_mask = neg_mask[ res2 - 2 ];

		sp = tns_filt -> coef;
		for (t=tns_filt->order; t>0; t--)  {
		    tmp = (short)getbits( res2 );
		    *sp++ = (tmp & s_mask) ? (short)(tmp | n_mask) : (short)tmp;
		}
	    }
	    tns_filt++;
	}
    }   /* subblock loop */
    return 1;
}

/*
 * NEC noiseless coding
 */
struct Nec_Info
{
    int pulse_data_present;
    int number_pulse;
    int pulse_start_sfb;
    int pulse_position[NUM_NEC_LINES];
    int pulse_offset[NUM_NEC_LINES];
    int pulse_amp[NUM_NEC_LINES];
};
struct Nec_Info nec_info;

static void
get_nec_nc(struct Nec_Info *nec_info)
{
    int i;
    nec_info->number_pulse = getbits(LEN_NEC_NPULSE);
    nec_info->pulse_start_sfb = getbits(LEN_NEC_ST_SFB);
    for(i=0; i<nec_info->number_pulse+1; i++) {
	nec_info->pulse_offset[i] = getbits(LEN_NEC_POFF);
	nec_info->pulse_amp[i] = getbits(LEN_NEC_PAMP);
    }
}

static void
nec_nc(int *coef, struct Nec_Info *nec_info)
{
    int i, k;

    k = only_long_info.sbk_sfb_top[0][nec_info->pulse_start_sfb];

    for(i=0; i<=nec_info->number_pulse; i++) {
		k += nec_info->pulse_offset[i];
		if (coef[k]>0)
			coef[k] += nec_info->pulse_amp[i];
		else
			coef[k] -= nec_info->pulse_amp[i];
    }
}

int
getics(Info *info, int common_window, byte *win, byte *wshape, 
    byte *group, byte *max_sfb, int *lpflag, int *prstflag, 
    byte *cb_map, Float *coef, short *global_gain, 
    short *factors,
    NOK_LT_PRED_STATUS *nok_ltp_status,
    TNS_frame_info *tns
	   )
{
    int nsect, i, cb, top, bot, tot_sfb;
    byte sect[ 2*(MAXBANDS+1) ];

    /*
	* global gain
	*/
    *global_gain = (short)getbits(LEN_SCL_PCM);

    if (!common_window)
		get_ics_info(win, wshape, group, max_sfb, lpflag, prstflag
		, nok_ltp_status
		);
    memcpy(info, winmap[*win], sizeof(Info));

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
    nsect = huffcb(sect, info->sectbits, tot_sfb, info->sfb_per_sbk[0], *max_sfb);
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
    if(!hufffac(info, group, nsect, sect, *global_gain, factors))
		return 0;

	/*
	* NEC noiseless coding
	*/
    if ((nec_info.pulse_data_present = getbits(LEN_PULSE_PRES))) {
		if (info->islong) {
			get_nec_nc(&nec_info);
		}
		else {
			CommonExit(1,"Pulse data not allowed for short blocks");
		}
    }

    /*
	* tns data
	*/
    if (getbits(LEN_TNS_PRES)) {
		get_tns(info, tns);
    }
    else {
		clr_tns(info, tns);
    }

    if (getbits(LEN_GAIN_PRES)) {   
      CommonExit(1, "Gain control not implemented");   
    }   

    return huffspec(info, nsect, sect, factors, coef);
}

/*
 * read the codebook and boundaries
 */
int
huffcb(byte *sect, int *sectbits, int tot_sfb, int sfb_per_sbk, byte max_sfb)
{
    int nsect, n, base, bits, len;

    bits = sectbits[0];
    len = (1 << bits) - 1;
    nsect = 0;
    for(base = 0; base < tot_sfb && nsect < tot_sfb; ){
	*sect++ = (byte)getbits(LEN_CB);

	n = getbits(bits);
	while(n == len && base < tot_sfb){
	    base += len;
	    n = getbits(bits);
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
int
hufffac(Info *info, byte *group, int nsect, byte *sect,
		short global_gain, short *factors)
{
    Hcb *hcb;
    Huffman *hcw;
    int i, b, bb, t, n, sfb, top, fac, is_pos;
    int factor_transmitted[MAXBANDS], *fac_trans;
    int noise_pcm_flag = 1;
    int noise_nrg;

    /* clear array for the case of max_sfb == 0 */
	memset(factor_transmitted, 0, MAXBANDS*sizeof(*factor_transmitted));
	memset(factors, 0, MAXBANDS*sizeof(*factors));

    sfb = 0;
    fac_trans = factor_transmitted;
    for(i = 0; i < nsect; i++){
		top = sect[1];		/* top of section in sfb */
		t = sect[0];		/* codebook for this section */
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
    hcb = &book[BOOKSCL];
    hcw = hcb->hcw;
    bb = 0;
    for(b = 0; b < info->nsbk; ){
		n = info->sfb_per_sbk[b];
		b = *group++;
		for(i = 0; i < n; i++){
			switch (fac_trans[i]) {
			case ZERO_HCB:	    /* zero book */
				break;
			default:		    /* spectral books */
				/* decode scale factor */
				t = decode_huff_cw(hcw);
				fac += t - MIDFAC;    /* 1.5 dB */
				if(fac >= 2*TEXP || fac < 0)
					return 0;
				factors[i] = fac;
				break;
			case BOOKSCL:	    /* invalid books */
				return 0;
			case INTENSITY_HCB:	    /* intensity books */
			case INTENSITY_HCB2:
				/* decode intensity position */
				t = decode_huff_cw(hcw);
				is_pos += t - MIDFAC;
				factors[i] = is_pos;
				break;
			case NOISE_HCB:	    /* noise books */
                /* decode noise energy */
                if (noise_pcm_flag) {
					noise_pcm_flag = 0;
					t = getbits( NOISE_PCM_BITS ) - NOISE_PCM_OFFSET;
                }
                else
					t = decode_huff_cw(hcw) - MIDFAC;
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

/* rm2 inverse quantization
 * escape books need ftn call
 * other books done via macro
 */
#define iquant( q ) ( q >= 0 ) ? \
	(Float)iq_exp_tbl[ q ] : (Float)(-iq_exp_tbl[ - q ])

Float
esc_iquant(int q)
{
    if (q > 0) {
		if (q < MAX_IQ_TBL) {
			return((Float)iq_exp_tbl[q]);
		}
		else {
			return((float)pow(q, 4./3.));
		}
    }
    else {
		q = -q;
		if (q < MAX_IQ_TBL) {
			return((Float)(-iq_exp_tbl[q]));
		}
		else {
			return((float)(-pow(q, 4./3.)));
		}
    }
}



int
huffspec(Info *info, int nsect, byte *sect, short *factors, Float *coef)
{
    Hcb *hcb;
    Huffman *hcw;
    int i, j, k, table, step, temp, stop, bottom, top;
    short *bands, *bandp;
    int quant[LN2], *qp;	    /* probably could be short */

    int tmp_spec[LN2];

	memset(quant, 0, LN2*sizeof(*quant));

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
		}
		else {
			step = 2;
		}	hcb = &book[table];
		hcw = hcb->hcw;
		qp = quant+k;
//		kstart=k;
		for(j=bottom; j<top; j++) {
			stop = *bandp++;
			while(k < stop) {
				temp = decode_huff_cw(hcw);
				unpack_idx(qp, temp, hcb);

				if (!hcb->signed_cb)
					get_sign_bits(qp, step);
				if(table == ESCBOOK){
					qp[0] = getescape(qp[0]);
					qp[1] = getescape(qp[1]);
				}
				qp += step;
				k += step;
			}
		}
		bottom = top;
    }

    /* NEC noisless coding reconstruction */
    if ( (info->islong) && (nec_info.pulse_data_present) )
		nec_nc(quant, &nec_info);

    if (!info->islong) {
		deinterleave (quant,tmp_spec,
			(short)info->num_groups,   
			info->group_len,
			info->sfb_per_sbk,
			info->sfb_width_128);
		memcpy(quant,tmp_spec,sizeof(tmp_spec));
    }

    /* inverse quantization */
    for (i=0; i<info->bins_per_bk; i++) {
		coef[i] = esc_iquant(quant[i]);
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
					scale = exptable[fac];
				}
				else {
					if (fac == -SF_OFFSET) {
						scale = 0;
					}
					else {
						scale = (float)pow( 2.0,  0.25*fac );
					}
				}
				for ( ; k<top; k++) {
					*fp++ *= scale;
				}
			}
		}
    }
    return 1;
}

int getescape(int q)
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
		if(getbits(1) == 0)
			break;
    }

    if(i > 16){
		off = getbits(i-16) << 16;
		off |= getbits(16);
    } else
		off = getbits(i);

    i = off + (1<<i);
    if(neg)
		i = -i;
    return i;
}
