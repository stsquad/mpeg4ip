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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 * 
 * Contributor(s): 
 *			Cesar Gonzalez		cesar@eureka-sistemas.com
 */

#ifndef __LOOP_FEEDER_SINK_H__
#define __LOOP_FEEDER_SINK_H__ 1

#include "media_sink.h"
#include "mpeg4ip_config.h"
#include "loop_source.h"

class CAVMediaFlow;

class CLoopFeederSink : public CMediaSink {

protected:
	int ThreadMain(void);

	void DoStartSink(void);
	void DoStopSink(void);
	void DoWriteFrame(CMediaFrame* pFrame);

protected:
	static const u_int16_t MAX_INPUTS = 5;
	CLiveConfig *pConfigAux[MAX_INPUTS];
	CLoopSource *pInputAudio[MAX_INPUTS];
	CLoopSource *pInputVideo[MAX_INPUTS];
	CAVMediaFlow *pFlow[MAX_INPUTS];
};

#endif /* __FEEDER_SINK_H__ */
