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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#include "mp4live.h"
#include "mp4live_gui.h"


static GtkWidget *dialog = NULL;
static GtkWidget *file_combo;
static GtkWidget *file_entry;

static GtkWidget *video_raw_button;
static GtkWidget *video_encoded_button;
static GtkWidget *audio_raw_button;
static GtkWidget *audio_encoded_button;
static GtkWidget *append_button;
static GtkWidget *overwrite_button;
static GtkWidget *create_new_button;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
  gtk_grab_remove(dialog);
  gtk_widget_destroy(dialog);
  dialog = NULL;
} 

static void on_browse_button (GtkWidget *widget, gpointer *data)
{
	FileBrowser(file_entry);
}

static bool Validate()
{
	const char *name;

	name = gtk_entry_get_text(GTK_ENTRY(file_entry));
	if (name == NULL || *name == '\0') {
		ShowMessage("Error", "Must specify file");
		return false;
	}
  
	struct stat statbuf;

	if (stat(name, &statbuf) == 0 && !S_ISREG(statbuf.st_mode)) {
		ShowMessage("Error", "Specified name is not a file");
		return false;
	}

	return true;
}
  
static void Save()
{
	MyConfig->SetStringValue(CONFIG_RECORD_MP4_FILE_NAME,
		gtk_entry_get_text(GTK_ENTRY(file_entry)));

	MyConfig->UpdateFileHistory(
		gtk_entry_get_text(GTK_ENTRY(file_entry)));

	MyConfig->SetBoolValue(CONFIG_RECORD_RAW_VIDEO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(video_raw_button)));

	MyConfig->SetBoolValue(CONFIG_RECORD_ENCODED_VIDEO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(video_encoded_button)));

	MyConfig->SetBoolValue(CONFIG_RECORD_RAW_AUDIO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_raw_button)));

	MyConfig->SetBoolValue(CONFIG_RECORD_ENCODED_AUDIO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_encoded_button)));

	config_integer_t value = FILE_MP4_OVERWRITE;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(append_button))) {
	  value = FILE_MP4_APPEND;
	} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(create_new_button))) {
	  value = FILE_MP4_CREATE_NEW;
	}

	MyConfig->SetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS, value);

	MyConfig->Update();

	DisplayRecordingSettings();
}

static void on_ok_button (GtkWidget *widget, gpointer *data)
{
	if (!Validate()) {
		return;
	}
	Save();
    on_destroy_dialog(NULL, NULL);
}

static void on_cancel_button (GtkWidget *widget, gpointer *data)
{
	on_destroy_dialog(NULL, NULL);
}

void CreateRecordingDialog (void) 
{
	GtkWidget *hbox, *label;

	SDL_LockMutex(dialog_mutex);
	if (dialog != NULL) {
	  SDL_UnlockMutex(dialog_mutex);
	  return;
	}
	SDL_UnlockMutex(dialog_mutex);
	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
			 "destroy",
			 GTK_SIGNAL_FUNC(on_destroy_dialog),
			 &dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Recording Settings");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

	// first row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 5);

	// file label 
	label = gtk_label_new(" File:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	// file entry
	file_combo = CreateFileCombo(
		MyConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME));

	file_entry = GTK_COMBO(file_combo)->entry;

	gtk_widget_show(file_combo);
	gtk_box_pack_start(GTK_BOX(hbox), file_combo, TRUE, TRUE, 5);
	
	// file browse button
	GtkWidget *browse_button = gtk_button_new_with_label(" Browse... ");
	gtk_signal_connect(GTK_OBJECT(browse_button),
		 "clicked",
		 GTK_SIGNAL_FUNC(on_browse_button),
		 NULL);
	gtk_box_pack_start(GTK_BOX(hbox), browse_button, FALSE, FALSE, 0);
	gtk_widget_show(browse_button);

	// second row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);
	// overwrite button
	overwrite_button = 
		gtk_radio_button_new_with_label(NULL, "Always overwrite existing file");
	gtk_box_pack_start(GTK_BOX(hbox), overwrite_button, FALSE, FALSE, 5);
	gtk_widget_show(overwrite_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overwrite_button),
		MyConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS) ==
				     FILE_MP4_OVERWRITE);

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);
	append_button = 
	  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(overwrite_button),
						      "Append to file");
	gtk_box_pack_start(GTK_BOX(hbox), append_button, FALSE, FALSE, 5);
	gtk_widget_show(append_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(append_button),
		MyConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS) ==
				     FILE_MP4_APPEND);
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);
	create_new_button = 
	  gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(overwrite_button),
						      "Create New File based on name");
	gtk_box_pack_start(GTK_BOX(hbox), create_new_button, FALSE, FALSE, 5);
	gtk_widget_show(create_new_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(create_new_button),
		MyConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS) ==
				     FILE_MP4_CREATE_NEW);
	// third row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 5);

	// video label
	label = gtk_label_new(" Video:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	// video raw button
	video_raw_button = gtk_check_button_new_with_label("Raw");
	gtk_box_pack_start(GTK_BOX(hbox), video_raw_button, FALSE, FALSE, 5);
	gtk_widget_show(video_raw_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_raw_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO));
  
	// video encoded button
	video_encoded_button = gtk_check_button_new_with_label("Encoded");
	gtk_box_pack_start(GTK_BOX(hbox), video_encoded_button, FALSE, FALSE, 5);
	gtk_widget_show(video_encoded_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_encoded_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO));
  
	// fourth row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 5);

	// audio label
	label = gtk_label_new(" Audio:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	// audio raw button
	audio_raw_button = gtk_check_button_new_with_label("Raw");
	gtk_box_pack_start(GTK_BOX(hbox), audio_raw_button, FALSE, FALSE, 5);
	gtk_widget_show(audio_raw_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_raw_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO));
  
	// audio encoded button
	audio_encoded_button = gtk_check_button_new_with_label("Encoded");
	gtk_box_pack_start(GTK_BOX(hbox), audio_encoded_button, FALSE, FALSE, 5);
	gtk_widget_show(audio_encoded_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_encoded_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO));
  
	// fifth row
	// standard buttons 
	AddButtonToDialog(dialog,
		" OK ", 
		GTK_SIGNAL_FUNC(on_ok_button));
	AddButtonToDialog(dialog,
		" Cancel ", 
		GTK_SIGNAL_FUNC(on_cancel_button));

	gtk_widget_show(dialog);
	gtk_grab_add(dialog);
}

/* end recording_dialog.cpp */
