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
 * video_dialog.cpp - video dialog
 */
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <errno.h>
#include "gui_utils.h"
#include "gui_private.h"
#include "live_apis.h"

static GtkWidget *vid_dialog;
static GtkWidget *ok_button, *cancel_button, *apply_button;
static GtkWidget *video_inputs_menu;
static GtkWidget *height, *width, *fps;
static GtkWidget *kbps;
static GtkWidget *crop_height, *crop_width;
static GtkWidget *crop_enabled_button;

static size_t local_capture_index;
static size_t local_video_input_index;
static size_t local_h, local_w, local_fps;
static size_t local_video_encoder_index, local_kbps;
static int local_crop_enabled;
static size_t local_crop_h, local_crop_w;



static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
  gtk_grab_remove(vid_dialog);
  gtk_widget_destroy(vid_dialog);
  vid_dialog = NULL;
}
 
static void on_capture_inputs (GtkWidget *widget, gpointer gdata)
{
  size_t data;
  data = (size_t) gdata;
  if (data != local_video_input_index) {
    local_video_input_index = data;
    gtk_widget_set_sensitive(apply_button, 1);
  }
}
static void on_crop_enabled_button (GtkWidget *widget, gpointer gdata)
{
  if (local_crop_enabled != 
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(crop_enabled_button))) {
    if (local_crop_enabled) {
      local_crop_enabled = FALSE;
    } else {
      local_crop_enabled = TRUE;
    }
    gtk_widget_set_sensitive(apply_button, 1);
    gtk_editable_set_editable(GTK_EDITABLE(crop_height), local_crop_enabled);
    gtk_editable_set_editable(GTK_EDITABLE(crop_width), local_crop_enabled);
  }
    
}

static void on_video_encoder (GtkWidget *widget, gpointer gdata)
{
  size_t data;
  data = (size_t) gdata;
  if (data != local_video_encoder_index) {
    local_video_encoder_index = data;
    gtk_widget_set_sensitive(apply_button, 1);
  }
}
static void on_capture_device (GtkWidget *widget, gpointer gdata)
{
  size_t data;
  data = (size_t) gdata;
  if (data != local_capture_index) {
    local_capture_index = data;
    // We have a new capture device - make sure that we set the video capture
    // inputs correctly.
    const char **name;
    size_t max;
    name = get_video_capture_inputs(local_capture_index,
				    max, 
				    local_video_input_index);
    video_inputs_menu = CreateOptionMenu(video_inputs_menu,
					 name, 
					 max, 
					 local_video_input_index, 
					 GTK_SIGNAL_FUNC(on_capture_inputs));
    gtk_widget_set_sensitive(apply_button, 1);
  }
}

static int check_values (void)
{
  size_t old_capture, old_input;
  size_t h, w, f, ch, cw, k;
  size_t err = 0;
  gboolean crop;
  
#define BAD_HEIGHT 0x01
#define BAD_WIDTH  0x02
#define BAD_FPS    0x04
#define BAD_CROP_H 0x08
#define BAD_CROP_W 0x10
#define BAD_KBPS   0x20
  if (GetNumberValueFromEntry(height, &h) == 0) {
    err |= BAD_HEIGHT;
  }

  if (GetNumberValueFromEntry(width, &w) == 0) {
    err |= BAD_WIDTH;
  }

  if (GetNumberValueFromEntry(fps, &f) == 0) {
    err |= BAD_FPS;
  }

  if (GetNumberValueFromEntry(kbps, &k) == 0) {
    err |= BAD_KBPS;
  }

  crop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(crop_enabled_button));
  if (crop) {
    if (GetNumberValueFromEntry(crop_height, &ch) == 0) 
      err |= BAD_CROP_H;
    if (GetNumberValueFromEntry(crop_width, &cw) == 0)
      err |= BAD_CROP_W;
  } else {
    cw = ch = 0;
  }
  
  const char *errmsg;
  if (err == 0) {
    old_capture = get_capture_device_index();
    old_input = get_video_capture_input_index();
    set_capture_device(local_capture_index);
    set_video_capture_inputs(local_video_input_index);
    if (check_video_parameters(h, w, crop, ch, cw, f, 
			       local_video_encoder_index, k, &errmsg) != 0) {
      set_video_parameters(h, w, crop, ch, cw, f, local_video_encoder_index, k);
      DisplayVideoSettings();  // display settings in main window
      return (1);
    } else {
      set_capture_device(old_capture);
      set_video_capture_inputs(old_input);
      ShowMessage("Entry Error", errmsg);
    }
  } else {
    errmsg = "????";

    if (err & BAD_HEIGHT) 
      errmsg = "Invalid Height";
    else if (err & BAD_WIDTH)
      errmsg = "Invalid Width";
    else if (err & BAD_FPS) 
      errmsg = "Invalid Frames Per Second";
    else if (err & BAD_KBPS) 
      errmsg = "Invalid Encoder KBits Per Second";
    else if (crop) {
      if (err & BAD_CROP_H)
	errmsg = "Invalid Crop Height";
      else errmsg = "Invalid Crop Width";

    }
    ShowMessage("Invalid Entry", errmsg);
  }
  return (0);
}
static void on_changed (GtkWidget *widget, gpointer *data)
{
  gtk_widget_set_sensitive(apply_button, 1);
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

void CreateVideoDialog (void) 
{
  GtkWidget *hbox, *label, *omenu;

  vid_dialog = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(vid_dialog),
		     "destroy",
		     GTK_SIGNAL_FUNC(on_destroy_dialog),
		     &vid_dialog);

  gtk_window_set_title(GTK_WINDOW(vid_dialog), "Video Configuration");
  
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vid_dialog)->vbox),
		     hbox,
		     TRUE,
		     TRUE, 
		     5);

  label = gtk_label_new("Capture Device");
  gtk_widget_ref(label);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

  size_t max;
  const char **names;

  names = get_capture_devices(max, local_capture_index);
  omenu = CreateOptionMenu(NULL, names, max, local_capture_index, 
			   GTK_SIGNAL_FUNC(on_capture_device));
  gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);

  // Video Capture inputs
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vid_dialog)->vbox),
		     hbox,
		     TRUE,
		     TRUE, 
		     5);

  label = gtk_label_new("Video Inputs");
  gtk_widget_ref(label);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
  names = get_video_capture_inputs(local_capture_index, 
				   max, 
				   local_video_input_index);
  video_inputs_menu = CreateOptionMenu(NULL,
				       names, 
				       max, 
				       local_video_input_index, 
				       GTK_SIGNAL_FUNC(on_capture_inputs));
  gtk_box_pack_start(GTK_BOX(hbox), video_inputs_menu, TRUE, TRUE, 0);

  // Height, Width, FPS
  local_h = get_video_height();
  local_w = get_video_width();
  local_fps = get_video_frames_per_second();
  local_crop_enabled = get_video_crop_enabled();
  local_crop_h = get_video_crop_height();
  local_crop_w = get_video_crop_width();
  local_kbps = get_video_encoder_kbps();

  height = AddNumberEntryBoxWithLabel(GTK_DIALOG(vid_dialog)->vbox,
				      "Height:",
				      local_h,
				      4);
  gtk_signal_connect(GTK_OBJECT(height),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     NULL);
  
  width = AddNumberEntryBoxWithLabel(GTK_DIALOG(vid_dialog)->vbox,
				     "Width:",
				     local_w,
				     4);
  gtk_signal_connect(GTK_OBJECT(width),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     NULL);
  
  fps = AddNumberEntryBoxWithLabel(GTK_DIALOG(vid_dialog)->vbox,
				   "Frames Per Second:",
				   local_fps,
				   3);
  gtk_signal_connect(GTK_OBJECT(fps),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     NULL);

  GtkWidget *frame, *vbox;
  frame = gtk_frame_new("Encoding");
  gtk_frame_set_label_align(GTK_FRAME(frame), .5, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vid_dialog)->vbox), frame, TRUE, TRUE, 5);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);
  // add encoder type...

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox),
		     hbox,
		     TRUE,
		     TRUE, 
		     5);

  label = gtk_label_new("Video Encoder:");
  gtk_widget_ref(label);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

  names = get_video_encoder_types(max, local_video_encoder_index);
  omenu = CreateOptionMenu(NULL, names, max, local_video_encoder_index, 
			   GTK_SIGNAL_FUNC(on_video_encoder));
  gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);

  kbps = AddNumberEntryBoxWithLabel(vbox, 
				    "KBits Per Second:",
				    local_kbps,
				    5);
  gtk_signal_connect(GTK_OBJECT(kbps),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     NULL);

  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(frame);
  
  frame = gtk_frame_new("Cropping");
  gtk_frame_set_label_align(GTK_FRAME(frame), .5, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(vid_dialog)->vbox), frame, TRUE, TRUE, 5);
  
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

  crop_enabled_button = gtk_check_button_new_with_label("Enabled");
  gtk_box_pack_start(GTK_BOX(hbox), crop_enabled_button, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(crop_enabled_button),
		     "toggled",
		     GTK_SIGNAL_FUNC(on_crop_enabled_button),
		     NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(crop_enabled_button),
			       local_crop_enabled);
  gtk_widget_show(crop_enabled_button);
			    
  crop_height = AddNumberEntryBoxWithLabel(vbox,
					   "Height After Crop:",
					   local_crop_h,
					   4);
  gtk_signal_connect(GTK_OBJECT(crop_height),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     NULL);
    
    
  crop_width = AddNumberEntryBoxWithLabel(vbox,
					  "Width After Crop:",
					  local_crop_w,
					  4);
  gtk_signal_connect(GTK_OBJECT(crop_width),
		     "changed",
		     GTK_SIGNAL_FUNC(on_changed),
		     NULL);
  if (local_crop_enabled == 0) {
    gtk_editable_set_editable(GTK_EDITABLE(crop_height), FALSE);
    gtk_editable_set_editable(GTK_EDITABLE(crop_width), FALSE);
  }
  
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(frame);

  // Add buttons at bottom
  ok_button = AddButtonToDialog(vid_dialog, 
				"Ok", 
				GTK_SIGNAL_FUNC(on_ok_button));
  cancel_button = AddButtonToDialog(vid_dialog,
				    "Cancel", 
				    GTK_SIGNAL_FUNC(on_cancel_button));
  apply_button = AddButtonToDialog(vid_dialog,
				   "Apply", 
				   GTK_SIGNAL_FUNC(on_apply_button));
  gtk_widget_set_sensitive(apply_button, 0);
  gtk_widget_show(vid_dialog);
  gtk_grab_add(vid_dialog);
}

/* end video_dialog.cpp */


