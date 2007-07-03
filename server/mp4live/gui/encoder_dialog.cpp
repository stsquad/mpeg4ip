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
 * Copyright (C) Cisco Systems Inc. 2001-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#define __STDC_LIMIT_MACROS
#include "mp4live.h"
#include "mp4live_gui.h"
#include "encoder_gui_options.h"
#include "support.h"

// profile defines
static GtkWidget *EncoderSettingsDialog = NULL;


void
on_EncoderSettingsDialog_response         (GtkWidget       *dialog,
					  gint             response_id,
					  gpointer         user_data)
{
  GtkWidget *temp;
  CConfigEntry *pConfig = (CConfigEntry *)user_data;
  const char *str;
  if (response_id == GTK_RESPONSE_OK) {
    temp = lookup_widget(dialog, "EncoderTableData");
    encoder_gui_options_base_t **enc_settings = 
      (encoder_gui_options_base_t **)temp;
    uint enc_settings_count;
    temp = lookup_widget(dialog, "EncoderTableDataSize");
    enc_settings_count = GPOINTER_TO_INT(temp);
    for (uint ix = 0; ix < enc_settings_count; ix++) {
      encoder_gui_options_base_t *bptr = enc_settings[ix];
      char buffer[80];
      sprintf(buffer, "action%u", ix);
      GtkWidget *action = lookup_widget(dialog, buffer);
      switch (bptr->type) {
      case GUI_TYPE_BOOL:
	pConfig->SetBoolValue(*bptr->index,
			      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(action)));
	break;
      case GUI_TYPE_INT:
      case GUI_TYPE_INT_RANGE:
	pConfig->SetIntegerValue(*bptr->index,
				 gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(action)));
	break;
      case GUI_TYPE_FLOAT:
      case GUI_TYPE_FLOAT_RANGE:
	pConfig->SetFloatValue(*bptr->index,
			       gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(action)));
	break;
      case GUI_TYPE_STRING: 
	str = gtk_entry_get_text(GTK_ENTRY(action));
	if (str != NULL) {
	  ADV_SPACE(str);
	  if (*str == '\0') {
	    str = NULL;
	  }
	}
	pConfig->SetStringValue(*bptr->index, str);
	break;
      case GUI_TYPE_STRING_DROPDOWN: {
	encoder_gui_options_string_drop_t *sptr = 
	  (encoder_gui_options_string_drop_t *)bptr;
	uint inputIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(action));
	pConfig->SetStringValue(*bptr->index, sptr->options[inputIndex]);
      }
	
	break;
      }
    }
  } 
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static GtkWidget *CreateCheckbox (CConfigEntry *pConfig,
				  encoder_gui_options_base_t *bptr)
{
  GtkWidget *temp = gtk_check_button_new_with_mnemonic(bptr->label);
  gtk_widget_show(temp);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), 
			       pConfig->GetBoolValue(*bptr->index));
  return temp;
}

static GtkWidget *CreateInt (CConfigEntry *pConfig, 
			     encoder_gui_options_base_t *bptr)
{
  GtkObject *adj;
  GtkWidget *spin;
  if (bptr->type == GUI_TYPE_INT) {
    adj = gtk_adjustment_new(pConfig->GetIntegerValue(*bptr->index),
			     0, 1e+11, 1, 10, 10);
  } else {
    int page;
    encoder_gui_options_int_range_t *iptr =
      (encoder_gui_options_int_range_t *)bptr;
    page = iptr->max_range > 10 ? 10 : iptr->max_range / 2;
    adj = gtk_adjustment_new(pConfig->GetIntegerValue(*bptr->index),
			     iptr->min_range, iptr->max_range, 1, 
			     page, page);
  }
  spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
  gtk_widget_show(spin);
  return spin;
}
static GtkWidget *CreateFloat (CConfigEntry *pConfig, 
			       encoder_gui_options_base_t *bptr)
{
  GtkObject *adj;
  GtkWidget *spin;

  if (bptr->type == GUI_TYPE_FLOAT) {
    adj = gtk_adjustment_new(pConfig->GetFloatValue(*bptr->index),
			     0, 1e+11, .1, 1, 1);
  } else {
    encoder_gui_options_float_range_t *iptr =
      (encoder_gui_options_float_range_t *)bptr;
    adj = gtk_adjustment_new(pConfig->GetFloatValue(*bptr->index),
			     iptr->min_range, iptr->max_range, .1, 1, 1);
  }
  spin = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 4);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
  gtk_widget_show(spin);
  return spin;
}

static GtkWidget *CreateString (CConfigEntry *pConfig, 
				encoder_gui_options_base_t *bptr)
{
  GtkWidget *ret;
  ret = gtk_entry_new();
  const char *str = pConfig->GetStringValue(*bptr->index);
  if (str != NULL) {
    gtk_entry_set_width_chars(GTK_ENTRY(ret), strlen(str) + 5);
  }
  gtk_widget_show(ret);
  if (str != NULL)
    gtk_entry_set_text(GTK_ENTRY(ret), str);
  return ret;
}
static GtkWidget *CreateStringDropdown (CConfigEntry *pConfig, 
					encoder_gui_options_base_t *bptr)
{
  encoder_gui_options_string_drop_t *sptr;
  sptr = (encoder_gui_options_string_drop_t *)bptr;
  const char *exist_value = pConfig->GetStringValue(*bptr->index);
  uint exist_index = 0;
  for (uint ix = 0; ix < sptr->num_options; ix++) {
    if (strcasecmp(sptr->options[ix], exist_value) == 0) {
      exist_index = ix;
    }
  }

  GtkWidget *menu = gtk_option_menu_new();
  gtk_widget_show(menu);

  CreateOptionMenu(menu, sptr->options, sptr->num_options, exist_index);
  return menu;
}

void CreateEncoderSettingsDialog (CConfigEntry *config,
				  GtkWidget *calling_dialog,
				  const char *encoder_name,
				  encoder_gui_options_base_t **enc_settings,
				  uint enc_settings_count)
{
  GtkWidget *dialog_vbox4;
  GtkWidget *EncoderSettingsTable;
  GtkWidget *dialog_action_area3;
  GtkWidget *cancelbutton3;
  GtkWidget *okbutton3;
  GtkTooltips *tooltips;
  char buffer[128];

  tooltips = gtk_tooltips_new();

  EncoderSettingsDialog = gtk_dialog_new();
  sprintf(buffer, "%s Encoder Settings", encoder_name);
  gtk_window_set_title(GTK_WINDOW(EncoderSettingsDialog), buffer);
  gtk_window_set_modal(GTK_WINDOW(EncoderSettingsDialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(EncoderSettingsDialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(EncoderSettingsDialog), 
			       GTK_WINDOW(calling_dialog));
  gtk_window_set_type_hint(GTK_WINDOW(EncoderSettingsDialog), GDK_WINDOW_TYPE_HINT_DIALOG);

  dialog_vbox4 = GTK_DIALOG(EncoderSettingsDialog)->vbox;
  gtk_widget_show(dialog_vbox4);

  EncoderSettingsTable = gtk_table_new(enc_settings_count + 1, 2, FALSE);
  gtk_widget_show(EncoderSettingsTable);
  gtk_box_pack_start(GTK_BOX(dialog_vbox4), EncoderSettingsTable, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(EncoderSettingsTable), 3);
  GtkWidget *label = gtk_label_new(buffer);
  gtk_widget_show(label);
#define OUR_TABLE_OPTIONS (GTK_EXPAND | GTK_FILL)
  //#define OUR_TABLE_OPTIONS (GTK_FILL)
  gtk_table_attach(GTK_TABLE(EncoderSettingsTable), label, 
		   0, 2, 0, 1,
		   (GtkAttachOptions)(OUR_TABLE_OPTIONS),
		   (GtkAttachOptions)(OUR_TABLE_OPTIONS),
		   0, 0);
  GLADE_HOOKUP_OBJECT(EncoderSettingsDialog, label, "label");

  for (uint ix = 0; ix < enc_settings_count; ix++) {
    encoder_gui_options_base_t *bptr = enc_settings[ix];

    if (bptr->type != GUI_TYPE_BOOL) {
      label = gtk_label_new(bptr->label);
      gtk_widget_show(label);
      gtk_table_attach(GTK_TABLE(EncoderSettingsTable), label, 
		       0, 1, ix + 1, ix + 2,
		       (GtkAttachOptions)(OUR_TABLE_OPTIONS),
		       (GtkAttachOptions)(OUR_TABLE_OPTIONS),
		       0, 0);
      sprintf(buffer, "label%u", ix);
      GLADE_HOOKUP_OBJECT(EncoderSettingsDialog, label, buffer);
    } 
    GtkWidget *action = NULL;
    switch (bptr->type) {
    case GUI_TYPE_BOOL:
      action = CreateCheckbox(config, bptr);
      break;
    case GUI_TYPE_INT:
    case GUI_TYPE_INT_RANGE:
      action = CreateInt(config, bptr);
      break;
    case GUI_TYPE_FLOAT:
    case GUI_TYPE_FLOAT_RANGE:
      action = CreateFloat(config, bptr);
      break;
    case GUI_TYPE_STRING:
      action = CreateString(config, bptr);
      break;
    case GUI_TYPE_STRING_DROPDOWN:
      action = CreateStringDropdown(config, bptr);
      break;
    }
    if (action == NULL) error_message("action null type %u", bptr->type);
    if (action != NULL) {
      uint start = 1;
      if (bptr->type == GUI_TYPE_BOOL) start = 0;
      gtk_table_attach(GTK_TABLE(EncoderSettingsTable), action,
		       start, 2, ix + 1, ix + 2,
		       (GtkAttachOptions)(OUR_TABLE_OPTIONS),
		       (GtkAttachOptions)(OUR_TABLE_OPTIONS),
		       0, 0);
      sprintf(buffer, "action%u", ix);
      GLADE_HOOKUP_OBJECT(EncoderSettingsDialog, action, buffer);
    }
  }

  dialog_action_area3 = GTK_DIALOG(EncoderSettingsDialog)->action_area;
  gtk_widget_show(dialog_action_area3);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area3), GTK_BUTTONBOX_END);

  cancelbutton3 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton3);
  gtk_dialog_add_action_widget(GTK_DIALOG(EncoderSettingsDialog), cancelbutton3, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton3, GTK_CAN_DEFAULT);

  okbutton3 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton3);
  gtk_dialog_add_action_widget(GTK_DIALOG(EncoderSettingsDialog), okbutton3, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton3, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) EncoderSettingsDialog, "response",
                    G_CALLBACK(on_EncoderSettingsDialog_response),
                    config);
  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(EncoderSettingsDialog, EncoderSettingsDialog, "EncoderSettingsDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(EncoderSettingsDialog, dialog_vbox4, "dialog_vbox4");
  GLADE_HOOKUP_OBJECT(EncoderSettingsDialog, EncoderSettingsTable, "EncoderSettingsTable");
  GLADE_HOOKUP_OBJECT_NO_REF(EncoderSettingsDialog, dialog_action_area3, "dialog_action_area3");
  GLADE_HOOKUP_OBJECT(EncoderSettingsDialog, cancelbutton3, "cancelbutton3");
  GLADE_HOOKUP_OBJECT(EncoderSettingsDialog, okbutton3, "okbutton3");
  GLADE_HOOKUP_OBJECT_NO_REF(EncoderSettingsDialog, tooltips, "tooltips");
  GLADE_HOOKUP_OBJECT_NO_REF(EncoderSettingsDialog, enc_settings, "EncoderTableData");
  GLADE_HOOKUP_OBJECT_NO_REF(EncoderSettingsDialog, GINT_TO_POINTER(enc_settings_count), "EncoderTableDataSize");

  gtk_widget_show(EncoderSettingsDialog);
}

/* end encoder_dialog.cpp */
