#ifndef __IFO_H__
#define __IFO_H__

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

#define OFFSET_IFO		0x0000
#define OFFSET_VTS		0x0000
#define OFFSET_LEN		0x00C0
#define IFO_OFFSET_TAT		0x00C0
#define OFFSET_VTSI_MAT		0x0080
#define IFO_OFFSET_VIDEO	0x0100
#define IFO_OFFSET_AUDIO	0x0200
#define IFO_OFFSET_SUBPIC	0x0250


// for debug and error output

/**
 * Video Info Table 
 */

typedef struct {
#if BYTE_ORDER == BIG_ENDIAN
	u_char compression      : 2;
	u_char system           : 2;
	u_char ratio            : 2;
	u_char perm_displ       : 2;

	u_char line21_1         : 1;
	u_char line21_2         : 1;
	u_char source_res       : 2;
	u_char letterboxed      : 1;
	u_char mode             : 1;
#else
	u_char perm_displ       : 2;
	u_char ratio            : 2;
	u_char system           : 2;
	u_char compression      : 2;

	u_char mode             : 1;
	u_char letterboxed      : 1;
	u_char source_res       : 2;
	u_char line21_2         : 1;
	u_char line21_1         : 1;
#endif
} ifo_video_info_t;

/**
 * Audio Table
 */

typedef struct {
#if MPEG3_LITTLE_ENDIAN
	u_char num_channels	: 3;    // number of channels (n+1)
	u_char sample_freq	: 2;    // sampling frequency
	u_char quantization	: 2;    // quantization
	u_char appl_mode2	: 1;    // audio application mode

	u_char appl_mode1	: 1;    // 
	u_char type		: 2;    // audio type (language included?)
	u_char multichannel_extension: 1;
	u_char coding_mode	: 2;
#else
	u_char appl_mode2	: 1;
	u_char quantization	: 2;
	u_char sample_freq	: 2;
	u_char num_channels	: 3;

	u_char coding_mode	: 2;
	u_char multichannel_extension: 1;
	u_char type		: 2;
	u_char appl_mode1	: 1;
#endif
	u_short lang_code	: 16;   // <char> description
	u_int   foo		: 8;    // 0x00000000 ?
	u_int   caption		: 8;
	u_int   bar		: 8;    // 0x00000000 ?
} ifo_audio_t;

#define IFO_AUDIO_LEN 7

/**
 * Subpicture Table
 */

typedef struct {
	u_short prefix  : 16;           // 0x0100 ?
	u_short lang_code : 16;         // <char> description
	u_char foo      : 8;            // dont know
	u_char caption  : 8;            // 0x00 ?
} ifo_spu_t;


/**
 * Time Map Table header entry
 */

#if 0
typedef struct {
	u_char  tu		: 16;    // time unit (in seconds)
	u_int			: 16;   // don't know
} ifo_tmt_hdr_t;
#endif

typedef struct {
	u_int			: 24;   // don't know
	u_char  tu		: 8;    // time unit (in seconds)
} ifoq_tmt_hdr_t;

//#define IFO_TMT_HDR_LEN 4
#define IFOQ_TMT_HDR_LEN 1


/**
 * hmm
 */

typedef struct {
        u_short vob_id          : 16;   // Video Object Identifier
        u_char  cell_id         : 8;    // Cell Identifier
        u_char                  : 8;    // don't know
        u_int   start           : 32;   // Cell start
        u_int   end             : 32;   // Cell end
} ifo_cell_addr_t;


typedef struct {
	u_short vob_id		: 16;	// Video Object Identifier
	u_short cell_id		: 16;	// Cell Identifier
} ifo_pgc_cell_pos_t;

/**
 * Part of Title AND Title set Cell Address
 */

typedef struct {
	u_short pgc;		// Program Chain (PTT)
	u_short pg;		// Program (PTT)
	u_long	start;		// Start of VOBU (VTS? CADDR)
	u_long	end;		// End of VOBU (VTS? CADDR)
} ifo_ptt_data_t;

typedef struct {
	u_int num;		// Number of Chapters
	ifo_ptt_data_t *data;	// Data
} ifo_ptt_sub_t;

typedef struct {
	u_int num;		// Number of Titles
	ifo_ptt_sub_t *title;	// Titles
} ifo_ptt_t;

typedef struct {
	u_char	chain_info	: 8; // 0x5e 0xde(2 angles, no overlay), 0x5f 0x9f 0x9f 0xdf(4 angles overlay), 0x2 0xa 0x8(1 angle)
	u_char	foo		: 8; // parent control ??
	u_char	still_time	: 8;
	u_char	cell_cmd	: 8;
        //u_int   foo             : 32;
        u_int   len_time        : 32;
        u_int   vobu_start      : 32;   // 1st vobu start
        u_int   ilvu_end        : 32;
        u_int   vobu_last_start : 32;
        u_int   vobu_last_end   : 32;
} ifo_pgci_cell_addr_t;

#define PGCI_CELL_ADDR_LEN 24

#define ID_NUM_MENU_VOBS 0
#define ID_NUM_TITLE_VOBS 1

#define ID_MAT			0
#define ID_PTT			1
#define ID_TSP			1
#define ID_TITLE_PGCI		2
#define ID_MENU_PGCI		3
#define ID_TMT			4
#define ID_MENU_CELL_ADDR	5
#define ID_MENU_VOBU_ADDR_MAP	6
#define ID_TITLE_CELL_ADDR	7
#define ID_TITLE_VOBU_ADDR_MAP 	8


/**
 * Information Table - for internal use only
 */
 
typedef struct {
	u_int num_menu_vobs;
	u_int vob_start;

	u_char *data[10];
	
	int fd;		// file descriptor
	__off64_t pos;	// offset of ifo file on device 
} ifo_t;


/**
 * Generic header
 */

#define IFO_HDR_LEN 8
#define IFOQ_HDR_LEN 2

typedef struct {
        u_short num     : 16;   // number of entries
        u_short         : 16;   // don't known (reserved?)
        u_int   len     : 32;   // length of table
} ifo_hdr_t;

typedef struct {
        u_short         : 16;   // don't known (reserved?)
        u_short num     : 16;   // number of entries
        u_int   len     : 32;   // length of table
} ifoq_hdr_t;


/**
 * Prototypes
 */

ifo_t *ifoOpen (int fd, __off64_t pos);
int ifoClose (ifo_t *ifo);

u_int ifoGetVOBStart	(ifo_t *ifo);
int ifoGetNumberOfTitles (ifo_t *ifo);
int ifoGetNumberOfParts (ifo_t *ifo);

int ifoGetVMGPTT	(ifo_hdr_t *hdr, char **ptr);
int ifoGetPGCI		(ifo_hdr_t *hdr, int title, char **ptr);
int ifoGetCLUT		(char *pgc, char **ptr);
u_int ifoGetCellPlayInfo	(u_char *pgc, u_char **ptr);
u_int ifoGetCellPos	(u_char *pgc, u_char **ptr);
int ifoGetProgramMap	(char *pgc, char **ptr);
int ifoGetCellAddr	(char *cell_addr, char **ptr);
int ifoGetCellAddrNum	(char *hdr);

int ifoGetAudio		(char *hdr, char **ptr);
int ifoGetSPU		(char *hdr, char **ptr);

ifo_ptt_t *ifo_get_ptt (ifo_t *ifo);
int ifo_get_num_title_pgci (ifo_t *ifo);
u_char *ifo_get_ptr_title_pgci (ifo_t *ifo, int index);

char *ifoDecodeLang (u_short descr);

int ifoIsVTS (ifo_t *ifo);
int ifoIsVMG (ifo_t *ifo);

void ifoPrintVideo		(u_char *ptr);

void ifoPrintCellPlayInfo	(u_char *ptr, u_int num);
void ifoPrintCellInfo		(u_char *ptr, u_int num);
void ifoPrintCellPos		(u_char *ptr, u_int num);
void ifoPrintCLUT		(u_char *ptr); 
void ifoPrintProgramMap		(u_char *ptr, u_int num);

#ifdef PARSER
void ifoPrintAudio		(ifo_audio_t *ptr, u_int num);
void ifoPrintSPU		(ifo_spu_t *ptr, u_int num);
void ifoPrintTMT		(ifo_t *ifo);

void ifoPrintVMOP		(u_char *opcode);

void ifoPrint_ptt (ifo_ptt_t *ptt);
void ifoPrint_vts_vobu_addr_map (ifo_t *ifo);
void ifoPrint_vtsm_vobu_addr_map (ifo_t *ifo);
void ifoPrint_vts_cell_addr (ifo_t *ifo);
void ifoPrint_vtsm_cell_addr (ifo_t *ifo);
void ifoPrint_title_pgci (ifo_t *ifo);
void ifoPrint_pgc_cmd (u_char *pgc_ptr);
void ifoPrintTSP (u_char *toast);
void ifoPrint_pgc (u_char *ptr);
#endif
#endif

