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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"

#include <sys/mman.h>

#ifdef ADD_FFMPEG_ENCODER
#include <avcodec.h>
#include <dsputil.h>
#include <mpegvideo.h>
#endif /* ADD_FFMPEG_ENCODER */

#ifdef ADD_DIVX_ENCODER
#include <encore.h>
#endif /* ADD_DIVX_ENCODER */

#ifdef ADD_XVID_ENCODER
#include <xvid.h>
#endif /* ADD_XVID_ENCODER */

#include "video_source.h"
#include "rgb2yuv.h"

int CVideoSource::ThreadMain(void) 
{
	while (true) {
		int rc;

		if (m_capture) {
			rc = SDL_SemTryWait(m_myMsgQueueSemaphore);
		} else {
			rc = SDL_SemWait(m_myMsgQueueSemaphore);
		}

		// semaphore error
		if (rc == -1) {
			break;
		} 

		// message pending
		if (rc == 0) {
			CMsg* pMsg = m_myMsgQueue.get_message();
		
			if (pMsg != NULL) {
				switch (pMsg->get_value()) {
				case MSG_NODE_STOP_THREAD:
					DoStopCapture();	// ensure things get cleaned up
					delete pMsg;
					return 0;
				case MSG_START_CAPTURE:
					DoStartCapture();
					break;
				case MSG_STOP_CAPTURE:
					DoStopCapture();
					break;
				case MSG_START_PREVIEW:
					DoStartPreview();
					break;
				case MSG_STOP_PREVIEW:
					DoStopPreview();
					break;
				case MSG_GENERATE_KEY_FRAME:
					DoGenerateKeyFrame();
					break;
				}

				delete pMsg;
			}
		}

		if (m_capture) {
			ProcessVideo();
		}
	}

	return -1;
}

void CVideoSource::DoStartCapture(void)
{
	if (m_capture) {
		return;
	}
	if (!Init()) {
		return;
	}

	m_capture = true;
}

void CVideoSource::DoStopCapture(void)
{
	if (!m_capture) {
		return;
	}
	if (m_preview) {
		DoStopPreview();
	}

	// shutdown encoder
	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODE)) {
		StopEncoder();
	}

	// release device resources
	munmap(m_videoMap, m_videoMbuf.size);
	m_videoMap = NULL;

	close(m_videoDevice);
	m_videoDevice = -1;

	m_capture = false;
}

void CVideoSource::DoStartPreview(void)
{
#ifndef NOGUI
	if (m_preview) {
		return;
	}
	if (!m_capture) {
		DoStartCapture();
		
		if (!m_capture) {
			return;
		}
	}

	u_int32_t sdlVideoFlags = SDL_HWSURFACE;

	if (m_pConfig->m_videoPreviewWindowId) {
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%u", 
			m_pConfig->m_videoPreviewWindowId);
		setenv("SDL_WINDOWID", buffer, 1);
		setenv("SDL_VIDEO_CENTERED", "1", 1);
		sdlVideoFlags |= SDL_NOFRAME;
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		error_message("Could not init SDL video: %s", SDL_GetError());
	}
	char driverName[32];
	if (!SDL_VideoDriverName(driverName, 1)) {
		error_message("Could not init SDL video: %s", SDL_GetError());
	}

	m_sdlScreen = SDL_SetVideoMode(m_pConfig->m_videoWidth, 
		m_pConfig->m_videoHeight, 32, sdlVideoFlags);

	m_sdlScreenRect.x = 0;
	m_sdlScreenRect.y = 0;
	m_sdlScreenRect.w = m_pConfig->m_videoWidth;
	m_sdlScreenRect.h = m_pConfig->m_videoHeight;

	m_sdlImage = SDL_CreateYUVOverlay(m_pConfig->m_videoWidth, 
		m_pConfig->m_videoHeight, SDL_YV12_OVERLAY, m_sdlScreen);

	// currently we can only do one type of preview
	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)
	  && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {
		// so resolve any misconfiguration
		m_pConfig->SetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW, false);
	}

	m_preview = true;
#endif /* NOGUI */
}

void CVideoSource::DoStopPreview(void)
{
#ifndef NOGUI
	if (!m_preview) {
		return;
	}

	// do a final blit to set the screen to all black
	SDL_LockYUVOverlay(m_sdlImage);
	memset(m_sdlImage->pixels[0], 16, m_ySize);
	memset(m_sdlImage->pixels[1], 128, m_uvSize);
	memset(m_sdlImage->pixels[2], 128, m_uvSize);
	SDL_DisplayYUVOverlay(m_sdlImage, &m_sdlScreenRect);
	SDL_UnlockYUVOverlay(m_sdlImage);

	SDL_FreeYUVOverlay(m_sdlImage);
	SDL_FreeSurface(m_sdlScreen);
	SDL_Quit();

	m_sdlImage = NULL;
	m_sdlScreen = NULL;

	m_preview = false;
#endif /* NOGUI */
}

void CVideoSource::DoGenerateKeyFrame(void)
{
	m_wantKeyFrame = true;
}

bool CVideoSource::Init(void)
{
	if (!InitDevice()) {
		return false;
	}

	if (m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL) == VIDEO_MODE_NTSC) {
		m_rawFrameRate = NTSC_INT_FPS;
	} else {
		m_rawFrameRate = PAL_INT_FPS;
	}
	InitSampleFrames(
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE), 
		m_rawFrameRate);

	InitSizes();

	if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODE)) {
		if (!InitEncoder()) {
			close(m_videoDevice);
			m_videoDevice = -1;
			return false;
		}
	}

	m_rawFrameNumber = 0xFFFFFFFF;
	m_rawFrameDuration = TimestampTicks / m_rawFrameRate;
	m_encodedFrameNumber = 0;
	m_targetFrameDuration = TimestampTicks 
		/ m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE);

	m_prevVopBuf = NULL;
	m_prevVopBufLength = 0;

	m_skippedFrames = 0;
	m_accumDrift = 0;
	m_maxDrift = m_targetFrameDuration;

	return true;
}

bool CVideoSource::InitDevice(void)
{
	int rc;
	char* deviceName = m_pConfig->GetStringValue(CONFIG_VIDEO_DEVICE_NAME);
	u_int16_t format;

	// open the video device
	m_videoDevice = open(deviceName, O_RDWR);
	if (m_videoDevice < 0) {
		error_message("Failed to open %s: %s", 
			deviceName, strerror(errno));
		return false;
	}

	// get device capabilities
	struct video_capability videoCapability;
	rc = ioctl(m_videoDevice, VIDIOCGCAP, &videoCapability);
	if (rc < 0) {
		error_message("Failed to get video capabilities for %s",
			deviceName);
		goto failure;
	}

	if (!(videoCapability.type & VID_TYPE_CAPTURE)) {
		error_message("Device %s is not capable of video capture!",
			deviceName);
		goto failure;
	}

	// N.B. "channel" here is really an input source
	struct video_channel videoChannel;
	videoChannel.channel = m_pConfig->GetIntegerValue(CONFIG_VIDEO_INPUT);
	rc = ioctl(m_videoDevice, VIDIOCGCHAN, &videoChannel);
	if (rc < 0) {
		error_message("Failed to get video channel info for %s",
			deviceName);
		goto failure;
	}

	// select video input and signal type
	videoChannel.norm = m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL);
	rc = ioctl(m_videoDevice, VIDIOCSCHAN, &videoChannel);
	if (rc < 0) {
		error_message("Failed to set video channel info for %s",
			deviceName);
		goto failure;
	}

	// input source has a TV tuner
	if (videoChannel.flags & VIDEO_VC_TUNER) {
		struct video_tuner videoTuner;

		// get tuner info
		if ((int32_t)m_pConfig->GetIntegerValue(CONFIG_VIDEO_TUNER) == -1) {
			m_pConfig->SetIntegerValue(CONFIG_VIDEO_TUNER, 0);
		}
		videoTuner.tuner = m_pConfig->GetIntegerValue(CONFIG_VIDEO_TUNER);
		rc = ioctl(m_videoDevice, VIDIOCGTUNER, &videoTuner);
		if (rc < 0) {
			error_message("Failed to get video tuner info for %s",
				deviceName);
		}
		
		// set tuner and signal type
		videoTuner.mode = m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL);
		rc = ioctl(m_videoDevice, VIDIOCSTUNER, &videoTuner);
		if (rc < 0) {
			error_message("Failed to set video tuner info for %s",
				deviceName);
		}

		// tune in the desired frequency (channel)
		struct CHANNEL_LIST* pChannelList = ListOfChannelLists[
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL)];
		struct CHANNEL* pChannel = pChannelList[
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX)].list;
		unsigned long videoFrequencyKHz = pChannel[
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX)].freq;
		unsigned long videoFrequencyTuner =
			((videoFrequencyKHz / 1000) << 4) 
			| ((videoFrequencyKHz % 1000) >> 4);

		rc = ioctl(m_videoDevice, VIDIOCSFREQ, &videoFrequencyTuner);
		if (rc < 0) {
			error_message("Failed to set video tuner frequency for %s",
				deviceName);
		}
	}

	// get info on video capture buffers 
	rc = ioctl(m_videoDevice, VIDIOCGMBUF, &m_videoMbuf);
	if (rc < 0) {
		error_message("Failed to get video capture info for %s", 
			deviceName);
		goto failure;
	}

	// map the video capture buffers
	m_videoMap = mmap(0, m_videoMbuf.size, 
		PROT_READ, MAP_SHARED, m_videoDevice, 0);
	if (m_videoMap == MAP_FAILED) {
		error_message("Failed to map video capture memory for %s", 
			deviceName);
		goto failure;
	}

	// allocate enough frame maps
	m_videoFrameMap = (struct video_mmap*)
		malloc(m_videoMbuf.frames * sizeof(struct video_mmap));
	if (m_videoFrameMap == NULL) {
		error_message("Failed to allocate enough memory"); 
		goto failure;
	}

	m_captureHead = 0;
	m_encodeHead = -1;
	format = VIDEO_PALETTE_YUV420P;

	for (int i = 0; i < m_videoMbuf.frames; i++) {
		// initialize frame map
		m_videoFrameMap[i].frame = i;
		m_videoFrameMap[i].width = 
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
		m_videoFrameMap[i].height = 
			m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
		m_videoFrameMap[i].format = format;

		// give frame to the video capture device
		rc = ioctl(m_videoDevice, VIDIOCMCAPTURE, &m_videoFrameMap[i]);
		if (rc < 0) {
			// try RGB24 palette instead
			if (i == 0 && format == VIDEO_PALETTE_YUV420P) {
				format = VIDEO_PALETTE_RGB24;
				i--;
				continue;
			} 

			error_message("Failed to allocate video capture buffer for %s", 
				deviceName);
			goto failure;
		}
	}

	if (format == VIDEO_PALETTE_RGB24) {
		m_pConfig->m_videoNeedRgbToYuv = true;
	}

	return true;

failure:
	free(m_videoFrameMap);
	if (m_videoMap) {
		munmap(m_videoMap, m_videoMbuf.size);
		m_videoMap = NULL;
	}
	close(m_videoDevice);
	m_videoDevice = -1;
	return false;
}

void CVideoSource::InitSampleFrames(u_int16_t targetFps, u_int16_t rawFps)
{
	float faccum = 0.0;
	float fout = 1.0;
	float epsilon = 0.01;
	float ratio = (float)targetFps / (float)rawFps;
	u_int16_t f;

	for (f = 0; f < rawFps; f++) {
		faccum += ratio;
		if (faccum + epsilon >= fout && fout <= (float)targetFps) {
			fout += 1.0;
			m_sampleFrames[f] = true;
		} else {
			m_sampleFrames[f] = false;
		}
	}

	// use to get an estimate 
	// of how many raw frames until
	// the next desired frame
	m_frameRateRatio = rawFps / targetFps;
}

void CalculateVideoFrameSize(CLiveConfig* pConfig)
{
	u_int16_t frameHeight;
	float aspectRatio = pConfig->GetFloatValue(CONFIG_VIDEO_ASPECT_RATIO);

	// crop video to appropriate aspect ratio modulo 16 pixels
	if ((aspectRatio - VIDEO_STD_ASPECT_RATIO) < 0.1) {
		frameHeight = pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	} else {
		frameHeight = (u_int16_t)(
			(float)pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH) 
			/ aspectRatio);

		if ((frameHeight % 16) != 0) {
			frameHeight += 16 - (frameHeight % 16);
		}

		if (frameHeight > pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT)) {
			// OPTION might be better to insert black lines 
			// to pad image but for now we crop down
			frameHeight = pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
			if ((frameHeight % 16) != 0) {
				frameHeight -= (frameHeight % 16);
			}
		}
	}

	pConfig->m_videoWidth = pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
	pConfig->m_videoHeight = frameHeight;
}

void CVideoSource::InitSizes()
{
	CalculateVideoFrameSize(m_pConfig);

	m_yRawSize = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH)
		* m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	m_uvRawSize = m_yRawSize / 4;
	m_yuvRawSize = m_yRawSize + (2 * m_uvRawSize);

	m_yOffset = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH)
		* ((m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT)
			- m_pConfig->m_videoHeight) / 2);
	m_uvOffset = m_yOffset / 4;
		
	m_ySize = m_pConfig->m_videoWidth * m_pConfig->m_videoHeight;
	m_uvSize = m_ySize / 4;
	m_yuvSize = m_ySize + 2 * m_uvSize;
}

bool CVideoSource::InitEncoder()
{
	char* encoderName = 
		m_pConfig->GetStringValue(CONFIG_VIDEO_ENCODER);

	if (!strcasecmp(encoderName, VIDEO_ENCODER_FFMPEG)) {
		m_encoder = USE_FFMPEG;
		return InitFfmpegEncoder();
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_DIVX)) {
		m_encoder = USE_DIVX;
		return InitDivxEncoder();
	} else if (!strcasecmp(encoderName, VIDEO_ENCODER_XVID)) {
		m_encoder = USE_XVID;
		return InitXvidEncoder();
	}

	error_message("unknown encoder specified");
	return false;
}

bool CVideoSource::InitFfmpegEncoder()
{
#ifdef ADD_FFMPEG_ENCODER
	// use ffmpeg "divx" aka mpeg4 encoder
	m_avctx.frame_number = 0;
	m_avctx.width = m_pConfig->m_videoWidth;
	m_avctx.height = m_pConfig->m_videoHeight;
	m_avctx.rate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE);
	m_avctx.bit_rate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000;
	m_avctx.gop_size = m_avctx.rate * 2;
	m_avctx.want_key_frame = 0;
	m_avctx.flags = 0;
	m_avctx.codec = &divx_encoder;
	m_avctx.priv_data = malloc(m_avctx.codec->priv_data_size);
	memset(m_avctx.priv_data, 0, m_avctx.codec->priv_data_size);

	divx_encoder.init(&m_avctx);

	return true;
#else
	error_message("ffmpeg encoder not available in this build\n");
	return false;
#endif /* ADD_FFMPEG_ENCODER */
}

bool CVideoSource::InitDivxEncoder()
{
#ifdef ADD_DIVX_ENCODER
	// setup DivX Encore parameters
	ENC_PARAM divxParams;

	divxParams.x_dim = m_pConfig->m_videoWidth;
	divxParams.raw_y_dim = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	divxParams.y_dim = m_pConfig->m_videoHeight;
	divxParams.framerate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE);
	divxParams.bitrate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE);
	divxParams.rc_period = 2000;
	divxParams.rc_reaction_period = 10;
	divxParams.rc_reaction_ratio = 20;
	divxParams.max_key_interval = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE) * 2;
	divxParams.search_range = 16;
	divxParams.max_quantizer = 15;
	divxParams.min_quantizer = 2;
	divxParams.enable_8x8_mv = 0;

	if (encore(m_divxHandle, ENC_OPT_INIT, &divxParams, NULL) != ENC_OK) {
		error_message("Failed to initialize Divx encoder");
		return false;
	}

	return true;
#else
	error_message("divx encoder not available in this build\n");
	return false;
#endif /* ADD_DIVX_ENCODER */
}
				
bool CVideoSource::InitXvidEncoder()
{
#ifdef ADD_XVID_ENCODER
	XVID_INIT_PARAM xvidInitParams;

	memset(&xvidInitParams, 0, sizeof(xvidInitParams));

	if (xvid_init(NULL, 0, &xvidInitParams, NULL) != XVID_ERR_OK) {
		error_message("Failed to initialize Xvid");
		return false;
	}

	debug_message("Xvid CPU flags %08x\n", xvidInitParams.cpu_flags);

	XVID_ENC_PARAM xvidEncParams;

	memset(&xvidEncParams, 0, sizeof(xvidEncParams));

	xvidEncParams.width = m_pConfig->m_videoWidth;
	xvidEncParams.height = m_pConfig->m_videoHeight;
	xvidEncParams.fincr = 1001;
	xvidEncParams.fbase = 
		(int)(1001 * m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE));
	xvidEncParams.bitrate = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE) * 1000;
	xvidEncParams.rc_period = 2000;
	xvidEncParams.rc_reaction_period = 10;
	xvidEncParams.rc_reaction_ratio = 20;
	xvidEncParams.max_quantizer = 31;
	xvidEncParams.min_quantizer = 1;
	xvidEncParams.max_key_interval = 
		m_pConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE) * 2;
	xvidEncParams.motion_search = 1;
	xvidEncParams.quant_type = 1;
	xvidEncParams.lum_masking = 1;

	if (xvid_encore(NULL, XVID_ENC_CREATE, &xvidEncParams, NULL) != XVID_ERR_OK) {
		error_message("Failed to initialize Xvid encoder");
		return false;
	}

	m_xvidHandle = xvidEncParams.handle; 

	return true;
#else
	error_message("xvid encoder not available in this build\n");
	return false;
#endif /* ADD_XVID_ENCODER */
}

void CVideoSource::StopEncoder()
{
	switch (m_encoder) {
	case USE_FFMPEG:
		StopFfmpegEncoder();
		break;
	case USE_DIVX:
		StopDivxEncoder();
		break;
	case USE_XVID:
		StopXvidEncoder();
		break;
	}
}

void CVideoSource::StopFfmpegEncoder()
{
#ifdef ADD_FFMPEG_ENCODER
	divx_encoder.close(&m_avctx);
	free(m_avctx.priv_data);
#endif /* ADD_FFMPEG_ENCODER */
}

void CVideoSource::StopDivxEncoder()
{
#ifdef ADD_DIVX_ENCODER
	encore(m_divxHandle, ENC_OPT_RELEASE, NULL, NULL);
#endif /* ADD_DIVX_ENCODER */
}

void CVideoSource::StopXvidEncoder()
{
#ifdef ADD_XVID_ENCODER
	xvid_encore(m_xvidHandle, XVID_ENC_DESTROY, NULL, NULL);
#endif /* ADD_XVID_ENCODER */
}

int8_t CVideoSource::AcquireFrame(void)
{
	int rc;

	rc = ioctl(m_videoDevice, VIDIOCSYNC, &m_videoFrameMap[m_captureHead]);
	if (rc != 0) {
		return -1;
	}

	int8_t capturedFrame = m_captureHead;
	m_captureHead = (m_captureHead + 1) % m_videoMbuf.frames;

	if (m_captureHead == m_encodeHead) {
		debug_message("Video capture buffer overflow");
		return -1;
	}
	return capturedFrame;
}

void CVideoSource::ProcessVideo(void)
{
	u_int8_t* yuvImage = NULL;
	u_int8_t* yImage;
	u_int8_t* uImage;
	u_int8_t* vImage;

	// for efficiency, process 1 second before returning to check for commands
	for (int pass = 0; pass < m_rawFrameRate; pass++) {

		// get next frame from video capture device
		m_encodeHead = AcquireFrame();
		if (m_encodeHead == -1) {
			continue;
		}
		Timestamp frameTimestamp = GetTimestamp();
		m_rawFrameNumber++;

		if (m_rawFrameNumber == 0) {
			m_startTimestamp = frameTimestamp;
			m_elapsedDuration = 0;
		}

		// check if we want this frame (to match target fps)
		if (!m_sampleFrames[m_rawFrameNumber % m_rawFrameRate]) {
			goto release;
		}

		// check if we are falling behind due to encoding speed
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODE)) {
			if (m_accumDrift >= m_maxDrift) {
				if (m_accumDrift <= m_targetFrameDuration) {
					m_accumDrift = 0;
				} else {
					m_accumDrift -= m_targetFrameDuration;
				}

				// skip this frame			
				m_skippedFrames++;
				goto release;
			}
		}

		if (m_pConfig->m_videoNeedRgbToYuv) {
			yuvImage = (u_int8_t*)malloc(m_yuvRawSize);
			if (yuvImage == NULL) {
				debug_message("Can't malloc YUV buffer!");
				goto release;
			}
			RGB2YUV(
				m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH), 
				m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT), 
				(u_int8_t*)m_videoMap + m_videoMbuf.offsets[m_encodeHead],
				yuvImage,
				yuvImage + m_yRawSize,
				yuvImage + m_yRawSize + m_uvRawSize,
				1);
		} else {
			yuvImage = (u_int8_t*)m_videoMap 
				+ m_videoMbuf.offsets[m_encodeHead];
		}

		yImage = yuvImage + m_yOffset;
		uImage = yuvImage + m_yRawSize + m_uvOffset;
		vImage = uImage + m_uvRawSize;

#ifndef NOGUI
		if (m_preview && m_pConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW)) {
			SDL_LockYUVOverlay(m_sdlImage);
			memcpy(m_sdlImage->pixels[0], yImage, m_ySize);
			memcpy(m_sdlImage->pixels[1], vImage, m_uvSize);
			memcpy(m_sdlImage->pixels[2], uImage, m_uvSize);
			SDL_DisplayYUVOverlay(m_sdlImage, &m_sdlScreenRect);
			SDL_UnlockYUVOverlay(m_sdlImage);
		}
#endif

		// encode video frame to MPEG-4
		if (m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODE)) {

			u_int8_t* vopBuf = (u_int8_t*)malloc(m_maxVopSize);
			u_int32_t vopBufLength = 0;
			if (vopBuf == NULL) {
				debug_message("Can't malloc VOP buffer!");
				goto release;
			}

#ifdef ADD_DIVX_ENCODER
			ENC_RESULT divxResult;
#endif

#ifdef ADD_XVID_ENCODER
			XVID_ENC_FRAME xvidFrame;
			XVID_ENC_STATS xvidResult;
#endif

			// call encoder
			switch (m_encoder) {
			case USE_FFMPEG: {
#ifdef ADD_FFMPEG_ENCODER
				u_int8_t* yuvPlanes[3];
				yuvPlanes[0] = yImage;
				yuvPlanes[1] = uImage;
				yuvPlanes[2] = vImage;

				m_avctx.want_key_frame = m_wantKeyFrame;

				vopBufLength = divx_encoder.encode(&m_avctx, 
					vopBuf, m_maxVopSize, yuvPlanes);

				m_avctx.frame_number++;
#endif /* ADD_FFMPEG_ENCODER */
				break;
			}
			case USE_DIVX: {
#ifdef ADD_DIVX_ENCODER
				ENC_FRAME divxFrame;
				divxFrame.image = yuvImage;
				divxFrame.bitstream = vopBuf;
				divxFrame.length = m_maxVopSize;

				u_int32_t divxFlags =
					(m_wantKeyFrame ? ENC_OPT_WANT_KEY_FRAME : 0);

				if (encore(m_divxHandle, divxFlags, &divxFrame, &divxResult)
				  != ENC_OK) {
					debug_message("Divx can't encode frame!");
					goto release;
				}

				vopBufLength = divxFrame.length;
#endif /* ADD_DIVX_ENCODER */
				break;
			}
			case USE_XVID: {
#ifdef ADD_XVID_ENCODER
				xvidFrame.image = yuvImage;
				xvidFrame.bitstream = vopBuf;
				xvidFrame.colorspace = XVID_CSP_YV12;
				xvidFrame.quant = 4;
				xvidFrame.intra = m_wantKeyFrame;

				if (xvid_encore(m_xvidHandle, XVID_ENC_ENCODE, &xvidFrame, 
				  &xvidResult) != XVID_ERR_OK) {
					debug_message("Xvid can't encode frame!");
					goto release;
				}

				vopBufLength = xvidFrame.length;
#endif /* ADD_XVID_ENCODER */
				break;
			}
			}

			// clear this flag
			m_wantKeyFrame = false;

			// if desired, preview reconstructed image
#ifndef NOGUI
			if (m_preview 
			  && m_pConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW)) {
				SDL_LockYUVOverlay(m_sdlImage);

				switch (m_encoder) {
				case USE_FFMPEG:
#ifdef ADD_FFMPEG_ENCODER
					memcpy(m_sdlImage->pixels[0], 
						((MpegEncContext*)m_avctx.priv_data)->
							current_picture[0],
						m_ySize);
					memcpy(m_sdlImage->pixels[1], 
						((MpegEncContext*)m_avctx.priv_data)->
							current_picture[2],
						m_uvSize);
					memcpy(m_sdlImage->pixels[2], 
						((MpegEncContext*)m_avctx.priv_data)->
							current_picture[1],
						m_uvSize);
#endif /* ADD_FFMPEG_ENCODER */
					break;
				case USE_DIVX:
#ifdef ADD_DIVX_ENCODER
					memcpy2to1(m_sdlImage->pixels[0], 
						(u_int16_t*)divxResult.reconstruct_y,
						m_ySize);
					memcpy2to1(m_sdlImage->pixels[1], 
						(u_int16_t*)divxResult.reconstruct_v,
						m_uvSize);
					memcpy2to1(m_sdlImage->pixels[2], 
						(u_int16_t*)divxResult.reconstruct_u,
						m_uvSize);
#endif /* ADD_DIVX_ENCODER */
					break;
				case USE_XVID:
#ifdef ADD_XVID_ENCODER
					imgcpy(m_sdlImage->pixels[0], 
						xvidResult.image_y,
						m_pConfig->m_videoWidth, 
						m_pConfig->m_videoHeight,
						xvidResult.stride_y);
					imgcpy(m_sdlImage->pixels[1], 
						xvidResult.image_u,
						m_pConfig->m_videoWidth / 2, 
						m_pConfig->m_videoHeight / 2,
						xvidResult.stride_uv);
					imgcpy(m_sdlImage->pixels[2], 
						xvidResult.image_v,
						m_pConfig->m_videoWidth / 2, 
						m_pConfig->m_videoHeight / 2,
						xvidResult.stride_uv);
#endif /* ADD_XVID_ENCODER */
					break;
				}

				SDL_DisplayYUVOverlay(m_sdlImage, &m_sdlScreenRect);
				SDL_UnlockYUVOverlay(m_sdlImage);
			}
#endif /* NOGUI */

			// forward previously encoded vop to sinks
			if (m_prevVopBuf) {
				// calculate frame duration
				// making an adjustment to account 
				// for any raw frames dropped by the driver

				Duration elapsedTime = 
					frameTimestamp - m_startTimestamp;

				Duration frameDuration = 
					(m_skippedFrames + 1) * m_targetFrameDuration;

				Duration elapsedDuration = 
					m_elapsedDuration + frameDuration;

				Duration skew = elapsedTime - elapsedDuration;

				if (skew > 0) {
					frameDuration += 
						(skew / m_targetFrameDuration) * m_targetFrameDuration;
				}

				CMediaFrame* pFrame = new CMediaFrame(
					CMediaFrame::Mpeg4VideoFrame, 
					m_prevVopBuf, 
					m_prevVopBufLength,
					m_prevVopTimestamp, 
					frameDuration);
				ForwardFrame(pFrame);
				delete pFrame;

				m_elapsedDuration += frameDuration;
			}

			// hold onto this encoded vop until next one is ready
			m_prevVopBuf = vopBuf;
			m_prevVopBufLength = vopBufLength;
			m_prevVopTimestamp = frameTimestamp;

			m_encodedFrameNumber++;

			// reset skipped frames
			m_skippedFrames = 0;

			// calculate how we're doing versus target frame rate
			Duration encodingTime = GetTimestamp() - frameTimestamp;
			if (encodingTime >= m_targetFrameDuration) {
				m_accumDrift += encodingTime - m_targetFrameDuration;
			} else {
				m_accumDrift -= m_targetFrameDuration - encodingTime;
				if (m_accumDrift < 0) {
					m_accumDrift = 0;
				}
			}
		}

		// if desired, forward raw video to sinks
		if (m_pConfig->GetBoolValue(CONFIG_RECORD_RAW)) {
			u_int8_t* yuvBuf = (u_int8_t*)malloc(m_yuvSize);
			if (yuvBuf == NULL) {
				debug_message("Can't malloc YUV buffer!");
				goto release;
			}

			memcpy(yuvBuf, yImage, m_ySize);
			memcpy(yuvBuf + m_ySize, uImage, m_uvSize);
			memcpy(yuvBuf + m_ySize + m_uvSize, vImage, m_uvSize);

			CMediaFrame* pFrame =
				new CMediaFrame(CMediaFrame::YuvVideoFrame, 
					yuvBuf, m_yuvSize,
					frameTimestamp, m_targetFrameDuration);
			ForwardFrame(pFrame);
			delete pFrame;
		} 

release:
		// release video frame buffer back to video capture device
		if (ReleaseFrame(m_encodeHead)) {
			m_encodeHead = (m_encodeHead + 1) % m_videoMbuf.frames;
		} else {
			debug_message("Couldn't release capture buffer!");
		}
		if (m_pConfig->m_videoNeedRgbToYuv) {
			free(yuvImage);
			yuvImage = NULL;
		}
	}
}

char CVideoSource::GetMpeg4VideoFrameType(CMediaFrame* pFrame)
{
	if (pFrame == NULL) {
		return 0;
	}
	if (pFrame->GetDataLength() < 5) {
		return 0;
	}

	u_int8_t* pData = (u_int8_t*)pFrame->GetData();
	if (pData[0] != 0x00 || pData[1] != 0x00 
	  || pData[2] != 0x01 || pData[3] != 0xB6) {
		return 0;
	}

	switch (pData[4] >> 6) {
	case 0:
		return 'I';
	case 1:
		return 'P';
	case 2:
		return 'B';
	case 3:
		return 'S';
	}

	return 0;
}

bool CVideoSource::InitialVideoProbe(CLiveConfig* pConfig)
{
	static char* devices[] = {
		"/dev/video", 
		"/dev/video0", 
		"/dev/video1", 
		"/dev/video2", 
		"/dev/video3"
	};
	char* deviceName = pConfig->GetStringValue(CONFIG_VIDEO_DEVICE_NAME);
	CVideoCapabilities* pVideoCaps;

	// first try the device we're configured with
	pVideoCaps = new CVideoCapabilities(deviceName);

	if (pVideoCaps->IsValid()) {
		pConfig->m_videoCapabilities = pVideoCaps;
		return true;
	}

	delete pVideoCaps;

	// no luck, go searching
	for (u_int32_t i = 0; i < sizeof(devices) / sizeof(char*); i++) {

		// don't waste time trying something that's already failed
		if (!strcmp(devices[i], deviceName)) {
			continue;
		} 

		pVideoCaps = new CVideoCapabilities(devices[i]);

		if (pVideoCaps->IsValid()) {
			pConfig->SetStringValue(CONFIG_VIDEO_DEVICE_NAME, devices[i]);
			pConfig->m_videoCapabilities = pVideoCaps;
			return true;
		}
		
		delete pVideoCaps;
	}

	return false;
}

bool CVideoCapabilities::ProbeDevice()
{
	int rc;

	int videoDevice = open(m_deviceName, O_RDWR);
	if (videoDevice < 0) {
		m_canOpen = false;
		return false;
	}
	m_canOpen = true;

	// get device capabilities
	struct video_capability videoCapability;
	rc = ioctl(videoDevice, VIDIOCGCAP, &videoCapability);
	if (rc < 0) {
		debug_message("Failed to get video capabilities for %s", m_deviceName);
		m_canCapture = false;
		close(videoDevice);
		return false;
	}

	if (!(videoCapability.type & VID_TYPE_CAPTURE)) {
		debug_message("Device %s is not capable of video capture!", 
			m_deviceName);
		m_canCapture = false;
		close(videoDevice);
		return false;
	}
	m_canCapture = true;

	m_driverName = stralloc(videoCapability.name);
	m_numInputs = videoCapability.channels;

	m_minWidth = videoCapability.minwidth;
	m_minHeight = videoCapability.minheight;
	m_maxWidth = videoCapability.maxwidth;
	m_maxHeight = videoCapability.maxheight;

	m_inputNames = (char**)malloc(m_numInputs * sizeof(char*));
	memset(m_inputNames, 0, m_numInputs * sizeof(char*));

	m_inputSignalTypes = (u_int8_t*)malloc(m_numInputs * sizeof(u_int8_t));
	memset(m_inputSignalTypes, 0, m_numInputs * sizeof(u_int8_t));

	m_inputHasTuners = (bool*)malloc(m_numInputs * sizeof(bool));
	memset(m_inputHasTuners, 0, m_numInputs * sizeof(bool));

	m_inputTunerSignalTypes = (u_int8_t*)malloc(m_numInputs * sizeof(u_int8_t));
	memset(m_inputTunerSignalTypes, 0, m_numInputs * sizeof(u_int8_t));


	for (int i = 0; i < m_numInputs; i++) {
		// N.B. "channel" here is really an input source
		struct video_channel videoChannel;
		videoChannel.channel = i;
		rc = ioctl(videoDevice, VIDIOCGCHAN, &videoChannel);
		if (rc < 0) {
			debug_message("Failed to get video channel info for %s:%u",
				m_deviceName, i);
			continue;
		}
		m_inputNames[i] = stralloc(videoChannel.name);
		m_inputSignalTypes[i] = videoChannel.norm;

		if (videoChannel.flags & VIDEO_VC_TUNER) {
			// ignore videoChannel.tuners for now
			// current bt drivers only support 1 tuner per input port

			struct video_tuner videoTuner;
			videoTuner.tuner = 0;
			rc = ioctl(videoDevice, VIDIOCGTUNER, &videoTuner);
			if (rc < 0) {
				debug_message("Failed to get video tuner info for %s:%u",
					m_deviceName, i);
				continue;
			}
				
			m_inputHasTuners[i] = true;
			m_inputTunerSignalTypes[i] = videoTuner.flags & 0x7;
		}
	}

	close(videoDevice);
	return true;
}

