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
 * $Id: interface.h,v 1.6 2002/01/11 00:55:17 wmaycisco Exp $
 */

#ifndef _interface_h_
#define _interface_h_

/*
 * interface between the encoder and decoder
 */

#define C_LN10      2.30258509299404568402      /* ln(10) */
#define C_PI        3.14159265358979323846f     /* pi */
#define C_SQRT2     1.41421356237309504880      /* sqrt(2) */

#define MINTHR      .5
#define SF_C1       (13.33333/1.333333)

/* prediction */
#define PRED_ORDER  2
#define PRED_ALPHA  0.90625f
#define PRED_A      0.953125f
#define PRED_B      0.953125f

enum
{
    /*
     * block switching
     */
    LN          = 2048,
    SN          = 256,
    LN2         = LN/2,
    SN2         = SN/2,
    LN4         = LN/4,
    SN4         = SN/4,
    NSHORT      = LN/SN,
    MAX_SBK     = NSHORT,

    ONLY_LONG_WINDOW    = 0,
    LONG_START_WINDOW,
    EIGHT_SHORT_WINDOW,
    LONG_STOP_WINDOW,
    NUM_WIN_SEQ,

    WLONG       = ONLY_LONG_WINDOW,
    WSTART,
    WSHORT,
    WSTOP,

    MAXBANDS        = 16*NSHORT,    /* max number of scale factor bands */
    MAXFAC      = 121,      /* maximum scale factor */
    MIDFAC      = (MAXFAC-1)/2,
    SF_OFFSET       = 100,      /* global gain must be positive */

    /*
     * specify huffman tables as signed (1) or unsigned (0)
     */
    HUF1SGN     = 1,
    HUF2SGN     = 1,
    HUF3SGN     = 0,
    HUF4SGN     = 0,
    HUF5SGN     = 1,
    HUF6SGN     = 1,
    HUF7SGN     = 0,
    HUF8SGN     = 0,
    HUF9SGN     = 0,
    HUF10SGN        = 0,
    HUF11SGN        = 0,

    ZERO_HCB        = 0,
    BY4BOOKS        = 4,
    ESCBOOK     = 11,
    NSPECBOOKS      = ESCBOOK + 1,
    BOOKSCL     = NSPECBOOKS,
    NBOOKS      = NSPECBOOKS+1,
    INTENSITY_HCB2  = 14,
    INTENSITY_HCB   = 15,
    NOISE_HCB       = 13,

    NOISE_PCM_BITS      = 9,
    NOISE_PCM_OFFSET    = (1 << (NOISE_PCM_BITS-1)),

    NOISE_OFFSET        = 90,

    LONG_SECT_BITS  = 5,
    SHORT_SECT_BITS = 3,

    /*
     * Program Configuration
     */
    AACMAIN = 0,
    AACLC = 1,
    AACSSR = 2,
    AACLTP = 3,

    Fs_48       = 3,
    Fs_44       = 4,
    Fs_32       = 5,

    /*
     * Misc constants
     */
    CC_DOM      = 0,    /* before TNS */
    CC_IND      = 1,

    /*
     * Raw bitstream constants
     */
    LEN_SE_ID       = 3,
    LEN_TAG     = 4,
    LEN_COM_WIN     = 1,
    LEN_ICS_RESERV  = 1,
    LEN_WIN_SEQ     = 2,
    LEN_WIN_SH      = 1,
    LEN_MAX_SFBL    = 6,
    LEN_MAX_SFBS    = 4,
    LEN_CB      = 4,
    LEN_SCL_PCM     = 8,
    LEN_PRED_PRES   = 1,
    LEN_PRED_RST    = 1,
    LEN_PRED_RSTGRP = 5,
    LEN_PRED_ENAB   = 1,
    LEN_MASK_PRES   = 2,
    LEN_MASK        = 1,
    LEN_PULSE_PRES  = 1,
    LEN_TNS_PRES    = 1,
    LEN_GAIN_PRES   = 1,

    LEN_NPULSE  = 2,
    LEN_PULSE_ST_SFB    = 6,
    LEN_POFF    = 5,
    LEN_PAMP    = 4,
    NUM_PULSE_LINES = 4,
    PULSE_OFFSET_AMP    = 4,

    LEN_IND_CCE_FLG = 1,
    LEN_NCC     = 3,
    LEN_IS_CPE      = 1,
    LEN_CC_LR       = 1,
    LEN_CC_DOM      = 1,
    LEN_CC_SGN      = 1,
    LEN_CCH_GES     = 2,
    LEN_CCH_CGP     = 1,

    LEN_D_ALIGN     = 1,
    LEN_D_CNT       = 8,
    LEN_D_ESC       = 8,
    LEN_F_CNT       = 4,
    LEN_F_ESC       = 8,
    LEN_NIBBLE      = 4,
    LEN_BYTE        = 8,
    LEN_PAD_DATA    = 8,
    MAX_DBYTES  = ((1<<LEN_D_CNT) + (1<<LEN_D_ESC)),

    LEN_PC_COMM     = 8,

    /* FILL */
    LEN_EX_TYPE     = 4,    /* don't use 0x0 or 0xA as extension type! */
    EX_FILL     = 0,
    EX_FILL_DATA    = 1,
    EX_DRC      = 11,

    /* sfb 40, coef 672, pred bw of 15.75 kHz at 48 kHz
     * this is also the highest number of bins used
     * by predictor for any sampling rate
     */
    MAX_PRED_SFB    = 40,   /* 48 kHz only, now obsolete */
    MAX_PRED_BINS   = 672,

    ID_SCE      = 0,
    ID_CPE,
    ID_CCE,
    ID_LFE,
    ID_DSE,
    ID_PCE,
    ID_FIL,
    ID_END,

    /* PLL's don't like idle channels! */
    FILL_VALUE      = 0x55,

    /*
     * program configuration element
     */
    LEN_OBJECTTYPE = 2,
    LEN_SAMP_IDX = 4,
    LEN_NUM_ELE = 4,
    LEN_NUM_LFE = 2,
    LEN_NUM_DAT = 3,
    LEN_NUM_CCE = 4,
    LEN_MIX_PRES = 1,
    LEN_MMIX_IDX = 2,
    LEN_PSUR_ENAB = 1,
    LEN_ELE_IS_CPE = 1,
    LEN_IND_SW_CCE = 1,
    LEN_COMMENT_BYTES = 8,

    /*
     * audio data interchange format header
     */
    LEN_ADIF_ID     = (32/8),
    LEN_COPYRT_PRES = 1,
    LEN_COPYRT_ID   = (72/8),
    LEN_ORIG        = 1,
    LEN_HOME        = 1,
    LEN_BS_TYPE     = 1,
    LEN_BIT_RATE    = 23,
    LEN_NUM_PCE     = 4,
    LEN_ADIF_BF     = 20,
    XXX
};

#endif   /* #ifndef _interface_h_ */
