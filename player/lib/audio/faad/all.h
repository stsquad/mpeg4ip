/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS and edited by
Yoshiaki Oikawa (Sony Corporation),
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

#ifndef	_all_h_
#define _all_h_

#include "mpeg4ip.h"
#include <math.h>
#include "interface.h"
#include "tns.h"
#include "nok_ltp_common.h"
#include "monopred.h"
#include "bits.h"
#ifndef _POSIX_SOURCE
#define                 _POSIX_SOURCE   /* stops repeat typdef of ulong */
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef	float Float;
typedef	unsigned char	byte;


enum
{
    /*
     * channels for 5.1 main profile configuration 
     * (modify for any desired decoder configuration)
     */
#ifdef BIGDECODER
    FChans	= 15,	/* front channels: left, center, right */
    FCenter	= 1,	/* 1 if decoder has front center channel */
    SChans	= 18,	/* side channels: */
    BChans	= 15,	/* back channels: left surround, right surround */
    BCenter	= 1,	/* 1 if decoder has back center channel */
    LChans	= 1,	/* LFE channels */
    XChans	= 1,	/* scratch space for parsing unused channels */  
#else
    FChans	= 3,	/* front channels: left, center, right */
    FCenter	= 0,	/* 1 if decoder has front center channel */
    SChans	= 2,	/* side channels: */
    BChans	= 1,	/* back channels: left surround, right surround */
    BCenter	= 0,	/* 1 if decoder has back center channel */
    LChans	= 1,	/* LFE channels */
    XChans	= 1,	/* scratch space for parsing unused channels */  
#endif
    
    Chans	= FChans + SChans + BChans + LChans + XChans
};

/* #define is required in order to use these args in #if () directive */
#if 0
#define ICChans	1	/* independently switched coupling channels */
#define DCChans	2	/* dependently switched coupling channels */
#define XCChans	1	/* scratch space for parsing unused coupling channels */
#define CChans	(ICChans + DCChans + XCChans)
#else
#define ICChans	0
#define DCChans	0
#define XCChans	0
#define CChans	0
#endif

enum
{
    /* block switch windows for single channels or channel pairs */
    Winds	= Chans,
    
    /* average channel block length, bytes */
    Avjframe	= 341,	

    TEXP	= 128, /* size of exp cache table */
    MAX_IQ_TBL	= 8192+15, /* size of inv quant table */
    MAXFFT	= LN4,

    XXXXX
};


typedef struct
{
    int	    islong;			/* true if long block */
    int	    nsbk;			/* sub-blocks (SB) per block */
    int	    bins_per_bk;		/* coef's per block */
    int	    sfb_per_bk;			/* sfb per block */
    int	    bins_per_sbk[MAX_SBK];	/* coef's per SB */
    int	    sfb_per_sbk[MAX_SBK];	/* sfb per SB */
    int	    sectbits[MAX_SBK];
    int   *sbk_sfb_top[MAX_SBK];	/* top coef per sfb per SB */
    int   *sfb_width_128;		/* sfb width for short blocks */
    int   bk_sfb_top[200];		/* cum version of above */
    int	    num_groups;
    int   group_len[8];
    int   group_offs[8];
} Info;

typedef struct {
    int	samp_rate;
    int	nsfb1024;
    int *SFbands1024;
    int	nsfb128;
    int *SFbands128;
} SR_Info;

typedef struct
{
    byte    this_bk;
    byte    prev_bk;
} Wnd_Shape;

typedef struct
{
    int len;
    unsigned long cw;
	char x, y, v, w;
} Huffman;

typedef struct
{
    int len;
    unsigned long cw;
	int scl;
} Huffscl;

typedef	struct
{
	int dim;
    int signed_cb;
    Huffman *hcw;
} Hcb;


typedef struct
{
    int present;	/* channel present */
    int tag;		/* element tag */
    int cpe;		/* 0 if single channel, 1 if channel pair */
    int	common_window;	/* 1 if common window for cpe */
    int	ch_is_left;	/* 1 if left channel of cpe */
    int	paired_ch;	/* index of paired channel in cpe */
    int widx;		/* window element index for this channel */
    int is_present;	/* intensity stereo is used */
    int ncch;		/* number of coupling channels for this ch */
    char *fext;		/* filename extension */
} Ch_Info;

typedef struct {
    int nch;		/* total number of audio channels */
    int nfsce;		/* number of front SCE's pror to first front CPE */
    int nfch;		/* number of front channels */
    int nsch;		/* number of side channels */
    int nbch;		/* number of back channels */
    int nlch;		/* number of lfe channels */
    int ncch;		/* number of valid coupling channels */
    int cch_tag[(1<<LEN_TAG)];	/* tags of valid CCE's */
    int object_type;
    int sampling_rate_idx;
    Ch_Info ch_info[Chans];
} MC_Info;

typedef struct {
    int num_ele;
    int ele_is_cpe[(1<<LEN_TAG)];
    int ele_tag[(1<<LEN_TAG)];
} EleList;

typedef struct {
    int present;
    int ele_tag;
    int pseudo_enab;
} MIXdown;

typedef struct {
    int object_type;
    int sampling_rate_idx;
    EleList front;
    EleList side;
    EleList back;
    EleList lfe;
    EleList data;
    EleList coupling;
    MIXdown mono_mix;
    MIXdown stereo_mix;
    MIXdown matrix_mix;
    char comments[(1<<LEN_PC_COMM)+1];
    long    buffer_fullness;	/* put this transport level info here */
} ProgConfig;

typedef struct {
    char    adif_id[LEN_ADIF_ID+1];
    int	    copy_id_present;
    char    copy_id[LEN_COPYRT_ID+1];
    int	    original_copy;
    int	    home;
    int	    bitstream_type;
    long    bitrate;
    int	    num_pce;
    int	    prog_tags[(1<<LEN_TAG)];
} ADIF_Header;

typedef struct {
    int	copy_id_bit;
    int copy_id_start;
	int frame_length;
	int buffer_fullness;
	int raw_blocks;
} ADTS_Variable;

typedef struct {
	int ID;
	int layer;
	int protection_absent;
	int object_type;
	int sampling_rate_idx;
	int private_bit;
	int channel_configuration;
	int original_copy;
	int home;
	int emphasis;
} ADTS_Fixed;

typedef struct {
	ADTS_Fixed fixed;
	ADTS_Variable variable;
	int adts_error_check;
} ADTS_Header;

struct Pulse_Info
{
    int pulse_data_present;
    int number_pulse;
    int pulse_start_sfb;
    int pulse_position[NUM_PULSE_LINES];
    int pulse_offset[NUM_PULSE_LINES];
    int pulse_amp[NUM_PULSE_LINES];
};


extern Huffman book1[];
extern Huffman book2[];
extern Huffman book3[];
extern Huffman book4[];
extern Huffman book5[];
extern Huffman book6[];
extern Huffman book7[];
extern Huffman book8[];
extern Huffman book9[];
extern Huffman book10[];
extern Huffman book11[];
extern Huffscl bookscl[];
extern Hcb book[NSPECBOOKS+2];
extern int sfbwidth128[];
extern SR_Info samp_rate_info[];
extern int tns_max_bands_tbl[(1<<LEN_SAMP_IDX)][4];
extern const int SampleRates[];
extern int pred_max_bands_tbl[(1<<LEN_SAMP_IDX)];

#ifdef _WIN32
  #pragma pack(push, 8)
  #ifndef FAADAPI
    #define FAADAPI __stdcall
  #endif
#else
  #ifndef FAADAPI
    #define FAADAPI
  #endif
#endif

#define FAAD_OK 0
#define FAAD_OK_CHUPDATE 1
#define FAAD_ERROR 2
#define FAAD_FATAL_ERROR 3

typedef void *faacProgConfig;

typedef struct faacDecConfiguration
{
  unsigned int defObjectType;
  unsigned int defSampleRate;
} faacDecConfiguration, *faacDecConfigurationPtr;
typedef enum {
    WS_FHG, WS_DOLBY, N_WINDOW_SHAPES
}
Window_shape;

typedef enum {
    WT_LONG,
    WT_SHORT,
    WT_FLAT,
    WT_ADV,         /* Advanced flat window */
    N_WINDOW_TYPES
}
WINDOW_TYPE;

typedef struct {
	int isMpeg4;
	int frameNum;
	int pceChannels;
	int numChannels;
  int chans_inited;

	/* Configuration data */
	faacDecConfiguration config;

	bitfile ld;

	int adif_header_present;
	int adts_header_present;
	ADIF_Header adif_header;
	ADTS_Header adts_header;

	/* decoder data */
	Float *coef[Chans];
	Float *data[Chans];
	Float *state[Chans];
	byte hasmask[Winds];
	byte *mask[Winds];
	byte *group[Chans];
	byte wnd[Chans];
	byte max_sfb[Chans];
	byte *cb_map[Chans];
    int *lpflag[Chans];
	int *prstflag[Chans];

	/* prediction */
	int last_rstgrp_num[Chans];
	PRED_STATUS	*sp_status[Chans];
	float *mnt_table;
	float *exp_table;
	int warn_flag;

	/* long term prediction */
	NOK_LT_PRED_STATUS *nok_lt_status[Chans];
	/* Pulse coding */
	struct Pulse_Info pulse_info;

	MC_Info mc_info;
	MC_Info save_mc_info;
	int default_config;
	int current_program;
	ProgConfig prog_config;
	Info eight_short_info;
	Info *win_seq_info[NUM_WIN_SEQ];
	Info *winmap[NUM_WIN_SEQ];
	Info only_long_info;

	Wnd_Shape wnd_shape[Chans];
	int *factors[Chans];
	TNS_frame_info *tns[Chans];

	int dolbyShortOffset_f2t;
	int dolbyShortOffset_t2f;
	int first_cpe;

	/* PNS data */
    long cur_noise_state;
    long noise_state_save[MAXBANDS];
    int lp_store[MAXBANDS];

	/* tables */
	Float *iq_exp_tbl;
	Float *exptable;

	/* FFT data */
	int *unscambled64;
	int *unscambled512;
  // block.c data
  Float *sin_long;
  Float *sin_short;
#ifndef WIN_TABLE
  Float *kbd_long;
  Float *kbd_short;
#endif
  Float *sin_edler;
  Float *kbd_edler;
  Float *sin_adv;
  Float *kbd_adv;

Float *windowPtr[N_WINDOW_TYPES][N_WINDOW_SHAPES];

} faacDecStruct, *faacDecHandle;

faacDecHandle FAADAPI faacDecOpen(void);

faacDecConfigurationPtr FAADAPI faacDecGetCurrentConfiguration(faacDecHandle hDecoder);

int FAADAPI faacDecSetConfiguration(faacDecHandle hDecoder,
									faacDecConfigurationPtr config);

int FAADAPI faacDecInit(faacDecHandle hDecoder,
			unsigned char *buffer,
			unsigned long *samplerate,
			unsigned long *channels);

int FAADAPI faacDecGetProgConfig(faacDecHandle hDecoder,
				 faacProgConfig *progConfig);

int FAADAPI faacDecDecode(faacDecHandle hDecoder,
			  unsigned char *buffer,
			  unsigned long *bytesconsumed,
			  short *sample_buffer,
			  unsigned long *samples);

void FAADAPI faacDecClose(faacDecHandle hDecoder);


#include "nok_lt_prediction.h"
#include "port.h"

#ifdef __cplusplus
}
#endif
#endif	/* _all_h_ */
