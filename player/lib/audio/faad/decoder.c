/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS
and edited by
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

#include "all.h"
#include "block.h"
#include "nok_lt_prediction.h"
#include "transfo.h"
#include "bits.h"
#include "util.h"


/* D E F I N E S */
#define ftol(A,B) {tmp = *(int*) & A - 0x4B7F8000; B = (short)( (tmp==(short)tmp) ? tmp : (tmp>>31)^0x7FFF);}
#define BYTE_NUMBIT 8       /* bits in byte (char) */
#define bit2byte(a) ((a)/BYTE_NUMBIT) /* (((a)+BYTE_NUMBIT-1)/BYTE_NUMBIT) */

static unsigned long ObjectTypesTable[32] = {0, 1, 1, 1, 1, };


int parse_audio_decoder_specific_info(faacDecHandle hDecoder,unsigned long *samplerate,unsigned long *channels)
{
    unsigned long ObjectTypeIndex, SamplingFrequencyIndex, ChannelsConfiguration;

    faad_byte_align(&hDecoder->ld);
    ObjectTypeIndex = faad_getbits(&hDecoder->ld, 5);
    SamplingFrequencyIndex = faad_getbits(&hDecoder->ld, 4);
    ChannelsConfiguration = faad_getbits(&hDecoder->ld, 4);

    if(ObjectTypesTable[ObjectTypeIndex] != 1){
        return -1;
    }

    *samplerate = SampleRates[SamplingFrequencyIndex];
    if(*samplerate == 0){
        return -2;
    }

    hDecoder->numChannels = *channels = ChannelsConfiguration;

    hDecoder->mc_info.object_type = ObjectTypeIndex - 1;
    hDecoder->mc_info.sampling_rate_idx = SamplingFrequencyIndex;

    if((ChannelsConfiguration != 1) && (ChannelsConfiguration != 2)){

        return -3;

    }

    return 0;

}

int FAADAPI faacDecInit2(faacDecHandle hDecoder,
                         unsigned char* pBuffer,unsigned long SizeOfDecoderSpecificInfo,
                         unsigned long *samplerate, unsigned long *channels)
{
    int rc;

    hDecoder->adif_header_present = 0;
    hDecoder->adts_header_present = 0;

    if((hDecoder == NULL)
        || (pBuffer == NULL)
        || (SizeOfDecoderSpecificInfo < 2)
        || (samplerate == NULL)
        || (channels == NULL)){
        return -1;
    }

    /* get adif header */

    faad_initbits(&hDecoder->ld, pBuffer, 0);

    rc = parse_audio_decoder_specific_info(hDecoder,samplerate,channels);

    if(rc != 0){
        return rc;
    }

    huffbookinit(hDecoder);
    nok_init_lt_pred(hDecoder->nok_lt_status, Chans);
    init_pred(hDecoder, hDecoder->sp_status, Chans);
    MakeFFTOrder(hDecoder);
    InitBlock(hDecoder);  /* calculate windows */

    hDecoder->winmap[0] = hDecoder->win_seq_info[ONLY_LONG_WINDOW];
    hDecoder->winmap[1] = hDecoder->win_seq_info[ONLY_LONG_WINDOW];
    hDecoder->winmap[2] = hDecoder->win_seq_info[EIGHT_SHORT_WINDOW];
    hDecoder->winmap[3] = hDecoder->win_seq_info[ONLY_LONG_WINDOW];

    return 0;
}


faacDecHandle FAADAPI faacDecOpen(void)
{
    int i;
    faacDecHandle hDecoder = NULL;

    hDecoder = (faacDecHandle)AllocMemory(sizeof(faacDecStruct));
#ifndef _WIN32
    SetMemory(hDecoder, 0, sizeof(faacDecStruct));
#endif

    hDecoder->frameNum = 0;
    hDecoder->isMpeg4 = 1;

    /* set defaults */
    hDecoder->current_program = -1;
    hDecoder->default_config = 1;
    hDecoder->dolbyShortOffset_f2t = 1;
    hDecoder->dolbyShortOffset_t2f = 1;
    hDecoder->first_cpe = 0;
    hDecoder->warn_flag = 1;

    hDecoder->config.defObjectType = AACMAIN;
    hDecoder->config.defSampleRate = 44100;

    for(i=0; i < Chans; i++)
    {
        hDecoder->coef[i] = (Float*)AllocMemory(LN2*sizeof(Float));
        hDecoder->data[i] = (Float*)AllocMemory(LN2*sizeof(Float));
        hDecoder->state[i] = (Float*)AllocMemory(LN*sizeof(Float));
        hDecoder->factors[i] = (int*)AllocMemory(MAXBANDS*sizeof(int));
        hDecoder->cb_map[i] = (byte*)AllocMemory(MAXBANDS*sizeof(byte));
        hDecoder->group[i] = (byte*)AllocMemory(NSHORT*sizeof(byte));
        hDecoder->lpflag[i] = (int*)AllocMemory(MAXBANDS*sizeof(int));
        hDecoder->prstflag[i] = (int*)AllocMemory((LEN_PRED_RSTGRP+1)*sizeof(int));
        hDecoder->tns[i] = (TNS_frame_info*)AllocMemory(sizeof(TNS_frame_info));
        hDecoder->nok_lt_status[i]  = (NOK_LT_PRED_STATUS*)AllocMemory(sizeof(NOK_LT_PRED_STATUS));
        hDecoder->sp_status[i]  = (PRED_STATUS*)AllocMemory(LN*sizeof(PRED_STATUS));

        hDecoder->last_rstgrp_num[i] = 0;
        hDecoder->wnd_shape[i].prev_bk = 0;

#ifndef _WIN32 /* WIN32 API sets memory to 0 in allocation */
        SetMemory(hDecoder->coef[i],0,LN2*sizeof(Float));
        SetMemory(hDecoder->data[i],0,LN2*sizeof(Float));
        SetMemory(hDecoder->state[i],0,LN*sizeof(Float));
        SetMemory(hDecoder->factors[i],0,MAXBANDS*sizeof(int));
        SetMemory(hDecoder->cb_map[i],0,MAXBANDS*sizeof(byte));
        SetMemory(hDecoder->group[i],0,NSHORT*sizeof(byte));
        SetMemory(hDecoder->lpflag[i],0,MAXBANDS*sizeof(int));
        SetMemory(hDecoder->prstflag[i],0,(LEN_PRED_RSTGRP+1)*sizeof(int));
        SetMemory(hDecoder->tns[i],0,sizeof(TNS_frame_info));
        SetMemory(hDecoder->nok_lt_status[i],0,sizeof(NOK_LT_PRED_STATUS));
        SetMemory(hDecoder->sp_status[i],0,LN*sizeof(PRED_STATUS));
#endif
    }

    hDecoder->mnt_table = AllocMemory(128*sizeof(float));
    hDecoder->exp_table = AllocMemory(256*sizeof(float));
    hDecoder->iq_exp_tbl = AllocMemory(MAX_IQ_TBL*sizeof(Float));
    hDecoder->exptable = AllocMemory(TEXP*sizeof(Float));
    hDecoder->unscambled64 = AllocMemory(64*sizeof(int));
    hDecoder->unscambled512 = AllocMemory(512*sizeof(int));

    SetMemory(hDecoder->lp_store, 0, MAXBANDS*sizeof(int));
    SetMemory(hDecoder->noise_state_save, 0, MAXBANDS*sizeof(long));

    for(i=0; i<Winds; i++)
    {
        hDecoder->mask[i] = (byte*)AllocMemory(MAXBANDS*sizeof(byte));
        SetMemory(hDecoder->mask[i],0,MAXBANDS*sizeof(byte));
    }

    for (i = 0; i < NUM_WIN_SEQ; i++)
        hDecoder->win_seq_info[i] = NULL;

    return hDecoder;
}

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(faacDecHandle hDecoder)
{
    faacDecConfigurationPtr config = &(hDecoder->config);

    return config;
}

int FAADAPI faacDecSetConfiguration(faacDecHandle hDecoder,
                                    faacDecConfigurationPtr config)
{
    hDecoder->config.defObjectType = config->defObjectType;
    hDecoder->config.defSampleRate = config->defSampleRate;

    /* OK */
    return 1;
}

int FAADAPI faacDecInit(faacDecHandle hDecoder,
			unsigned char *buffer,
                        unsigned long *samplerate,
			unsigned long *channels)
{
    int i, bits = 0;
    char chk_header[4];

    faad_initbits(&hDecoder->ld, buffer, 4);
#if 0
    faad_bookmark(&hDecoder->ld, 1);
#endif
    for (i = 0; i < 4; i++) {
      chk_header[i] = buffer[i];
    }

    /* Check if an ADIF header is present */
    if (stringcmp(chk_header, "ADIF", 4) == 0)
        hDecoder->adif_header_present = 1;
    else
        hDecoder->adif_header_present = 0;

    /* Check if an ADTS header is present */
    if (hDecoder->adif_header_present == 0)
    {
        if (((int) ( chk_header[0] == (char) 0xFF)) &&
            ((int) ( (chk_header[1] & (char) 0xF0) == (char) 0xF0)))
        {
            hDecoder->adts_header_present = 1;
        } else {
            hDecoder->adts_header_present = 0;
        }
    }

    /* get adif header */
    if (hDecoder->adif_header_present) {
        hDecoder->pceChannels = 2;
#if 0
        faad_bookmark(&hDecoder->ld, 0);
        faad_bookmark(&hDecoder->ld, 1);
#else
	faad_initbits(&hDecoder->ld, buffer, 4);
#endif
        get_adif_header(hDecoder);
        /* only MPEG2 ADIF header uses byte_alignment */
        /* but the PCE already byte aligns the data */
        /*
        if (!hDecoder->isMpeg4)
            byte_align(&hDecoder->ld);
        */
        bits = faad_get_processed_bits(&hDecoder->ld);
    } else if (hDecoder->adts_header_present) {
#if 0
        faad_bookmark(&hDecoder->ld, 0);
        faad_bookmark(&hDecoder->ld, 1);
#else
	faad_initbits(&hDecoder->ld, buffer, 4);
#endif
        get_adts_header(hDecoder);
        /* only MPEG2 ADTS header uses byte_alignment */
        /* but it already is a multiple of 8 bits */
        /*
        if (!hDecoder->isMpeg4)
            byte_align(&hDecoder->ld);
        */
        bits = 0;
    } else {
        hDecoder->mc_info.object_type = hDecoder->config.defObjectType;
        hDecoder->mc_info.sampling_rate_idx = get_sr_index(hDecoder->config.defSampleRate);
    }

    *samplerate = hDecoder->config.defSampleRate;
    hDecoder->numChannels = *channels = 2;
    hDecoder->chans_inited = 0;
    if (hDecoder->adif_header_present) {
      hDecoder->chans_inited = 1;
        *samplerate = SampleRates[hDecoder->prog_config.sampling_rate_idx];
        hDecoder->numChannels = *channels = hDecoder->pceChannels;
    } else if (hDecoder->adts_header_present) {
      hDecoder->chans_inited = 1;
        *samplerate = SampleRates[hDecoder->adts_header.fixed.sampling_rate_idx];
        hDecoder->numChannels = *channels = hDecoder->adts_header.fixed.channel_configuration; /* This works up to 6 channels */
    }

    huffbookinit(hDecoder);
    nok_init_lt_pred(hDecoder->nok_lt_status, Chans);
    init_pred(hDecoder, hDecoder->sp_status, Chans);
    MakeFFTOrder(hDecoder);
    InitBlock(hDecoder);  /* calculate windows */

    hDecoder->winmap[0] = hDecoder->win_seq_info[ONLY_LONG_WINDOW];
    hDecoder->winmap[1] = hDecoder->win_seq_info[ONLY_LONG_WINDOW];
    hDecoder->winmap[2] = hDecoder->win_seq_info[EIGHT_SHORT_WINDOW];
    hDecoder->winmap[3] = hDecoder->win_seq_info[ONLY_LONG_WINDOW];

#if 0
    faad_bookmark(&hDecoder->ld, 0);
#endif

    return bit2byte(bits);
}

void FAADAPI faacDecClose(faacDecHandle hDecoder)
{
    int i;

    EndBlock(hDecoder);
    nok_end_lt_pred(hDecoder->nok_lt_status, Chans);

    for(i=0; i < Chans; i++)
    {
        if (hDecoder->coef[i]) FreeMemory(hDecoder->coef[i]);
        if (hDecoder->data[i]) FreeMemory(hDecoder->data[i]);
        if (hDecoder->state[i]) FreeMemory(hDecoder->state[i]);
        if (hDecoder->factors[i]) FreeMemory(hDecoder->factors[i]);
        if (hDecoder->cb_map[i]) FreeMemory(hDecoder->cb_map[i]);
        if (hDecoder->group[i]) FreeMemory(hDecoder->group[i]);
        if (hDecoder->lpflag[i]) FreeMemory(hDecoder->lpflag[i]);
        if (hDecoder->prstflag[i]) FreeMemory(hDecoder->prstflag[i]);
        if (hDecoder->tns[i]) FreeMemory(hDecoder->tns[i]);
        if (hDecoder->nok_lt_status[i]) FreeMemory(hDecoder->nok_lt_status[i]);
        if (hDecoder->sp_status[i]) FreeMemory(hDecoder->sp_status[i]);
    }

    if (hDecoder->mnt_table) FreeMemory(hDecoder->mnt_table);
    if (hDecoder->exp_table) FreeMemory(hDecoder->exp_table);
    if (hDecoder->iq_exp_tbl) FreeMemory(hDecoder->iq_exp_tbl);
    if (hDecoder->exptable) FreeMemory(hDecoder->exptable);
    if (hDecoder->unscambled64) FreeMemory(hDecoder->unscambled64);
    if (hDecoder->unscambled512) FreeMemory(hDecoder->unscambled512);

    for(i=0; i<Winds; i++)
    {
        if (hDecoder->mask[i]) FreeMemory(hDecoder->mask[i]);
    }

    if (hDecoder) FreeMemory(hDecoder);
}

int FAADAPI faacDecGetProgConfig(faacDecHandle hDecoder,
                                 faacProgConfig *progConfig)
{
    return hDecoder->numChannels;
}

int FAADAPI faacDecDecode(faacDecHandle hDecoder,
                          unsigned char *buffer,
                          unsigned long *bytesconsumed,
                          short *sample_buffer,
                          unsigned long *samples)
{
    unsigned char d_bytes[MAX_DBYTES];
    int i, j, ch, wn, ele_id;
    int left, right, tmp;
    int d_tag, d_cnt;
    int channels = 0;
    Info *info;
    MC_Info *mip = &(hDecoder->mc_info);
    Ch_Info *cip;
    int retCode = FAAD_OK;

    faad_initbits(&hDecoder->ld, buffer, *bytesconsumed);

    if (hDecoder->adts_header_present)
    {
        if (get_adts_header(hDecoder))
        {
            goto error;
        }
        /* MPEG2 does byte_alignment() here
        * but ADTS header is always multiple of 8 bits in MPEG2
        * so not needed
        */
    }

    reset_mc_info(hDecoder, mip);
    while ((ele_id=faad_getbits(&hDecoder->ld, LEN_SE_ID)) != ID_END)
    {
        /* get audio syntactic element */
        switch (ele_id) {
        case ID_SCE:        /* single channel */
        case ID_CPE:        /* channel pair */
        case ID_LFE:        /* low freq effects channel */
            if (huffdecode(hDecoder, ele_id, mip, hDecoder->wnd, hDecoder->wnd_shape,
                hDecoder->cb_map, hDecoder->factors, hDecoder->group, hDecoder->hasmask,
                hDecoder->mask, hDecoder->max_sfb,
                hDecoder->lpflag, hDecoder->prstflag, hDecoder->nok_lt_status,
                hDecoder->tns, hDecoder->coef) < 0)
                goto error;
                /* CommonExit(1,"2022: Error in huffman decoder"); */
            if (ele_id == ID_CPE)
                channels += 2;
            else
                channels++;
            break;
        case ID_DSE:        /* data element */
            if (getdata(hDecoder, &d_tag, &d_cnt, d_bytes) < 0)
                goto error;
            break;
        case ID_PCE:        /* program config element */
            get_prog_config(hDecoder, &hDecoder->prog_config);
            break;
        case ID_FIL:        /* fill element */
            getfill(hDecoder, d_bytes);
            break;
        default:
            /* CommonWarning("Element not supported"); */
            goto error;
        }
    }

    if ((channels != hDecoder->numChannels) &&
	(hDecoder->chans_inited == 0)) {
        hDecoder->numChannels = channels;
        retCode = FAAD_OK_CHUPDATE;
        *bytesconsumed = 0;

#if 0
	// wmay = fall through...
        /* no errors, but channel update */
        return retCode;
#endif
    }

    if (!check_mc_info(hDecoder, mip, (hDecoder->frameNum==0 && hDecoder->default_config))) {
        goto error;
    }

    /* m/s stereo */
    for (ch=0; ch<Chans; ch++) {
        cip = &mip->ch_info[ch];
        if ((cip->present) && (cip->cpe) && (cip->ch_is_left)) {
            wn = cip->widx;
            if(hDecoder->hasmask[wn]) {
                left = ch;
                right = cip->paired_ch;
                info = hDecoder->winmap[hDecoder->wnd[wn]];
                if (hDecoder->hasmask[wn] == 1)
                    map_mask(info, hDecoder->group[wn], hDecoder->mask[wn],
                    hDecoder->cb_map[right]);
                synt(info, hDecoder->group[wn], hDecoder->mask[wn],
                    hDecoder->coef[right], hDecoder->coef[left]);
            }
        }
    }

    /* intensity stereo and prediction */
    for (ch=0; ch<Chans; ch++) {
        if (!(mip->ch_info[ch].present))
            continue;

        wn = mip->ch_info[ch].widx;
        info = hDecoder->winmap[hDecoder->wnd[wn]];

        pns(hDecoder, mip, info, wn, ch,
            hDecoder->group[wn], hDecoder->cb_map[ch], hDecoder->factors[ch],
            hDecoder->lpflag[wn], hDecoder->coef);

        intensity(mip, info, wn, ch,
            hDecoder->group[wn], hDecoder->cb_map[ch], hDecoder->factors[ch],
            hDecoder->lpflag[wn], hDecoder->coef);

        if (mip->object_type == AACLTP) {
            nok_lt_predict(hDecoder, info, hDecoder->wnd[wn], &hDecoder->wnd_shape[wn],
                hDecoder->nok_lt_status[ch]->sbk_prediction_used,
                hDecoder->nok_lt_status[ch]->sfb_prediction_used,
                hDecoder->nok_lt_status[ch], hDecoder->nok_lt_status[ch]->weight,
                hDecoder->nok_lt_status[ch]->delay, hDecoder->coef[ch],
                BLOCK_LEN_LONG, 0, BLOCK_LEN_SHORT, hDecoder->tns[ch]);
        } else if (mip->object_type == AACMAIN) {
            predict(hDecoder, info, mip->object_type, hDecoder->lpflag[wn],
                hDecoder->sp_status[ch], hDecoder->coef[ch]);
        }
    }

    for (ch = 0; ch < Chans; ch++) {
        if (!(mip->ch_info[ch].present))
            continue;

        wn = mip->ch_info[ch].widx;
        info = hDecoder->winmap[hDecoder->wnd[wn]];

        /* predictor reset */
        if (mip->object_type == AACMAIN) {
            left = ch;
            right = left;
            if ((mip->ch_info[ch].cpe) && (mip->ch_info[ch].common_window))
                /* prstflag's shared by channel pair */
                right = mip->ch_info[ch].paired_ch;
            predict_reset(hDecoder, info, hDecoder->prstflag[wn], hDecoder->sp_status,
                left, right, hDecoder->last_rstgrp_num);

            /* PNS predictor reset */
            predict_pns_reset(info, hDecoder->sp_status[ch], hDecoder->cb_map[ch]);
        }

        /* tns */
        for (i=j=0; i < hDecoder->tns[ch]->n_subblocks; i++) {

            tns_decode_subblock(hDecoder, &hDecoder->coef[ch][j],
                hDecoder->max_sfb[wn],
                info->sbk_sfb_top[i],
                info->islong,
                &(hDecoder->tns[ch]->info[i]) );

            j += info->bins_per_sbk[i];
        }

        /* inverse transform */
        freq2time_adapt(hDecoder, hDecoder->wnd[wn], &hDecoder->wnd_shape[wn],
            hDecoder->coef[ch], hDecoder->state[ch], hDecoder->data[ch]);

        if (mip->object_type == AACLTP) {
            nok_lt_update(hDecoder->nok_lt_status[ch], hDecoder->data[ch],
                hDecoder->state[ch], BLOCK_LEN_LONG);
        }
    }

    /* Copy output to a standard PCM buffer */
    for(i=0; i < 1024; i++) /* prkoat */
    {

        for (ch=0; ch < mip->nch; ch++)
        {
/* much faster FTOL */
#ifndef ENDIAN_SAFE
            float ftemp;
            /* ftemp = truncate(data[ch][i]) + 0xff8000; */
            ftemp = hDecoder->data[ch][i] + 0xff8000;
            ftol(ftemp, sample_buffer[(i*mip->nch)+ch]);
#else
            sample_buffer[(i*mip->nch)+ch] = (short)truncate(data[ch][i]);
#endif

        }
    }

    hDecoder->frameNum++;
    faad_byte_align(&hDecoder->ld);

    //printf("processed %d bits\n", faad_get_processed_bits(&hDecoder->ld));
    *bytesconsumed = bit2byte(faad_get_processed_bits(&hDecoder->ld));
    if (hDecoder->frameNum > 2)
        *samples = 1024*mip->nch;
    else
        *samples = 0;

    /* no errors */
    return retCode;

error:
#if 0
    //
    // wmay - remove this error recovery, and let calling app handle this -
    // with mpeg4ip, we can get better recovery, especially with RTP.
    //
    /* search for next ADTS header first, so decoding can be continued */
    /* ADIF and RAW AAC files will not be able to continue playing */
    if (hDecoder->adts_header_present) {
        int k, sync = 0;

        faad_byte_align(&hDecoder->ld);

        for(k = 0; sync != 4096 - 1; k++)
        {
            sync = faad_getbits(&hDecoder->ld, 12); /* 12 bit SYNCWORD */
            faad_getbits(&hDecoder->ld, 4);

            /* Bail out if no syncword found */
            if(k >= (6144 / 16))
            {
                SetMemory(sample_buffer, 0, sizeof(short)*mip->nch*1024);

                /* unrecoverable error */
                return FAAD_FATAL_ERROR;
            }
        }

        *bytesconsumed = bit2byte(hDecoder->ld.framebits - 16);
        SetMemory(sample_buffer, 0, sizeof(short)*mip->nch*1024);

        /* error, but recoverable */
        return FAAD_ERROR;
    } else {
        SetMemory(sample_buffer, 0, sizeof(short)*mip->nch*1024);

        /* unrecoverable error */
        return FAAD_FATAL_ERROR;
    }
#else
    *bytesconsumed = bit2byte(faad_get_processed_bits(&hDecoder->ld));
    return FAAD_ERROR;
#endif
}

