// wmp4player.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxadv.h>
#include "wmp4player.h"

#include "MainFrm.h"
#include "wmp4playerDoc.h"
#include "wmp4playerView.h"
#include "our_config_file.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerApp

BEGIN_MESSAGE_MAP(CWmp4playerApp, CWinApp)
	//{{AFX_MSG_MAP(CWmp4playerApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND_RANGE(ID_FILE_MRU_FILE1,ID_FILE_MRU_LAST, OnOpenRecentFile)
	ON_COMMAND(ID_VIDEO_FULLSCREEN, OnVideoFullscreen)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_MPEG4ISOONLY, OnUpdateDebugMpeg4isoonly)
	ON_COMMAND(ID_DEBUG_MPEG4ISOONLY, OnDebugMpeg4isoonly)
	ON_UPDATE_COMMAND_UI(ID_AUDIO_MUTE, OnUpdateAudioMute)
	ON_COMMAND(ID_AUDIO_MUTE, OnAudioMute)
	ON_COMMAND(ID_RTP_OVER_RTSP, OnRtpOverRtsp)
	ON_UPDATE_COMMAND_UI(ID_RTP_OVER_RTSP, OnUpdateRtpOverRtsp)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_UPDATE_COMMAND_UI_RANGE(ID_MEDIA_VIDEO_50, ID_MEDIA_VIDEO_200, OnUpdateMediaVideo)
	ON_COMMAND_RANGE(ID_MEDIA_VIDEO_50, ID_MEDIA_VIDEO_200, OnMediaVideo)
	ON_UPDATE_COMMAND_UI_RANGE(ID_RTP_DEBUG_EMERG, ID_RTP_DEBUG_DEBUG, OnUpdateDebugRtp)
	ON_COMMAND_RANGE(ID_RTP_DEBUG_EMERG, ID_RTP_DEBUG_DEBUG, OnDebugRtp)
	ON_UPDATE_COMMAND_UI_RANGE(ID_HTTP_EMERG, ID_HTTP_DEBUG, OnUpdateDebugHttp)
	ON_COMMAND_RANGE(ID_HTTP_EMERG, ID_HTTP_DEBUG, OnDebugHttp)
	ON_UPDATE_COMMAND_UI_RANGE(ID_RTSP_EMERG, ID_RTSP_DEBUG, OnUpdateDebugRtsp)
	ON_COMMAND_RANGE(ID_RTSP_EMERG, ID_RTSP_DEBUG, OnDebugRtsp)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SDP_EMERG, ID_SDP_DEBUG, OnUpdateDebugSdp)
	ON_COMMAND_RANGE(ID_SDP_EMERG, ID_SDP_DEBUG, OnDebugSdp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerApp construction

CWmp4playerApp::CWmp4playerApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_mp4if = NULL;
	m_video_size = 1;
	m_session_died = 0;
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CWmp4playerApp object

CWmp4playerApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerApp initialization

BOOL CWmp4playerApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Mpeg4ip"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)


	const char *str;
	config.InitializeIndexes();
	config.ReadVariablesFromRegistry("Software\\Mpeg4ip", "Config");
	str = config.get_config_string(CONFIG_PREV_FILE_0);
	if (str != NULL) {
		m_played.AddTail(str);
	}
	str = config.get_config_string(CONFIG_PREV_FILE_1);
	if (str != NULL) {
		m_played.AddTail(str);
	}
	str = config.get_config_string(CONFIG_PREV_FILE_2);
	if (str != NULL) {
		m_played.AddTail(str);
	}
	str = config.get_config_string(CONFIG_PREV_FILE_3);
	if (str != NULL) {
		m_played.AddTail(str);
	}
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CWmp4playerDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CWmp4playerView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CString	m_version;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_version.Format("%s - %s", AfxGetAppName(), MPEG4IP_VERSION);
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_VERSION_STRING, m_version);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWmp4playerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerApp message handlers


void CWmp4playerApp::OnFileOpen() 
{
	// TODO: Add your command handler code here
	OutputDebugString("\nApp - OnFileOpen()\n");
	int recent_files;
	CString file_name = "";
	
	recent_files = m_pRecentFileList->GetSize();
	if (recent_files > 0) {
		if (m_pRecentFileList->GetDisplayName(file_name, 
											  0,
											  NULL, 0, FALSE)) {
		} else file_name = "";

	} 

	CFileDialog fd(TRUE, 
				   NULL, 
				   file_name, 
				   OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, 
				   NULL, NULL);
	if (fd.DoModal() == IDOK) {
		StartSession(fd.GetPathName());
	}
	//CWinApp::AddToRecentFileList
}

void CWmp4playerApp::OnOpenRecentFile(UINT nID)
{
	int nIndex = nID - ID_FILE_MRU_FILE1;

	StartSession((*m_pRecentFileList)[nIndex]);

	OutputDebugString((*m_pRecentFileList)[nIndex]);
	OutputDebugString("\n");
	//if (OpenDocumentFile((*m_pRecentFileList)[nIndex]) == NULL)
	//	m_pRecentFileList->Remove(nIndex);

}


void CWmp4playerApp::StartSession (CString &name)
{
	int added_to_list = 0;
	BeginWaitCursor();
	config.WriteVariablesToRegistry("Software\\Mpeg4ip", "Config");
	if (m_mp4if != NULL) {
		StopSession();
	}

	m_current_playing = name;
	if (m_played.Find(name) == NULL) {
		added_to_list = 1;
		m_played.AddHead(name);
	}
	//m_pRecentFileList->Add(name);
	m_mp4if = new CMP4If(name);
	if (m_mp4if->is_valid() == FALSE) {
		OutputDebugString("Could not create mp4if\n");
		delete m_mp4if;
		m_mp4if = NULL;
		EndWaitCursor();
		AfxMessageBox("Could not create valid Client");
		return;
	}
	CString errmsg;
	int err = m_mp4if->get_initial_response(errmsg);
	if (err < 0) {
		OutputDebugString("mp4if response is false\n");
		OutputDebugString(errmsg);
		OutputDebugString("\n");
		delete m_mp4if;
		m_mp4if = NULL;
		EndWaitCursor();
		AfxMessageBox(errmsg);
		return;
	} else {
		if (err > 0) {
			AfxMessageBox(errmsg);
		}
		if (m_video_size != 1) {
			m_mp4if->set_screen_size(m_video_size);
		}
		if (added_to_list == 1) {
      config.set_config_string(CONFIG_PREV_FILE_3, 
				config.get_config_string(CONFIG_PREV_FILE_2));
      config.set_config_string(CONFIG_PREV_FILE_2, 
				config.get_config_string(CONFIG_PREV_FILE_1));
      config.set_config_string(CONFIG_PREV_FILE_1, 
				config.get_config_string(CONFIG_PREV_FILE_0));
      config.set_config_string(CONFIG_PREV_FILE_0, name);
		}
		//m_played.AddTail(name);
	}
	OnFileNew();
			
	EndWaitCursor();
	//OpenDocumentFile(name);
	//GetDocument()->SetPathName(name, FALSE);
	//OpenDocumentFile(name);
}

void CWmp4playerApp::StopSession (int terminating)
{
	OutputDebugString("Stopping Session\n");
	if (m_mp4if != NULL) {
		delete m_mp4if;
		m_mp4if = NULL;
	}
	if (terminating != 1)
		OnFileNew();
	OutputDebugString("Stopped Session\n");
	return;
}

void CWmp4playerApp::SessionDied (void)
{
	if (m_session_died == 0) {
		m_session_died = 1;
		AfxMessageBox("Client Session Crashed");
		StopSession();
		m_session_died = 0;
	}
}

void CWmp4playerApp::RemoveLast (void)
{
	m_pRecentFileList->Remove(0);
}



void CWmp4playerApp::OnVideoFullscreen() 
{
	// TODO: Add your command handler code here
	OutputDebugString("OnVIdeoFullScreen\n");
	
}

void CWmp4playerApp::OnUpdateMediaVideo(CCmdUI* pCmdUI) 
{
	int id = pCmdUI->m_nID - ID_MEDIA_VIDEO_50;

	// TODO: Add your command update UI handler code here
	pCmdUI->Enable();
	pCmdUI->SetRadio(m_video_size == id);
	
}

void CWmp4playerApp::OnMediaVideo(UINT id) 
{
	id -= ID_MEDIA_VIDEO_50;
	// TODO: Add your command handler code here
	m_video_size = id;
	if (m_mp4if)
		m_mp4if->set_screen_size(m_video_size);
}

void CWmp4playerApp::OnUpdateDebugRtp (CCmdUI* pCmdUI)
{
	config_integer_t id = pCmdUI->m_nID - ID_RTP_DEBUG_EMERG;

	pCmdUI->Enable();
	pCmdUI->SetRadio(config.get_config_value(CONFIG_RTP_DEBUG) == id);
}

void CWmp4playerApp::OnDebugRtp (UINT id)
{
	id -= ID_RTP_DEBUG_EMERG;

	config.set_config_value(CONFIG_RTP_DEBUG, id);
	UpdateClientConfig();
}

void CWmp4playerApp::OnUpdateDebugHttp (CCmdUI* pCmdUI)
{
	config_integer_t id = pCmdUI->m_nID - ID_HTTP_EMERG;

	pCmdUI->Enable();
	pCmdUI->SetRadio(config.get_config_value(CONFIG_HTTP_DEBUG) == id);
}

void CWmp4playerApp::OnDebugHttp (UINT id)
{
	id -= ID_HTTP_EMERG;

	config.set_config_value(CONFIG_HTTP_DEBUG, id);
	UpdateClientConfig();
}

void CWmp4playerApp::OnUpdateDebugRtsp (CCmdUI* pCmdUI)
{
	config_integer_t id = pCmdUI->m_nID - ID_RTSP_EMERG;

	pCmdUI->Enable();
	pCmdUI->SetRadio(config.get_config_value(CONFIG_RTSP_DEBUG) == id);
}

void CWmp4playerApp::OnDebugRtsp (UINT id)
{
	id -= ID_RTSP_EMERG;

	config.set_config_value(CONFIG_RTSP_DEBUG, id);
	UpdateClientConfig();
}

void CWmp4playerApp::OnUpdateDebugSdp (CCmdUI* pCmdUI)
{
	config_integer_t id = pCmdUI->m_nID - ID_SDP_EMERG;

	pCmdUI->Enable();
	pCmdUI->SetRadio(config.get_config_value(CONFIG_SDP_DEBUG) == id);
}

void CWmp4playerApp::OnDebugSdp (UINT id)
{
	id -= ID_SDP_EMERG;

	config.set_config_value(CONFIG_SDP_DEBUG, id);
	UpdateClientConfig();
}

int CWmp4playerApp::ExitInstance() 
{
	config.WriteVariablesToRegistry("Software\\Mpeg4ip", "Config");
	return CWinApp::ExitInstance();
}

void CWmp4playerApp::OnUpdateDebugMpeg4isoonly(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	config_index_t iso_check;
	pCmdUI->Enable();
    iso_check = config.FindIndexByName("Mpeg4IsoOnly");
    if (iso_check != UINT32_MAX) {
	    pCmdUI->SetCheck(config.GetBoolValue(iso_check));
	}
}

void CWmp4playerApp::OnDebugMpeg4isoonly() 
{
	// TODO: Add your command handler code here
	int value;
  config_index_t iso_only = config.FindIndexByName("Mpeg4IsoOnly");

  if (iso_only != UINT32_MAX) {
	value = config.GetBoolValue(iso_only);
	config.SetBoolValue(iso_only, !value);
  }
	
}

void CWmp4playerApp::UpdateClientConfig (void)
{
	if (m_mp4if) {
		m_mp4if->client_read_config();
	}
}

void CWmp4playerApp::OnUpdateAudioMute(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if (m_mp4if != NULL && m_mp4if->has_audio()) {
		pCmdUI->Enable();
		pCmdUI->SetCheck(m_mp4if->get_mute());
	} else
		pCmdUI->Enable(FALSE);

}

void CWmp4playerApp::OnAudioMute() 
{
	if (m_mp4if != NULL) {
	    m_mp4if->toggle_mute();
	}
}
	

void CWmp4playerApp::OnRtpOverRtsp() 
{
	int value;

	value = config.get_config_value(CONFIG_USE_RTP_OVER_RTSP);
	value = (value == 0) ? 1 : 0;
	config.set_config_value(CONFIG_USE_RTP_OVER_RTSP, value);	
}

void CWmp4playerApp::OnUpdateRtpOverRtsp(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();

	pCmdUI->SetCheck(config.get_config_value(CONFIG_USE_RTP_OVER_RTSP));
		
}
