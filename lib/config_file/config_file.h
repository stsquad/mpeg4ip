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

#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__ 1
#include "systems.h"

typedef enum {
  CONFIG_INT,
  CONFIG_STRING,
};


typedef struct config_variable_t {
  uint32_t config_index;
  const char *config_name;
  int config_type;
  int default_value;
  const char * default_string;
} config_variable_t;

class CConfig {
 public:
  CConfig(const config_variable_t *foo, uint32_t max);
  CConfig(const config_variable_t *foo, uint32_t max, const char *name);
  ~CConfig(void);
  int get_config_type(uint32_t cindex);
  int get_config_value(int cindex);
  int get_config_value(uint32_t cindex) { return get_config_value((int)cindex); };
  int get_config_default_value(uint32_t cindex);
  const char *get_config_string(uint32_t cindex);
  const char *get_config_default_string(uint32_t cindex);
  int read_config_file(const char *name = NULL);
#ifdef _WIN32
  int read_config_file(const char *reg_name, const char *config_section);
#endif
  void write_config_file(const char *name = NULL);
#ifdef _WIN32
  void write_config_file(const char *reg_name, const char *config_section);
#endif
  void set_config_value(uint32_t cindex, int value)
    {
      m_values[cindex] = value;
      m_changed = 1;
    };
  void set_config_string(uint32_t cindex, char *str)
    {
      if (m_strings[cindex] != NULL) free(m_strings[cindex]);
      m_strings[cindex] = str;
      m_changed = 1;
    };
  void move_config_strings(uint32_t dest_index, uint32_t from_index)
    {
      if (m_strings[dest_index] != NULL) free(m_strings[dest_index]);
      m_strings[dest_index] = m_strings[from_index];
      m_strings[from_index] = NULL;
      m_changed = 1;
    };
  int changed (void) { return m_changed; };
 private:
  void init(void);
  char *find_name (char *ptr, uint32_t &index);
  void get_default_name(char *buffer);
  const config_variable_t *m_config_var;
  uint32_t m_config_max;
  int *m_types;
  int *m_values;
  char **m_strings;
  int m_changed;

};

#endif
