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
#include <encore.h>		/* divx */
#include <avcodec.h>	/* ffmpeg */
#include <dsputil.h>	/* ffmpeg */
#include <mpegvideo.h>	/* ffmpeg */

#include "video_source.h"

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

void CVideoSource::DoStartCapture()
{
	if (m_capture) {
		return;
	}
	if (!Init()) {
		return;
	}

	m_capture = true;
}

void CVideoSource::DoStopCapture()
{
	if (!m_capture) {
		return;
	}
	if (m_preview) {
		DoStopPreview();
	}

	if (m_pConfig->m_videoEncode) {
		if (m_pConfig->m_videoUseDivxEncoder) {
			encore(m_divxHandle, ENC_OPT_RELEASE, NULL, NULL);
		} else { // ffmpeg
			divx_encoder.close(&m_avctx);
			free(m_avctx.priv_data);
		}
	}

	close(m_videoDevice);
	m_videoDevice = -1;

	m_capture = false;
}

void CVideoSource::DoStartPreview()
{
	if (m_preview) {
		return;
	}
	if (!m_capture) {
		DoStartCapture();
		
		if (!m_capture) {
			return;
		}
	}

	u_int32_t sdlVideoFlags = SDL_SWSURFACE | SDL_ASYNCBLIT;

	if (m_pConfig->m_videoPreviewWindowId) {
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%u", 
			m_pConfig->m_videoPreviewWindowId);
		setenv("SDL_WINDOWID", buffer, 1);
		setenv("SDL_VIDEO_CENTERED", "1", 1);
		sdlVideoFlags |= SDL_NOFRAME;
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
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
	m_sdlScreenRect.w = m_sdlScreen->w;
	m_sdlScreenRect.h = m_sdlScreen->h;

	m_sdlImage = SDL_CreateYUVOverlay(m_pConfig->m_videoWidth, 
		m_pConfig->m_videoHeight, SDL_YV12_OVERLAY, m_sdlScreen);

	m_preview = true;
}

void CVideoSource::DoStopPreview()
{
	if (!m_preview) {
		return;
	}

	SDL_FreeYUVOverlay(m_sdlImage);
	SDL_FreeSurface(m_sdlScreen);
	SDL_Quit();

	m_sdlImage = NULL;
	m_sdlScreen = NULL;

	m_preview = false;
}

bool CVideoSource::Init(void)
{
	if (!InitDevice()) {
		return false;
	}

	if (m_pConfig->m_videoSignal == VIDEO_MODE_NTSC) {
		m_rawFrameRate = NTSC_INT_FPS;
	} else {
		m_rawFrameRate = PAL_INT_FPS;
	}
	InitSampleFrames(m_pConfig->m_videoTargetFrameRate, m_rawFrameRate);

	InitSizes();

	if (m_pConfig->m_videoEncode) {
		if (!InitEncoder()) {
			close(m_videoDevice);
			m_videoDevice = -1;
			return false;
		}
	}

	m_rawFrameNumber = 0xFFFFFFFF;
	m_skippedFrames = 0;
	m_rawFrameDuration = TimestampTicks / m_rawFrameRate;
	m_targetFrameDuration = TimestampTicks / m_pConfig->m_videoTargetFrameRate;

	m_prevVopBuf = NULL;
	m_prevVopBufLength = 0;

	return true;
}

bool CVideoSource::InitDevice(void)
{
	int rc;

	// TBD review error handling

	// open the video device
	m_videoDevice = open(m_pConfig->m_videoDeviceName, O_RDWR);
	if (m_videoDevice < 0) {
		error_message("Failed to open %s", 
			m_pConfig->m_videoDeviceName);
		return false;
	}

	// get device capabilities
	struct video_capability videoCapability;
	rc = ioctl(m_videoDevice, VIDIOCGCAP, &videoCapability);
	if (rc < 0) {
		error_message("Failed to get video capabilities for %s",
			m_pConfig->m_videoDeviceName);
		close(m_videoDevice);
		m_videoDevice = -1;
		return false;
	}

	if (!(videoCapability.type & VID_TYPE_CAPTURE)) {
		error_message("Device %s is not capable of video capture!",
			m_pConfig->m_videoDeviceName);
		close(m_videoDevice);
		m_videoDevice = -1;
		return false;
	}

	// N.B. "channel" here is really an input source
	struct video_channel videoChannel;
	videoChannel.channel = m_pConfig->m_videoInput;
	rc = ioctl(m_videoDevice, VIDIOCGCHAN, &videoChannel);
	if (rc < 0) {
		error_message("Failed to get video channel info for %s",
			m_pConfig->m_videoDeviceName);
	}

	// select video input and signal type
	videoChannel.norm = m_pConfig->m_videoSignal;
	rc = ioctl(m_videoDevice, VIDIOCSCHAN, &videoChannel);
	if (rc < 0) {
		error_message("Failed to set video channel info for %s",
			m_pConfig->m_videoDeviceName);
	}

	// input source has a TV tuner
	if (videoChannel.flags & VIDEO_VC_TUNER) {
		struct video_tuner videoTuner;

		// get tuner info
		if (m_pConfig->m_videoTuner == -1) {
			m_pConfig->m_videoTuner = 0;
		}
		videoTuner.tuner = m_pConfig->m_videoTuner;
		rc = ioctl(m_videoDevice, VIDIOCGTUNER, &videoTuner);
		if (rc < 0) {
			error_message("Failed to get video tuner info for %s",
				m_pConfig->m_videoDeviceName);
		}
		
		// set tuner and signal type
		videoTuner.mode = m_pConfig->m_videoSignal;
		rc = ioctl(m_videoDevice, VIDIOCSTUNER, &videoTuner);
		if (rc < 0) {
			error_message("Failed to set video tuner info for %s",
				m_pConfig->m_videoDeviceName);
		}

		// tune in the desired frequency (channel)
		unsigned long videoFrequency;
		videoFrequency = m_pConfig->m_videoChannel->freq;
		rc = ioctl(m_videoDevice, VIDIOCSFREQ, &videoFrequency);
		if (rc < 0) {
			error_message("Failed to set video tuner frequency for %s",
				m_pConfig->m_videoDeviceName);
		}
	}

	// get info on video capture buffers 
	rc = ioctl(m_videoDevice, VIDIOCGMBUF, &m_videoMbuf);
	if (rc < 0) {
		error_message("Failed to get video capture info for %s", 
			m_pConfig->m_videoDeviceName);
		return false;
	}

	// map the video capture buffers
	m_videoMap = mmap(0, m_videoMbuf.size, 
		PROT_READ, MAP_SHARED, m_videoDevice, 0);
	if (m_videoMap == MAP_FAILED) {
		error_message("Failed to map video capture memory for %s", 
			m_pConfig->m_videoDeviceName);
		return false;
	}

	// allocate enough frame maps
	m_videoFrameMap = (struct video_mmap*)
		malloc(m_videoMbuf.frames * sizeof(struct video_mmap));
	if (m_videoFrameMap == NULL) {
		error_message("Failed to allocate enough memory"); 
		return false;
	}

	m_captureHead = 0;
	m_encodeHead = -1;

	for (int i = 0; i < m_videoMbuf.frames; i++) {
		// initialize frame map
		m_videoFrameMap[i].frame = i;
		m_videoFrameMap[i].width = m_pConfig->m_videoRawWidth;
		m_videoFrameMap[i].height = m_pConfig->m_videoRawHeight;
		m_videoFrameMap[i].format = VIDEO_PALETTE_YUV420P;

		// give frame to the video capture device
		rc = ioctl(m_videoDevice, VIDIOCMCAPTURE, &m_videoFrameMap[i]);
		if (rc < 0) {
			error_message("Failed to allocate video capture buffer for %s", 
				m_pConfig->m_videoDeviceName);
			return false;
		}
	}

	return true;
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

void CVideoSource::InitSizes()
{
	m_yRawSize = m_pConfig->m_videoRawWidth * m_pConfig->m_videoRawHeight;
	m_uvRawSize = m_yRawSize / 4;

	// setup to crop video to appropriate aspect ratio
	if (m_pConfig->m_videoAspectRatio != VIDEO_STD_ASPECT_RATIO) {

		u_int16_t frameHeight = (u_int16_t)
			((float)m_pConfig->m_videoRawWidth / m_pConfig->m_videoAspectRatio);
		if ((frameHeight % 16) != 0) {
			frameHeight += 16 - (frameHeight % 16);
		}

		m_pConfig->m_videoWidth = m_pConfig->m_videoRawWidth;
		m_pConfig->m_videoHeight = frameHeight;

		m_yOffset = m_pConfig->m_videoRawWidth 
			* ((m_pConfig->m_videoRawHeight - frameHeight) / 2);
		m_uvOffset = m_yOffset / 4;
		
	} else {
		m_pConfig->m_videoWidth = m_pConfig->m_videoRawWidth;
		m_pConfig->m_videoHeight = m_pConfig->m_videoRawHeight;

		m_yOffset = 0;
		m_uvOffset = 0;
	}

	m_ySize = m_pConfig->m_videoWidth * m_pConfig->m_videoHeight;
	m_uvSize = m_ySize / 4;
	m_yuvSize = m_ySize + 2 * m_uvSize;
}

bool CVideoSource::InitEncoder()
{
	if (m_pConfig->m_videoUseDivxEncoder) {
		// setup DivX Encore parameters
		ENC_PARAM divxParams;

		divxParams.x_dim = m_pConfig->m_videoWidth;
		divxParams.raw_y_dim = m_pConfig->m_videoRawHeight;
		divxParams.y_dim = m_pConfig->m_videoHeight;
		divxParams.framerate = m_pConfig->m_videoTargetFrameRate;
		divxParams.bitrate = m_pConfig->m_videoTargetBitRate;
		divxParams.rc_period = 2000;
		divxParams.rc_reaction_period = 10;
		divxParams.rc_reaction_ratio = 20;
		divxParams.max_key_interval = m_pConfig->m_videoTargetFrameRate * 2;
		divxParams.search_range = 16;
		// INVESTIGATE divxParams.search_range = 0;
		divxParams.max_quantizer = 15;
		divxParams.min_quantizer = 2;
		divxParams.enable_8x8_mv = 0;

		if (encore(m_divxHandle, ENC_OPT_INIT, &divxParams, NULL) != ENC_OK) {
			error_message("Counldn't initialize Divx encoder");
			return false;
		}
				
	} else { // use ffmpeg "divx" aka mpeg4 encoder
		m_avctx.frame_number = 0;
		m_avctx.width = m_pConfig->m_videoWidth;
		m_avctx.height = m_pConfig->m_videoHeight;
		m_avctx.rate = m_pConfig->m_videoTargetFrameRate;
		m_avctx.bit_rate = m_pConfig->m_videoTargetBitRate * 1000;
		m_avctx.gop_size = m_avctx.rate * 2;
		m_avctx.flags = 0;
		m_avctx.codec = &divx_encoder;
		m_avctx.priv_data = malloc(m_avctx.codec->priv_data_size);
		memset(m_avctx.priv_data, 0, m_avctx.codec->priv_data_size);
		divx_encoder.init(&m_avctx);
	}

	return true;
}

int8_t CVideoSource::AcquireFrame(void)
{
	int rc;

	rc = ioctl(m_videoDevice, VIDIOCSYNC, &m_videoFrameMap[m_captureHead]);
	if (rc != 0) {
		//error_message("Failed to sync video capture buffer for %s", 
		//	m_pConfig->m_videoDeviceName);
		return -1;
	}

	int8_t capturedFrame = m_captureHead;
	m_captureHead = (m_captureHead + 1) % m_videoMbuf.frames;

	if (m_captureHead == m_encodeHead) {
		debug_message("Video capture buffer overflow for %s",
			m_pConfig->m_videoDeviceName);
		return -1;
	}
	return capturedFrame;
}

void CVideoSource::ProcessVideo(void)
{
	u_int8_t* yuvImage;
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
		}

		// check if we want this frame (to match target fps)
		if (!m_sampleFrames[m_rawFrameNumber % m_rawFrameRate]) {
			goto release;
		}

#ifdef NOTDEF
		// check if we are falling behind due to encoding speed
		if (m_pConfig->m_videoEncode) {
			// if now is later than 
			// when we should be acquiring the next frame to be coded
			if (frameTimestamp >= m_startTimestamp 
			  + (m_rawFrameNumber + m_frameRateRatio + 1) * m_rawFrameDuration) {
				// skip this frame			
debug_message("skipping frame #%u ts %llu >= ts %llu", 
	m_rawFrameNumber, frameTimestamp, 
	m_startTimestamp + (m_rawFrameNumber + m_frameRateRatio + 1) * m_rawFrameDuration);
				m_skippedFrames++;
				goto release;
			} else {
				m_skippedFrames = 0;
			}
		}
#endif

		yuvImage = (u_int8_t*)m_videoMap 
			+ m_videoMbuf.offsets[m_encodeHead];
		yImage = yuvImage + m_yOffset;
		uImage = yuvImage + m_yRawSize + m_uvOffset;
		vImage = uImage + m_uvRawSize;

		if (m_pConfig->m_videoRawPreview) {
			SDL_LockYUVOverlay(m_sdlImage);
			memcpy(m_sdlImage->pixels[0], yImage, m_ySize);
			memcpy(m_sdlImage->pixels[1], vImage, m_uvSize);
			memcpy(m_sdlImage->pixels[2], uImage, m_uvSize);
			SDL_DisplayYUVOverlay(m_sdlImage, &m_sdlScreenRect);
			SDL_UnlockYUVOverlay(m_sdlImage);
		}

		// encode video frame to MPEG-4
		if (m_pConfig->m_videoEncode) {

			u_int8_t* vopBuf = (u_int8_t*)malloc(m_maxVopSize);
			u_int32_t vopBufLength;
			if (vopBuf == NULL) {
				// TBD error
				debug_message("Can't malloc VOP buffer!");
				goto release;
			}

			ENC_RESULT divxResult;

			// call encoder libraries
			if (m_pConfig->m_videoUseDivxEncoder) {
				ENC_FRAME divxFrame;

				divxFrame.image = yuvImage;
				divxFrame.bitstream = vopBuf;
				divxFrame.length = m_maxVopSize;
				if (encore(m_divxHandle, 0, &divxFrame, &divxResult)
				  != ENC_OK) {
					debug_message("Divx can't encode frame!");
					goto release;
				}
				vopBufLength = divxFrame.length;
			} else { // ffmpeg
				u_int8_t* yuvPlanes[3];
				yuvPlanes[0] = yImage;
				yuvPlanes[1] = uImage;
				yuvPlanes[2] = vImage;
				vopBufLength = divx_encoder.encode(&m_avctx, 
					vopBuf, m_maxVopSize, yuvPlanes);
				m_avctx.frame_number++;
			}

			// if desired, preview reconstructed image
			if (m_pConfig->m_videoEncodedPreview) {
				SDL_LockYUVOverlay(m_sdlImage);

				if (m_pConfig->m_videoUseDivxEncoder) {
					memcpy2to1(m_sdlImage->pixels[0], 
						(u_int16_t*)divxResult.reconstruct_y,
						m_ySize);
					memcpy2to1(m_sdlImage->pixels[1], 
						(u_int16_t*)divxResult.reconstruct_v,
						m_uvSize);
					memcpy2to1(m_sdlImage->pixels[2], 
						(u_int16_t*)divxResult.reconstruct_u,
						m_uvSize);
				} else { // ffmpeg
					memcpy(m_sdlImage->pixels[0], 
						((MpegEncContext*)m_avctx.priv_data)->current_picture[0],
						m_ySize);
					memcpy(m_sdlImage->pixels[1], 
						((MpegEncContext*)m_avctx.priv_data)->current_picture[2],
						m_uvSize);
					memcpy(m_sdlImage->pixels[2], 
						((MpegEncContext*)m_avctx.priv_data)->current_picture[1],
						m_uvSize);
				}

				SDL_DisplayYUVOverlay(m_sdlImage, &m_sdlScreenRect);
				SDL_UnlockYUVOverlay(m_sdlImage);
			}

			// forward previously encoded vop to sinks
			if (m_prevVopBuf) {
				CMediaFrame* pFrame =
					new CMediaFrame(CMediaFrame::Mpeg4VideoFrame, 
						m_prevVopBuf, m_prevVopBufLength,
						m_prevVopTimestamp, 
						(m_skippedFrames + 1) * m_targetFrameDuration);
				ForwardFrame(pFrame);
				delete pFrame;
			}

			// hold onto this encoded vop until next one is ready
			m_prevVopBuf = vopBuf;
			m_prevVopBufLength = vopBufLength;
			m_prevVopTimestamp = frameTimestamp;
		}

		// if desired, forward raw video to sinks
		if (m_pConfig->m_recordRaw) {

			u_int8_t* yuvBuf = (u_int8_t*)malloc(m_yuvSize);
			if (yuvBuf == NULL) {
				// TBD error
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

		// release video frame buffer back to video capture device
release:
		if (ReleaseFrame(m_encodeHead)) {
			m_encodeHead = (m_encodeHead + 1) % m_videoMbuf.frames;
		} else {
			debug_message("Couldn't release capture buffer!");
		}
	}
}

bool CVideoCapabilities::ProbeDevice()
{
	int rc;

	int videoDevice = open(m_deviceName, O_RDWR);
	if (videoDevice < 0) {
		m_canOpen = false;
		return false;
	}

	// get device capabilities
	struct video_capability videoCapability;
	rc = ioctl(videoDevice, VIDIOCGCAP, &videoCapability);
	if (rc < 0) {
		debug_message("Failed to get video capabilities for %s", m_deviceName);
		m_canOpen = false;
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

