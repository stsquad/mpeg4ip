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
 * config_file.cpp - a fairly simple config file reader.  It will read
 * either strings or ints out of .gmp4player_rc (or a specified file)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "our_config_file.h"
#include "player_util.h"

// Okay - this is an ugly global...

// Basic list of available config variables.  Future work could make this
// dynamic, but I'm not going to bother right now...

static config_variable_t configs[] = {
  { CONFIG_USE_MPEG4_ISO_ONLY, "Mpeg4IsoOnly", CONFIG_INT, 0, NULL },
  { CONFIG_PREV_FILE_0, "File0", CONFIG_STRING, 0, NULL },
  { CONFIG_PREV_FILE_1, "File1", CONFIG_STRING, 0, NULL },
  { CONFIG_PREV_FILE_2, "File2", CONFIG_STRING, 0, NULL },
  { CONFIG_PREV_FILE_3, "File3", CONFIG_STRING, 0, NULL },
};

CConfig config(configs, CONFIG_MAX);
/* end file config_file.cpp */
