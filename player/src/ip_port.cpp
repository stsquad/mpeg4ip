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
#include "mpeg4ip.h"
#include "ip_port.h"
#include "player_util.h"
#include "our_config_file.h"

/*
 * CIpPort::CIpPort() - create ip socket, bind it to the local address
 * (which assigns it a port number), and figure out what the port number
 * is.
 */
CIpPort::CIpPort (in_port_t startport, in_port_t maxport)
{
  struct sockaddr_in saddr;
  
  m_next = NULL;

  for (m_sock = -1; m_sock == -1 && startport <= maxport; startport++) {
    // Start with min, go to max
    m_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sock < 0) {
      return;
    }
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(startport);
    if (bind(m_sock, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {

      startport++;
      closesocket(m_sock);
      m_sock = -1;
    }
  } 

  socklen_t socklen;
  socklen = sizeof(saddr);
  if (getsockname(m_sock, (struct sockaddr *)&saddr, &socklen) < 0) {
    closesocket(m_sock);
    m_sock = -1;
    return;
  }

  m_port_num = ntohs(saddr.sin_port);
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
C2ConsecIpPort::C2ConsecIpPort (CIpPort **global, in_port_t start_port)
{
  CIpPort *newone;
  m_first = m_second = NULL;
  in_port_t firstport, maxport;

  maxport = (in_port_t)~0;
  if (start_port == 0) {
    firstport = 1024;
    if (config.get_config_value(CONFIG_IPPORT_MIN) != UINT32_MAX) {
      firstport = config.get_config_value(CONFIG_IPPORT_MIN);
      if (config.get_config_value(CONFIG_IPPORT_MAX) != UINT32_MAX) {
	maxport = config.get_config_value(CONFIG_IPPORT_MAX);
	if (maxport <= firstport) {
	  player_error_message("IP port configuration error - %u %u - using 65535 as max port value", 
			       firstport, maxport);
	  maxport = (in_port_t)~0;
	}
      }
    }
  } else {
    firstport = start_port;
  }
  while (1) {
    // first, get an even port number.  If not even, save it in the
    // global queue.
    do {
      newone = new CIpPort(firstport, maxport);
      if (newone->valid() == 0)
	return;
      if ((newone->get_port_num() & 0x1) == 0x1) {
	newone->set_next(*global);
	*global = newone;
	firstport++;
      }
    } while ((newone->get_port_num() & 0x1) == 0x1);

    player_debug_message("First port is %d", newone->get_port_num());

    // Okay, save the first, get the 2nd.  If okay, just return
    m_first = newone;
    in_port_t next;
    next = m_first->get_port_num() + 1;
    m_second = new CIpPort(next, next);
    if ((m_second->valid() == 1) &&
	(m_second->get_port_num() == next)) {
      player_debug_message("Ip ports are %u %u", next - 1, next);
      return;
    } else {
      player_debug_message("Got port %d invalid %d", m_second->get_port_num(),
			   m_second->valid());
    }
    // Not okay - save both off in the global queue, and try again...
    m_first->set_next(*global);
    *global = m_first;
    m_first = NULL;
    firstport = m_second->get_port_num() + 1;
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
