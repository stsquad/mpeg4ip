/**************************************************************************

This software module was originally developed by 

	Hideaki Kimata (NTT)


in the course of development of the MPEG-4 Video (ISO/IEC 14496-2) standard.
This software module is an implementation of a part of one or more MPEG-4 
Video (ISO/IEC 14496-2) tools as specified by the MPEG-4 Video (ISO/IEC 
14496-2) standard. 

ISO/IEC gives users of the MPEG-4 Video (ISO/IEC 14496-2) standard free 
license to this software module or modifications thereof for use in hardware
or software products claiming conformance to the MPEG-4 Video (ISO/IEC 
14496-2) standard. 

Those intending to use this software module in hardware or software products
are advised that its use may infringe existing patents. The original 
developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this 
software module or modifications thereof in an implementation. Copyright is 
not released for non MPEG-4 Video (ISO/IEC 14496-2) standard conforming 
products. 

NTT retains full right to use the code for his/her own 
purpose, assign or donate the code to a third party and to inhibit third 
parties from using the code for non MPEG-4 Video (ISO/IEC 14496-2) standard
conforming products. This copyright notice must be included in all copies or
derivative works. 

Copyright (c) 1999.

Module Name:

	newpenc.cpp

Abstract:

	Implementation of the CNewPredEncoder class.

Revision History:

**************************************************************************/

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#include "basic.hpp"
#include "typeapi.h"
#include "codehead.h"
#include "mode.hpp"
#include "global.hpp"
#include "bitstrm.hpp"
#include "newpred.hpp"
#include <string.h>

CNewPredEncoder::CNewPredEncoder() : CNewPred()
{
	m_enumWho = NP_ENCODER;
}

CNewPredEncoder::~CNewPredEncoder()
{
	endNEWPREDcnt( m_pNewPredControl );
}

void CNewPredEncoder::SetObject (
					int				iNumBitsTimeIncr,
					int				iWidth,
					int				iHeight,
					UInt			uiVOId,
					char*			pchRefname,
					char*			pchSlicePointParam,
					Bool			bNewpredSegmentType,
					int				iAUsage,
					int				bShapeOnly,
					CVOPU8YUVBA*	pNPvopcRefQ0,
					CVOPU8YUVBA*	pNPvopcRefQ1,
					CRct			rctNPFrameY,
					CRct			rctNPFrameUV
					)
{

	m_iNumBitsVopID = iNumBitsTimeIncr + NUMBITS_VOP_ID_PLUS;
	if( m_iNumBitsVopID > NP_MAX_NUMBITS_VOP_ID )
		m_iNumBitsVopID = NP_MAX_NUMBITS_VOP_ID;
	m_iWidth = iWidth;
	m_iHeight = iHeight;
	m_iNPNumMBX = m_iWidth/MB_SIZE;
	m_iNPNumMBY = m_iHeight/MB_SIZE;

	m_iNumBuffEnc = ENC_BUFCNT;
	m_bNewPredSegmentType = bNewpredSegmentType;

	m_iAUsage = iAUsage;
	m_bShapeOnly = bShapeOnly;
	
	m_pNPvopcRefQ0 = pNPvopcRefQ0;		// set reference picture memory
	m_pNPvopcRefQ1 = pNPvopcRefQ1;		// set original picture memory

	m_rctNPFrameY = rctNPFrameY;
	m_rctNPFrameUV = rctNPFrameUV;

	Int iHeightUV = iHeight/(MB_SIZE/BLOCK_SIZE);
	Int	iWidthUV  = iWidth/(MB_SIZE/BLOCK_SIZE);

	if (m_bShapeOnly == FALSE) {
		m_pchNewPredRefY = new PixelC[(2*EXPANDY_REF_FRAME+iWidth)*(2*EXPANDY_REF_FRAME+iHeight)];
		m_pchNewPredRefU = new PixelC[(2*EXPANDUV_REF_FRAME+iWidthUV)*(2*EXPANDUV_REF_FRAME+iHeightUV)];
		m_pchNewPredRefV = new PixelC[(2*EXPANDUV_REF_FRAME+iWidthUV)*(2*EXPANDUV_REF_FRAME+iHeightUV)];
	}
	
	GetSlicePoint(pchSlicePointParam);

	m_pNewPredControl = initNEWPREDcnt( uiVOId );
	sprintf( refname, "%s", pchRefname);
	load_ref_ind();	
}

Int CNewPredEncoder::SetVPData (
		NP_SYNTAX_TYPE	mode,
		int		*md_iVopID,	
		int		*md_iNumBitsVopID,
		int		*md_iVopID4Prediction_Indication,
		int		*md_iVopID4Prediction
		)
{
	Int		iPut = 0;
	static int CurrentSlice = 0;

	switch( mode )
	{
	case NP_VOP_HEADER:
		CurrentSlice = 0;
		m_iVopID4Prediction = m_pNewPredControl->ref[CurrentSlice];
		IncrementVopID(); 
		iPut += m_iNumBitsVopID;
		if((m_iVopID - 1) == m_iVopID4Prediction || m_iVopID4Prediction == 0)
			m_iVopID4Prediction_Indication = 0;
		else
			m_iVopID4Prediction_Indication = 1;
		
		iPut += NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION;
		if( m_iVopID4Prediction_Indication )
		{
			iPut += m_iNumBitsVopID;
		}
		iPut += MARKER_BIT;
		break;
	case NP_VP_HEADER:
		if (m_bNewPredSegmentType == 0)
			CurrentSlice ++;
		m_iVopID4Prediction = m_pNewPredControl->ref[CurrentSlice];
		iPut += m_iNumBitsVopID;

		if((m_iVopID - 1) == m_iVopID4Prediction || m_iVopID4Prediction == 0)
			m_iVopID4Prediction_Indication = 0;
		else
			m_iVopID4Prediction_Indication = 1;

		iPut += NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION;
		if( m_iVopID4Prediction_Indication )
		{
			iPut += m_iNumBitsVopID;
		}
		iPut += MARKER_BIT;
		break;
	}
	*md_iVopID = m_iVopID;	
	*md_iNumBitsVopID = m_iNumBitsVopID;
	*md_iVopID4Prediction_Indication = m_iVopID4Prediction_Indication;
	*md_iVopID4Prediction = m_iVopID4Prediction;
	return iPut;
}

/* move from newpred.cpp */
void  CNewPredEncoder::load_ref_ind()
{
	int		i, j;
	FILE	*read_file;
	char	buf[256];
	char	*dtp;
	int		count=0;
	int		line=0;
	int		err_flag = 0;
	int		int_dt;
	int		frameNo;
	int		first = 1;

	memset(m_pNewPredControl->ref_tbl, 0x00, 0x800*MAX_NumGOB*sizeof(int));

	if( (read_file = fopen(refname,"r")) == NULL ) {
		fprintf( stdout,
			"Unable to open Ref_Indication_file: %s\n",
			refname);
		exit( -1 );
	}

	while( !feof(read_file) ) {
		line++;

		if( first ) {
			if( fgets(buf,256,read_file) == 0 )
			{
				fprintf( stderr, "Read error!! : %s\n", refname );
				exit( -1 );
			}
			first = 0;
		}
		check_comment( buf );

		if( check_space(buf)!=0 ) {
			fgets( buf,256,read_file );
			continue;
		}

		i = 0, j = 0; 
		dtp = strtok( buf, " \t\n" );
		frameNo = atoi( dtp );

		while( (dtp = strtok(NULL, " \t\n")) != NULL ) {
			int_dt = atoi( dtp );

			if( int_dt < 0 || int_dt > count + 1 ) {
				err_flag = 1;
			}

			if(int_dt > (int)~(0xffffffff<<m_iNumBitsVopID)) {
				err_flag = 1;
			}

			m_pNewPredControl->ref_tbl[count][i++] = int_dt;
			if( i >= MAX_NumGOB )
				break;
		}

		if( err_flag != 0 || i > (m_iHeight/MB_SIZE) )
		{
			fprintf( stdout,
				"Error reference frame table: %s(%d Line)\n", refname, line );
			exit( -1 );
		}

		count++;
		if( count >= 0x800 )
			break;

		fgets( buf, 256, read_file );
	}

	fclose( read_file );
	first = 1;
}


NEWPREDcnt*  CNewPredEncoder::initNEWPREDcnt( UInt uiVO_id)
{
	int			i, j;
	NEWPREDcnt*	newpredCnt;

	newpredCnt = (NEWPREDcnt *) malloc(sizeof (NEWPREDcnt));
	memset(newpredCnt, 0, sizeof(NEWPREDcnt));
 
	m_iNumSlice = 1;
	for( i = 0; i < NP_MAX_NUMSLICE; i++ ) {
		if( m_piSlicePoint[i] < 0 )
			break;
		m_iNumSlice++;
	}
	--m_iNumSlice;

	newpredCnt->NPRefBuf = (NEWPRED_buf***)aalloc(m_iNumSlice,m_iNumBuffEnc,sizeof(NEWPRED_buf*));
	if (newpredCnt->NPRefBuf == NULL) {
	  fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(NEWPRED_buf)\n");
		return newpredCnt;
	}
	newpredCnt->ref = new int[m_iNumSlice];
	if (newpredCnt->ref == NULL) {
	    fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(ref)\n");
		return newpredCnt;
	}
	memset(newpredCnt->ref, 0, sizeof(int)*m_iNumSlice);
		
	Int *iY = new int[m_iNumSlice];
	m_iHMBNum = new int[m_iNumSlice];

	for (i= 0; (i < m_iNumSlice); i++) {
		if (i+1 < m_iNumSlice) {
			iY[i] = *(m_piSlicePoint + i+1) - *(m_piSlicePoint + i);
		} else {
			iY[i] = (m_iNPNumMBX*m_iNPNumMBY) - *(m_piSlicePoint + i);
		}
		m_iHMBNum[i] = iY[i] / m_iNPNumMBX;

		if (m_iHMBNum[i] == 0)
				m_iHMBNum[i] = 1;
	}
	delete []iY;

	for (i= 0; (i < m_iNumSlice) && (*(m_piSlicePoint + i) >= 0); i++) {
		Int	iWidthUV  = m_iWidth/(MB_SIZE/BLOCK_SIZE);
		Int iHeightUV = (MB_SIZE * m_iHMBNum[i])/(MB_SIZE/BLOCK_SIZE);
		
		for (j= 0; j<m_iNumBuffEnc; j++) {
			newpredCnt->NPRefBuf[i][j] = new NEWPRED_buf;
			if (newpredCnt->NPRefBuf[i][j] == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(NEWPRED_buf)\n");
				return newpredCnt;
			}

	     	newpredCnt->NPRefBuf[i][j]->vop_id = 0;

			newpredCnt->NPRefBuf[i][j]->iSizeY = (2*EXPANDY_REF_FRAME+m_iWidth)*(MB_SIZE* m_iHMBNum[i]);
			newpredCnt->NPRefBuf[i][j]->iSizeUV = (EXPANDY_REF_FRAME+iWidthUV)* iHeightUV;

			newpredCnt->NPRefBuf[i][j]->iSlice = i;

			newpredCnt->NPRefBuf[i][j]->pdata.pchY = new PixelC[newpredCnt->NPRefBuf[i][j]->iSizeY];
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchY == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(pchY)\n");
				return newpredCnt;
			}
			newpredCnt->NPRefBuf[i][j]->pdata.pchU = new PixelC[newpredCnt->NPRefBuf[i][j]->iSizeUV];
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchU == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(pchU)\n");
				return newpredCnt;
			}
			newpredCnt->NPRefBuf[i][j]->pdata.pchV = new PixelC[newpredCnt->NPRefBuf[i][j]->iSizeUV];
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchV == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(pchV)\n");
				return newpredCnt;
			}

			memset(newpredCnt->NPRefBuf[i][j]->pdata.pchY, 0, newpredCnt->NPRefBuf[i][j]->iSizeY);
			memset(newpredCnt->NPRefBuf[i][j]->pdata.pchU, 0, newpredCnt->NPRefBuf[i][j]->iSizeUV);
			memset(newpredCnt->NPRefBuf[i][j]->pdata.pchV, 0, newpredCnt->NPRefBuf[i][j]->iSizeUV);

		}
	}
    return(newpredCnt);
}

void  CNewPredEncoder::endNEWPREDcnt( NEWPREDcnt* newpredCnt )
{
	{
		int		i, j;

		for (i= 0; (i < m_iNumSlice) && (*(m_piSlicePoint + i) >= 0); i++) {
		
			for (j= 0; j<m_iNumBuffEnc; j++) {
				delete newpredCnt->NPRefBuf[i][j]->pdata.pchY;
				delete newpredCnt->NPRefBuf[i][j]->pdata.pchU;
				delete newpredCnt->NPRefBuf[i][j]->pdata.pchV;
				delete newpredCnt->NPRefBuf[i][j];
			}
		}

		delete []newpredCnt->ref;

		if (newpredCnt->NPRefBuf != NULL) {
			afree((int**)(newpredCnt->NPRefBuf));
		}
	}
	delete []m_iHMBNum;

	free(newpredCnt);
}

void  CNewPredEncoder::makeNextRef(
				NEWPREDcnt*	newpredCnt,
				int			slice_no)
{
	static int count=m_iNumSlice-1;
	if (m_iVopID == 0 && slice_no == 0)
		  count = m_iNumSlice-1;

	count++;
	newpredCnt->ref[slice_no] = newpredCnt->ref_tbl[((count/m_iNumSlice)+1)-2][slice_no];
	return;
}

void  CNewPredEncoder::makeNextBuf(
	NEWPREDcnt*	newpredCnt,
	int			vop_id,
	int			slice_no)
{
	m_pShiftBufTmp = newpredCnt->NPRefBuf[slice_no];
	shiftBuffer(vop_id, m_iNumBuffEnc);
}

