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
 * Copyright (C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May 		wmay@cisco.com
 */
#ifndef __PROFILE_TEXT__
#define __PROFILE_TEXT__ 1
#include "config_list.h"

#define TEXT_ENCODING_PLAIN "plain"
#define TEXT_ENCODING_HREF  "href"


DECLARE_CONFIG(CFG_TEXT_PROFILE_NAME);
DECLARE_CONFIG(CFG_TEXT_ENCODING);
DECLARE_CONFIG(CFG_TEXT_REPEAT_TIME_SECS);
DECLARE_CONFIG(CFG_TEXT_HREF_MAKE_AUTOMATIC);

#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable TextProfileConfigVariables[] = {
  CONFIG_STRING(CFG_TEXT_PROFILE_NAME, "name", NULL),
  CONFIG_STRING(CFG_TEXT_ENCODING, "textEncoding", TEXT_ENCODING_HREF),
  CONFIG_FLOAT(CFG_TEXT_REPEAT_TIME_SECS, "textRepeatTime", 1.0),
  CONFIG_BOOL(CFG_TEXT_HREF_MAKE_AUTOMATIC, "hrefMakeAutomatic", true),
};
#endif

class CTextProfile : public CConfigEntry
{
 public:
  CTextProfile(const char *filename, CConfigEntry *next) :
    CConfigEntry(filename, "text", next) {
  };
  ~CTextProfile(void) { };
  void LoadConfigVariables(void);
};

class CTextProfileList : public CConfigList
{
  public:
  CTextProfileList(const char *directory) :
    CConfigList(directory, "text") {
  };
  
  ~CTextProfileList(void) {
  };
  CTextProfile *FindProfile(const char *name) {
    return (CTextProfile *)FindConfigInt(name);
  };
 protected:
  CConfigEntry *CreateConfigInt(const char *fname, CConfigEntry *next) {
    CTextProfile *ret = new CTextProfile(fname, next);
    ret->LoadConfigVariables();
    return ret;
  };
};

#endif
