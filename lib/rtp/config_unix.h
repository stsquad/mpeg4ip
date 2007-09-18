/*
 *  config-unix.h
 *
 *  Unix specific definitions and includes
 *  
 *  $Revision: 1.12 $
 *  $Date: 2007/09/18 20:52:01 $
 *
 * Copyright (c) 1995-2000 University College London
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

#ifndef _WIN32
#ifndef _CONFIG_UNIX_H
#define _CONFIG_UNIX_H
#include "mpeg4ip.h"
//#include "../../include/mpeg4ip_version.h"
#include <limits.h>
#include <sys/types.h>
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#include <sys/resource.h>

#include <pwd.h>
#include <signal.h>
#include <ctype.h>

#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>   /* abs() */
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#ifdef __APPLE__
#define NEED_DRAND48
#define srand48 srand
#define lrand48 rand() * rand
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif /* HAVE_STROPTS_H */

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>  
#endif /* HAVE_SYS_FILIO_H */

#ifdef HAVE_IPv6

#ifdef HAVE_NETINET6_IN6_H
/* Expect IPV6_{JOIN,LEAVE}_GROUP in in6.h, otherwise expect                 */
/* IPV_{ADD,DROP}_MEMBERSHIP in in.h                                         */
#include <netinet6/in6.h>
#else
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#else
#include <netinet/in.h>
#endif
#endif /* HAVE_NETINET_IN6_H */

#ifndef IPV6_ADD_MEMBERSHIP
#ifdef  IPV6_JOIN_GROUP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#else
#error  No definition of IPV6_ADD_MEMBERSHIP, please report to mm-tools@cs.ucl.ac.uk.
#endif /* IPV6_JOIN_GROUP     */
#endif /* IPV6_ADD_MEMBERSHIP */

#ifndef IPV6_DROP_MEMBERSHIP
#ifdef  IPV6_LEAVE_GROUP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#else
#error  No definition of IPV6_LEAVE_GROUP, please report to mm-tools@cs.ucl.ac.uk.
#endif  /* IPV6_LEAVE_GROUP     */
#endif  /* IPV6_DROP_MEMBERSHIP */

#endif /* HAVE_IPv6 */

#include <net/if.h>

#ifndef HAVE_SOCKLEN_T
typedef unsigned int socklen_t;
#endif

typedef u_char	ttl_t;
typedef int	fd_t;

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define	TRUE	1
#endif /* TRUE */

#define USERNAMELEN	8

#ifndef max
#define max(a, b)	(((a) > (b))? (a): (b))
#endif
#ifndef min
#define min(a, b)	(((a) < (b))? (a): (b))
#endif

#if 1
#define ASSERT(x) if ((x) == 0) fprintf(stderr, "%s:%u: failed assertion\n", __FILE__, __LINE__)
#else
#include <assert.h>
#define ASSERT assert
#endif

#ifdef Solaris
#ifdef __cplusplus
extern "C" {
#endif
int gettimeofday(struct timeval *tp, void * );
int gethostname(char *name, int namelen);
#ifdef __cplusplus
}
#endif
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif /* EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 7
#endif /* EXIT_FAILURE */

#endif /* _CONFIG_UNIX_H_ */
#endif /* WIN32 */
