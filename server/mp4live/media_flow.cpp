/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include "media_flow.h"
#include "audio_oss_source.h"
#include "video_v4l_source.h"

#include "file_mp4_recorder.h"
#include "rtp_transmitter.h"
#include "file_raw_sink.h"
#include "mp4live_common.h"
#include "video_encoder.h"
#include "audio_encoder.h"
#include "text_encoder.h"
#include "text_encoder.h"
#include "media_stream.h"
#include "profile_video.h"
#include "profile_audio.h"
#include "profile_text.h"
#include "text_source.h"


CAVMediaFlow::CAVMediaFlow(CLiveConfig* pConfig)
{
  m_pConfig = pConfig;
  m_started = false;

  m_videoSource = NULL;
  m_audioSource = NULL;
  m_textSource = NULL;
  m_mp4RawRecorder = NULL;
  m_rawSink = NULL;
  m_video_profile_list = NULL;
  m_audio_profile_list = NULL;
  m_text_profile_list = NULL;
  m_stream_list = NULL;
  m_video_encoder_list = NULL;
  m_audio_encoder_list = NULL;
  m_text_encoder_list = NULL;
  ReadStreams();
  ValidateAndUpdateStreams();
}

CAVMediaFlow::~CAVMediaFlow() 
{
  CAVMediaFlow::Stop();
  if (m_videoSource != NULL) {
    m_videoSource->StopThread();
    delete m_videoSource;
    m_videoSource = NULL;
  }
  if (m_stream_list != NULL) {
    delete m_stream_list;
    m_stream_list = NULL;
  }
  if (m_video_profile_list != NULL) {
    delete m_video_profile_list;
    m_video_profile_list = NULL;
  }
  if (m_audio_profile_list != NULL) {
    delete m_audio_profile_list;
    m_audio_profile_list = NULL;
  }
  if (m_text_profile_list != NULL) {
    delete m_text_profile_list;
    m_text_profile_list = NULL;
  }
}


void CAVMediaFlow::Stop(void)
{
	if (!m_started) {
		return;
	}
	//debug_message("media flow - stop");
	bool oneSource = (m_audioSource == m_videoSource);

	// Stop the sources
	if (m_audioSource) {
	  m_audioSource->RemoveAllSinks();
		m_audioSource->StopThread();
		delete m_audioSource;
		m_audioSource = NULL;
	}

	if (!m_pConfig->IsCaptureVideoSource()) {
		if (m_videoSource && !oneSource) {
		  debug_message("Stopping video source thread");
		  m_videoSource->RemoveAllSinks();
			m_videoSource->StopThread();
			delete m_videoSource;
		}
		m_videoSource = NULL;

	}
	if (m_textSource) {
	  m_textSource->RemoveAllSinks();
	  m_textSource->StopThread();
	  delete m_textSource;
	  m_textSource = NULL;
	  debug_message("Text stopped");
	}

	// stop the encoders - remove the sinks from the source
	// This will stop the sinks, and deletes the rtp destinations
	CMediaCodec *mc = m_video_encoder_list, *p;
	while (mc != NULL) {
	  //debug_message("stopping video profile %s", mc->GetProfileName());
	  if (m_videoSource != NULL) {
	    m_videoSource->RemoveSink(mc);
	  }
	  //	  debug_message("sinks removed");
	  mc->StopThread();
	  //debug_message("thread_stopped");
	  mc->StopSinks();
	  //debug_message("sinks stopped");
	  p = mc;
	  mc = mc->GetNext();
	  delete p;
	}
	m_video_encoder_list = NULL;
	mc = m_audio_encoder_list;
	while (mc != NULL) {
	  // debug_message("stopping audio profile %s", mc->GetProfileName());
	  mc->StopThread();
	  //debug_message("thread stopped");
	  mc->StopSinks();
	  //debug_message("stopping sinks");
	  p = mc;
	  mc = mc->GetNext();
	  delete p;
	}
	m_audio_encoder_list = NULL;

	mc = m_text_encoder_list;
	while (mc != NULL) {
	   debug_message("stopping text profile %s", mc->GetProfileName());
	  mc->StopThread();
	  debug_message("thread stopped");
	  mc->StopSinks();
	  debug_message("stopping sinks");
	  p = mc;
	  mc = mc->GetNext();
	  delete p;
	}
	m_text_encoder_list = NULL;

	// Now, stop the streams (stops and deletes file recorders
	CMediaStream *stream;
	stream = m_stream_list->GetHead();
	while (stream != NULL) {
	  stream->Stop();
	  stream = stream->GetNext();
	}

	// Note - m_videoSource may still be active at this point
	// be sure to remove any sinks attached to it.

	// any other sinks that were added should be stopped here.
	if (m_rawSink) {
	  if (m_videoSource != NULL) {
	    m_videoSource->RemoveSink(m_rawSink);
	  }
		m_rawSink->StopThread();
		delete m_rawSink;
		m_rawSink = NULL;
	}
	
	
	if (m_mp4RawRecorder != NULL) {
	  if (m_videoSource != NULL) {
	    m_videoSource->RemoveSink(m_mp4RawRecorder);
	  }
	  m_mp4RawRecorder->StopThread();
	  delete m_mp4RawRecorder;
	  m_mp4RawRecorder = NULL;
	}
	
	m_started = false;
	
}


void CAVMediaFlow::SetPictureControls(void)
{
	if (m_videoSource) {
		m_videoSource->SetPictureControls();
	}
}


void CAVMediaFlow::SetAudioOutput(bool mute)
{
	static int muted = 0;
	static int lastVolume;

	const char* mixerName = 
		m_pConfig->GetStringValue(CONFIG_AUDIO_MIXER_NAME);

	int mixer = open(mixerName, O_RDONLY);

	if (mixer < 0) {
		error_message("Couldn't open mixer %s", mixerName);
		return;
	}

	if (mute) {
		ioctl(mixer, SOUND_MIXER_READ_LINE, &lastVolume);
		ioctl(mixer, SOUND_MIXER_WRITE_LINE, &muted);
	} else {
		int newVolume;
		ioctl(mixer, SOUND_MIXER_READ_LINE, &newVolume);
		if (newVolume == 0) {
			ioctl(mixer, SOUND_MIXER_WRITE_LINE, &lastVolume);
		}
	}

	close(mixer);
}

bool CAVMediaFlow::GetStatus(u_int32_t valueName, void* pValue) 
{
	CMediaSource* source = NULL;
	if (m_videoSource) {
		source = m_videoSource;
	} else if (m_audioSource) {
		source = m_audioSource;
	} else if (m_textSource) {
	  source = m_textSource;
	}

	switch (valueName) {
	case FLOW_STATUS_DONE: 
		{
		bool done = true;
		if (m_videoSource) {
			done = m_videoSource->IsDone();
		}
		if (m_audioSource) {
			done = (done && m_audioSource->IsDone());
		}
		if (m_textSource) {
		  done = (done && m_textSource->IsDone());
		}
		*(bool*)pValue = done;
		}
		break;
	case FLOW_STATUS_PROGRESS:
		if (source) {
			*(float*)pValue = source->GetProgress();
		} else {
			*(float*)pValue = 0.0;
		}
		break;
	default:
	  return false;
	}
	return true;
}

// CheckandCreateDir - based on name, check if directory exists
// if not, create it.  If so, check that it is a directory
static bool CheckandCreateDir (const char *name)
{
  struct stat statbuf;

  if (stat(name, &statbuf) == 0) {
    if (!S_ISDIR(statbuf.st_mode)) {
      error_message("%s is not a directory", name);
      return false;
    }
  } else {
    if (mkdir(name, S_IRWXU) < 0) {
      error_message("can't create directory \"%s\" - %s", 
		    name, strerror(errno));
      return false;
    }
  }
  return true;
}
  

/*
 * Check dup - checks for duplicates in media stream addresses.
 */
static bool CheckDup (CMediaStream *orig, 
		      CMediaStream *check,
		      config_index_t profile_ix,
		      config_index_t orig_addr_ix,
		      config_index_t orig_port_ix,
		      config_index_t check_profile_ix,
		      config_index_t check_addr_ix,
		      config_index_t check_port_ix)
{
  // see if the address and port match
  if (orig->GetIntegerValue(orig_port_ix) == 
      check->GetIntegerValue(check_port_ix) &&
      strcmp(orig->GetStringValue(orig_addr_ix),
	     orig->GetStringValue(check_addr_ix)) == 0) {
    // we have the same address:port
    if (profile_ix == check_profile_ix) {
      // See if the profiles match - if they do, no dup
      if (strcmp(orig->GetStringValue(profile_ix),
		 check->GetStringValue(check_profile_ix)) == 0) {
	return false;
      }
    } 
    // we have an address:port match, and we are not on the same profile
    return true;
  }
  return false;
}

// CheckAddressForDup - check audio, video and text addresses, unless the
// stream matches.
bool CheckAddressForDup (CMediaStreamList *stream_list, 
			 CMediaStream *s,
			 config_index_t profile_ix, 
			 config_index_t addr_ix, 
			 config_index_t port_ix)

{
  CMediaStream *p = stream_list->GetHead();

  while (p != NULL) {
    if (p != s) {
      if (CheckDup(s, p,
		   profile_ix, addr_ix, port_ix, 
		   STREAM_VIDEO_PROFILE, STREAM_VIDEO_DEST_ADDR, STREAM_VIDEO_DEST_PORT)) {
	return true;
      }
      if (CheckDup(s, p,
		   profile_ix, addr_ix, port_ix, 
		   STREAM_AUDIO_PROFILE, STREAM_AUDIO_DEST_ADDR, STREAM_AUDIO_DEST_PORT)) {
	return true;
      }
      if (CheckDup(s, p,
		   profile_ix, addr_ix, port_ix, 
		   STREAM_TEXT_PROFILE, STREAM_TEXT_DEST_ADDR, STREAM_TEXT_DEST_PORT)) {
	return true;
      }
    }
    p = p->GetNext();
  }
  return false;
}

// SetRestOfProfile - all streams that have matching profiles and do
// not have fixed addresses to the same address:port
void SetRestOfProfile (CMediaStream *s, 
		       config_index_t profile,
		       config_index_t fixed,
		       config_index_t addr,
		       config_index_t port)
{
  CMediaStream *p = s->GetNext();

  while (p != NULL) {
    if (strcmp(p->GetStringValue(profile), s->GetStringValue(profile)) == 0) {
      if (p->GetBoolValue(fixed) == false) {
	p->SetStringValue(addr, s->GetStringValue(addr));
	p->SetIntegerValue(port, s->GetIntegerValue(port));
      }
    }
    p = p->GetNext();
  }
}

// ValidateAddressAndPort does a number of things - verifies that the
// address and port are in a valid range, set all streams with similiar
// profiles to that address, and makes sure there are no duplicate addresses
void ValidateIpAddressAndPort (CMediaStreamList *stream_list, 
			       CMediaStream *s,
			       config_index_t profile_ix, 
			       config_index_t fixed_ix,
			       config_index_t addr_ix, 
			       config_index_t port_ix)
{
  bool corrected;
  debug_message("Checking %s %s %s %s:%u",
		s->GetName(),
		s->GetNameFromIndex(profile_ix),
		s->GetStringValue(profile_ix),
		s->GetStringValue(addr_ix),
		s->GetIntegerValue(port_ix));
  do {
    if (ValidateIpAddress(s->GetStringValue(addr_ix)) == false) {
      struct in_addr in;
      debug_message("Stream %s %s address was invalid \"%s\"",
		    s->GetName(),
		    s->GetNameFromIndex(profile_ix),
		    s->GetStringValue(addr_ix));
      in.s_addr = GetRandomMcastAddress();
      s->SetStringValue(addr_ix, inet_ntoa(in));
      debug_message("changed to \"%s\"", s->GetStringValue(addr_ix));
    }
    
    if (s->GetIntegerValue(port_ix) >= 0xffff ||
	ValidateIpPort(s->GetIntegerValue(port_ix)) == false) {
      debug_message("Stream %s %s address was invalid %u",
		    s->GetName(),
		    s->GetNameFromIndex(profile_ix),
		    s->GetIntegerValue(port_ix));
      s->SetIntegerValue(port_ix, GetRandomPort());
      debug_message("Changed to %u", s->GetIntegerValue(port_ix));
    }

    if (s->GetBoolValue(fixed_ix) == false) {
      SetRestOfProfile(s, profile_ix, fixed_ix, addr_ix, port_ix);
    }
    corrected = CheckAddressForDup(stream_list, 
				   s,
				   profile_ix,
				   addr_ix, 
				   port_ix);
    if (corrected == true) {
      // keep address, move port - by setting port to 0, we will trigger
      // the above 
      s->SetIntegerValue(port_ix, 0);
    }
  } while (corrected);
}

// ReadStreams - read all the profiles (audio and video), then reads
// the streams
bool CAVMediaFlow::ReadStreams (void)
{
  char base[PATH_MAX]; 
  const char *d;
  d = m_pConfig->GetStringValue(CONFIG_APP_PROFILE_DIRECTORY);
  if (d != NULL) {
    strcpy(base, d);
  } else {
    GetHomeDirectory(base);
    strcat(base, ".mp4live_d");
  }
  if (CheckandCreateDir(base) == false) 
    return false;

  // Load video profiles  Make sure there is a "default" profile
  char profile_dir[PATH_MAX];
  snprintf(profile_dir, PATH_MAX, "%s/Video", base);
  if (CheckandCreateDir(profile_dir) == false) {
    return false;
  }
  m_video_profile_list = new CVideoProfileList(profile_dir);
  m_video_profile_list->Load();
  if (m_video_profile_list->GetCount() == 0 ||
      m_video_profile_list->FindProfile("default") == NULL) {
    if (m_video_profile_list->CreateConfig("default") == false) {
      error_message("Can't create video default profile");
      return false;
    } else {
      debug_message("Created default video profile");
    }
  }

  snprintf(profile_dir, PATH_MAX, "%s/Audio", base);
  if (CheckandCreateDir(profile_dir) == false) {
    return false;
  }
  // load audio profiles - make sure there is a default profile
  m_audio_profile_list = new CAudioProfileList(profile_dir);
  m_audio_profile_list->Load();
  if (m_audio_profile_list->GetCount() == 0 ||
      m_audio_profile_list->FindProfile("default") == NULL) {
    if (m_audio_profile_list->CreateConfig("default") == false) {
      error_message("Can't create audio default profile");
      return false;
    } else {
      debug_message("Created default audio profile");
    }
  }

  snprintf(profile_dir, PATH_MAX, "%s/Text", base);
  if (CheckandCreateDir(profile_dir) == false) {
    return false;
  }
  // load text profiles - make sure there is a default profile
  m_text_profile_list = new CTextProfileList(profile_dir);
  m_text_profile_list->Load();
  if (m_text_profile_list->GetCount() == 0 ||
      m_text_profile_list->FindProfile("default") == NULL) {
    if (m_text_profile_list->CreateConfig("default") == false) {
      error_message("Can't create text default profile");
      return false;
    } else {
      debug_message("Created default text profile");
    }
  }

  // load streams
  d = m_pConfig->GetStringValue(CONFIG_APP_STREAM_DIRECTORY);
  if (d != NULL) {
    strcpy(base, d);
  } else {
    GetHomeDirectory(base);
    strcat(base, ".mp4live_d/Streams");
  }
  if (CheckandCreateDir(base) == false) {
    return false;
  }
  m_stream_list = new CMediaStreamList(base,
				       m_video_profile_list,
				       m_audio_profile_list,
				       m_text_profile_list);
  m_stream_list->Load();
  if (m_stream_list->GetCount() == 0) {
    if (m_stream_list->CreateConfig("default") == false) {
      error_message("Can't create default stream");
    } else {
      debug_message("Created default stream");
    }
  }
  return true;
}

// ValidateAndUpdateStreams is called when a change is made to a 
// profile, or a new profile is selected in a stream
// It will check the addresses, and make sure that the source has the
// "max" values
void CAVMediaFlow::ValidateAndUpdateStreams (void)
{

  // Check the streams addresses
  CMediaStream *s = m_stream_list->GetHead();
  while (s != NULL) {
    ValidateIpAddressAndPort(m_stream_list, s, 
			     STREAM_VIDEO_PROFILE, STREAM_VIDEO_ADDR_FIXED,
			     STREAM_VIDEO_DEST_ADDR, STREAM_VIDEO_DEST_PORT);
    ValidateIpAddressAndPort(m_stream_list, s, 
			     STREAM_AUDIO_PROFILE, STREAM_AUDIO_ADDR_FIXED,
			     STREAM_AUDIO_DEST_ADDR, STREAM_AUDIO_DEST_PORT);
    ValidateIpAddressAndPort(m_stream_list, s, 
			     STREAM_TEXT_PROFILE, STREAM_TEXT_ADDR_FIXED,
			     STREAM_TEXT_DEST_ADDR, STREAM_TEXT_DEST_PORT);

    s = s->GetNext();
  }

  s = m_stream_list->GetHead();
  uint32_t max_w = 0, max_h = 0, max_sample_rate = 0, max_chans = 0;
  bool have_video = false, have_audio = false, have_text = false;
  while (s != NULL) {
    // get the maximum width and maximum height
    if (s->GetBoolValue(STREAM_VIDEO_ENABLED)) {
      have_video = true;
      max_w = 
	MAX(max_w, s->GetVideoProfile()->GetIntegerValue(CFG_VIDEO_WIDTH));
      max_h = 
	MAX(max_h, s->GetVideoProfile()->GetIntegerValue(CFG_VIDEO_HEIGHT));
    }
    // get the max channels and sampling rate
    if (s->GetBoolValue(STREAM_AUDIO_ENABLED)) {
      have_audio = true;
      uint32_t orig_rate = 
	s->GetAudioProfile()->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE);
      uint32_t ret_rate;
      ret_rate = m_pConfig->m_audioCapabilities->CheckSampleRate(orig_rate);
      if (ret_rate != orig_rate) {
	error_message("Audio Profile %s illegal sample rate %u changed to %u", 
		      s->GetAudioProfile()->GetName(), 
		      orig_rate, ret_rate);
	s->GetAudioProfile()->SetIntegerValue(CFG_AUDIO_SAMPLE_RATE, 
					      ret_rate);
      }
      max_sample_rate = MAX(max_sample_rate, ret_rate);
      max_chans = 
	MAX(max_chans, 
	    s->GetAudioProfile()->GetIntegerValue(CFG_AUDIO_CHANNELS));
    }
    if (s->GetBoolValue(STREAM_TEXT_ENABLED)) {
      have_text = true;
    }
    createStreamSdp(m_pConfig, s);
    s = s->GetNext();
  }
  m_pConfig->SetBoolValue(CONFIG_AUDIO_ENABLE, have_audio);
  m_pConfig->SetBoolValue(CONFIG_TEXT_ENABLE, have_text);
  m_pConfig->SetBoolValue(CONFIG_VIDEO_ENABLE, have_video);
  // streams should all be loaded.
  if (max_chans > 0) {
    if (m_pConfig->GetBoolValue(CONFIG_AUDIO_TAKE_STREAM_VALUES)) {
      m_pConfig->SetIntegerValue(CONFIG_AUDIO_CHANNELS,
				 max_chans);
    }
    if (m_pConfig->GetBoolValue(CONFIG_AUDIO_TAKE_STREAM_VALUES) ||
	max_sample_rate > m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE)) {
      m_pConfig->SetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE,
				 max_sample_rate);
    }
  }
  if (max_w != 0) {
    m_pConfig->SetIntegerValue(CONFIG_VIDEO_RAW_WIDTH,
			       max_w);
    m_pConfig->SetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT,
			       max_h);
  }
  m_pConfig->Update();
  m_pConfig->WriteToFile();
}

// Start
void CAVMediaFlow::Start (bool startvideo, 
			  bool startaudio,
			  bool starttext)
{
  if (m_pConfig == NULL) {
    return;
  }
  
  if (!startvideo && !startaudio && !starttext) return;

  // Create audio and video sources
  if (m_pConfig->GetBoolValue(CONFIG_AUDIO_ENABLE) &&
      m_audioSource == NULL &&
      startaudio) {
    m_audioSource = CreateAudioSource(m_pConfig, m_videoSource);
  }
  if (m_pConfig->GetBoolValue(CONFIG_TEXT_ENABLE) &&
      m_textSource == NULL &&
      starttext) {
    m_textSource = CreateTextSource(m_pConfig);
    debug_message("Created text source %p", m_textSource);
  }

  if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) && startvideo) {
    if (m_videoSource == NULL) {
      debug_message("start - creating video source");
      m_videoSource = CreateVideoSource(m_pConfig);
    }
    if (m_audioSource != NULL) {
      m_audioSource->SetVideoSource(m_videoSource);
    }
  }

  m_maxAudioSamplesPerFrame = 0;

  CMediaStream *s;
  s = m_stream_list->GetHead();

  // Create the components for each stream.  This make sure
  // that no more than 1 instance of a profile is being encoded.
  while (s != NULL) {
    CAudioEncoder *ae_ptr = NULL;
    CVideoEncoder *ve_ptr = NULL;
    CTextEncoder *te_ptr = NULL;
    if (s->GetBoolValue(STREAM_VIDEO_ENABLED) && startvideo) {
      // see if profile has already been started
      ve_ptr = FindOrCreateVideoEncoder(s->GetVideoProfile());
      s->SetVideoEncoder(ve_ptr);
      m_pConfig->SetBoolValue(CONFIG_VIDEO_ENABLE, true);
    }
    if (s->GetBoolValue(STREAM_AUDIO_ENABLED) && startaudio) {
      // see if profile has already been started
      ae_ptr = FindOrCreateAudioEncoder(s->GetAudioProfile());
      s->SetAudioEncoder(ae_ptr);
      m_pConfig->SetBoolValue(CONFIG_AUDIO_ENABLE, true);
      // when we start the encoder, we will have to pass the channels
      // configured, as well as the initial sample rate (basically, 
      // replicate SetAudioSrc here...
    }
    if (s->GetBoolValue(STREAM_TEXT_ENABLED) && starttext) {
      // see if profile has already been started
      te_ptr = FindOrCreateTextEncoder(s->GetTextProfile());
      s->SetTextEncoder(te_ptr);
      m_pConfig->SetBoolValue(CONFIG_TEXT_ENABLE, true);
    }
    if (s->GetBoolValue(STREAM_TRANSMIT)) {
      // check if transmitter has been started on encoder
      // create rtp destination, add to transmitter
      if (ve_ptr != NULL) {
	ve_ptr->AddRtpDestination(s,
				  m_pConfig->GetBoolValue(CONFIG_RTP_DISABLE_TS_OFFSET),
				  m_pConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL),
				  s->GetIntegerValue(STREAM_VIDEO_SRC_PORT));
      }
      if (ae_ptr != NULL) {
	ae_ptr->AddRtpDestination(s,
				  m_pConfig->GetBoolValue(CONFIG_RTP_DISABLE_TS_OFFSET),
				  m_pConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL),
				  s->GetIntegerValue(STREAM_AUDIO_SRC_PORT));
      }
      if (te_ptr != NULL) {
	te_ptr->AddRtpDestination(s,
				  m_pConfig->GetBoolValue(CONFIG_RTP_DISABLE_TS_OFFSET),
				  m_pConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL),
				  s->GetIntegerValue(STREAM_TEXT_SRC_PORT));
      }
      createStreamSdp(m_pConfig, s);
    }

    if (s->GetBoolValue(STREAM_RECORD)) {
      // create file sink, add to above encoders.
      CMediaSink *recorder = s->CreateFileRecorder(m_pConfig);
      if (ve_ptr != NULL) {
	ve_ptr->AddSink(recorder);
      }
      if (ae_ptr != NULL) {
	ae_ptr->AddSink(recorder);
      }
      if (te_ptr != NULL) {
	te_ptr->AddSink(recorder);
      }
    }
    s = s->GetNext();
  }
  
  if (m_audioSource) {
    m_audioSource->SetAudioSrcSamplesPerFrame(m_maxAudioSamplesPerFrame);
    debug_message("Setting source sample per frame %u", m_maxAudioSamplesPerFrame);
  }
  // If we need raw stuff, we do it here
  bool createdRaw = false;
  if (m_pConfig->GetBoolValue(CONFIG_RAW_ENABLE)) {
    if (m_rawSink == NULL) {
      m_rawSink = new CRawFileSink();
      m_rawSink->SetConfig(m_pConfig);
      createdRaw = true;
    }
    if (m_audioSource != NULL) {
      m_audioSource->AddSink(m_rawSink);
    }
    if (m_videoSource != NULL) {
      m_videoSource->AddSink(m_rawSink);
    }
    if (createdRaw) {
      m_rawSink->StartThread();
      m_rawSink->Start();
    }
  }

  if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4)) {
    if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_VIDEO) ||
	m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_AUDIO)) {
      bool createMp4Raw = false;
      if (m_mp4RawRecorder == NULL) {
	m_mp4RawRecorder = new CMp4Recorder(NULL);
	m_mp4RawRecorder->SetConfig(m_pConfig);
	createMp4Raw = true;
      }
      if (m_audioSource != NULL &&
	  m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_AUDIO)) {
	m_audioSource->AddSink(m_mp4RawRecorder);
      }
      if (m_videoSource != NULL &&
	  m_pConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4_VIDEO)) {
	m_videoSource->AddSink(m_mp4RawRecorder);
      }
      if (createMp4Raw) {
	m_mp4RawRecorder->StartThread();
	m_mp4RawRecorder->Start();
      }
    }
  }
  // start encoders and any sinks...  This may result in some sinks
  // file, in particular, receiving multiple starts
  CMediaCodec *mc = m_video_encoder_list;
  if (mc == NULL && m_videoSource != m_audioSource) {
    delete m_videoSource;
    m_videoSource = NULL;
  }
  while (mc != NULL) {
    mc->Start();
    mc->StartSinks();
    mc = mc->GetNext();
  }
  mc = m_audio_encoder_list;
  while (mc != NULL) {
    mc->Start();
    mc->StartSinks();
    mc = mc->GetNext();
  }
  mc = m_text_encoder_list;
  while (mc != NULL) {
    mc->Start();
    mc->StartSinks();
    mc = mc->GetNext();
  }
  // finally, start sources...
  if (m_videoSource && m_videoSource == m_audioSource) {
    m_videoSource->Start();
  } else {
    if (m_audioSource) {
      m_audioSource->Start();
    }
    if (m_videoSource) {
      m_videoSource->Start();
    }
  }
  
  if (m_textSource != NULL) {
    m_textSource->Start();
  }

  if (m_videoSource && startaudio) {
    // force video source to generate a key frame
    // so that sinks can quickly sync up
    m_videoSource->RequestKeyFrame(0);
  }
  
  m_started = true;
}

// FindOrCreateVideoEncoder - create a new encoder, or return
// one already on the list
CVideoEncoder *CAVMediaFlow::FindOrCreateVideoEncoder (CVideoProfile *vp,
						       bool create)
{
  const char *vp_name = vp->GetName();

  CVideoEncoder *ve_ptr = m_video_encoder_list;

  while (ve_ptr != NULL) {
    if (strcmp(vp_name, ve_ptr->GetProfileName()) == 0) {
      return ve_ptr;
    }
    ve_ptr = ve_ptr->GetNext();
  }
 
  if (create == false) return NULL;

  ve_ptr = 
    VideoEncoderCreate(vp, 
		       m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE), 
		       m_video_encoder_list /* TODO realTime */);
  m_video_encoder_list = ve_ptr;
  ve_ptr->StartThread();
  m_videoSource->AddSink(ve_ptr);
  debug_message("Added video encoder %s", vp_name);
  return ve_ptr;
}
  
CAudioEncoder *CAVMediaFlow::FindOrCreateAudioEncoder (CAudioProfile *ap)
{
  const char *ap_name = ap->GetName();
  CAudioEncoder *ae_ptr = m_audio_encoder_list;
 
  while (ae_ptr != NULL) {
    if (strcmp(ap_name, ae_ptr->GetProfileName()) == 0) {
      return ae_ptr;
    }
    ae_ptr = ae_ptr->GetNext();
  }
  
  ae_ptr = AudioEncoderCreate(ap, m_audio_encoder_list, 
			      m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS),
			      m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE),
			      m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE));
  m_audio_encoder_list = ae_ptr;
  // need to init, find max samples here..
  ae_ptr->Init();
  m_maxAudioSamplesPerFrame = MAX(m_maxAudioSamplesPerFrame, 
				  ae_ptr->GetSamplesPerFrame());

  ae_ptr->StartThread();
  m_audioSource->AddSink(ae_ptr);
  debug_message("Added audio encoder %s", ap_name);
  return ae_ptr;
}
CTextEncoder *CAVMediaFlow::FindOrCreateTextEncoder (CTextProfile *tp)
{
  const char *tp_name = tp->GetName();
  CTextEncoder *te_ptr = m_text_encoder_list;
 
  while (te_ptr != NULL) {
    if (strcmp(tp_name, te_ptr->GetProfileName()) == 0) {
      return te_ptr;
    }
    te_ptr = te_ptr->GetNext();
  }
  
  te_ptr = 
    TextEncoderCreate(tp, 
		      m_pConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE), 
		      m_text_encoder_list);

  m_text_encoder_list = te_ptr;
  // need to init, find max samples here..
  te_ptr->StartThread();
  m_textSource->AddSink(te_ptr);
  debug_message("Added text encoder %s", tp_name);
  return te_ptr;
}

// AddStream - called when we have a new stream from the GUI
bool CAVMediaFlow::AddStream (const char *name)
{
  if (m_started) {
    return false;
  }

  CMediaStream *ms = m_stream_list->FindStream(name);
  if (ms != NULL) {
    error_message("Stream %s already exists\n", name);
    return false;
  }
  if (m_stream_list->CreateConfig(name) == false) {
    return false;
  }
  ValidateAndUpdateStreams();
  return true;
}

bool CAVMediaFlow::DeleteStream (const char *name, bool remove_file)
{
  char filename[PATH_MAX];

  if (m_started) {
    return false;
  }

  CMediaStream *ms = m_stream_list->FindStream(name);
  if (ms == NULL) {
    error_message("Cannot find stream %s to delete", name);
    return false;
  }
  if (m_stream_list->GetCount() == 1) {
    error_message("Cannot delete last stream %s", name);
    return false;
  }
  m_stream_list->RemoveEntryFromList(ms);
  strcpy(filename, ms->GetFileName());
  delete ms; // this will write the file settings
  if (remove_file) {
    unlink(filename);
  }
  ValidateAndUpdateStreams();
  return true;
}

void CAVMediaFlow::RestartFileRecording (void)
{
  if (m_started == false) return;

  bool hint_tracks = m_pConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS);
  if (hint_tracks) {
    error_message("Disabling writing of hint tracks for file record restart");
  }
  m_pConfig->SetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS, false);
  CMediaStream *stream;
  stream = m_stream_list->GetHead();
  while (stream != NULL) {
    stream->RestartFileRecording();
    stream = stream->GetNext();
  }
  m_pConfig->SetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS, hint_tracks);
}
/* end file media_flow.cpp */	 
