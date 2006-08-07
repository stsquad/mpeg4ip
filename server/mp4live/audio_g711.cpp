/*
 * The following code is subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this code
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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4live.h"
#include "audio_g711.h"

static uint32_t g711_sample_rate[] = { 8000, };
static uint32_t *g711_bitrate_for_samplerate (uint32_t samplerate, 
					      uint8_t chans, 
					      uint32_t *ret_size)
{
  uint32_t *ret = MALLOC_STRUCTURE(uint32_t);

  *ret = 64000;
  *ret_size = 1;
  return ret;
}

audio_encoder_table_t g711_alaw_audio_encoder_table = {
  "G.711 alaw",
  AUDIO_ENCODER_G711,
  AUDIO_ENCODING_ALAW,
  g711_sample_rate,
  NUM_ELEMENTS_IN_ARRAY(g711_sample_rate),
  g711_bitrate_for_samplerate,
  1, 
  NULL
};

audio_encoder_table_t g711_ulaw_audio_encoder_table = {
  "G.711 ulaw",
  AUDIO_ENCODER_G711,
  AUDIO_ENCODING_ULAW,
  g711_sample_rate,
  NUM_ELEMENTS_IN_ARRAY(g711_sample_rate),
  g711_bitrate_for_samplerate,
  1,
  NULL
};

MediaType g711_mp4_fileinfo (CAudioProfile *pConfig,
			     bool *mpeg4,
			     bool *isma_compliant,
			     uint8_t *audioProfile,
			     uint8_t **audioConfig,
			     uint32_t *audioConfigLen,
			     uint8_t *mp4AudioType)
{
  const char *encodingName = pConfig->GetStringValue(CFG_AUDIO_ENCODING);
  *mpeg4 = false;
  *isma_compliant = false;
  *audioProfile = 0;
  *audioConfig = NULL;
  *audioConfigLen = 0;
  if (mp4AudioType != NULL) {
    *mp4AudioType = 0;
  }
  if (!strcasecmp(encodingName, AUDIO_ENCODING_ULAW)) {
    if (mp4AudioType != NULL) {
      *mp4AudioType = MP4_ULAW_AUDIO_TYPE;
    }
    return ULAWAUDIOFRAME;
  } else if (!strcasecmp(encodingName, AUDIO_ENCODING_ALAW)) {
    if (mp4AudioType != NULL) {
      *mp4AudioType = MP4_ALAW_AUDIO_TYPE;
    }
    return ALAWAUDIOFRAME;
  }
  return UNDEFINEDFRAME;
}

media_desc_t *g711_create_audio_sdp (CAudioProfile *pConfig,
				     bool *mpeg4,
				     bool *isma_compliant,
				     uint8_t *audioProfile,
				     uint8_t **audioConfig,
				     uint32_t *audioConfigLen)
{
  media_desc_t *sdpMediaAudio;
  format_list_t *sdpMediaAudioFormat;
  MediaType type;

  type = g711_mp4_fileinfo(pConfig, mpeg4, isma_compliant, audioProfile,
			     audioConfig, audioConfigLen, NULL);

  sdpMediaAudio = MALLOC_STRUCTURE(media_desc_t);
  memset(sdpMediaAudio, 0, sizeof(*sdpMediaAudio));

  sdpMediaAudioFormat = MALLOC_STRUCTURE(format_list_t);
  memset(sdpMediaAudioFormat, 0, sizeof(*sdpMediaAudioFormat));
  sdpMediaAudio->fmt_list = sdpMediaAudioFormat;
  sdpMediaAudioFormat->media = sdpMediaAudio;
  

  if (type == ALAWAUDIOFRAME) {
    sdpMediaAudioFormat->fmt = strdup("8");
  } else if (type == ULAWAUDIOFRAME) {
    sdpMediaAudioFormat->fmt = strdup("0");
  } 

  return sdpMediaAudio;
}
bool g711_get_audio_rtp_info (CAudioProfile *pConfig,
			      MediaType *audioFrameType,
			      uint32_t *audioTimeScale,
			      uint8_t *audioPayloadNumber,
			      uint8_t *audioPayloadBytesPerPacket,
			      uint8_t *audioPayloadBytesPerFrame,
			      uint8_t *audioQueueMaxCount,
			      audio_set_rtp_payload_f *audio_set_rtp_payload,
			      audio_set_rtp_header_f *audio_set_header,
			      audio_set_rtp_jumbo_frame_f *audio_set_jumbo,
			      void **ud)
{
  const char *encodingName = pConfig->GetStringValue(CFG_AUDIO_ENCODING);
  if (strcasecmp(encodingName, AUDIO_ENCODING_ALAW) == 0 ||
      strcasecmp(encodingName, AUDIO_ENCODING_ULAW) == 0) {
    if (strcasecmp(encodingName, AUDIO_ENCODING_ALAW) == 0) {
      *audioFrameType = ALAWAUDIOFRAME;
      *audioPayloadNumber = 8;
    } else {
      *audioFrameType = ULAWAUDIOFRAME;
      *audioPayloadNumber = 0;
    }
    *audioTimeScale = 8000;
    *audioPayloadBytesPerPacket = 0;
    *audioPayloadBytesPerFrame = 0;
    *audioQueueMaxCount = 1;
    *audio_set_header = NULL;
    *audio_set_jumbo = NULL;
    *ud = NULL;
    return true;
  }

  return false;
	    
}
  
CG711AudioEncoder::CG711AudioEncoder(CAudioProfile *ap,
				     CAudioEncoder *next, 
				     u_int8_t srcChannels,
				     u_int32_t srcSampleRate,
				     uint16_t mtu,
				     bool realTime) :
  CAudioEncoder(ap, next, srcChannels, srcSampleRate, mtu, realTime)
{
	m_pFrameBuffer = NULL;
	m_frameBufferLength = 0;
}

bool CG711AudioEncoder::Init (void)
{
#if 1
	m_samplesPerFrame = 160;	// 20ms worth of samples
#else
	m_samplesPerFrame = 320;	// 40ms worth of samples
#endif

	m_useULAW = !strcasecmp(
		Profile()->GetStringValue(CFG_AUDIO_ENCODING),
		AUDIO_ENCODING_ULAW);

	Profile()->SetIntegerValue(CFG_AUDIO_BIT_RATE,
		m_pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS)
		* m_pConfig->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE) * 8);
	Initialize();
	return true;
}

u_int32_t CG711AudioEncoder::GetSamplesPerFrame()
{
	return m_samplesPerFrame;
}
static uint8_t linear2alaw(int16_t pcm_val);
static uint8_t linear2ulaw(int16_t pcm_val);

bool CG711AudioEncoder::EncodeSamples(
	int16_t* pSamples, 
	u_int32_t numSamplesPerChannel,
	u_int8_t numChannels)
{
	if (numChannels != 1) {
		return false;	// invalid numChannels
	}

	// check for signal to end encoding
	if (pSamples == NULL) {
		// unlike lame, G711 doesn't need to finish up anything
		return false;
	}

	// get an output buffer setup
	free(m_pFrameBuffer);

	m_frameBufferLength = numSamplesPerChannel
		* m_pConfig->GetIntegerValue(CFG_AUDIO_CHANNELS);
	m_pFrameBuffer = (u_int8_t*)Malloc(m_frameBufferLength);

	for (uint32_t ix = 0; ix < numSamplesPerChannel; ix++) {
	  if (m_useULAW) {
	    m_pFrameBuffer[ix] = linear2ulaw(pSamples[ix]);
	  } else {
	    m_pFrameBuffer[ix] = linear2alaw(pSamples[ix]);
	  }
	}



	return true;
}

bool CG711AudioEncoder::GetEncodedFrame(
	u_int8_t** ppBuffer, 
	u_int32_t* pBufferLength,
	u_int32_t* pNumSamplesPerChannel)
{
	*ppBuffer = m_pFrameBuffer;
	*pBufferLength = m_frameBufferLength;
	*pNumSamplesPerChannel = m_frameBufferLength;

	m_pFrameBuffer = NULL;
	m_frameBufferLength = 0;

	return true;
}

void CG711AudioEncoder::StopEncoder (void)
{
	free(m_pFrameBuffer);
	m_pFrameBuffer = NULL;
}


/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use.  Users may copy or modify this source code without
 * charge.
 *
 * SUN SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 * THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS SOFTWARE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * g711.c
 *
 * u-law, A-law and linear PCM conversions.
 */
#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		/* Number of A-law segments. */
#define	SEG_SHIFT	(4)		/* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

// wmay - changed from short to int16_t
static uint16_t seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF,
			      0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
// wmay - removed tables _u2a _a2u

// wmay - change 
static uint8_t
search(
    uint16_t	val,
    uint16_t	*table,
    uint8_t	size)
{
    uint8_t	i;

    for (i = 0; i < size; i++)
    {
        if (val <= *table++)
            return (i);
    }
    return (size);
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
// wmay - changed definitions of variable
static uint8_t
linear2alaw(
    int16_t	pcm_val)	/* 2's complement (16-bit range) */
{
    uint8_t	mask;
    uint8_t	seg;
    uint8_t	aval;

    if (pcm_val >= 0)
    {
        mask = 0xD5; 		/* sign (7th) bit = 1 */
    }
    else
    {
        mask = 0x55; 		/* sign bit = 0 */
        pcm_val = ~pcm_val; // wmay -pcm_val - 8;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = search((uint16_t)pcm_val, seg_end, 8);

    /* Combine the sign, segment, and quantization bits. */

    if (seg >= 8)		/* out of range, return maximum value. */
        return (0x7F ^ mask);
    else
    {
        aval = seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 4) & QUANT_MASK;
        else
            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
        return (aval ^ mask);
    }
}


// wmay - removed alaw2linear

#define	BIAS		(0x84)		/* Bias for linear code. */

/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static uint8_t
linear2ulaw(
    int16_t	pcm_val)	/* 2's complement (16-bit range) */
{
    uint8_t	mask;
    uint8_t	seg;
    uint8_t	uval;

    /* if someone has passed in a bum short (e.g. they manipulated an
     * int rather than a short, so the value looks positive, not
     * negative) fix that */
#if 0
    if(pcm_val & 0x8000)
    {
	pcm_val = - (~pcm_val & 0x7fff) - 1;
    }
#endif

    /* Get the sign and the magnitude of the value. */
    if (pcm_val < 0)
    {
        pcm_val = BIAS - pcm_val;
        mask = 0x7F;
    }
    else
    {
        pcm_val += BIAS;
        mask = 0xFF;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_end, 8);

    /*
     * Combine the sign, segment, quantization bits;
     * and complement the code word.
     */
    if (seg >= 8)		/* out of range, return maximum value. */
        return (0x7F ^ mask);
    else
    {
        uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
        return (uval ^ mask);
    }

}

//  wmay - remove ulaw2linear, alaw2ulaw, ulaw2alaw
