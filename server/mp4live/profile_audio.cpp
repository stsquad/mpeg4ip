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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May 		wmay@cisco.com
 */

#define DECLARE_CONFIG_VARIABLES 1
#include "profile_audio.h"
#undef DECLARE_CONFIG_VARIABLES
#include "audio_encoder.h"

void CAudioProfile::LoadConfigVariables (void)
{
  AddConfigVariables(AudioProfileConfigVariables, 
		     NUM_ELEMENTS_IN_ARRAY(AudioProfileConfigVariables));
  // eventually will add interface to read each encoder's variables
}

void CAudioProfile::Update (void)
{
  AudioProfileCheck(this);
}
