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
 * ip_port.h - provides a method of getting and reserving IP ports until
 * ready to use them.  Since RTP needs consecutive even-odd port numbers
 * for each stream, this will "hold" weird odd ports.
 */
#ifndef __IP_PORT_H__
#define __IP_PORT_H__ 1

// CIpPort will grab the next ip port available when created (and valid).
// It will hold on to the port until deleted.
class CIpPort {
 public:
  CIpPort(in_port_t startport, in_port_t endport);
  ~CIpPort();
  CIpPort *get_next (void) { return m_next;};
  void set_next (CIpPort *p) { m_next = p;};
  in_port_t get_port_num (void) { return m_port_num;};
  int valid(void) { return (m_sock == -1 ? 0 : 1);};
 private:
  CIpPort *m_next;
  int m_sock;
  in_port_t m_port_num;
};

// C2ConsecIpPort will attempt to save 2 consecutive ip ports (starting with
// an even port number).  It needs to be passed a pointer to a list of
// invalid ip ports.  This is to keep sockets open for those ports.
class C2ConsecIpPort {
 public:
  C2ConsecIpPort(CIpPort **global, in_port_t start_port = 0);
  ~C2ConsecIpPort(void);
  int valid (void) { return m_first != NULL && m_second != NULL; };
  in_port_t first_port (void) { return m_first->get_port_num(); };
  in_port_t second_port (void) { return m_second->get_port_num(); };
 private:
  CIpPort *m_first, *m_second;
  in_port_t m_start_port;
};

#endif
