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

static GtkWidget *video_raw_button;
static GtkWidget *video_encoded_button;
static GtkWidget *audio_raw_button;
static GtkWidget *audio_encoded_button;

// forward declaration
static void Save();

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
  gtk_grab_remove(dialog);
  gtk_widget_destroy(dialog);
  dialog = NULL;
} 

static void on_video_raw_button(GtkWidget *widget, gpointer *data)
{
}

static void on_video_encoded_button(GtkWidget *widget, gpointer *data)
{
}

static void on_audio_raw_button(GtkWidget *widget, gpointer *data)
{
}

static void on_audio_encoded_button(GtkWidget *widget, gpointer *data)
{
}

static void on_overwrite_yes_button (void)
{
	Save();
	on_destroy_dialog(NULL, NULL);
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

	if (stat(name, &statbuf) == 0) {
		if (!S_ISREG(statbuf.st_mode)) {
			ShowMessage("Error", "Specified name is not a file");
			return false;
		}

		// We have a file, check that overwriting is OK 
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), 
			"%s already exists\nDo you want to overwrite it?", 
			name);
		YesOrNo("File Exists", buffer, 1, 
			GTK_SIGNAL_FUNC(on_overwrite_yes_button), NULL);
		return false;
	}

	return true;
}
  
static void Save()
{
	MyConfig->SetStringValue(CONFIG_RECORD_MP4_FILE_NAME,
		gtk_entry_get_text(GTK_ENTRY(file_entry)));

	MyConfig->SetStringValue(CONFIG_RECORD_MP4_FILE_NAME, 
		gtk_entry_get_text(GTK_ENTRY(file_entry)));

	MyConfig->SetBoolValue(CONFIG_RECORD_RAW_VIDEO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(video_raw_button)));

	MyConfig->SetBoolValue(CONFIG_RECORD_ENCODED_VIDEO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(video_encoded_button)));

	MyConfig->SetBoolValue(CONFIG_RECORD_RAW_AUDIO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_raw_button)));

	MyConfig->SetBoolValue(CONFIG_RECORD_ENCODED_AUDIO,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_encoded_button)));

	MyConfig->Update();

	DisplayRecordingSettings();
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

static void on_changed (GtkWidget *widget, gpointer *data)
{
}

void AddOkCancel(GtkWidget* dialog);

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
	const char *file = MyConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME);
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
	
	// file browse button
	GtkWidget *browse_button = gtk_button_new_with_label(" Browse... ");
	gtk_signal_connect(GTK_OBJECT(browse_button),
		 "clicked",
		 GTK_SIGNAL_FUNC(on_browse_button),
		 NULL);
	gtk_box_pack_start(GTK_BOX(hbox), browse_button, FALSE, FALSE, 10);
	gtk_widget_show(browse_button);

	// second row
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
	gtk_signal_connect(GTK_OBJECT(video_raw_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_video_raw_button),
		NULL);
	gtk_widget_show(video_raw_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_raw_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_RAW_VIDEO));
  
	// video encoded button
	video_encoded_button = gtk_check_button_new_with_label("Encoded");
	gtk_box_pack_start(GTK_BOX(hbox), video_encoded_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(video_encoded_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_video_encoded_button),
		NULL);
	gtk_widget_show(video_encoded_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_encoded_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_ENCODED_VIDEO));
  
	// third row
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
	gtk_signal_connect(GTK_OBJECT(audio_raw_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_audio_raw_button),
		NULL);
	gtk_widget_show(audio_raw_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_raw_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_RAW_AUDIO));
  
	// audio encoded button
	audio_encoded_button = gtk_check_button_new_with_label("Encoded");
	gtk_box_pack_start(GTK_BOX(hbox), audio_encoded_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(audio_encoded_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_audio_encoded_button),
		NULL);
	gtk_widget_show(audio_encoded_button);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_encoded_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_ENCODED_AUDIO));
  
	// Add standard buttons at bottom
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
