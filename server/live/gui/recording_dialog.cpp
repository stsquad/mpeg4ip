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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include "mp4live_gui.h"

static GtkWidget *dialog;
static GtkWidget *file_entry;
static GtkWidget *filesel;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
  gtk_grab_remove(dialog);
  gtk_widget_destroy(dialog);
  dialog = NULL;
} 

// on_yes_button - yes button hit after file select menu asks if
// you want to overwrite file.
static void on_yes_button (void)
{
	MyConfig->m_recordMp4FileName = gtk_entry_get_text(GTK_ENTRY(file_entry));
	DisplayRecordingSettings();
	on_destroy_dialog(NULL, NULL);
}

// record_values - check and record the values
static int record_values (void)
{
	const char *name;

	name = gtk_entry_get_text(GTK_ENTRY(file_entry));
	if (name == NULL || *name == '\0') {
	  ShowMessage("Error", "Must specify file");
	  return (0);
	}
  
  if (name != NULL && *name != '\0') {
    struct stat statbuf;
    if (stat(name, &statbuf) == 0) {
      if (!S_ISREG(statbuf.st_mode)) {
	ShowMessage("Error", "Specified name is not a file");
	return (0);
      }
      // We have a file name - see if we overwrite it...
	// TBD and it was not selected by browsing!!!
      char buffer[1024];
      snprintf(buffer, sizeof(buffer), 
	       "%s already exists\nDo you want to overwrite it?", 
	       name);
      YesOrNo("File Exists", buffer, 1, GTK_SIGNAL_FUNC(on_yes_button), NULL);
      return (0);
    }
  }
  
	MyConfig->m_recordMp4FileName = gtk_entry_get_text(GTK_ENTRY(file_entry));
	DisplayRecordingSettings();

	return (1);
}

static void on_filename_selected (GtkFileSelection *widget, 
				  gpointer data)
{
  const gchar *name;
  name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel));
  gtk_entry_set_text(GTK_ENTRY(file_entry), name);
  gtk_widget_show(file_entry);
  gtk_grab_remove(filesel);
  gtk_widget_destroy(filesel);
}

static void on_browse_button (GtkWidget *widget, gpointer *data)
{
  filesel = gtk_file_selection_new("Select File to Record Into");
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(filesel));
  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_filename_selected),
		     filesel);
  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(gtk_widget_destroy),
		     GTK_OBJECT(filesel));
  gtk_widget_show(filesel);
  gtk_grab_add(filesel);
}

static void on_ok_button (GtkWidget *widget, gpointer *data)
{
  if (record_values() != 0)
    on_destroy_dialog(NULL, NULL);
}

static void on_cancel_button (GtkWidget *widget, gpointer *data)
{
  on_destroy_dialog(NULL, NULL);
}

static void on_changed (GtkWidget *widget, gpointer *data)
{
}

void CreateRecordingDialog (void) 
{
  GtkWidget *hbox, *label;

  dialog = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(dialog),
		     "destroy",
		     GTK_SIGNAL_FUNC(on_destroy_dialog),
		     &dialog);

  gtk_window_set_title(GTK_WINDOW(dialog), "Recording Settings");
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

  // File: entrybox browse
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 5);

  label = gtk_label_new(" File:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  gtk_widget_show(label);

  const char *file = MyConfig->m_recordMp4FileName;
  file_entry = gtk_entry_new_with_max_length(PATH_MAX);
  if (file != NULL) {
    gtk_entry_set_text(GTK_ENTRY(file_entry), file);
  }
  gtk_box_pack_start(GTK_BOX(hbox), file_entry, TRUE, TRUE, 5);
  gtk_widget_show(file_entry);
  gtk_signal_connect(GTK_OBJECT(file_entry),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     NULL);

  GtkWidget *browse_button = gtk_button_new_with_label(" Browse... ");
  gtk_signal_connect(GTK_OBJECT(browse_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_browse_button),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox), browse_button, FALSE, FALSE, 10);
  gtk_widget_show(browse_button);
  
  // Add buttons at bottom
  GtkWidget *button;
  button = AddButtonToDialog(dialog,
			     "   OK   ", 
			     GTK_SIGNAL_FUNC(on_ok_button));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

  button = AddButtonToDialog(dialog,
			     " Cancel ", 
			     GTK_SIGNAL_FUNC(on_cancel_button));

  gtk_widget_show(dialog);
  gtk_grab_add(dialog);
}

/* end recording_dialog.cpp */
