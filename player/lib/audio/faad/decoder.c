#include "faad.h"
#include "all.h"
#include "block.h"
#include "dolby_def.h"
#include "nok_lt_prediction.h"
#include "transfo.h"
#include "filestream.h"

int binisopen = 0;
FILE_STREAM *input_file = NULL;
const int SampleRates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};

/* prediction */
NOK_LT_PRED_STATUS *nok_lt_status[Chans];

Float *coef[Chans], *data[Chans], *state[Chans];
byte hasmask[Winds], *mask[Winds], *group[Chans],
   wnd[Chans], max_sfb[Chans],
   *cb_map[Chans];
Wnd_Shape wnd_shape[Chans];
short *factors[Chans];
int *lpflag[Chans], *prstflag[Chans];
TNS_frame_info *tns[Chans];
#if (CChans > 0)
Float *cc_coef[CChans], *cc_gain[CChans][Chans];
byte cc_wnd[CChans];
Wnd_Shape cc_wnd_shape[CChans];
#if (ICChans > 0)
Float *cc_state[ICChans];
#endif
#endif

Info *info;
MC_Info *mip = &mc_info;
Ch_Info *cip;

void do_init()
{
#if (CChans > 0)
    init_cc();
#endif
    huffbookinit();
    predinit();
    MakeFFTOrder();

	winmap[0] = win_seq_info[ONLY_LONG_WINDOW];
	winmap[1] = win_seq_info[ONLY_LONG_WINDOW];
	winmap[2] = win_seq_info[EIGHT_SHORT_WINDOW];
	winmap[3] = win_seq_info[ONLY_LONG_WINDOW];
}

void predinit()
{
    int ch;

	for (ch = 0; ch < Chans; ch++)
      nok_init_lt_pred(nok_lt_status[ch]);
}

void getfill()
{
    int i, cnt;

    if ((cnt = getbits(LEN_F_CNT)) == (1<<LEN_F_CNT)-1)
		cnt +=  getbits(LEN_F_ESC) - 1;

	for (i=0; i<cnt; i++)
		getbits(LEN_BYTE);
}

static int get_adts_header(void)
{
	int tmp, j;

//	if (adts_header.first == 0) {
//		adts_header.first = 1;

		for (j = 0; j < 12; j++) {
			tmp = getbits(1); // 12 bit SYNCWORD
			if (tmp != 1)
				return -1;
		}
		adts_header.fixed.ID = getbits(1);
		adts_header.fixed.layer = getbits(2);
		adts_header.fixed.protection_absent = getbits(1);
		adts_header.fixed.profile = getbits(2);
		adts_header.fixed.sampling_rate_idx = getbits(4);
		adts_header.fixed.private_bit = getbits(1);
		adts_header.fixed.channel_configuration = getbits(3);
		adts_header.fixed.original_copy = getbits(1);
		adts_header.fixed.home = getbits(1);
		adts_header.fixed.emphasis = getbits(2);
//	} else {
//		tmp = getbits(16); // No more than 16 at a time
//		tmp = getbits(14);
//	}

	adts_header.variable.copy_id_bit = getbits(1);
	adts_header.variable.copy_id_start = getbits(1);
	adts_header.variable.frame_length = getbits(13);
	//	printf("Frame length %x\n", adts_header.variable.frame_length);
	adts_header.variable.buffer_fullness = getbits(11);
	adts_header.variable.raw_blocks = getbits(2);

	return 0;
}

int aac_decode_seek(int pos_ms)
{
	int  j, k;
	int frames, to_read;
	double block_per_sec, offset_sec;

	block_per_sec = (double)SampleRates[adts_header.fixed.sampling_rate_idx]/1024.0;

	offset_sec = pos_ms / 1000.0;
	seek_filestream(input_file, 0, SEEK_SET);
	reset_bits();

	frames = (int)((offset_sec * block_per_sec));

	for (j = 0; j < frames; j++)
	{
		if (get_adts_header())
			return -1;

		getbits(6); // Now we have read 64 bits since starting of frame
		to_read = adts_header.variable.frame_length - 8;
		
		for(k=0; k < to_read; k++)
			getbits(8);
	}

	return 0;
}

int aac_decode_init_your_filestream (FILE_STREAM *fs)
{
  input_file = fs;
  return(0);
}

int aac_decode_init_filestream (const char *fn)
{
  if (binisopen)
    {
      close_filestream(input_file);
      input_file = NULL;
    }

  binisopen = 0;

  input_file = open_filestream(fn);

  if (input_file == NULL)
    {
      CommonExit(-1, "Error: Cannot open input file");
      return -1;
    }

  binisopen = 1;
  return(0);
}

int aac_decode_init(faadAACInfo *fInfo)
{
	int i;
	char header_chk[4];

    bno = 0;  //prkoat: for Winamp plugin.. - if you are playing many files with different channel configurations, the decoder would crap out in the check_mc_info() 

	/* set defaults */
    current_program = -1;
    default_config = 1;
    mc_info.profile = Main_Profile;
    mc_info.sampling_rate_idx = Fs_44;

    stop_now = 0;

    for(i=0; i<Chans; i++)
	{
		coef[i] = (Float *)malloc(LN2*sizeof(*coef[0]));
		data[i] = (Float *)malloc(LN2*sizeof(*data[0]));
		state[i] = (Float *)malloc(LN*sizeof(*state[0]));
		factors[i] = (short *)malloc(MAXBANDS*sizeof(*factors[0]));
		cb_map[i] = (byte *)malloc( MAXBANDS*sizeof(*cb_map[0]));
		group[i] = (byte *)malloc( NSHORT*sizeof(group[0]));
		lpflag[i] = (int *)malloc( MAXBANDS*sizeof(*lpflag[0]));
		prstflag[i] = (int *)malloc( (LEN_PRED_RSTGRP+1)*sizeof(*prstflag[0]));
		tns[i] = (TNS_frame_info *)malloc( sizeof(*tns[0]));
		nok_lt_status[i]  = (NOK_LT_PRED_STATUS *)malloc(sizeof(*nok_lt_status[0]));
		nok_lt_status[i]->delay =  (int*)malloc(MAX_SHORT_WINDOWS*sizeof(int));

		memset(coef[i],0,LN2*sizeof(*coef[0]));
		memset(data[i],0,LN2*sizeof(*data[0]));
		memset(state[i],0,LN*sizeof(*state[0]));
		memset(factors[i],0,MAXBANDS*sizeof(*factors[0]));
		memset(cb_map[i],0,MAXBANDS*sizeof(*cb_map[0]));
		memset(group[i],0,NSHORT*sizeof(group[0]));
		memset(lpflag[i],0,MAXBANDS*sizeof(*lpflag[0]));
		memset(prstflag[i],0,(LEN_PRED_RSTGRP+1)*sizeof(*prstflag[0]));
		memset(tns[i],0,sizeof(*tns[0]));
	}
    for(i=0; i<Winds; i++) {
		mask[i] = (byte *)malloc( MAXBANDS*sizeof(mask[0]));
		memset(mask[i],0,MAXBANDS*sizeof(mask[0]));
    }
#if (CChans > 0)
    for(i=0; i<CChans; i++){
		cc_coef[i] = (Float *)malloc( LN2*sizeof(*cc_coef[0]));
		for(j=0; j<Chans; j++)
			cc_gain[i][j] = (Float *)malloc( MAXBANDS*sizeof(*cc_gain[0][0]));
#if (ICChans > 0)
		if (i < ICChans) {
			cc_state[i] = (Float *)malloc( LN*sizeof(*state[0]));  /* changed LN4 to LN 1/97 mfd */
		}
#endif
    }
#endif

      peek_buffer_filestream(input_file, header_chk, 4);

	/* Check if an ADIF header is present */
	if (strncmp(header_chk, "ADIF", 4) == 0)
		adif_header_present = 1;
	else
		adif_header_present = 0;

	/* Check if an ADTS header is present */
	if (adif_header_present == 0)
	{
		if (((int) ( header_chk[0] == (char) 0xFF)) &&
			((int) ( (header_chk[1] & (char) 0xF0) == (char) 0xF0)))
		{
			adts_header_present = 1;
			adts_header.first = 0;
		}
	else
		{
			adts_header_present = 0;
		}
	}

	reset_bits();
	startblock();

	fInfo->bit_rate = 128000;
	fInfo->sampling_rate = 44100;
	fInfo->channels = 2;

	if (adif_header_present)
	{
		fInfo->bit_rate = adif_header.bitrate;
		fInfo->sampling_rate = SampleRates[prog_config.sampling_rate_idx];
	}

	fInfo->length = (int)((filelength_filestream(input_file)/(((fInfo->bit_rate*8)/1000)*16))*1000);

	do_init();

	return 0;
}

void aac_decode_free()
{
	int i;

	stop_now = 1;

	if (binisopen)
	{
		if(input_file)
			close_filestream(input_file);

		input_file = NULL;
		binisopen = 0;
	}

    for(i=0; i < Chans; i++)
	{
		if (coef[i]) free(coef[i]);
		if (data[i]) free(data[i]);
		if (state[i]) free(state[i]);
		if (factors[i]) free(factors[i]);
		if (cb_map[i]) free(cb_map[i]);
		if (group[i]) free(group[i]);
		if (lpflag[i]) free(lpflag[i]);
		if (prstflag[i]) free(prstflag[i]);
		if (tns[i]) free(tns[i]);
		if (nok_lt_status[i]) free(nok_lt_status[i]);
    }

    for(i=0; i<Winds; i++)
	{
		if (mask[i]) free(mask[i]);
    }

#if (CChans > 0)
    for(i=0; i<CChans; i++)
	{
		if (cc_coef[i]) free(cc_coef[i]);
		
		for(j=0; j<Chans; j++)
			if (cc_gain[i][j]) free(cc_gain[i][j]);
#if (ICChans > 0)
		if (i < ICChans)
		{
			if (cc_state[i]) free(cc_state[i]);
		}
#endif
    }
#endif
}

int aac_decode_frame(short *sample_buffer)
{
	int i, j,  ch, wn, ele_id;
	int left, right;

	framebits = 0;

	if (adts_header_present)
	{
		if (get_adts_header())
		{
		  adts_header.first = 0;
		  stop_now = 1;
		  return -2;
		}
		adts_header_present = 1;
	}

#ifdef TEST_OUTPUT
	fprintf(stdout, "FR: %5.5d ", bno);
#endif

	reset_mc_info(mip);
	while ((ele_id=getbits(LEN_SE_ID)) != ID_END)
	{
		/* get audio syntactic element */
		switch (ele_id) {
		case ID_SCE:		/* single channel */
		case ID_CPE:		/* channel pair */
		case ID_LFE:		/* low freq effects channel */
			if (huffdecode(ele_id, mip, wnd, wnd_shape,
				cb_map, factors, 
				group, hasmask, mask, max_sfb,
				lpflag, prstflag
				, nok_lt_status
				, tns, coef) < 0)
				return -3;
				//CommonExit(1,"2022: Error in huffman decoder");
			break;
#if (CChans > 0)
		case ID_CCE:		/* coupling channel */
			if (getcc(mip, cc_wnd, cc_wnd_shape, cc_coef, cc_gain) < 0)
				return 0;
//				CommonExit(1,"2023: Error in coupling channel");
			break;
#endif				
		case ID_PCE:		/* program config element */
			get_prog_config(&prog_config);
			break;
		case ID_FIL:		/* fill element */
			getfill();
			break;
		default:
//			CommonWarning("Element not supported");
			return -4;
		}
	}
	check_mc_info(mip, (bno==0 && default_config));

#if (ICChans > 0)
	/* transform independently switched coupling channels */
	ind_coupling(mip, wnd, wnd_shape, cc_wnd, cc_wnd_shape, cc_coef,
		cc_state);
#endif	

	/* m/s stereo */
	for (ch=0; ch<Chans; ch++) {
		cip = &mip->ch_info[ch];
		if ((cip->present) && (cip->cpe) && (cip->ch_is_left)) {
			wn = cip->widx;
			if(hasmask[wn]) {
				left = ch;
				right = cip->paired_ch;
				info = winmap[wnd[wn]];
				if (hasmask[wn] == 1)
					map_mask(info, group[wn], mask[wn], cb_map[right]);
				synt(info, group[wn], mask[wn], coef[right], coef[left]);
			}
		}
	}

	/* intensity stereo and prediction */
	for (ch=0; ch<Chans; ch++) {
		if (!(mip->ch_info[ch].present)) continue;
		wn = mip->ch_info[ch].widx;
		info = winmap[wnd[wn]];
		pns( mip, info, wn, ch,
			group[wn], cb_map[ch], factors[ch], 
			lpflag[wn], coef );
		intensity(mip, info, wn, ch, 
			group[wn], cb_map[ch], factors[ch], 
			lpflag[wn], coef);
		nok_lt_predict (mip->profile,
				info,
				(WINDOW_TYPE)wnd[ch], &wnd_shape[ch],
			nok_lt_status[ch]->sbk_prediction_used,
			nok_lt_status[ch]->sfb_prediction_used,
			nok_lt_status[ch], nok_lt_status[ch]->weight,
			nok_lt_status[ch]->delay, coef[ch],
			BLOCK_LEN_LONG, 0, BLOCK_LEN_SHORT);
	}

	for (ch=0; ch<Chans; ch++) {
		if (!(mip->ch_info[ch].present)) continue;
		wn = mip->ch_info[ch].widx;
		info = winmap[wnd[wn]];

#if (CChans > 0)
		/* if cc_domain indicates before TNS */
		coupling(info, mip, coef, cc_coef, cc_gain, ch, CC_DOM, !CC_IND);
#endif

		/* tns */
		for (i=j=0; i<tns[ch]->n_subblocks; i++) {

			tns_decode_subblock(&coef[ch][j],
				max_sfb[wn],
				info->sbk_sfb_top[i],
				info->islong,
				&(tns[ch]->info[i]) );

			j += info->bins_per_sbk[i];
		}

#if (CChans > 0)
		/* if cc_domain indicated after TNS */
		coupling(info, mip, coef, cc_coef, cc_gain, ch, CC_DOM, !CC_IND);
#endif

#ifdef TEST_OUTPUT
		fprintf(stdout, "BLCK: %d SHP: %s ", wnd[wn], wnd_shape[wn].this_bk ? "KBD":"SIN");
#endif

		/* inverse transform */
		freq2time_adapt (wnd [wn], &wnd_shape [wn], coef [ch], state [ch], data [ch]);

#if (CChans > 0)
		/* independently switched coupling */
		coupling(info, mip, coef, cc_coef, cc_gain, ch, CC_DOM, CC_IND);
#endif
	}

	/* Copy output to a standard PCM buffer */

	for(i=0; i < 1024; i++) //prkoat
		for (ch=0; ch < mip->nch; ch++)
			sample_buffer[(i*mip->nch)+ch] = double_to_int(data[ch][i]);

	bno++;

	if (stop_now)
		return -5;

	byte_align();

	return framebits;
}

int aac_get_sample_rate (void)
{
  if (adts_header_present) {
    return (SampleRates[adts_header.fixed.sampling_rate_idx]);
  }
  if (adif_header_present) {
    return (SampleRates[prog_config.sampling_rate_idx]);
  }
  return (0);
}
int aac_get_channels (void)
{
  return (mip->nch);
}

#ifndef PLUGIN
void CommonWarning(char *message)
{
  printf("warning: %s\n", message);
}

void CommonExit(int errorcode, char *message)
{
  printf("%s\n", message);
  /* exit(errorcode); */
}
#endif

