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
 * player_mem_bytestream.c - provide a memory base bytestream
 */

#include "player_mem_bytestream.h"


CInByteStreamMem::CInByteStreamMem ()
{
  init();
}

CInByteStreamMem::~CInByteStreamMem()
{

}

void CInByteStreamMem::init()
{
  m_offset = 0;
  m_bookmark_set = 0;
}

char CInByteStreamMem::get (void)
{
  char ret;

  ret = *m_memptr++;
  m_offset++;

  if (m_offset > m_len && m_bookmark_set == 0) {
      throw "PAST END";
  } 

  return (ret);
}

char CInByteStreamMem::peek (void) 
{
  return (*m_memptr);
}

void CInByteStreamMem::bookmark (int bSet)
{
  if (bSet) {
    m_bookmark_set = 1;
    m_bookmark_offset = m_offset;
    m_bookmark_memptr = m_memptr;
  } else {
    m_bookmark_set = 0;
    m_memptr = m_bookmark_memptr;
    m_offset = m_bookmark_offset;
  }
}

/* end player_mem_bytestream.cpp */
