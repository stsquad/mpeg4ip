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

#include "bytestrm.hpp"

unsigned char CInByteStreamBase::get (void) 
{
  if (m_offset >= m_len) {
    if (m_bookmark_set) {
      (m_get_more)(m_ud, &m_memptr, &m_len, 
		   m_bookmark_offset, 0);
      m_offset -= m_bookmark_offset;
      m_bookmark_offset = 0;
    } else {
      (m_get_more)(m_ud, &m_memptr, &m_len,
		   m_offset, 1);
      m_offset = 0;
    }
  }
  unsigned char ret = m_memptr[m_offset];
  m_offset++;
  return (ret);
}

unsigned char CInByteStreamBase::peek (void)
{
  return (m_memptr[m_offset]);
}

void CInByteStreamBase::bookmark (int bSet)
{
  if (bSet) {
    m_bookmark_set = 1;
    m_bookmark_offset = m_offset;
  } else {
    m_bookmark_set = 0;
    m_offset = m_bookmark_offset;
  }
}

