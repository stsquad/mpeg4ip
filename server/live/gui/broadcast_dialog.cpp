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
 *              Bill May        wmay@cisco.com
 */
/*
 * broadcast_dialog.cpp - figure it out.
 */
#include <stdlib.h>
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include "gui_utils.h"
#include "gui_private.h"
#include "live_apis.h"

static GtkWidget *dialog;
static GtkWidget *address_entry;
static GtkWidget *video_port_entry;
static GtkWidget *audio_port_entry;
static GtkWidget *ok_button, *cancel_button, *apply_button;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
  gtk_grab_remove(dialog);
  gtk_widget_destroy(dialog);
  dialog = NULL;
} 

static int check_values (void)
{
  size_t temp;
  uint16_t vport, aport;
  const char *address;
  struct in_addr temp_addr;

  if ((GetNumberValueFromEntry(video_port_entry, &temp) == 0) ||
      (temp > UINT16_MAX)) {
    ShowMessage("Entry Error", "Invalid Video Port Entry");
    return (0);
  }

  if (temp & 0x1) {
    ShowMessage("Video Port Error", "Video port must be even");
    return (0);
  }

  vport = temp & UINT16_MAX;

  if ((GetNumberValueFromEntry(audio_port_entry, &temp) == 0) ||
      (temp > UINT16_MAX)) {
    ShowMessage("Entry Error", "Invalid Audio Port Entry");
    return (0);
  }

  if (temp & 0x1) {
    ShowMessage("Audio Port Error", "Audio port must be even");
    return (0);
  }

  aport = temp & UINT16_MAX;

  if (aport == vport) {
    ShowMessage("Entry Error", "Audio port and Video port must be different");
    return (0);
  }
  address = gtk_entry_get_text(GTK_ENTRY(address_entry));
  if (inet_aton(address, &temp_addr) == 0) {
    // Might have a DNS address...
    if (gethostbyname(address) == NULL) {
      // have an error
      ShowMessage("Audio Address Error", "Invalid address entered");
      return (0);
    }
  }

  const char *errmsg;
  if (set_broadcast_parameters(address, vport, aport, &errmsg) == 0) {
    ShowMessage("Error", errmsg);
    return (0);
  }
  DisplayBroadcastSettings();  // display settings in main window
  return (1);
}


static void on_apply_button (GtkWidget *widget, gpointer *data)
{
  check_values();
  gtk_widget_set_sensitive(widget, 0);
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

static int audio_port_modified;

static void on_changed (GtkWidget *widget, gpointer *data)
{
  gtk_widget_set_sensitive(apply_button, 1);
  if (widget == audio_port_entry) {
    audio_port_modified = 1;
  }
}

static void on_video_port_left (GtkWidget *widget, gpointer *data)
{
  if (audio_port_modified == 0) {
    size_t result;
    if (GetNumberValueFromEntry(video_port_entry, &result) != 0) {
      if ((result < (UINT16_MAX - 2)) && 
	  ((result & 0x1) == 0)) {
	uint16_t value;
	value = result & UINT16_MAX;
	SetNumberEntryBoxValue(audio_port_entry, value + 2);
	gtk_widget_show(audio_port_entry);
	audio_port_modified = 0;
      }
    }
  }
}
void CreateBroadcastDialog (void) 
{
  audio_port_modified = 0;

  dialog = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(dialog),
		     "destroy",
		     GTK_SIGNAL_FUNC(on_destroy_dialog),
		     &dialog);

  gtk_window_set_title(GTK_WINDOW(dialog), "Broadcast Address Configuration");

  // Address
  address_entry = AddEntryBoxWithLabel(GTK_DIALOG(dialog)->vbox,
				       "Address:",
				       get_broadcast_address(),
				       64);

  gtk_signal_connect(GTK_OBJECT(address_entry),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     video_port_entry);

  // Video Port
  uint16_t broadcast_video_port, broadcast_audio_port;
  broadcast_video_port = get_broadcast_video_port();
  broadcast_audio_port = get_broadcast_audio_port();

  if (broadcast_video_port == 0) {
    video_port_entry = AddEntryBoxWithLabel(GTK_DIALOG(dialog)->vbox,
					    "Video Port:",
					    "",
					    5);
  } else {
    video_port_entry = AddNumberEntryBoxWithLabel(GTK_DIALOG(dialog)->vbox,
						  "Video Port:",
						  broadcast_video_port,
						  5);
  }

  gtk_signal_connect(GTK_OBJECT(video_port_entry),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     video_port_entry);

  gtk_signal_connect(GTK_OBJECT(video_port_entry),
		     "focus-out-event",
		     GTK_SIGNAL_FUNC(on_video_port_left),
		     video_port_entry);

  gtk_signal_connect(GTK_OBJECT(video_port_entry),
		     "leave-notify-event",
		     GTK_SIGNAL_FUNC(on_video_port_left),
		     video_port_entry);

  if (broadcast_audio_port == 0) {
    audio_port_entry = AddEntryBoxWithLabel(GTK_DIALOG(dialog)->vbox,
					    "Audio Port:",
					    "",
					    5);
  } else {
    audio_port_entry = AddNumberEntryBoxWithLabel(GTK_DIALOG(dialog)->vbox,
						  "Audio Port:",
						  broadcast_audio_port,
						  5);
  }

  gtk_signal_connect(GTK_OBJECT(audio_port_entry),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     video_port_entry);

  
  // Add buttons at bottom
  ok_button = AddButtonToDialog(dialog,
				"Ok", 
				GTK_SIGNAL_FUNC(on_ok_button));
  GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT);

  cancel_button = AddButtonToDialog(dialog,
				    "Cancel", 
				    GTK_SIGNAL_FUNC(on_cancel_button));
  apply_button = AddButtonToDialog(dialog,
				   "Apply", 
				   GTK_SIGNAL_FUNC(on_apply_button));
  gtk_widget_set_sensitive(apply_button, 0);
  gtk_widget_show(dialog);
  gtk_grab_add(dialog);
}

/* end video_dialog.cpp */


