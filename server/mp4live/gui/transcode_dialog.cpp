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

static GtkWidget *video_dst_menu;
static GtkWidget *video_src_delete_button;

static GtkWidget *audio_dst_menu;
static GtkWidget *audio_src_delete_button;

static int videoDstValues[] = {
	0, 1, 2
}; 
static char* videoDstNames[] = {
	"MPEG-4", "H.26L", "VP3.2"
};
static u_int8_t videoDstIndex;

static int audioDstValues[] = {
	0, 1
}; 
static char* audioDstNames[] = {
	"AAC", "MP3"
};
static u_int8_t audioDstIndex;


static void on_video_dst_menu_activate(GtkWidget *widget, gpointer data)
{
	videoDstIndex = ((unsigned int)data) & 0xFF;
}

static void on_audio_dst_menu_activate(GtkWidget *widget, gpointer data)
{
	audioDstIndex = ((unsigned int)data) & 0xFF;
}

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
	dialog = NULL;
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
	}

	return true;
}

static void Save()
{
	MyConfig->SetStringValue(CONFIG_TRANSCODE_SRC_FILE_NAME,
		gtk_entry_get_text(GTK_ENTRY(file_entry)));

	// TBD save

	DisplayTranscodingSettings();
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
  filesel = gtk_file_selection_new("Select File to Re-Encode ");
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

static void on_video_src_delete_button (GtkWidget *widget, gpointer *data)
{
}

static void on_audio_src_delete_button (GtkWidget *widget, gpointer *data)
{
}

static void on_start_button (GtkWidget *widget, gpointer *data)
{
	if (!Validate()) {
		return;
	}
	Save();

	AVLive->StopVideoPreview();

	AVFlow = AVTranscode;

	DoStart();

	on_destroy_dialog(NULL, NULL);
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

void CreateTranscodingDialog(void) 
{
	GtkWidget *hbox, *label, *button;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
			 "destroy",
			 GTK_SIGNAL_FUNC(on_destroy_dialog),
			 &dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Re-Encoding Settings");
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
	const char *file = MyConfig->GetStringValue(CONFIG_TRANSCODE_SRC_FILE_NAME);
	if (file == NULL || *file == '\0') {
		file = MyConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME);
	}
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
	label = gtk_label_new(" Convert Raw Video to");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	// video target
	video_dst_menu = CreateOptionMenu(
		NULL,
		videoDstNames, 
		sizeof(videoDstNames) / sizeof(char*),
		videoDstIndex,
		GTK_SIGNAL_FUNC(on_video_dst_menu_activate));
	gtk_box_pack_start(GTK_BOX(hbox), video_dst_menu, TRUE, TRUE, 0);


	// video src delete button
	video_src_delete_button = 
		gtk_check_button_new_with_label("Delete Raw Video");
	gtk_box_pack_start(GTK_BOX(hbox), video_src_delete_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(video_src_delete_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_video_src_delete_button),
		NULL);
	gtk_widget_show(video_src_delete_button);

	// third row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 5);

	// audio label
	label = gtk_label_new(" Convert Raw Audio to");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	// audio target
	audio_dst_menu = CreateOptionMenu(
		NULL,
		audioDstNames, 
		sizeof(audioDstNames) / sizeof(char*),
		audioDstIndex,
		GTK_SIGNAL_FUNC(on_audio_dst_menu_activate));
	gtk_box_pack_start(GTK_BOX(hbox), audio_dst_menu, TRUE, TRUE, 0);

	// audio src delete button
	audio_src_delete_button = 
		gtk_check_button_new_with_label("Delete Raw Audio");
	gtk_box_pack_start(GTK_BOX(hbox), audio_src_delete_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(audio_src_delete_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_audio_src_delete_button),
		NULL);
	gtk_widget_show(audio_src_delete_button);
	
	// fourth row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 5);

	// start button
	button = gtk_button_new_with_label(" Start ");
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 20);
	gtk_signal_connect(GTK_OBJECT(button), 
		     "clicked",
		     GTK_SIGNAL_FUNC(on_start_button),
		     NULL);
	gtk_widget_show(button);

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

/* end transcoding_dialog.cpp */
