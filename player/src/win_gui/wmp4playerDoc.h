// wmp4playerDoc.h : interface of the CWmp4playerDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WMP4PLAYERDOC_H__5A91C651_D020_411F_AC70_785A4FF16584__INCLUDED_)
#define AFX_WMP4PLAYERDOC_H__5A91C651_D020_411F_AC70_785A4FF16584__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWmp4playerDoc : public CDocument
{
protected: // create from serialization only
	CWmp4playerDoc();
	DECLARE_DYNCREATE(CWmp4playerDoc)

// Attributes
public:
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWmp4playerDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWmp4playerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWmp4playerDoc)
	afx_msg void OnFileClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMP4PLAYERDOC_H__5A91C651_D020_411F_AC70_785A4FF16584__INCLUDED_)
