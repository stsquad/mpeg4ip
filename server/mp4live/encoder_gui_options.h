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

#ifndef __ENCODER_GUI_OPTIONS_H__
#define __ENCODER_GUI_OPTIONS_H__ 1

typedef enum {
  GUI_TYPE_BOOL,
  GUI_TYPE_INT, 
  GUI_TYPE_INT_RANGE,
  GUI_TYPE_FLOAT,
  GUI_TYPE_FLOAT_RANGE,
  GUI_TYPE_STRING,
  GUI_TYPE_STRING_DROPDOWN
} gui_types;


typedef struct encoder_gui_options_base_t {
  gui_types type;
  config_index_t *index;
  const char *label;
} encoder_gui_options_base_t;

typedef struct encoder_gui_options_t {
  encoder_gui_options_base_t base;
} encoder_gui_options_t;

typedef struct encoder_gui_options_int_range_t {
  encoder_gui_options_base_t base;
  uint32_t min_range;
  uint32_t max_range;
} encoder_gui_options_int_range_t;

typedef struct encoder_gui_options_float_range_t {
  encoder_gui_options_base_t base;
  float min_range;
  float max_range;
} encoder_gui_options_float_range_t;

typedef struct encoder_gui_options_string_drop_t {
  encoder_gui_options_base_t base;
  const char **options;
  uint num_options;
} encoder_gui_options_string_drop_t;

typedef bool (*get_gui_options_list_f)(encoder_gui_options_base_t ***value,
				     uint *count);
#define GUI_BOOL(name, config_name, label) \
  static encoder_gui_options_t (name) = {{GUI_TYPE_BOOL, &config_name, label}};
  
#define GUI_INT(name, config_name, label) \
  static encoder_gui_options_t (name) = {{GUI_TYPE_INT, &config_name, label}};

#define GUI_FLOAT(name, config_name, label) \
  static encoder_gui_options_t (name) = {{GUI_TYPE_FLOAT, &config_name, label}};

#define GUI_STRING(name, config_name, label) \
  static encoder_gui_options_t (name) = {{GUI_TYPE_STRING, &config_name, label}};

#define GUI_INT_RANGE(name, config_name, label, min, max) \
  static encoder_gui_options_int_range_t (name) = {{GUI_TYPE_INT_RANGE, &config_name, label}, min, max };
#define GUI_FLOAT_RANGE(name, config_name, label, min, max) \
  static encoder_gui_options_float_range_t (name) = {{GUI_TYPE_FLOAT_RANGE, &config_name, label}, min, max };

#define GUI_STRING_DROPDOWN(name, config_name, label, string_list, count) \
  static encoder_gui_options_string_drop_t (name) = {{GUI_TYPE_STRING_DROPDOWN, &config_name, label}, string_list, count };

#define EXTERN_TABLE_F(name)  extern bool name##_f(encoder_gui_options_base_t ***value, uint *count);

#define DECLARE_TABLE(name) static encoder_gui_options_base_t *name[]
#define DECLARE_TABLE_FUNC(name) \
bool name##_f (encoder_gui_options_base_t ***value, uint *count) \
{ \
   if (value != NULL) *value = (encoder_gui_options_base_t **)&name; \
   if (count != NULL) *count = NUM_ELEMENTS_IN_ARRAY(name); \
   return true; \
}\

#define TABLE_GUI(name) &name.base
#define TABLE_FUNC(name) name##_f
#endif
