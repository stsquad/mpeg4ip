/*
 * FILE:     mbus.c
 * AUTHOR:   Colin Perkins
 * MODIFIED: Orion Hodson
 *           Markus Germeier
 * 
 * Copyright (c) 1997-2000 University College London
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
 *      Department at University College London
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
 */

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "net_udp.h"
#include "hmac.h"
#include "qfDES.h"
#include "base64.h"
#include "gettimeofday.h"
#include "vsnprintf.h"
#include "mbus.h"
#include "mbus_config.h"
#include "mbus_parser.h"
#include "mbus_addr.h"

#define MBUS_BUF_SIZE	  1500
#define MBUS_ACK_BUF_SIZE 1500
#define MBUS_MAX_ADDR	    10
#define MBUS_MAX_QLEN	    50 /* Number of messages we can queue with mbus_qmsg() */

#define MBUS_MAGIC	0x87654321
#define MBUS_MSG_MAGIC	0x12345678

struct mbus_msg {
	struct mbus_msg	*next;
	struct timeval	 send_time;	/* Time the message was sent, to trigger a retransmit */
	struct timeval	 comp_time;	/* Time the message was composed, the timestamp in the packet header */
	char		*dest;
	int		 reliable;
	int		 complete;	/* Indicates that we've finished adding cmds to this message */
	int		 seqnum;
	int		 retransmit_count;
	int		 message_size;
	int		 num_cmds;
	char		*cmd_list[MBUS_MAX_QLEN];
	char		*arg_list[MBUS_MAX_QLEN];
	uint32_t	 idx_list[MBUS_MAX_QLEN];
	uint32_t	 magic;		/* For debugging... */
};

struct mbus {
	socket_udp	 	 *s;
	char		 	 *addr;				/* Addresses we respond to. 					*/
	int		 	  max_other_addr;
	int		 	  num_other_addr;
	char			**other_addr;			/* Addresses of other entities on the mbus. 			*/
        struct timeval          **other_hello;                  /* Time of last mbus.hello we received from other entities      */
	int		 	  seqnum;
	struct mbus_msg	 	 *cmd_queue;			/* Queue of messages waiting to be sent */
	struct mbus_msg	 	 *waiting_ack;			/* The last reliable message sent, if we have not yet got the ACK */
	char		 	 *hashkey;
	int		 	  hashkeylen;
	char		 	 *encrkey;
	int		 	  encrkeylen;
	struct timeval	 	  last_heartbeat;		/* Last time we sent a heartbeat message */
	struct mbus_config	 *cfg;
	void (*cmd_handler)(char *src, char *cmd, char *arg, void *dat);
	void (*err_handler)(int seqnum, int reason);
	uint32_t		  magic;			/* For debugging...                                             */
	uint32_t		  index;
	uint32_t		  index_sent;
};

static void mbus_validate(struct mbus *m)
{
#ifdef DEBUG
	int	i;

	ASSERT(m->num_other_addr <= m->max_other_addr);
	ASSERT(m->num_other_addr >= 0);
	for (i = 0; i < m->num_other_addr; i++) {
		ASSERT(m->other_addr[i]  != NULL);
		ASSERT(m->other_hello[i] != NULL);
	}
	for (i = m->num_other_addr + 1; i < m->max_other_addr; i++) {
		ASSERT(m->other_addr[i]  == NULL);
		ASSERT(m->other_hello[i] == NULL);
	}
#endif
	ASSERT(m->magic == MBUS_MAGIC);
	xmemchk();
}

static void mbus_msg_validate(struct mbus_msg *m)
{
#ifdef DEBUG
	int	i;

	ASSERT((m->num_cmds < MBUS_MAX_QLEN) && (m->num_cmds >= 0));
	for (i = 0; i < m->num_cmds; i++) {
		ASSERT(m->cmd_list[i] != NULL);
		ASSERT(m->arg_list[i] != NULL);
		if (i > 0) {
			ASSERT(m->idx_list[i] > m->idx_list[i-1]);
		}
	}
	for (i = m->num_cmds + 1; i < MBUS_MAX_QLEN; i++) {
		ASSERT(m->cmd_list[i] == NULL);
		ASSERT(m->arg_list[i] == NULL);
	}	
	ASSERT(m->dest != NULL);
#endif
	ASSERT(m->magic == MBUS_MSG_MAGIC);
}

static void store_other_addr(struct mbus *m, char *a)
{
	/* This takes the address a and ensures it is stored in the   */
	/* m->other_addr field of the mbus structure. The other_addr  */
	/* field should probably be a hash table, but for now we hope */
	/* that there are not too many entities on the mbus, so the   */
	/* list is small.                                             */
	int	i;

	mbus_validate(m);

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], a)) {
			/* Already in the list... */
			gettimeofday(m->other_hello[i],NULL);
			return;
		}
	}

	if (m->num_other_addr == m->max_other_addr) {
		/* Expand the list... */
		m->max_other_addr *= 2;
		m->other_addr = (char **) xrealloc(m->other_addr, m->max_other_addr * sizeof(char *));
		m->other_hello = (struct timeval **) xrealloc(m->other_hello, m->max_other_addr * sizeof(struct timeval *));
	}
	m->other_hello[m->num_other_addr]=(struct timeval *)xmalloc(sizeof(struct timeval));
	gettimeofday(m->other_hello[m->num_other_addr],NULL);
	m->other_addr[m->num_other_addr++] = xstrdup(a);
}

static void remove_other_addr(struct mbus *m, char *a)
{
	/* Removes the address a from the m->other_addr field of the */
	/* mbus structure.                                           */
	int	i, j;

	mbus_validate(m);

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], a)) {
			xfree(m->other_addr[i]);
			xfree(m->other_hello[i]);
			for (j = i+1; j < m->num_other_addr; j++) {
				m->other_addr[j-1] = m->other_addr[j];
				m->other_hello[j-1] = m->other_hello[j];
			}
			m->other_addr[m->num_other_addr  - 1] = NULL;
			m->other_hello[m->num_other_addr - 1] = NULL;
			m->num_other_addr--;
		}
	}
}

static void remove_inactiv_other_addr(struct mbus *m, struct timeval t, int interval){
	/* Remove addresses we haven't heard from for about 5 * interval */
	/* Count backwards so it is safe to remove entries               */
	int i;
    
	mbus_validate(m);

	for (i=m->num_other_addr-1; i>=0; i--){
		if ((t.tv_sec-(m->other_hello[i]->tv_sec)) > 5 * interval) {
			debug_msg("remove dead entity (%s)\n", m->other_addr[i]);
			remove_other_addr(m, m->other_addr[i]);
		}
	}
}

int mbus_addr_valid(struct mbus *m, char *addr)
{
	int	i;

	mbus_validate(m);

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], addr)) {
			return TRUE;
		}
	}
	return FALSE;
}

static int mbus_addr_unique(struct mbus *m, char *addr)
{
	int     i, n = 0;

	mbus_validate(m);

	for (i = 0; i < m->num_other_addr; i++) {
		if (mbus_addr_match(m->other_addr[i], addr)) {
			n++;
		}
	}
	return n==1;
}

/* The mb_* functions are used to build an mbus message up in the */
/* mb_buffer, and to add authentication and encryption before the */
/* message is sent.                                               */
char	 mb_cryptbuf[MBUS_BUF_SIZE];
char	*mb_buffer;
char	*mb_bufpos;

#define MBUS_AUTH_LEN 16

static void mb_header(int seqnum, int ts, char reliable, const char *src, const char *dst, int ackseq)
{
	xmemchk();
	mb_buffer   = (char *) xmalloc(MBUS_BUF_SIZE + 1);
	memset(mb_buffer,   0, MBUS_BUF_SIZE);
	memset(mb_buffer, ' ', MBUS_AUTH_LEN);
	mb_bufpos = mb_buffer + MBUS_AUTH_LEN;
	sprintf(mb_bufpos, "\nmbus/1.0 %6d %9d %c (%s) %s ", seqnum, ts, reliable, src, dst);
	mb_bufpos += 33 + strlen(src) + strlen(dst);
	if (ackseq == -1) {
		sprintf(mb_bufpos, "()\n");
		mb_bufpos += 3;
	} else {
		sprintf(mb_bufpos, "(%6d)\n", ackseq);
		mb_bufpos += 9;
	}
}

static void mb_add_command(const char *cmnd, const char *args)
{
	int offset = strlen(cmnd) + strlen(args) + 5;

	ASSERT((mb_bufpos + offset - mb_buffer) < MBUS_BUF_SIZE);

	sprintf(mb_bufpos, "%s (%s)\n", cmnd, args);
	mb_bufpos += offset - 1; /* The -1 in offset means we're not NUL terminated - fix in mb_send */
}

static void mb_send(struct mbus *m)
{
	char		digest[16];
	int		len;
	unsigned char	initVec[8] = {0,0,0,0,0,0,0,0};
 
	mbus_validate(m);

	*(mb_bufpos++) = '\0';
	ASSERT((mb_bufpos - mb_buffer) < MBUS_BUF_SIZE);
	ASSERT(strlen(mb_buffer) < MBUS_BUF_SIZE);

	/* Pad to a multiple of 8 bytes, so the encryption can work... */
	while (((mb_bufpos - mb_buffer) % 8) != 0) {
		*(mb_bufpos++) = '\0';
	}
	len = mb_bufpos - mb_buffer;
	ASSERT(len < MBUS_BUF_SIZE);
	ASSERT(strlen(mb_buffer) < MBUS_BUF_SIZE);

	xmemchk();
	if (m->hashkey != NULL) {
		/* Authenticate... */
		hmac_md5(mb_buffer + MBUS_AUTH_LEN+1, strlen(mb_buffer) - (MBUS_AUTH_LEN+1), m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 12, mb_buffer, MBUS_AUTH_LEN);
	}
	xmemchk();
	if (m->encrkey != NULL) {
		/* Encrypt... */
		memset(mb_cryptbuf, 0, MBUS_BUF_SIZE);
		memcpy(mb_cryptbuf, mb_buffer, len);
		ASSERT((len % 8) == 0);
		ASSERT(len < MBUS_BUF_SIZE);
		ASSERT(m->encrkeylen == 8);
		xmemchk();
		qfDES_CBC_e(m->encrkey, mb_cryptbuf, len, initVec);
		xmemchk();
		memcpy(mb_buffer, mb_cryptbuf, len);
	}
	xmemchk();
	udp_send(m->s, mb_buffer, len);
	xfree(mb_buffer);
}

static void resend(struct mbus *m, struct mbus_msg *curr) 
{
	/* Don't need to check for buffer overflows: this was done in mbus_send() when */
	/* this message was first transmitted. If it was okay then, it's okay now.     */
	int	 i;

	mbus_validate(m);

	mb_header(curr->seqnum, curr->comp_time.tv_sec, (char)(curr->reliable?'R':'U'), m->addr, curr->dest, -1);
	for (i = 0; i < curr->num_cmds; i++) {
		mb_add_command(curr->cmd_list[i], curr->arg_list[i]);
	}
	mb_send(m);
	curr->retransmit_count++;
}

void mbus_retransmit(struct mbus *m)
{
	struct mbus_msg	*curr = m->waiting_ack;
	struct timeval	time;
	long		diff;

	mbus_validate(m);

	if (!mbus_waiting_ack(m)) {
		return;
	}

	mbus_msg_validate(curr);

	gettimeofday(&time, NULL);

	/* diff is time in milliseconds that the message has been awaiting an ACK */
	diff = ((time.tv_sec * 1000) + (time.tv_usec / 1000)) - ((curr->send_time.tv_sec * 1000) + (curr->send_time.tv_usec / 1000));
	if (diff > 10000) {
		debug_msg("Reliable mbus message failed!\n");
		if (m->err_handler == NULL) {
			abort();
		}
		m->err_handler(curr->seqnum, MBUS_MESSAGE_LOST);
		/* if we don't delete this failed message, the error handler
                   gets triggered every time we call mbus_retransmit */
		while (m->waiting_ack->num_cmds > 0) {
		    m->waiting_ack->num_cmds--;
		    xfree(m->waiting_ack->cmd_list[m->waiting_ack->num_cmds]);
		    xfree(m->waiting_ack->arg_list[m->waiting_ack->num_cmds]);
		}
		xfree(m->waiting_ack->dest);
		xfree(m->waiting_ack);
		m->waiting_ack = NULL;
		return;
	} 
	/* Note: We only send one retransmission each time, to avoid
	 * overflowing the receiver with a burst of requests...
	 */
	if ((diff > 750) && (curr->retransmit_count == 2)) {
		resend(m, curr);
		return;
	} 
	if ((diff > 500) && (curr->retransmit_count == 1)) {
		resend(m, curr);
		return;
	} 
	if ((diff > 250) && (curr->retransmit_count == 0)) {
		resend(m, curr);
		return;
	}
}

void mbus_heartbeat(struct mbus *m, int interval)
{
	struct timeval	curr_time;
	char	*a = (char *) xmalloc(3);
	sprintf(a, "()");

	mbus_validate(m);

	gettimeofday(&curr_time, NULL);
	if (curr_time.tv_sec - m->last_heartbeat.tv_sec >= interval) {
		mb_header(++m->seqnum, (int) curr_time.tv_sec, 'U', m->addr, "()", -1);
		mb_add_command("mbus.hello", "");
		mb_send(m);

		m->last_heartbeat = curr_time;
		/* Remove dead sources */
		remove_inactiv_other_addr(m, curr_time, interval);
	}
	xfree(a);
}

int mbus_waiting_ack(struct mbus *m)
{
	mbus_validate(m);
	return m->waiting_ack != NULL;
}

int mbus_sent_all(struct mbus *m)
{
	mbus_validate(m);
	return (m->cmd_queue == NULL) && (m->waiting_ack == NULL);
}

struct mbus *mbus_init(void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat), 
		       void  (*err_handler)(int seqnum, int reason),
		       char  *addr)
{
	struct mbus		*m;
	struct mbus_key	 	 k;
	struct mbus_parser	*mp;
	int		 	 i;
	char            	*net_addr, *tmp;
	uint16_t         	 net_port;
	int              	 net_scope;

	m = (struct mbus *) xmalloc(sizeof(struct mbus));
	if (m == NULL) {
		debug_msg("Unable to allocate memory for mbus\n");
		return NULL;
	}

	m->cfg = mbus_create_config();
	mbus_lock_config_file(m->cfg);
	net_addr = (char *) xmalloc(20);
	mbus_get_net_addr(m->cfg, net_addr, &net_port, &net_scope);
	m->s		  = udp_init(net_addr, net_port, net_port, net_scope);
        if (m->s == NULL) {
                debug_msg("Unable to initialize mbus address\n");
                xfree(m);
                return NULL;
        }
	m->seqnum         = 0;
	m->cmd_handler    = cmd_handler;
	m->err_handler	  = err_handler;
	m->num_other_addr = 0;
	m->max_other_addr = 10;
	m->other_addr     = (char **) xmalloc(sizeof(char *) * 10);
	m->other_hello    = (struct timeval **) xmalloc(sizeof(struct timeval *) * 10);
	for (i = 0; i < 10; i++) {
		m->other_addr[i]  = NULL;
		m->other_hello[i] = NULL;
	}
	m->cmd_queue	  = NULL;
	m->waiting_ack	  = NULL;
	m->magic          = MBUS_MAGIC;
	m->index          = 0;
	m->index_sent     = 0;

	mp = mbus_parse_init(xstrdup(addr));
	if (!mbus_parse_lst(mp, &tmp)) {
		debug_msg("Invalid mbus address\n");
		abort();
	}
	m->addr = xstrdup(tmp);
	mbus_parse_done(mp);
	ASSERT(m->addr != NULL);

	gettimeofday(&(m->last_heartbeat), NULL);

	mbus_get_encrkey(m->cfg, &k);
	m->encrkey    = k.key;
	m->encrkeylen = k.key_len;

	mbus_get_hashkey(m->cfg, &k);
	m->hashkey    = k.key;
	m->hashkeylen = k.key_len;

	mbus_unlock_config_file(m->cfg);

	xfree(net_addr);

	return m;
}

void mbus_cmd_handler(struct mbus *m, void  (*cmd_handler)(char *src, char *cmd, char *arg, void *dat))
{
	mbus_validate(m);
	m->cmd_handler = cmd_handler;
}

static void mbus_flush_msgs(struct mbus_msg **queue)
{
        struct mbus_msg *curr, *next;
        int i;
	
        curr = *queue;
        while(curr) {
                next = curr->next;
                xfree(curr->dest);
                for(i = 0; i < curr->num_cmds; i++) {
                        xfree(curr->cmd_list[i]);
                        xfree(curr->arg_list[i]);
                }
		xfree(curr);
                curr = next;
        }
	*queue = NULL;
}

void mbus_exit(struct mbus *m) 
{
        int i;

        ASSERT(m != NULL);
	mbus_validate(m);

	mbus_qmsg(m, "()", "mbus.bye", "", FALSE);
	mbus_send(m);

	/* FIXME: It should be a fatal error to call mbus_exit() if some messages are still outstanding. */
	/*        We will need an mbus_flush() call first though, to ensure nothing is waiting.          */
        mbus_flush_msgs(&m->cmd_queue);
        mbus_flush_msgs(&m->waiting_ack);

        if (m->encrkey != NULL) {
                xfree(m->encrkey);
        }
        if (m->hashkey != NULL) {
        	xfree(m->hashkey);
	}

        udp_exit(m->s);

	/* Clean up other_* */
	for (i=m->num_other_addr-1; i>=0; i--){
	    remove_other_addr(m, m->other_addr[i]);
	}

        xfree(m->addr);
	xfree(m->other_addr);
	xfree(m->other_hello);
	xfree(m->cfg);
        xfree(m);
}

void mbus_send(struct mbus *m)
{
	/* Send one, or more, messages previosly queued with mbus_qmsg(). */
	/* Messages for the same destination are batched together. Stops  */
	/* when a reliable message is sent, until the ACK is received.    */
	struct mbus_msg	*curr = m->cmd_queue;
	int		 i;

	mbus_validate(m);
	if (m->waiting_ack != NULL) {
		return;
	}

	while (curr != NULL) {
		mbus_msg_validate(curr);
		/* It's okay for us to send messages which haven't been marked as complete - */
		/* that just means we're sending something which has the potential to have   */
		/* more data piggybacked. However, if it's not complete it MUST be the last  */
		/* in the list, or something has been reordered - which is bad.              */
		if (!curr->complete) {
			ASSERT(curr->next == NULL);
		}

		if (curr->reliable) {
		        if (!mbus_addr_valid(m, curr->dest)) {
			    debug_msg("Trying to send reliably to an unknown address...\n");
			    if (m->err_handler == NULL) {
				abort();
			    }
			    m->err_handler(curr->seqnum, MBUS_DESTINATION_UNKNOWN);
			}
		        if (!mbus_addr_unique(m, curr->dest)) {
			    debug_msg("Trying to send reliably but address is not unique...\n");
			    if (m->err_handler == NULL) {
				abort();
			    }
			    m->err_handler(curr->seqnum, MBUS_DESTINATION_NOT_UNIQUE);
			}
		}
		/* Create the message... */
		mb_header(curr->seqnum, curr->comp_time.tv_sec, (char)(curr->reliable?'R':'U'), m->addr, curr->dest, -1);
		for (i = 0; i < curr->num_cmds; i++) {
			ASSERT(m->index_sent == (curr->idx_list[i] - 1));
			m->index_sent = curr->idx_list[i];
			mb_add_command(curr->cmd_list[i], curr->arg_list[i]);
		}
		mb_send(m);
		
		m->cmd_queue = curr->next;
		if (curr->reliable) {
			/* Reliable message, wait for the ack... */
			gettimeofday(&(curr->send_time), NULL);
			m->waiting_ack = curr;
			curr->next = NULL;
			return;
		} else {
			while (curr->num_cmds > 0) {
				curr->num_cmds--;
				xfree(curr->cmd_list[curr->num_cmds]); curr->cmd_list[curr->num_cmds] = NULL;
				xfree(curr->arg_list[curr->num_cmds]); curr->arg_list[curr->num_cmds] = NULL;
			}
			xfree(curr->dest);
			xfree(curr);
		}
		curr = m->cmd_queue;
	}
}

void mbus_qmsg(struct mbus *m, const char *dest, const char *cmnd, const char *args, int reliable)
{
	/* Queue up a message for sending. The message is not */
	/* actually sent until mbus_send() is called.         */
	struct mbus_msg	*curr = m->cmd_queue;
	struct mbus_msg	*prev = NULL;
	int		 alen = strlen(cmnd) + strlen(args) + 4;
	int		 i;

	mbus_validate(m);
	while (curr != NULL) {
		mbus_msg_validate(curr);
		if (!curr->complete) {
			/* This message is still open for new commands. It MUST be the last in the */
			/* cmd_queue, else commands will be reordered.                             */
			ASSERT(curr->next == NULL);
			if (mbus_addr_identical(curr->dest, dest) &&
		            (curr->num_cmds < MBUS_MAX_QLEN) && ((curr->message_size + alen) < (MBUS_BUF_SIZE - 500))) {
				curr->num_cmds++;
				curr->reliable |= reliable;
				curr->cmd_list[curr->num_cmds-1] = xstrdup(cmnd);
				curr->arg_list[curr->num_cmds-1] = xstrdup(args);
				curr->idx_list[curr->num_cmds-1] = ++(m->index);
				curr->message_size += alen;
				mbus_msg_validate(curr);
				return;
			} else {
				curr->complete = TRUE;
			}
		}
		prev = curr;
		curr = curr->next;
	}
	/* If we get here, we've not found an open message in the cmd_queue.  We */
	/* have to create a new message, and add it to the end of the cmd_queue. */
	curr = (struct mbus_msg *) xmalloc(sizeof(struct mbus_msg));
	curr->magic            = MBUS_MSG_MAGIC;
	curr->next             = NULL;
	curr->dest             = xstrdup(dest);
	curr->retransmit_count = 0;
	curr->message_size     = alen + 60 + strlen(dest) + strlen(m->addr);
	curr->seqnum           = ++m->seqnum;
	curr->reliable         = reliable;
	curr->complete         = FALSE;
	curr->num_cmds         = 1;
	curr->cmd_list[0]      = xstrdup(cmnd);
	curr->arg_list[0]      = xstrdup(args);
	curr->idx_list[curr->num_cmds-1] = ++(m->index);
	for (i = 1; i < MBUS_MAX_QLEN; i++) {
		curr->cmd_list[i] = NULL;
		curr->arg_list[i] = NULL;
	}
	if (prev == NULL) {
		m->cmd_queue = curr;
	} else {
		prev->next = curr;
	}
	gettimeofday(&(curr->send_time), NULL);
	gettimeofday(&(curr->comp_time), NULL);
	mbus_msg_validate(curr);
}

#define mbus_qmsgf(m, dest, reliable, cmnd, format, var) \
{ \
char buffer[MBUS_BUF_SIZE]; \
mbus_validate(m); \
snprintf(buffer, MBUS_BUF_SIZE, format, var); \
mbus_qmsg(m, dest, cmnd, buffer, reliable); \
}

#if 0
void mbus_qmsgf(struct mbus *m, const char *dest, int reliable, const char *cmnd, const char *format, ...)
{
	/* This is a wrapper around mbus_qmsg() which does a printf() style format into  */
	/* a buffer. Saves the caller from having to a a malloc(), write the args string */
	/* and then do a free(), and also saves worring about overflowing the buffer, so */
	/* removing a common source of bugs!                                             */
	char	buffer[MBUS_BUF_SIZE];
	va_list	ap;

	mbus_validate(m);
	va_start(ap, format);
#ifdef WIN32
        _vsnprintf(buffer, MBUS_BUF_SIZE, format, ap);
#else
        vsnprintf(buffer, MBUS_BUF_SIZE, format, ap);
#endif
	va_end(ap);
	mbus_qmsg(m, dest, cmnd, buffer, reliable);
}
#endif

int mbus_recv(struct mbus *m, void *data, struct timeval *timeout)
{
	char			*auth, *ver, *src, *dst, *ack, *r, *cmd, *param, *npos;
	char	 		 buffer[MBUS_BUF_SIZE];
	int	 		 buffer_len, seq, a, rx, ts, authlen, loop_count;
	char	 		 ackbuf[MBUS_ACK_BUF_SIZE];
	char	 		 digest[16];
	unsigned char		 initVec[8] = {0,0,0,0,0,0,0,0};
	struct timeval		 t;
	struct mbus_parser	*mp, *mp2;

	mbus_validate(m);

	rx = FALSE;
	loop_count = 0;
	while (loop_count++ < 10) {
		memset(buffer, 0, MBUS_BUF_SIZE);
                ASSERT(m->s != NULL);
		udp_fd_zero();
		udp_fd_set(m->s);
		t.tv_sec  = timeout->tv_sec;
		t.tv_usec = timeout->tv_usec;
                if ((udp_select(&t) > 0) && udp_fd_isset(m->s)) {
			buffer_len = udp_recv(m->s, buffer, MBUS_BUF_SIZE);
			if (buffer_len > 0) {
				rx = TRUE;
			} else {
				return rx;
			}
		} else {
			return FALSE;
		}

		if (m->encrkey != NULL) {
			/* Decrypt the message... */
			if ((buffer_len % 8) != 0) {
				debug_msg("Encrypted message not a multiple of 8 bytes in length\n");
				continue;
			}
			memcpy(mb_cryptbuf, buffer, buffer_len);
			memset(initVec, 0, 8);
			qfDES_CBC_d(m->encrkey, mb_cryptbuf, buffer_len, initVec);
			memcpy(buffer, mb_cryptbuf, buffer_len);
		}

		/* Sanity check that this is a vaguely sensible format message... Should prevent */
		/* problems if we're fed complete garbage, but won't prevent determined hackers. */
		if (strncmp(buffer + MBUS_AUTH_LEN + 1, "mbus/1.0", 8) != 0) {
			continue;
		}

		mp = mbus_parse_init(buffer);
		/* remove trailing 0 bytes */
		npos = (char *) strchr(buffer,'\0');
		if(npos!=NULL) {
			buffer_len=npos-buffer;
		}
		/* Parse the authentication header */
		if (!mbus_parse_sym(mp, &auth)) {
			debug_msg("Failed to parse authentication header\n");
			mbus_parse_done(mp);
			continue;
		}

		/* Check that the packet authenticates correctly... */
		authlen = strlen(auth);
		hmac_md5(buffer + authlen + 1, buffer_len - authlen - 1, m->hashkey, m->hashkeylen, digest);
		base64encode(digest, 12, ackbuf, 16);
		if ((strlen(auth) != 16) || (strncmp(auth, ackbuf, 16) != 0)) {
			debug_msg("Failed to authenticate message...\n");
			mbus_parse_done(mp);
			continue;
		}

		/* Parse the header */
		if (!mbus_parse_sym(mp, &ver)) {
			mbus_parse_done(mp);
			debug_msg("Parser failed version (1): %s\n",ver);
			continue;
		}
		if (strcmp(ver, "mbus/1.0") != 0) {
			mbus_parse_done(mp);
			debug_msg("Parser failed version (2): %s\n",ver);
			continue;
		}
		if (!mbus_parse_int(mp, &seq)) {
			mbus_parse_done(mp);
			debug_msg("Parser failed seq\n");
			continue;
		}
		if (!mbus_parse_int(mp, &ts)) {
			mbus_parse_done(mp);
			debug_msg("Parser failed ts\n");
			continue;
		}
		if (!mbus_parse_sym(mp, &r)) {
			mbus_parse_done(mp);
			debug_msg("Parser failed reliable\n");
			continue;
		}
		if (!mbus_parse_lst(mp, &src)) {
			mbus_parse_done(mp);
			debug_msg("Parser failed src\n");
			continue;
		}
		if (!mbus_parse_lst(mp, &dst)) {
			mbus_parse_done(mp);
			debug_msg("Parser failed dst\n");
			continue;
		}
		if (!mbus_parse_lst(mp, &ack)) {
			mbus_parse_done(mp);
			debug_msg("Parser failed ack\n");
			continue;
		}

		store_other_addr(m, src);

		/* Check if the message was addressed to us... */
		if (mbus_addr_match(m->addr, dst)) {
			/* ...if so, process any ACKs received... */
			mp2 = mbus_parse_init(ack);
			while (mbus_parse_int(mp2, &a)) {
				if (mbus_waiting_ack(m)) {
					if (m->waiting_ack->seqnum == a) {
						while (m->waiting_ack->num_cmds > 0) {
							m->waiting_ack->num_cmds--;
							xfree(m->waiting_ack->cmd_list[m->waiting_ack->num_cmds]);
							xfree(m->waiting_ack->arg_list[m->waiting_ack->num_cmds]);
						}
						xfree(m->waiting_ack->dest);
						xfree(m->waiting_ack);
						m->waiting_ack = NULL;
					} else {
						debug_msg("Got ACK %d but wanted %d\n", a, m->waiting_ack->seqnum);
					}
				} else {
					debug_msg("Got ACK %d but wasn't expecting it\n", a);
				}
			}
			mbus_parse_done(mp2);
			/* ...if an ACK was requested, send one... */
			if (strcmp(r, "R") == 0) {
				char 		*newsrc = (char *) xmalloc(strlen(src) + 3);
				struct timeval	 t;

				sprintf(newsrc, "(%s)", src);	/* Yes, this is a kludge. */
				gettimeofday(&t, NULL);
				mb_header(++m->seqnum, (int) t.tv_sec, 'U', m->addr, newsrc, seq);
				mb_send(m);
				xfree(newsrc);
			} else if (strcmp(r, "U") == 0) {
				/* Unreliable message.... not need to do anything */
			} else {
				debug_msg("Message with invalid reliability field \"%s\" ignored\n", r);
			}
			/* ...and process the commands contained in the message */
			while (mbus_parse_sym(mp, &cmd)) {
				if (mbus_parse_lst(mp, &param)) {
					char 		*newsrc = (char *) xmalloc(strlen(src) + 3);
					sprintf(newsrc, "(%s)", src);	/* Yes, this is a kludge. */
					/* Finally, we snoop on the message we just passed to the application, */
					/* to do housekeeping of our list of known mbus sources...             */
					if (strcmp(cmd, "mbus.bye") == 0) {
						remove_other_addr(m, newsrc);
					} 
					if (strcmp(cmd, "mbus.hello") == 0) {
						/* Mark this source as activ. We remove dead sources in mbus_heartbeat */
						store_other_addr(m, newsrc);
					}
					m->cmd_handler(newsrc, cmd, param, data);
					xfree(newsrc);
				} else {
					debug_msg("Unable to parse mbus command:\n");
					debug_msg("cmd = %s\n", cmd);
					debug_msg("arg = %s\n", param);
					break;
				}
			}
		}
		mbus_parse_done(mp);
	}
	return rx;
}

#define RZ_HANDLE_WAITING 1
#define RZ_HANDLE_GO      2

struct mbus_rz {
	char		*peer;
	char		*token;
	struct mbus	*m;
	void		*data;
	int		 mode;
	void (*cmd_handler)(char *src, char *cmd, char *args, void *data);
};

static void rz_handler(char *src, char *cmd, char *args, void *data)
{
	struct mbus_rz		*r = (struct mbus_rz *) data;
	struct mbus_parser	*mp;

	if ((r->mode == RZ_HANDLE_WAITING) && (strcmp(cmd, "mbus.waiting") == 0)) {
		char	*t;

		mp = mbus_parse_init(args);
		mbus_parse_str(mp, &t);
		if (strcmp(mbus_decode_str(t), r->token) == 0) {
                        if (r->peer != NULL) xfree(r->peer);
			r->peer = xstrdup(src);
		}
		mbus_parse_done(mp);
	} else if ((r->mode == RZ_HANDLE_GO) && (strcmp(cmd, "mbus.go") == 0)) {
		char	*t;

		mp = mbus_parse_init(args);
		mbus_parse_str(mp, &t);
		if (strcmp(mbus_decode_str(t), r->token) == 0) {
                        if (r->peer != NULL) xfree(r->peer);
			r->peer = xstrdup(src);
		}
		mbus_parse_done(mp);
	} else {
		r->cmd_handler(src, cmd, args, r->data);
	}
}

char *mbus_rendezvous_waiting(struct mbus *m, char *addr, char *token, void *data)
{
	/* Loop, sending mbus.waiting(token) to "addr", until we get mbus.go(token) */
	/* back from our peer. Any other mbus commands received whilst waiting are  */
	/* processed in the normal manner, as if mbus_recv() had been called.       */
	char		*token_e, *peer;
	struct timeval	 timeout;
	struct mbus_rz	*r;

	mbus_validate(m);

	r = (struct mbus_rz *) xmalloc(sizeof(struct mbus_rz));
	r->peer        = NULL;
	r->token       = token;
	r->m           = m;
	r->data        = data;
	r->mode        = RZ_HANDLE_GO;
	r->cmd_handler = m->cmd_handler;
	m->cmd_handler = rz_handler;
	token_e        = mbus_encode_str(token);
	while (r->peer == NULL) {
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		mbus_heartbeat(m, 1);
		mbus_qmsgf(m, addr, FALSE, "mbus.waiting", "%s", token_e);
		mbus_send(m);
		mbus_recv(m, r, &timeout);
		mbus_retransmit(m);
	}
	m->cmd_handler = r->cmd_handler;
	peer = r->peer;
	xfree(r);
	xfree(token_e);
	return peer;
}

char *mbus_rendezvous_go(struct mbus *m, char *token, void *data)
{
	/* Wait until we receive mbus.waiting(token), then send mbus.go(token) back to   */
	/* the sender of that message. Whilst waiting, other mbus commands are processed */
	/* in the normal manner as if mbus_recv() had been called.                       */
	char		*token_e, *peer;
	struct timeval	 timeout;
	struct mbus_rz	*r;

	mbus_validate(m);

	r = (struct mbus_rz *) xmalloc(sizeof(struct mbus_rz));
	r->peer        = NULL;
	r->token       = token;
	r->m           = m;
	r->data        = data;
	r->mode        = RZ_HANDLE_WAITING;
	r->cmd_handler = m->cmd_handler;
	m->cmd_handler = rz_handler;
	token_e        = mbus_encode_str(token);
	while (r->peer == NULL) {
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		mbus_heartbeat(m, 1);
		mbus_send(m);
		mbus_recv(m, r, &timeout);
		mbus_retransmit(m);
	}

	mbus_qmsgf(m, r->peer, TRUE, "mbus.go", "%s", token_e);
	do {
		mbus_heartbeat(m, 1);
		mbus_retransmit(m);
		mbus_send(m);
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;
		mbus_recv(m, r, &timeout);
	} while (!mbus_sent_all(m));

	m->cmd_handler = r->cmd_handler;
	peer = r->peer;
	xfree(r);
	xfree(token_e);
	return peer;
}

