/*
 * crdef.cc --
 *
 *      Conditional Replenisher object source code
 *
 * Copyright (c) 1997-2002 The Regents of the University of California.
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
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/mpeg4ip/mpeg4ip/server/mp4live/h261/crdef.cpp,v 1.1 2003/04/09 00:44:42 wmaycisco Exp $";
#endif

#include "crdef.h"

ConditionalReplenisher::ConditionalReplenisher()
	:frmno_(0), crvec_(0), rover_(0)
{
	/*FIXME*/
	idle_low_ = 2;
	idle_high_ = 2;
}

ConditionalReplenisher::~ConditionalReplenisher()
{
	delete[] crvec_;
}

#define NLEVEL 4
int levelMap[] = { 0, 2, 1, 2}; /*FIXME needs to be parameterizable from tcl*/

int ConditionalReplenisher::get_level()
{
	/*FIXME this needs to be parameterizable from tcl*/
	return (frmno_ & (NLEVEL - 1));
}

void ConditionalReplenisher::crinit(int w, int h)
{
	blkw_ = w >> 4;
	blkh_ = h >> 4;
	scan_ = 0;
	nblk_ = blkw_ * blkh_;
	delete[] crvec_;
	crvec_ = new u_char[nblk_];
	send_all();
}


void ConditionalReplenisher::mark_all_send()
{
	if (crvec_ != 0) {
		for (int i = 0; i < nblk_; ++i)
			crvec_[i] = CR_SEND;
	}
}


void ConditionalReplenisher::send_all()
{
	if (crvec_ != 0) {
		for (int i = 0; i < nblk_; ++i)
			crvec_[i] = CR_MOTION_BIT|CR_LQ;
	}
}

int ConditionalReplenisher::age_blocks()
{
	++frmno_;
	int level = get_level();
	int hlev = levelMap[level];

	for (int i = 0; i < nblk_; ++i) {
		int s = crvec_[i] & 0x3f;
		/*
		 * Age this block.
		 * Once we hit the age threshold, we
		 * set CR_SEND as a hint to send a
		 * higher-quality version of the block.
		 * After this the block will stop aging,
		 * until there is motion.  In the meantime,
		 * we might send it as background fill
		 * using the highest quality.
		 */
		if (s & CR_AGE_BIT) {
			int age = s & 0x1f;
			age += 1;
			if (age >= CR_AGETHRESH)
				s = CR_MQ;
			else
				s = CR_AGE_BIT | age;
		} else if (s & CR_MOTION_BIT) {
			int slev = s & 0xf;
			if (hlev == 0) {
				s = CR_AGE_BIT;
				if (slev > 0)
					s |= CR_LQ;
			} else if (hlev < slev)
				s = CR_LQ|CR_MOTION_BIT | hlev;
		}
		crvec_[i] = s;
	}
	/*
	 * Bump the CR scan pointer.  This variable controls which
	 * scan line of a block we use to make the replenishment
	 * decision.  We skip 3 lines at a time to quickly precess
	 * over the block.  Since 3 and 8 are coprime, we will
	 * sweep out every line.
	 */
	scan_ = (scan_ + 3) & 7;

	/*
	 * Now go through and look for some idle blocks to send
	 * as background fill.  But only do background fill
	 * on the base layer.
	 */
	if (hlev != 0)
		return (hlev);

	int blkno = rover_;
#ifdef notdef
	int n = (delta_ * 2. < frametime_)? idle_high_ : idle_low_;
#endif
	/* FIXME */
	int n = 2;
	while (n > 0) {
		int s = crvec_[blkno];
		/*
		 * If this block isn't being sent because of
		 * motion, send it as a high-quality background
		 * block.  We used to check also that the block
		 * was not aging, but removed this constraint since
		 * at low bandwidth it takes too long to cycle
		 * through the whole image...
		 */
		if ((s & CR_MOTION_BIT) == 0) {
			crvec_[blkno] = CR_HQ;
			--n;
		}
		if (++blkno >= nblk_) {
			blkno = 0;
			/* one way to guarantee loop termination */
			break;
		}
	}
	rover_ = blkno;

	return (hlev);
}
