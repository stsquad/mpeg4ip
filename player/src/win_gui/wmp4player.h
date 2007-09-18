// wmp4player.h : main header file for the WMP4PLAYER application
//

#if !defined(AFX_WMP4PLAYER_H__4C7383A1_6883_4ED3_A2CD_A1EC58FD9785__INCLUDED_)
#define AFX_WMP4PLAYER_H__4C7383A1_6883_4ED3_A2CD_A1EC58FD9785__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include "mp4if.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerApp:
// See wmp4player.cpp for the implementation of this class
//
#define WM_CLOSED_SESSION (WM_USER)
#define WM_SESSION_DIED   (WM_USER + 1)
#define WM_SDL_KEY		  (WM_USER + 2)
#define WM_SESSION_ERROR  (WM_USER + 3)
#define WM_SESSION_WARNING (WM_USER + 4)

class CWmp4playerApp : public CWinApp
{
public:
	CWmp4playerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWmp4playerApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	void RemoveLast(void);
	CString m_current_playing;
	CStringList m_played;
	void StartSession(CString &name);
	void StopSession(int terminating = 0);
	void SessionDied(void);
// Implementation
	//{{AFX_MSG(CWmp4playerApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	afx_msg void OnOpenRecentFile(UINT nID);
	afx_msg void OnVideoFullscreen();
	afx_msg void OnUpdateDebugMpeg4isoonly(CCmdUI* pCmdUI);
	afx_msg void OnDebugMpeg4isoonly();
	afx_msg void OnUpdateAudioMute(CCmdUI* pCmdUI);
	afx_msg void OnAudioMute();
	afx_msg void OnRtpOverRtsp();
	afx_msg void OnUpdateRtpOverRtsp(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnUpdateMediaVideo(CCmdUI* pCmdUI);
	afx_msg void OnMediaVideo(UINT id);
	afx_msg void OnUpdateDebugRtp(CCmdUI* pCmdUI);
	afx_msg void OnDebugRtp(UINT id);
	afx_msg void OnUpdateDebugHttp(CCmdUI* pCmdUI);
	afx_msg void OnDebugHttp(UINT id);
	afx_msg void OnUpdateDebugRtsp(CCmdUI* pCmdUI);
	afx_msg void OnDebugRtsp(UINT id);
	afx_msg void OnUpdateDebugSdp(CCmdUI* pCmdUI);
	afx_msg void OnDebugSdp(UINT id);
	DECLARE_MESSAGE_MAP()

	public:
//  Our local data
		CMP4If *m_mp4if;
	private:
		void UpdateClientConfig(void);
		int m_video_size;
		int m_session_died;
};

extern CWmp4playerApp theApp;
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMP4PLAYER_H__4C7383A1_6883_4ED3_A2CD_A1EC58FD9785__INCLUDED_)
