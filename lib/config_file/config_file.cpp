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
#include "systems.h"
#include "config_file.h"
// Basic list of available config variables.  Future work could make this
// dynamic, but I'm not going to bother right now...

CConfig::CConfig(const config_variable_t *config, uint32_t max)
{
  m_config_var = config;
  m_config_max = max;
  init();
}

CConfig::CConfig (const config_variable_t *config, 
		  uint32_t max, 
		  const char *name)
{
  m_config_var = config;
  m_config_max = max;
  init();
  read_config_file(name);
  m_changed = 0;
}

void CConfig::init (void)
{
  m_types = (int *)malloc(sizeof(int) * m_config_max);
  m_values = (int *)malloc(sizeof(int) * m_config_max);
  m_strings = (char **)malloc(sizeof(char *) * m_config_max);

  for (uint32_t ix = 0; ix < m_config_max; ix++) {
    int index = m_config_var[ix].config_index;
    m_types[index] = m_config_var[ix].config_type;
    m_values[index] = m_config_var[ix].default_value;
    if (m_config_var[ix].default_string != NULL) {
      m_strings[index] = strdup(m_config_var[ix].default_string);
    } else {
      m_strings[index] = NULL;
    }
  }
  m_changed = 0;
}

CConfig::~CConfig(void)
{
  if (m_changed != 0) {
    write_config_file();
  }
  for (uint32_t ix = 0; ix < m_config_max; ix++) {
    if (m_strings[ix] != NULL) {
      free(m_strings[ix]);
      m_strings[ix] = NULL;
    }
  }
  free(m_types);
  m_types = NULL;
  free(m_strings);
  m_strings = NULL;
  free(m_values);
  m_values = NULL;
}

int CConfig::get_config_type (uint32_t index)
{
  if (index < 0 || index >= m_config_max)
    return (-1);
  return (m_types[index]);
}

int CConfig::get_config_value (int ix)
{
  uint32_t index = ix;
  if (index < 0 || index >= m_config_max)
    return (-1);
  if (m_types[index] != CONFIG_INT)
    return (-1);
  return (m_values[index]);
}

int CConfig::get_config_default_value (uint32_t index)
{
  for (uint32_t ix = 0; ix < m_config_max; ix++) {
    if (index == m_config_var[ix].config_index) {
      if (m_config_var[ix].config_type == CONFIG_INT)
	return (m_config_var[ix].default_value);
      else
	return (-1);
    }
  }
  return (-1);
}

const char *CConfig::get_config_string (uint32_t index)
{
  if (index < 0 || index >= m_config_max)
    return (NULL);
  if (m_types[index] != CONFIG_STRING)
    return (NULL);
  return (m_strings[index]);
}
		
const char *CConfig::get_config_default_string (uint32_t index)
{
  for (uint32_t ix = 0; ix < m_config_max; ix++) {
    if (index == m_config_var[ix].config_index) {
      if (m_config_var[ix].config_type == CONFIG_STRING)
	return (m_config_var[ix].default_string);
      else
	return (NULL);
    }
  }
  return (NULL);
}

void CConfig::get_default_name (char *buffer) 
{
  char *home = getenv("HOME");
  if (home == NULL) {
#ifdef _WIN32
	strcpy(buffer, "gmp4player_rc");
#else
    strcpy(buffer, ".gmp4player_rc");
#endif
  } else {
    strcpy(buffer, home);
    strcat(buffer, "/.gmp4player_rc");
  }
}

#define ADV_SPACE(a) {while (isspace(*(a)) && (*(a) != '\0'))(a)++;}

char *CConfig::find_name (char *ptr, uint32_t &ix)
{
  for (ix = 0; ix < m_config_max; ix++) {
    uint32_t len;
    len = strlen(m_config_var[ix].config_name);
    if (strncasecmp(ptr, 
		    m_config_var[ix].config_name, 
		    len) == 0) {
      ptr += len;
      return (ptr);
    }
  }
  return (NULL);
}
  
int CConfig::read_config_file (const char *name)
{
  FILE *ifile;

  char buffer[1024];
  if (name == NULL) {
    get_default_name(buffer);
    name = buffer;
  } 
  
  ifile = fopen(name, "r");
  if (ifile == NULL) {
    printf("File %s not found", name);
    return (-1);
  }

  while (fgets(buffer, sizeof(buffer), ifile) != NULL) {
    char *ptr = buffer;
    while (*ptr != '\0') {
      ptr++;
    }
    ptr--;
    while (isspace(*ptr)) { 
      *ptr = '\0';
      ptr--;
    }
    ptr = buffer;
    ADV_SPACE(ptr);
    if (ptr != '\0') {
      uint32_t index;
      ptr = find_name(ptr, index);
      if (ptr != NULL) {
	ADV_SPACE(ptr);
	if (*ptr == '=') ptr++;
	ADV_SPACE(ptr);
	if (m_config_var[index].config_type == CONFIG_INT) {
	  set_config_value(m_config_var[index].config_index,
			   atoi(ptr));
	} else {
	  set_config_string(m_config_var[index].config_index,
			    strdup(ptr));
	}
      }
    }
  }
  fclose(ifile);
  m_changed = 0; // since we only read it, don't indicate we need to set it...
  return (0);
}

void CConfig::write_config_file (const char *name)
{
  FILE *ofile;
  char buffer[1024];
  if (name == NULL) {
    get_default_name(buffer);
    name = buffer;
  } 
  
  ofile = fopen(name, "w");
  for (uint32_t ix = 0; ix < m_config_max; ix++) {
    uint32_t index = m_config_var[ix].config_index;
    if (m_config_var[ix].config_type == CONFIG_INT) {
      if (m_config_var[ix].default_value != m_values[index]) {
	fprintf(ofile, 
		"%s=%d\n", 
		m_config_var[ix].config_name, 
		m_values[index]);
      }
    } else {
      if (m_strings[index] != NULL) {
	if (m_config_var[ix].default_string == NULL || 
	    strcasecmp(m_config_var[ix].default_string, m_strings[index]) != 0) {
	  fprintf(ofile, "%s=%s\n", m_config_var[ix].config_name, m_strings[index]);
	}
      }
    }
  }
  fclose(ofile);
  m_changed = 0;
}

/* end file config_file.cpp */
