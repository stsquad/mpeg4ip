// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__3731CD97_9B52_4F61_AB71_26B87A4A03A4__INCLUDED_)
#define AFX_MAINFRM_H__3731CD97_9B52_4F61_AB71_26B87A4A03A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	LRESULT OnCloseSession(WPARAM, LPARAM);
	LRESULT OnSessionDied(WPARAM, LPARAM);
	LRESULT OnSessionWarning(WPARAM, LPARAM);
	LRESULT OnSessionError(WPARAM, LPARAM);
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPageUp();
	afx_msg void OnPageDown();
	afx_msg void OnSpace();
	afx_msg void OnHome();
	afx_msg void OnEnd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__3731CD97_9B52_4F61_AB71_26B87A4A03A4__INCLUDED_)
