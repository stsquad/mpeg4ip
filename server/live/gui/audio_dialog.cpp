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

#define __STDC_LIMIT_MACROS
#include "mp4live.h"
#include "mp4live_gui.h"

static GtkWidget *aud_dialog;
static GtkWidget *aud_kbps;
static GtkWidget *ok_button, *cancel_button, *apply_button;

static size_t local_freq_index;
static size_t local_codec_index, local_encoding_kbps;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
  gtk_grab_remove(aud_dialog);
  gtk_widget_destroy(aud_dialog);
  aud_dialog = NULL;
}

static int check_values (void)
{
  size_t kb;

  if (GetNumberValueFromEntry(aud_kbps, &kb) == 0) {
    ShowMessage("Entry Error", "Invalid Bitrate Entry");
    return (0);
  }

  const char *errmsg;

#ifdef NOTDEF
  if (set_audio_parameters(local_codec_index, 
			   local_freq_index, 
			   kb, 
			   &errmsg) < 0)
    {
      ShowMessage("Entry Error", errmsg);
      return (0);
    } 
#endif

  DisplayAudioSettings();  // display settings in main window
  return (1);
}


static void on_apply_button (GtkWidget *widget, gpointer *data)
{
  check_values();
  gtk_widget_set_sensitive(apply_button, 0);
}

static void on_ok_button (GtkWidget *widget, gpointer *data)
{
  if (check_values() != 0)
    on_destroy_dialog(NULL, NULL);
}

static void on_cancel_button (GtkWidget *widget, gpointer *data)
{
  on_destroy_dialog(NULL, NULL);
}

static void on_freq (GtkWidget *widget, gpointer *gdata)
{
  size_t data;
  data = (size_t)gdata;
  if (data != local_freq_index) {
    local_freq_index = data;
    gtk_widget_set_sensitive(apply_button, 1);
  }
}

static void on_codec (GtkWidget *widget, gpointer *gdata)
{
  size_t data;
  data = (size_t) gdata;
  if (data != local_codec_index) {
    local_codec_index = data;
    gtk_widget_set_sensitive(apply_button, 1);
  }
}

static void on_changed (GtkWidget *widget, gpointer *gdata)
{
  gtk_widget_set_sensitive(apply_button, 1);
}


void CreateAudioDialog (void) 
{
  GtkWidget *hbox, *label, *omenu;
const char *samplingRates[] = { "32000", "44100", "48000", NULL };
  const char **names = samplingRates;
  size_t max;

  aud_dialog = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(aud_dialog),
		     "destroy",
		     GTK_SIGNAL_FUNC(on_destroy_dialog),
		     &aud_dialog);

  gtk_window_set_title(GTK_WINDOW(aud_dialog), "Audio Settings");

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(aud_dialog)->vbox), hbox, 
	TRUE, TRUE, 5);

  // Audio Sampling Frequency
  label = gtk_label_new(" Sampling Rate:");
  gtk_widget_ref(label);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

#ifdef NOTDEF
#ifdef NOTDEF
  names = get_audio_frequencies(max, local_freq_index);
#endif
  omenu = CreateOptionMenu(NULL, names, max, local_freq_index, 
			   GTK_SIGNAL_FUNC(on_freq));
  gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(aud_dialog)->vbox),
		     hbox,
		     TRUE,
		     TRUE, 
		     5);
#endif

	// Bitrate
  local_encoding_kbps = MyConfig->m_audioTargetBitRate;

  aud_kbps = AddNumberEntryBoxWithLabel(GTK_DIALOG(aud_dialog)->vbox,
					"Encoded Bitrate (Kbps):",
					local_encoding_kbps,
					5);
  
  gtk_signal_connect(GTK_OBJECT(aud_kbps),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     aud_kbps);

  // Add buttons at bottom
  ok_button = AddButtonToDialog(aud_dialog,
				" Ok ", 
				GTK_SIGNAL_FUNC(on_ok_button));
  cancel_button = AddButtonToDialog(aud_dialog,
				    " Cancel ", 
				    GTK_SIGNAL_FUNC(on_cancel_button));
  apply_button = AddButtonToDialog(aud_dialog,
				   "Apply", 
				   GTK_SIGNAL_FUNC(on_apply_button));
  gtk_widget_set_sensitive(apply_button, 0);
  gtk_widget_show(aud_dialog);
  gtk_grab_add(aud_dialog);
}

/* end audio_dialog.cpp */
