/*
 * FILE:     rtp.c
 * AUTHOR:   Colin Perkins   <c.perkins@cs.ucl.ac.uk>
 * MODIFIED: Orion Hodson    <o.hodson@cs.ucl.ac.uk>
 *           Markus Germeier <mager@tzi.de>
 *           Bill Fenner     <fenner@research.att.com>
 *           Timur Friedman  <timur@research.att.com>
 *
 * The routines in this file implement the Real-time Transport Protocol,
 * RTP, as specified in RFC1889 with current updates under discussion in
 * the IETF audio/video transport working group. Portions of the code are
 * derived from the algorithms published in that specification.
 *
 * $Revision: 1.2 $ 
 * $Date: 2001/05/09 21:15:12 $
 * 
 * Copyright (c) 1998-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London.
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "config_unix.h"
#include "config_win32.h"
#include "memory.h"
#include "debug.h"
#include "net_udp.h"
#include "crypt_random.h"
#include "drand48.h"
#include "gettimeofday.h"
#include "qfDES.h"
#include "md5.h"
#include "ntp.h"

#include "rtp.h"

#define MAX_DROPOUT    3000
#define MAX_MISORDER   100
// wmay - changed to 1
#define MIN_SEQUENTIAL 1

#if 0
//wmay
#define RTP_SEQ_PUSH_MAX 12
uint16_t rtp_seq_push[16] = { 0xfffe, 1, 0xffff, 0, 2, 3, 5, 7, 6, 8, 9, 324, 10, 0xffff };
int rtp_seq_push_ix = 0;
#endif

/*
 * Definitions for the RTP/RTCP packets on the wire...
 */

#define RTP_SEQ_MOD        0x10000
#define RTP_MAX_SDES_LEN   256

#define RTCP_SR   200
#define RTCP_RR   201
#define RTCP_SDES 202
#define RTCP_BYE  203
#define RTCP_APP  204

typedef struct {
#ifdef WORDS_BIGENDIAN
	unsigned short  version:2;	/* packet type            */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  count:5;	/* varies by payload type */
	unsigned short  pt:8;		/* payload type           */
#else
	unsigned short  count:5;	/* varies by payload type */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  version:2;	/* packet type            */
	unsigned short  pt:8;		/* payload type           */
#endif
	uint16_t         length;	/* packet length          */
} rtcp_common;

typedef struct {
	rtcp_common   common;	
	union {
		struct {
			rtcp_sr		sr;
			rtcp_rr       	rr[1];		/* variable-length list */
		} sr;
		struct {
			uint32_t         ssrc;		/* source this RTCP packet is coming from */
			rtcp_rr       	rr[1];		/* variable-length list */
		} rr;
		struct rtcp_sdes_t {
			uint32_t	ssrc;
			rtcp_sdes_item 	item[1];	/* list of SDES */
		} sdes;
		struct {
			uint32_t         ssrc[1];	/* list of sources */
							/* can't express the trailing text... */
		} bye;
		struct {
			uint32_t         ssrc;           
			uint8_t          name[4];
			uint8_t          data[1];
		} app;
	} r;
} rtcp_t;

typedef struct _rtcp_rr_wrapper {
	struct _rtcp_rr_wrapper	*next;
	struct _rtcp_rr_wrapper	*prev;
        uint32_t                  reporter_ssrc;
	rtcp_rr			*rr;
	struct timeval		*ts;	/* Arrival time of this RR */
} rtcp_rr_wrapper;

/*
 * The RTP database contains source-specific information needed 
 * to make it all work. 
 */

typedef struct _source {
	struct _source	*next;
	struct _source	*prev;
	uint32_t	 ssrc;
	char		*cname;
	char		*name;
	char		*email;
	char		*phone;
	char		*loc;
	char		*tool;
	char		*note;
	rtcp_sr		*sr;
	struct timeval	 last_sr;
	struct timeval	 last_active;
	int		 sender;
	int		 got_bye;	/* TRUE if we've received an RTCP bye from this source */
	uint32_t	 base_seq;
	uint16_t	 max_seq;
	uint32_t	 bad_seq;
	uint32_t	 cycles;
	int		 received;
	int		 received_prior;
	int		 expected_prior;
	int		 probation;
	uint32_t	 jitter;
	uint32_t	 transit;
	uint32_t	 magic;		/* For debugging... */
} source;

/* The size of the hash table used to hold the source database. */
/* Should be large enough that we're unlikely to get collisions */
/* when sources are added, but not too large that we waste too  */
/* much memory. Sedgewick ("Algorithms", 2nd Ed, Addison-Wesley */
/* 1988) suggests that this should be around 1/10th the number  */
/* of entries that we expect to have in the database and should */
/* be a prime number. Everything continues to work if this is   */
/* too low, it just goes slower... for now we assume around 100 */
/* participants is a sensible limit so we set this to 11.       */   
#define RTP_DB_SIZE	11

/*
 *  Options for an RTP session are stored in the "options" struct.
 */

typedef struct {
	int 	promiscuous_mode;
	int	wait_for_rtcp;
	int	filter_my_packets;
} options;

/*
 * The "struct rtp" defines an RTP session.
 */

struct rtp {
	socket_udp	*rtp_socket;
	socket_udp	*rtcp_socket;
	char		*addr;
	uint16_t	 rx_port;
	uint16_t	 tx_port;
	int		 ttl;
	uint32_t	 my_ssrc;
	source		*db[RTP_DB_SIZE];
        rtcp_rr_wrapper  rr[RTP_DB_SIZE][RTP_DB_SIZE]; 	/* Indexed by [hash(reporter)][hash(reportee)] */
	options		*opt;
	void		*userdata;
	int		 invalid_rtp_count;
	int		 invalid_rtcp_count;
	int		 ssrc_count;
	int		 ssrc_count_prev;		/* ssrc_count at the time we last recalculated our RTCP interval */
	int		 sender_count;
	int		 initial_rtcp;
	double		 avg_rtcp_size;
	int		 we_sent;
	double		 rtcp_bw;
	struct timeval	 last_update;
	struct timeval	 last_rtcp_send_time;
	struct timeval	 next_rtcp_send_time;
	double		 rtcp_interval;
	int		 sdes_count_pri;
	int		 sdes_count_sec;
	int		 sdes_count_ter;
	uint16_t	 rtp_seq;
	uint32_t	 rtp_pcount;
	uint32_t	 rtp_bcount;
        char            *encryption_key;
	void (*callback)(struct rtp *session, rtp_event *event);
	uint32_t	 magic;				/* For debugging...  */
};

static int filter_event(struct rtp *session, uint32_t ssrc)
{
	int	filter;

	rtp_getopt(session, RTP_OPT_FILTER_MY_PACKETS, &filter);
	return filter && (ssrc == rtp_my_ssrc(session));
}

static double tv_diff(struct timeval curr_time, struct timeval prev_time)
{
    /* Return curr_time - prev_time */
    double	ct, pt;

    ct = (double) curr_time.tv_sec + (((double) curr_time.tv_usec) / 1000000.0);
    pt = (double) prev_time.tv_sec + (((double) prev_time.tv_usec) / 1000000.0);
    return (ct - pt);
}

static void tv_add(struct timeval *ts, double offset)
{
	/* Add offset seconds to ts */
	double offset_sec, offset_usec;

	offset_usec = modf(offset, &offset_sec) * 1000000;
	ts->tv_sec  += (long) offset_sec;
	ts->tv_usec += (long) offset_usec;
	if (ts->tv_usec > 1000000) {
		ts->tv_sec++;
		ts->tv_usec -= 1000000;
	}
}

static int tv_gt(struct timeval a, struct timeval b)
{
	/* Returns (a>b) */
	if (a.tv_sec > b.tv_sec) {
		return TRUE;
	}
	if (a.tv_sec < b.tv_sec) {
		return FALSE;
	}
	assert(a.tv_sec == b.tv_sec);
	return a.tv_usec > b.tv_usec;
}

static int ssrc_hash(uint32_t ssrc)
{
	/* Hash from an ssrc to a position in the source database.   */
	/* Assumes that ssrc values are uniformly distributed, which */
	/* should be true but probably isn't (Rosenberg has reported */
	/* that many implementations generate ssrc values which are  */
	/* not uniformly distributed over the space, and the H.323   */
	/* spec requires that they are non-uniformly distributed).   */
	/* This routine is written as a function rather than inline  */
	/* code to allow it to be made smart in future: probably we  */
	/* should run MD5 on the ssrc and derive a hash value from   */
	/* that, to ensure it's more uniformly distributed?          */
	return ssrc % RTP_DB_SIZE;
}

static void insert_rr(struct rtp *session, uint32_t reporter_ssrc, rtcp_rr *rr, struct timeval *ts)
{
        /* Insert the reception report into the receiver report      */
        /* database. This database is a two dimensional table of     */
        /* rr_wrappers indexed by hashes of reporter_ssrc and        */
        /* reportee_src.  The rr_wrappers in the database are        */
        /* sentinels to reduce conditions in list operations.        */
	/* The ts is used to determine when to timeout this rr.      */

        rtcp_rr_wrapper *cur, *start;

        start = &session->rr[ssrc_hash(reporter_ssrc)][ssrc_hash(rr->ssrc)];
        cur   = start->next;

        while (cur != start) {
                if (cur->reporter_ssrc == reporter_ssrc && cur->rr->ssrc == rr->ssrc) {
                        /* Replace existing entry in the database  */
                        xfree(cur->rr);
			xfree(cur->ts);
                        cur->rr = rr;
			cur->ts = (struct timeval *) xmalloc(sizeof(struct timeval));
			memcpy(cur->ts, ts, sizeof(struct timeval));
                        return;
                }
                cur = cur->next;
        }
        
        /* No entry in the database so create one now. */
        cur = (rtcp_rr_wrapper*)xmalloc(sizeof(rtcp_rr_wrapper));
        cur->reporter_ssrc = reporter_ssrc;
        cur->rr = rr;
	cur->ts = (struct timeval *) xmalloc(sizeof(struct timeval));
	memcpy(cur->ts, ts, sizeof(struct timeval));
        /* Fix links */
        cur->next       = start->next;
        cur->next->prev = cur;
        cur->prev       = start;
        cur->prev->next = cur;

        debug_msg("Created new rr entry for 0x%08lx from source 0x%08lx\n", rr->ssrc, reporter_ssrc);
        return;
}

static void remove_rr(struct rtp *session, uint32_t ssrc)
{
	/* Remove any RRs from "s" which refer to "ssrc" as either   */
        /* reporter or reportee.                                     */
        rtcp_rr_wrapper *start, *cur, *tmp;
        int i;

        /* Remove rows, i.e. ssrc == reporter_ssrc                   */
        for(i = 0; i < RTP_DB_SIZE; i++) {
                start = &session->rr[ssrc_hash(ssrc)][i];
                cur   = start->next;
                while (cur != start) {
                        if (cur->reporter_ssrc == ssrc) {
                                tmp = cur;
                                cur = cur->prev;
                                tmp->prev->next = tmp->next;
                                tmp->next->prev = tmp->prev;
				xfree(tmp->ts);
                                xfree(tmp->rr);
                                xfree(tmp);
                        }
                        cur = cur->next;
                }
        }

        /* Remove columns, i.e.  ssrc == reporter_ssrc */
        for(i = 0; i < RTP_DB_SIZE; i++) {
                start = &session->rr[i][ssrc_hash(ssrc)];
                cur   = start->next;
                while (cur != start) {
                        if (cur->rr->ssrc == ssrc) {
                                tmp = cur;
                                cur = cur->prev;
                                tmp->prev->next = tmp->next;
                                tmp->next->prev = tmp->prev;
				xfree(tmp->ts);
                                xfree(tmp->rr);
                                xfree(tmp);
                        }
                        cur = cur->next;
                }
        }
}

static void timeout_rr(struct rtp *session, struct timeval *curr_ts)
{
	/* Timeout any reception reports which have been in the database for more than 3 */
	/* times the RTCP reporting interval without refresh.                            */
        rtcp_rr_wrapper *start, *cur, *tmp;
	rtp_event	 event;
        int 		 i, j;

        for(i = 0; i < RTP_DB_SIZE; i++) {
        	for(j = 0; j < RTP_DB_SIZE; j++) {
			start = &session->rr[i][j];
			cur   = start->next;
			while (cur != start) {
				if (tv_diff(*curr_ts, *(cur->ts)) > (session->rtcp_interval * 3)) {
					/* Signal the application... */
					if (!filter_event(session, cur->reporter_ssrc)) {
						event.ssrc = cur->reporter_ssrc;
						event.type = RR_TIMEOUT;
						event.data = cur->rr;
						event.ts   = curr_ts;
						session->callback(session, &event);
					}
					/* Delete this reception report... */
					tmp = cur;
					cur = cur->prev;
					tmp->prev->next = tmp->next;
					tmp->next->prev = tmp->prev;
					xfree(tmp->ts);
					xfree(tmp->rr);
					xfree(tmp);
				}
				cur = cur->next;
			}
		}
	}
}

static const rtcp_rr* get_rr(struct rtp *session, uint32_t reporter_ssrc, uint32_t reportee_ssrc)
{
        rtcp_rr_wrapper *cur, *start;

        start = &session->rr[ssrc_hash(reporter_ssrc)][ssrc_hash(reportee_ssrc)];
        cur   = start->next;
        while (cur != start) {
                if (cur->reporter_ssrc == reporter_ssrc &&
                    cur->rr->ssrc      == reportee_ssrc) {
                        return cur->rr;
                }
                cur = cur->next;
        }
        return NULL;
}

static void check_source(source *s)
{
	assert(s != NULL);
	assert(s->magic == 0xc001feed);
}

static void check_database(struct rtp *session)
{
	/* This routine performs a sanity check on the database. */
	/* This should not call any of the other routines which  */
	/* manipulate the database, to avoid common failures.    */
#ifdef DEBUG
	source 	 	*s;
	int	 	 source_count;
	int		 chain;
#endif
	assert(session != NULL);
	assert(session->magic == 0xfeedface);
#ifdef DEBUG
	/* Check that we have a database entry for our ssrc... */
	/* We only do this check if ssrc_count > 0 since it is */
	/* performed during initialisation whilst creating the */
	/* source entry for my_ssrc.                           */
	if (session->ssrc_count > 0) {
		for (s = session->db[ssrc_hash(session->my_ssrc)]; s != NULL; s = s->next) {
			if (s->ssrc == session->my_ssrc) {
				break;
			}
		}
		assert(s != NULL);
	}

	source_count = 0;
	for (chain = 0; chain < RTP_DB_SIZE; chain++) {
		/* Check that the linked lists making up the chains in */
		/* the hash table are correctly linked together...     */
		for (s = session->db[chain]; s != NULL; s = s->next) {
			check_source(s);
			source_count++;
			if (s->prev == NULL) {
				assert(s == session->db[chain]);
			} else {
				assert(s->prev->next == s);
			}
			if (s->next != NULL) {
				assert(s->next->prev == s);
			}
			/* Check that the SR is for this source... */
			if (s->sr != NULL) {
				assert(s->sr->ssrc == s->ssrc);
			}
		}
	}
	/* Check that the number of entries in the hash table  */
	/* matches session->ssrc_count                         */
	assert(source_count == session->ssrc_count);
#endif
}

static source *get_source(struct rtp *session, uint32_t ssrc)
{
	source *s;

	check_database(session);
	for (s = session->db[ssrc_hash(ssrc)]; s != NULL; s = s->next) {
		if (s->ssrc == ssrc) {
			check_source(s);
			return s;
		}
	}
	return NULL;
}

static source *create_source(struct rtp *session, uint32_t ssrc)
{
	/* Create a new source entry, and add it to the database.    */
	/* The database is a hash table, using the separate chaining */
	/* algorithm.                                                */
	rtp_event	 event;
	struct timeval	 event_ts;
	source		*s = get_source(session, ssrc);
	int		 h;

	if (s != NULL) {
		/* Source is already in the database... Mark it as */
		/* active and exit (this is the common case...)    */
		gettimeofday(&(s->last_active), NULL);
		return s;
	}
	check_database(session);
	/* This is a new source, we have to create it... */
	h = ssrc_hash(ssrc);
	s = (source *) xmalloc(sizeof(source));
	s->magic          = 0xc001feed;
	s->next           = session->db[h];
	s->prev           = NULL;
	s->ssrc           = ssrc;
	s->cname          = NULL;
	s->name           = NULL;
	s->email          = NULL;
	s->phone          = NULL;
	s->loc            = NULL;
	s->tool           = NULL;
	s->note           = NULL;
	s->sr             = NULL;
	s->got_bye        = FALSE;
	s->sender         = FALSE;
	s->base_seq       = 0;
	s->max_seq        = 0;
	s->bad_seq        = 0;
	s->cycles         = 0;
	s->received       = 0;
	s->received_prior = 0;
	s->expected_prior = 0;
	s->probation      = -1;
	s->jitter         = 0;
	s->transit        = 0;
	gettimeofday(&(s->last_active), NULL);
	/* Now, add it to the database... */
	if (session->db[h] != NULL) {
		session->db[h]->prev = s;
	}
	session->db[ssrc_hash(ssrc)] = s;
	session->ssrc_count++;
	check_database(session);
	// wmay - be consistent - print as a decimal
	debug_msg("Created database entry for ssrc %08lu\n", ssrc);
        if (ssrc != session->my_ssrc) {
                /* Do not send during rtp_init since application cannot map the address */
                /* of the rtp session to anything since rtp_init has not returned yet.  */
		if (!filter_event(session, ssrc)) {
			gettimeofday(&event_ts, NULL);
			event.ssrc = ssrc;
			event.type = SOURCE_CREATED;
			event.data = NULL;
			event.ts   = &event_ts;
			session->callback(session, &event);
		}
        }

	return s;
}

static void delete_source(struct rtp *session, uint32_t ssrc)
{
	/* Remove a source from the RTP database... */
	source		*s = get_source(session, ssrc);
	int		 h = ssrc_hash(ssrc);
	rtp_event	 event;
	struct timeval	 event_ts;

	assert(s != NULL);	/* Deleting a source which doesn't exist is an error... */

	gettimeofday(&event_ts, NULL);

	check_source(s);
	check_database(session);
	if (session->db[h] == s) {
		/* It's the first entry in this chain... */
		session->db[h] = s->next;
		if (s->next != NULL) {
			s->next->prev = NULL;
		}
	} else {
		assert(s->prev != NULL);	/* Else it would be the first in the chain... */
		s->prev->next = s->next;
		if (s->next != NULL) {
			s->next->prev = s->prev;
		}
	}
	/* Free the memory allocated to a source... */
	if (s->cname != NULL) xfree(s->cname);
	if (s->name  != NULL) xfree(s->name);
	if (s->email != NULL) xfree(s->email);
	if (s->phone != NULL) xfree(s->phone);
	if (s->loc   != NULL) xfree(s->loc);
	if (s->tool  != NULL) xfree(s->tool);
	if (s->note  != NULL) xfree(s->note);
	if (s->sr    != NULL) xfree(s->sr);

        remove_rr(session, ssrc); 

	/* Reduce our SSRC count, and perform reverse reconsideration on the RTCP */
	/* reporting interval (draft-ietf-avt-rtp-new-05.txt, section 6.3.4). To  */
	/* make the transmission rate of RTCP packets more adaptive to changes in */
	/* group membership, the following "reverse reconsideration" algorithm    */
	/* SHOULD be executed when a BYE packet is received that reduces members  */
	/* to a value less than pmembers:                                         */
	/* o  The value for tn is updated according to the following formula:     */
	/*       tn = tc + (members/pmembers)(tn - tc)                            */
	/* o  The value for tp is updated according the following formula:        */
	/*       tp = tc - (members/pmembers)(tc - tp).                           */
	/* o  The next RTCP packet is rescheduled for transmission at time tn,    */
	/*    which is now earlier.                                               */
	/* o  The value of pmembers is set equal to members.                      */
	session->ssrc_count--;
	if (session->ssrc_count < session->ssrc_count_prev) {
		tv_add(&(session->next_rtcp_send_time), (session->ssrc_count / session->ssrc_count_prev) 
						     * tv_diff(session->next_rtcp_send_time, event_ts));
		tv_add(&(session->last_rtcp_send_time), - ((session->ssrc_count / session->ssrc_count_prev) 
						     * tv_diff(event_ts, session->next_rtcp_send_time)));
		session->ssrc_count_prev = session->ssrc_count;
	}

	/* Signal to the application that this source is dead... */
	if (!filter_event(session, ssrc)) {
		event.ssrc = ssrc;
		event.type = SOURCE_DELETED;
		event.data = NULL;
		event.ts   = &event_ts;
		session->callback(session, &event);
	}
        xfree(s);
	check_database(session);
}

static void init_seq(source *s, uint16_t seq)
{
	/* Taken from draft-ietf-avt-rtp-new-01.txt */
	check_source(s);
	s->base_seq = seq;
	s->max_seq = seq;
	s->bad_seq = RTP_SEQ_MOD + 1;
	s->cycles = 0;
	s->received = 0;
	s->received_prior = 0;
	s->expected_prior = 0;
}

static int update_seq(source *s, uint16_t seq)
{
	/* Taken from draft-ietf-avt-rtp-new-01.txt */
	uint16_t udelta = seq - s->max_seq;

	/*
	 * Source is not valid until MIN_SEQUENTIAL packets with
	 * sequential sequence numbers have been received.
	 */
	check_source(s);
	if (s->probation) {
		  /* packet is in sequence */
		  if (seq == s->max_seq + 1) {
				s->probation--;
				s->max_seq = seq;
				if (s->probation == 0) {
					 init_seq(s, seq);
					 s->received++;
					 return 1;
				}
		  } else {
				s->probation = MIN_SEQUENTIAL - 1;
				s->max_seq = seq;
		  }
		  return 0;
	} else if (udelta < MAX_DROPOUT) {
		  /* in order, with permissible gap */
		  if (seq < s->max_seq) {
				/*
				 * Sequence number wrapped - count another 64K cycle.
				 */
				s->cycles += RTP_SEQ_MOD;
		  }
		  s->max_seq = seq;
	} else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
		  /* the sequence number made a very large jump */
		  if (seq == s->bad_seq) {
				/*
				 * Two sequential packets -- assume that the other side
				 * restarted without telling us so just re-sync
				 * (i.e., pretend this was the first packet).
				 */
				init_seq(s, seq);
		  } else {
				s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
				return 0;
		  }
	} else {
		  /* duplicate or reordered packet */
	}
	s->received++;
	return 1;
}

static double rtcp_interval(struct rtp *session)
{
	/* Minimum average time between RTCP packets from this site (in   */
	/* seconds).  This time prevents the reports from `clumping' when */
	/* sessions are small and the law of large numbers isn't helping  */
	/* to smooth out the traffic.  It also keeps the report interval  */
	/* from becoming ridiculously small during transient outages like */
	/* a network partition.                                           */
	double const RTCP_MIN_TIME = 5.0;
	/* Fraction of the RTCP bandwidth to be shared among active       */
	/* senders.  (This fraction was chosen so that in a typical       */
	/* session with one or two active senders, the computed report    */
	/* time would be roughly equal to the minimum report time so that */
	/* we don't unnecessarily slow down receiver reports.) The        */
	/* receiver fraction must be 1 - the sender fraction.             */
	double const RTCP_SENDER_BW_FRACTION = 0.25;
	double const RTCP_RCVR_BW_FRACTION   = (1-RTCP_SENDER_BW_FRACTION);
	/* To compensate for "unconditional reconsideration" converging   */
	/* to a value below the intended average.                         */
	double const COMPENSATION            = 2.71828 - 1.5;

	double t;				              /* interval */
	double rtcp_min_time = RTCP_MIN_TIME;
	int n;			        /* no. of members for computation */
	double rtcp_bw = session->rtcp_bw;

	/* Very first call at application start-up uses half the min      */
	/* delay for quicker notification while still allowing some time  */
	/* before reporting for randomization and to learn about other    */
	/* sources so the report interval will converge to the correct    */
	/* interval more quickly.                                         */
	if (session->initial_rtcp) {
		rtcp_min_time /= 2;
	}

	/* If there were active senders, give them at least a minimum     */
	/* share of the RTCP bandwidth.  Otherwise all participants share */
	/* the RTCP bandwidth equally.                                    */
	n = session->ssrc_count;
	if (session->sender_count > 0 && session->sender_count < session->ssrc_count * RTCP_SENDER_BW_FRACTION) {
		if (session->we_sent) {
			rtcp_bw *= RTCP_SENDER_BW_FRACTION;
			n = session->sender_count;
		} else {
			rtcp_bw *= RTCP_RCVR_BW_FRACTION;
			n -= session->sender_count;
		}
	}

	/* The effective number of sites times the average packet size is */
	/* the total number of octets sent when each site sends a report. */
	/* Dividing this by the effective bandwidth gives the time        */
	/* interval over which those packets must be sent in order to     */
	/* meet the bandwidth target, with a minimum enforced.  In that   */
	/* time interval we send one report so this time is also our      */
	/* average time between reports.                                  */
	t = session->avg_rtcp_size * n / rtcp_bw;
	if (t < rtcp_min_time) {
		t = rtcp_min_time;
	}
	session->rtcp_interval = t;

	/* To avoid traffic bursts from unintended synchronization with   */
	/* other sites, we then pick our actual next report interval as a */
	/* random number uniformly distributed between 0.5*t and 1.5*t.   */
	return (t * (drand48() + 0.5)) / COMPENSATION;
}

#define MAXCNAMELEN	255

static char *get_cname(socket_udp *s)
{
        /* Set the CNAME. This is "user@hostname" or just "hostname" if the username cannot be found. */
        const char              *hname;
        char                    *uname;
        char                    *cname;
#ifndef WIN32
        struct passwd           *pwent;
#else
        char *name;
        int   namelen;
#endif

        cname = (char *) xmalloc(MAXCNAMELEN + 1);
        cname[0] = '\0';

        /* First, fill in the username... */
#ifdef WIN32
        name = NULL;
        namelen = 0;
        GetUserName(NULL, &namelen);
        if (namelen > 0) {
                name = (char*)xmalloc(namelen+1);
                GetUserName(name, &namelen);
        } else {
                uname = getenv("USER");
                if (uname != NULL) {
                        name = xstrdup(uname);
                }
        }
        if (name != NULL) {
                strncpy(cname, name, MAXCNAMELEN - 1);
                strcat(cname, "@");
                xfree(name);
        }
#else
        pwent = getpwuid(getuid());
        uname = pwent->pw_name;
        if (uname != NULL) {
                strncpy(cname, uname, MAXCNAMELEN - 1);
                strcat(cname, "@");
        }

#endif
        
        /* Now the hostname. Must be dotted-quad IP address. */
        hname = udp_host_addr(s);
	if (hname == NULL) {
		/* If we can't get our IP address we use the loopback address... */
		/* This is horrible, but it stops the code from failing.         */
		hname = "127.0.0.1";
	}
        strncpy(cname + strlen(cname), hname, MAXCNAMELEN - strlen(cname));
        return cname;
}

static void init_opt(struct rtp *session)
{
	/* Default option settings. */
	rtp_setopt(session, RTP_OPT_PROMISC,           FALSE);
	rtp_setopt(session, RTP_OPT_WEAK_VALIDATION,   TRUE);
	rtp_setopt(session, RTP_OPT_FILTER_MY_PACKETS, FALSE);
}

static void init_rng(const char *s)
{
        static uint32_t seed;
	if (s == NULL) s = "ARANDOMSTRINGSOWEDONTCOREDUMP";
        if (seed == 0) {
                pid_t p = getpid();
		int32_t i, n;
                while (*s) {
                        seed += (uint32_t)*s++;
                        seed = seed * 31 + 1;
                }
                seed = 1 + seed * 31 + (uint32_t)p;
                srand48(seed);
		/* At time of writing we use srand48 -> srand on Win32 which is only 16 bit. */
		/* lrand48 -> rand which is only 15 bits, step a long way through table seq  */
#ifdef WIN32
		n = (seed >> 16) & 0xffff;
		for(i = 0; i < n; i++) {
			seed = lrand48();
		}
#endif /* WIN32 */
		UNUSED(i);
		UNUSED(n);
	}
}

struct rtp *rtp_init(char *addr, uint16_t rx_port, uint16_t tx_port, int ttl, double rtcp_bw, 
                     void (*callback)(struct rtp *session, rtp_event *e),
                     void *userdata)
{
	return rtp_init_if(addr, NULL, rx_port, tx_port, ttl, rtcp_bw, callback, userdata);
}

struct rtp *rtp_init_if(char *addr, char *iface, uint16_t rx_port, uint16_t tx_port, int ttl, double rtcp_bw, 
                        void (*callback)(struct rtp *session, rtp_event *e),
                        void *userdata)
{
	struct rtp 	*session;
	int         	 i, j;
	char		*cname;

        if (ttl < 0) {
                debug_msg("ttl must be greater than zero\n");
                return NULL;
        }
        if (rx_port % 2) {
                debug_msg("rx_port must be even\n");
                return NULL;
        }
        if (tx_port % 2) {
                debug_msg("tx_port must be even\n");
                return NULL;
        }

	session 		= (struct rtp *) xmalloc(sizeof(struct rtp));
	session->magic		= 0xfeedface;
	session->opt		= (options *) xmalloc(sizeof(options));
	session->userdata	= userdata;
	session->addr		= xstrdup(addr);
	session->rx_port	= rx_port;
	session->tx_port	= tx_port;
	session->ttl		= min(ttl, 127);
	session->rtp_socket	= udp_init_if(addr, iface, rx_port, tx_port, ttl);
	session->rtcp_socket	= udp_init_if(addr, iface, (uint16_t) (rx_port+1), (uint16_t) (tx_port+1), ttl);

	init_opt(session);

	if (session->rtp_socket == NULL || session->rtcp_socket == NULL) {
		xfree(session);
		return NULL;
	}

        init_rng(udp_host_addr(session->rtp_socket));

	session->my_ssrc            = (uint32_t) lrand48();
	session->callback           = callback;
	session->invalid_rtp_count  = 0;
	session->invalid_rtcp_count = 0;
	session->ssrc_count         = 0;
	session->ssrc_count_prev    = 0;
	session->sender_count       = 0;
	session->initial_rtcp       = TRUE;
	session->avg_rtcp_size      = 70.0;	/* Guess for a sensible starting point... */
	session->we_sent            = FALSE;
	session->rtcp_bw            = rtcp_bw;
	session->sdes_count_pri     = 0;
	session->sdes_count_sec     = 0;
	session->sdes_count_ter     = 0;
	session->rtp_seq            = (uint16_t) lrand48();
	session->rtp_pcount         = 0;
	session->rtp_bcount         = 0;
	gettimeofday(&(session->last_update), NULL);
	gettimeofday(&(session->last_rtcp_send_time), NULL);
	gettimeofday(&(session->next_rtcp_send_time), NULL);
        session->encryption_key      = NULL;

	/* Calculate when we're supposed to send our first RTCP packet... */
	tv_add(&(session->next_rtcp_send_time), rtcp_interval(session));

	/* Initialise the source database... */
	for (i = 0; i < RTP_DB_SIZE; i++) {
		session->db[i] = NULL;
	}

        /* Initialize sentinels in rr table */
        for (i = 0; i < RTP_DB_SIZE; i++) {
                for (j = 0; j < RTP_DB_SIZE; j++) {
                        session->rr[i][j].next = &session->rr[i][j];
                        session->rr[i][j].prev = &session->rr[i][j];
                }
        }

	/* Create a database entry for ourselves... */
	create_source(session, session->my_ssrc);
	cname = get_cname(session->rtp_socket);
	rtp_set_sdes(session, session->my_ssrc, RTCP_SDES_CNAME, cname, strlen(cname));
	xfree(cname);	/* cname is copied by rtp_set_sdes()... */

	return session;
}

int rtp_set_my_ssrc(struct rtp *session, uint32_t ssrc)
{
        source *s;
        uint32_t h;
        /* Sets my_ssrc when called immediately after rtp_init. */
        /* Returns TRUE on success, FALSE otherwise.  Needed to */
        /* co-ordinate SSRC's between LAYERED SESSIONS, SHOULD  */
        /* NOT BE USED OTHERWISE.                               */
        if (session->ssrc_count != 1 && session->sender_count != 0) {
                return FALSE;
        }
        /* Remove existing source */
        h = ssrc_hash(session->my_ssrc);
        s = session->db[h];
        session->db[h] = NULL;
        /* Fill in new ssrc       */
        session->my_ssrc = ssrc;
        s->ssrc          = ssrc;
        h                = ssrc_hash(ssrc);
        /* Put source back        */
        session->db[h]   = s;
        return TRUE;
}

int rtp_setopt(struct rtp *session, int optname, int optval)
{
	assert((optval == TRUE) || (optval == FALSE));

	switch (optname) {
		case RTP_OPT_WEAK_VALIDATION:
			session->opt->wait_for_rtcp = optval;
			break;
	        case RTP_OPT_PROMISC:
			session->opt->promiscuous_mode = optval;
			break;
	        case RTP_OPT_FILTER_MY_PACKETS:
			session->opt->filter_my_packets = optval;
			break;
        	default:
			debug_msg("Ignoring unknown option (%d) in call to rtp_setopt().\n", optname);
                        return FALSE;
	}
        return TRUE;
}

int rtp_getopt(struct rtp *session, int optname, int *optval)
{
	switch (optname) {
		case RTP_OPT_WEAK_VALIDATION:
			*optval = session->opt->wait_for_rtcp;
                        break;
        	case RTP_OPT_PROMISC:
			*optval = session->opt->promiscuous_mode;
                        break;
	        case RTP_OPT_FILTER_MY_PACKETS:
			*optval = session->opt->filter_my_packets;
			break;
        	default:
                        *optval = 0;
			debug_msg("Ignoring unknown option (%d) in call to rtp_getopt().\n", optname);
                        return FALSE;
	}
        return TRUE;
}

void *rtp_get_userdata(struct rtp *session)
{
	check_database(session);
	return session->userdata;
}

uint32_t rtp_my_ssrc(struct rtp *session)
{
	check_database(session);
	return session->my_ssrc;
}

static int validate_rtp(rtp_packet *packet, int len)
{
	/* This function checks the header info to make sure that the packet */
	/* is valid. We return TRUE if the packet is valid, FALSE otherwise. */
	/* See Appendix A.1 of the RTP specification.                        */

	/* We only accept RTPv2 packets... */
	if (packet->v != 2) {
		debug_msg("rtp_header_validation: v != 2\n");
		return FALSE;
	}
	/* Check for valid payload types..... 72-76 are RTCP payload type numbers, with */
	/* the high bit missing so we report that someone is running on the wrong port. */
	if (packet->pt >= 72 && packet->pt <= 76) {
		debug_msg("rtp_header_validation: payload-type invalid");
		if (packet->m) {
			debug_msg(" (RTCP packet on RTP port?)");
		}
		debug_msg("\n");
		return FALSE;
	}
	/* Check that the length of the packet is sensible... */
	if (len < (12 + (4 * packet->cc))) {
		debug_msg("rtp_header_validation: packet length is smaller than the header\n");
		return FALSE;
	}
	/* Check that the amount of padding specified is sensible. */
	/* Note: have to include the size of any extension header! */
	if (packet->p) {
		int	payload_len = len - 12 - (packet->cc * 4);
                if (packet->x) {
                        /* extension header and data */
                        payload_len -= 4 * (1 + packet->extn_len);
                }
                if (packet->data[packet->data_len - 1] > payload_len) {
                        debug_msg("rtp_header_validation: padding greater than payload length\n");
                        return FALSE;
                }
                if (packet->data[packet->data_len - 1] < 1) {
			debug_msg("rtp_header_validation: padding zero\n");
			return FALSE;
		}
        }
	return TRUE;
}

static void process_rtp(struct rtp *session, uint32_t curr_rtp_ts, rtp_packet *packet, source *s)
{
	int		 i, d, transit;
	rtp_event	 event;
	struct timeval	 event_ts;

	if (packet->cc > 0) {
		for (i = 0; i < packet->cc; i++) {
			create_source(session, packet->csrc[i]);
		}
	}
	/* Update the source database... */
	if (s->sender == FALSE) {
		s->sender = TRUE;
		session->sender_count++;
	}
	transit    = curr_rtp_ts - packet->ts;
	d      	   = transit - s->transit;
	s->transit = transit;
	if (d < 0) {
		d = -d;
	}
	s->jitter += d - ((s->jitter + 8) / 16);
	
	/* Callback to the application to process the packet... */
	if (!filter_event(session, packet->ssrc)) {
		gettimeofday(&event_ts, NULL);
		event.ssrc = packet->ssrc;
		event.type = RX_RTP;
		event.data = (void *) packet;	/* The callback function MUST free this! */
		event.ts   = &event_ts;
		session->callback(session, &event);
	}
}

static void rtp_recv_data(struct rtp *session, uint32_t curr_rtp_ts)
{
	/* This routine preprocesses an incoming RTP packet, deciding whether to process it. */
	rtp_packet	*packet = (rtp_packet *) xmalloc(RTP_MAX_PACKET_LEN);
	uint8_t		*buffer = ((uint8_t *) packet) + RTP_PACKET_HEADER_SIZE;
	int		 buflen;
	source		*s;
	uint8_t 	 initVec[8] = {0,0,0,0,0,0,0,0};

	buflen = udp_recv(session->rtp_socket, buffer, RTP_MAX_PACKET_LEN - RTP_PACKET_HEADER_SIZE);
	if (buflen > 0) {
		if (session->encryption_key != NULL) {
			qfDES_CBC_d(session->encryption_key, buffer, buflen, initVec);
		}
		/* Convert header fields to host byte order... */
		packet->next = packet->prev = NULL;
		packet->seq      = ntohs(packet->seq);
		packet->ts       = ntohl(packet->ts);
		packet->ssrc     = ntohl(packet->ssrc);
		/* Setup internal pointers, etc... */
		if (packet->cc) {
			packet->csrc = (uint32_t *)(buffer + 12);
		} else {
			packet->csrc = NULL;
		}
		if (packet->x) {
			packet->extn      = buffer + 12 + (packet->cc * 4);
			packet->extn_len  = (packet->extn[2] << 16) | packet->extn[3];
			packet->extn_type = (packet->extn[0] << 16) | packet->extn[1];
		} else {
			packet->extn      = NULL;
			packet->extn_len  = 0;
			packet->extn_type = 0;
		}
		packet->data     = buffer + 12 + (packet->cc * 4);
		packet->data_len = buflen -  (packet->cc * 4) - 12;
		if (packet->extn != NULL) {
			packet->data += ((packet->extn_len + 1) * 4);
			packet->data_len -= ((packet->extn_len + 1) * 4);
		}
		if (validate_rtp(packet, buflen)) {
                        int weak = 0, promisc = 0;
                        rtp_getopt(session, RTP_OPT_WEAK_VALIDATION, &weak);
			if (weak) {
				s = get_source(session, packet->ssrc);
			} else {
				s = create_source(session, packet->ssrc);
			}
                        rtp_getopt(session, RTP_OPT_PROMISC, &promisc);
			if (promisc) {
				if (s == NULL) {
					create_source(session, packet->ssrc);
					s = get_source(session, packet->ssrc);
				}
				process_rtp(session, curr_rtp_ts, packet, s);
				return; /* We don't free "packet", that's done by the callback function... */
			} 
			if (s != NULL) {
				if (s->probation == -1) {
					s->probation = MIN_SEQUENTIAL;
					s->max_seq   = packet->seq - 1;
				}
				if (update_seq(s, packet->seq)) {
#if 0
				  // wmay
				  if (rtp_seq_push_ix <= RTP_SEQ_PUSH_MAX) {
				    packet->seq = rtp_seq_push[rtp_seq_push_ix];
				    rtp_seq_push_ix++;
				  }
#endif
					process_rtp(session, curr_rtp_ts, packet, s);
					return;	/* we don't free "packet", that's done by the callback function... */
				} else {
					/* This source is still on probation... */
					debug_msg("RTP packet from probationary source ignored...\n");
				}
			} else {
			  // wmay - added better error message
				debug_msg("RTP packet from unknown source %d ignored\n", packet->ssrc);
			}
		} else {
			session->invalid_rtp_count++;
			debug_msg("Invalid RTP packet discarded\n");
		}
	}
	xfree(packet);
}

static int validate_rtcp(uint8_t *packet, int len)
{
	/* Validity check for a compound RTCP packet. This function returns */
	/* TRUE if the packet is okay, FALSE if the validity check fails.   */
        /*                                                                  */
	/* The following checks can be applied to RTCP packets [RFC1889]:   */
        /* o RTP version field must equal 2.                                */
        /* o The payload type field of the first RTCP packet in a compound  */
        /*   packet must be equal to SR or RR.                              */
        /* o The padding bit (P) should be zero for the first packet of a   */
        /*   compound RTCP packet because only the last should possibly     */
        /*   need padding.                                                  */
        /* o The length fields of the individual RTCP packets must total to */
        /*   the overall length of the compound RTCP packet as received.    */

	rtcp_t	*pkt  = (rtcp_t *) packet;
	rtcp_t	*end  = (rtcp_t *) (((char *) pkt) + len);
	rtcp_t	*r    = pkt;
	int	 l    = 0;
	int	 pc   = 1;

	/* All RTCP packets must be compound packets (RFC1889, section 6.1) */
	if (((ntohs(pkt->common.length) + 1) * 4) == len) {
		debug_msg("Bogus RTCP packet: not a compound packet\n");
		return FALSE;
	}

	/* Check the RTCP version, payload type and padding of the first in  */
	/* the compund RTCP packet...                                        */
	if (pkt->common.version != 2) {
		debug_msg("Bogus RTCP packet: version number != 2 in the first sub-packet\n");
		return FALSE;
	}
	if (pkt->common.p != 0) {
		debug_msg("Bogus RTCP packet: padding bit is set on first packet in compound\n");
		return FALSE;
	}
	if ((pkt->common.pt != RTCP_SR) && (pkt->common.pt != RTCP_RR)) {
		debug_msg("Bogus RTCP packet: compund packet does not start with SR or RR\n");
		return FALSE;
	}

	/* Check all following parts of the compund RTCP packet. The RTP version */
	/* number must be 2, and the padding bit must be zero on all apart from  */
	/* the last packet.                                                      */
	do {
		if (r->common.version != 2) {
			debug_msg("Bogus RTCP packet: version number != 2 in sub-packet %d\n", pc);
			return FALSE;
		}
		l += (ntohs(r->common.length) + 1) * 4;
		r  = (rtcp_t *) (((uint32_t *) r) + ntohs(r->common.length) + 1);
		pc++;	/* count of sub-packets, for debugging... */
	} while (r < end && r->common.p == 0);

        if (r < end && r->common.p == 1) {
                debug_msg("Bogus RTCP packet: padding bit set before last in compound (sub-packet %d)\n", pc);
                return FALSE;
        }

	/* Check that the length of the packets matches the length of the UDP */
	/* packet in which they were received...                              */
	if (l != len) {
		debug_msg("Bogus RTCP packet: RTCP packet length does not match UDP packet length (%d != %d)\n", l, len);
		return FALSE;
	}
	if (r != end) {
		debug_msg("Bogus RTCP packet: RTCP packet length does not match UDP packet length (%p != %p)\n", r, end);
		return FALSE;
	}

	return TRUE;
}

static void process_report_blocks(struct rtp *session, rtcp_t *packet, uint32_t ssrc, rtcp_rr *rrp, struct timeval *event_ts)
{
	int i;
	rtp_event	 event;
	rtcp_rr		*rr;

	/* ...process RRs... */
	if (packet->common.count == 0) {
		if (!filter_event(session, ssrc)) {
			event.ssrc = ssrc;
			event.type = RX_RR_EMPTY;
			event.data = NULL;
			event.ts   = event_ts;
			session->callback(session, &event);
		}
	} else {
		for (i = 0; i < packet->common.count; i++, rrp++) {
			rr = (rtcp_rr *) xmalloc(sizeof(rtcp_rr));
			rr->ssrc          = ntohl(rrp->ssrc);
			rr->fract_lost    = rrp->fract_lost;	/* Endian conversion handled in the */
			rr->total_lost    = rrp->total_lost;	/* definition of the rtcp_rr type.  */
			rr->last_seq      = ntohl(rrp->last_seq);
			rr->jitter        = ntohl(rrp->jitter);
			rr->lsr           = ntohl(rrp->lsr);
			rr->dlsr          = ntohl(rrp->dlsr);

			/* Create a database entry for this SSRC, if one doesn't already exist... */
			create_source(session, rr->ssrc);

			/* Store the RR for later use... */
			insert_rr(session, ssrc, rr, event_ts);

			/* Call the event handler... */
			if (!filter_event(session, ssrc)) {
				event.ssrc = ssrc;
				event.type = RX_RR;
				event.data = (void *) rr;
				event.ts   = event_ts;
				session->callback(session, &event);
			}
		}
	}
}

static void process_rtcp_sr(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	uint32_t	 ssrc;
	rtp_event	 event;
	rtcp_sr		*sr;
	source		*s;

	ssrc = ntohl(packet->r.sr.sr.ssrc);
	s = create_source(session, ssrc);
	if (s == NULL) {
	  // wmay - be consistent
		debug_msg("Source %08u invalid, skipping...\n", ssrc);
		return;
	}

	/* Process the SR... */
	sr = (rtcp_sr *) xmalloc(sizeof(rtcp_sr));
	sr->ssrc          = ssrc;
	sr->ntp_sec       = ntohl(packet->r.sr.sr.ntp_sec);
	sr->ntp_frac      = ntohl(packet->r.sr.sr.ntp_frac);
	sr->rtp_ts        = ntohl(packet->r.sr.sr.rtp_ts);
	sr->sender_pcount = ntohl(packet->r.sr.sr.sender_pcount);
	sr->sender_bcount = ntohl(packet->r.sr.sr.sender_bcount);

	/* Store the SR for later retrieval... */
	if (s->sr != NULL) {
		xfree(s->sr);
	}
	s->sr = sr;
	s->last_sr = *event_ts;

	/* Call the event handler... */
	if (!filter_event(session, ssrc)) {
		event.ssrc = ssrc;
		event.type = RX_SR;
		event.data = (void *) sr;
		event.ts   = event_ts;
		session->callback(session, &event);
	}

	process_report_blocks(session, packet, ssrc, packet->r.sr.rr, event_ts);

	if (((packet->common.count * 6) + 1) < (ntohs(packet->common.length) - 5)) {
		debug_msg("Profile specific SR extension ignored\n");
	}
}

static void process_rtcp_rr(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	uint32_t		 ssrc;
	source		*s;

	ssrc = ntohl(packet->r.rr.ssrc);
	s = create_source(session, ssrc);
	if (s == NULL) {
	  // wmay - be consistent
		debug_msg("Source %08u invalid, skipping...\n", ssrc);
		return;
	}

	process_report_blocks(session, packet, ssrc, packet->r.rr.rr, event_ts);

	if (((packet->common.count * 6) + 1) < ntohs(packet->common.length)) {
		debug_msg("Profile specific RR extension ignored\n");
	}
}

static void process_rtcp_sdes(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	int 			count = packet->common.count;
	struct rtcp_sdes_t 	*sd   = &packet->r.sdes;
	rtcp_sdes_item 		*rsp; 
	rtcp_sdes_item		*rspn;
	rtcp_sdes_item 		*end  = (rtcp_sdes_item *) ((uint32_t *)packet + packet->common.length + 1);
	source 			*s;
	rtp_event		 event;

	while (--count >= 0) {
		rsp = &sd->item[0];
		if (rsp >= end) {
			break;
		}
		sd->ssrc = ntohl(sd->ssrc);
		s = create_source(session, sd->ssrc);
		if (s == NULL) {
		  // wmay - be consistent
			debug_msg("Can't get valid source entry for %08u, skipping...\n", sd->ssrc);
		} else {
			for (; rsp->type; rsp = rspn ) {
				rspn = (rtcp_sdes_item *)((char*)rsp+rsp->length+2);
				if (rspn >= end) {
					rsp = rspn;
					break;
				}
				if (rtp_set_sdes(session, sd->ssrc, rsp->type, rsp->data, rsp->length)) {
					if (!filter_event(session, sd->ssrc)) {
						event.ssrc = sd->ssrc;
						event.type = RX_SDES;
						event.data = (void *) rsp;
						event.ts   = event_ts;
						session->callback(session, &event);
					}
				} else {
				  // wmay - be consistent 
					debug_msg("Invalid sdes item for source %08u, skipping...\n", sd->ssrc);
				}
			}
		}
		sd = (struct rtcp_sdes_t *) ((uint32_t *)sd + (((char *)rsp - (char *)sd) >> 2)+1);
	}
	if (count >= 0) {
		debug_msg("Invalid RTCP SDES packet, some items ignored.\n");
	}
}

static void process_rtcp_bye(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	int		 i;
	uint32_t		 ssrc;
	rtp_event	 event;
	source		*s;

	for (i = 0; i < packet->common.count; i++) {
		ssrc = ntohl(packet->r.bye.ssrc[i]);
		/* This is kind-of strange, since we create a source we are about to delete. */
		/* This is done to ensure that the source mentioned in the event which is    */
		/* passed to the user of the RTP library is valid, and simplify client code. */
		create_source(session, ssrc);
		/* Call the event handler... */
		if (!filter_event(session, ssrc)) {
			event.ssrc = ssrc;
			event.type = RX_BYE;
			event.data = NULL;
			event.ts   = event_ts;
			session->callback(session, &event);
		}
		/* Mark the source as ready for deletion. Sources are not deleted immediately */
		/* since some packets may be delayed and arrive after the BYE...              */
		s = get_source(session, ssrc);
		s->got_bye = TRUE;
		check_source(s);
	}
}

static void process_rtcp_app(struct rtp *session, rtcp_t *packet, struct timeval *event_ts)
{
	uint32_t          ssrc;
	rtp_event        event;
	rtcp_app        *app;
	source          *s;
	int              data_len;


	/* Update the database for this source. */
	ssrc = ntohl(packet->r.app.ssrc);
	create_source(session, ssrc);
	s = get_source(session, ssrc);
	if (s == NULL) {
	        /* This should only occur in the event of database malfunction. */
	  // wmay - be consistent
	        debug_msg("Source %08u invalid, skipping...\n", ssrc);
	        return;
	}
	check_source(s);

	/* Copy the entire packet, converting the header (only) into host byte order. */
	app = (rtcp_app *) xmalloc(RTP_MAX_PACKET_LEN);
	app->version        = packet->common.version;
	app->p              = packet->common.p;
	app->subtype        = packet->common.count;
	app->pt             = packet->common.pt;
	app->length         = ntohs(packet->common.length);
	app->ssrc           = ssrc;
	app->name[0]        = packet->r.app.name[0];
	app->name[1]        = packet->r.app.name[1];
	app->name[2]        = packet->r.app.name[2];
	app->name[3]        = packet->r.app.name[3];
	data_len            = (app->length - 2) * 4;
	memcpy(app->data, packet->r.app.data, data_len);

	/* Callback to the application to process the app packet... */
	if (!filter_event(session, ssrc)) {
		event.ssrc = ssrc;
		event.type = RX_APP;
		event.data = (void *) app;       /* The callback function MUST free this! */
		event.ts   = event_ts;
		session->callback(session, &event);
	}
}

static void rtp_process_ctrl(struct rtp *session, uint8_t *buffer, int buflen)
{
	/* This routine processes incoming RTCP packets */
	rtp_event	 event;
	struct timeval	 event_ts;
	rtcp_t		*packet;
	uint8_t 	 initVec[8] = {0,0,0,0,0,0,0,0};
	int		 first;
	uint32_t	 packet_ssrc = rtp_my_ssrc(session);

	gettimeofday(&event_ts, NULL);
	if (buflen > 0) {
		if (session->encryption_key != NULL) {
			/* Decrypt the packet... */
			qfDES_CBC_d(session->encryption_key, buffer, buflen, initVec);
			buffer += 4;	/* Skip the random prefix... */
			buflen -= 4;
		}
		if (validate_rtcp(buffer, buflen)) {
			first  = TRUE;
			packet = (rtcp_t *) buffer;
			while (packet < (rtcp_t *) (buffer + buflen)) {
				switch (packet->common.pt) {
					case RTCP_SR:
						if (first && !filter_event(session, ntohl(packet->r.sr.sr.ssrc))) {
							event.ssrc  = ntohl(packet->r.sr.sr.ssrc);
							event.type  = RX_RTCP_START;
							event.data  = &buflen;
							event.ts    = &event_ts;
							packet_ssrc = event.ssrc;
							session->callback(session, &event);
						}
						process_rtcp_sr(session, packet, &event_ts);
						break;
					case RTCP_RR:
						if (first && !filter_event(session, ntohl(packet->r.rr.ssrc))) {
							event.ssrc  = ntohl(packet->r.rr.ssrc);
							event.type  = RX_RTCP_START;
							event.data  = &buflen;
							event.ts    = &event_ts;
							packet_ssrc = event.ssrc;
							session->callback(session, &event);
						}
						process_rtcp_rr(session, packet, &event_ts);
						break;
					case RTCP_SDES:
						if (first && !filter_event(session, ntohl(packet->r.sdes.ssrc))) {
							event.ssrc  = ntohl(packet->r.sdes.ssrc);
							event.type  = RX_RTCP_START;
							event.data  = &buflen;
							event.ts    = &event_ts;
							packet_ssrc = event.ssrc;
							session->callback(session, &event);
						}
						process_rtcp_sdes(session, packet, &event_ts);
						break;
					case RTCP_BYE:
						if (first && !filter_event(session, ntohl(packet->r.bye.ssrc[0]))) {
							event.ssrc  = ntohl(packet->r.bye.ssrc[0]);
							event.type  = RX_RTCP_START;
							event.data  = &buflen;
							event.ts    = &event_ts;
							packet_ssrc = event.ssrc;
							session->callback(session, &event);
						}
						process_rtcp_bye(session, packet, &event_ts);
						break;
				        case RTCP_APP:
						if (first && !filter_event(session, ntohl(packet->r.app.ssrc))) {
							event.ssrc  = ntohl(packet->r.app.ssrc);
							event.type  = RX_RTCP_START;
							event.data  = &buflen;
							event.ts    = &event_ts;
							packet_ssrc = event.ssrc;
							session->callback(session, &event);
						}
					        process_rtcp_app(session, packet, &event_ts);
						break;
					default: 
						debug_msg("RTCP packet with unknown type (%d) ignored.\n", packet->common.pt);
						break;
				}
				packet = (rtcp_t *) ((char *) packet + (4 * (ntohs(packet->common.length) + 1)));
				first  = FALSE;
			}
			/* The constants here are 1/16 and 15/16 (section 6.3.3 of draft-ietf-avt-rtp-new-02.txt) */
			session->avg_rtcp_size = (0.0625 * buflen) + (0.9375 * session->avg_rtcp_size);
			/* Signal that we've finished processing this packet */
			if (!filter_event(session, packet_ssrc)) {
				event.ssrc = packet_ssrc;
				event.type = RX_RTCP_FINISH;
				event.data = NULL;
				event.ts   = &event_ts;
				session->callback(session, &event);
			}
		} else {
			debug_msg("Invalid RTCP packet discarded\n");
			session->invalid_rtcp_count++;
		}
	}
}

int rtp_recv(struct rtp *session, struct timeval *timeout, uint32_t curr_rtp_ts)
{
	check_database(session);
	udp_fd_zero();
	udp_fd_set(session->rtp_socket);
	udp_fd_set(session->rtcp_socket);
	if (udp_select(timeout) > 0) {
		if (udp_fd_isset(session->rtp_socket)) {
			rtp_recv_data(session, curr_rtp_ts);
		}
		if (udp_fd_isset(session->rtcp_socket)) {
                        uint8_t		 buffer[RTP_MAX_PACKET_LEN];
                        int		 buflen;
                        buflen = udp_recv(session->rtcp_socket, buffer, RTP_MAX_PACKET_LEN);
			rtp_process_ctrl(session, buffer, buflen);
		}
		check_database(session);
                return TRUE;
	}
	check_database(session);
        return FALSE;
}

int rtp_add_csrc(struct rtp *session, uint32_t csrc)
{
	check_database(session);
	UNUSED(csrc);
	return FALSE;
}

int rtp_set_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type, char *value, int length)
{
	source	*s;
	char	*v;

	check_database(session);

	s = get_source(session, ssrc);
	if (s == NULL) {
	  // wmay - be consistent 
		debug_msg("Invalid source %08u\n", ssrc);
		return FALSE;
	}
	check_source(s);

	v = (char *) xmalloc(length + 1);
	memset(v, '\0', length + 1);
	memcpy(v, value, length);

	switch (type) {
		case RTCP_SDES_CNAME: 
			if (s->cname) xfree(s->cname);
			s->cname = v; 
			break;
		case RTCP_SDES_NAME:
			if (s->name) xfree(s->name);
			s->name = v; 
			break;
		case RTCP_SDES_EMAIL:
			if (s->email) xfree(s->email);
			s->email = v; 
			break;
		case RTCP_SDES_PHONE:
			if (s->phone) xfree(s->phone);
			s->phone = v; 
			break;
		case RTCP_SDES_LOC:
			if (s->loc) xfree(s->loc);
			s->loc = v; 
			break;
		case RTCP_SDES_TOOL:
			if (s->tool) xfree(s->tool);
			s->tool = v; 
			break;
		case RTCP_SDES_NOTE:
			if (s->note) xfree(s->note);
			s->note = v; 
			break;
		default :
			debug_msg("Unknown SDES item (type=%d, value=%s)\n", type, v);
                        xfree(v);
			check_database(session);
			return FALSE;
	}
	check_database(session);
	return TRUE;
}

const char *rtp_get_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type)
{
	source	*s;

	check_database(session);

	s = get_source(session, ssrc);
	if (s == NULL) {
	  // wmay - be consistent
		debug_msg("Invalid source %08u\n", ssrc);
		return NULL;
	}
	check_source(s);

	switch (type) {
        case RTCP_SDES_CNAME: 
                return s->cname;
        case RTCP_SDES_NAME:
                return s->name;
        case RTCP_SDES_EMAIL:
                return s->email;
        case RTCP_SDES_PHONE:
                return s->phone;
        case RTCP_SDES_LOC:
                return s->loc;
        case RTCP_SDES_TOOL:
                return s->tool;
        case RTCP_SDES_NOTE:
                return s->note;
        default:
                /* This includes RTCP_SDES_PRIV and RTCP_SDES_END */
                debug_msg("Unknown SDES item (type=%d)\n", type);
	}
	return NULL;
}

const rtcp_sr *rtp_get_sr(struct rtp *session, uint32_t ssrc)
{
	/* Return the last SR received from this ssrc. The */
	/* caller MUST NOT free the memory returned to it. */
	source	*s;

	check_database(session);

	s = get_source(session, ssrc);
	if (s == NULL) {
		return NULL;
	} 
	check_source(s);
	return s->sr;
}

const rtcp_rr *rtp_get_rr(struct rtp *session, uint32_t reporter, uint32_t reportee)
{
	check_database(session);
        return get_rr(session, reporter, reportee);
}

int rtp_send_data(struct rtp *session, uint32_t rtp_ts, char pt, int m, int cc, uint32_t csrc[], 
                  char *data, int data_len, char *extn, uint16_t extn_len, uint16_t extn_type)
{
	int		 buffer_len, i, rc, pad, pad_len;
	uint8_t		*buffer;
	rtp_packet	*packet;
	uint8_t 	 initVec[8] = {0,0,0,0,0,0,0,0};

	check_database(session);

	assert(data_len > 0);

	buffer_len = data_len + 12 + (4 * cc);
	if (extn != NULL) {
		buffer_len += (extn_len + 1) * 4;
	}

	/* Do we need to pad this packet to a multiple of 64 bits? */
	/* This is only needed if encryption is enabled, since DES */
	/* only works on multiples of 64 bits. We just calculate   */
	/* the amount of padding to add here, so we can reserve    */
	/* space - the actual padding is added later.              */
	if ((session->encryption_key != NULL) && ((buffer_len % 8) != 0)) {
		pad         = TRUE;
		pad_len     = 8 - (buffer_len % 8);
		buffer_len += pad_len; 
		assert((buffer_len % 8) == 0);
	} else {
		pad     = FALSE;
		pad_len = 0;
	}

	/* Allocate memory for the packet... */
	buffer     = (uint8_t *) xmalloc(buffer_len + RTP_PACKET_HEADER_SIZE);
	packet     = (rtp_packet *) buffer;

	/* These are internal pointers into the buffer... */
	packet->csrc = (uint32_t *) (buffer + RTP_PACKET_HEADER_SIZE + 12);
	packet->extn = (uint8_t  *) (buffer + RTP_PACKET_HEADER_SIZE + 12 + (4 * cc));
	packet->data = (uint8_t  *) (buffer + RTP_PACKET_HEADER_SIZE + 12 + (4 * cc));
	if (extn != NULL) {
		packet->data += (extn_len + 1) * 4;
	}
	/* ...and the actual packet header... */
	packet->v    = 2;
	packet->p    = pad;
	packet->x    = (extn != NULL);
	packet->cc   = cc;
	packet->m    = m;
	packet->pt   = pt;
	packet->seq  = htons(session->rtp_seq++);
	packet->ts   = htonl(rtp_ts);
	packet->ssrc = htonl(rtp_my_ssrc(session));
	/* ...now the CSRC list... */
	for (i = 0; i < cc; i++) {
		packet->csrc[i] = htonl(csrc[i]);
	}
	/* ...a header extension? */
	if (extn != NULL) {
		/* We don't use the packet->extn_type field here, that's for receive only... */
		uint16_t *base = (uint16_t *) packet->extn;
		base[0] = htons(extn_type);
		base[1] = htons(extn_len);
		memcpy(packet->extn + 4, extn, extn_len * 4);
	}
	/* ...and the media data... */
	memcpy(packet->data, data, data_len);
	/* ...and any padding... */
	if (pad) {
		for (i = 0; i < pad_len; i++) {
			buffer[buffer_len + RTP_PACKET_HEADER_SIZE - pad_len + i] = 0;
		}
		buffer[buffer_len + RTP_PACKET_HEADER_SIZE - 1] = (char) pad_len;
	}
	
	/* Finally, encrypt if desired... */
	if (session->encryption_key != NULL) {
		assert((buffer_len % 8) == 0);
		qfDES_CBC_e(session->encryption_key, buffer + RTP_PACKET_HEADER_SIZE, buffer_len, initVec); 
	}

	rc = udp_send(session->rtp_socket, buffer + RTP_PACKET_HEADER_SIZE, buffer_len);
	xfree(buffer);

	/* Update the RTCP statistics... */
	session->we_sent     = TRUE;
	session->rtp_pcount += 1;
	session->rtp_bcount += buffer_len;

	check_database(session);
	return rc;
}

static int format_report_blocks(rtcp_rr *rrp, int remaining_length, struct rtp *session)
{
	int nblocks = 0;
	int h;
	source *s;
	struct timeval now;

	gettimeofday(&now, NULL);

	for (h = 0; h < RTP_DB_SIZE; h++) {
		for (s = session->db[h]; s != NULL; s = s->next) {
			check_source(s);
			if ((nblocks == 31) || (remaining_length < 24)) {
				break; /* Insufficient space for more report blocks... */
			}
			if (s->sender) {
				/* Much of this is taken from A.3 of draft-ietf-avt-rtp-new-01.txt */
				int	extended_max      = s->cycles + s->max_seq;
       				int	expected          = extended_max - s->base_seq + 1;
       				int	lost              = expected - s->received;
				int	expected_interval = expected - s->expected_prior;
       				int	received_interval = s->received - s->received_prior;
       				int 	lost_interval     = expected_interval - received_interval;
				int	fraction;
				uint32_t lsr;
				uint32_t dlsr;

       				s->expected_prior = expected;
       				s->received_prior = s->received;
       				if (expected_interval == 0 || lost_interval <= 0) {
					fraction = 0;
       				} else {
					fraction = (lost_interval << 8) / expected_interval;
				}

				if (s->sr == NULL) {
					lsr = 0;
					dlsr = 0;
				} else {
					lsr = ntp64_to_ntp32(s->sr->ntp_sec, s->sr->ntp_frac);
					dlsr = (uint32_t)(tv_diff(now, s->last_sr) * 65536);
				}
				rrp->ssrc       = htonl(s->ssrc);
				rrp->fract_lost = fraction;
				rrp->total_lost = lost & 0x00ffffff;
				rrp->last_seq   = htonl(extended_max);
				rrp->jitter     = htonl(s->jitter / 16);
				rrp->lsr        = htonl(lsr);
				rrp->dlsr       = htonl(dlsr);
				rrp++;
				s->sender = FALSE;
				remaining_length -= 24;
				nblocks++;
				session->sender_count--;
				if (session->sender_count == 0) {
					break; /* No point continuing, since we've reported on all senders... */
				}
			}
		}
	}
	return nblocks;
}

static uint8_t *format_rtcp_sr(uint8_t *buffer, int buflen, struct rtp *session, uint32_t rtp_ts)
{
	/* Write an RTCP SR into buffer, returning a pointer to */
	/* the next byte after the header we have just written. */
	rtcp_t		*packet = (rtcp_t *) buffer;
	int		 remaining_length;
	uint32_t	 ntp_sec, ntp_frac;

	assert(buflen >= 28);	/* ...else there isn't space for the header and sender report */

	packet->common.version = 2;
	packet->common.p       = 0;
	packet->common.count   = 0;
	packet->common.pt      = RTCP_SR;
	packet->common.length  = htons(1);

        ntp64_time(&ntp_sec, &ntp_frac);

	packet->r.sr.sr.ssrc          = htonl(rtp_my_ssrc(session));
	packet->r.sr.sr.ntp_sec       = htonl(ntp_sec);
	packet->r.sr.sr.ntp_frac      = htonl(ntp_frac);
	packet->r.sr.sr.rtp_ts        = htonl(rtp_ts);
	packet->r.sr.sr.sender_pcount = htonl(session->rtp_pcount);
	packet->r.sr.sr.sender_bcount = htonl(session->rtp_bcount);

	/* Add report blocks, until we either run out of senders */
	/* to report upon or we run out of space in the buffer.  */
	remaining_length = buflen - 28;
	packet->common.count = format_report_blocks(packet->r.sr.rr, remaining_length, session);
	packet->common.length = htons((uint16_t) (6 + (packet->common.count * 6)));
	return buffer + 28 + (24 * packet->common.count);
}

static uint8_t *format_rtcp_rr(uint8_t *buffer, int buflen, struct rtp *session)
{
	/* Write an RTCP RR into buffer, returning a pointer to */
	/* the next byte after the header we have just written. */
	rtcp_t		*packet = (rtcp_t *) buffer;
	int		 remaining_length;

	assert(buflen >= 8);	/* ...else there isn't space for the header */

	packet->common.version = 2;
	packet->common.p       = 0;
	packet->common.count   = 0;
	packet->common.pt      = RTCP_RR;
	packet->common.length  = htons(1);
	packet->r.rr.ssrc      = htonl(session->my_ssrc);

	/* Add report blocks, until we either run out of senders */
	/* to report upon or we run out of space in the buffer.  */
	remaining_length = buflen - 8;
	packet->common.count = format_report_blocks(packet->r.rr.rr, remaining_length, session);
	packet->common.length = htons((uint16_t) (1 + (packet->common.count * 6)));
	return buffer + 8 + (24 * packet->common.count);
}

static int add_sdes_item(uint8_t *buf, int buflen, int type, const char *val)
{
	/* Fill out an SDES item. It is assumed that the item is a NULL    */
	/* terminated string.                                              */
        rtcp_sdes_item *shdr = (rtcp_sdes_item *) buf;
        int             namelen;

        if (val == NULL) {
                debug_msg("Cannot format SDES item. type=%d val=%xp\n", type, val);
                return 0;
        }
        shdr->type = type;
        namelen = strlen(val);
        shdr->length = namelen;
        strncpy(shdr->data, val, buflen - 2); /* The "-2" accounts for the other shdr fields */
        return namelen + 2;
}

static uint8_t *format_rtcp_sdes(uint8_t *buffer, int buflen, uint32_t ssrc, struct rtp *session)
{
        /* From draft-ietf-avt-profile-new-00:                             */
        /* "Applications may use any of the SDES items described in the    */
        /* RTP specification. While CNAME information is sent every        */
        /* reporting interval, other items should be sent only every third */
        /* reporting interval, with NAME sent seven out of eight times     */
        /* within that slot and the remaining SDES items cyclically taking */
        /* up the eighth slot, as defined in Section 6.2.2 of the RTP      */
        /* specification. In other words, NAME is sent in RTCP packets 1,  */
        /* 4, 7, 10, 13, 16, 19, while, say, EMAIL is used in RTCP packet  */
        /* 22".                                                            */
	uint8_t		*packet = buffer;
	rtcp_common	*common = (rtcp_common *) buffer;
	const char	*item;
	size_t		 remaining_len;
        int              pad;

	assert(buflen > (int) sizeof(rtcp_common));

	common->version = 2;
	common->p       = 0;
	common->count   = 1;
	common->pt      = RTCP_SDES;
	common->length  = 0;
	packet += sizeof(common);

	*((uint32_t *) packet) = htonl(ssrc);
	packet += 4;

	remaining_len = buflen - (packet - buffer);
	item = rtp_get_sdes(session, ssrc, RTCP_SDES_CNAME);
	if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
		packet += add_sdes_item(packet, remaining_len, RTCP_SDES_CNAME, item);
	}

	remaining_len = buflen - (packet - buffer);
	item = rtp_get_sdes(session, ssrc, RTCP_SDES_NOTE);
	if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
		packet += add_sdes_item(packet, remaining_len, RTCP_SDES_NOTE, item);
	}

	remaining_len = buflen - (packet - buffer);
	if ((session->sdes_count_pri % 3) == 0) {
		session->sdes_count_sec++;
		if ((session->sdes_count_sec % 8) == 0) {
			/* Note that the following is supposed to fall-through the cases */
			/* until one is found to send... The lack of break statements in */
			/* the switch is not a bug.                                      */
			switch (session->sdes_count_ter % 4) {
			case 0: item = rtp_get_sdes(session, ssrc, RTCP_SDES_TOOL);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_TOOL, item);
					break;
				}
			case 1: item = rtp_get_sdes(session, ssrc, RTCP_SDES_EMAIL);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_EMAIL, item);
					break;
				}
			case 2: item = rtp_get_sdes(session, ssrc, RTCP_SDES_PHONE);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_PHONE, item);
					break;
				}
			case 3: item = rtp_get_sdes(session, ssrc, RTCP_SDES_LOC);
				if ((item != NULL) && ((strlen(item) + (size_t) 2) <= remaining_len)) {
					packet += add_sdes_item(packet, remaining_len, RTCP_SDES_LOC, item);
					break;
				}
			}
			session->sdes_count_ter++;
		} else {
			item = rtp_get_sdes(session, ssrc, RTCP_SDES_NAME);
			if (item != NULL) {
				packet += add_sdes_item(packet, remaining_len, RTCP_SDES_NAME, item);
			}
		}
	}
	session->sdes_count_pri++;

	/* Pad to a multiple of 4 bytes... */
        pad = 4 - ((packet - buffer) & 0x3);
        while (pad--) {
               *packet++ = RTCP_SDES_END;
        }

	common->length = htons((uint16_t) (((int) (packet - buffer) / 4) - 1));

	return packet;
}

static uint8_t *format_rtcp_app(uint8_t *buffer, int buflen, uint32_t ssrc, rtcp_app *app)
{
	/* Write an RTCP APP into the outgoing packet buffer. */
	rtcp_app    *packet       = (rtcp_app *) buffer;
	int          pkt_octets   = (app->length + 1) * 4;
	int          data_octets  =  pkt_octets - 12;

	assert(data_octets >= 0);          /* ...else not a legal APP packet.               */
	assert(buflen      >  pkt_octets); /* ...else there isn't space for the APP packet. */

	/* Copy one APP packet from "app" to "packet". */
	packet->version        =   RTP_VERSION;
	packet->p              =   app->p;
	packet->subtype        =   app->subtype;
	packet->pt             =   RTCP_APP;
	packet->length         =   htons(app->length);
	packet->ssrc           =   htonl(ssrc);
	memcpy(packet->name, app->name, 4);
	memcpy(packet->data, app->data, data_octets);

	/* Return a pointer to the byte that immediately follows the last byte written. */
	return buffer + pkt_octets;
}

static void send_rtcp(struct rtp *session, uint32_t rtp_ts,
		     rtcp_app *(*appcallback)(struct rtp *session, uint32_t rtp_ts, int max_size))
{
	/* Construct and send an RTCP packet. The order in which packets are packed into a */
	/* compound packet is defined by section 6.1 of draft-ietf-avt-rtp-new-03.txt and  */
	/* we follow the recommended order.                                                */
	uint8_t	   buffer[RTP_MAX_PACKET_LEN + 8];	/* The +8 is to allow for padding when encrypting */
	uint8_t	  *ptr = buffer;
	uint8_t   *old_ptr;
	uint8_t   *lpt;		/* the last packet in the compound */
	rtcp_app  *app;
	uint8_t    initVec[8] = {0,0,0,0,0,0,0,0};

	check_database(session);
	/* If encryption is enabled, add a 32 bit random prefix to the packet */
	if (session->encryption_key != NULL) {
		*((uint32_t *) ptr) = lbl_random();
		ptr += 4;
	}

	/* The first RTCP packet in the compound packet MUST always be a report packet...  */
	if (session->we_sent) {
		ptr = format_rtcp_sr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session, rtp_ts);
	} else {
		ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);
	}
	/* Add the appropriate SDES items to the packet... This should really be after the */
	/* insertion of the additional report blocks, but if we do that there are problems */
	/* with us being unable to fit the SDES packet in when we run out of buffer space  */
	/* adding RRs. The correct fix would be to calculate the length of the SDES items  */
	/* in advance and subtract this from the buffer length but this is non-trivial and */
	/* probably not worth it.                                                          */
	lpt = ptr;
	ptr = format_rtcp_sdes(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), rtp_my_ssrc(session), session);

	/* Following that, additional RR packets SHOULD follow if there are more than 31   */
	/* senders, such that the reports do not fit into the initial packet. We give up   */
	/* if there is insufficient space in the buffer: this is bad, since we always drop */
	/* the reports from the same sources (those at the end of the hash table).         */
	while ((session->sender_count > 0)  && ((RTP_MAX_PACKET_LEN - (ptr - buffer)) >= 8)) {
		lpt = ptr;
		ptr = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);
	}

	/* Finish with as many APP packets as the application will provide. */
	old_ptr = ptr;
	if (appcallback) {
		while ((app = (*appcallback)(session, rtp_ts, RTP_MAX_PACKET_LEN - (ptr - buffer)))) {
			lpt = ptr;
			ptr = format_rtcp_app(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), rtp_my_ssrc(session), app);
			assert(ptr > old_ptr);
			old_ptr = ptr;
			assert(RTP_MAX_PACKET_LEN - (ptr - buffer) >= 0);
		}
	}

	/* And encrypt if desired... */
	if (session->encryption_key != NULL) {
		if (((ptr - buffer) % 8) != 0) {
			/* Add padding to the last packet in the compound, if necessary. */
			/* We don't have to worry about overflowing the buffer, since we */
			/* intentionally allocated it 8 bytes longer to allow for this.  */
			int	padlen = 8 - ((ptr - buffer) % 8);
			int	i;

			for (i = 0; i < padlen; i++) {
				*(ptr++) = '\0';
			}
			*ptr = (uint8_t) padlen;
			assert(((ptr - buffer) % 8) == 0); 

			((rtcp_t *) lpt)->common.p = TRUE;
			((rtcp_t *) lpt)->common.length = htons((int16_t)(((ptr - lpt) / 4) - 1));
		}
		qfDES_CBC_e(session->encryption_key, buffer, ptr - buffer, initVec); 
	}
	udp_send(session->rtcp_socket, buffer, ptr - buffer);

        /* Loop the data back to ourselves so local participant can */
        /* query own stats when using unicast or multicast with no  */
        /* loopback.                                                */
        rtp_process_ctrl(session, buffer, ptr - buffer);
	check_database(session);
}

void rtp_send_ctrl(struct rtp *session, uint32_t rtp_ts,
                   rtcp_app *(*appcallback)(struct rtp *session, uint32_t rtp_ts, int max_size))
{
	/* Send an RTCP packet, if one is due... */
	struct timeval	 curr_time;

	check_database(session);
	gettimeofday(&curr_time, NULL);
	if (tv_gt(curr_time, session->next_rtcp_send_time)) {
		/* The RTCP transmission timer has expired. The following */
		/* implements draft-ietf-avt-rtp-new-02.txt section 6.3.6 */
		int		 h;
		source		*s;
		struct timeval	 new_send_time;
		double		 new_interval;

		new_interval  = rtcp_interval(session);
		new_send_time = session->last_rtcp_send_time;
		tv_add(&new_send_time, new_interval);
		if (tv_gt(curr_time, new_send_time)) {
			send_rtcp(session, rtp_ts, appcallback);
			session->initial_rtcp        = FALSE;
			session->we_sent             = FALSE;
			session->last_rtcp_send_time = curr_time;
			session->next_rtcp_send_time = curr_time; 
			tv_add(&(session->next_rtcp_send_time), new_interval);
			/* We're starting a new RTCP reporting interval, zero out */
			/* the per-interval statistics.                           */
			session->sender_count = 0;
			for (h = 0; h < RTP_DB_SIZE; h++) {
				for (s = session->db[h]; s != NULL; s = s->next) {
					check_source(s);
					s->sender = FALSE;
				}
			}
		} else {
			session->next_rtcp_send_time = session->last_rtcp_send_time; 
			tv_add(&(session->next_rtcp_send_time), new_interval);
		}
		session->ssrc_count_prev = session->ssrc_count;
	} 
	check_database(session);
}

void rtp_update(struct rtp *session)
{
	/* Perform housekeeping on the source database... */
	int	 	 h;
	source	 	*s, *n;
	struct timeval	 curr_time;
	double		 delay;

	check_database(session);
	gettimeofday(&curr_time, NULL);
	if (tv_diff(curr_time, session->last_update) < session->rtcp_interval) {
		/* We only cleanup once per RTCP reporting interval... */
		check_database(session);
		return;
	}
	session->last_update = curr_time;

	for (h = 0; h < RTP_DB_SIZE; h++) {
		for (s = session->db[h]; s != NULL; s = n) {
			check_source(s);
			n = s->next;
			/* Expire sources which haven't been heard from for a long time.   */
			/* Section 6.2.1 of the RTP specification details the timers used. */

			/* How long since we last heard from this source?  */
			delay = tv_diff(curr_time, s->last_active);
			
			/* Check if we've received a BYE packet from this source.    */
			/* If we have, and it was received more than 2 seconds ago   */
			/* then the source is deleted. The arbitrary 2 second delay  */
			/* is to ensure that all delayed packets are received before */
			/* the source is timed out.                                  */
			if (s->got_bye && (delay > 2.0)) {
				delete_source(session, s->ssrc);
			}
			/* If a source hasn't been heard from for more than 5 RTCP   */
			/* reporting intervals, we delete it from our database...    */
			if (delay > (session->rtcp_interval * 5)) {
				delete_source(session, s->ssrc);
			}
		}
	}

	/* Timeout those reception reports which haven't been refreshed for a long time */
	timeout_rr(session, &curr_time);
	check_database(session);
}

void rtp_send_bye(struct rtp *session)
{
	/* The function sends an RTCP BYE packet. It should implement BYE reconsideration, */
	/* at present it either: a) sends a BYE packet immediately (if there are less than */
	/* 50 members in the group), or b) returns without sending a BYE (if there are 50  */
	/* or more members). See draft-ietf-avt-rtp-new-01.txt (section 6.3.7).            */
	uint8_t	 	 buffer[RTP_MAX_PACKET_LEN + 8];	/* + 8 to allow for padding when encrypting */
	uint8_t		*ptr = buffer;
	rtcp_common	*common;
	uint8_t    	 initVec[8] = {0,0,0,0,0,0,0,0};

	check_database(session);
	/* If encryption is enabled, add a 32 bit random prefix to the packet */
	if (session->encryption_key != NULL) {
		*((uint32_t *) ptr) = lbl_random();
		ptr += 4;
	}

	if (session->ssrc_count < 50) {
		ptr    = format_rtcp_rr(ptr, RTP_MAX_PACKET_LEN - (ptr - buffer), session);    
		common = (rtcp_common *) ptr;
		
		common->version = 2;
		common->p       = 0;
		common->count   = 1;
		common->pt      = RTCP_BYE;
		common->length  = htons(1);
		ptr += sizeof(common);
		
		*((uint32_t *) ptr) = htonl(session->my_ssrc);  
		ptr += 4;
		
		if (session->encryption_key != NULL) {
			if (((ptr - buffer) % 8) != 0) {
				/* Add padding to the last packet in the compound, if necessary. */
				/* We don't have to worry about overflowing the buffer, since we */
				/* intentionally allocated it 8 bytes longer to allow for this.  */
				int	padlen = 8 - ((ptr - buffer) % 8);
				int	i;

				for (i = 0; i < padlen; i++) {
					*(ptr++) = '\0';
				}
				*ptr = (uint8_t) padlen;

				common->p      = TRUE;
				common->length = htons((int16_t)(((ptr - (uint8_t *) common) / 4) - 1));
			}
			assert(((ptr - buffer) % 8) == 0); 
			qfDES_CBC_e(session->encryption_key, buffer, ptr - buffer, initVec); 
		}
		udp_send(session->rtcp_socket, buffer, ptr - buffer);
		/* Loop the data back to ourselves so local participant can */
		/* query own stats when using unicast or multicast with no  */
		/* loopback.                                                */
		rtp_process_ctrl(session, buffer, ptr - buffer);
	}
	check_database(session);
}

void rtp_done(struct rtp *session)
{
	int i;
        source *s, *n;

	check_database(session);
        /* In delete_source, check database gets called and this assumes */
        /* first added and last removed is us.                           */
	for (i = 0; i < RTP_DB_SIZE; i++) {
                s = session->db[i];
                while (s != NULL) {
                        n = s->next;
                        if (s->ssrc != session->my_ssrc) {
                                delete_source(session,session->db[i]->ssrc);
                        }
                        s = n;
                }
	}

	delete_source(session, session->my_ssrc);

        if (session->encryption_key != NULL) {
                xfree(session->encryption_key);
        }

	udp_exit(session->rtp_socket);
	udp_exit(session->rtcp_socket);
	xfree(session->addr);
	xfree(session->opt);
	xfree(session);
}

int rtp_set_encryption_key(struct rtp* session, const char *passphrase)
{
	/* Convert the user supplied key into a form suitable for use with RTP */
	/* and install it as the active key. Passing in NULL as the passphrase */
	/* disables encryption. The passphrase is converted into a DES key as  */
	/* specified in RFC1890, that is:                                      */
	/*   - convert to canonical form                                       */
	/*   - derive an MD5 hash of the canonical form                        */
	/*   - take the first 56 bits of the MD5 hash                          */
	/*   - add parity bits to form a 64 bit key                            */
	/* Note that versions of rat prior to 4.1.2 do not convert the pass-   */
	/* phrase to canonical form before taking the MD5 hash, and so will    */
	/* not be compatible for keys which are non-invarient under this step. */
	char	*canonical_passphrase;
	u_char	 hash[16];
	MD5_CTX	 context;
	int	 i, j, k;

	check_database(session);
        if (session->encryption_key != NULL) {
                xfree(session->encryption_key);
        }

	if (passphrase == NULL) {
		/* A NULL passphrase means disable encryption... */
		session->encryption_key = NULL;
		check_database(session);
		return TRUE;
	}

	debug_msg("Enabling RTP/RTCP encryption\n");
        session->encryption_key = (char *) xmalloc(8);

	/* Step 1: convert to canonical form, comprising the following steps:  */
	/*   a) convert the input string to the ISO 10646 character set, using */
	/*      the UTF-8 encoding as specified in Annex P to ISO/IEC          */
	/*      10646-1:1993 (ASCII characters require no mapping, but ISO     */
	/*      8859-1 characters do);                                         */
	/*   b) remove leading and trailing white space characters;            */
	/*   c) replace one or more contiguous white space characters by a     */
	/*      single space (ASCII or UTF-8 0x20);                            */
	/*   d) convert all letters to lower case and replace sequences of     */
	/*      characters and non-spacing accents with a single character,    */
	/*      where possible.                                                */
	canonical_passphrase = (char *) xstrdup(passphrase);	/* FIXME */

	/* Step 2: derive an MD5 hash */
	MD5Init(&context);
	MD5Update(&context, (u_char *) canonical_passphrase, strlen(canonical_passphrase));
	MD5Final((u_char *) hash, &context);

	/* Step 3: take first 56 bits of the MD5 hash */
	session->encryption_key[0] = hash[0];
	session->encryption_key[1] = hash[0] << 7 | hash[1] >> 1;
	session->encryption_key[2] = hash[1] << 6 | hash[2] >> 2;
	session->encryption_key[3] = hash[2] << 5 | hash[3] >> 3;
	session->encryption_key[4] = hash[3] << 4 | hash[4] >> 4;
	session->encryption_key[5] = hash[4] << 3 | hash[5] >> 5;
	session->encryption_key[6] = hash[5] << 2 | hash[6] >> 6;
	session->encryption_key[7] = hash[6] << 1;

	/* Step 4: add parity bits */
	for (i = 0; i < 8; ++i) {
		k = session->encryption_key[i] & 0xfe;
		j = k;
		j ^= j >> 4;
		j ^= j >> 2;
		j ^= j >> 1;
		j = (j & 1) ^ 1;
		session->encryption_key[i] = k | j;
	}

	check_database(session);
	return TRUE;
}

char *rtp_get_addr(struct rtp *session)
{
	check_database(session);
	return session->addr;
}

uint16_t rtp_get_rx_port(struct rtp *session)
{
	check_database(session);
	return session->rx_port;
}

uint16_t rtp_get_tx_port(struct rtp *session)
{
	check_database(session);
	return session->tx_port;
}

int rtp_get_ttl(struct rtp *session)
{
	check_database(session);
	return session->ttl;
}

