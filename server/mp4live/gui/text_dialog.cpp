/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1(the "License"); you may not use this file
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
 * Copyright(C) Cisco Systems Inc. 2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Bill May 		wmay@cisco.com
 */

#define __STDC_LIMIT_MACROS
#include "mp4live.h"
#include "mp4live_gui.h"
#include "support.h"
#include "text_source.h"
#include "text_encoder.h"
#include "profile_text.h"

static void  EnableFromTextMenu (GtkWidget *dialog, 
				 uint index)
{
  bool enable = index != 0;

  GtkWidget *wid;
  wid = lookup_widget(dialog, "TextFileEntry");
  gtk_widget_set_sensitive(wid, enable);
  wid = lookup_widget(dialog, "FileBrowseButton");
  gtk_widget_set_sensitive(wid, enable);
}

static void
on_TextSourceDialog_response           (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  if (GTK_RESPONSE_OK == response_id) {
    GtkWidget *temp;

    temp = lookup_widget(GTK_WIDGET(dialog), "TextSourceMenu");
    uint index = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));
    const char *type = NULL;
    switch (index) {
    case 0: type = TEXT_SOURCE_DIALOG; break;
    case 1: type = TEXT_SOURCE_TIMED_FILE; break;
    case 2: type = TEXT_SOURCE_FILE_WITH_DIALOG; break;
    }
    if (type != NULL) {
      MyConfig->SetStringValue(CONFIG_TEXT_SOURCE_TYPE, type);
      if (index != 0) {
	temp = lookup_widget(GTK_WIDGET(dialog), "TextFileEntry");
	MyConfig->SetStringValue(CONFIG_TEXT_SOURCE_FILE_NAME, 
				 gtk_entry_get_text(GTK_ENTRY(temp)));
      }
      MainWindowDisplaySources();
    }
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void
on_TextSourceMenu_changed              (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  EnableFromTextMenu(dialog, gtk_option_menu_get_history(optionmenu));
}




static void
on_FileBrowseButton_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  GtkWidget *entry = lookup_widget(dialog, "TextFileEntry");
  FileBrowser(entry, dialog, NULL);
}

void create_TextSourceDialog (void)
{
  GtkWidget *TextSourceDialog;
  GtkWidget *dialog_vbox10;
  GtkWidget *table7;
  GtkWidget *label193;
  GtkWidget *TextSourceMenu;
  GtkWidget *menu17;
  GtkWidget *text_entry_dialog1;
  GtkWidget *timed_from_file1;
  GtkWidget *file_with_queued_dialog1;
  GtkWidget *label194;
  GtkWidget *hbox103;
  GtkWidget *TextFileEntry;
  GtkWidget *FileBrowseButton;
  GtkWidget *alignment27;
  GtkWidget *hbox104;
  GtkWidget *image33;
  GtkWidget *label195;
  GtkWidget *dialog_action_area9;
  GtkWidget *cancelbutton7;
  GtkWidget *okbutton9;

  TextSourceDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(TextSourceDialog), _("Text Source"));
  gtk_window_set_modal(GTK_WINDOW(TextSourceDialog), TRUE);

  dialog_vbox10 = GTK_DIALOG(TextSourceDialog)->vbox;
  gtk_widget_show(dialog_vbox10);

  table7 = gtk_table_new(2, 2, FALSE);
  gtk_widget_show(table7);
  gtk_box_pack_start(GTK_BOX(dialog_vbox10), table7, TRUE, TRUE, 2);
  gtk_table_set_row_spacings(GTK_TABLE(table7), 9);
  gtk_table_set_col_spacings(GTK_TABLE(table7), 17);

  label193 = gtk_label_new(_("Text Source:"));
  gtk_widget_show(label193);
  gtk_table_attach(GTK_TABLE(table7), label193, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label193), 0, 0.5);

  TextSourceMenu = gtk_option_menu_new();
  gtk_widget_show(TextSourceMenu);
  gtk_table_attach(GTK_TABLE(table7), TextSourceMenu, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  menu17 = gtk_menu_new();

  text_entry_dialog1 = gtk_menu_item_new_with_mnemonic(_("Dialog with Text Entry"));
  gtk_widget_show(text_entry_dialog1);
  gtk_container_add(GTK_CONTAINER(menu17), text_entry_dialog1);

  timed_from_file1 = gtk_menu_item_new_with_mnemonic(_("File with Timing Info"));
  gtk_widget_show(timed_from_file1);
  gtk_container_add(GTK_CONTAINER(menu17), timed_from_file1);

  file_with_queued_dialog1 = gtk_menu_item_new_with_mnemonic(_("Dialog Using File"));
  gtk_widget_show(file_with_queued_dialog1);
  gtk_container_add(GTK_CONTAINER(menu17), file_with_queued_dialog1);

  gtk_option_menu_set_menu(GTK_OPTION_MENU(TextSourceMenu), menu17);

  label194 = gtk_label_new(_("File:"));
  gtk_widget_show(label194);
  gtk_table_attach(GTK_TABLE(table7), label194, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label194), 0, 0.5);

  hbox103 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox103);
  gtk_table_attach(GTK_TABLE(table7), hbox103, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(GTK_FILL), 0, 0);

  TextFileEntry = gtk_entry_new();
  gtk_widget_show(TextFileEntry);
  gtk_box_pack_start(GTK_BOX(hbox103), TextFileEntry, TRUE, TRUE, 0);

  FileBrowseButton = gtk_button_new();
  gtk_widget_show(FileBrowseButton);
  gtk_box_pack_start(GTK_BOX(hbox103), FileBrowseButton, FALSE, FALSE, 0);

  alignment27 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment27);
  gtk_container_add(GTK_CONTAINER(FileBrowseButton), alignment27);

  hbox104 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox104);
  gtk_container_add(GTK_CONTAINER(alignment27), hbox104);

  image33 = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image33);
  gtk_box_pack_start(GTK_BOX(hbox104), image33, FALSE, FALSE, 0);

  label195 = gtk_label_new_with_mnemonic(_("Browse"));
  gtk_widget_show(label195);
  gtk_box_pack_start(GTK_BOX(hbox104), label195, FALSE, FALSE, 0);

  dialog_action_area9 = GTK_DIALOG(TextSourceDialog)->action_area;
  gtk_widget_show(dialog_action_area9);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area9), GTK_BUTTONBOX_END);

  cancelbutton7 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton7);
  gtk_dialog_add_action_widget(GTK_DIALOG(TextSourceDialog), cancelbutton7, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton7, GTK_CAN_DEFAULT);

  okbutton9 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton9);
  gtk_dialog_add_action_widget(GTK_DIALOG(TextSourceDialog), okbutton9, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton9, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) TextSourceDialog, "response",
                    G_CALLBACK(on_TextSourceDialog_response),
                    NULL);
  g_signal_connect((gpointer) TextSourceMenu, "changed",
                    G_CALLBACK(on_TextSourceMenu_changed),
                    TextSourceDialog);
  g_signal_connect((gpointer) FileBrowseButton, "clicked",
                    G_CALLBACK(on_FileBrowseButton_clicked),
                    TextSourceDialog);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(TextSourceDialog, TextSourceDialog, "TextSourceDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(TextSourceDialog, dialog_vbox10, "dialog_vbox10");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, table7, "table7");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, label193, "label193");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, TextSourceMenu, "TextSourceMenu");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, menu17, "menu17");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, text_entry_dialog1, "text_entry_dialog1");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, timed_from_file1, "timed_from_file1");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, file_with_queued_dialog1, "file_with_queued_dialog1");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, label194, "label194");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, hbox103, "hbox103");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, TextFileEntry, "TextFileEntry");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, FileBrowseButton, "FileBrowseButton");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, alignment27, "alignment27");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, hbox104, "hbox104");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, image33, "image33");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, label195, "label195");
  GLADE_HOOKUP_OBJECT_NO_REF(TextSourceDialog, dialog_action_area9, "dialog_action_area9");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, cancelbutton7, "cancelbutton7");
  GLADE_HOOKUP_OBJECT(TextSourceDialog, okbutton9, "okbutton9");

  const char *type = MyConfig->GetStringValue(CONFIG_TEXT_SOURCE_TYPE);
  uint index = 0;
  if (strcmp(type, TEXT_SOURCE_TIMED_FILE) == 0) {
    index = 1;
  } else if (strcmp(type, TEXT_SOURCE_DIALOG) == 0) {
    index = 0;
  } else if (strcmp(type, TEXT_SOURCE_FILE_WITH_DIALOG) == 0) {
    index = 2;
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(TextSourceMenu), index);
  const char *file = MyConfig->GetStringValue(CONFIG_TEXT_SOURCE_FILE_NAME);
  if (file != NULL) {
    gtk_entry_set_text(GTK_ENTRY(TextFileEntry), file);
  }
  EnableFromTextMenu(TextSourceDialog, index);
  gtk_widget_show(TextSourceDialog);
}

/*************************************************************************
 * Text profile dialog
 *************************************************************************/
static const char **encoderNames = NULL;

static GtkWidget *TextProfileDialog;
void
on_TextProfileEncoder_changed          (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  uint32_t encoderIndex = gtk_option_menu_get_history(optionmenu);

  bool set;
  set = text_encoder_table[encoderIndex].get_gui_options != NULL;
  if (set) {
    set = (text_encoder_table[encoderIndex].get_gui_options)(NULL, NULL);
  }
  GtkWidget *temp = lookup_widget(TextProfileDialog, "TextEncoderSettingsButton");
  gtk_widget_set_sensitive(temp, set);
}


void
on_TextEncoderSettingsButton_clicked   (GtkButton       *button,
                                        gpointer         user_data)
{
  CTextProfile *profile = (CTextProfile *)user_data;
  GtkWidget *temp;
  temp = lookup_widget(TextProfileDialog, "TextProfileEncoder");
  encoder_gui_options_base_t **settings_array;
  uint settings_array_count;
  uint index = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));
  
  if ((text_encoder_table[index].get_gui_options)(&settings_array, &settings_array_count) == false) {
    return;
  }
  
  CreateEncoderSettingsDialog(profile, TextProfileDialog,
			      text_encoder_table[index].dialog_selection_name,
			      settings_array,
			      settings_array_count);
}

static void
on_TextProfileDialog_response          (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  GtkWidget *wid;
  CTextProfile *ret_profile = NULL;
  if (GTK_RESPONSE_OK == response_id) {
    CTextProfile *tp = (CTextProfile *)user_data;
    if (tp == NULL) {
      wid = lookup_widget(GTK_WIDGET(dialog), 
			  "TextProfileEntry");
      const char *name = gtk_entry_get_text(GTK_ENTRY(wid));
      if (name == NULL || *name == '\0') {
	ShowMessage("Error", "Must enter profile name");
	return;
      }
      if (AVFlow->m_text_profile_list->FindProfile(name) != NULL) {
	ShowMessage("Error", "Profile already exists");
	return;
      }
      if (AVFlow->m_text_profile_list->CreateConfig(name) == false) {
	ShowMessage("Error", "profile creation error");
	return;
      }
      tp = AVFlow->m_text_profile_list->FindProfile(name);
      ret_profile = tp;
    }
    wid = lookup_widget(GTK_WIDGET(dialog), "TextProfileEncoder");
    
    tp->SetStringValue(CFG_TEXT_ENCODING, 
		       text_encoder_table[gtk_option_menu_get_history(GTK_OPTION_MENU(wid))].text_encoding);
    wid = lookup_widget(GTK_WIDGET(dialog), "TextRepeatTimeSpinner");
    tp->SetFloatValue(CFG_TEXT_REPEAT_TIME_SECS,
		      gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(wid)));
    
    tp->Update();
  }
  free(encoderNames);
  OnTextProfileFinished(ret_profile);
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

void create_TextProfileDialog(CTextProfile *tp)
{
  GtkWidget *dialog_vbox11;
  GtkWidget *table8;
  GtkWidget *TextProfileEncoder;
  GtkWidget *label211;
  GtkWidget *label212;
  GtkObject *TextRepeatTimeSpinner_adj;
  GtkWidget *TextRepeatTimeSpinner;
  GtkWidget *label213;
  GtkWidget *TextProfileName;
  GtkWidget *label224;
  GtkWidget *TextEncoderSettingsButton;
  GtkWidget *alignment38;
  GtkWidget *hbox117;
  GtkWidget *image43;
  GtkWidget *label225;
  GtkWidget *dialog_action_area10;
  GtkWidget *cancelbutton8;
  GtkWidget *okbutton10;

  TextProfileDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(TextProfileDialog), _("Text Encoder"));
  gtk_window_set_modal(GTK_WINDOW(TextProfileDialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(TextProfileDialog), FALSE);
  gtk_window_set_type_hint(GTK_WINDOW(TextProfileDialog), GDK_WINDOW_TYPE_HINT_DIALOG);

  dialog_vbox11 = GTK_DIALOG(TextProfileDialog)->vbox;
  gtk_widget_show(dialog_vbox11);

  table8 = gtk_table_new(4, 2, FALSE);
  gtk_widget_show(table8);
  gtk_box_pack_start(GTK_BOX(dialog_vbox11), table8, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table8), 11);
  gtk_table_set_col_spacings(GTK_TABLE(table8), 7);

  TextProfileEncoder = gtk_option_menu_new();
  gtk_widget_show(TextProfileEncoder);
  gtk_table_attach(GTK_TABLE(table8), TextProfileEncoder, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  label211 = gtk_label_new(_("Text Encoder:"));
  gtk_widget_show(label211);
  gtk_table_attach(GTK_TABLE(table8), label211, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label211), 0, 0.5);

  label212 = gtk_label_new(_("Repeat Time:"));
  gtk_widget_show(label212);
  gtk_table_attach(GTK_TABLE(table8), label212, 0, 1, 2, 3,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label212), 0, 0.5);

  TextRepeatTimeSpinner_adj = gtk_adjustment_new(tp != NULL ?
						 tp->GetFloatValue(CFG_TEXT_REPEAT_TIME_SECS) : 1, 0, 100, 1, 10, 10);
  TextRepeatTimeSpinner = gtk_spin_button_new(GTK_ADJUSTMENT(TextRepeatTimeSpinner_adj), 1, 1);
  gtk_widget_show(TextRepeatTimeSpinner);
  gtk_table_attach(GTK_TABLE(table8), TextRepeatTimeSpinner, 1, 2, 2, 3,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(TextRepeatTimeSpinner), TRUE);

  label213 = gtk_label_new(_("Text Profile:"));
  gtk_widget_show(label213);
  gtk_table_attach(GTK_TABLE(table8), label213, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label213), 0, 0.5);

  TextProfileName = gtk_entry_new();
  gtk_widget_show(TextProfileName);
  gtk_table_attach(GTK_TABLE(table8), TextProfileName, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  label224 = gtk_label_new(_("Encoder Settings:"));
  gtk_widget_show(label224);
  gtk_table_attach(GTK_TABLE(table8), label224, 0, 1, 3, 4,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label224), 0, 0.5);

  TextEncoderSettingsButton = gtk_button_new();
  gtk_widget_show(TextEncoderSettingsButton);
  gtk_table_attach(GTK_TABLE(table8), TextEncoderSettingsButton, 1, 2, 3, 4,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  alignment38 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment38);
  gtk_container_add(GTK_CONTAINER(TextEncoderSettingsButton), alignment38);

  hbox117 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox117);
  gtk_container_add(GTK_CONTAINER(alignment38), hbox117);

  image43 = gtk_image_new_from_stock("gtk-preferences", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image43);
  gtk_box_pack_start(GTK_BOX(hbox117), image43, FALSE, FALSE, 0);

  label225 = gtk_label_new_with_mnemonic(_("Settings"));
  gtk_widget_show(label225);
  gtk_box_pack_start(GTK_BOX(hbox117), label225, FALSE, FALSE, 0);

  dialog_action_area10 = GTK_DIALOG(TextProfileDialog)->action_area;
  gtk_widget_show(dialog_action_area10);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area10), GTK_BUTTONBOX_END);

  cancelbutton8 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton8);
  gtk_dialog_add_action_widget(GTK_DIALOG(TextProfileDialog), cancelbutton8, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton8, GTK_CAN_DEFAULT);

  okbutton10 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton10);
  gtk_dialog_add_action_widget(GTK_DIALOG(TextProfileDialog), okbutton10, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton10, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) TextProfileDialog, "response",
                    G_CALLBACK(on_TextProfileDialog_response),
                    tp);
  g_signal_connect((gpointer) TextProfileEncoder, "changed",
                    G_CALLBACK(on_TextProfileEncoder_changed),
                    tp);
  g_signal_connect((gpointer) TextEncoderSettingsButton, "clicked",
                    G_CALLBACK(on_TextEncoderSettingsButton_clicked),
                    tp);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(TextProfileDialog, TextProfileDialog, "TextProfileDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(TextProfileDialog, dialog_vbox11, "dialog_vbox11");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, table8, "table8");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, TextProfileEncoder, "TextProfileEncoder");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, label211, "label211");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, label212, "label212");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, TextRepeatTimeSpinner, "TextRepeatTimeSpinner");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, label213, "label213");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, TextProfileName, "TextProfileName");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, label224, "label224");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, TextEncoderSettingsButton, "TextEncoderSettingsButton");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, alignment38, "alignment38");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, hbox117, "hbox117");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, image43, "image43");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, label225, "label225");
  GLADE_HOOKUP_OBJECT_NO_REF(TextProfileDialog, dialog_action_area10, "dialog_action_area10");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, cancelbutton8, "cancelbutton8");
  GLADE_HOOKUP_OBJECT(TextProfileDialog, okbutton10, "okbutton10");

  uint encoderIndex = 0;
  uint ix = 0;
  encoderNames = (const char **)malloc(text_encoder_table_size *
				       sizeof(const char *));
  for (ix = 0; ix < text_encoder_table_size; ix++) {
    if (tp != NULL && 
	strcasecmp(tp->GetStringValue(CFG_TEXT_ENCODING),
		   text_encoder_table[ix].text_encoding) == 0) {
      encoderIndex = ix;
    }
    encoderNames[ix] = text_encoder_table[ix].dialog_selection_name;
  }
  CreateOptionMenu(TextProfileEncoder,
		   encoderNames, 
		   text_encoder_table_size, 
		   encoderIndex);
  if (tp != NULL) {
    gtk_entry_set_text(GTK_ENTRY(TextProfileName), 
		       tp->GetStringValue(CFG_TEXT_PROFILE_NAME));
    gtk_widget_set_sensitive(TextProfileName, false);
  }
  gtk_widget_show(TextProfileDialog);

}


/***************************************************************************
 * Text File Dialog for running events
 ***************************************************************************/
static guint timer_id = 0;
static const GtkTargetEntry drop_types[] = {
  { "text/plain", 0, 1 },
  { "text/uri-list", 0, 2},
  { "text/x-moz-url", 0, 3},
  { "text/html", 0, 4},
  { "UTF8_STRING", 0, 5},
  { "STRING", 0, 6},
};

typedef struct text_line_offset_t {
  text_line_offset_t *next_line;
  uint index;
  uint64_t offset;
} text_line_offset_t;

typedef struct text_file_data_t {
  FILE *m_file;
  char m_buffer[PATH_MAX]; // a nice large number
  uint m_index; // line in file we're on
  uint m_max_index;
  text_line_offset_t *m_line_offset_head, *m_line_offset_tail;
};

static bool ReadNextLine (text_file_data_t *tptr)
{
  off_t start;
  start = ftello(tptr->m_file);
  if (fgets(tptr->m_buffer, PATH_MAX, tptr->m_file) == NULL) {
    tptr->m_max_index = tptr->m_index;
    return false;
  }
  char *end = tptr->m_buffer + strlen(tptr->m_buffer) - 1;
  while (isspace(*end) && end > tptr->m_buffer) {
    *end = '\0';
    end--;
  }
  debug_message("Read line %u %s", tptr->m_index, tptr->m_buffer);
  
  if (tptr->m_line_offset_tail == NULL ||
      tptr->m_line_offset_tail->index < tptr->m_index) {
    text_line_offset_t *tlptr = MALLOC_STRUCTURE(text_line_offset_t);
    tlptr->next_line = NULL;
    tlptr->index = tptr->m_index;
    tlptr->offset = start;
    if (tptr->m_line_offset_head == NULL) {
      tptr->m_line_offset_head = tptr->m_line_offset_tail = tlptr;
    } else {
      tptr->m_line_offset_tail->next_line = tlptr;
      tptr->m_line_offset_tail = tlptr;
    }
    debug_message("Add to end");
  }
  tptr->m_index++;
  return true;
}

static void GoToLine (text_file_data_t *tptr, text_line_offset_t *tlptr) 
{
  debug_message("Go to line - %u offset "U64,
		tlptr->index, tlptr->offset);
  fseeko(tptr->m_file, tlptr->offset, SEEK_SET);
  tptr->m_index = tlptr->index;
  ReadNextLine(tptr);
}

static void GoToLine (text_file_data_t *tptr, uint index)
{
  uint ix;
  debug_message("go to line %u", index);
  if (tptr->m_line_offset_tail != NULL) {
    debug_message("tail index %u", index);
  }
  if (tptr->m_line_offset_tail != NULL &&
      tptr->m_line_offset_tail->index >= index) {
    debug_message("Looking for tail");
    text_line_offset_t *tlptr;
    for (ix = 0, tlptr = tptr->m_line_offset_head; ix < index; ix++) {
      tlptr = tlptr->next_line;
    }
    if (tlptr->index != index) {
      error_message("Seek not right %u %u", tlptr->index, index);
    }
    GoToLine(tptr, tlptr);
    return;
  }
  uint start_index = 0;
  if (tptr->m_line_offset_tail) {
    start_index = tptr->m_line_offset_tail->index;
    GoToLine(tptr, tptr->m_line_offset_tail);
  }
  for (ix = start_index; ix < index; ix++) {
    if (ReadNextLine(tptr) == false)
      return;
  }
}

static void GoToLine (text_file_data_t *tptr, int index)
{
  return GoToLine(tptr, (uint)index);
}

static gboolean
on_TextFileDialog_delete_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  // don't do anything - the main window will control this dialog
  return TRUE;
}

static text_file_data_t *GetTextFileDataFromUserData (gpointer user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  
  return (text_file_data_t *)lookup_widget(dialog, "TextFileData");
}

static void DisplayLineInBuffer (gpointer user_data, 
				 text_file_data_t *tptr)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  GtkWidget *entry = lookup_widget(dialog, "LineEntry");
  gtk_entry_set_text(GTK_ENTRY(entry), tptr->m_buffer);
  debug_message("Set text to %s", tptr->m_buffer);
}
  
static void
on_TextFileDialog_destroy         (GtkObject       *widget,
				   gpointer         user_data)
{
  text_file_data_t *tptr = 
    (text_file_data_t *)lookup_widget(GTK_WIDGET(widget), "TextFileData");

  if (timer_id != 0)
    gtk_timeout_remove(timer_id);
  if (tptr != NULL) {
    text_line_offset_t *tlptr = tptr->m_line_offset_head;
    while (tlptr != NULL) {
      tptr->m_line_offset_head = tlptr->next_line;
      free(tlptr);
      tlptr = tptr->m_line_offset_head;
    }
    free(tptr);
  }
}

static void
on_StartButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  debug_message("start clicked");
  text_file_data_t *tptr = GetTextFileDataFromUserData(user_data);
  GoToLine(tptr, 0);
  DisplayLineInBuffer(user_data, tptr);
}


static void
on_PrevButton_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  debug_message("prev clicked");
  text_file_data_t *tptr = GetTextFileDataFromUserData(user_data);
  uint index = tptr->m_index;
  if (tptr->m_index > 2) index = tptr->m_index - 2;
  else index = 0;
  GoToLine(tptr, index);
  DisplayLineInBuffer(user_data, tptr);
}


static void
on_NextButton_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  text_file_data_t *tptr = GetTextFileDataFromUserData(user_data);
  ReadNextLine(tptr);
  DisplayLineInBuffer(user_data, tptr);
}


static void
on_EndButton_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  text_file_data_t *tptr = GetTextFileDataFromUserData(user_data);
  if (tptr->m_line_offset_tail != NULL) {
    if (tptr->m_index < tptr->m_line_offset_tail->index) {
      GoToLine(tptr, tptr->m_line_offset_tail);
    }
  }
  while (ReadNextLine(tptr));
  DisplayLineInBuffer(user_data, tptr);
}

gint on_TextDialog_timeout (gpointer user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  GtkWidget *wid = lookup_widget(dialog, "statusbar2");
  gtk_statusbar_pop(GTK_STATUSBAR(wid), 0);
  gtk_statusbar_push(GTK_STATUSBAR(wid), 0, "");
  timer_id = 0;
  return 0;
}

static void
on_SendButton_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  GtkWidget *wid = lookup_widget(dialog, "LineEntry");
  const char *line = gtk_entry_get_text(GTK_ENTRY(wid));
  if (AVFlow->GetTextSource() == NULL) {
    error_message("no text source");
    return;
  }
  AVFlow->GetTextSource()->SourceString(line);
  
  char buffer[60];
  snprintf(buffer, sizeof(buffer),"Wrote: %.25s...", line);
  wid = lookup_widget(dialog, "statusbar2");
  gtk_statusbar_pop(GTK_STATUSBAR(wid), 0);
  gtk_statusbar_push(GTK_STATUSBAR(wid), 0, buffer);
  if (timer_id != 0) {
    gtk_timeout_remove(timer_id);
  }
  timer_id = gtk_timeout_add(3 * 1000, 
			     on_TextDialog_timeout, 
			     user_data);
  text_file_data_t *tptr = GetTextFileDataFromUserData(user_data);
  if (tptr != NULL) {
    ReadNextLine(tptr);
    DisplayLineInBuffer(user_data, tptr);
  } 
}

static void
on_LineEntry_activate  (GtkEntry *ent,
			gpointer user_data)
{
  on_SendButton_clicked(NULL, user_data);
}
static void on_drag_data_received_entry (GtkWidget *widget,
					 GdkDragContext *context,
					 gint x,
					 gint y,
					 GtkSelectionData *selection_data,
					 guint info,
					 guint time)
{

  gchar *temp, *string;

  string = (gchar *)selection_data->data;
  ADV_SPACE(string);
  temp = string + strlen(string) - 1;
  while (isspace(*temp)) {
    *temp = '\0';
    temp--;
  }

  gtk_entry_set_text(GTK_ENTRY(widget), string);
}
static void on_drag_data_received (GtkWidget *widget,
				   GdkDragContext *context,
				   gint x,
				   gint y,
				   GtkSelectionData *selection_data,
				   guint info,
				   guint time)
{
  GtkWidget *entry = lookup_widget(widget, "LineEntry");
  on_drag_data_received_entry(entry, context, x, y, selection_data, info, time);
}
      

GtkWidget *create_TextFileDialog (bool do_file)
{
  GtkWidget *TextFileDialog;
  GtkWidget *vbox42;
  GtkWidget *hbox105 = NULL;
  GtkWidget *label196 = NULL;
  GtkWidget *FileNameLabel = NULL;
  GtkWidget *LineEntry;
  GtkWidget *hbox111;
  GtkWidget *vbox43;
  GtkWidget *StartButton;
  GtkWidget *alignment32;
  GtkWidget *hbox112;
  GtkWidget *image38;
  GtkWidget *label204;
  GtkWidget *vbox44;
  GtkWidget *vbox45;
  GtkWidget *PrevButton;
  GtkWidget *alignment33;
  GtkWidget *hbox113;
  GtkWidget *image39;
  GtkWidget *label205;
  GtkWidget *label206;
  GtkWidget *vbox46;
  GtkWidget *NextButton;
  GtkWidget *alignment34;
  GtkWidget *hbox114;
  GtkWidget *label207;
  GtkWidget *image40;
  GtkWidget *vbox47;
  GtkWidget *EndButton;
  GtkWidget *alignment35;
  GtkWidget *hbox115;
  GtkWidget *label208;
  GtkWidget *image41;
  GtkWidget *label209;
  GtkWidget *vbox48;
  GtkWidget *SendButton;
  GtkWidget *alignment36;
  GtkWidget *hbox116;
  GtkWidget *label210;
  GtkWidget *image42;
  GtkWidget *statusbar2;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new();

  TextFileDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(TextFileDialog), _("Text File Transmission"));
  gtk_window_set_position(GTK_WINDOW(TextFileDialog), GTK_WIN_POS_CENTER);

  vbox42 = gtk_vbox_new(FALSE, 13);
  gtk_widget_show(vbox42);
  gtk_container_add(GTK_CONTAINER(TextFileDialog), vbox42);

  if (do_file) {
    hbox105 = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox105);
    gtk_box_pack_start(GTK_BOX(vbox42), hbox105, FALSE, FALSE, 0);

    label196 = gtk_label_new(_("File Name:"));
    gtk_widget_show(label196);
    gtk_box_pack_start(GTK_BOX(hbox105), label196, TRUE, TRUE, 0);
    gtk_misc_set_padding(GTK_MISC(label196), 0, 9);


    FileNameLabel = gtk_label_new("");
    gtk_widget_show(FileNameLabel);
    gtk_box_pack_start(GTK_BOX(hbox105), FileNameLabel, TRUE, TRUE, 0);
  }
  LineEntry = gtk_entry_new();
  gtk_widget_show(LineEntry);
  gtk_box_pack_start(GTK_BOX(vbox42), LineEntry, FALSE, FALSE, 0);

  hbox111 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox111);
  gtk_box_pack_start(GTK_BOX(vbox42), hbox111, TRUE, TRUE, 0);

  vbox43 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox43);
  gtk_box_pack_start(GTK_BOX(hbox111), vbox43, FALSE, TRUE, 0);

  StartButton = gtk_button_new();
  gtk_widget_show(StartButton);
  gtk_box_pack_start(GTK_BOX(vbox43), StartButton, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, StartButton, _("Move to beginning of file"), NULL);

  alignment32 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment32);
  gtk_container_add(GTK_CONTAINER(StartButton), alignment32);

  hbox112 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox112);
  gtk_container_add(GTK_CONTAINER(alignment32), hbox112);

  image38 = gtk_image_new_from_stock("gtk-goto-first", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image38);
  gtk_box_pack_start(GTK_BOX(hbox112), image38, FALSE, FALSE, 0);

  label204 = gtk_label_new_with_mnemonic(_("Start"));
  gtk_widget_show(label204);
  gtk_box_pack_start(GTK_BOX(hbox112), label204, FALSE, FALSE, 0);

  vbox44 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox44);
  gtk_box_pack_start(GTK_BOX(hbox111), vbox44, FALSE, TRUE, 0);

  vbox45 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox45);
  gtk_box_pack_start(GTK_BOX(vbox44), vbox45, TRUE, FALSE, 0);

  PrevButton = gtk_button_new();
  gtk_widget_show(PrevButton);
  gtk_box_pack_start(GTK_BOX(vbox45), PrevButton, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, PrevButton, _("Move to previous entry"), NULL);

  alignment33 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment33);
  gtk_container_add(GTK_CONTAINER(PrevButton), alignment33);

  hbox113 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox113);
  gtk_container_add(GTK_CONTAINER(alignment33), hbox113);

  image39 = gtk_image_new_from_stock("gtk-go-back", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image39);
  gtk_box_pack_start(GTK_BOX(hbox113), image39, FALSE, FALSE, 0);

  label205 = gtk_label_new_with_mnemonic(_("Previous"));
  gtk_widget_show(label205);
  gtk_box_pack_start(GTK_BOX(hbox113), label205, FALSE, FALSE, 0);

  label206 = gtk_label_new("");
  gtk_widget_show(label206);
  gtk_box_pack_start(GTK_BOX(hbox111), label206, TRUE, TRUE, 11);

  vbox46 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox46);
  gtk_box_pack_start(GTK_BOX(hbox111), vbox46, TRUE, TRUE, 0);

  NextButton = gtk_button_new();
  gtk_widget_show(NextButton);
  gtk_box_pack_start(GTK_BOX(vbox46), NextButton, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, NextButton, _("Move to next entry"), NULL);

  alignment34 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment34);
  gtk_container_add(GTK_CONTAINER(NextButton), alignment34);

  hbox114 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox114);
  gtk_container_add(GTK_CONTAINER(alignment34), hbox114);

  label207 = gtk_label_new_with_mnemonic(_("Next"));
  gtk_widget_show(label207);
  gtk_box_pack_start(GTK_BOX(hbox114), label207, FALSE, FALSE, 0);

  image40 = gtk_image_new_from_stock("gtk-go-forward", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image40);
  gtk_box_pack_start(GTK_BOX(hbox114), image40, FALSE, FALSE, 0);

  vbox47 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox47);
  gtk_box_pack_start(GTK_BOX(hbox111), vbox47, TRUE, TRUE, 0);

  EndButton = gtk_button_new();
  gtk_widget_show(EndButton);
  gtk_box_pack_start(GTK_BOX(vbox47), EndButton, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, EndButton, _("Move to last entry in file"), NULL);

  alignment35 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment35);
  gtk_container_add(GTK_CONTAINER(EndButton), alignment35);

  hbox115 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox115);
  gtk_container_add(GTK_CONTAINER(alignment35), hbox115);

  label208 = gtk_label_new_with_mnemonic(_("End"));
  gtk_widget_show(label208);
  gtk_box_pack_start(GTK_BOX(hbox115), label208, FALSE, FALSE, 0);

  image41 = gtk_image_new_from_stock("gtk-goto-last", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image41);
  gtk_box_pack_start(GTK_BOX(hbox115), image41, FALSE, FALSE, 0);

  label209 = gtk_label_new("");
  gtk_widget_show(label209);
  gtk_box_pack_start(GTK_BOX(hbox111), label209, TRUE, TRUE, 26);

  vbox48 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox48);
  gtk_box_pack_start(GTK_BOX(hbox111), vbox48, TRUE, FALSE, 0);

  SendButton = gtk_button_new();
  gtk_widget_show(SendButton);
  gtk_box_pack_start(GTK_BOX(vbox48), SendButton, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, SendButton, _("Transmit file"), NULL);

  alignment36 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment36);
  gtk_container_add(GTK_CONTAINER(SendButton), alignment36);

  hbox116 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox116);
  gtk_container_add(GTK_CONTAINER(alignment36), hbox116);

  label210 = gtk_label_new_with_mnemonic(_("Send"));
  gtk_widget_show(label210);
  gtk_box_pack_start(GTK_BOX(hbox116), label210, FALSE, FALSE, 0);

  image42 = gtk_image_new_from_stock("gtk-ok", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image42);
  gtk_box_pack_start(GTK_BOX(hbox116), image42, FALSE, FALSE, 0);

  statusbar2 = gtk_statusbar_new ();
  gtk_widget_show (statusbar2);
  gtk_box_pack_start (GTK_BOX (vbox42), statusbar2, FALSE, FALSE, 0);

  g_signal_connect((gpointer) TextFileDialog, "delete_event",
                    G_CALLBACK(on_TextFileDialog_delete_event),
                    NULL);
  g_signal_connect((gpointer) TextFileDialog, "destroy",
		   G_CALLBACK(on_TextFileDialog_destroy),
		   NULL);
  if (do_file) {
    g_signal_connect((gpointer) StartButton, "clicked",
		     G_CALLBACK(on_StartButton_clicked),
		     TextFileDialog);
    g_signal_connect((gpointer) PrevButton, "clicked",
		     G_CALLBACK(on_PrevButton_clicked),
		     TextFileDialog);
    g_signal_connect((gpointer) NextButton, "clicked",
		     G_CALLBACK(on_NextButton_clicked),
		     TextFileDialog);
    g_signal_connect((gpointer) EndButton, "clicked",
		     G_CALLBACK(on_EndButton_clicked),
		     TextFileDialog);
  }
  g_signal_connect((gpointer) SendButton, "clicked",
                    G_CALLBACK(on_SendButton_clicked),
                    TextFileDialog);
  g_signal_connect((gpointer)LineEntry, "activate", 
		   G_CALLBACK(on_LineEntry_activate),
		   TextFileDialog);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(TextFileDialog, TextFileDialog, "TextFileDialog");
  GLADE_HOOKUP_OBJECT(TextFileDialog, vbox42, "vbox42");
  if (hbox105 != NULL) {
    GLADE_HOOKUP_OBJECT(TextFileDialog, hbox105, "hbox105");
    GLADE_HOOKUP_OBJECT(TextFileDialog, label196, "label196");
    GLADE_HOOKUP_OBJECT(TextFileDialog, FileNameLabel, "FileNameLabel");
  }
  GLADE_HOOKUP_OBJECT(TextFileDialog, LineEntry, "LineEntry");
  GLADE_HOOKUP_OBJECT(TextFileDialog, hbox111, "hbox111");
  GLADE_HOOKUP_OBJECT(TextFileDialog, vbox43, "vbox43");
  GLADE_HOOKUP_OBJECT(TextFileDialog, StartButton, "StartButton");
  GLADE_HOOKUP_OBJECT(TextFileDialog, alignment32, "alignment32");
  GLADE_HOOKUP_OBJECT(TextFileDialog, hbox112, "hbox112");
  GLADE_HOOKUP_OBJECT(TextFileDialog, image38, "image38");
  GLADE_HOOKUP_OBJECT(TextFileDialog, label204, "label204");
  GLADE_HOOKUP_OBJECT(TextFileDialog, vbox44, "vbox44");
  GLADE_HOOKUP_OBJECT(TextFileDialog, vbox45, "vbox45");
  GLADE_HOOKUP_OBJECT(TextFileDialog, PrevButton, "PrevButton");
  GLADE_HOOKUP_OBJECT(TextFileDialog, alignment33, "alignment33");
  GLADE_HOOKUP_OBJECT(TextFileDialog, hbox113, "hbox113");
  GLADE_HOOKUP_OBJECT(TextFileDialog, image39, "image39");
  GLADE_HOOKUP_OBJECT(TextFileDialog, label205, "label205");
  GLADE_HOOKUP_OBJECT(TextFileDialog, label206, "label206");
  GLADE_HOOKUP_OBJECT(TextFileDialog, vbox46, "vbox46");
  GLADE_HOOKUP_OBJECT(TextFileDialog, NextButton, "NextButton");
  GLADE_HOOKUP_OBJECT(TextFileDialog, alignment34, "alignment34");
  GLADE_HOOKUP_OBJECT(TextFileDialog, hbox114, "hbox114");
  GLADE_HOOKUP_OBJECT(TextFileDialog, label207, "label207");
  GLADE_HOOKUP_OBJECT(TextFileDialog, image40, "image40");
  GLADE_HOOKUP_OBJECT(TextFileDialog, vbox47, "vbox47");
  GLADE_HOOKUP_OBJECT(TextFileDialog, EndButton, "EndButton");
  GLADE_HOOKUP_OBJECT(TextFileDialog, alignment35, "alignment35");
  GLADE_HOOKUP_OBJECT(TextFileDialog, hbox115, "hbox115");
  GLADE_HOOKUP_OBJECT(TextFileDialog, label208, "label208");
  GLADE_HOOKUP_OBJECT(TextFileDialog, image41, "image41");
  GLADE_HOOKUP_OBJECT(TextFileDialog, label209, "label209");
  GLADE_HOOKUP_OBJECT(TextFileDialog, vbox48, "vbox48");
  GLADE_HOOKUP_OBJECT(TextFileDialog, SendButton, "SendButton");
  GLADE_HOOKUP_OBJECT(TextFileDialog, alignment36, "alignment36");
  GLADE_HOOKUP_OBJECT(TextFileDialog, hbox116, "hbox116");
  GLADE_HOOKUP_OBJECT(TextFileDialog, label210, "label210");
  GLADE_HOOKUP_OBJECT(TextFileDialog, image42, "image42");
  GLADE_HOOKUP_OBJECT_NO_REF(TextFileDialog, tooltips, "tooltips");
  GLADE_HOOKUP_OBJECT (TextFileDialog, statusbar2, "statusbar2");

  if (do_file) {
    text_file_data_t *tptr = MALLOC_STRUCTURE(text_file_data_t);
    memset(tptr, 0, sizeof(*tptr));
    const char *fname = 
      MyConfig->GetStringValue(CONFIG_TEXT_SOURCE_FILE_NAME);
    
    tptr->m_file = fopen(fname, "r");
    if (tptr->m_file == NULL) {
      char buffer[PATH_MAX];
      snprintf(buffer, PATH_MAX, "Can't open file %s", fname);
      ShowMessage("Can't open file",buffer);
      gtk_widget_destroy(TextFileDialog);
      return NULL;
    }
    gtk_label_set_text(GTK_LABEL(FileNameLabel), fname);
    ReadNextLine(tptr);
    DisplayLineInBuffer(TextFileDialog, tptr);
    GLADE_HOOKUP_OBJECT_NO_REF(TextFileDialog, tptr, "TextFileData");
  } else {
    gtk_widget_set_sensitive(StartButton, false);
    gtk_widget_set_sensitive(PrevButton, false);
    gtk_widget_set_sensitive(NextButton, false);
    gtk_widget_set_sensitive(EndButton, false);
    GLADE_HOOKUP_OBJECT_NO_REF(TextFileDialog, NULL, "TextFileData");
  }

  gtk_drag_dest_set(TextFileDialog, 
		    GTK_DEST_DEFAULT_ALL,
		    drop_types,
		    NUM_ELEMENTS_IN_ARRAY(drop_types), 
		    GDK_ACTION_COPY);
  gtk_drag_dest_set(LineEntry, 
		    GTK_DEST_DEFAULT_ALL,
		    drop_types,
		    NUM_ELEMENTS_IN_ARRAY(drop_types), 
		    GDK_ACTION_COPY);

  g_signal_connect((gpointer)TextFileDialog, "drag_data_received",
		   G_CALLBACK(on_drag_data_received),
		   TextFileDialog);

  g_signal_connect((gpointer)LineEntry, "drag_data_received",
		   G_CALLBACK(on_drag_data_received_entry),
		   TextFileDialog);

  gtk_widget_show(TextFileDialog);

  if (do_file) {
    gtk_widget_grab_focus(SendButton);
    on_SendButton_clicked(GTK_BUTTON(SendButton), TextFileDialog);
  } else {
    gtk_widget_grab_focus(LineEntry);
  }
  return TextFileDialog;
}



/* end text_dialog.cpp */
