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

typedef enum {
  CONFIG_INT,
  CONFIG_STRING,
};


typedef struct config_variable_t {
  size_t config_index;
  const char *config_name;
  int config_type;
  int default_value;
  const char * default_string;
} config_variable_t;

class CConfig {
 public:
  CConfig(const config_variable_t *foo, size_t max);
  CConfig(const config_variable_t *foo, size_t max, const char *name);
  ~CConfig(void);
  int get_config_type(size_t index);
  int get_config_value(int index);
  int get_config_value(size_t index) { return get_config_value((int)index); };
  int get_config_default_value(size_t index);
  const char *get_config_string(size_t index);
  const char *get_config_default_string(size_t index);
  int read_config_file(const char *name = NULL);
  void write_config_file(const char *name = NULL);
  void set_config_value(size_t index, int value)
    {
      m_values[index] = value;
      m_changed = 1;
    };
  void set_config_string(size_t index, char *str)
    {
      if (m_strings[index] != NULL) free(m_strings[index]);
      m_strings[index] = str;
      m_changed = 1;
    };
  void move_config_strings(size_t dest_index, size_t from_index)
    {
      if (m_strings[dest_index] != NULL) free(m_strings[dest_index]);
      m_strings[dest_index] = m_strings[from_index];
      m_strings[from_index] = NULL;
      m_changed = 1;
    };
 private:
  void init(void);
  char *find_name (char *ptr, size_t &index);
  void get_default_name(char *buffer);
  const config_variable_t *m_config_var;
  size_t m_config_max;
  int *m_types;
  int *m_values;
  char **m_strings;
  int m_changed;
};

#endif