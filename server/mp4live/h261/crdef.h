/*
 * crdef.h --
 *
 *      The bit definitions for the values stored in the conditional
 *      replenishment vector.
 *
 * Copyright (c) 1993-2002 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * A. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * B. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * C. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#) $Header: /cvsroot/mpeg4ip/mpeg4ip/server/mp4live/h261/crdef.h,v 1.4 2003/09/12 23:19:43 wmaycisco Exp $
 */

/*
 * The bit definitions for the values stored in the conditional
 * replenishment vector.  We use a variant of the algorithm used
 * in nv (which is very similar to the algorithms in many hardware
 * codecs) where we only send a block if it changes beyond some threshold
 * due to scene motion.  In this case, we send it at a "low-quality"
 * to trade of quality for frame rate in areas of high motion.
 * We then age the block and after it hasn't changed for a few frames
 * we send a higher quality version.  Finally, the process that scans
 * the background filling in blocks sends the highest quality version.
 *
 * A finite state machine defines the algorithm.  When motion is detected,
 * the block is reset to state MOTION.  On no motion, it is aged to AGE1,
 * AGE2, ... etc., until it reaches state AGEn (the age threshold), and
 * then it transitions to IDLE.  If the background process finds a block
 * in the IDLE state, it can promote it to the BG state.  On the next
 * frame, it reverts to the IDLE state.
 *
 * Blocks are transmitted only in the MOTION, AGEn, and BG state,
 * at low, medium, and high quality, respectively.  The high bit
 * is reserved to indicate the block is in one of these states
 * and should be sent.
 */

#ifndef mash_crdef_h
#define mash_crdef_h

#include "mpeg4ip.h"
#include <sys/types.h>

#define CR_SEND		0xc0
#define CR_LQ		0x40
#define CR_MQ		0x80
#define CR_HQ		0xc0
#define CR_QUALITY(s)	((s) & CR_SEND)

#define CR_AGE_BIT	0x20
#define CR_MOTION_BIT	0x10
/*
 * The threshold for aging a block.  This value cannot
 * be larger than 15 without changing the layout of
 * CR record (i.e., it would need to otherwise be wider)
 */
#define CR_AGETHRESH	15

#ifdef __cplusplus

class ConditionalReplenisher {
public:
	ConditionalReplenisher();
	~ConditionalReplenisher();
	void crinit(int w, int h);
	int age_blocks();
	void send_all();
	void mark_all_send();
	inline void fillrate(int v) { idle_high_ = v; }
protected:
	int get_level();
	int frmno_;
	int blkw_;
	int blkh_;
	int scan_;
	int nblk_;
	u_char* crvec_;
	int rover_;
private:
	int idle_low_;
	int idle_high_;

};
#endif
#endif /* mash_crdef_h */
