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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4live.h"
#include "audio_encoder.h"

#ifdef ADD_LAME_ENCODER
#include "audio_lame.h"
#endif

#ifdef ADD_FAAC_ENCODER
#include "audio_faac.h"
#endif


CAudioEncoder* AudioEncoderCreate(const char* encoderName)
{
	if (!strcasecmp(encoderName, AUDIO_ENCODER_FAAC)) {
#ifdef ADD_FAAC_ENCODER
		return new CFaacAudioEncoder();
#else
		error_message("faac encoder not available in this build");
		return false;
#endif
	} else if (!strcasecmp(encoderName, AUDIO_ENCODER_LAME)) {
#ifdef ADD_LAME_ENCODER
		return new CLameAudioEncoder();
#else
		error_message("lame encoder not available in this build");
#endif
	} else {
		error_message("unknown encoder specified");
	}
	return NULL;
}

