/*
 * FILE:     net_udp.c
 * AUTHOR:   Colin Perkins 
 * MODIFIED: Orion Hodson, Piers O'Hanlon, Kristian Hasler
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

/* If this machine supports IPv6 the symbol HAVE_IPv6 should */
/* be defined in either config_unix.h or config_win32.h. The */
/* appropriate system header files should also be included   */
/* by those files.                                           */

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "inet_pton.h"
#include "inet_ntop.h"
#include "vsnprintf.h"
#include "net_udp.h"
#ifndef _WIN32
//#include "mpeg4ip_config.h"
#endif

#ifdef NEED_ADDRINFO_H
#include "addrinfo.h"
#endif

#define IPv4	4
#define IPv6	6

#if defined(WIN2K_IPV6) || defined(WINXP_IPV6)
const struct	in6_addr	in6addr_any = {IN6ADDR_ANY_INIT};
#endif

/* This is pretty nasty but it's the simplest way to get round */
/* the Detexis bug that means their MUSICA IPv6 stack uses     */
/* IPPROTO_IP instead of IPPROTO_IPV6 in setsockopt calls      */
/* We also need to define in6addr_any */
#ifdef  MUSICA_IPV6
#define	IPPROTO_IPV6	IPPROTO_IP
struct	in6_addr	in6addr_any = {IN6ADDR_ANY_INIT};

/* These DEF's are required as MUSICA's winsock6.h causes a clash with some of the 
 * standard ws2tcpip.h definitions (eg struct in_addr6).
 * Note: winsock6.h defines AF_INET6 as 24 NOT 23 as in winsock2.h - I have left it
 * set to the MUSICA value as this is used in some of their function calls. 
 */
//#define AF_INET6        23
#define IP_MULTICAST_LOOP      11 /*set/get IP multicast loopback */
#define	IP_MULTICAST_IF		9 /* set/get IP multicast i/f  */
#define	IP_MULTICAST_TTL       10 /* set/get IP multicast ttl */
#define	IP_MULTICAST_LOOP      11 /*set/get IP multicast loopback */
#define	IP_ADD_MEMBERSHIP      12 /* add an IP group membership */
#define	IP_DROP_MEMBERSHIP     13/* drop an IP group membership */
#define IP_ADD_SOURCE_MEMBERSHIP 39

#define IN6_IS_ADDR_UNSPECIFIED(a) (((a)->s6_addr32[0] == 0) && \
									((a)->s6_addr32[1] == 0) && \
									((a)->s6_addr32[2] == 0) && \
									((a)->s6_addr32[3] == 0))
struct ip_mreq {
	struct in_addr imr_multiaddr;	/* IP multicast address of group */
	struct in_addr imr_interface;	/* local IP address of interface */
};
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

static char G_Multicast_Src[256];
static int G_IGMP_V3=0;

struct socket_udp_ {
	int	 	 mode;	/* IPv4 or IPv6 */
        char	        *addr;
	uint16_t	 rx_port;
	uint16_t	 tx_port;
	ttl_t	 	 ttl;
	fd_t	 	 fd;
	struct in_addr	 addr4;
	struct in_addr	 iface4_addr;
#ifdef HAVE_IPv6
	struct in6_addr	 addr6;
#endif /* HAVE_IPv6 */
};

#ifdef _WIN32
/* Want to use both Winsock 1 and 2 socket options, but since
* IPv6 support requires Winsock 2 we have to add own backwards
* compatibility for Winsock 1.
*/
#define SETSOCKOPT winsock_versions_setsockopt
#else
#define SETSOCKOPT setsockopt
#endif /* WIN32 */

/*****************************************************************************/
/* Support functions...                                                      */
/*****************************************************************************/

static void
socket_error(const char *msg, ...)
{
	char		buffer[255];
	uint32_t	blen = sizeof(buffer) / sizeof(buffer[0]);
	va_list		ap;

#ifdef WIN32
#define WSERR(x) {#x,x}
	struct wse {
		char  errname[20];
		int my_errno;
	};
	struct wse ws_errs[] = {
		WSERR(WSANOTINITIALISED), WSERR(WSAENETDOWN),     WSERR(WSAEACCES),
		WSERR(WSAEINVAL),         WSERR(WSAEINTR),        WSERR(WSAEINPROGRESS),
		WSERR(WSAEFAULT),         WSERR(WSAENETRESET),    WSERR(WSAENOBUFS),
		WSERR(WSAENOTCONN),       WSERR(WSAENOTSOCK),     WSERR(WSAEOPNOTSUPP),
		WSERR(WSAESHUTDOWN),      WSERR(WSAEWOULDBLOCK),  WSERR(WSAEMSGSIZE),
		WSERR(WSAEHOSTUNREACH),   WSERR(WSAECONNABORTED), WSERR(WSAECONNRESET),
		WSERR(WSAEADDRNOTAVAIL),  WSERR(WSAEAFNOSUPPORT), WSERR(WSAEDESTADDRREQ),
		WSERR(WSAENETUNREACH),    WSERR(WSAETIMEDOUT),    WSERR(0)
	};
	
	int i, e = WSAGetLastError();
	i = 0;
	while(ws_errs[i].my_errno && ws_errs[i].my_errno != e) {
		i++;
	}
	va_start(ap, msg);
	_vsnprintf(buffer, blen, msg, ap);
	va_end(ap);
	rtp_message(LOG_ALERT, "ERROR: %s, (%d - %s)\n", buffer, e, ws_errs[i].errname);
#else
	uint32_t retlen;
	va_start(ap, msg);
	retlen = vsnprintf(buffer, blen, msg, ap);
	va_end(ap);
	rtp_message(LOG_ALERT, "%s:%s", buffer, strerror(errno));
#endif
}

#ifdef WIN32
/* ws2tcpip.h defines these constants with different values from
* winsock.h so files that use winsock 2 values but try to use 
* winsock 1 fail.  So what was the motivation in changing the
* constants ?
*/
#define WS1_IP_MULTICAST_IF     2 /* set/get IP multicast interface   */
#define WS1_IP_MULTICAST_TTL    3 /* set/get IP multicast timetolive  */
#define WS1_IP_MULTICAST_LOOP   4 /* set/get IP multicast loopback    */
#define WS1_IP_ADD_MEMBERSHIP   5 /* add  an IP group membership      */
#define WS1_IP_DROP_MEMBERSHIP  6 /* drop an IP group membership      */

/* winsock_versions_setsockopt tries 1 winsock version of option 
* optname and then winsock 2 version if that failed.
* note: setting the TTL never fails, so we have to try both.
*/

static int
winsock_versions_setsockopt(SOCKET s, int level, int optname, const char FAR * optval, int optlen)
{
	int success = -1;
	switch (optname) {
	case IP_MULTICAST_IF:
		success = setsockopt(s, level, WS1_IP_MULTICAST_IF, optval, optlen);
		break;
	case IP_MULTICAST_TTL:
		success = setsockopt(s, level, WS1_IP_MULTICAST_TTL, optval, optlen);
 		success = setsockopt(s, level, optname, optval, optlen);
		break;
	case IP_MULTICAST_LOOP:
		success = setsockopt(s, level, WS1_IP_MULTICAST_LOOP, optval, optlen);
		break;
	case IP_ADD_MEMBERSHIP: 
		success = setsockopt(s, level, WS1_IP_ADD_MEMBERSHIP, optval, optlen);
		break;
	case IP_DROP_MEMBERSHIP: 
		success = setsockopt(s, level, WS1_IP_DROP_MEMBERSHIP, optval, optlen);
		break;
	}
	if (success != -1) {
		return success;
	}
	return setsockopt(s, level, optname, optval, optlen);
}
#endif

#if defined(NEED_INET_ATON) || !defined(HAVE_INET_ATON)
#ifdef NEED_INET_ATON_STATIC
static 
#endif
int inet_aton(const char *name, struct in_addr *addr)
{
	addr->s_addr = inet_addr(name);
	return (addr->s_addr != (in_addr_t) INADDR_NONE);
}
#endif

#ifdef NEED_IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(addr) ((addr)->s6_addr[0] == 0xffU)
#endif

#if defined(NEED_IN6_IS_ADDR_UNSPECIFIED) && defined(MUSICA_IPV6)
#define IN6_IS_ADDR_UNSPECIFIED(addr) IS_UNSPEC_IN6_ADDR(*addr)
#endif



/*****************************************************************************/
/* IPv4 specific functions...                                                */
/*****************************************************************************/

static int udp_addr_valid4(const char *dst)
{
        struct in_addr addr4;
	struct hostent *h;

	if (inet_pton(AF_INET, dst, &addr4)) {
		return TRUE;
	} 

	h = gethostbyname(dst);
	if (h != NULL) {
		return TRUE;
	}
	socket_error("Can't resolve IP address for %s", dst);

        return FALSE;
}

static int have_recv_buf_size =
#ifdef _WIN32
     1
#else
     0
#endif
;
static int recv_buf_size_value = 65536;

static socket_udp *udp_init4 (const char *addr, const char *iface, 
			      uint16_t rx_port, uint16_t tx_port, int ttl)
{
  int                 	 reuse = 1;
  struct sockaddr_in  	 s_in;
  int recv_buf_size;
  int test_buffer;
  socklen_t test_buffer_size=sizeof(test_buffer);

  socket_udp         	*s = (socket_udp *)malloc(sizeof(socket_udp));
  s->mode    = IPv4;
  s->addr    = NULL;
  s->rx_port = rx_port;
  s->tx_port = tx_port;
  s->ttl     = ttl;
  if (addr != NULL) {
    if (inet_pton(AF_INET, addr, &s->addr4) != 1) {
      struct hostent *h = gethostbyname(addr);
      if (h == NULL) {
	socket_error("Can't resolve IP address for %s", addr);
	free(s);
	return NULL;
      }
      memcpy(&(s->addr4), h->h_addr_list[0], sizeof(s->addr4));
    }
  } else {
    s->addr4.s_addr = 0;
  }

  if (iface != NULL) {
    if (inet_pton(AF_INET, iface, &s->iface4_addr) != 1) {
      rtp_message(LOG_ERR, "Illegal interface specification");
      free(s);
      return NULL;
    }
  } else {
    s->iface4_addr.s_addr = 0;
  }
  s->fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (s->fd < 0) {
    socket_error("socket");
    free(s);
    return NULL;
  }
  if (have_recv_buf_size != 0) {
    recv_buf_size = recv_buf_size_value;
    if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_RCVBUF, (char *)&recv_buf_size, sizeof(int)) != 0) {
      socket_error("setsockopt SO_RCVBUF");
      closesocket(s->fd);
      free(s);
      return NULL;
    }

    //Since setsockopt would not return the error if /proc/sys/net/core/rmem_max is smaller
    //then the value you are trying to set. use sysctl -w net.core.rmem_max=new_val
    //to set the value higher than what is desired in RCVBUF
    if( getsockopt( s->fd, SOL_SOCKET, SO_RCVBUF, (void*)&test_buffer, &test_buffer_size ) == -1 )
      {
	socket_error("getsockopt SO_RCVBUF");
      } else {
	//See if we could set the desired value
	if(test_buffer < recv_buf_size) {
	  rtp_message(LOG_WARNING, "Failed to set the RCVBUF to %d, only could set %d\n. Check the Max kernel receive buffer size using \"sysctl net.core.rmem_max\"\n", recv_buf_size, test_buffer);
	}
      }
  }
	
  if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0) {
    socket_error("setsockopt SO_REUSEADDR");
    closesocket(s->fd);
    free(s);
    return NULL;
  }
#ifdef SO_REUSEPORT
  if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0) {
    closesocket(s->fd);
    free(s);
    socket_error("setsockopt SO_REUSEPORT");
    return NULL;
  }
#endif
  s_in.sin_family      = AF_INET;
  s_in.sin_addr.s_addr = INADDR_ANY;
  s_in.sin_port        = htons(rx_port);
  if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0) {
    socket_error("bind: port %d", rx_port);
    closesocket(s->fd);
    free(s);
    return NULL;
  }
  if (IN_MULTICAST(ntohl(s->addr4.s_addr))) {
    char            loop = 1;
#ifndef HAVE_IGMP_V3
    struct ip_mreq  imr;
    imr.imr_multiaddr.s_addr = s->addr4.s_addr;
    imr.imr_interface.s_addr = s->iface4_addr.s_addr;

    if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) != 0) {
      socket_error("setsockopt IP_ADD_MEMBERSHIP");
      closesocket(s->fd);
      free(s);
      return NULL;
    }
#else 
    rtp_message(LOG_DEBUG,"IGMPV3 src:%s\n", G_Multicast_Src);
    /* Join Multicast group with source filter */
    if (G_IGMP_V3 != 0 &&
	strcmp(G_Multicast_Src, "0.0.0.0") != 0) {
      struct ip_mreq_source imr;
      imr.imr_multiaddr.s_addr = s->addr4.s_addr;
      imr.imr_interface.s_addr = s->iface4_addr.s_addr;
      if (inet_aton(G_Multicast_Src, &imr.imr_sourceaddr) == 0) {
	rtp_message(LOG_ERR, "inet_aton failed for %s\n",
		    G_Multicast_Src);
	return NULL;
      }

      if( setsockopt( s->fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
		      (char*)&imr,
		      sizeof(struct ip_mreq_source) ) == -1 )
	{
	  socket_error("setsockopt IP_ADD_SOURCE_MEMBERSHIP");
	  closesocket(s->fd);
	  free(s);
	  return NULL;
	}
    }
#endif
        
#ifndef WIN32
    if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
      socket_error("setsockopt IP_MULTICAST_LOOP");
      closesocket(s->fd);
      free(s);
      return NULL;
    }
#endif
    if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &s->ttl, sizeof(s->ttl)) != 0) {
      socket_error("setsockopt IP_MULTICAST_TTL");
      closesocket(s->fd);
      free(s);
      return NULL;
    }
    if (s->iface4_addr.s_addr != 0) {
      if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_MULTICAST_IF, (char *) &s->iface4_addr, sizeof(s->iface4_addr)) != 0) {
	closesocket(s->fd);
	free(s);
	socket_error("setsockopt IP_MULTICAST_IF");
	return NULL;
      }
    }
  }
  if (addr != NULL) 
    s->addr = strdup(addr);
  return s;
}

static void udp_exit4(socket_udp *s)
{
	if (IN_MULTICAST(ntohl(s->addr4.s_addr))) {
#ifndef HAVE_IGMP_V3
		struct ip_mreq  imr;
		imr.imr_multiaddr.s_addr = s->addr4.s_addr;
		if (s->iface4_addr.s_addr != 0) {
		  imr.imr_interface.s_addr = s->iface4_addr.s_addr;
		} else {
		  imr.imr_interface.s_addr = INADDR_ANY;
		}
		if (SETSOCKOPT(s->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ip_mreq)) != 0) {
			socket_error("setsockopt IP_DROP_MEMBERSHIP");
			abort();
		}
#else
             rtp_message(LOG_DEBUG,"IGMPV3 leaving src:%s\n", G_Multicast_Src);
             /* Join Multicast group with source filter */
             struct ip_mreq_source imr;
	     imr.imr_multiaddr.s_addr = s->addr4.s_addr;
	     if (s->iface4_addr.s_addr != 0) {
		  imr.imr_interface.s_addr = s->iface4_addr.s_addr;
	     } else {
		  imr.imr_interface.s_addr = INADDR_ANY;
	     }
             if (inet_aton(G_Multicast_Src, &imr.imr_sourceaddr) == 0) {
                rtp_message(LOG_ERR, "inet_aton failed for %s\n",
                        G_Multicast_Src);
                abort();
              }
             if( setsockopt( s->fd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP,
                         (char*)&imr,
                         sizeof(struct ip_mreq_source) ) == -1 )
             {
		socket_error("setsockopt IP_DROP_SOURCE_MEMBERSHIP");
		//abort();
             }
#endif
		rtp_message(LOG_INFO,  "Dropped membership of multicast group");
	}
	closesocket(s->fd);
	if (s->addr != NULL)
	  free(s->addr);
	free(s);
}

static int udp_send4(socket_udp *s, const uint8_t *buffer, uint32_t buflen)
{
	struct sockaddr_in	s_in;
	
	ASSERT(s != NULL);
	ASSERT(s->mode == IPv4);
	ASSERT(buffer != NULL);
	ASSERT(buflen > 0);
	
	s_in.sin_family      = AF_INET;
	s_in.sin_addr.s_addr = s->addr4.s_addr;
	s_in.sin_port        = htons(s->tx_port);
	return sendto(s->fd, buffer, buflen, 0, (struct sockaddr *) &s_in, sizeof(s_in));
}

#ifdef HAVE_STRUCT_IOVEC
static int udp_send_iov4(socket_udp *s, struct iovec *iov, int count)
{
	struct sockaddr_in	s_in;
	struct msghdr 		msg;
	
	ASSERT(s != NULL);
	ASSERT(s->mode == IPv4);
	ASSERT(iov != NULL);
	ASSERT(count > 0);
	
	s_in.sin_family      = AF_INET;
	s_in.sin_addr.s_addr = s->addr4.s_addr;
	s_in.sin_port        = htons(s->tx_port);

	memset(&msg, 0, sizeof(msg));
	msg.msg_name 		 = (void *)&s_in;
	msg.msg_namelen 	 = sizeof(s_in);
	msg.msg_iov 		 = iov;
	msg.msg_iovlen 		 = count;

	return sendmsg(s->fd, &msg, 0);
}
#endif

static char *udp_host_addr4(void)
{
  char    		 hname[MAXHOSTNAMELEN];
  struct hostent 		*hent;
  struct in_addr  	 iaddr;
	
	if (gethostname(hname, MAXHOSTNAMELEN) != 0) {
		rtp_message(LOG_ERR, "Cannot get hostname!");
		abort();
	}
	hent = gethostbyname(hname);
	if (hent == NULL) {
		socket_error("Can't resolve IP address for %s", hname);
		return NULL;
	}
	ASSERT(hent->h_addrtype == AF_INET);
	memcpy(&iaddr.s_addr, hent->h_addr, sizeof(iaddr.s_addr));
	strncpy(hname, inet_ntoa(iaddr), MAXHOSTNAMELEN);
	return xstrdup(hname);
}

/*****************************************************************************/
/* IPv6 specific functions...                                                */
/*****************************************************************************/

static int udp_addr_valid6(const char *dst)
{
#ifdef HAVE_IPv6
        struct in6_addr addr6;
	switch (inet_pton(AF_INET6, dst, &addr6)) {
        case 1:  
                return TRUE;
                break;
        case 0: 
                return FALSE;
                break;
        case -1: 
                rtp_message(LOG_ERR, "inet_pton failed");
                errno = 0;
        }
#endif /* HAVE_IPv6 */
        UNUSED(dst);
        return FALSE;
}

static socket_udp *udp_init6(const char *addr, const char *iface, uint16_t rx_port, uint16_t tx_port, int ttl)
{
#ifdef HAVE_IPv6
  int                 reuse = 1;
  struct sockaddr_in6 s_in;
  socket_udp         *s = (socket_udp *) malloc(sizeof(socket_udp));
  s->mode    = IPv6;
  s->addr    = NULL;
  s->rx_port = rx_port;
  s->tx_port = tx_port;
  s->ttl     = ttl;
	
  if (iface != NULL) {
    debug_msg("Not yet implemented\n");
    abort();
  }

  if (addr != NULL) {
    if (inet_pton(AF_INET6, addr, &s->addr6) != 1) {
      /* We should probably try to do a DNS lookup on the name */
      /* here, but I'm trying to get the basics going first... */
      debug_msg("IPv6 address conversion failed\n");
      free(s);
      return NULL;	
    }
  }
  s->fd = socket(AF_INET6, SOCK_DGRAM, 0);
  if (s->fd < 0) {
    socket_error("socket");
    return NULL;
  }
  if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse)) != 0) {
    socket_error("setsockopt SO_REUSEADDR");
    return NULL;
  }
#ifdef SO_REUSEPORT
  if (SETSOCKOPT(s->fd, SOL_SOCKET, SO_REUSEPORT, (char *) &reuse, sizeof(reuse)) != 0) {
    socket_error("setsockopt SO_REUSEPORT");
    return NULL;
  }
#endif
	
  memset((char *)&s_in, 0, sizeof(s_in));
  s_in.sin6_family = AF_INET6;
  s_in.sin6_port   = htons(rx_port);
#ifdef HAVE_SIN6_LEN
  s_in.sin6_len    = sizeof(s_in);
#endif
  s_in.sin6_addr = in6addr_any;
  if (bind(s->fd, (struct sockaddr *) &s_in, sizeof(s_in)) != 0) {
    socket_error("bind");
    return NULL;
  }
	
  if (addr != NULL && IN6_IS_ADDR_MULTICAST(&(s->addr6))) {
    unsigned int      loop = 1;
    struct ipv6_mreq  imr;
#ifdef MUSICA_IPV6
    imr.i6mr_interface = 1;
    imr.i6mr_multiaddr = s->addr6;
#else
    imr.ipv6mr_multiaddr = s->addr6;
    imr.ipv6mr_interface = 0;
#endif
		
    if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char *) &imr, sizeof(imr)) != 0) {
      socket_error("setsockopt IPV6_ADD_MEMBERSHIP");
      return NULL;
    }
		
    if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *) &loop, sizeof(loop)) != 0) {
      socket_error("setsockopt IPV6_MULTICAST_LOOP");
      return NULL;
    }
    if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *) &ttl, sizeof(ttl)) != 0) {
      socket_error("setsockopt IPV6_MULTICAST_HOPS");
      return NULL;
    }
  }

  ASSERT(s != NULL);
  
  if (addr != NULL) {
    s->addr = strdup(addr);
  }
  return s;
#else
  UNUSED(addr);
  UNUSED(iface);
  UNUSED(rx_port);
  UNUSED(tx_port);
  UNUSED(ttl);
  return NULL;
#endif
}

static void udp_exit6(socket_udp *s)
{
#ifdef HAVE_IPv6
	if (IN6_IS_ADDR_MULTICAST(&(s->addr6))) {
		struct ipv6_mreq  imr;
#ifdef MUSICA_IPV6
		imr.i6mr_interface = 1;
		imr.i6mr_multiaddr = s->addr6;
#else
		imr.ipv6mr_multiaddr = s->addr6;
		imr.ipv6mr_interface = 0;
#endif
		
		if (SETSOCKOPT(s->fd, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (char *) &imr, sizeof(struct ipv6_mreq)) != 0) {
			socket_error("setsockopt IPV6_DROP_MEMBERSHIP");
			abort();
		}
	}
	closesocket(s->fd);
	if (s->addr != NULL) {
	  free(s->addr);
	}
	free(s);
#else
	UNUSED(s);
#endif  /* HAVE_IPv6 */
}

static int udp_send6(socket_udp *s, const uint8_t *buffer, uint32_t buflen)
{
#ifdef HAVE_IPv6
	struct sockaddr_in6	s_in;
	
	ASSERT(s != NULL);
	ASSERT(s->mode == IPv6);
	ASSERT(buffer != NULL);
	ASSERT(buflen > 0);
	
	memset((char *)&s_in, 0, sizeof(s_in));
	s_in.sin6_family = AF_INET6;
	s_in.sin6_addr   = s->addr6;
	s_in.sin6_port   = htons(s->tx_port);
#ifdef HAVE_SIN6_LEN
	s_in.sin6_len    = sizeof(s_in);
#endif
	return sendto(s->fd, buffer, buflen, 0, (struct sockaddr *) &s_in, sizeof(s_in));
#else
	UNUSED(s);
	UNUSED(buffer);
	UNUSED(buflen);
	return -1;
#endif
}

#ifdef HAVE_STRUCT_IOVEC
static int udp_send_iov6(socket_udp *s, struct iovec *iov, int count)
{
#ifdef HAVE_IPv6
	struct sockaddr_in6	s_in;
	struct msghdr msg;
	
	ASSERT(s != NULL);
	ASSERT(s->mode == IPv6);
	
	memset((char *)&s_in, 0, sizeof(s_in));
	s_in.sin6_family = AF_INET6;
	s_in.sin6_addr   = s->addr6;
	s_in.sin6_port   = htons(s->tx_port);
#ifdef HAVE_SIN6_LEN
	s_in.sin6_len    = sizeof(s_in);
#endif
	memset(&msg, 0, sizeof(msg));
	msg.msg_name 		 = &s_in;
	msg.msg_namelen 	 = sizeof(s_in);
	msg.msg_iov 		 = iov;
	msg.msg_iovlen 		 = count;
	/*
	  handled in memset, don't need HAVE_MSGHDR_MSGCTRL
	msg.msg_control 	 = NULL;
	msg.msg_controllen 	 = 0;
	msg.msg_flags 		 = 0;
	*/

	return sendmsg(s->fd, &msg, 0);
#else
	UNUSED(s);
	UNUSED(iov);
	UNUSED(count);
	return -1;
#endif
}
#endif

static char *udp_host_addr6(socket_udp *s)
{
#ifdef HAVE_IPv6
	char		 hname[MAXHOSTNAMELEN];
	int 			 gai_err, newsock;
	struct addrinfo 	 hints, *ai;
	struct sockaddr_in6 	 local, addr6;
	socklen_t len = sizeof(local);
	int result = 0;

	newsock=socket(AF_INET6, SOCK_DGRAM,0);
    memset ((char *)&addr6, 0, len);
    addr6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
    addr6.sin6_len    = len;
#endif
    bind (newsock, (struct sockaddr *) &addr6, len);
    addr6.sin6_addr = s->addr6;
    addr6.sin6_port = htons (s->rx_port);
    connect (newsock, (struct sockaddr *) &addr6, len);

    memset ((char *)&local, 0, len);
	if ((result = getsockname(newsock,(struct sockaddr *)&local, &len)) < 0){
		local.sin6_addr = in6addr_any;
		local.sin6_port = 0;
		debug_msg("getsockname failed\n");
	}

	closesocket (newsock);

	if (IN6_IS_ADDR_UNSPECIFIED(&local.sin6_addr) || IN6_IS_ADDR_MULTICAST(&local.sin6_addr)) {
		if (gethostname(hname, MAXHOSTNAMELEN) != 0) {
			debug_msg("gethostname failed\n");
			abort();
		}
		
		hints.ai_protocol  = 0;
		hints.ai_flags     = 0;
		hints.ai_family    = AF_INET6;
		hints.ai_socktype  = SOCK_DGRAM;
		hints.ai_addrlen   = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr      = NULL;
		hints.ai_next      = NULL;

		if ((gai_err = getaddrinfo(hname, NULL, &hints, &ai))) {
			debug_msg("getaddrinfo: %s: %s\n", hname, gai_strerror(gai_err));
			abort();
		}
		
		if (inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)(ai->ai_addr))->sin6_addr), hname, MAXHOSTNAMELEN) == NULL) {
			debug_msg("inet_ntop: %s: \n", hname);
			abort();
		}
		freeaddrinfo(ai);
		return xstrdup(hname);
	}
	if (inet_ntop(AF_INET6, &local.sin6_addr, hname, MAXHOSTNAMELEN) == NULL) {
		debug_msg("inet_ntop: %s: \n", hname);
		abort();
	}
	return xstrdup(hname);
#else  /* HAVE_IPv6 */
	UNUSED(s);
	return xstrdup("::");	/* The unspecified address... */
#endif /* HAVE_IPv6 */
}
	
/*****************************************************************************/
/* Generic functions, which call the appropriate protocol specific routines. */
/*****************************************************************************/

/**
 * udp_addr_valid:
 * @addr: string representation of IPv4 or IPv6 network address.
 *
 * Returns TRUE if @addr is valid, FALSE otherwise.
 **/

int udp_addr_valid(const char *addr)
{
        return udp_addr_valid4(addr) | udp_addr_valid6(addr);
}

/**
 * udp_init:
 * @addr: character string containing an IPv4 or IPv6 network address.
 * @rx_port: receive port.
 * @tx_port: transmit port.
 * @ttl: time-to-live value for transmitted packets.
 *
 * Creates a session for sending and receiving UDP datagrams over IP
 * networks. 
 *
 * Returns: a pointer to a valid socket_udp structure on success, NULL otherwise.
 **/
socket_udp *udp_init(const char *addr, uint16_t rx_port, uint16_t tx_port, int ttl)
{
	return udp_init_if(addr, NULL, rx_port, tx_port, ttl);
}

/**
 * udp_init_if:
 * @addr: character string containing an IPv4 or IPv6 network address.
 * @iface: character string containing an interface name.
 * @rx_port: receive port.
 * @tx_port: transmit port.
 * @ttl: time-to-live value for transmitted packets.
 *
 * Creates a session for sending and receiving UDP datagrams over IP
 * networks.  The session uses @iface as the interface to send and
 * receive datagrams on.
 * 
 * Return value: a pointer to a socket_udp structure on success, NULL otherwise.
 **/
socket_udp *udp_init_if(const char *addr, const char *iface, uint16_t rx_port, uint16_t tx_port, int ttl)
{
	socket_udp *res;
	
	if (strchr(addr, ':') == NULL) {
		res = udp_init4(addr, iface, rx_port, tx_port, ttl);
	} else {
		res = udp_init6(addr, iface, rx_port, tx_port, ttl);
	}
	return res;
}

socket_udp *udp_init_for_receive (const char *iface, uint16_t rx_port, int is_ipv4)
{
  if (is_ipv4) {
    return udp_init4(NULL, iface, rx_port, 0, 0);
  }
  return udp_init6(NULL, iface, rx_port, 0, 0);
}
/**
 * udp_exit:
 * @s: UDP session to be terminated.
 *
 * Closes UDP session.
 * 
 **/
void udp_exit(socket_udp *s)
{
    switch(s->mode) {
    case IPv4 : udp_exit4(s); break;
    case IPv6 : udp_exit6(s); break;
    default   : abort();
    }
}

/**
 * udp_send:
 * @s: UDP session.
 * @buffer: pointer to buffer to be transmitted.
 * @buflen: length of @buffer.
 * 
 * Transmits a UDP datagram containing data from @buffer.
 * 
 * Return value: 0 on success, -1 on failure.
 **/
int udp_send(socket_udp *s, const uint8_t *buffer, uint32_t buflen)
{
	switch (s->mode) {
	case IPv4 : return udp_send4(s, buffer, buflen);
	case IPv6 : return udp_send6(s, buffer, buflen);
	default   : abort(); /* Yuk! */
	}
	return -1;
}

int udp_sendto (socket_udp *s, const uint8_t *buffer, uint32_t buflen, 
		const struct sockaddr *to, const socklen_t tolen)
{
	ASSERT(s != NULL);
	ASSERT(buffer != NULL);
	ASSERT(buflen > 0);
	
	return sendto(s->fd, buffer, buflen, 0, to, tolen);
}

#ifdef HAVE_STRUCT_IOVEC
int udp_send_iov(socket_udp *s, struct iovec *iov, int count)
{
	switch (s->mode) {
	case IPv4 : return udp_send_iov4(s, iov, count);
	case IPv6 : return udp_send_iov6(s, iov, count);
	default   : abort();
	}
	return -1;
}

int udp_sendto_iov(socket_udp *s, struct iovec *iov, int count,
		   const struct sockaddr *to, const socklen_t tolen)
{
	struct msghdr 		msg;
	int ret;

	ASSERT(s != NULL);
	ASSERT(iov != NULL);
	ASSERT(count > 0);
	
	memset(&msg, 0, sizeof(msg));
	msg.msg_name 		 = (void *)to;
	msg.msg_namelen 	 = tolen;
	msg.msg_iov 		 = iov;
	msg.msg_iovlen 		 = count;

	ret = sendmsg(s->fd, &msg, 0);
	if (ret < 0) {
	  rtp_message(LOG_ERR, "sendmsg %d %s", 
		      errno, strerror(errno));
	}
	return ret;

}
#endif

/**
 * udp_recv:
 * @s: UDP session.
 * @buffer: buffer to read data into.
 * @buflen: length of @buffer.
 * 
 * Reads from datagram queue associated with UDP session.
 *
 * Return value: number of bytes read, returns 0 if no data is available.
 **/
uint32_t udp_recv_with_source (socket_udp *s, uint8_t *buffer, uint32_t buflen,
			       struct sockaddr *sock, socklen_t *socklen)
{
	/* Reads data into the buffer, returning the number of bytes read.   */
	/* If no data is available, this returns the value zero immediately. */
	/* Note: since we don't care about the source address of the packet  */
	/* we receive, this function becomes protocol independent.           */
	int		len;

	ASSERT(buffer != NULL);
	ASSERT(buflen > 0);

	len = recvfrom(s->fd, buffer, buflen, 0, sock, socklen);
	if (len > 0) {
		return len;
	}
	if (errno != ECONNREFUSED) {
		socket_error("recvfrom");
	}
	return 0;
}

uint32_t udp_recv(socket_udp *s, uint8_t *buffer, uint32_t buflen)
{
	/* Reads data into the buffer, returning the number of bytes read.   */
	/* If no data is available, this returns the value zero immediately. */
	/* Note: since we don't care about the source address of the packet  */
	/* we receive, this function becomes protocol independent.           */
  return udp_recv_with_source(s, buffer, buflen, NULL, 0);
}

struct udp_set_ {
  fd_set	rfd;
  fd_t	max_fd;
};

udp_set *udp_init_for_session (void)
{
  return (udp_set *)malloc(sizeof(udp_set));
}

void udp_close_session (udp_set *s)
{
  free((void *)s);
}
/**
 * udp_fd_zero:
 * 
 * Clears file descriptor from set associated with UDP sessions (see select(2)).
 * 
 **/
void udp_fd_zero(udp_set *session)
{
	FD_ZERO(&session->rfd);
	session->max_fd = 0;
}

/**
 * udp_fd_set:
 * @s: UDP session.
 * 
 * Adds file descriptor associated of @s to set associated with UDP sessions.
 **/
void udp_fd_set(udp_set *session, socket_udp *s)
{
	FD_SET(s->fd, &session->rfd);
	if (s->fd > (fd_t)session->max_fd) {
		session->max_fd = s->fd;
	}
}

/**
 * udp_fd_isset:
 * @s: UDP session.
 * 
 * Checks if file descriptor associated with UDP session is ready for
 * reading.  This function should be called after udp_select().
 *
 * Returns: non-zero if set, zero otherwise.
 **/
int udp_fd_isset(udp_set *session, socket_udp *s)
{
	return FD_ISSET(s->fd, &session->rfd);
}

/**
 * udp_select:
 * @timeout: maximum period to wait for data to arrive.
 * 
 * Waits for data to arrive for UDP sessions.
 * 
 * Return value: number of UDP sessions ready for reading.
 **/
int udp_select(udp_set *session, struct timeval *timeout)
{
	return select(session->max_fd + 1, &session->rfd, NULL, NULL, timeout);
}

/**
 * udp_host_addr:
 * @s: UDP session.
 * 
 * Return value: character string containing network address
 * associated with session @s.
 **/
char *udp_host_addr(socket_udp *s)
{
  if (s && s->mode == IPv6) {
    return udp_host_addr6(s);
  } 
  return udp_host_addr4();
}

/**
 * udp_fd:
 * @s: UDP session.
 * 
 * This function allows applications to apply their own socketopt()'s
 * and ioctl()'s to the UDP session.
 * 
 * Return value: file descriptor of socket used by session @s.
 **/
int udp_fd(socket_udp *s)
{
	if (s && s->fd > 0) {
		return s->fd;
	} 
	return 0;
}

void rtp_set_receive_buffer_default_size (int bufsize)
{
  have_recv_buf_size = 1;
  recv_buf_size_value = bufsize;
}

void udp_set_multicast_src(const char* src)
{
     //TODO - In future use a list and add more that one src
     if(src != 	NULL) {
         strncpy(G_Multicast_Src, src, sizeof(G_Multicast_Src));
         G_IGMP_V3 = 1;
     }
}
