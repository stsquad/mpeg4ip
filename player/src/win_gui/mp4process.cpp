#include "stdafx.h"
#include <windows.h>
#include "wmp4player.h"
#include "mp4process.h"
#include "our_msg_queue.h"


static DWORD __stdcall c_receive_thread (LPVOID lpThreadParameter)
{
	CMP4Process *mp4 = (CMP4Process *)lpThreadParameter;

	mp4->receive_thread();
	ExitThread(0);
	return 0;
}

CMP4Process::CMP4Process(CString &name)
{
	init();
	start_process(name); 
}

CMP4Process::~CMP4Process(void)
{
	m_terminating = 1;
	if (kill_process() == FALSE) {
		TerminateProcess(m_piProcInfo.hProcess, -1);
	}
	clean_up_process();
}

void CMP4Process::init(void)
{
	m_terminating = 0;
	m_receive_thread = NULL;
	m_thread_created = FALSE;
	m_map_file_write = NULL;
	m_to_client_event = NULL;
	m_to_client_resp_event = NULL;
	m_from_client_event = NULL;
	m_from_client_resp_event = NULL;
	m_map_file = NULL;
	ZeroMemory(&m_piProcInfo, sizeof(m_piProcInfo));
	// Set up security attributes for all handles/files
	m_saAttr.nLength = sizeof(m_saAttr);
    m_saAttr.bInheritHandle = TRUE;
    m_saAttr.lpSecurityDescriptor = NULL;
}

BOOL CMP4Process::start_process (CString &name)
{
	if (m_thread_created != FALSE) {
		return FALSE;
	}
   ZeroMemory( &m_siStartInfo, sizeof(STARTUPINFO) );
   m_siStartInfo.cb = sizeof(STARTUPINFO); 

	m_map_file = CreateFileMapping((HANDLE)0xFFFFFFFF,
									 NULL,
									 PAGE_READWRITE,
									 0,
									 2048,
									 "MP4ThreadToEventFileMap");
 
	if (m_map_file == NULL) {
		OutputDebugString("Can't CreateFileMapping\n");
		clean_up_process();
		return FALSE;
	}
	m_map_file_write = MapViewOfFile(m_map_file,
									FILE_MAP_ALL_ACCESS,
									0, 0, 0);
	if (m_map_file_write == NULL) {
		OutputDebugString("Can't MapViewOfFile");
		clean_up_process();
		return FALSE;
	}

   m_to_client_event = 
	   CreateEvent(&m_saAttr, FALSE, FALSE, "MP4GuiToClient");

   m_from_client_event = 
	   CreateEvent(&m_saAttr, FALSE, FALSE, "MP4ClientToGui");

   m_to_client_resp_event = 
	   CreateEvent(&m_saAttr, FALSE, FALSE, "MP4GuiToClientResp");

   m_from_client_resp_event = 
	   CreateEvent(&m_saAttr, FALSE, FALSE, "MP4ClientToGuiResp");

   m_stop_receive_thread = 0;
   
   m_receive_thread = CreateThread(NULL, 0, c_receive_thread, this, 
								   0, &m_receive_thread_id);
#if 0
	char path[MAX_PATH];
	path[0] = '\0';
	if (GetModuleFileName(NULL, path, MAX_PATH) > 0) {
		char *end = strrchr(path, '\\');
		*end = '\0';
	} 
#endif
   char buffer[512];
   _snprintf(buffer, sizeof(buffer), "wmp4client.exe \"%s\"", 
	   name);
   m_thread_created = CreateProcess(
	   "wmp4client.exe",
		buffer,       // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&m_siStartInfo,  // STARTUPINFO pointer 
		&m_piProcInfo);  // receives PROCESS_INFORMATION 
   if (m_thread_created == 0) {
	   char outbuffer[512];
	   _snprintf(outbuffer, 512, "Thread error is %d\n%s", GetLastError(), buffer);
	   AfxMessageBox(buffer);
   }
   return m_thread_created;
}

int CMP4Process::get_initial_response (msg_initial_resp_t *init_resp,
										CString &errmsg)
{
	int ret;
	ret = WaitForSingleObject(m_to_client_resp_event, 30 * 1000);
	if (ret != WAIT_OBJECT_0) {
		errmsg = "No response from client process";
	} else {
		msg_mbox_t *pmbox = (msg_mbox_t *)m_map_file_write;
		memcpy(init_resp, 
			   pmbox->to_client_mbox, 
			   sizeof(msg_initial_resp_t));
		errmsg = ((char *)pmbox->to_client_mbox) + sizeof(msg_initial_resp_t);

		return pmbox->from_client_response;
	}
	return -1;
}

#define CLOSE_AND_CLEAR(a) if ((a) != NULL) { CloseHandle(a); (a) = NULL; }
void CMP4Process::clean_up_process(void)
{
	if (m_receive_thread != NULL) {
		m_stop_receive_thread = 1;
		SetEvent(m_from_client_event);
        SetThreadPriority(m_receive_thread, THREAD_PRIORITY_HIGHEST); 
        WaitForSingleObject(m_receive_thread, INFINITE);
		CloseHandle(m_receive_thread);
		m_receive_thread = NULL;
	}
            
	if (m_thread_created) {
		CLOSE_AND_CLEAR(m_to_client_event);
		CLOSE_AND_CLEAR(m_to_client_resp_event);
		CLOSE_AND_CLEAR(m_from_client_event);
		CLOSE_AND_CLEAR(m_from_client_resp_event);
		CLOSE_AND_CLEAR(m_piProcInfo.hThread);
		CLOSE_AND_CLEAR(m_piProcInfo.hProcess);
		if (m_map_file_write != NULL) {
			UnmapViewOfFile(m_map_file_write);
			m_map_file_write = NULL;
		}

		CLOSE_AND_CLEAR(m_map_file);
		m_thread_created = FALSE;
	}
}
	

bool CMP4Process::send_message (unsigned __int32 message,
							   unsigned __int64 *retval,
							   const char *msg_body,
							   int msg_len)
{
	msg_mbox_t *pmbox = (msg_mbox_t *)m_map_file_write;

	pmbox->to_client_command = message;
	if (msg_body != NULL) {
		memcpy(pmbox->to_client_mbox,
			msg_body,
			MIN(msg_len, MBOX_SIZE));
	}
	pmbox->from_client_response = MBOX_RESP_EMPTY;
	ResetEvent(m_to_client_resp_event);
	SetEvent(m_to_client_event);

	// Now wait for response.
	WaitForSingleObject(m_to_client_resp_event, 2 * 1000);
	if (pmbox->from_client_response != MBOX_RESP_EMPTY) {
		if (retval != NULL) *retval = pmbox->from_client_response;
		return (TRUE);
	}
	return (FALSE);
}

bool CMP4Process::kill_process (void)
{
	bool ret;
	DWORD timeout;
	unsigned __int64 ret_from_client;

	if (m_thread_created == FALSE) 
		return TRUE;

	ret = send_message(CLIENT_MSG_TERMINATE, &ret_from_client);
	if (ret == FALSE) {
		return FALSE;
	}
	timeout = WaitForSingleObject(m_piProcInfo.hProcess, 10 * 1000);
	if (timeout != WAIT_OBJECT_0) {
		OutputDebugString("Process didn't terminate\n");
		return FALSE;
	}
	clean_up_process();
	return TRUE;
}

void CMP4Process::receive_thread (void)
{
	HANDLE wait_handle[2];
	int ret;

	msg_mbox_t *pmbox = (msg_mbox_t *)m_map_file_write;
	while (m_stop_receive_thread == 0) {
		wait_handle[0] = m_from_client_event;
		wait_handle[1] = m_piProcInfo.hProcess;

		ret = WaitForMultipleObjects(2,
									 wait_handle,
									 FALSE,
									 INFINITE);
		switch (ret) {
		case WAIT_OBJECT_0:
			// This sends messages to theApp
			char buffer[80];
			sprintf(buffer, "Received Message %x from client\n", 
				pmbox->to_gui_command);
			OutputDebugString(buffer);
			switch (pmbox->to_gui_command) {
			case GUI_MSG_RECEIVED_QUIT:
				// Don't use this - release version crashes.
				//theApp.PostThreadMessage(WM_CLOSED_SESSION, 0, 0);
				theApp.m_pMainWnd->PostMessage(WM_CLOSED_SESSION);
				break;
			case GUI_MSG_SDL_KEY:
				const sdl_event_msg_t *ptr;
				ptr = (const sdl_event_msg_t *)pmbox->to_gui_mbox;
				((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveView()->PostMessage(WM_SDL_KEY,
					ptr->sym, ptr->mod);
				break;
			case GUI_MSG_SESSION_WARNING:
			case GUI_MSG_SESSION_ERROR:
				const char *p;
				p = strdup((const char *)pmbox->to_gui_mbox);
				theApp.m_pMainWnd->PostMessage(
					pmbox->to_gui_command == GUI_MSG_SESSION_WARNING ?
                    WM_SESSION_WARNING : WM_SESSION_ERROR, (WPARAM)p , 0);
				break;
					
			default:
				//theApp.PostThreadMessage(WM_USER, 0, 0);
				break;
			}
			pmbox->to_gui_command = BLANK_MSG;
			SetEvent(m_from_client_resp_event);
			break;
		case WAIT_OBJECT_0 + 1:
			if (m_terminating == 0)
				theApp.m_pMainWnd->PostMessage(WM_SESSION_DIED);
			break;
		default:
			TRACE2("Illegal code %d received in receive thread %u\n", 
				    ret, GetLastError());
			break;
		}
#if 0
((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveView()->PostMessage(WM_CLOSED_SESSION);		// This sends messages to MainFrm
		if (!theApp.m_pMainWnd->PostMessage(WM_USER)) {
			DWORD error = GetLastError();
			TRACE1("Can't post message - error %d\n", error);
		}
#endif
	}
	OutputDebugString("Exiting process receive thread\n");
	ExitThread(0);
}
