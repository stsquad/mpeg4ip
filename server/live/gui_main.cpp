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
 * gui_main.cpp - Contains the gtk based gui 
 */
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <time.h>
#include <sys/time.h>
#include "gui_utils.h"
#include "gui_private.h"
#include "live_apis.h"
#include "util.h"

/* Local variables */
static GtkWidget *main_window;
static GtkWidget *main_vbox;
static GtkWidget *video_enabled_button;
static GtkWidget *video_preview_button;
static GtkWidget *video_settings_button;
static GtkWidget *video_settings_label1;
static GtkWidget *video_settings_label2;
static GtkWidget *video_settings_label3;
static GtkWidget *video_broadcast_label;

static GtkWidget *audio_enabled_button;
static GtkWidget *audio_preview_button;
static GtkWidget *audio_settings_button, *audio_settings_label;
static GtkWidget *broadcast_button, *audio_broadcast_label;
static GtkWidget *recording_setting_button, *recording_label;
static GtkWidget *record_button;
static GtkWidget *current_time;
static GtkWidget *start_time;
static GtkWidget *duration;

static CFileWriteBase *file_recording = NULL;

/*
 * delete_event - called when window closed
 */
static void delete_event (GtkWidget *widget, gpointer *data)
{
  gtk_main_quit();
}

void DisplayVideoSettings (void)
{
  char buffer[256];
  
  snprintf(buffer, sizeof(buffer), "%s %s", get_capture_device(),
	   get_video_capture_input());
  gtk_label_set_text(GTK_LABEL(video_settings_label1), buffer);
  gtk_widget_show(video_settings_label1);
  snprintf(buffer, sizeof(buffer), "%ux%u, %u FPS", 
	   get_video_height(),
	   get_video_width(),
	   get_video_frames_per_second());
  gtk_label_set_text(GTK_LABEL(video_settings_label2), buffer);
  gtk_widget_show(video_settings_label2);

  snprintf(buffer, sizeof(buffer), "%s at %u KBps",
	   get_video_encoder_type(), get_video_encoder_kbps());
  gtk_label_set_text(GTK_LABEL(video_settings_label3), buffer);
  gtk_widget_show(video_settings_label3);
}

void DisplayAudioSettings (void)
{
  char buffer[256];
 
  snprintf(buffer, sizeof(buffer), "%s, %s at %u KBps",
	   get_audio_frequency(),
	   get_audio_codec(),
	   get_audio_kbitrate());
  gtk_label_set_text(GTK_LABEL(audio_settings_label), buffer);
  gtk_widget_show(audio_settings_label);
}

void DisplayBroadcastSettings (void)
{
  const char *addr;
  uint16_t aport, vport;
  char buffer[80];

  addr = get_broadcast_address();
  if (addr == NULL) {
    gtk_label_set_text(GTK_LABEL(audio_broadcast_label), 
		       "Audio: No Address Configured");
    gtk_label_set_text(GTK_LABEL(video_broadcast_label), 
		       "Video: No Address Configured");
  } else {
    aport = get_broadcast_audio_port();
    vport = get_broadcast_video_port();
    snprintf(buffer, sizeof(buffer), "Audio: %s, Port %u", addr, aport);
    gtk_label_set_text(GTK_LABEL(audio_broadcast_label), buffer);
    snprintf(buffer, sizeof(buffer), "Video: %s, Port %u", addr, vport);
    gtk_label_set_text(GTK_LABEL(video_broadcast_label), buffer);
  }
  gtk_widget_show(audio_broadcast_label);
  gtk_widget_show(video_broadcast_label);
  
}

void DisplayRecordingSettings (void)
{
  int aud, vid;
  const char *file;
  char buffer[120];

  aud = get_record_audio();
  vid = get_record_video();
  file = get_record_file();
  if (file == NULL) {
    gtk_label_set_text(GTK_LABEL(recording_label), "No file specified");
  } else if (aud == 0 && vid == 0) {
    gtk_label_set_text(GTK_LABEL(recording_label), "Not recording to file");
  } else {
    if (aud != 0 && vid == 0) {
      snprintf(buffer, sizeof(buffer), "%s (Audio only)", file);
    } if (vid != 0 && aud == 0) {
      snprintf(buffer, sizeof(buffer), "%s (Video only)", file);
    } else {
      snprintf(buffer, sizeof(buffer), file);
    }
    gtk_label_set_text(GTK_LABEL(recording_label), buffer);
  }
  gtk_widget_show(recording_label);
}

static void on_video_enabled_button (GtkWidget *widget, gpointer *data)
{
  printf("video enabled\n");
}

static void on_video_preview_button (GtkWidget *widget, gpointer *data)
{
  printf("video preview\n");
}

static void on_video_settings_button (GtkWidget *widget, gpointer *data)
{
  CreateVideoDialog();
  DisplayVideoSettings();
}
static void on_audio_enabled_button (GtkWidget *widget, gpointer *data)
{
  printf("audio enabled\n");
}

static void on_audio_preview_button (GtkWidget *widget, gpointer *data)
{
  printf("audio preview\n");
}

static void on_audio_settings_button (GtkWidget *widget, gpointer *data)
{
  CreateAudioDialog();
}
static void on_broadcast_button (GtkWidget *widget, gpointer *data)
{
  CreateBroadcastDialog();
}

static void on_recording_setting_button (GtkWidget *widget, gpointer *data)
{
  CreateRecordingDialog();
}

static int recording = 0;

static void on_record_button (GtkWidget *widget, gpointer *data)
{
  int audio_enabled, video_enabled;
  audio_enabled = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_enabled_button));
  video_enabled = 
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(video_enabled_button));

  if (audio_enabled == 0) {
    ShowMessage("Can't record", "Neither audio nor video are enabled");
    return;
  }
  if (recording == 0) {
    debug_message("Starting recording");
    file_recording = start_record(audio_enabled, video_enabled);
    if (audio_enabled != 0) 
      start_audio_recording(file_recording);
    recording = 1;
  } else {
    debug_message("Stop recording");
    if (audio_enabled != 0) {
      stop_audio_recording();
    }
    if (file_recording != NULL) {
      delete file_recording;
      file_recording = NULL;
    }
    recording = 0;
  }
    
}
/*
 * Main timer routine - runs ~every 500 msec
 * Might be able to adjust this to handle just when playing.
 */
static gint main_timer (gpointer raw)
{
  struct timeval tv;
  const struct tm *local;
  char buffer[80];

  gettimeofday(&tv, NULL);
  local = localtime(&tv.tv_sec);
  strftime(buffer, sizeof(buffer), "Current Time: %r", local);
  gtk_label_set_text(GTK_LABEL(current_time), buffer);
  gtk_widget_show(current_time);
  return (TRUE);  // keep timer going
}


/*
 * Main routine - set up window
 */
int main (int argc, char **argv)
{
  GtkWidget *frame;
  GtkWidget *vbox, *hbox;

  init_audio();

  gtk_init(&argc, &argv);

  /*
   * Set up main window
   */
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(main_window), FALSE, TRUE, FALSE);
  gtk_window_set_title(GTK_WINDOW(main_window), 
		       "cisco Open Source MPEG4 Live Encoder");
  //  gtk_widget_set_uposition(GTK_WIDGET(main_window), 10, 10);
  // gtk_widget_set_usize(main_window, 450, 185);
  // gtk_window_set_default_size(GTK_WINDOW(main_window), 450, 185);
  gtk_signal_connect(GTK_OBJECT(main_window),
		     "delete_event",
		     GTK_SIGNAL_FUNC(delete_event),
		     NULL);
  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(main_vbox);
  // Create Video Frame and objects
  frame = gtk_frame_new("Video");
  gtk_frame_set_label_align(GTK_FRAME(frame), .5, 0);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
  
  video_enabled_button = gtk_check_button_new_with_label("Enabled");
  gtk_box_pack_start(GTK_BOX(hbox), video_enabled_button, TRUE, TRUE, 5);
  gtk_signal_connect(GTK_OBJECT(video_enabled_button), 
		     "toggled",
		     GTK_SIGNAL_FUNC(on_video_enabled_button),
		     NULL);
  gtk_widget_show(video_enabled_button);

  video_preview_button = gtk_check_button_new_with_label("Preview");
  gtk_box_pack_start(GTK_BOX(hbox), video_preview_button, TRUE, TRUE, 5);
  gtk_signal_connect(GTK_OBJECT(video_preview_button), 
		     "toggled",
		     GTK_SIGNAL_FUNC(on_video_preview_button),
		     NULL);
  gtk_widget_show(video_preview_button);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);

  GtkWidget *vbox2 = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox2);
  gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 5);

  video_settings_button = gtk_button_new_with_label("Settings");
  gtk_box_pack_start(GTK_BOX(vbox2), video_settings_button, FALSE, FALSE, 5);

  gtk_signal_connect(GTK_OBJECT(video_settings_button), 
		     "clicked",
		     GTK_SIGNAL_FUNC(on_video_settings_button),
		     NULL);
  gtk_widget_show(video_settings_button);

  GtkWidget *sep;

  vbox2 = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox2);
  gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 10);

  video_settings_label1 = gtk_label_new("");
  gtk_widget_ref(video_settings_label1);
  gtk_box_pack_start(GTK_BOX(vbox2), video_settings_label1, TRUE, TRUE, 0);
  video_settings_label2 = gtk_label_new("");
  gtk_widget_ref(video_settings_label2);
  gtk_box_pack_start(GTK_BOX(vbox2), video_settings_label2, TRUE, TRUE, 0);
  video_settings_label3 = gtk_label_new("");
  gtk_widget_ref(video_settings_label3);
  gtk_box_pack_start(GTK_BOX(vbox2), video_settings_label3, TRUE, TRUE, 0);
  DisplayVideoSettings();

  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(frame);

  // Audio Frame
  frame = gtk_frame_new("Audio");
  gtk_frame_set_label_align(GTK_FRAME(frame), .5, 0);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
  
  audio_enabled_button = gtk_check_button_new_with_label("Enabled");
  gtk_box_pack_start(GTK_BOX(hbox), audio_enabled_button, TRUE, TRUE, 5);
  gtk_signal_connect(GTK_OBJECT(audio_enabled_button), 
		     "toggled",
		     GTK_SIGNAL_FUNC(on_audio_enabled_button),
		     NULL);
  gtk_widget_show(audio_enabled_button);

  audio_preview_button = gtk_check_button_new_with_label("Preview");
  gtk_box_pack_start(GTK_BOX(hbox), audio_preview_button, TRUE, TRUE, 5);
  gtk_signal_connect(GTK_OBJECT(audio_preview_button), 
		     "toggled",
		     GTK_SIGNAL_FUNC(on_audio_preview_button),
		     NULL);
  gtk_widget_show(audio_preview_button);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

  audio_settings_button = gtk_button_new_with_label("Settings");
  gtk_box_pack_start(GTK_BOX(hbox), audio_settings_button, FALSE, FALSE, 5);
  gtk_signal_connect(GTK_OBJECT(audio_settings_button), 
		     "clicked",
		     GTK_SIGNAL_FUNC(on_audio_settings_button),
		     NULL);
  gtk_widget_show(audio_settings_button);

  audio_settings_label = gtk_label_new("");
  gtk_widget_ref(audio_settings_label);
  gtk_widget_show(audio_settings_label);
  gtk_box_pack_start(GTK_BOX(hbox), audio_settings_label, TRUE, TRUE, 0);
  DisplayAudioSettings();

  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(frame);

  // Broadcast settings
  frame = gtk_frame_new("Broadcast");
  gtk_frame_set_label_align(GTK_FRAME(frame), .5, 0);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);

  video_broadcast_label = gtk_label_new("");
  gtk_widget_ref(video_broadcast_label);
  gtk_widget_show(video_broadcast_label);
  gtk_box_pack_start(GTK_BOX(vbox), video_broadcast_label, TRUE, TRUE, 0);

  audio_broadcast_label = gtk_label_new("");
  gtk_widget_ref(audio_broadcast_label);
  gtk_widget_show(audio_broadcast_label);
  gtk_box_pack_start(GTK_BOX(vbox), audio_broadcast_label, TRUE, TRUE, 0);

  DisplayBroadcastSettings();
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  broadcast_button = gtk_button_new_with_label("Settings");
  gtk_box_pack_start(GTK_BOX(hbox), broadcast_button, TRUE, FALSE, 5);
  gtk_signal_connect(GTK_OBJECT(broadcast_button), 
		     "clicked",
		     GTK_SIGNAL_FUNC(on_broadcast_button),
		     NULL);
  gtk_widget_show(broadcast_button);


  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(frame);

  // Recording Frame
  frame = gtk_frame_new("Recording");
  gtk_frame_set_label_align(GTK_FRAME(frame), .5, 0);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);
 
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);


  recording_setting_button = gtk_button_new_with_label("Settings");
  gtk_box_pack_start(GTK_BOX(hbox), recording_setting_button, FALSE, FALSE, 5);
  gtk_signal_connect(GTK_OBJECT(recording_setting_button), 
		     "clicked",
		     GTK_SIGNAL_FUNC(on_recording_setting_button),
		     NULL);
  gtk_widget_show(recording_setting_button);

  recording_label = gtk_label_new("");
  gtk_widget_ref(recording_label);
  gtk_box_pack_start(GTK_BOX(hbox), recording_label, TRUE, TRUE, 0);
  DisplayRecordingSettings();

  gtk_widget_show(frame);

  // Control frame
  frame = gtk_frame_new("Status");
  gtk_frame_set_label_align(GTK_FRAME(frame), .5, 0);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

  record_button = gtk_button_new_with_label("Record");
  gtk_box_pack_start(GTK_BOX(vbox), record_button, FALSE, FALSE, 5);
  gtk_signal_connect(GTK_OBJECT(record_button), 
		     "clicked",
		     GTK_SIGNAL_FUNC(on_record_button),
		     NULL);
  gtk_widget_show(record_button);

  sep = gtk_vseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 10);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

  current_time = gtk_label_new("Current Time:");
  gtk_widget_ref(current_time);
  gtk_widget_show(current_time);
  gtk_box_pack_start(GTK_BOX(vbox), current_time, TRUE, TRUE, 0);

  start_time = gtk_label_new("Start Time:");
  gtk_widget_ref(start_time);
  gtk_widget_show(start_time);
  gtk_box_pack_start(GTK_BOX(vbox), start_time, TRUE, TRUE, 0);

  duration = gtk_label_new("Duration:");
  gtk_widget_ref(duration);
  gtk_widget_show(duration);
  gtk_box_pack_start(GTK_BOX(vbox), duration, TRUE, TRUE, 0);

  gtk_widget_show(frame); // show control frame
  gtk_container_add(GTK_CONTAINER(main_window), main_vbox);

  gtk_timeout_add(500, main_timer, main_window);
  gtk_widget_show(main_window);

  gtk_main();
  exit(0);
}

/* end gui_main.cpp */
