#include "mp4live.h"
#include "media_flow.h"
#include "preview_flow.h"
#include "mp4live_common.h"
#include "video_sdl_preview.h"
#include "video_encoder.h"

void CPreviewAVMediaFlow::Start (bool startvideo, 
				 bool startaudio,
				 bool starttext)
{
  // If we're getting an encoded preview, we're going to delete the
  // encoder, then recreate the preview below
  if (m_PreviewEncoder != NULL) {
    m_videoSource->RemoveSink(m_PreviewEncoder);
    m_PreviewEncoder->StopThread();
    m_PreviewEncoder->RemoveSink(m_videoPreview);
    delete m_PreviewEncoder;
    m_video_encoder_list = NULL;
    m_PreviewEncoder = NULL;
  }
    
  CAVMediaFlow::Start(startvideo, startaudio, starttext);

  if (m_pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW) && 
      startvideo) {
    CreatePreview();
    ConnectPreview(false);
  }
}

// DisconnectPreview - remove the preview from the feeder it
// was attached to - either the video source, or the encoder
void CPreviewAVMediaFlow::DisconnectPreview (void)
{
  debug_message("disconnecting preview");
  if (m_videoPreview == NULL) {
    return;
  }
  if (m_PreviewEncoder != NULL) {
    m_PreviewEncoder->StopPreview();
    m_PreviewEncoder->RemoveSink(m_videoPreview);
  } else if (m_videoSource != NULL) {
    m_videoSource->RemoveSink(m_videoPreview);
  }
}

// CreatePreview - create it if it does not exist
void CPreviewAVMediaFlow::CreatePreview (void)
{
  if (m_videoPreview == NULL) {
    debug_message("Creating preview");
      m_videoPreview = new CSDLVideoPreview();
      m_videoPreview->SetConfig(m_pConfig);
      m_videoPreview->StartThread();
      m_videoPreview->Start();
  } 
}

// ConnectPreview - Connect the video preview sink to the correct
// feeder, encoder or video source.
// We will create the encoder if it doesn't exist, if we say
// to create it (we won't say to create it if we're being called
// from Start()
void CPreviewAVMediaFlow::ConnectPreview (bool create_encoder) 
{
  const char *stream_name = 
    m_pConfig->GetStringValue(CONFIG_VIDEO_PREVIEW_STREAM);
  if (stream_name == NULL) {
    if (m_videoSource) {
      m_videoSource->AddSink(m_videoPreview);
      debug_message("preview connected to raw source");
    }
  } else {
    CMediaStream *stream = m_stream_list->FindStream(stream_name);
    m_PreviewEncoder = FindOrCreateVideoEncoder(stream->GetVideoProfile(),
						create_encoder);
    if (m_PreviewEncoder != NULL) {
      m_PreviewEncoder->AddSink(m_videoPreview);
      m_PreviewEncoder->StartPreview();
      debug_message("preview connected to stream %s", stream_name);
    }
  }
}

// Stop when we're previewed will disconnect the preview, stop the
// flow, then restart the preview.
void CPreviewAVMediaFlow::Stop (void) 
{
  debug_message("preview stop");
  DisconnectPreview();
  CAVMediaFlow::Stop();
  m_PreviewEncoder = NULL; // stopped above
  debug_message("restarting video preview");
  StartVideoPreview();
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

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW) == false) {
	  debug_message("preview is false");
	  if (m_videoSource != NULL) {
	    m_videoSource->Stop();
	  }
	  return;
	}

	StopVideoPreview(false);

	if (m_videoSource == NULL) {
          m_videoSource = CreateVideoSource(m_pConfig);
	}
       
	CreatePreview();

	ConnectPreview(true);

	m_videoSource->Start();
	m_videoPreview->Start();
}

void CPreviewAVMediaFlow::StopVideoPreview (bool delete_it)
{
  debug_message("Stopping preview %u", delete_it);

	if (!m_pConfig->IsCaptureVideoSource()) {
		return;
	}

	DisconnectPreview();
	
	debug_message("done disconnect");
	if (m_PreviewEncoder != NULL) {
	  if (m_started == false) {
	    if (m_videoSource != NULL)
	      m_videoSource->RemoveSink(m_PreviewEncoder);
	    m_PreviewEncoder->StopThread();
	    delete m_PreviewEncoder;
	    m_video_encoder_list = NULL;
	  }
	  m_PreviewEncoder = NULL;
	}

	if (m_videoSource) {
		if (m_started == false) {
		  m_videoSource->StopThread();
		  delete m_videoSource;
		  m_videoSource = NULL;
		} 
	}

	if (m_videoPreview && delete_it) {
	  m_videoPreview->StopThread();
	  delete m_videoPreview;
	  m_videoPreview = NULL;
	}
}

bool CPreviewAVMediaFlow::ProcessSDLEvents (bool process_quit)
{
  bool ret = false;

  if (m_videoPreview == NULL) 
    return false;

  SDL_Event event;
  while (SDL_PollEvent(&event) > 0) {
    switch (event.type) {
    case SDL_QUIT:
      if (process_quit)
	StopVideoPreview();
      else
	ret = true;
      break;
    }
  }
  return ret;
}

void CPreviewAVMediaFlow::RestartVideo (void)
{
  bool have_preview = m_videoPreview != NULL;
  debug_message("in restart video");
  if (have_preview || m_started) {
    if (m_videoSource != NULL) {
      m_videoSource->Stop();
      m_videoSource->Start();
    }
  }
  if (have_preview) {
    StartVideoPreview();
  }
}
