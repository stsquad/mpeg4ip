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

	newpred.cpp

Abstract:

	Implementation of the CNewPred class.

Revision History:

	Aug.30, 1999:   change position of #ifdef _DEBUG by Hideaki Kimata (NTT)

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
#include "entropy/bitstrm.hpp"
#include "newpred.hpp"
#include <string.h>


#ifdef _DEBUG
#include <crtdbg.h>
#define _CRTDBG_MAP_ALLOC
//CMemoryState CMemOld, CMemNew;
#endif

CNewPred::CNewPred()
{
	m_iVopID = 0;
	m_pDecbufY = NULL;
	m_pDecbufU = NULL;
	m_pDecbufV = NULL;

	m_pchNewPredRefY = NULL;
	m_pchNewPredRefU = NULL;
	m_pchNewPredRefV = NULL;

	m_piSlicePoint = NULL;
}

CNewPred::~CNewPred()
{
	if (m_pchNewPredRefY != NULL) delete []m_pchNewPredRefY; 
	if (m_pchNewPredRefU != NULL) delete []m_pchNewPredRefU;
	if (m_pchNewPredRefV != NULL) delete []m_pchNewPredRefV;

	if (m_piSlicePoint != NULL)	delete[] m_piSlicePoint;
}

Bool CNewPred::CheckSlice(int iMBX, int iMBY, Bool bChkTop)
{
	int iNumMBX = m_iWidth / MB_SIZE;
	int iMBNum = iMBX + (iMBY * iNumMBX);
	for( int iLocal = 0; *(m_piSlicePoint + iLocal) >= 0; iLocal++ ) {
		if( *(m_piSlicePoint + iLocal) > iMBNum )
			break;
		if( *(m_piSlicePoint + iLocal) == iMBNum ) {
			if( bChkTop )
				return TRUE;
			else {
				if( iMBNum )
					return TRUE;
			}
		}
	}
	return FALSE;
}

int CNewPred::GetSliceNum(int iMBX, int iMBY)
{	
	int iNumMBX = m_iWidth / MB_SIZE;
	int iMBNum = iMBX + (iMBY * iNumMBX);
	int iLocal;
	for( iLocal = 0; *(m_piSlicePoint + iLocal) >= 0; iLocal++ ) {
		if( iMBNum < *(m_piSlicePoint + iLocal))
			return (iLocal-1);
	}
	return (iLocal-1); // return Slice number
}

void CNewPred::IncrementVopID()
{
	for( int iLocal = 0, iMask = 1; iLocal < m_iNumBitsVopID; iLocal++, iMask <<= 1 ) {
		if( m_iVopID & iMask )
			continue;
		else {
			++m_iVopID;
			return;
		}
	}
	m_iVopID = 1;
}

int CNewPred::NextSliceHeadMBA(int iMBX, int iMBY)
{	
	int iNumMBX = m_iWidth / MB_SIZE;
	int iMBNum = iMBX + (iMBY * iNumMBX);
	for( int iLocal = 0; *(m_piSlicePoint + iLocal) >= 0; iLocal++ ) {
		if( *(m_piSlicePoint + iLocal) > iMBNum ) {	
			return(*(m_piSlicePoint + iLocal));
		}
	}
	return(-1);
}


int CNewPred::NowMBA(int vp_id)
{
  int i;
	for( i = 0; *(m_piSlicePoint + i) >= 0; i++ );

	if(vp_id >= i) return(-1);

	return(*(m_piSlicePoint + vp_id));
}

int CNewPred::GetCurrentVOP_id()
{
	return( m_iVopID );
}

void CNewPred::CopyBuftoNPRefBuf(int iSlice, int iBufCnt)
{
	if (m_bShapeOnly == FALSE) {	
		CopyBufUtoNPRefBufY(iSlice, iBufCnt);
		CopyBufUtoNPRefBufU(iSlice, iBufCnt);
		CopyBufUtoNPRefBufV(iSlice, iBufCnt);
	}
	return;
}

void CNewPred::SetQBuf( CVOPU8YUVBA *pRefQ0, CVOPU8YUVBA *pRefQ1 )
{                                                         
	m_pNPvopcRefQ0 = pRefQ0;
	m_pNPvopcRefQ1 = pRefQ1;
	return;
}

void CNewPred::CopyBufUtoNPRefBufY(int iSlice, int iBufCnt)
{
	Int i;
	Int iSize;

	iSize =0;
	for( i=0; i < iSlice; i++ ) {
		if (*(m_piSlicePoint+i+1)%m_iNPNumMBX)
			continue;
		iSize += m_pNewPredControl->NPRefBuf[i][iBufCnt]->iSizeY;
	}
	PixelC* RefpointY = (PixelC*) m_pNPvopcRefQ1->pixelsY () +
		(EXPANDY_REF_FRAME * m_rctNPFrameY.width) + iSize;
	memcpy(m_pNewPredControl->NPRefBuf[iSlice][iBufCnt]->pdata.pchY, RefpointY,
		 m_pNewPredControl->NPRefBuf[iSlice][iBufCnt]->iSizeY);
	return;
}

void CNewPred::CopyBufUtoNPRefBufU(int iSlice, int iBufCnt)
{
	Int i;
	Int iSize;
 
	iSize =0;
 	for( i=0; i < iSlice; i++ ) {
		if (*(m_piSlicePoint+i+1)%m_iNPNumMBX)
			continue;
		iSize += m_pNewPredControl->NPRefBuf[i][iBufCnt]->iSizeUV;
	}
	PixelC* RefpointU = (PixelC*) m_pNPvopcRefQ1->pixelsU () +
		((EXPANDY_REF_FRAME/2) * m_rctNPFrameUV.width) + iSize;
	memcpy(m_pNewPredControl->NPRefBuf[iSlice][iBufCnt]->pdata.pchU, RefpointU,
		 m_pNewPredControl->NPRefBuf[iSlice][iBufCnt]->iSizeUV);
	return;
}

void CNewPred::CopyBufUtoNPRefBufV(int iSlice, int iBufCnt)
{
	Int i;
	Int iSize;

	iSize =0;
 	for( i=0; i < iSlice; i++ ) {
		if (*(m_piSlicePoint+i+1)%m_iNPNumMBX)
			continue;
		iSize += m_pNewPredControl->NPRefBuf[i][iBufCnt]->iSizeUV;
	}
	PixelC* RefpointV = (PixelC*) m_pNPvopcRefQ1->pixelsV () +
		((EXPANDY_REF_FRAME/2) * m_rctNPFrameUV.width) + iSize;
	memcpy(m_pNewPredControl->NPRefBuf[iSlice][iBufCnt]->pdata.pchV, RefpointV,
		 m_pNewPredControl->NPRefBuf[iSlice][iBufCnt]->iSizeUV);
	return;
}

NP_WHO_AM_I CNewPred::Who_Am_I()
{
	return this->m_enumWho;
}

void * CNewPred::aalloc(int col, int row , int size)
{
	int i;
	void **buf1;
	void *buf2;
    
	buf1 = (void **)malloc(col*size);
	if (buf1 == NULL) {
		return ((void *)NULL);
	}
	buf2 = (void *)calloc(size,col*row);
	if (buf2 == NULL) {
		free(buf1);
		return ((void *)NULL);
	}

	for(i=0; i<col;i++){
		buf1[i] = (void *)((char *)buf2+ row*size*i );
	}

	return(buf1);
}

void CNewPred::afree(int **p)
{
	free(p[0]);
	free(p);
}

void CNewPred::SetNPRefBuf(NEWPRED_buf **pNewBuf, int vop_id, int iBufCnt)
{
	pNewBuf[iBufCnt]->vop_id = vop_id;

	CopyBuftoNPRefBuf(pNewBuf[iBufCnt]->iSlice, iBufCnt);

	return;
}

void CNewPred::ChangeRefOfSliceYUV(const PixelC*	ppxlcRef, const PixelC* Refbuf0,
		 Int iMBX, Int iMBY,CRct RefSize, char mode)
{
	Int i=0;
	Int	MBA;
	Int magnification = 0;
	Int flag = 0;         // check whether padding in horizontal direction
	Int	head_x, length_x;
	PixelC  *left_pix, *right_pix, *point;
	Int p, q;

	PixelC*		Refpoint = (PixelC *)ppxlcRef + RefSize.left;

	switch(mode)
	{
	case 'Y':
		CopyBufYtoRefY(Refbuf0, RefSize);
		magnification = 1;
		break;
	case 'U':
		CopyBufUtoRefU(Refbuf0, RefSize);
		magnification = MB_SIZE/BLOCK_SIZE;
		break;
	case 'V':
		CopyBufVtoRefV(Refbuf0, RefSize);
		magnification = MB_SIZE/BLOCK_SIZE;
		break;
	default:
		assert(0);
		break;
	}

	MBA = NextSliceHeadMBA(iMBX, iMBY);

	head_x = iMBX * MB_SIZE / magnification;
	length_x = (((MBA - 1) % (m_iWidth / MB_SIZE)) + 1) * MB_SIZE / magnification - head_x;
	if (length_x < 0)
		length_x = m_iWidth / magnification - head_x;
	if ((head_x == 0) && (length_x == m_iWidth/magnification))
		flag = 1;

	// padding in horizontal direction
	if (flag == 0) {
		for (p = 0; p <	MB_SIZE / magnification; p++) {
			left_pix = Refpoint + (-RefSize.left) + head_x;
			right_pix = left_pix + length_x - 1;
			point = Refpoint;
			for (q = 0; q < head_x + (-RefSize.left); q++) {
				memcpy(point++, left_pix, 1);
			}
			point = right_pix + 1;
			for (q = 0; q < m_iWidth / magnification - head_x - length_x + (-RefSize.left); q++) {
				memcpy(point++, right_pix, 1);
			}
			Refpoint += RefSize.width;
		}
	}

	// padding above
	Refpoint = (PixelC *)ppxlcRef + RefSize.left;
	while(1) {
		Refpoint -= RefSize.width;
		memcpy(Refpoint , ppxlcRef + RefSize.left, RefSize.width);
		if(Refpoint == Refbuf0) break;
		assert(Refpoint > Refbuf0);
	}
	Refpoint = (PixelC *)ppxlcRef + RefSize.left;

	if(MBA == -1){
		Refpoint += (m_iHeight/MB_SIZE - iMBY) * RefSize.width * MB_SIZE / magnification;
	}else{
		i = MBA/(m_iWidth / MB_SIZE) - iMBY;
		if (flag == 0)
			i = 1;
		Refpoint += i*RefSize.width * MB_SIZE / magnification;
	}

	// padding below
	memcpy(Refpoint, Refpoint - RefSize.width, RefSize.width);
	while(1){
		Refpoint += RefSize.width;
		memcpy(Refpoint, Refpoint - RefSize.width, RefSize.width);
		if(Refpoint == Refbuf0 + (RefSize.bottom - RefSize.top - 1) * RefSize.width) break;
		assert(Refpoint < Refbuf0 + (RefSize.bottom - RefSize.top- 1) * RefSize.width);
	}
}

void CNewPred::CopyReftoBuf(const PixelC* RefbufY, const PixelC* RefbufU, const PixelC* RefbufV, 
							CRct rctRefFrameY, CRct rctRefFrameUV)
{
	CopyRefYtoBufY(RefbufY, rctRefFrameY);
	CopyRefUtoBufU(RefbufU, rctRefFrameUV);
	CopyRefVtoBufV(RefbufV, rctRefFrameUV);
}

void CNewPred::CopyRefYtoBufY(const PixelC*	ppxlcRefY,CRct RefSize)
{
	memcpy(m_pchNewPredRefY, ppxlcRefY, RefSize.width*(RefSize.bottom - RefSize.top));
}

void CNewPred::CopyBufYtoRefY(const PixelC*	ppxlcRefY,CRct RefSize)
{
	memcpy((PixelC *)ppxlcRefY, m_pchNewPredRefY, RefSize.width*(RefSize.bottom - RefSize.top));
}

void CNewPred::CopyRefUtoBufU(const PixelC*	ppxlcRefU,CRct RefSize)
{
	memcpy(m_pchNewPredRefU, ppxlcRefU, RefSize.width*(RefSize.bottom - RefSize.top));
}

void CNewPred::CopyBufUtoRefU(const PixelC*	ppxlcRefU,CRct RefSize)
{
	memcpy((PixelC *)ppxlcRefU, m_pchNewPredRefU, RefSize.width*(RefSize.bottom - RefSize.top));
}

void CNewPred::CopyRefVtoBufV(const PixelC*	ppxlcRefV,CRct RefSize)
{
	memcpy(m_pchNewPredRefV, ppxlcRefV, RefSize.width*(RefSize.bottom - RefSize.top));
}

void CNewPred::CopyBufVtoRefV(const PixelC*	ppxlcRefV,CRct RefSize)
{
	memcpy((PixelC *)ppxlcRefV, m_pchNewPredRefV, RefSize.width*(RefSize.bottom - RefSize.top));
}

void CNewPred::ChangeRefOfSlice(const PixelC* ppxlcRefY, const PixelC* RefbufY, const PixelC* ppxlcRefU,
								const PixelC* RefbufU, const PixelC* ppxlcRefV, const PixelC* RefbufV,
								Int iMBX, Int iMBY, CRct rctRefFrameY, CRct rctRefFrameUV)
{
	ChangeRefOfSliceYUV(ppxlcRefY, RefbufY, iMBX, iMBY,rctRefFrameY,'Y');
	ChangeRefOfSliceYUV(ppxlcRefU, RefbufU, iMBX, iMBY,rctRefFrameUV,'U');
	ChangeRefOfSliceYUV(ppxlcRefV, RefbufV, iMBX, iMBY,rctRefFrameUV,'V');
}

Bool CNewPred::CopyNPtoVM(Int iSlice_no, PixelC* RefpointY, PixelC* RefpointU, PixelC* RefpointV)
{
	int ww, hh, st;
	int dist;
	int j;
	int cur_slice_no, next_slice_no;

	cur_slice_no = *(m_piSlicePoint + iSlice_no);
	next_slice_no = *(m_piSlicePoint + iSlice_no +1);
	if (next_slice_no == -1)
		next_slice_no = m_iNPNumMBX*m_iNPNumMBY;

	st = (cur_slice_no % m_iNPNumMBX) + EXPANDY_REF_FRAME/MB_SIZE;
	ww = next_slice_no - cur_slice_no;
	if (ww <= m_iNPNumMBX)
		hh = 1;
	else {
		hh = ww / m_iNPNumMBX;
		ww = m_iNPNumMBX;
	}

	for (Int i=0; i < (Who_Am_I() == NP_ENCODER?m_iNumBuffEnc:m_iNumBuffDec); i++) {
		if (m_pNewPredControl->ref[iSlice_no] == 0) {
			break;
		}
		if( (m_pNewPredControl->NPRefBuf[iSlice_no][i]->vop_id 
			== m_pNewPredControl->ref[iSlice_no]) )
		{
			for (j = 0; j < hh * MB_SIZE; j++) {
				dist = st*MB_SIZE + j* m_rctNPFrameY.width;
				memcpy(RefpointY+dist, m_pNewPredControl->NPRefBuf[iSlice_no][i]->pdata.pchY+dist, ww*MB_SIZE);
			}
			for (j = 0; j < hh * BLOCK_SIZE; j++) {
				dist = st*BLOCK_SIZE + j* m_rctNPFrameUV.width;
				memcpy(RefpointU+dist, m_pNewPredControl->NPRefBuf[iSlice_no][i]->pdata.pchU+dist, ww*BLOCK_SIZE);
				memcpy(RefpointV+dist, m_pNewPredControl->NPRefBuf[iSlice_no][i]->pdata.pchV+dist, ww*BLOCK_SIZE);
			}
			return TRUE;
		}
#ifdef _DEBUG
		else
		{
			NPDebugMessage("Not find NP memory (%02d:%02d)\n", m_pNewPredControl->ref[iSlice_no], iSlice_no);
		}
#endif
	}
	return FALSE;
}

Bool CNewPred::CopyNPtoPrev(Int iSlice_no, PixelC* RefpointY, PixelC* RefpointU, PixelC* RefpointV)
{
	int ww, hh, st;
	int dist;
	int j;
	int cur_slice_no, next_slice_no;

	cur_slice_no = *(m_piSlicePoint + iSlice_no);
	next_slice_no = *(m_piSlicePoint + iSlice_no +1);
	if (next_slice_no == -1)
		next_slice_no = m_iNPNumMBX*m_iNPNumMBY;

	st = (cur_slice_no % m_iNPNumMBX) + EXPANDY_REF_FRAME/MB_SIZE;
	ww = next_slice_no - cur_slice_no;
	if (ww <= m_iNPNumMBX)
		hh = 1;
	else {
		hh = ww / m_iNPNumMBX;
		ww = m_iNPNumMBX;
	}
	if (Who_Am_I() == NP_ENCODER) {
		m_pNewPredControl->ref[iSlice_no] = m_pNewPredControl->NPRefBuf[iSlice_no][0]->vop_id;
	} else {
		if (m_pNewPredControl->NPRefBuf[iSlice_no][0]->vop_id != 1) {
			m_pNewPredControl->ref[iSlice_no] = m_pNewPredControl->NPRefBuf[iSlice_no][0]->vop_id - 1;
		} else {
			m_pNewPredControl->ref[iSlice_no] = m_maxVopID;
		}
	}
	for (Int i=0; i < (Who_Am_I() == NP_ENCODER?m_iNumBuffEnc:m_iNumBuffDec); i++) {
		if( (m_pNewPredControl->NPRefBuf[iSlice_no][i]->vop_id 
			== m_pNewPredControl->ref[iSlice_no]) )
		{
			for (j = 0; j < hh * MB_SIZE; j++) {
				dist = st*MB_SIZE + j* m_rctNPFrameY.width;
				memcpy(RefpointY+dist, m_pNewPredControl->NPRefBuf[iSlice_no][i]->pdata.pchY+dist, ww*MB_SIZE);
			}
			for (j = 0; j < hh * BLOCK_SIZE; j++) {
				dist = st*BLOCK_SIZE + j* m_rctNPFrameUV.width;
				memcpy(RefpointU+dist, m_pNewPredControl->NPRefBuf[iSlice_no][i]->pdata.pchU+dist, ww*BLOCK_SIZE);
				memcpy(RefpointV+dist, m_pNewPredControl->NPRefBuf[iSlice_no][i]->pdata.pchV+dist, ww*BLOCK_SIZE);
			}
			return TRUE;

		}
	}
	return FALSE;
}


#include <stdarg.h>

// 1999.8.30 NTT
#ifdef	_DEBUG
void cdecl CNewPred::NPDebugMessage( char* pszMsg, ... )
{
//#ifdef	_DEBUG
	va_list	arg_ptr;
	char	szBuf[256];

	va_start( arg_ptr, pszMsg );
	vsprintf( szBuf, pszMsg, arg_ptr );
	
	_RPT0(_CRT_WARN, szBuf);
//#endif
}
#endif

int CNewPred::SliceTailMBA(int iMBX, int iMBY)
{	
	int iNumMBX = m_iWidth / MB_SIZE;
	int iMBNum = iMBX + (iMBY * iNumMBX);
	int iresult;

	if(iMBNum > m_iWidth/MB_SIZE*m_iHeight/MB_SIZE-1) return(-1);
	for( int iLocal = 0; *(m_piSlicePoint + iLocal) >= 0; iLocal++ ) {
		if( *(m_piSlicePoint + iLocal) > iMBNum ) {
			if(*(m_piSlicePoint + iLocal) == -1){
				iresult = m_iWidth/MB_SIZE*m_iHeight/MB_SIZE-1;
			}
			else{
				iresult = *(m_piSlicePoint + iLocal)-1;
			}
			return(iresult);
		}
	}
	return(m_iWidth/MB_SIZE*m_iHeight/MB_SIZE-1);
}

void CNewPred::GetSlicePoint(char * pchSlicePointParam) 
{
	char*	pchSlicePoint = pchSlicePointParam;
	int		iLocal;
	int		iScanValue = 0;
	char* pchWrok = pchSlicePoint;

	if (pchSlicePointParam[0]) {
		iLocal = 0;
		while (sscanf( pchWrok, "%d", &iScanValue ) != EOF) {
			iLocal++;
			pchWrok = strchr( pchWrok, ',' );
			if (pchWrok == NULL)
				break;
			++pchWrok;
		}
		if (m_bNewPredSegmentType == 1) // VOP segment
			iLocal = 1;
		m_piSlicePoint = new int[iLocal+1];			// generate slice point

		if (m_bNewPredSegmentType == 1) { // VOP segment
			m_iNumSlice = 1;
			iLocal = 0;
			*(m_piSlicePoint + iLocal) = 0;
			iLocal++;
			*(m_piSlicePoint + iLocal) = -1;
		} else {
			m_iNumSlice = iLocal;
			iLocal = 0;
				while (sscanf(pchSlicePoint, "%d", &iScanValue ) != EOF) {
				*(m_piSlicePoint + iLocal) = iScanValue;
				iLocal++;
				pchSlicePoint = strchr( pchSlicePoint, ',' );
				if (pchSlicePoint == NULL)
					break;
				++pchSlicePoint;
			}
			*(m_piSlicePoint + iLocal) = -1;

			if (*m_piSlicePoint != 0) {
				fprintf (stderr, "Wrong slice number\n");
				exit (1);
			}

			int i, before_x, next_x, flag = 0;
			for (iLocal = 0; iLocal < m_iNumSlice; iLocal++) {
				if ((*(m_piSlicePoint + iLocal)) % m_iNPNumMBX) {
					flag = 0;
					before_x = ((*(m_piSlicePoint + iLocal)) / m_iNPNumMBX) * m_iNPNumMBX;
					next_x = (before_x == (m_iNPNumMBY-1)*m_iNPNumMBX) ? -1 : (before_x + m_iNPNumMBX);
					for (i = 0; i <= m_iNumSlice; i++) {
						if ((*(m_piSlicePoint + i)) == before_x)
							flag++;
						if ((*(m_piSlicePoint + i)) == next_x)
							flag++;					
					}
					if (flag != 2) {
						fprintf (stderr, "Wrong slice number\n");
						exit (1);
					}
				}
			}
		}
	}
}


void  CNewPred::check_comment(char *buf)
{
	while(*buf != '\0'){
		if (*buf == '%') {
			*buf = '\0';
		}
		buf++;
	}
}

short  CNewPred::check_space(char *buf)
{
	short	i;
	i=0;

	if(*buf == (char)NULL)
		i=1;
	else if(strspn(buf," \t\n")==strlen(buf))
		i=1;

	return(i);
}

int CNewPred::make_next_decbuf(
	NEWPREDcnt*	newpredCnt,
	int			vop_id,
	int			slice_no)
{
  int		noStore_vop_id = 0;

	m_pShiftBufTmp = newpredCnt->NPRefBuf[slice_no];
	shiftBuffer(vop_id, m_iNumBuffDec);

	return(noStore_vop_id);
}

void  CNewPred::shiftBuffer(
				int		vop_id,
				int		max_refsel)
{
	int			i;
	int			iMoveCnt = max_refsel-1;
	NEWPRED_buf *tmpbuf = NULL;

	if(m_pShiftBufTmp) {
		tmpbuf = m_pShiftBufTmp[iMoveCnt];
	}

	for(i=iMoveCnt; i>0; i--) {
		if(m_pShiftBufTmp)
		{
			m_pShiftBufTmp[i] = m_pShiftBufTmp[i-1];
		}
	}

	if(m_pShiftBufTmp) {
		m_pShiftBufTmp[0] = tmpbuf;
		SetNPRefBuf(m_pShiftBufTmp, vop_id, 0);
	}
	return;
}

