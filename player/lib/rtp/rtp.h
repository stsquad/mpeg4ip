/*
 * FILE:   rtp.h
 * AUTHOR: Colin Perkins <c.perkins@cs.ucl.ac.uk>
 *
 * $Revision: 1.4 $ 
 * $Date: 2001/06/28 23:54:22 $
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
 */

#ifndef __RTP_H__
#define __RTP_H__

#include "config_unix.h"
#include "config_win32.h"
#ifdef __cplusplus
extern "C" {
#endif

#define RTP_VERSION 2
// wmay - added next, prev pointers to rtp packet
#define RTP_PACKET_HEADER_SIZE	((sizeof(char *) * 2) + sizeof(uint32_t *) + \
(2 * sizeof(int)) + (2 * sizeof(rtp_packet *)))
#define RTP_MAX_PACKET_LEN 1500

#if !defined(WORDS_BIGENDIAN) && !defined(WORDS_SMALLENDIAN)
#error RTP library requires WORDS_BIGENDIAN or WORDS_SMALLENDIAN to be defined.
#endif

typedef struct rtp_packet {
	/* The following are pointers to the data in the packet as    */
	/* it came off the wire. The packet it read in such that the  */
	/* header maps onto the latter part of this struct, and the   */
	/* fields in this first part of the struct point into it. The */
	/* entire packet can be freed by freeing this struct, without */
	/* having to free the csrc, data and extn blocks separately.  */
	/* WARNING: Don't change the size of the first portion of the */
	/* struct without changing RTP_PACKET_HEADER_SIZE to match.   */
  // wmay - added next, prev pointers
  struct rtp_packet *next, *prev;
	uint32_t	*csrc;
	char		*data;
	int		 data_len;
	char		*extn;
	uint16_t	 extn_len;	/* Size of the extension in 32 bit words minus one */
	uint16_t	 extn_type;	/* Extension type field in the RTP packet header   */
	/* The following map directly onto the RTP packet header...   */
#ifdef WORDS_BIGENDIAN
	unsigned short   v:2;		/* packet type                */
	unsigned short   p:1;		/* padding flag               */
	unsigned short   x:1;		/* header extension flag      */
	unsigned short   cc:4;		/* CSRC count                 */
	unsigned short   m:1;		/* marker bit                 */
	unsigned short   pt:7;		/* payload type               */
#else
	unsigned short   cc:4;		/* CSRC count                 */
	unsigned short   x:1;		/* header extension flag      */
	unsigned short   p:1;		/* padding flag               */
	unsigned short   v:2;		/* packet type                */
	unsigned short   pt:7;		/* payload type               */
	unsigned short   m:1;		/* marker bit                 */
#endif
	uint16_t          seq;		/* sequence number            */
	uint32_t          ts;		/* timestamp                  */
	uint32_t          ssrc;		/* synchronization source     */
	/* The csrc list, header extension and data follow, but can't */
	/* be represented in the struct.                              */
} rtp_packet;

typedef struct {
	uint32_t         ssrc;
	uint32_t         ntp_sec;
	uint32_t         ntp_frac;
	uint32_t         rtp_ts;
	uint32_t         sender_pcount;
	uint32_t         sender_bcount;
} rtcp_sr;

typedef struct {
	uint32_t	ssrc;		/* The ssrc to which this RR pertains */
#ifdef WORDS_BIGENDIAN
	uint32_t	fract_lost:8;
	uint32_t	total_lost:24;
#else
	uint32_t	total_lost:24;
	uint32_t	fract_lost:8;
#endif	
	uint32_t	last_seq;
	uint32_t	jitter;
	uint32_t	lsr;
	uint32_t	dlsr;
} rtcp_rr;

typedef struct {
	uint8_t		type;		/* type of SDES item              */
	uint8_t		length;		/* length of SDES item (in bytes) */
	char		data[1];	/* text, not zero-terminated      */
} rtcp_sdes_item;

typedef struct {
#ifdef WORDS_BIGENDIAN
	unsigned short  version:2;	/* RTP version            */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  subtype:5;	/* application dependent  */
#else
	unsigned short  subtype:5;	/* application dependent  */
	unsigned short  p:1;		/* padding flag           */
	unsigned short  version:2;	/* RTP version            */
#endif
	unsigned short  pt:8;		/* packet type            */
	uint16_t        length;		/* packet length          */
	uint32_t        ssrc;
	char            name[4];        /* four ASCII characters  */
	char            data[1];        /* variable length field  */
} rtcp_app;

/* rtp_event type values... use enum so compiler can check switch/if statements to see everything is covered */
typedef enum {
        RX_RTP,
        RX_SR,
        RX_RR,
        RX_SDES,
        RX_BYE,         /* Source is leaving the session, database entry is still valid */
        SOURCE_CREATED,
        SOURCE_DELETED, /* Source has been removed from the database                    */
        RX_RR_EMPTY,    /* We've received an empty reception report block                                               */
        RX_RTCP_START,  /* We're about to start processing a compound RTCP packet. The SSRC is not valid in this event. */
        RX_RTCP_FINISH,	/* We've just finished processing a compound RTCP packet. The SSRC is not valid in this event.  */
        RR_TIMEOUT,
        RX_APP
} rtp_event_type;

typedef struct {
	uint32_t	 ssrc;
	rtp_event_type	 type;
	void		*data;
	struct timeval	*ts;
} rtp_event;

/* RTP options */
typedef enum {
        RTP_OPT_PROMISC =	    1,
        RTP_OPT_WEAK_VALIDATION	=   2,
        RTP_OPT_FILTER_MY_PACKETS = 3
} rtp_option;

/* SDES packet types... */
typedef enum {
        RTCP_SDES_END   = 0,
        RTCP_SDES_CNAME = 1,
        RTCP_SDES_NAME  = 2,
        RTCP_SDES_EMAIL = 3,
        RTCP_SDES_PHONE = 4,
        RTCP_SDES_LOC   = 5,
        RTCP_SDES_TOOL  = 6,
        RTCP_SDES_NOTE  = 7,
        RTCP_SDES_PRIV  = 8
} rtcp_sdes_type;

struct rtp;

struct rtp	*rtp_init(char *addr, uint16_t rx_port, uint16_t tx_port, int ttl, double rtcp_bw, 
			  void (*callback)(struct rtp *session, rtp_event *e),
			  void *user_data);
struct rtp	*rtp_init_if(char *addr, char *iface, uint16_t rx_port, uint16_t tx_port, int ttl, double rtcp_bw, 
			     void (*callback)(struct rtp *session, rtp_event *e),
			     void *user_data);
int 		 rtp_setopt(struct rtp *session, int optname, int optval);
int 		 rtp_getopt(struct rtp *session, int optname, int *optval);
void 		*rtp_get_userdata(struct rtp *session);
int 		 rtp_recv(struct rtp *session, struct timeval *timeout, uint32_t curr_rtp_ts);
int 		 rtp_send_data(struct rtp *session, uint32_t rtp_ts, char pt, int m, int cc, uint32_t csrc[], char *data, int data_len, char *extn, uint16_t extn_len, uint16_t extn_type);
#ifndef _WIN32
int 		 rtp_send_data_iov(struct rtp *session, uint32_t rtp_ts, char pt, int m, int cc, uint32_t csrc[], struct iovec *iov, int iov_count, char *extn, uint16_t extn_len, uint16_t extn_type);
#endif
void 		 rtp_send_ctrl(struct rtp *session, uint32_t rtp_ts, 
			       rtcp_app *(*appcallback)(struct rtp *session, uint32_t rtp_ts, int max_size));
void 		 rtp_update(struct rtp *session);

uint32_t	 rtp_my_ssrc(struct rtp *session);
int		 rtp_add_csrc(struct rtp *session, uint32_t csrc);
int		 rtp_valid_ssrc(struct rtp *session, uint32_t ssrc);
int		 rtp_set_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type, char *value, int length);
const char	*rtp_get_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type);
const rtcp_sr	*rtp_get_sr(struct rtp *session, uint32_t ssrc);
const rtcp_rr	*rtp_get_rr(struct rtp *session, uint32_t reporter, uint32_t reportee);
void		 rtp_send_bye(struct rtp *session);
void		 rtp_done(struct rtp *session);
int              rtp_set_encryption_key(struct rtp *session, const char *passphrase);

char 		*rtp_get_addr(struct rtp *session);
uint16_t	 rtp_get_rx_port(struct rtp *session);
uint16_t	 rtp_get_tx_port(struct rtp *session);
int		 rtp_get_ttl(struct rtp *session);

int              rtp_set_my_ssrc(struct rtp *session, uint32_t ssrc);

#ifdef __cplusplus
}
#endif
#endif /* __RTP_H__ */
