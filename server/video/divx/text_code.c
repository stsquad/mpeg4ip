
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
 *  text_code.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some functions for text coding of image.            */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "text_defs.h"
#include "mot_code.h"
#include "bitstream.h"
#include "putvlc.h"
#include "mot_util.h"
#include "text_code_mb.h"
#include "text_code.h"

#define SKIPP       6

extern FILE *ftrace;

Void  	Bits_CountMB_combined (	Int DQUANT,
			Int Mode,
			Int COD,
			Int ACpred_flag,
			Int CBP,
			Int vop_type,
			Bits *bits,
			Image *mottext_bitstream,
			Int *MB_transp_pattern
	);
Int  	doDCACpred (	Int *qcoeff,
			Int *CBP,
			Int ncoeffs,
			Int x_pos,
			Int y_pos,
			Int ***DC_store,
			Int QP,
			Int MB_width,
			Int direction[],
			Int mid_grey
	);
Void nullfill(Int pred[], Int mid_grey);

#define Idir_c(val, QP) \
	((val) < 0 ? ((val)-(QP)/2)/(QP) : ((val)+(QP)/2)/(QP))

Int  	IntraDCSwitch_Decision _P_((	Int Mode,
			Int intra_dc_vlc_thr,
			Int Qp
	));
Int  	FindCBP _P_((	Int *qcoeff,
			Int Mode,
			Int ncoeffs
	));


/***********************************************************CommentBegin******
 *
 * -- VopCodeShapeTextIntraCom --Intra texture encoding of one vop,
 *                          Combined shape/(motion)/texture mode
 *
 * Purpose :
 *	Intra texture encoding of one vop (combined shape/(mot)/text mode)
 *
 * Arguments in :
 *	    Vop curr : the current vop to be coded
 *      Int intra_dcpred_disable : disable intradc prediction
 *      Image* AB_SizeConversionDecisions:
 *      Image* AB_first_MMR_values
 *      VolConfig *vol_config : configuration information
 *      Int rc_type : rate control type:
 *
 * Arguments out :
 *	    Vop *rec_curr : the reconstructed current vop
 * 	    Image *texture_bitstream : the output bitstream
 *      Bits : statistics information
 *
 * Description :
 *	This function performs Intra texture encoding of one vop.
 *
 ***********************************************************CommentEnd********/

Void VopCodeShapeTextIntraCom(Vop *curr,
Vop *rec_curr, Image *mottext_bitstream)
{
	Int QP = GetVopIntraQuantizer(curr);
	Int Mode = MODE_INTRA;
	Int* qcoeff;
	Int i, j;
	Int CBP, COD;
	Int CBPY, CBPC;
	Int num_pixels = GetImageSizeX(GetVopY(curr));
	Int num_lines = GetImageSizeY(GetVopY(curr));
	Int vop_type;
	Int ***DC_store;
	Int MB_width = num_pixels / MB_SIZE;
	Int MB_height = num_lines / MB_SIZE;
	Int m;
	Int ACpred_flag=-1;
	Int direction[6];
	Int switched=0;
	Int DQUANT =0;

	Bits nbits, *bits;
	bits = &nbits;

	qcoeff = (Int *) malloc (sizeof (Int) * 384);

	#ifdef _RC_DEBUG_
	fprintf(ftrace, "RC - VopCodeShapeTextIntraCom(): ---> CODING WITH: %d \n",QP);
	#endif

	for (i = 0; i < 6; i++)
		direction[i] = 0;

	/* allocate space for 3D matrix to keep track of prediction values
	   for DC/AC prediction */

	DC_store = (Int ***)calloc(MB_width*MB_height, sizeof(Int **));
	for (i = 0; i < MB_width*MB_height; i++)
	{
		DC_store[i] = (Int **)calloc(6, sizeof(Int *));
		for (j = 0; j < 6; j++)
			DC_store[i][j] = (Int *)calloc(15, sizeof(Int));
	}

	Bits_Reset (bits);
	vop_type = PCT_INTRA;

	for (j = 0; j < num_lines/MB_SIZE; j++)		  /* Macro Block loop */
	{
		for (i = 0; i < num_pixels/MB_SIZE; i++)
		{
			DQUANT = 0;

			COD = 0;
			bits->no_intra++;

			CodeMB (curr, rec_curr, NULL, i*MB_SIZE, j*MB_SIZE,
				num_pixels, QP+DQUANT, MODE_INTRA, qcoeff);

			m =0;

			DC_store[j*MB_width+i][0][m] = qcoeff[m]*cal_dc_scaler(QP+DQUANT,1);
			DC_store[j*MB_width+i][1][m] = qcoeff[m+64]*cal_dc_scaler(QP+DQUANT,1);
			DC_store[j*MB_width+i][2][m] = qcoeff[m+128]*cal_dc_scaler(QP+DQUANT,1);
			DC_store[j*MB_width+i][3][m] = qcoeff[m+192]*cal_dc_scaler(QP+DQUANT,1);
			DC_store[j*MB_width+i][4][m] = qcoeff[m+256]*cal_dc_scaler(QP+DQUANT,2);
			DC_store[j*MB_width+i][5][m] = qcoeff[m+320]*cal_dc_scaler(QP+DQUANT,2);
				
			for (m = 1; m < 8; m++)
			{
				DC_store[j*MB_width+i][0][m] = qcoeff[m];
				DC_store[j*MB_width+i][1][m] = qcoeff[m+64];
				DC_store[j*MB_width+i][2][m] = qcoeff[m+128];
				DC_store[j*MB_width+i][3][m] = qcoeff[m+192];
				DC_store[j*MB_width+i][4][m] = qcoeff[m+256];
				DC_store[j*MB_width+i][5][m] = qcoeff[m+320];
			}
			for (m = 0; m < 7; m++)
			{
				DC_store[j*MB_width+i][0][m+8] = qcoeff[(m+1)*8];
				DC_store[j*MB_width+i][1][m+8] = qcoeff[(m+1)*8+64];
				DC_store[j*MB_width+i][2][m+8] = qcoeff[(m+1)*8+128];
				DC_store[j*MB_width+i][3][m+8] = qcoeff[(m+1)*8+192];
				DC_store[j*MB_width+i][4][m+8] = qcoeff[(m+1)*8+256];
				DC_store[j*MB_width+i][5][m+8] = qcoeff[(m+1)*8+320];
			}

			CBP = FindCBP(qcoeff,Mode,64);

			/* Do the DC/AC prediction, changing the qcoeff values as
			   appropriate */
			if (GetVopIntraACDCPredDisable(curr) == 0)
			{
				ACpred_flag = doDCACpred(qcoeff, &CBP, 64, i, j, DC_store,
					QP+DQUANT, MB_width,
					direction,GetVopMidGrey(curr));
			}
			else
				ACpred_flag = -1;

			switched = IntraDCSwitch_Decision(Mode,
				GetVopIntraDCVlcThr(curr),
				QP);
			if (switched)
				CBP = FindCBP(qcoeff,MODE_INTER,64);
			if (DQUANT) Mode=MODE_INTRA_Q;else Mode=MODE_INTRA;

			QP+=DQUANT;

			CBPY = CBP >> 2;
			CBPY = CBPY & 15;			  /* last 4 bits */
			CBPC = CBP & 3;				  /* last 2 bits */

			Bits_CountMB_combined (DQUANT, Mode, COD, ACpred_flag, CBP,
				vop_type,
				bits, mottext_bitstream,/*MB_transp_pattern*/NULL);

			/* added the variable intra_dcpred_diable */
			MB_CodeCoeff(bits, qcoeff, Mode, CBP, 64,
				GetVopIntraACDCPredDisable(curr),
				NULL, mottext_bitstream,
				/*MB_transp_pattern*/NULL, direction,
				1 /*GetVopErrorResDisable(curr)*/,
				0 /*GetVopReverseVlc(curr)*/,
				switched,
				0 /*curr->alternate_scan*/);
		}
	}

	/* Free allocated memory for 3D matrix */
	for (i = 0; i < MB_width*MB_height; i++)
	{
		for (j = 0; j < 6; j++)
			free(DC_store[i][j]);
		free(DC_store[i]);
	}
	free(DC_store);

	free ((Char*)qcoeff);
}

/***********************************************************CommentBegin******
 *
 * -- VopShapeMotText -- Combined Inter encoding of shape motion and texture
 *
 * Purpose :
 *	Combined Inter encoding of texture and motion.
 * 	Used by VopCodeMotTextInter
 *
 * Arguments in :
 *	Vop *curr : the current vop to be encoded
 * 	Vop *rec_prev: the previous reconstructed vop
 * 	Image *mot_x : the x-coordinates of the motion vectors
 * 	Image *mot_y : the y-coordinates of the motion vectors
 * 	Image *MB_decisions: Contains for each macroblock the encoding mode
 *	Int	f_code_for: MV search range 1/2 pel: 1=32,2=64,...,7=2048
 *      Image* AB_SizeConversionDecisions:
 *      Image* AB_first_MMR_values :
 *      Int intra_dcpred_disable : disable the intra dc prediction
 *      VolConfig *vol_config : configuration information
 *      Int rc_type : rate control type
 *
 * Arguments out :
 *	Vop *rec_curr : the reconstructed current vop
 * 	Image *mottext_bitstream : the output texture/motion bitstream
 *  Bits *bits : Coding statistics
 *
 ***********************************************************CommentEnd********/

Void VopShapeMotText (Vop *curr, Vop *comp, 
Image *MB_decisions, Image *mot_x, Image *mot_y,
Int f_code_for, 
Int intra_acdc_pred_disable,
Vop *rec_curr,
Image *mottext_bitstream
)
{
	Int Mode=0;
	Int QP = GetVopQuantizer(curr);
	Int* qcoeff=NULL;
	Int i, j;
	Int CBP;
	Int COD;
	Int CBPY, CBPC;
	Int MB_in_width, MB_in_height, B_in_width, mbnum, boff;
	SInt p;
	SInt *ptr=NULL;
	Float *motx_ptr=NULL, *moty_ptr=NULL;
	Int num_pixels;
	Int num_lines;
	Int vop_type=PCT_INTER;
	Int ***DC_store=NULL;
	Int m, n;
	Int ACpred_flag=-1;
	Int direction[6];
	Int switched=0;
	Int DQUANT=0;

	Bits nbits, *bits;
	bits = &nbits;

	qcoeff = (Int *) malloc (sizeof (Int) * 384);

	num_pixels = GetImageSizeX(GetVopY(curr));
	num_lines = GetImageSizeY(GetVopY(curr));
	MB_in_width = num_pixels / MB_SIZE;
	MB_in_height = num_lines / MB_SIZE;
	B_in_width = 2 * MB_in_width;

	for (i = 0; i < 6; i++) direction[i] = 0;

	#ifdef _RC_DEBUG_
	printf("RC - VopShapeMotText(): ---> CODING WITH: %d \n",QP);
	#endif

	/* allocate space for 3D matrix to keep track of prediction values
	for DC/AC prediction */
	DC_store = (Int ***)calloc(MB_in_width*MB_in_height,
		sizeof(Int **));
	for (i = 0; i < MB_in_width*MB_in_height; i++)
	{
		DC_store[i] = (Int **)calloc(6, sizeof(Int *));
		for (j = 0; j < 6; j++)
			DC_store[i][j] = (Int *)calloc(15, sizeof(Int));
	}

	Bits_Reset (bits);

	vop_type = PCT_INTER;

	ptr = (SInt *) GetImageData(MB_decisions);
	motx_ptr = (Float *) GetImageData(mot_x);
	moty_ptr = (Float *) GetImageData(mot_y);

	for (j = 0; j < num_lines/MB_SIZE; j++)
	{
		for (i = 0; i < MB_in_width; i++)
		{
			switched=0;
			p = *ptr;
			DQUANT = 0;

			/* Fill DC_store with default coeff values */
			for (m = 0; m < 6; m++)
			{
				DC_store[j*MB_in_width+i][m][0] = GetVopMidGrey(curr)*8;
				for (n = 1; n < 15; n++)
					DC_store[j*MB_in_width+i][m][n] = 0;
			}

			switch (p)
			{

				case MBM_INTRA:
					Mode = (DQUANT == 0) ? MODE_INTRA : MODE_INTRA_Q;
					bits->no_intra++;
					break;

				case MBM_INTER16:
					Mode = (DQUANT == 0) ? MODE_INTER : MODE_INTER_Q;
					bits->no_inter++;
					break;

				case MBM_INTER8:
					Mode = MODE_INTER4V;
					bits->no_inter4v++;
					DQUANT = 0;				  /* Can't change QP for 8x8 mode */
					break;

				default:
					printf("invalid MB_decision value :%d\n", p);
					exit(0);
			}

			CodeMB (curr, rec_curr, comp, i*MB_SIZE, j*MB_SIZE,
				num_pixels, QP + DQUANT, Mode, qcoeff);

			mbnum  = j*MB_in_width + i;
			boff = (2 * (mbnum  / MB_in_width) * B_in_width
				+ 2 * (mbnum % MB_in_width));

			CBP = FindCBP(qcoeff,Mode,64);

			if ((CBP == 0) && (p == 1) && (*(motx_ptr +boff) == 0.0)
				&& (*(moty_ptr +boff) == 0.0))
			{
				COD = 1;				  /* skipped macroblock */
				BitstreamPutBits(mottext_bitstream, (long) (COD), 1L);
				bits->COD ++;

				*ptr = SKIPP;
				Mode = MODE_INTER;
			}
			else
			{
				COD = 0;				  /* coded macroblock */

				if ((Mode == MODE_INTRA) || (Mode == MODE_INTRA_Q))
				{

					/* Store the qcoeff-values needed later for prediction */
					m =0;

					DC_store[j*MB_in_width+i][0][m] = qcoeff[m]*cal_dc_scaler(QP+DQUANT,1);
					DC_store[j*MB_in_width+i][1][m] = qcoeff[m+64]*cal_dc_scaler(QP+DQUANT,1);
					DC_store[j*MB_in_width+i][2][m] = qcoeff[m+128]*cal_dc_scaler(QP+DQUANT,1);
					DC_store[j*MB_in_width+i][3][m] = qcoeff[m+192]*cal_dc_scaler(QP+DQUANT,1);
					DC_store[j*MB_in_width+i][4][m] = qcoeff[m+256]*cal_dc_scaler(QP+DQUANT,2);
					DC_store[j*MB_in_width+i][5][m] = qcoeff[m+320]*cal_dc_scaler(QP+DQUANT,2);

					for (m = 1; m < 8; m++)
					{
						DC_store[j*MB_in_width+i][0][m] = qcoeff[m];
						DC_store[j*MB_in_width+i][1][m] = qcoeff[m+64];
						DC_store[j*MB_in_width+i][2][m] = qcoeff[m+128];
						DC_store[j*MB_in_width+i][3][m] = qcoeff[m+192];
						DC_store[j*MB_in_width+i][4][m] = qcoeff[m+256];
						DC_store[j*MB_in_width+i][5][m] = qcoeff[m+320];
					}
					for (m = 0; m < 7; m++)
					{
						DC_store[j*MB_in_width+i][0][m+8] = qcoeff[(m+1)*8];
						DC_store[j*MB_in_width+i][1][m+8] = qcoeff[(m+1)*8+64];
						DC_store[j*MB_in_width+i][2][m+8] = qcoeff[(m+1)*8+128];
						DC_store[j*MB_in_width+i][3][m+8] = qcoeff[(m+1)*8+192];
						DC_store[j*MB_in_width+i][4][m+8] = qcoeff[(m+1)*8+256];
						DC_store[j*MB_in_width+i][5][m+8] = qcoeff[(m+1)*8+320];
					}

					if (intra_acdc_pred_disable == 0)
						ACpred_flag = doDCACpred(qcoeff, &CBP, 64, i, j,
							DC_store,
							QP+DQUANT, MB_in_width,
							direction,GetVopMidGrey(curr));
					else
						ACpred_flag = -1;  /* Not to go into bitstream */
				}

				switched = IntraDCSwitch_Decision(Mode,
					GetVopIntraDCVlcThr(curr),
					QP);
				if (switched)
					CBP = FindCBP(qcoeff,MODE_INTER,64);

				CBPY = CBP >> 2;
				CBPY = CBPY & 15;		  /* last 4 bits */
				CBPC = CBP & 3;			  /* last 2 bits */

				Bits_CountMB_combined (DQUANT, Mode, COD, ACpred_flag, CBP,
					vop_type, bits,
					mottext_bitstream,/*MB_transp_pattern*/NULL);

				Bits_CountMB_Motion( mot_x, mot_y, NULL, 
					MB_decisions, i, j, f_code_for, 0 /*quarter_pel*/,
					mottext_bitstream,
					1 /*GetVopErrorResDisable(curr)*/, 0,
					(Int **)NULL, 0 /*GetVopShape(curr)*/);

				MB_CodeCoeff(bits, qcoeff, Mode, CBP, 64,
					intra_acdc_pred_disable,
					NULL, mottext_bitstream,
					/*MB_transp_pattern*/NULL, direction,
					1/*GetVopErrorResDisable(curr)*/,
					0/*GetVopReverseVlc(curr)*/,
					switched,
					0 /*curr->alternate_scan*/);
			}

			ptr++;

		} /* for i loop */
	} /* for j loop */

	/* Free allocated memory for 3D matrix */
	for (i = 0; i < MB_in_width*MB_in_height; i++)
	{

		for (j = 0; j < 6; j++)
			free(DC_store[i][j]);
		free(DC_store[i]);
	}
	free(DC_store);

	free ((Char*)qcoeff);

}

/***********************************************************CommentBegin******
 *
 * -- Bits_CountMB_combined -- texture encoding for combined texture/motion
 *
 * Purpose :
 *	Used for texture encoding in case of combined texture/motion
 * 		encoding. This function encodes the :
 *		- COD flag
 *		- MCBPC flag
 *		- CBPY flag
 *		- CBPC flag
 *		- DQUANT information
 *
 * Arguments in :
 *	SInt Mode : The macroblock encoding mode
 * 	Int CBP : Coded block pattern information
 * 	Int COD : Indicates whether this macroblock is coded or not
 *	Int ACpred_flag
 * 	Int vop_type : indicates the picture coding type
 *		(Intra,Inter)
 *
 * Arguments out :
 *	Bits* bits : a structure counting the number of bits
 * 	Image *bitstream : output texture bit stream *
 *
 ***********************************************************CommentEnd********/

Void Bits_CountMB_combined(Int DQUANT, Int Mode, Int COD, Int ACpred_flag,
Int CBP, Int vop_type,
Bits* bits, Image *mottext_bitstream,Int *MB_transp_pattern)
{
	Int   cbpy ,cbpc, length;
	Int   MBtype=-1;

	if ( Mode == MODE_INTRA ) MBtype = 3;
	if ( Mode == MODE_INTER ) MBtype = 0;
	if ( Mode == MODE_INTRA_Q) MBtype = 4;
	if ( Mode == MODE_INTER_Q) MBtype = 1;
	if ( Mode == MODE_INTER4V) MBtype = 2;

	/* modified by NTT for GMC coding : start
	  if ( Mode == MODE_DYN_SP) MBtype = 0;
	  if ( Mode == MODE_DYN_SP_Q) MBtype = 1;
	*/
	if ( Mode == MODE_GMC) MBtype = 0;
	if ( Mode == MODE_GMC_Q) MBtype = 1;
	/* modified by NTT for GMC coding : end */

	#ifdef D_TRACE
	fprintf(ftrace, "DQUANT : %d\tMODE : %d\tVop Type : %d\n", DQUANT, Mode, vop_type);
	fprintf(ftrace, "COD : %d\tCBP : %d\tAC Pred Flag : %d\n\n", COD, CBP, ACpred_flag);
	#endif

	cbpc = CBP & 3;
	cbpy = CBP>>2;

	/* COD */

	if (vop_type != PCT_INTRA )
	{
		if (COD)
		{
			printf("COD = 1 in Bits_CountMB_combined \n");
			printf("This function should not be used if COD is '1' \n");
			exit(1);
		}

												  /* write COD */
		BitstreamPutBits(mottext_bitstream, (long)(COD), 1L);
		bits->COD++;
	}

	/* MCBPC */

	if (vop_type == PCT_INTRA)
		length = PutMCBPC_Intra (cbpc, MBtype, mottext_bitstream);
	else
		length = PutMCBPC_Inter (cbpc, MBtype, mottext_bitstream);

	bits->MCBPC += length;

	/* MCSEL syntax */
	/* modified by NTT for GMC coding : start
	 if (((Mode == MODE_INTER) || (Mode == MODE_INTER_Q) || (Mode == MODE_DYN_SP) || (Mode == MODE_DYN_SP_Q))  && (vop_type == PCT_SPRITE))
	*/
	if (((Mode == MODE_INTER) || (Mode == MODE_INTER_Q) || (Mode == MODE_GMC) || (Mode == MODE_GMC_Q))  && (vop_type == PCT_SPRITE))
		/* modified by NTT for GMC coding : end */
	{
		if ((Mode == MODE_INTER) || (Mode == MODE_INTER_Q))
			BitstreamPutBits(mottext_bitstream, (long) 0, 1L);
		/* modified by NTT for GMC coding : start
			  if ((Mode == MODE_DYN_SP) || (Mode == MODE_DYN_SP_Q))
		*/
		if ((Mode == MODE_GMC) || (Mode == MODE_GMC_Q))
			/* modified by NTT for GMC coding : end */
			BitstreamPutBits(mottext_bitstream, (long) 1, 1L);

		bits->MCBPC += 1;
	}

	/* ACpred_flag */
	/* 17-Jan-97 JDL : correction no ACpred_flag in combined mode when intra_acdc_pred_disable is true */
	if ((Mode == MODE_INTRA || Mode==MODE_INTRA_Q) && ACpred_flag != -1)
	{
		BitstreamPutBits(mottext_bitstream, (long)ACpred_flag, 1L);
		bits->ACpred_flag += 1;
	}

	/* CBPY */

	length = PutCBPY (cbpy, (Char)(Mode==MODE_INTRA||Mode==MODE_INTRA_Q),/*MB_transp_pattern*/NULL,mottext_bitstream);

	bits->CBPY += length;

	/* DQUANT */

	/* modified by NTT for GMC coding : start
	  if ((Mode == MODE_INTER_Q) || (Mode == MODE_INTRA_Q)|| (Mode == MODE_DYN_SP_Q))
	*/
	if ((Mode == MODE_INTER_Q) || (Mode == MODE_INTRA_Q)|| (Mode == MODE_GMC_Q))
		/* modified by NTT for GMC coding : end */
	{
		switch (DQUANT)
		{
			case -1:
				BitstreamPutBits(mottext_bitstream, 0L, 2L);
				break;
			case -2:
				BitstreamPutBits(mottext_bitstream, 1L, 2L);
				break;
			case 1:
				BitstreamPutBits(mottext_bitstream, 2L, 2L);
				break;
			case 2:
				BitstreamPutBits(mottext_bitstream, 3L, 2L);
				break;
			default:
				fprintf(stderr,"Invalid DQUANT\n");
				exit(1);
		}
		bits->DQUANT += 2;
	}
}


/***********************************************************CommentBegin******
 *
 * -- doDCACpred -- Does DC/AC prediction. Changes qcoeff values as
 *		    appropriate.
 *
 * Purpose :
 *	Does DC/AC prediction. Changes qcoeff values as appropriate.
 *
 * Arguments in :
 *	Int CBP
 *	Int ncoeffs
 *	Int x_pos
 *	Int y_pos
 *	Int DC_store[][6][15]  	Stores coefficient values per MB for
 *			       	prediction (for one Vop)
 *	Int QP			QP value for this MB
 *	Int MB_width
 *
 * Arguments in/out :
 *	Int *qcoeff
 *
 * Return values :
 *	Int	The ACpred_flag, which is to be put into the bitstream
 *
 * Side effects :
 *	Modifies qcoeff if needed for the prediction.
 *
 ***********************************************************CommentEnd********/

Int doDCACpred(Int *qcoeff, Int *CBP, Int ncoeffs, Int x_pos, Int y_pos,
Int ***DC_store, Int QP, Int MB_width,
Int direction[], Int mid_grey )
{
	Int i, m;
	Int block_A, block_B, block_C;
	Int Xpos[6] = {-1, 0, -1, 0, -1, -1};
	Int Ypos[6] = {-1, -1, 0, 0, -1, -1};
	Int Xtab[6] = {1, 0, 3, 2, 4, 5};
	Int Ytab[6] = {2, 3, 0, 1, 4, 5};
	Int Ztab[6] = {3, 2, 1, 0, 4, 5};
	Int grad_hor, grad_ver, DC_pred;
	Int pred_A[15], pred_C[15];
	Int S = 0, S1, S2;
	Int diff;
	Int pcoeff[384];
	Int ACpred_flag=-1;

	/* Copy qcoeff to the prediction array pcoeff */
	for (i = 0; i < (6*ncoeffs); i++)
	{
		pcoeff[i] = qcoeff[i];
	}

	for (i = 0; i < 6; i++)
	{
		if ((x_pos == 0) && y_pos == 0)			  /* top left corner */
		{
			block_A = (i == 1 || i == 3) ? DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][0] : mid_grey*8;
			block_B = (i == 3) ? DC_store[(y_pos+Ypos[i])*MB_width+(x_pos+Xpos[i])][Ztab[i]][0] : mid_grey*8;
			block_C = (i == 2 || i == 3) ? DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][0] : mid_grey*8;
		}
		else if (x_pos == 0)					  /* left edge */
		{
			block_A = (i == 1 || i == 3) ? DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][0] : mid_grey*8;
			block_B = (i == 1 || i == 3) ? DC_store[(y_pos+Ypos[i])*MB_width+(x_pos+Xpos[i])][Ztab[i]][0] : mid_grey*8;
			block_C = DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][0];
		}
		else if (y_pos == 0)					  /* top row */
		{
			block_A = DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][0];
			block_B = (i == 2 || i == 3) ? DC_store[(y_pos+Ypos[i])*MB_width+(x_pos+Xpos[i])][Ztab[i]][0] : mid_grey*8;
			block_C = (i == 2 || i == 3) ? DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][0] : mid_grey*8;
		}
		else
		{
			block_A = DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][0];
			block_B = (DC_store[(y_pos+Ypos[i])*MB_width+(x_pos+Xpos[i])]
				[Ztab[i]][0]);
			block_C = DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][0];
		}
		grad_hor = block_B - block_C;
		grad_ver = block_A - block_B;

		if ((ABS(grad_ver)) < (ABS(grad_hor)))
		{
			DC_pred = block_C;
			direction[i] = 2;
		}
		else
		{
			DC_pred = block_A;
			direction[i] = 1;
		}

		pcoeff[i*ncoeffs] = qcoeff[i*ncoeffs] - (DC_pred+cal_dc_scaler(QP,(i<4)?1:2)/2)/cal_dc_scaler(QP,(i<4)?1:2);

		/* Find AC predictions */
		if ((x_pos == 0) && y_pos == 0)			  /* top left corner */
		{
			if (i == 1 || i == 3)
				for (m = 0; m < 15; m++)
					pred_A[m] = Idir_c(((DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][m]) * QP*2) , 2*QP);
			else
				nullfill(pred_A,mid_grey);
			if (i == 2 || i == 3)
				for (m = 0; m < 15; m++)
					pred_C[m] = Idir_c(((DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][m]) * QP*2) , 2*QP);
			else
				nullfill(pred_C,mid_grey);
		}
		else if (x_pos == 0)					  /* left edge */
		{
			if (i == 1 || i == 3)
				for (m = 0; m < 15; m++)
					pred_A[m] = Idir_c(((DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][m]) * QP*2) , 2*QP);
			else
				nullfill(pred_A,mid_grey);
			for (m = 0; m < 15; m++)
				pred_C[m] = Idir_c(((DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][m]) * QP*2) , 2*QP);
		}
		else if (y_pos == 0)					  /* top row */
		{
			for (m = 0; m < 15; m++)
				pred_A[m] = Idir_c(((DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][m]) * QP*2) , 2*QP);
			if (i == 2 || i == 3)
				for (m = 0; m < 15; m++)
					pred_C[m] = Idir_c(((DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][m]) * QP*2) , 2*QP);
			else
				nullfill(pred_C,mid_grey);
		}
		else
		{
			for (m = 0; m < 15; m++)
			{
				pred_A[m] = Idir_c(((DC_store[y_pos*MB_width+(x_pos+Xpos[i])][Xtab[i]][m]) * QP*2) , 2*QP);
				pred_C[m] = Idir_c(((DC_store[(y_pos+Ypos[i])*MB_width+x_pos][Ytab[i]][m]) * QP*2) , 2*QP);
			}
		}

		#if 1									  /* I think it should be like this, 14-NOV-1996 MW */
		S1 = 0;
		S2 = 0;
		/* Now decide on AC prediction */
		if (direction[i] == 1)					  /* Horizontal, left COLUMN of block A */
		{
			for (m = 0; m < 7; m++)
			{
				S1 += ABS(qcoeff[i*ncoeffs+(m+1)*8]);
				diff = pcoeff[i*ncoeffs+(m+1)*8]
					= qcoeff[i*ncoeffs+(m+1)*8] - pred_A[m+8];
				S2 += ABS(diff);
			}
		}
		else									  /* Vertical, top ROW of block C */
		{
			for (m = 1; m < 8; m++)
			{
				S1 += ABS(qcoeff[i*ncoeffs+m]);
				diff = pcoeff[i*ncoeffs+m]
					= qcoeff[i*ncoeffs+m] - pred_C[m];
				S2 += ABS(diff);
			}
		}
		S += (S1 - S2);
		#endif
	}
	/* Now change qcoeff for DC pred or DC/AC pred */
	if (S >=0)
	{
		for (i=0;i<ncoeffs*6; i++)
			/* Modified due to N2171 Cl. 2.2.14 MW 25-MAR-1998 */
			/* if ((i%64)&&(abs(pcoeff[i])>127)) { */
			if ((i%64)&&(abs(pcoeff[i])>2047))
		{
			printf("predicted AC out of range");
			S=-1;break;
		}
	}
	if (S >= 0)									  /* Both DC and AC prediction */
	{
		ACpred_flag = 1;
		for (i = 0; i < ncoeffs*6; i++)
		{
			qcoeff[i] = pcoeff[i];
		}
		/* Update CBP for predicted coeffs. */
		*CBP = FindCBP(qcoeff, MODE_INTRA, 64);
	}
	else										  /* Only DC prediction */
	{
		ACpred_flag = 0;
		for (i = 0; i < 6; i++)
		{
			qcoeff[i*ncoeffs] = pcoeff[i*ncoeffs];
			direction[i] = 0;
		}
	}
	return ACpred_flag;							  /* To be put into bitstream */
}

/**
 * Small routine to fill default prediction values into a DC_store entry
 */

Void nullfill(Int pred[], Int mid_grey)
{
	Int i;

	pred[0] = mid_grey*8;
	for (i = 1; i < 15; i++)
	{
		pred[i] = 0;
	}
}


/***********************************************************CommentBegin******
 *
 * -- IntraDCSwitch_decisions --
 *
 * Purpose :
 *	decide whether to use inter AC table to encode DC
 *
 * Arguments in :
 *	Int Mode
 * 	Int intra_dc_vlc_thr
 * 	Int Qp
 *
 ***********************************************************CommentEnd********/

Int IntraDCSwitch_Decision(Int Mode,Int intra_dc_vlc_thr,Int Qp)
{
	Int switched =0;
	if (Mode == MODE_INTRA || Mode == MODE_INTRA_Q)
	{
		if (intra_dc_vlc_thr==0)
			switched=0;
		else if (intra_dc_vlc_thr==7)
			switched=1;
		else if (Qp>=intra_dc_vlc_thr*2+11)
			switched=1;
	}

	return switched;
}


/***********************************************************CommentBegin******
 *
 * -- cal_dc_scaler -- calculation of DC quantization scale according
 *					  to the incoming Q and type;
 *
 * Arguments in :
 *   Int Qp
 *
 ***********************************************************CommentEnd********/

Int cal_dc_scaler (Int QP, Int type)
{

	Int dc_scaler;
	if (type == 1)
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP > 4 && QP < 9) dc_scaler = 2 * QP;
		else if (QP > 8 && QP < 25) dc_scaler = QP + 8;
		else dc_scaler = 2 * QP - 16;
	}
	else
	{
		if (QP > 0 && QP < 5) dc_scaler = 8;
		else if (QP > 4 && QP < 25) dc_scaler = (QP + 13) / 2;
		else dc_scaler = QP - 6;
	}
	return dc_scaler;
}



/***********************************************************CommentBegin******
 *
 * -- FindCBP -- Find the CBP for a macroblock
 *
 * Purpose :
 * Find the CBP for a macroblock
 *
 * Arguments in :
 * Int *qcoeff : pointer to quantized coefficients
 *  Int Mode : macroblock encoding mode information
 *  Int ncoeffs : the number of coefficients
 *
 * Return values :
 * Int CBP : The coded block pattern for a macroblock
 *
 ***********************************************************CommentEnd********/

Int
FindCBP (Int* qcoeff, Int Mode, Int ncoeffs)
{
	Int i,j;
	Int CBP = 0;
	Int intra = (Mode == MODE_INTRA || Mode == MODE_INTRA_Q);

	/* Set CBP for this Macroblock */

	for (i = 0; i < 6; i++)
	{
		for (j = i*ncoeffs + intra; j < (i+1)*ncoeffs; j++)
		{

			if (qcoeff[j])
			{
				if (i == 0) {CBP |= 32;}
				else if (i == 1) {CBP |= 16;}
				else if (i == 2) {CBP |= 8;}
				else if (i == 3) {CBP |= 4;}
				else if (i == 4) {CBP |= 2;}
				else if (i == 5) {CBP |= 1;}
				else
				{
					fprintf (stderr, "Error in CBP assignment\n");
					exit(-1);
				}

				break;
			}
		}
	}

	return CBP;
}

