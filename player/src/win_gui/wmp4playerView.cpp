// wmp4playerView.cpp : implementation of the CWmp4playerView class
//

#include "stdafx.h"
#include "wmp4player.h"

#include "wmp4playerDoc.h"
#include "wmp4playerView.h"
#include "our_config_file.h"
#include <SDL.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerView

IMPLEMENT_DYNCREATE(CWmp4playerView, CFormView)

BEGIN_MESSAGE_MAP(CWmp4playerView, CFormView)
	//{{AFX_MSG_MAP(CWmp4playerView)
	ON_BN_CLICKED(IDC_BROWSE_BUTTON, OnBrowseButton)
	ON_CBN_DROPDOWN(IDC_COMBO1, OnDropdownCombo1)
	ON_CBN_DBLCLK(IDC_COMBO1, OnDblclkCombo1)
	ON_COMMAND(ID_ENTER, OnEnter)
	ON_CBN_SETFOCUS(IDC_COMBO1, OnSetfocusCombo1)
	ON_CBN_KILLFOCUS(IDC_COMBO1, OnKillfocusCombo1)
	ON_CBN_SELENDOK(IDC_COMBO1, OnSelendokCombo1)
	ON_BN_CLICKED(IDC_PLAY_BUTTON, OnPlayButton)
	ON_BN_CLICKED(IDC_PAUSE_BUTTON, OnPauseButton)
	ON_BN_CLICKED(IDC_STOP_BUTTON, OnStopButton)
	ON_WM_TIMER()
	ON_COMMAND(ID_AUDIO_MUTE, OnAudioMute)
	//}}AFX_MSG_MAP
	ON_WM_HSCROLL()
	ON_MESSAGE(WM_SDL_KEY, OnSdlKey)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerView construction/destruction

CWmp4playerView::CWmp4playerView()
	: CFormView(CWmp4playerView::IDD)
{
	//{{AFX_DATA_INIT(CWmp4playerView)
	//}}AFX_DATA_INIT
	// TODO: add construction code here
	m_nTimer = 0;
	m_timer_slider_selected = 0;
}

CWmp4playerView::~CWmp4playerView()
{
	theApp.StopSession(1);
}

void CWmp4playerView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWmp4playerView)
	DDX_Control(pDX, IDC_SLIDER2, m_volume_slider);
	DDX_Control(pDX, IDC_STOP_BUTTON, m_stop_button);
	DDX_Control(pDX, IDC_PAUSE_BUTTON, m_pause_button);
	DDX_Control(pDX, IDC_PLAY_BUTTON, m_play_button);
	DDX_Control(pDX, IDC_SLIDER1, m_time_slider);
	DDX_Control(pDX, IDC_COMBO1, m_combobox);
	//}}AFX_DATA_MAP
}

BOOL CWmp4playerView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	m_play_button.LoadBitmaps("PLAY_BUTTONU", "PLAY_BUTTOND",
							  NULL, "PLAY_BUTTON_DIS");
	m_pause_button.LoadBitmaps("PAUSE_BUTTONU", "PAUSE_BUTTOND",
							   NULL, "PAUSE_BUTTON_DIS");
	m_stop_button.LoadBitmaps("STOP_BUTTONU", "STOP_BUTTOND",
							  NULL, "STOP_BUTTON_DIS");

	return CFormView::PreCreateWindow(cs);
}

void CWmp4playerView::OnInitialUpdate()
{

	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();
	m_time_slider.SetRange(0, 100);
#if 0
	m_play_button.EnableWindow(FALSE);
	m_pause_button.EnableWindow(FALSE);
	m_stop_button.EnableWindow(FALSE);
	m_time_slider.EnableWindow(FALSE);
#endif
}

void CWmp4playerView::OnUpdate (CView* pSender, LPARAM lHint, CObject* pHint)
{
	CFormView::OnUpdate(pSender, lHint, pHint);
	int seekable = 0;
	int timer_on = 0;
	int volume = 0;
	if (theApp.m_mp4if != NULL) {
		OutputDebugString("On update and have app\n");
		seekable = theApp.m_mp4if->is_seekable();
		int state = theApp.m_mp4if->get_state();
		TRACE1("State is %d", state);
		if (state == MP4IF_STATE_PLAY) {
			// Playing
			m_play_button.SetState(TRUE);
			m_pause_button.SetState(FALSE);
			m_stop_button.SetState(FALSE);
			m_pause_button.EnableWindow(TRUE);
			m_stop_button.EnableWindow(TRUE);
			timer_on = 1;
		} else {
			// Stopped or paused
			m_play_button.SetState(FALSE);
			m_pause_button.SetState(state == MP4IF_STATE_PAUSE);
			m_stop_button.SetState(state == MP4IF_STATE_STOP);
			m_play_button.EnableWindow(TRUE);
			m_pause_button.EnableWindow(FALSE);
			m_stop_button.EnableWindow(FALSE);
		}
		if (theApp.m_mp4if->has_audio()) {
			volume = 1;
		}
	} else {
		m_pause_button.SetState(FALSE);
		m_play_button.SetState(FALSE);
		m_stop_button.SetState(FALSE);
		m_play_button.EnableWindow(FALSE);
		m_pause_button.EnableWindow(FALSE);
		m_stop_button.EnableWindow(FALSE);
	}


	m_volume_slider.SetPos(config.get_config_value(CONFIG_VOLUME));
	if (volume == 0) {
		m_volume_slider.EnableWindow(FALSE);
	} else {
		m_volume_slider.EnableWindow(TRUE);
	}
	m_time_slider.EnableWindow(seekable);
	if (seekable == 0)
	   m_time_slider.SetPos(0);
	if (timer_on) {
		m_nTimer = SetTimer(1, 500, NULL);
	} else {
		if (m_nTimer != 0) {
			KillTimer(m_nTimer);
			m_nTimer = 0;
		}
	}
	
}
/////////////////////////////////////////////////////////////////////////////
// CWmp4playerView diagnostics

#ifdef _DEBUG
void CWmp4playerView::AssertValid() const
{
	CFormView::AssertValid();
}

void CWmp4playerView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CWmp4playerDoc* CWmp4playerView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWmp4playerDoc)));
	return (CWmp4playerDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWmp4playerView message handlers

void CWmp4playerView::OnBrowseButton() 
{
	// TODO: Add your control notification handler code here
	theApp.OnFileOpen();
}



void CWmp4playerView::OnDropdownCombo1() 
{
	// TODO: Add your control notification handler code here
   m_combobox.ResetContent();

   POSITION pos;

   pos = theApp.m_played.GetTailPosition(); 
   if (pos == NULL) {
   } else {
	   while (pos != NULL)
	   {
		   CString val = theApp.m_played.GetPrev(pos);
		   int retval = m_combobox.AddString(val);
	   }	
   }   
}

void CWmp4playerView::OnDblclkCombo1() 
{
	// TODO: Add your control notification handler code here
	
}

void CWmp4playerView::OnEnter() 
{
	// TODO: Add your command handler code here
	CString result;
	OutputDebugString("Captured enter\n");
	int sel = m_combobox.GetCurSel();
	if (sel == CB_ERR) {
		m_combobox.GetWindowText(result);
	
	} else {
		m_combobox.GetLBText(sel, result);
	}

	if (!result.IsEmpty()) {
	   theApp.StartSession(result);
	}
}

void CWmp4playerView::OnSetfocusCombo1() 
{
	// TODO: Add your control notification handler code here
	OutputDebugString("Set focus combo 1\n");
}

void CWmp4playerView::OnKillfocusCombo1() 
{
	OutputDebugString("kill focus Combo1\n");
	// TODO: Add your control notification handler code here
	
}

void CWmp4playerView::OnSelendokCombo1() 
{
	// TODO: Add your control notification handler code here
	CString result;
	int sel = m_combobox.GetCurSel();
	if (sel != CB_ERR) {
		m_combobox.GetLBText(sel, result);
		if (!result.IsEmpty()) {
			theApp.StartSession(result);
		}
	}
}

void CWmp4playerView::OnHScroll (UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER1) {
		switch (nSBCode) {
		case TB_LINEUP:
		case TB_LINEDOWN:
		case TB_PAGEUP:
		case TB_PAGEDOWN:
		case TB_THUMBPOSITION:
		case TB_THUMBTRACK:
		case TB_TOP:
		case TB_BOTTOM:
			m_timer_slider_selected = 1;
			break;
		case TB_ENDTRACK: 
			m_timer_slider_selected = 0;
			if (theApp.m_mp4if == NULL ||
				theApp.m_mp4if->is_seekable() == 0) {
				m_time_slider.SetPos(0);
				break;
			}
			double seek_time = theApp.m_mp4if->get_max_time();
			nPos = m_time_slider.GetPos();
			TRACE1("Pos is %d\n", nPos);
			seek_time *= nPos;
			seek_time /= 100.0;
			theApp.m_mp4if->seek_to(seek_time);
			break;
		}
	} else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER2) {
		if (nSBCode == TB_ENDTRACK) {
			config.set_config_value(CONFIG_VOLUME, 
								    m_volume_slider.GetPos());
			TRACE1("Volume to %d\n", m_volume_slider.GetPos());
			if (theApp.m_mp4if &&
				theApp.m_mp4if->has_audio()) {
				theApp.m_mp4if->client_read_config();
			}
		}
	}
}

void CWmp4playerView::OnPlayButton() 
{
	if (theApp.m_mp4if && 
		theApp.m_mp4if->get_state() != MP4IF_STATE_PLAY) {
		theApp.m_mp4if->play();
	}
	OnUpdate(NULL, 0, NULL);
}

void CWmp4playerView::OnPauseButton() 
{
	if (theApp.m_mp4if && 
		theApp.m_mp4if->get_state() == MP4IF_STATE_PLAY) {
		theApp.m_mp4if->pause();
	}
	OnUpdate(NULL, 0, NULL);
	
}

void CWmp4playerView::OnStopButton() 
{
	if (theApp.m_mp4if && 
		theApp.m_mp4if->get_state() == MP4IF_STATE_PLAY) {
		theApp.m_mp4if->stop();
	}
	OnUpdate(NULL, 0, NULL);
}

void CWmp4playerView::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	//	TRACE1("OnTimer %d\n", nIDEvent);
	if (m_timer_slider_selected == 0) {
		uint64_t curr_time;
		double max_time;
		max_time = theApp.m_mp4if->get_max_time();
		if (max_time != 0.0) {
			if (theApp.m_mp4if->get_current_time(&curr_time)) {
				uint64_t time = (uint64_t)(max_time * 1000.0);
				curr_time *= 100;
				curr_time /= time;
				m_time_slider.SetPos(curr_time);
			}
		}
	}
	CFormView::OnTimer(nIDEvent);
}

void CWmp4playerView::OnAudioMute() 
{
	// TODO: Add your command handler code here
	theApp.OnAudioMute();
	OnUpdate(NULL, 0, NULL);
}

void CWmp4playerView::OnCloseSession(void)
{
	OutputDebugString("Got window close\n");
}

afx_msg LRESULT CWmp4playerView::OnSdlKey(WPARAM key, LPARAM mod)
{
	TRACE2("SDL key %x %x\n", key, mod);
	int screen_size;
	int volume;

	CMP4If *mp4if = theApp.m_mp4if;
	if (mp4if == NULL) return 0;
	
	switch (key) {
	case SDLK_c:
		if ((mod & (KMOD_LCTRL | KMOD_RCTRL)) != 0) {
			theApp.StopSession();
		}
		break;
	case SDLK_x:
		if ((mod & (KMOD_LCTRL | KMOD_RCTRL)) != 0) {
			// don't know how to do this yet...
		}
		break;
	case SDLK_UP:
		volume = mp4if->get_audio_volume();
		volume += 10;
		if (volume > 100) volume = 100;
		config.set_config_value(CONFIG_VOLUME, volume);
		mp4if->set_audio_volume(volume);
		break;
	case SDLK_DOWN:
		volume = mp4if->get_audio_volume();
		volume -= 10;
		if (volume < 0) volume = 0;
		config.set_config_value(CONFIG_VOLUME, volume);
		mp4if->set_audio_volume(volume);
		break;
	case SDLK_SPACE:
		if (mp4if->get_state() == MP4IF_STATE_PLAY) {
			mp4if->pause();
		} else {
			mp4if->play();
		}
		break;
	case SDLK_END:
		// They want the end - just close, or go on to the next playlist.
		theApp.StopSession();
		break;
	case SDLK_HOME:
		mp4if->seek_to(0.0);
		break;
	case SDLK_RIGHT:
		if (mp4if->is_seekable()) {
			uint64_t play_time;
			
			if (mp4if->get_current_time(&play_time) == FALSE) return 0;

			double ptime, maxtime;
			play_time += 10 * M_64;
			ptime = (double)
#ifdef _WIN32
				(int64_t)
#endif
				play_time;
			ptime /= 1000.0;
			maxtime = mp4if->get_max_time();
			if (ptime < maxtime) {
				mp4if->seek_to(ptime);
			}
		}
		break;
	case SDLK_LEFT:
		if (mp4if->is_seekable()) {
			uint64_t play_time;
			
			if (mp4if->get_current_time(&play_time) == FALSE) return 0;
			double ptime;
			if (play_time > 10 * M_64) {
				play_time -= 10 * M_64;
				ptime = (double)
#ifdef _WIN32
					(int64_t)
#endif
					play_time;
				ptime /= 1000.0;
			} else ptime = 0.0;
			mp4if->seek_to(ptime);
		}
		break;
	case SDLK_PAGEUP:
		screen_size = mp4if->get_screen_size();
		if (screen_size < 2 && mp4if->get_fullscreen_state() == FALSE) {
			screen_size++;
			theApp.OnMediaVideo(screen_size + ID_MEDIA_VIDEO_50);
		}
		break;
	case SDLK_PAGEDOWN:
		screen_size = mp4if->get_screen_size();
		if (screen_size >= 1 && mp4if->get_fullscreen_state() == 0) {
			screen_size--;
			theApp.OnMediaVideo(screen_size + ID_MEDIA_VIDEO_50);
		}
		break;
	case SDLK_RETURN:
		if ((mod & (KMOD_LALT | KMOD_RALT)) != 0) {
			mp4if->set_fullscreen_state(TRUE);
		}
		break;
	case SDLK_ESCAPE:
		mp4if->set_fullscreen_state(FALSE);
		break;
	default:
		break;
	}	
	return 0;
}
