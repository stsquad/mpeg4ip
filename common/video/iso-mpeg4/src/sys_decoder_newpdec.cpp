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

	newpdec.cpp

Abstract:

	Implementation of the CNewPredDecoder class.

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

CNewPredDecoder::CNewPredDecoder() : CNewPred()
{
	m_DecoderError=0;
	m_enumWho = NP_DECODER;
	m_iNumSlice = 0;
}

CNewPredDecoder::~CNewPredDecoder()
{
	endNEWPREDcnt(m_pNewPredControl);
}

void CNewPredDecoder::SetObject (
					int				iNumBitsTimeIncr,
					int				iWidth,
					int				iHeight,
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
	m_maxVopID = 1;
	m_maxVopID = m_maxVopID << m_iNumBitsVopID;
	m_maxVopID--;
	m_iWidth = iWidth;
	m_iHeight = iHeight;
	m_iNPNumMBX = m_iWidth/MB_SIZE;
	m_iNPNumMBY = m_iHeight/MB_SIZE;

	m_iNumBuffDec = DEC_BUFCNT;

	m_bNewPredSegmentType = bNewpredSegmentType;

	m_iAUsage = iAUsage;
	m_bShapeOnly = bShapeOnly;

	m_pNPvopcRefQ0 = pNPvopcRefQ0;		// set reference picture memory
	m_pNPvopcRefQ1 = pNPvopcRefQ1;		// set original picture memory

	m_rctNPFrameY = rctNPFrameY;
	m_rctNPFrameUV = rctNPFrameUV;

	GetSlicePoint(pchSlicePointParam);

	Int iHeightUV = iHeight/(MB_SIZE/BLOCK_SIZE);
	Int	iWidthUV  = iWidth/(MB_SIZE/BLOCK_SIZE);

	if (m_bShapeOnly == FALSE) {
		m_pchNewPredRefY = new PixelC[(2*EXPANDY_REF_FRAME+iWidth)*(2*EXPANDY_REF_FRAME+iHeight)];
		m_pchNewPredRefU = new PixelC[(2*EXPANDUV_REF_FRAME+iWidthUV)*(2*EXPANDUV_REF_FRAME+iHeightUV)];
		m_pchNewPredRefV = new PixelC[(2*EXPANDUV_REF_FRAME+iWidthUV)*(2*EXPANDUV_REF_FRAME+iHeightUV)];

		m_pDecbufY = new PixelC[(2*EXPANDY_REF_FRAME+iWidth)*(2*EXPANDY_REF_FRAME+iHeight)];
		m_pDecbufU = new PixelC[(2*EXPANDUV_REF_FRAME+iWidthUV)*(2*EXPANDUV_REF_FRAME+iHeightUV)];
		m_pDecbufV = new PixelC[(2*EXPANDUV_REF_FRAME+iWidthUV)*(2*EXPANDUV_REF_FRAME+iHeightUV)];
	}

	m_pNewPredControl = initNEWPREDcnt();
	if( m_DecoderError > 0 )
		;
	else
	{
		if (m_DecoderError) {
			printf("Error!! : Initialize failure.\n");
			endNEWPREDcnt(m_pNewPredControl);
			exit(-1);
		}
	}
}

Bool CNewPredDecoder::GetRef(
		NP_SYNTAX_TYPE	mode,
		VOPpredType type,
		int		md_iVopID,	
		int		md_iVopID4Prediction_Indication,
		int		md_iVopID4Prediction
		)						
{
	static int iSlice;
	m_iVopID =  md_iVopID;	
	m_iVopID4Prediction_Indication = md_iVopID4Prediction_Indication;
	m_iVopID4Prediction = md_iVopID4Prediction;

	if (type) {
		switch( mode )
		{
		case NP_VOP_HEADER:
			iSlice = 0;
			if( m_iVopID4Prediction_Indication )
			{
				m_pNewPredControl->ref[iSlice] = m_iVopID4Prediction;
			}
			else {
				m_pNewPredControl->ref[iSlice] = 0;
			}
			break;
		case NP_VP_HEADER:
			if (m_bNewPredSegmentType == 0)
				iSlice++;
			if(m_iVopID4Prediction_Indication )
			{
				m_pNewPredControl->ref[iSlice] = m_iVopID4Prediction;
			}
			else {
				m_pNewPredControl->ref[iSlice] = 0;
			}
			break;
		}
	}
	return TRUE;
}

NEWPREDcnt *CNewPredDecoder::initNEWPREDcnt()
{
	int		i,j;
	NEWPREDcnt	*newpredCnt;

	newpredCnt = (NEWPREDcnt *) malloc(sizeof (NEWPREDcnt));
	if (newpredCnt == NULL) {
		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(NEWPREDcnt)\n");
		m_DecoderError=-1;
		return newpredCnt;
	}
	memset(newpredCnt, 0, sizeof(NEWPREDcnt));

	if (m_iNumSlice) {
		newpredCnt->NPRefBuf = (NEWPRED_buf***)aalloc(m_iNumSlice,m_iNumBuffDec,sizeof(NEWPRED_buf*));
 		if (newpredCnt->NPRefBuf == NULL) {
				fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(NEWPRED_buf)\n");
			m_DecoderError=-1;
			return newpredCnt;
		}

		newpredCnt->ref = new int[m_iNumSlice];
		if (newpredCnt->ref == NULL) {
				fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(ref)\n");
			m_DecoderError=-1;
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
	}

	for (i= 0; (i < m_iNumSlice) && (*(m_piSlicePoint + i) >= 0); i++) {

		Int	iWidthUV  = m_iWidth/(MB_SIZE/BLOCK_SIZE);
		Int iHeightUV = (MB_SIZE * m_iHMBNum[i])/(MB_SIZE/BLOCK_SIZE);
		
		for (j= 0; j<m_iNumBuffDec; j++) {
			newpredCnt->NPRefBuf[i][j] = new NEWPRED_buf;
			if (newpredCnt->NPRefBuf[i][j] == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(NEWPRED_buf)\n");
				m_DecoderError=-1;
				return newpredCnt;
			}

	     	newpredCnt->NPRefBuf[i][j]->vop_id = 0;
			newpredCnt->NPRefBuf[i][j]->iSizeY = (2*EXPANDY_REF_FRAME+m_iWidth)*(MB_SIZE* m_iHMBNum[i]);
			newpredCnt->NPRefBuf[i][j]->iSizeUV = (EXPANDY_REF_FRAME+iWidthUV)* iHeightUV;

			newpredCnt->NPRefBuf[i][j]->iSlice = i;
			newpredCnt->NPRefBuf[i][j]->pdata.pchY = new PixelC[newpredCnt->NPRefBuf[i][j]->iSizeY];
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchY == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(pchY)\n");
				m_DecoderError=-1;
				return newpredCnt;
			}
			newpredCnt->NPRefBuf[i][j]->pdata.pchU = new PixelC[newpredCnt->NPRefBuf[i][j]->iSizeUV];
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchU == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(pchU)\n");
				m_DecoderError=-1;
				return newpredCnt;
			}
			newpredCnt->NPRefBuf[i][j]->pdata.pchV = new PixelC[newpredCnt->NPRefBuf[i][j]->iSizeUV];
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchV == NULL) {
	    		fprintf(stderr, "initNEWPREDcnt: ERROR Memory allocate error(pchV)\n");
				m_DecoderError=-1;
				return newpredCnt;
			}

			memset(newpredCnt->NPRefBuf[i][j]->pdata.pchY, 0, newpredCnt->NPRefBuf[i][j]->iSizeY);
			memset(newpredCnt->NPRefBuf[i][j]->pdata.pchU, 0, newpredCnt->NPRefBuf[i][j]->iSizeUV);
			memset(newpredCnt->NPRefBuf[i][j]->pdata.pchV, 0, newpredCnt->NPRefBuf[i][j]->iSizeUV);
		}
	}
    return(newpredCnt);
}

void CNewPredDecoder::endNEWPREDcnt(NEWPREDcnt	*newpredCnt)
{
	int		i, j;
	for (i= 0; (i < m_iNumSlice) && (*(m_piSlicePoint + i) >= 0); i++) {
		
		for (j= 0; j<m_iNumBuffDec; j++) {

			if (newpredCnt->NPRefBuf[i][j]->pdata.pchY != NULL)
				delete newpredCnt->NPRefBuf[i][j]->pdata.pchY;
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchU != NULL)
				delete newpredCnt->NPRefBuf[i][j]->pdata.pchU;
			if (newpredCnt->NPRefBuf[i][j]->pdata.pchV != NULL)
				delete newpredCnt->NPRefBuf[i][j]->pdata.pchV;

			delete newpredCnt->NPRefBuf[i][j];
		}
	}

	if (m_iNumSlice) {
		if (newpredCnt->ref != NULL)  delete []newpredCnt->ref;
		if (newpredCnt->NPRefBuf != NULL) {
			afree((int**)newpredCnt->NPRefBuf);
		}
	
		free(newpredCnt);

		if (m_iHMBNum != NULL) delete []m_iHMBNum;
	}

	if (m_pDecbufY != NULL) delete []m_pDecbufY; 
	if (m_pDecbufU != NULL) delete []m_pDecbufU; 
	if (m_pDecbufV != NULL) delete []m_pDecbufV; 

	return;
}

