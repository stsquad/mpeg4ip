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

	newpred.hpp

Abstract:

	Interface of the CNewPred class.

Revision History:

**************************************************************************/

#if !defined(AFX_NEWPRED_H__2E698A43_7818_11D1_80C6_0000F82273F4__INCLUDED_)
#define AFX_NEWPRED_H__2E698A43_7818_11D1_80C6_0000F82273F4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef __MFC_
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW				   
#endif // __MFC_

#define NP_REQUESTED_BACKWARD_MESSAGE_TYPE  3	// backchannel message type
#define ENC_BUFCNT 5							// number of encoder additional memory
#define DEC_BUFCNT 5							// number of decoder additional memory

#define NP_MAX_NUMSLICE	18					// maxium number of NP segmet
#define	INPUT_BUF_MAX	256
#define	MAX_NumGOB		NP_MAX_NUMSLICE

#define NUMBITS_NEWPRED_ENABLE						1
#define NUMBITS_REQUESTED_BACKWARD_MESSAGE_TYPE		2
#define NUMBITS_NEWPRED_SEGMENT_TYPE				1
#define NUMBITS_VOP_ID_FOR_PREDICTION_INDICATION	1

#define NUMBITS_VOP_ID_PLUS							3
#define NP_MAX_NUMBITS_VOP_ID						15 // maxium bit number of vop_id

#define	NP_MAX(x, y)		((x) >= (y) ? (x) : (y))

typedef struct NEWPRED_Slice_data {
	PixelC*		pchY;
	PixelC*		pchU;
	PixelC*		pchV;
} NEWPRED_dat;

typedef struct NEWPRED_Slice_buf {
	int			iSizeY;
	int			iSizeUV;
	int			vop_id;
	int			iSlice;
	NEWPRED_dat pdata;		// backup reference picture data
} NEWPRED_buf;

typedef struct NEWPRED_control {
	int		ref_tbl[0x800][MAX_NumGOB];
	NEWPRED_buf***	NPRefBuf;
	int*			ref;
} NEWPREDcnt;

typedef enum
{
	NP_ENCODER,
	NP_DECODER,
}	NP_WHO_AM_I;

typedef enum
{
	NP_VOP_HEADER,
	NP_VP_HEADER
}	NP_SYNTAX_TYPE;

class CNewPred
{
public:
	CNewPred();
	virtual ~CNewPred();

	Bool	CheckSlice(int iMBX, int iMBY, Bool bChkTop = TRUE);
	int		CNewPred::GetSliceNum(int iMBX, int iMBY);
	int		NextSliceHeadMBA(int iMBX, int iMBY);
	int		GetCurrentVOP_id();

	void	shiftBuffer(							// store reference picture memory
				int			vop_id,
				int			max_refsel
			);

	int		make_next_decbuf(
				NEWPREDcnt*	newpredCnt,
				int			vop_id,
				int			slice_no
			);

	int		NowMBA(int vp_id);

	void	CopyBuftoNPRefBuf(int iSlice, int iBufCnt);
	void	CopyBufUtoNPRefBufY(int iSlice, int iBufCnt);
	void	CopyBufUtoNPRefBufU(int iSlice, int iBufCnt);
	void	CopyBufUtoNPRefBufV(int iSlice, int iBufCnt);

	void 	*aalloc(int col, int row , int size);
	void 	afree(int** p);
	void	SetQBuf(									// set NP and Original reference picture memory
				CVOPU8YUVBA*	pRefQ0,
				CVOPU8YUVBA*	pRefQ1
			);

	void 	SetNPRefBuf(						// set data from reference to NP memory
				NEWPRED_buf**	pNewBuf,
				int				vop_id,
				int				iBufCnt
			);

	Bool	CopyNPtoVM(							// set data from NP to reference memory
				Int				iSlice_no,
				PixelC*			RefpointY,
				PixelC*			RefpointU,
				PixelC*			RefpointV
			);

	Bool	CopyNPtoPrev(
				Int				iSlice_no,
				PixelC*			RefpointY,
				PixelC*			RefpointU,
				PixelC*			RefpointV
			);

	void	ChangeRefOfSlice(					// padding reference picture
				const PixelC*	ppxlcRefY,
				const PixelC*	RefbufY,
				const PixelC*	ppxlcRefU,
				const PixelC*	RefbufU,
				const PixelC*	ppxlcRefV,
				const PixelC*	RefbufV,
				Int				iMBX,
				Int				iMBY,
				CRct			rctRefFrameY,
				CRct			rctRefFrameUV
			);

	void	ChangeRefOfSliceYUV(				// padding reference picture
				const PixelC*	ppxlcRef,
				const PixelC*	Refbuf0,
				Int				iMBX,
				Int				iMBY,
				CRct			RefSize,
				char			mode
			);

	void	CopyReftoBuf(						// backup reference picture to virtual memory
				const PixelC*	RefbufY,
				const PixelC*	RefbufU,
				const PixelC*	RefbufV,
				CRct			rctRefFrameY,
				CRct			rctRefFrameUV
			);

	void	CopyRefYtoBufY(const PixelC* ppxlcRefY, CRct RefSize);
	void	CopyBufYtoRefY(const PixelC* ppxlcRefY, CRct RefSize);
	void	CopyRefUtoBufU(const PixelC* ppxlcRefU, CRct RefSize);
	void	CopyBufUtoRefU(const PixelC* ppxlcRefU, CRct RefSize);
	void	CopyRefVtoBufV(const PixelC* ppxlcRefV, CRct RefSize);
	void	CopyBufVtoRefV(const PixelC* ppxlcRefV, CRct RefSize);

	void	GetSlicePoint(char * pchSlicePointParam);
	int		getwidth(){return(m_iWidth);}

	int CNewPred::SliceTailMBA(int iMBX, int iMBY);

#ifdef _DEBUG
	void cdecl NPDebugMessage( char* pszMsg, ... );
#endif

	/*
	 *  variable
	 */
	Bool			m_bNewPredEnable;
	int				m_iRequestedBackwardMessegeType;
	Bool			m_bNewPredSegmentType;
	int				m_iNumBuffEnc;
	int				m_iNumBuffDec;
	NP_WHO_AM_I		m_enumWho;
	NEWPREDcnt*		m_pNewPredControl;
	int				m_iNumSlice;
	NEWPRED_buf**	m_pShiftBufTmp;

	int				m_iAUsage;
	int				m_bShapeOnly;

	Int				m_iNPNumMBX, m_iNPNumMBY;
	int*			m_iHMBNum;

	UInt	m_maxVopID;

	PixelC*		m_pchNewPredRefY;
	PixelC*		m_pchNewPredRefU;
	PixelC*		m_pchNewPredRefV;

protected:

	NP_WHO_AM_I		Who_Am_I();					// check wther encoder or decoder
	void			IncrementVopID();
	short			check_space(char* buf);
	void			check_comment(char* buf);

	/*
	 *  variable
	 */
	int*	m_piSlicePoint;
	int		m_iVopID;
	int		m_iNumBitsVopID;
	int		m_iVopID4Prediction_Indication;
	int		m_iVopID4Prediction;

	int		m_iWidth;
	int		m_iHeight;
	int		num_MB;
	int		m_iCurrentSlice;
	CRct	m_rctNPFrameY;
	CRct	m_rctNPFrameUV;

	UInt	m_uiFirstFrame;
	UInt	m_uiLastFrame;
	CVOPU8YUVBA*	m_pNPvopcRefQ0;
	CVOPU8YUVBA*	m_pNPvopcRefQ1;

	PixelC*	m_pDecbufY;
	PixelC*	m_pDecbufU;
	PixelC*	m_pDecbufV;

};
		
class CNewPredEncoder : public CNewPred
{
public:
	CNewPredEncoder();
	virtual ~CNewPredEncoder();
	void	SetObject(
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
		);

	Int		SetVPData(
		NP_SYNTAX_TYPE	mode,
		int		*md_iVopID,	
		int		*md_iNumBitsVopID,
		int		*md_iVopID4Prediction_Indication,
		int		*md_iVopID4Prediction
		);
	
	NEWPREDcnt*		initNEWPREDcnt(
		UInt		uiVO_id
		);
	void	endNEWPREDcnt(
				NEWPREDcnt*	newpredCnt
		);
	void	makeNextRef(
				NEWPREDcnt*	newpredCnt,
				int			slice_no
		);
	void	makeNextBuf(
				NEWPREDcnt*	newpredCnt,
				int			vop_id,
				int			slice_no
		);

	void	load_ref_ind();


protected:
	char	refname[INPUT_BUF_MAX];

	struct	{
		int		iHeader;
		int		iData;
	}	m_stSliceSize[NP_MAX_NUMSLICE];
};

class CNewPredDecoder : public CNewPred
{
public:
	CNewPredDecoder();
	virtual ~CNewPredDecoder();
	void	SetObject(
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
		);

	Bool	GetRef(
		NP_SYNTAX_TYPE	mode,
		VOPpredType type,
		int		md_iVopID,	
		int		md_iVopID4Prediction_Indication,
		int		md_iVopID4Prediction
		);						

	NEWPREDcnt*	initNEWPREDcnt();

	void	endNEWPREDcnt(NEWPREDcnt* newpredCnt);

	void	ResetObject(int iCurrentVOP_id)
	{
		m_iVopID = iCurrentVOP_id;
	}

protected:
	int				m_DecoderError;
};

#endif // !defined(AFX_NEWPRED_H__2E698A43_7818_11D1_80C6_0000F82273F4__INCLUDED_)
