#include "mp4live.h"
#include "media_flow.h"
#include "preview_flow.h"
#include "mp4live_common.h"
#include "video_sdl_preview.h"

void CPreviewAVMediaFlow::Start (void)
{
  CAVMediaFlow::Start();
  if (m_videoPreview == NULL) {
    m_videoPreview = new CSDLVideoPreview();
    m_videoPreview->SetConfig(m_pConfig);
    m_videoPreview->StartThread();
    m_videoPreview->Start();
    if (m_videoSource) {
      m_videoSource->AddSink(m_videoPreview);
    }
  }
}


void CPreviewAVMediaFlow::StartVideoPreview(void)
{
	if (m_pConfig == NULL) {
	  debug_message("pconfig is null");
		return;
	}

	if (!m_pConfig->IsCaptureVideoSource()) {
	  debug_message("Not capture source");
		return;
	}


	if (m_videoSource == NULL) {
          m_videoSource = CreateVideoSource(m_pConfig);
	}
	if (m_videoPreview == NULL) {
		m_videoPreview = new CSDLVideoPreview();
		m_videoSource->AddSink(m_videoPreview);
		m_videoPreview->SetConfig(m_pConfig);
		m_videoPreview->StartThread();
	} else {
	  m_videoSource->AddSink(m_videoPreview);
	}

	m_videoSource->StartVideo();
	m_videoPreview->Start();
}

void CPreviewAVMediaFlow::StopVideoPreview(void)
{
	if (!m_pConfig->IsCaptureVideoSource()) {
		return;
	}

	if (m_videoSource) {
		if (!m_started) {
			m_videoSource->StopThread();
			delete m_videoSource;
			m_videoSource = NULL;
		} else {
			m_videoSource->Stop();
		}
	}

	if (m_videoPreview) {
		if (!m_started) {
			m_videoPreview->StopThread();
			delete m_videoPreview;
			m_videoPreview = NULL;
		} else {
			m_videoPreview->Stop();
		}
	}
}
