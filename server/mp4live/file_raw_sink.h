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
 *		Bill May 		wmay@cisco.com
 */

#ifndef __FILE_RAW_SINK_H__
#define __FILE_RAW_SINK_H__

#include "media_sink.h"

class CRawFileSink : public CMediaSink {
public:
	CRawFileSink() : CMediaSink() {
		m_pcmFile = -1;
		m_yuvFile = -1;
	}

protected:
	int ThreadMain(void);

	void DoStartSink(void);
	void DoStopSink(void);
	void DoWriteFrame(CMediaFrame* pFrame);

	int OpenFile(const char* fileName, bool useFifo);

protected:
	int		m_pcmFile;
	int		m_yuvFile;
};

#endif /* __RAW_FILE_SINK_H__ */
