
/**************************************************************************
 *                                                                        *
 * This code is developed by Adam Li.  This software is an                *
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
 * http://www.projectmayo.com/opendivx/license.php .                      *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *
 *  text_bits.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some utility functions to write to bitstreams for   */
/* texture part of the coding.                                            */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "momusys.h"
#include "text_defs.h"
#include "bitstream.h"
#include "text_bits.h"
#include "putvlc.h"
#include "zigzag.h"								  /* added, 14-NOV-1996 MW */
#include "max_level.h"							  /* 3-mode esc */

Int  	IntraDC_dpcm (Int val, Int lum, Image *bitstream);
Int  	CodeCoeff (Int j_start, Int Mode, Int qcoeff[],
		Int block, Int ncoeffs, Image *bitstream);
Int  	CodeCoeff_RVLC (Int j_start, Int Mode, Int qcoeff[],
		Int block, Int ncoeffs, Image *bitstream);

/***********************************************************CommentBegin******
 *
 * -- MB_CodeCoeff -- Codes coefficients, does DC/AC prediction
 *
 * Purpose :
 *	Codes coefficients, does DC/AC prediction
 *
 * Arguments in :
 *	Int *qcoeff : quantized dct-coefficients
 * 	Int Mode : encoding mode
 * 	Int CBP : coded block pattern
 * 	Int ncoeffs : number of coefficients per block
 * 	Int x_pos : the horizontal position of the macroblock in the vop
 *      Int intra_dcpred_disable : disable the intradc prediction
 *	Int transp_pattern[]:	Describes which blocks are transparent
 *
 * Arguments out :
 *	Bits *bits : struct to count the number of texture bits
 * 	Image *bitstream : output bitstream
 *
 * Description :
 *	The intradc prediction can be switched off by setting the variable
 *	intradc_pred_disable to '1'.
 *
 ***********************************************************CommentEnd********/
Void MB_CodeCoeff(Bits* bits, Int *qcoeff,
Int Mode, Int CBP, Int ncoeffs,
Int intra_dcpred_disable,
Image *DCbitstream,
Image *bitstream,
Int transp_pattern[], Int direction[],
Int error_res_disable,
Int reverse_vlc,
Int switched,
Int alternate_scan)
{
	Int i, m, coeff[64];
	Int *zz = alternate_scan ? zigzag_v : zigzag;

	if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
	{
		if (intra_dcpred_disable == 0)
		{
			for (i = 0; i < 6; i++)
			{
//				if (i>3 || transp_pattern[i]!=1)  /* Not transparent */
				{
					if (!alternate_scan)
					{
						switch (direction[i])
						{
							case 1: zz = zigzag_v; break;
							case 2: zz = zigzag_h; break;
							case 0: break;
							default: fprintf(stderr, "MB_CodeCoeff(): Error in zigzag direction\n");
							exit(-1);
						}
					}
					/* Do the zigzag scanning of coefficients */
					for (m = 0; m < 64; m++)
					{
						*(coeff + zz[m]) = qcoeff[i*ncoeffs+m];
					}

					if (switched==0)
					{
						if (error_res_disable)
						{
							if (i < 4)
								bits->Y += IntraDC_dpcm(coeff[0],1,bitstream);
							else
								bits->C += IntraDC_dpcm(coeff[0],0,bitstream);
						}
						else
						{
							if (i < 4)
								bits->Y += IntraDC_dpcm(coeff[0],1,DCbitstream);
							else
								bits->C += IntraDC_dpcm(coeff[0],0,DCbitstream);
						}
					}

					/* Code AC coeffs. dep. on block pattern MW 15-NOV-1996 */
					if ((i==0 && CBP&32) ||
						(i==1 && CBP&16) ||
						(i==2 && CBP&8)  ||
						(i==3 && CBP&4)  ||
						(i==4 && CBP&2)  ||
						(i==5 && CBP&1))
					{
						if (error_res_disable || ((!error_res_disable) && (!reverse_vlc)))
						{
							if (i < 4)
								bits->Y += CodeCoeff(1-switched,Mode, coeff,i,ncoeffs,bitstream);
							else
								bits->C += CodeCoeff(1-switched,Mode, coeff,i,ncoeffs,
									bitstream);
						}
						else
						{
							if (i < 4)
								bits->Y += CodeCoeff_RVLC(1-switched,Mode, coeff, i,
									ncoeffs, bitstream);
							else
								bits->C += CodeCoeff_RVLC(1-switched,Mode, coeff, i,
									ncoeffs, bitstream);
						}
					}
				}
			}
		}
		else									  /* Without ACDC prediction */
		{
			for (i = 0; i < 6; i++)
			{
//				if (i>3 || transp_pattern[i]!=1)  /* Not transparent */
				{
					/* Do the zigzag scanning of coefficients */
					for (m = 0; m < 64; m++)
						*(coeff + zz[m]) = qcoeff[i*ncoeffs+m];

					if (switched==0)
					{
						if (error_res_disable)
						{
							if (coeff[0] != 128)
								BitstreamPutBits(bitstream,(long)(coeff[0]),8L);
							else
								BitstreamPutBits(bitstream, 255L, 8L);
						}
						else
						{
							if (coeff[0] != 128)
								BitstreamPutBits(DCbitstream,(long)(coeff[0]),8L);
							else
								BitstreamPutBits(DCbitstream,255L, 8L);
						}

						if (i < 4)
							bits->Y += 8;
						else
							bits->C += 8;
					}

					if ((i==0 && CBP&32) || (i==1 && CBP&16) ||
						(i==2 && CBP&8) || (i==3 && CBP&4) ||
						(i==4 && CBP&2) || (i==5 && CBP&1))
					{
						/* send coeff, not qcoeff !!! MW 07-11-96 */

						if (error_res_disable || ((!error_res_disable) && (!reverse_vlc)))
						{
							if (i < 4)
								bits->Y += CodeCoeff(1-switched,Mode, coeff,i,ncoeffs,
									bitstream);
							else
								bits->C += CodeCoeff(1-switched,Mode, coeff,i,ncoeffs,
									bitstream);
						}
						else
						{
							if (i < 4)
								bits->Y += CodeCoeff_RVLC(1-switched,Mode, coeff, i,
									ncoeffs, bitstream);
							else
								bits->C += CodeCoeff_RVLC(1-switched,Mode, coeff, i,
									ncoeffs, bitstream);
						}

					}
				}
			}
		}
	}
	else										  /* inter block encoding */
	{
		for (i = 0; i < 6; i++)
		{
			/* Do the zigzag scanning of coefficients */
			for (m = 0; m < 64; m++)
				*(coeff + zz[m]) = qcoeff[i*ncoeffs+m];
			if ((i==0 && CBP&32) ||
				(i==1 && CBP&16) ||
				(i==2 && CBP&8) ||
				(i==3 && CBP&4) ||
				(i==4 && CBP&2) ||
				(i==5 && CBP&1))
			{
				if (error_res_disable || ((!error_res_disable) && (!reverse_vlc)))
				{
					if (i < 4)
						bits->Y += CodeCoeff(0,Mode, coeff, i, ncoeffs, bitstream);
					else
						bits->C += CodeCoeff(0,Mode, coeff, i, ncoeffs, bitstream);
				}
				else
				{
					if (i < 4)
						bits->Y += CodeCoeff_RVLC(0,Mode, coeff, i, ncoeffs,
							bitstream);
					else
						bits->C += CodeCoeff_RVLC(0,Mode, coeff, i, ncoeffs,
							bitstream);
				}

			}
		}
	}
}


/***********************************************************CommentBegin******
 *
 * -- IntraDC_dpcm -- DPCM Encoding of INTRADC in case of Intra macroblocks
 *
 * Purpose :
 *	DPCM Encoding of INTRADC in case of Intra macroblocks
 *
 * Arguments in :
 *	Int val : the difference value with respect to the previous
 *		INTRADC value
 * 	Int lum : indicates whether the block is a luminance block (lum=1) or
 * 		a chrominance block ( lum = 0)
 *
 * Arguments out :
 *	Image* bitstream  : a pointer to the output bitstream
 *
 ***********************************************************CommentEnd********/

Int
IntraDC_dpcm(Int val, Int lum, Image *bitstream)
{
	Int n_bits;
	Int absval, size = 0;

	absval = ( val <0)?-val:val;				  /* abs(val) */

	/* compute dct_dc_size */

	size = 0;
	while(absval)
	{
		absval>>=1;
		size++;
	}

	if (lum)
	{											  /* luminance */
		n_bits = PutDCsize_lum (size, bitstream);
	}
	else
	{											  /* chrominance */
		n_bits = PutDCsize_chrom (size, bitstream);
	}

	if ( size != 0 )
	{
		if (val>=0)
		{
			;
		}
		else
		{
			absval = -val;						  /* set to "-val" MW 14-NOV-1996 */
			val = (absval ^( (int)pow(2.0,(double)size)-1) );
		}
		BitstreamPutBits(bitstream, (long)(val), (long)(size));
		n_bits += size;

		if (size > 8)
			BitstreamPutBits(bitstream, (long)1, (long)1);
	}

	return n_bits;								  /* # bits for intra_dc dpcm */

}


/***********************************************************CommentBegin******
 *
 * -- CodeCoeff -- VLC encoding of quantized DCT coefficients
 *
 * Purpose :
 *	VLC encoding of quantized DCT coefficients, except for
 *      INTRADC in case of Intra macroblocks
 *	Used by Bits_CountCoeff
 *
 * Arguments in :
 *	Int Mode : encoding mode
 * 	Int *qcoeff: pointer to quantized dct coefficients
 * 	Int block : number of the block in the macroblock (0-5)
 * 	Int ncoeffs : the number of coefficients per block
 *
 * Arguments out :
 *	Image *bitstream : pointer to the output bitstream
 *
 * Return values :
 *	Int bits : number of bits added to the bitstream (?)
 *
 ***********************************************************CommentEnd********/

Int CodeCoeff(Int j_start, Int Mode, Int qcoeff[], Int block, Int ncoeffs, Image *bitstream)
{
	Int j, bits;
	Int prev_run, run, prev_level, level, first;
	Int prev_ind, ind, prev_s, s, length;

	run = bits = 0;
	first = 1;
	prev_run = prev_level = prev_ind = level = s = prev_s = ind = 0;

	for (j = j_start; j< ncoeffs; j++)
	{
		/* The INTRADC encoding for INTRA Macroblocks is done in
		   Bits_CountCoeff */

		{
			/* do not enter this part for INTRADC coding for INTRA macro-
			   blocks */

			/* encode AC coeff */

			s = 0;

			/* Increment run if coeff is zero */

			if ((level = qcoeff[j]) == 0)
			{
				run++;
			}
			else
			{
				/* code run & level and count bits */

				if (level < 0)
				{
					s = 1;
					level = -level;
				}

				ind = level | run<<4;
				ind = ind | 0<<12;				  /* Not last coeff */

				if (!first)
				{
					/* Encode the previous ind */

					if ((prev_run < 64) &&
						(((prev_level < 13) && (Mode != MODE_INTRA &&
						Mode != MODE_INTRA_Q))
						|| ((prev_level < 28) && (Mode == MODE_INTRA ||
						Mode == MODE_INTRA_Q))))
					{
						/* Separate tables for Intra luminance blocks */
						if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
						{
							length = PutCoeff_Intra(prev_run, prev_level,
								0, bitstream);
						}
						else
						{
							length = PutCoeff_Inter(prev_run, prev_level,
								0, bitstream);
						}
					}
					else
						length = 0;

					/* First escape mode. Level offset */
					if (length == 0)
					{

						if ( prev_run < 64 )
						{

							/* subtraction of Max level, last = 0 */
							int level_minus_max;

							if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
								level_minus_max = prev_level -
									intra_max_level[0][prev_run];
							else
								level_minus_max = prev_level -
									inter_max_level[0][prev_run];

							if (  ( (level_minus_max < 13) && (Mode != MODE_INTRA && Mode != MODE_INTRA_Q) ) ||
								( (level_minus_max < 28) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) ) )
							{
								/* Separate tables for Intra luminance blocks */
								if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
								{
									length = PutLevelCoeff_Intra(prev_run, level_minus_max, 0, bitstream);
								}
								else
								{
									length = PutLevelCoeff_Inter(prev_run, level_minus_max, 0, bitstream);
								}
							} else
							length = 0;
						}
						else length = 0;
					}

					/* Second escape mode. Run offset */
					if (length == 0)
					{
						if ( ((prev_level < 13) && (Mode != MODE_INTRA && Mode != MODE_INTRA_Q)) ||
							((prev_level < 28) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) )
						{

							/* subtraction of Max Run, last = 0 */
							int run_minus_max;

							if (prev_level == 0)
							{
								fprintf (stdout, "ERROR(CodeCoeff-second esc): level is %d\n", prev_level);
								exit(-1);
							}

							if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
								run_minus_max = prev_run - (intra_max_run0[prev_level]+1);
							else
								run_minus_max = prev_run - (inter_max_run0[prev_level]+1);

												  /* boon 120697 */
							if (run_minus_max < 64)
							{
								/* Separate tables for Intra luminance blocks */
								if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
								{
									length = PutRunCoeff_Intra(run_minus_max, prev_level, 0, bitstream);
								}
								else
								{
									length = PutRunCoeff_Inter(run_minus_max, prev_level, 0, bitstream);
								}
							} else
							length = 0;
						}
						else length = 0;
					}

					/* Third escape mode. FLC */
					if (length == 0)
					{							  /* Escape coding */

						if (prev_s == 1)
						{
							/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
							/* prev_level = (prev_level^0xff)+1; */
							prev_level = (prev_level^0xfff)+1;
						}
						BitstreamPutBits(bitstream, 3L, 7L);
												  /* boon */
						BitstreamPutBits(bitstream, 3L, 2L);

												  /* last */
						BitstreamPutBits(bitstream, 0L, 1L);
												  /* run */
						BitstreamPutBits(bitstream, (long)(prev_run), 6L);

						/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
						BitstreamPutBits(bitstream, MARKER_BIT, 1);

						/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
						/* BitstreamPutBits(bitstream, (long)(prev_level), 8L); */
						/* bits += 24; */
												  /* level */
						BitstreamPutBits(bitstream, (long)(prev_level), 12L);

						/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
						BitstreamPutBits(bitstream, MARKER_BIT, 1);

						/*bits += 28;*/
						bits += 30;
					}
					else
					{
						BitstreamPutBits(bitstream, (long)(prev_s), 1L);
						bits += length + 1;
					}
				}

				prev_run = run; prev_s = s;
				prev_level = level; prev_ind = ind;

				run = first = 0;
			}
		}
	}

	/* Encode the last coeff */

	if (!first)
	{
		prev_ind = prev_ind | 1<<12;			  /* last coeff */

		if ((prev_run < 64) &&
			(((prev_level < 4) && (Mode != MODE_INTRA &&
			Mode != MODE_INTRA_Q))
			|| ((prev_level < 9) && ((Mode == MODE_INTRA) ||
			(Mode == MODE_INTRA_Q)))))
		{
			/* Separate tables for Intra luminance blocks */
			if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
			{
				length = PutCoeff_Intra(prev_run, prev_level, 1,
					bitstream);
			}
			else
			{
				length = PutCoeff_Inter(prev_run, prev_level, 1,
					bitstream);
			}
		}
		else
			length = 0;

		/* First escape mode. Level offset */
		if (length == 0)
		{
			if ( prev_run < 64 )
			{

				/* subtraction of Max level, last = 0 */
				int level_minus_max;

				if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
					level_minus_max = prev_level - intra_max_level[1][prev_run];
				else
					level_minus_max = prev_level - inter_max_level[1][prev_run];

				if (  ( (level_minus_max < 4) && (Mode != MODE_INTRA && Mode != MODE_INTRA_Q) ) ||
					( (level_minus_max < 9) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q) ) )
				{
					/* Separate tables for Intra luminance blocks */
					if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
					{
						length = PutLevelCoeff_Intra(prev_run, level_minus_max, 1, bitstream);
					}
					else
					{
						length = PutLevelCoeff_Inter(prev_run, level_minus_max, 1, bitstream);
					}
				} else
				length = 0;
			}
			else length = 0;
		}

		/* Second escape mode. Run offset */
		if (length == 0)
		{
			if (((prev_level < 4) && (Mode != MODE_INTRA && Mode != MODE_INTRA_Q))||
				((prev_level < 9) && (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)) )
			{

				/* subtraction of Max Run, last = 1 */
				int run_minus_max;

				if (prev_level == 0)
				{
					fprintf (stdout, "ERROR(CodeCoeff-second esc): level is %d\n", prev_level);
					exit(-1);
				}

				if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
					run_minus_max = prev_run - (intra_max_run1[prev_level]+1);
				else
					run_minus_max = prev_run - (inter_max_run1[prev_level]+1);

				if (run_minus_max < 64)			  /* boon 120697 */
				{
					/* Separate tables for Intra luminance blocks */
					if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
					{
						length = PutRunCoeff_Intra(run_minus_max, prev_level, 1, bitstream);
					}
					else
					{
						length = PutRunCoeff_Inter(run_minus_max, prev_level, 1, bitstream);
					}
				} else
				length = 0;
			}
			else length = 0;
		}

		/* Third escape mode. FLC */
		if (length == 0)
		{										  /* Escape coding */

			if (prev_s == 1)
			{
				/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
				/* prev_level = (prev_level^0xff)+1; */
				prev_level = (prev_level^0xfff)+1;
			}
			BitstreamPutBits(bitstream, 3L, 7L);
			BitstreamPutBits(bitstream, 3L, 2L);  /* boon */

			BitstreamPutBits(bitstream, 1L, 1L);  /* last */
			BitstreamPutBits(bitstream, (long)(prev_run), 6L);

			/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
			BitstreamPutBits(bitstream, MARKER_BIT, 1);

			/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
			/* BitstreamPutBits(bitstream, (long)(prev_level), 8L); */
			/* bits += 24; */
												  /* level */
			BitstreamPutBits(bitstream, (long)(prev_level), 12L);

			/* 11.08.98 Sven Brandau:  "insert marker_bit" due to N2339, Clause 2.1.21 */
			BitstreamPutBits(bitstream, MARKER_BIT, 1);

			/*bits += 28;*/
			bits += 30;

		}
		else
		{
			BitstreamPutBits(bitstream, (long)(prev_s), 1L);
			bits += length + 1;
		}
	}

	return bits;
}


/***********************************************************CommentBegin******
 *
 * -- CodeCoeff_RVLC -- RVLC encoding of quantized DCT coefficients
 *
 * Purpose :
 *	RVLC encoding of quantized DCT coefficients, except for
 *      INTRADC in case of Intra macroblocks
 *
 * Arguments in :
 *	Int Mode : encoding mode
 * 	Int *qcoeff: pointer to quantized dct coefficients
 * 	Int block : number of the block in the macroblock (0-5)
 * 	Int ncoeffs : the number of coefficients per block
 *
 * Arguments out :
 *	Image *bitstream : pointer to the output bitstream
 *
 * Return values :
 *	Int bits : number of bits added to the bitstream (?)
 *
 ***********************************************************CommentEnd********/

Int CodeCoeff_RVLC(Int j_start, Int Mode, Int qcoeff[], Int block, Int ncoeffs, Image *bitstream)
{
	Int j, bits;
	Int prev_run, run, prev_level, level, first;
	Int prev_ind, ind, prev_s, s, length;

	run = bits = 0;
	first = 1;
	prev_run = prev_level = prev_ind = level = s = prev_s = ind = 0;

	for (j = j_start; j< ncoeffs; j++)
	{
		/* The INTRADC encoding for INTRA Macroblocks is done in
		   Bits_CountCoeff */

		{
			/* do not enter this part for INTRADC coding for INTRA macro-
			   blocks */

			/* encode AC coeff */

			s = 0;

			/* Increment run if coeff is zero */

			if ((level = qcoeff[j]) == 0)
			{
				run++;
			}
			else
			{
				/* code run & level and count bits */

				if (level < 0)
				{
					s = 1;
					level = -level;
				}

				ind = level | run<<4;
				ind = ind | 0<<12;				  /* Not last coeff */
				if (!first)
				{
					/* Encode the previous ind */

					if (prev_level  < 28 && prev_run < 39)
						/* Separate tables for Intra luminance blocks */
						if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
					{
						length = PutCoeff_Intra_RVLC(prev_run, prev_level, 0,
							bitstream);
					}
					else
					{
						length = PutCoeff_Inter_RVLC(prev_run, prev_level, 0,
							bitstream);
					}
					else
						length = 0;

					if (length == 0)
					{							  /* Escape coding */

												  /* ESCAPE */
						BitstreamPutBits(bitstream, 1L, 5L);

												  /* LAST   */
						BitstreamPutBits(bitstream, 0L, 1L);

						BitstreamPutBits(bitstream,
							(long)(prev_run), 6L);/* RUN 	  */

						/* 11.08.98 Sven Brandau:  "changed length for LEVEL (11 bit)" due to N2339, Clause 2.1.21 */
						/* 11.08.98 Sven Brandau:  "insert marker_bit befor and after LEVEL" due to N2339, Clause 2.1.21 */
						/* BitstreamPutBits(bitstream,
								 (long)(prev_level), 7L);*/
						/* LEVEL */
						BitstreamPutBits( bitstream, MARKER_BIT, 1 );
												  /* LEVEL */
						BitstreamPutBits( bitstream, (long)(prev_level), 11L);
						BitstreamPutBits( bitstream, MARKER_BIT, 1 );

												  /* ESCAPE */
						BitstreamPutBits(bitstream, 0L, 4L);

												  /* ESCAPE's */
						BitstreamPutBits(bitstream,
							(long)(prev_s),1L);

						bits += 5 + 1 + 6 + 1 + 11 + 1 + 4 + 1;
						/* bits += 24; */
					}
					else
					{
						BitstreamPutBits(bitstream,
							(long)(prev_s), 1L);
						bits += length + 1;
					}
				}

				prev_run = run; prev_s = s;
				prev_level = level; prev_ind = ind;

				run = first = 0;
			}
		}
	}

	/* Encode the last coeff */

	if (!first)
	{
		prev_ind = prev_ind | 1<<12;			  /* last coeff */

		if (prev_level  < 5 && prev_run < 45)
			/* Separate tables for Intra luminance blocks */
			if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
		{
			length = PutCoeff_Intra_RVLC(prev_run, prev_level, 1,
				bitstream);
		}
		else
		{
			length = PutCoeff_Inter_RVLC(prev_run, prev_level, 1,
				bitstream);
		}
		else
			length = 0;

		if (length == 0)
		{										  /* Escape coding */

			BitstreamPutBits(bitstream, 1L, 5L);  /* ESCAPE	*/

			BitstreamPutBits(bitstream, 1L, 1L);  /* LAST		*/

												  /* RUN		*/
			BitstreamPutBits(bitstream, (long)(prev_run), 6L);

			/* 11.08.98 Sven Brandau:  "changed length for LEVEL (11 bit)" due to N2339, Clause 2.1.21	     */
			/* 11.08.98 Sven Brandau:  "insert marker_bit befor and after LEVEL" due to N2339, Clause 2.1.21 */
			/* BitstreamPutBits(bitstream, (long)(prev_level), 7L);*/
			BitstreamPutBits( bitstream, MARKER_BIT, 1 );
												  /* LEVEL 	*/
			BitstreamPutBits( bitstream, (long)(prev_level), 11L);
			BitstreamPutBits( bitstream, MARKER_BIT, 1 );

			BitstreamPutBits(bitstream, 0L, 4L);  /* ESCAPE 	*/

												  /* ESCAPE's 	*/
			BitstreamPutBits(bitstream, (long)(prev_s), 1L);

			bits += 24;

		}
		else
		{
			BitstreamPutBits(bitstream, (long)(prev_s), 1L);
			bits += length + 1;
		}
	}

	return bits;
}


/***********************************************************CommentBegin******
 *
 * -- Bits_Reset -- To reset the structure bits
 *
 * Purpose :
 *	To reset the structure bits, used for counting the number
 * 	of texture bits
 *
 * Arguments in :
 *	Bits* bits : a pointer to the struct Bits
 *
 ***********************************************************CommentEnd********/

void
Bits_Reset (Bits *bits)
{
	memset(bits, 0, sizeof(Bits));
}
