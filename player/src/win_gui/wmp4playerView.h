// wmp4playerView.h : interface of the CWmp4playerView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WMP4PLAYERVIEW_H__CD6DE0F0_92C8_4851_B12D_777B951E65DA__INCLUDED_)
#define AFX_WMP4PLAYERVIEW_H__CD6DE0F0_92C8_4851_B12D_777B951E65DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWmp4playerView : public CFormView
{
protected: // create from serialization only
	CWmp4playerView();
	DECLARE_DYNCREATE(CWmp4playerView)

public:
	//{{AFX_DATA(CWmp4playerView)
	enum { IDD = IDD_WMP4PLAYER_FORM };
	CSliderCtrl	m_volume_slider;
	CBitmapButton	m_stop_button;
	CBitmapButton	m_pause_button;
	CBitmapButton	m_play_button;
	CSliderCtrl	m_time_slider;
	CComboBox	m_combobox;
	//}}AFX_DATA

// Attributes
public:
	CWmp4playerDoc* GetDocument();
	void OnCloseSession(void);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWmp4playerView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL
afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
// Implementation
public:
	virtual ~CWmp4playerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void OnUpdate( CView* pSender, LPARAM lHint, CObject* pHint );
// Generated message map functions
protected:
	//{{AFX_MSG(CWmp4playerView)
	afx_msg void OnBrowseButton();
	afx_msg void OnDropdownCombo1();
	afx_msg void OnDblclkCombo1();
	afx_msg void OnEnter();
	afx_msg void OnSetfocusCombo1();
	afx_msg void OnKillfocusCombo1();
	afx_msg void OnSelendokCombo1();
	afx_msg void OnPlayButton();
	afx_msg void OnPauseButton();
	afx_msg void OnStopButton();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnAudioMute();
	//}}AFX_MSG
	afx_msg LRESULT OnSdlKey(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
private:
	int m_timer_slider_selected;
	int m_nTimer;
};

#ifndef _DEBUG  // debug version in wmp4playerView.cpp
inline CWmp4playerDoc* CWmp4playerView::GetDocument()
   { return (CWmp4playerDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMP4PLAYERVIEW_H__CD6DE0F0_92C8_4851_B12D_777B951E65DA__INCLUDED_)
