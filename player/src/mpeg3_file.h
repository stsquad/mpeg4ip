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
 * Copyright (C) Cisco Systems Inc. 2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * mpeg3_file.h - contains interfacing to mpg files for local playback
 */
#ifndef __MPEG3_FILE_H__
#define __MPEG3_FILE_H__ 1
#include <mpeg2_ps.h>
#include "mpeg4ip_sdl_includes.h"

class CPlayerSession;

int create_media_for_mpeg_file (CPlayerSession *psptr,
				const char *name,
				int have_audio_driver,
				control_callback_vft_t *cc_vft);

#endif
