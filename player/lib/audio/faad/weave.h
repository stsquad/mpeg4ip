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


#if !defined(weave_h)
#define weave_h

/*
*	Interleave Definitions for start and stop blocks
*
*	Start block contains 1 576-pt spectrum (A) and 4 128-pt spectra (B-E)
*	  Input spectra are interleaved in repeating segements of 17 bins,
*		9 bins from A (A0-A8), and 2 bins from each of the shorts.
*	  Within the segments the bins are interleaved as:
*		A0 A1 A2 A3 A4 B0 C0 D0 E0 A5 A6 A7 A8 B1 C1 D1 E1
*
*	Stop block contains 3 128-pt spectra (A-C) and 1 576-pt spectrum (D)
*	  Input spectra are interleaved in repeating segements of 15 bins,
*		2 bins from each of the shorts, and 9 bins from D (D0-D8).
*	  Within the segments the bins are interleaved as:
*		A0 B0 C0 D0 D1 D2 D3 D4 A1 B1 C1 D5 D6 D7 D8
*	  The last 64 bins are (should be) set to 0.
*/
#define	START_OFFSET 0
#define	SHORT_IN_START_OFFSET 5
#define	STOP_OFFSET 3
#define SHORT_IN_STOP_OFFSET 0

#define WEAVE_START	0		/* type parameter for start blocks */
#define WEAVE_STOP	1		/* type paremeter for stop blocks */


void unfold (Float *data_in, Float *data_out, int inLeng);

#endif	/* weave_h */
