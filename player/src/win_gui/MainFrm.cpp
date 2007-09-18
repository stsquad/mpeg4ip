// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "wmp4player.h"

#include "MainFrm.h"
#include <SDL.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_PAGE_UP, OnPageUp)
	ON_COMMAND(ID_PAGE_DOWN, OnPageDown)
	ON_COMMAND(ID_SPACE, OnSpace)
	ON_COMMAND(ID_HOME, OnHome)
	ON_COMMAND(ID_END, OnEnd)
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_MESSAGE(WM_CLOSED_SESSION, OnCloseSession)
	ON_MESSAGE(WM_SESSION_DIED, OnSessionDied)
	ON_MESSAGE(WM_SESSION_ERROR, OnSessionError)
	ON_MESSAGE(WM_SESSION_WARNING, OnSessionWarning)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

LRESULT CMainFrame::OnCloseSession (WPARAM, LPARAM)
{
	theApp.StopSession();
	return 0;
}

LRESULT CMainFrame::OnSessionDied (WPARAM, LPARAM)
{
	theApp.SessionDied();
	return 0;
}

LRESULT CMainFrame::OnSessionWarning(WPARAM temp, LPARAM notemp)
{
	const char *p = (const char *)temp;
	AfxMessageBox(p);
	return 0;
}

LRESULT CMainFrame::OnSessionError(WPARAM temp, LPARAM notemp)
{
	const char *p = (const char *)temp;
	AfxMessageBox(p);
	theApp.StopSession();
	return 0;
}
	
/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers



void CMainFrame::OnPageUp() 
{
	GetActiveView()->PostMessage(WM_SDL_KEY, SDLK_PAGEUP, 0);
}

void CMainFrame::OnPageDown() 
{
	GetActiveView()->PostMessage(WM_SDL_KEY, SDLK_PAGEDOWN, 0);	
}

void CMainFrame::OnSpace() 
{
	GetActiveView()->PostMessage(WM_SDL_KEY, SDLK_SPACE, 0);	
}

void CMainFrame::OnHome() 
{
	GetActiveView()->PostMessage(WM_SDL_KEY, SDLK_HOME, 0);	
}

void CMainFrame::OnEnd() 
{
	GetActiveView()->PostMessage(WM_SDL_KEY, SDLK_END, 0);		
}
