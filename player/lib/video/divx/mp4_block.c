/**************************************************************************
 *                                                                        *
 * This code has been developed by Andrea Graziani. This software is an   *
 * implementation of a part of one or more MPEG-4 Video tools as          *
 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
 * software module in hardware or software products are advised that its  *
 * use may infringe existing patents or copyrights, and any such use      *
 * would be at such party's own risk.  The original developer of this     *
 * software module and his/her company, and subsequent editors and their  *
 * companies (including Project Mayo), will have no liability for use of  *
 * this software or modifications or derivatives thereof.                 *
 *                                                                        *
 * Project Mayo gives users of the Codec a license to this software       *
 * module or modifications thereof for use in hardware or software        *
 * products claiming conformance to the MPEG-4 Video Standard as          *
 * described in the Open DivX license.                                    *
 *                                                                        *
 * The complete Open DivX license can be found at                         *
 * http://www.projectmayo.com/opendivx/license.php                        *
 *                                                                        *
 **************************************************************************/
/**
*  Copyright (C) 2001 - Project Mayo
 *
 * Andrea Graziani (Ag)
 *
 * DivX Advanced Research Center <darc@projectmayo.com>
*
**/
/// mp4_block.c //

#include <math.h>
#include <stdlib.h>

#include "mp4_decoder.h"
#include "global.h"

#include "mp4_predict.h"
#include "mp4_vld.h"
#include "debug.h"
#include "mp4_block.h"

/**
 *
**/

static int getDCsizeLum(void);
static int getDCsizeChr(void);
static int getDCdiff(int dct_dc_size);

#ifdef NOT_USED
static int getACdir();
#endif
static __inline void clearblock(short * psblock);
static __inline void iquant(short * psblock, int intraFlag);

static void setDCscaler(int block_num) 
{
	int type = (block_num < 4) ? 0 : 1;
	int quant = mp4_hdr.quantizer;

	if (type == 0) {
		if (quant > 0 && quant < 5) 
			mp4_hdr.dc_scaler = 8;
		else if (quant > 4 && quant < 9) 
			mp4_hdr.dc_scaler = (2 * quant);
		else if (quant > 8 && quant < 25) 
			mp4_hdr.dc_scaler = (quant + 8);
		else 
			mp4_hdr.dc_scaler = (2 * quant - 16);
	}
  else {
		if (quant > 0 && quant < 5) 
			mp4_hdr.dc_scaler = 8;
		else if (quant > 4 && quant < 25) 
			mp4_hdr.dc_scaler = ((quant + 13) / 2);
		else 
			mp4_hdr.dc_scaler = (quant - 6);
	}
}

/***/

// Purpose: texture decoding of block_num
int block(int block_num, int coded)
{
	int i;
	int dct_dc_size, dct_dc_diff;
	int intraFlag = ((mp4_hdr.derived_mb_type == INTRA) || 
		(mp4_hdr.derived_mb_type == INTRA_Q)) ? 1 : 0;
	event_t event;

	clearblock(ld->block[block_num]); // clearblock

	if (intraFlag)
	{
		setDCscaler(block_num); // calculate DC scaler

		if (block_num < 4) {
			dct_dc_size = getDCsizeLum();
			if (dct_dc_size != 0) 
				dct_dc_diff = getDCdiff(dct_dc_size);
			else 
				dct_dc_diff = 0;
			if (dct_dc_size > 8)
				getbits1(); // marker bit
		}
		else {
			dct_dc_size = getDCsizeChr();
			if (dct_dc_size != 0)
				dct_dc_diff = getDCdiff(dct_dc_size);
			else 
				dct_dc_diff = 0;
			if (dct_dc_size > 8)
				getbits1(); // marker bit
		}

		ld->block[block_num][0] = (short) dct_dc_diff;
//		_Print("DC diff: %d\n", dct_dc_diff);
	}
	if (intraFlag)
	{
		// dc reconstruction, prediction direction
		dc_recon(block_num, &ld->block[block_num][0]);
	}

	if (coded) 
	{
		unsigned char * zigzag; // zigzag scan dir

		if ((intraFlag) && (mp4_hdr.ac_pred_flag == 1)) {

			if (coeff_pred->predict_dir == TOP)
				zigzag = alternate_horizontal_scan;
			else
				zigzag = alternate_vertical_scan;
		}
		else {
			zigzag = zig_zag_scan;
		}

		i = intraFlag ? 1 : 0;
		do // event vld
		{
			event = vld_event(intraFlag);
/***
			if (event.run == -1)
			{
				printf("Error: invalid vld code\n");
				exit(201);
			}
***/			
			i+= event.run;
			ld->block[block_num][zigzag[i]] = (short) event.level;

//			_Print("Vld Event: Run Level Last %d %d %d\n", event.run, event.level, event.last);

			i++;
		} while (! event.last);
	}

	if (intraFlag)
	{
		// ac reconstruction
		ac_recon(block_num, &ld->block[block_num][0]);
	}

#ifdef _DEBUG_B_ACDC
	if (intraFlag)
	{
		int i;
		_Print("After AcDcRecon:\n");
		_Print("   x ");
		for (i = 1; i < 64; i++) {
			if ((i != 0) && ((i % 8) == 0))
				_Print("\n");
			_Print("%4d ", ld->block[block_num][i]);
		}
		_Print("\n");
	}
#endif // _DEBUG_ACDC

	if (mp4_hdr.quant_type == 0)
	{
		// inverse quantization
		iquant(ld->block[block_num], intraFlag);
	}
	else 
	{
		_Error("Error: MPEG-2 inverse quantization NOT implemented\n");
		exit(110);
	}

#ifdef _DEBUG_B_QUANT
	{
		int i;
		_Print("After IQuant:\n");
		_Print("   x ");
		for (i = 1; i < 64; i++) {
			if ((i != 0) && ((i % 8) == 0))
				_Print("\n");
			_Print("%4d ", ld->block[block_num][i]);
		}
		_Print("\n");
	}
#endif // _DEBUG_B_QUANT

	// inverse dct
	idct(ld->block[block_num]);

	return 1;
}

/***/

int blockIntra(int block_num, int coded)
{
	int i;
	int dct_dc_size, dct_dc_diff;
	event_t event;

	clearblock(ld->block[block_num]); // clearblock

	// dc coeff
	setDCscaler(block_num); // calculate DC scaler

	if (block_num < 4) {
		dct_dc_size = getDCsizeLum();
		if (dct_dc_size != 0) 
			dct_dc_diff = getDCdiff(dct_dc_size);
		else 
			dct_dc_diff = 0;
		if (dct_dc_size > 8)
			getbits1(); // marker bit
	}
	else {
		dct_dc_size = getDCsizeChr();
		if (dct_dc_size != 0)
			dct_dc_diff = getDCdiff(dct_dc_size);
		else 
			dct_dc_diff = 0;
		if (dct_dc_size > 8)
			getbits1(); // marker bit
	}

	ld->block[block_num][0] = (short) dct_dc_diff;

	// dc reconstruction, prediction direction
	dc_recon(block_num, &ld->block[block_num][0]);

	if (coded) 
	{
		unsigned char * zigzag; // zigzag scan dir

		if (mp4_hdr.ac_pred_flag == 1) {

			if (coeff_pred->predict_dir == TOP)
				zigzag = alternate_horizontal_scan;
			else
				zigzag = alternate_vertical_scan;
		}
		else {
			zigzag = zig_zag_scan;
		}

		i = 1;
		do // event vld
		{
			event = vld_intra_dct();
/***
			if (event.run == -1)
			{
				printf("Error: invalid vld code\n");
				exit(201);
			}
***/			
			i+= event.run;
			ld->block[block_num][zigzag[i]] = (short) event.level;

//			_Print("Vld Event: Run Level Last %d %d %d\n", event.run, event.level, event.last);

			i++;
		} while (! event.last);
	}

	// ac reconstruction
	ac_recon(block_num, &ld->block[block_num][0]);

	if (mp4_hdr.quant_type == 0)
	{
		// inverse quantization
		iquant(ld->block[block_num], 1);
	}
	else 
	{
		_Error("Error: MPEG-2 inverse quantization NOT implemented\n");
		exit(110);
	}

	// inverse dct
	idct(ld->block[block_num]);

	return 1;
}

/***/

int blockInter(int block_num, int coded)
{
	event_t event;
	unsigned char * zigzag = zig_zag_scan; // zigzag scan dir
	int i;

	clearblock(ld->block[block_num]); // clearblock

	if (coded) 
	{
		int q_scale = mp4_hdr.quantizer;
		int q_2scale = q_scale << 1;
		int q_add = (q_scale & 1) ? q_scale : (q_scale - 1);

		i = 0;
		do // event vld
		{
			event = vld_inter_dct();
/***
			if (event.run == -1)
			{
				printf("Error: invalid vld code\n");
				exit(201);
			}
***/			
			i+= event.run;
			if (event.level > 0) {
				ld->block[block_num][zigzag[i]] = (q_2scale * event.level) + q_add;
			}
			else {
				ld->block[block_num][zigzag[i]] = (q_2scale * event.level) - q_add;
			}

//			_Print("Vld Event: Run Level Last %d %d %d\n", event.run, event.level, event.last);

			i++;
		} while (! event.last);
	}

	if (mp4_hdr.quant_type != 0)
	{
		_Error("Error: MPEG-2 inverse quantization NOT implemented\n");
		exit(110);
	}

	// inverse dct
	idct(ld->block[block_num]);

	return 1;
}

/***/

static int getDCsizeLum(void)
{
	int code;

	// [Ag][note] bad code

	if (showbits(11) == 1) {
		flushbits(11);
		return 12;
	}
  if (showbits(10) == 1) {
    flushbits(10);
    return 11;
  }
  if (showbits(9) == 1) {
    flushbits(9);
    return 10;
	}
	if (showbits(8) == 1) {
		flushbits(8);
		return 9;
	}
	if (showbits(7) == 1) {
		flushbits(7);
		return 8;
	}
	if (showbits(6) == 1) {
		flushbits(6);
		return 7;
	}  
	if (showbits(5) == 1) {
		flushbits(5);
		return 6;
	}
	if (showbits(4) == 1) {
		flushbits(4);
		return 5;
	}

	code = showbits(3);

	if (code == 1) {
		flushbits(3);
		return 4;
	} else if (code == 2) {
		flushbits(3);
		return 3;
	} else if (code == 3) {
		flushbits(3);
		return 0;
	}

  code = showbits(2);

  if (code == 2) {
		flushbits(2);
		return 2;
	} else if (code == 3) {
		flushbits(2);
		return 1;
	}     

	return 0;
}

static int getDCsizeChr(void)
{
	// [Ag][note] bad code

	if (showbits(12) == 1) {
		flushbits(12);
		return 12;
	}
	if (showbits(11) == 1) {
		flushbits(11);
		return 11;
	}
	if (showbits(10) == 1) {
		flushbits(10);
		return 10;
	}
	if (showbits(9) == 1) {
		flushbits(9);
		return 9;
	}
	if (showbits(8) == 1) {
		flushbits(8);
		return 8;
	}
	if (showbits(7) == 1) {
		flushbits(7);
		return 7;
	}
	if (showbits(6) == 1) {
		flushbits(6);
		return 6;
	}
	if (showbits(5) == 1) {
		flushbits(5);
		return 5;
	}
	if (showbits(4) == 1) {
		flushbits(4);
		return 4;
	} 
	if (showbits(3) == 1) {
		flushbits(3);
		return 3;
	} 

	return (3 - getbits(2));
}

/***/

static int getDCdiff(int dct_dc_size)
{
	int code = getbits(dct_dc_size);
	int msb = code >> (dct_dc_size - 1);

	if (msb == 0) {
		return (-1 * (code^((int) pow(2.0,(double) dct_dc_size) - 1)));
	}
  else { 
		return code;
	}
}

/***/


/***/

#ifdef WIN32

static void clearblock (short *psblock)
#if 0 // def _DEBUG
{
  int *bp;
  int i;

	bp = (int *) psblock;

  for (i = 0; i < 8; i++)
  {
    bp[0] = bp[1] = bp[2] = bp[3] = 0;
    bp += 4;
  }
}
#else
{
	_asm
	{
		mov		edx, -16 // clear loop counter
		mov		esi, psblock // capture block address

		pxor  mm0, mm0 // mm0 = 0x00
lq1:	
		movq	qword ptr [esi], mm0 // copy output to destination
		add		esi, 8
		inc		edx
		jnz		lq1
		emms
	}
}
#endif // _DEBUG

#else

#ifdef LINUX

static __inline void clearblock(short * psblock)
{
	int i;

	for (i = 0; i < 64; i++) {
		psblock[i] = 0;
	}
}

#endif // LINUX

#endif  //WIN32

/***/
#if NOT_USED
int getACdir()
{
	return coeff_pred->predict_dir;
}
#endif
/***/

static __inline void iquant(short * psblock, int intraFlag)
{
	int i;
	int q_scale = mp4_hdr.quantizer;
	int q_2scale = q_scale << 1;
	int q_add = (q_scale & 1) ? q_scale : (q_scale - 1);

	for (i = intraFlag; i < 64; i++)
	{
		if (psblock[i] != 0) {
			if (psblock[i] > 0)
			{
				psblock[i] = (q_2scale * psblock[i]) + q_add;
			}
			else if (psblock[i] < 0)
			{
				psblock[i] *= -1;
				psblock[i] = (q_2scale * psblock[i]) + q_add;
				psblock[i] *= -1;
			}
		}
	}
}
