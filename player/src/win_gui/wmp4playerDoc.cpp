// wmp4playerDoc.cpp : implementation of the CWmp4playerDoc class
//

#include "stdafx.h"
#include "wmp4player.h"

#include "wmp4playerDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerDoc

IMPLEMENT_DYNCREATE(CWmp4playerDoc, CDocument)

BEGIN_MESSAGE_MAP(CWmp4playerDoc, CDocument)
	//{{AFX_MSG_MAP(CWmp4playerDoc)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerDoc construction/destruction

CWmp4playerDoc::CWmp4playerDoc()
{
	// TODO: add one-time construction code here

}

CWmp4playerDoc::~CWmp4playerDoc()
{
}

BOOL CWmp4playerDoc::OnNewDocument()
{
	OutputDebugString("\nDocument - OnNewDocument\n");
	
	if (theApp.m_mp4if == NULL) {
		this->SetTitle("");
	} else
	this->SetTitle(theApp.m_current_playing);
#if 0
	if (!CDocument::OnNewDocument())
		return FALSE;
#endif
	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CWmp4playerDoc serialization

void CWmp4playerDoc::Serialize(CArchive& ar)
{
	OutputDebugString("\nDoc - serialize\n");
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerDoc diagnostics

#ifdef _DEBUG
void CWmp4playerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWmp4playerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerDoc commands

void CWmp4playerDoc::OnFileClose() 
{
	// TODO: Add your command handler code here
	theApp.StopSession();
}

