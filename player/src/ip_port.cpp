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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * ip_port.cpp - reserve IP port numbers for audio/video RTP streams
 */
#include "systems.h"
#include "ip_port.h"

/*
 * CIpPort::CIpPort() - create ip socket, bind it to the local address
 * (which assigns it a port number), and figure out what the port number
 * is.
 */
CIpPort::CIpPort (void)
{
  struct sockaddr_in sin;
  
  m_sock = -1;
  m_port_num = 0;
  m_next = NULL;

  m_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (m_sock < 0) {
    return;
  }

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = 0;

  if (bind(m_sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    closesocket(m_sock);
    m_sock = -1;
    return;
  }

#if !defined(HAVE_SOCKLEN_T) || defined(_WIN32)
  int socklen;
#else
  socklen_t socklen;
#endif
  socklen = sizeof(sin);
  if (getsockname(m_sock, (struct sockaddr *)&sin, &socklen) < 0) {
    closesocket(m_sock);
    m_sock = -1;
    return;
  }

  m_port_num = ntohs(sin.sin_port);
}

/*
 * CIpPort::~CIpPort() - close the socket we opened.
 */
CIpPort::~CIpPort (void)
{
  if (m_sock != -1) {
    closesocket(m_sock);
    m_sock = -1;
  }
}

/*
 * C2ConsecIpPort::C2ConsecIpPort() - get 2 consecutive, even-odd, ip
 * port numbers
 */
C2ConsecIpPort::C2ConsecIpPort (CIpPort **global)
{
  CIpPort *newone;
  m_first = m_second = NULL;

  while (1) {
    // first, get an even port number.  If not even, save it in the
    // global queue.
    do {
      newone = new CIpPort();
      if (newone->valid() == 0)
	return;
      if ((newone->get_port_num() & 0x1) == 0x1) {
	newone->set_next(*global);
	*global = newone;
      }
    } while ((newone->get_port_num() & 0x1) == 0x1);

    // Okay, save the first, get the 2nd.  If okay, just return
    m_first = newone;
    m_second = new CIpPort();
    if ((m_second->valid() == 1) &&
	(m_second->get_port_num() == (m_first->get_port_num() + 1))) {
      return;
    }
    // Not okay - save both off in the global queue, and try again...
    m_first->set_next(*global);
    *global = m_first;
    m_first = NULL;
    m_second->set_next(*global);
    *global = m_second;
    m_second = NULL;
  }
}

/*
 * C2ConsecIpPort::~C2ConsecIpPort() - just delete the CIpPorts that
 * we've saved.
 */
C2ConsecIpPort::~C2ConsecIpPort (void)
{
  if (m_second) {
    delete m_second;
    m_second = NULL;
  }
  if (m_first) {
    delete m_first;
    m_first = NULL;
  }

}
