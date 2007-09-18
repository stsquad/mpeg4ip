/*
 * FILE:   rtp.h
 * AUTHOR: Colin Perkins <c.perkins@cs.ucl.ac.uk>
 *
 * $Revision: 1.23 $ 
 * $Date: 2007/09/18 20:52:01 $
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

#ifndef HAVE_STRUCT_SOCKADDR_STORAGE
#include "sockstorage.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
  
#define RTP_PACKET_HEADER_SIZE	(sizeof(rtp_packet_data))
#define RTP_VERSION 2
#define RTP_MAX_PACKET_LEN 1500


  // moved here by nori
typedef int (*rtp_encrypt_f)(void *, uint8_t *, uint32_t *);
typedef int (*rtp_decrypt_f)(void *ud, uint8_t *packet_head, uint32_t *packet_len);

struct rtp;

/* XXX gtkdoc doesn't seem to be able to handle functions that return
 * struct *'s. */
typedef struct rtp *rtp_t;

typedef struct rtp_packet_header {
#ifdef WORDS_BIGENDIAN
	unsigned short   ph_v:2;	/* packet type                */
	unsigned short   ph_p:1;	/* padding flag               */
	unsigned short   ph_x:1;	/* header extension flag      */
	unsigned short   ph_cc:4;	/* CSRC count                 */
	unsigned short   ph_m:1;	/* marker bit                 */
	unsigned short   ph_pt:7;	/* payload type               */
#else
	unsigned short   ph_cc:4;	/* CSRC count                 */
	unsigned short   ph_x:1;	/* header extension flag      */
	unsigned short   ph_p:1;	/* padding flag               */
	unsigned short   ph_v:2;	/* packet type                */
	unsigned short   ph_pt:7;	/* payload type               */
	unsigned short   ph_m:1;	/* marker bit                 */
#endif
	uint16_t          ph_seq;	/* sequence number            */
	uint32_t          ph_ts;	/* timestamp                  */
	uint32_t          ph_ssrc;	/* synchronization source     */
	/* The csrc list, header extension and data follow, but can't */
	/* be represented in the struct.                              */
} rtp_packet_header;

typedef struct rtp_packet_data {
  struct rtp_packet *rtp_pd_next, *rtp_pd_prev;
  uint32_t	*rtp_pd_csrc;
  uint8_t	*rtp_pd_data;
  uint32_t	 rtp_pd_data_len;
  uint8_t	*rtp_pd_extn;
  uint16_t	 rtp_pd_extn_len; /* Size of the extension in 32 bit words minus one */
  uint16_t	 rtp_pd_extn_type;/* Extension type field in the RTP packet header   */
  uint8_t       *pd_buf_start;
  uint32_t       pd_buflen; /* received buffer len (w/rtp header) */
  int            rtp_pd_have_timestamp;
  uint64_t       rtp_pd_timestamp;
  struct sockaddr_storage rtp_rx_addr;
  socklen_t      rtp_rx_addr_len;
  rtp_packet_header *ph;
} rtp_packet_data;

  

#define packet_start    pd.pd_buf_start
#define packet_length   pd.pd_buflen
#define rtp_next      pd.rtp_pd_next
#define rtp_prev      pd.rtp_pd_prev
#define rtp_csrc      pd.rtp_pd_csrc
#define rtp_data      pd.rtp_pd_data
#define rtp_data_len  pd.rtp_pd_data_len
#define rtp_extn      pd.rtp_pd_extn
#define rtp_extn_len  pd.rtp_pd_extn_len
#define rtp_extn_type pd.rtp_pd_extn_type
  
#define rtp_pak_v    pd.ph->ph_v
#define rtp_pak_p    pd.ph->ph_p
#define rtp_pak_x    pd.ph->ph_x
#define rtp_pak_cc   pd.ph->ph_cc
#define rtp_pak_m    pd.ph->ph_m
#define rtp_pak_pt   pd.ph->ph_pt
#define rtp_pak_seq  pd.ph->ph_seq
#define rtp_pak_ts   pd.ph->ph_ts
#define rtp_pak_ssrc pd.ph->ph_ssrc

typedef struct rtp_packet {
  /* The following are pointers to the data in the packet as    */
  /* it came off the wire. The packet it read in such that the  */
  /* header maps onto the latter part of this struct, and the   */
  /* fields in this first part of the struct point into it. The */
  /* entire packet can be freed by freeing this struct, without */
  /* having to free the csrc, data and extn blocks separately.  */
  rtp_packet_data pd;
  /* The following map directly onto the RTP packet header...   */
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

/* rtp_event type values. */
typedef enum {
        RX_RTP,
        RX_SR,
        RX_RR,
        RX_SDES,
        RX_BYE,         /* Source is leaving the session, database entry is still valid                           */
        SOURCE_CREATED,
        SOURCE_DELETED, /* Source has been removed from the database                                              */
        RX_RR_EMPTY,    /* We've received an empty reception report block                                         */
        RX_RTCP_START,  /* Processing a compound RTCP packet about to start. The SSRC is not valid in this event. */
        RX_RTCP_FINISH,	/* Processing a compound RTCP packet finished. The SSRC is not valid in this event.       */
        RR_TIMEOUT,
        RX_APP
} rtp_event_type;

typedef struct {
	uint32_t	 ssrc;
	rtp_event_type	 type;
	void		*data;
	struct timeval	*ts;
} rtp_event;

/* Callback types */
typedef void (*rtp_callback_f)(struct rtp *session, rtp_event *e);
typedef rtcp_app* (*rtcp_app_callback_f)(struct rtp *session, uint32_t rtp_ts, uint32_t max_size);

typedef int (*send_packet_f)(void *userdata, uint8_t *buffer, uint32_t buflen);
typedef int (*send_packet_iov_f)(void *userdata, struct iovec *iov, int count);
/* SDES packet types... */
typedef enum  {
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

typedef struct {
	uint8_t		type;		/* type of SDES item              */
	uint8_t		length;		/* length of SDES item (in bytes) */
	char		data[1];	/* text, not zero-terminated      */
} rtcp_sdes_item;

/* RTP options */
typedef enum {
        RTP_OPT_PROMISC =	    1,
        RTP_OPT_WEAK_VALIDATION	=   2,
        RTP_OPT_FILTER_MY_PACKETS = 3
} rtp_option;

typedef struct socket_udp_ socket_udp; 

typedef struct rtp_stream_params_t {
  const char *rtp_addr;
  in_port_t   rtp_rx_port;
  in_port_t   rtp_tx_port;
  uint8_t     rtp_ttl;

  const char *rtcp_addr;
  in_port_t   rtcp_rx_port;
  in_port_t   rtcp_tx_port;
  uint8_t     rtcp_ttl;

  const char *physical_interface_addr;
  int dont_init_sockets;  // for use with tcp
  int transmit_initial_rtcp; // transmit a fast rtcp packet

  double rtcp_bandwidth;  
  
  socket_udp  *rtp_socket;  // rtp socket to use
  socket_udp  *rtcp_socket; // rtcp socket to use
  // callbacks
  rtp_callback_f rtp_callback;
  send_packet_f rtp_send_packet;
  send_packet_iov_f rtp_send_packet_iov;
  send_packet_f rtcp_send_packet;
  // user data
  void *send_userdata;
  void *recv_userdata;
} rtp_stream_params_t;

typedef struct rtp_encryption_params_t {
  void *userdata;
  rtp_encrypt_f rtp_encrypt, rtcp_encrypt;
  rtp_decrypt_f rtp_decrypt, rtcp_decrypt;
    
  uint8_t rtp_auth_alloc_extra;
  uint8_t rtcp_auth_alloc_extra;
} rtp_encryption_params_t;

rtp_stream_params_t *rtp_default_params(rtp_stream_params_t *ptr);

rtp_t rtp_init_stream(rtp_stream_params_t *param_ptr);
  
/* API */
rtp_t		rtp_init(const char *addr, 
			 uint16_t rx_port, uint16_t tx_port, 
			 int ttl, double rtcp_bw, 
			 rtp_callback_f callback,
			 void *recv_userdata);
  // rtp_init_xmitter - for transmitters - send an RTCP with the first packet
rtp_t		rtp_init_if(const char *addr, char *iface, 
			    uint16_t rx_port, uint16_t tx_port, 
			    int ttl, double rtcp_bw, 
			    rtp_callback_f callback,
			    void *recv_userdata,
			    int dont_init_sockets);

void		 rtp_send_bye(struct rtp *session);
void		 rtp_done(struct rtp *session);

int 		 rtp_set_option(struct rtp *session, rtp_option optname, int optval);
int 		 rtp_get_option(struct rtp *session, rtp_option optname, int *optval);

int 		 rtp_recv(struct rtp *session, 
			  struct timeval *timeout, uint32_t curr_rtp_ts);
  void rtp_recv_data(struct rtp *session, uint32_t curr_rtp_ts);
int 		 rtp_send_data(struct rtp *session, 
			       uint32_t rtp_ts, int8_t pt, int m, 
			       unsigned int cc, uint32_t csrc[], 
                               uint8_t *data, uint32_t data_len, 
			       uint8_t *extn, uint16_t extn_len, uint16_t extn_type);
int            rtp_send_data_iov(struct rtp *session, 
				 uint32_t rtp_ts, int8_t pt, int m, 
				 unsigned int cc, uint32_t csrc[], 
				 struct iovec *iov, uint32_t iov_count, 
				 uint8_t *extn, uint16_t extn_len, 
				 uint16_t extn_type, uint16_t seq_num_add);
void 		 rtp_send_ctrl(struct rtp *session, uint32_t rtp_ts, 
			       rtcp_app_callback_f appcallback);
void 		 rtp_send_ctrl_2(struct rtp *session, uint32_t rtp_ts,
				 uint64_t ntp_ts, 
				 rtcp_app_callback_f appcallback);
void 		 rtp_update(struct rtp *session);

uint32_t	 rtp_my_ssrc(struct rtp *session);
int		 rtp_add_csrc(struct rtp *session, uint32_t csrc);
int		 rtp_del_csrc(struct rtp *session, uint32_t csrc);

int		 rtp_set_sdes(struct rtp *session, uint32_t ssrc, 
			      rtcp_sdes_type type, 
			      const char *value, int length);
const char	*rtp_get_sdes(struct rtp *session, uint32_t ssrc, rtcp_sdes_type type);

const rtcp_sr	*rtp_get_sr(struct rtp *session, uint32_t ssrc);
const rtcp_rr	*rtp_get_rr(struct rtp *session, uint32_t reporter, uint32_t reportee);

int              rtp_set_my_ssrc(struct rtp *session, uint32_t ssrc);

char 		*rtp_get_addr(struct rtp *session);
uint16_t	 rtp_get_rx_port(struct rtp *session);
uint16_t	 rtp_get_tx_port(struct rtp *session);
int		 rtp_get_ttl(struct rtp *session);
void		*rtp_get_recv_userdata(struct rtp *session);

  
int rtp_process_recv_data(struct rtp *session,
			  uint32_t curr_rtp_ts,
			  rtp_packet *packet);

void rtp_process_ctrl(struct rtp *session, uint8_t *buffer, uint32_t buflen);

  int rtp_set_encryption_params (struct rtp *session, rtp_encryption_params_t *params);
  
  int rtp_get_encryption_enabled(struct rtp *session);
  

socket_udp *get_rtp_data_socket(struct rtp *session);
socket_udp *get_rtp_rtcp_socket(struct rtp *session);

  void rtp_set_receive_buffer_default_size(int bufsize);  
  unsigned int rtp_get_mtu_adjustment (struct rtp *session);

  void rtp_set_rtp_callback(struct rtp *session, rtp_callback_f rtp,
			    void *userdata);
#ifdef __cplusplus
}
#endif
#endif /* __RTP_H__ */

