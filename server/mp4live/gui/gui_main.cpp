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

/*
 * gui_main.cpp - Contains the gtk based gui 
 */
#define __STDC_LIMIT_MACROS
#include "mp4live.h"
#include "mp4live_gui.h"
#include "gdk/gdkx.h"

CLiveConfig* MyConfig;
CAVMediaFlow* AVFlow;

/* Local variables */
static GtkWidget *main_window;
static GtkWidget *main_hbox;
static GtkWidget *main_vbox1;
static GtkWidget *main_vbox2;
static GtkWidget *video_preview;

static GtkWidget *video_enabled_button;
static GtkWidget *video_preview_button;
static GtkWidget *video_settings_label1;
static GtkWidget *video_settings_label2;
static GtkWidget *video_settings_label3;
static GtkWidget *video_settings_button;

static GtkWidget *audio_enabled_button;
static GtkWidget *audio_mute_button;
static GtkWidget *audio_settings_label;
static GtkWidget *audio_settings_button;

static GtkWidget *record_enabled_button;
static GtkWidget *record_settings_label;
static GtkWidget *record_settings_button;

static GtkWidget *transmit_enabled_button;
static GtkWidget *transmit_settings_label;
static GtkWidget *transmit_settings_button;

static GtkWidget *start_button;
static GtkWidget *start_button_label;
static GtkWidget *duration_spinner;
static GtkWidget *duration_units_menu;

static u_int16_t durationUnitsValues[] = {
	1, 60, 3600
};
static char* durationUnitsNames[] = {
	"Seconds", "Minutes", "Hours"
};
static u_int8_t durationUnitsIndex = 1;

static GtkWidget *start_time_label;
static GtkWidget *start_time;
static GtkWidget *start_time_units;

static GtkWidget *duration_label;
static GtkWidget *duration;
static GtkWidget *duration_units;

static GtkWidget *current_time_label;
static GtkWidget *current_time;
static GtkWidget *current_time_units;

static GtkWidget *current_size_label;
static GtkWidget *current_size;
static GtkWidget *current_size_units;

static GtkWidget *actual_fps_label;
static GtkWidget *actual_fps;
static GtkWidget *actual_fps_units;

static Timestamp StartTime;
static Timestamp StopTime;
static u_int32_t StartEncodedFrameNumber;

/*
 * delete_event - called when window closed
 */
static void delete_event (GtkWidget *widget, gpointer *data)
{
	gtk_main_quit();
}

void NewVideoWindow()
{
	// We use the Gtk Preview widget to get the right type of window created
	// and then hand it over to SDL to actually do the blitting

	if (video_preview != NULL) {
		gtk_container_remove(GTK_CONTAINER(main_vbox1), 
			GTK_WIDGET(video_preview));
		video_preview = NULL;
	}

	video_preview = gtk_preview_new(GTK_PREVIEW_COLOR);

	gtk_preview_size(GTK_PREVIEW(video_preview), 
		MyConfig->m_videoWidth, MyConfig->m_videoHeight);

	gtk_widget_show(video_preview);

	gtk_box_pack_start(GTK_BOX(main_vbox1), video_preview, FALSE, FALSE, 5);

	// Let video source know which window to draw into
	gtk_widget_realize(video_preview);	// so XCreateWindow() is called

	if (video_preview->window) {
		MyConfig->m_videoPreviewWindowId = 
			GDK_WINDOW_XWINDOW(video_preview->window);
	}
}

void DisplayVideoSettings(void)
{
	char buffer[256];

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_enabled_button),
		MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE));
  
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_preview_button),
		MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW));

	snprintf(buffer, sizeof(buffer), " MPEG-4 at %u Kbps",
		MyConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE));
	gtk_label_set_text(GTK_LABEL(video_settings_label1), buffer);
	gtk_widget_show(video_settings_label1);

	snprintf(buffer, sizeof(buffer), " %ux%u at %u fps", 
		MyConfig->m_videoWidth,
		MyConfig->m_videoHeight,
		MyConfig->GetIntegerValue(CONFIG_VIDEO_FRAME_RATE));
	gtk_label_set_text(GTK_LABEL(video_settings_label2), buffer);
	gtk_widget_show(video_settings_label2);
}

void DisplayAudioSettings(void)
{
	char buffer[256];
 
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_enabled_button),
		MyConfig->GetBoolValue(CONFIG_AUDIO_ENABLE));
  
	snprintf(buffer, sizeof(buffer), " MP3 at %u Kbps",
		MyConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE));
	gtk_label_set_text(GTK_LABEL(audio_settings_label), buffer);
	gtk_widget_show(audio_settings_label);
}

void DisplayTransmitSettings (void)
{
	char buffer[256];
	const char *addr = MyConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(transmit_enabled_button),
		MyConfig->GetBoolValue(CONFIG_RTP_ENABLE));
  
	if (addr == NULL) {
		gtk_label_set_text(GTK_LABEL(transmit_settings_label), 
			"RTP/UDP to <Automatic Multicast>");
	} else {
		snprintf(buffer, sizeof(buffer), " RTP/UDP to %s", addr); 
		gtk_label_set_text(GTK_LABEL(transmit_settings_label), buffer);
	}
	gtk_widget_show(transmit_settings_label);
}

void DisplayRecordingSettings(void)
{
  char *fileName;
  char buffer[256];

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(record_enabled_button),
	MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE));
  
  if (MyConfig->GetBoolValue(CONFIG_RECORD_MP4)) {
	fileName = strrchr(
		MyConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME), '/');
	if (!fileName) {
		fileName = MyConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME);
	} else {
		fileName++;
	}
    snprintf(buffer, sizeof(buffer), " %s", fileName);
	gtk_label_set_text(GTK_LABEL(record_settings_label), buffer);
  } else {
    gtk_label_set_text(GTK_LABEL(record_settings_label), 
		" No file specified");
  }
  gtk_widget_show(record_settings_label);
}

static void on_video_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->SetBoolValue(CONFIG_VIDEO_ENABLE,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		if (MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
			AVFlow->StartVideoPreview();
		}
	} else {
		AVFlow->StopVideoPreview();
	}
}

static void on_video_preview_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	
	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)
	  && MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
		AVFlow->StartVideoPreview();
	} else {
		// whether preview is running or not
		AVFlow->StopVideoPreview();
	}
}

static void on_video_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateVideoDialog();
}

static void on_audio_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->SetBoolValue(CONFIG_AUDIO_ENABLE,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void on_audio_mute_button (GtkWidget *widget, gpointer *data)
{
	AVFlow->SetAudioOutput(
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void on_audio_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateAudioDialog();
}

static void on_record_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->SetBoolValue(CONFIG_RECORD_ENABLE,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void on_record_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateRecordingDialog();
}

static void on_transmit_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->SetBoolValue(CONFIG_RTP_ENABLE, 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void on_transmit_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateTransmitDialog();
}

static void LockoutChanges(bool lockout)
{
	gtk_widget_set_sensitive(GTK_WIDGET(video_enabled_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(video_settings_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(audio_enabled_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(audio_settings_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(record_enabled_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(record_settings_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(transmit_enabled_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(transmit_settings_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(duration_spinner), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(duration_units_menu), !lockout);
}

static guint status_timer_id;

// forward declaration
static void on_start_button (GtkWidget *widget, gpointer *data);

/*
 * Status timer routine
 */
static gint status_timer (gpointer raw)
{
	time_t secs;
	const struct tm *local;
	char buffer[80];

	Timestamp now = GetTimestamp();
	secs = (time_t)GetSecsFromTimestamp(now);
	local = localtime(&secs);
	strftime(buffer, sizeof(buffer), "%l:%M:%S", local);
	gtk_label_set_text(GTK_LABEL(current_time), buffer);
	gtk_widget_show(current_time);

	strftime(buffer, sizeof(buffer), "%p", local);
	gtk_label_set_text(GTK_LABEL(current_time_units), buffer);
	gtk_widget_show(current_time);

	time_t duration_secs = (time_t)GetSecsFromTimestamp(now - StartTime);

	if (duration_secs <= 2) {
		secs = (time_t)GetSecsFromTimestamp(StartTime);
		local = localtime(&secs);
		strftime(buffer, sizeof(buffer), "%l:%M:%S", local);
		gtk_label_set_text(GTK_LABEL(start_time), buffer);
		gtk_widget_show(start_time);

		strftime(buffer, sizeof(buffer), "%p", local);
		gtk_label_set_text(GTK_LABEL(start_time_units), buffer);
		gtk_widget_show(start_time_units);
	}

	snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", 
		duration_secs / 3600, (duration_secs % 3600) / 60, duration_secs % 60);
	gtk_label_set_text(GTK_LABEL(duration), buffer);
	gtk_widget_show(duration);

	if (MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
		struct stat stats;
		stat(MyConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME), &stats);
		snprintf(buffer, sizeof(buffer), " %lu", stats.st_size / 1000000);
		gtk_label_set_text(GTK_LABEL(current_size), buffer);
		gtk_widget_show(current_size);

		gtk_label_set_text(GTK_LABEL(current_size_units), "MB");
		gtk_widget_show(current_size_units);
	}

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		u_int32_t encodedFrames;
		AVFlow->GetStatus(FLOW_STATUS_VIDEO_ENCODED_FRAMES, &encodedFrames);
		u_int32_t totalFrames = encodedFrames - StartEncodedFrameNumber;

		snprintf(buffer, sizeof(buffer), " %u", (u_int32_t)
			(((float)totalFrames / (float)duration_secs) + 0.5));
		gtk_label_set_text(GTK_LABEL(actual_fps), buffer);
		gtk_widget_show(actual_fps);

		gtk_label_set_text(GTK_LABEL(actual_fps_units), "fps");
		gtk_widget_show(actual_fps_units);
	}

	if (now >= StopTime) {
		// automatically "press" stop button
		on_start_button(start_button, NULL);

		if (MyConfig->m_appAutomatic) {
			// In automatic mode, time to exit app
			gtk_main_quit();
		} else {
			// Make sure user knows that were done
			char *notice;

			if (MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE)
			  && MyConfig->GetBoolValue(CONFIG_RTP_ENABLE)) {
				notice = "Recording and transmission completed";
			} else if (MyConfig->GetBoolValue(CONFIG_RTP_ENABLE)) {
				notice = "Transmission completed";
			} else {
				notice = "Recording completed";
			}
			ShowMessage("Duration Elapsed", notice);
		}

		return (FALSE);
	}

	return (TRUE);  // keep timer going
}

static void on_start_button (GtkWidget *widget, gpointer *data)
{
	static bool started = false;

	if (!started) {
		if (!MyConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
		&& !MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
			ShowMessage("Can't record", "Neither audio nor video are enabled");
			return;
		}

		// lock out change to settings while media is running
		LockoutChanges(true);

		AVFlow->Start();

		StartTime = GetTimestamp(); 

		if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
			AVFlow->GetStatus(FLOW_STATUS_VIDEO_ENCODED_FRAMES, 
				&StartEncodedFrameNumber);
		}

		StopTime = StartTime +
			gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(duration_spinner))
			* durationUnitsValues[durationUnitsIndex] * TimestampTicks;

		gtk_label_set_text(GTK_LABEL(start_button_label), "  Stop  ");

		status_timer_id = gtk_timeout_add(1000, status_timer, main_window);

		started = true;

	} else {
		AVFlow->Stop();

		gtk_timeout_remove(status_timer_id);

		// unlock changes to settings
		LockoutChanges(false);

		gtk_label_set_text(GTK_LABEL(start_button_label), "  Start  ");

		started = false;
	}
}

static void on_duration_changed(GtkWidget* widget, gpointer* data)
{
	MyConfig->SetIntegerValue(CONFIG_APP_DURATION,
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(duration_spinner)));
}

static void on_duration_units_menu_activate(GtkWidget *widget, gpointer data)
{
	durationUnitsIndex = (unsigned int)data & 0xFF;;
	MyConfig->SetIntegerValue(CONFIG_APP_DURATION_UNITS,
		durationUnitsValues[durationUnitsIndex]);
}

static gfloat frameLabelAlignment = 0.025;

static void LayoutVideoFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Video");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 5);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);

	// create first row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
  
	// enabled button
	video_enabled_button = gtk_check_button_new_with_label("Enabled");
	gtk_box_pack_start(GTK_BOX(hbox), video_enabled_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(video_enabled_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_video_enabled_button),
		NULL);
	gtk_widget_show(video_enabled_button);

	// preview button
	video_preview_button = gtk_check_button_new_with_label("Preview");
	gtk_box_pack_start(GTK_BOX(hbox), video_preview_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(video_preview_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_video_preview_button),
		NULL);
	gtk_widget_show(video_preview_button);

	// create second row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	// secondary vbox for two labels
	GtkWidget *vbox2 = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox2);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 5);

	video_settings_label1 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(video_settings_label1), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2), video_settings_label1, TRUE, TRUE, 0);

	video_settings_label2 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(video_settings_label2), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox2), video_settings_label2, TRUE, TRUE, 0);

	// secondary vbox to match stacked labels
	GtkWidget* vbox3 = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox3);
	gtk_box_pack_start(GTK_BOX(hbox), vbox3, FALSE, FALSE, 5);

	// settings button
	video_settings_button = gtk_button_new_with_label(" Settings... ");
	gtk_box_pack_start(GTK_BOX(vbox3), video_settings_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(video_settings_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_video_settings_button),
		NULL);
	gtk_widget_show(video_settings_button);

	// empty label to get sizing correct
	video_settings_label3 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox3), video_settings_label3, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(frame);
}

static void LayoutAudioFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Audio");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 5);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);

	// create first row, homogenous
	hbox = gtk_hbox_new(TRUE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
  
	// enabled button
	audio_enabled_button = gtk_check_button_new_with_label("Enabled");
	gtk_box_pack_start(GTK_BOX(hbox), audio_enabled_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(audio_enabled_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_audio_enabled_button),
		NULL);
	gtk_widget_show(audio_enabled_button);

	// mute button
	audio_mute_button = gtk_check_button_new_with_label("Mute");
	gtk_box_pack_start(GTK_BOX(hbox), audio_mute_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(audio_mute_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_audio_mute_button),
		NULL);
	gtk_widget_show(audio_mute_button);

	// create second row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	// settings summary
	audio_settings_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(audio_settings_label), 0.0, 0.5);
	gtk_widget_show(audio_settings_label);
	gtk_box_pack_start(GTK_BOX(hbox), audio_settings_label, TRUE, TRUE, 0);

	// settings button
	audio_settings_button = gtk_button_new_with_label(" Settings... ");
	gtk_box_pack_start(GTK_BOX(hbox), audio_settings_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(audio_settings_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_audio_settings_button),
		NULL);
	gtk_widget_show(audio_settings_button);

	// finalize
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(frame);
}

static void LayoutRecordingFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Recording");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 5);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);

	// create first row, homogenous
	hbox = gtk_hbox_new(TRUE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
  
	// enabled button
	record_enabled_button = gtk_check_button_new_with_label("Enabled");
	gtk_box_pack_start(GTK_BOX(hbox), record_enabled_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(record_enabled_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_record_enabled_button),
		NULL);
	gtk_widget_show(record_enabled_button);

	// create second row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	// settings summary
	record_settings_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(record_settings_label), 0.0, 0.5);
	gtk_widget_show(record_settings_label);
	gtk_box_pack_start(GTK_BOX(hbox), record_settings_label, TRUE, TRUE, 0);

	// settings button
	record_settings_button = gtk_button_new_with_label(" Settings... ");
	gtk_box_pack_start(GTK_BOX(hbox), record_settings_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(record_settings_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_record_settings_button),
		NULL);
	gtk_widget_show(record_settings_button);

	// finalize
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(frame);
}

static void LayoutTransmitFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Transmission");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 5);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);

	// create first row, homogenous
	hbox = gtk_hbox_new(TRUE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
  
	// enabled button
	transmit_enabled_button = gtk_check_button_new_with_label("Enabled");
	gtk_box_pack_start(GTK_BOX(hbox), transmit_enabled_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(transmit_enabled_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_transmit_enabled_button),
		NULL);
	gtk_widget_show(transmit_enabled_button);

	// create second row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	// settings summary
	transmit_settings_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(transmit_settings_label), 0.0, 0.5);
	gtk_widget_show(transmit_settings_label);
	gtk_box_pack_start(GTK_BOX(hbox), transmit_settings_label, TRUE, TRUE, 0);

	// settings button
	transmit_settings_button = gtk_button_new_with_label(" Settings... ");
	gtk_box_pack_start(GTK_BOX(hbox), transmit_settings_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(transmit_settings_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_transmit_settings_button),
		NULL);
	gtk_widget_show(transmit_settings_button);

	// finalize
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(frame);
}

// Control frame
void LayoutControlFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkObject* adjustment;

	frame = gtk_frame_new("Control");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_end(GTK_BOX(box), frame, FALSE, FALSE, 5);

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	label = gtk_label_new(" Duration:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_APP_DURATION),
		1, 24 * 60 * 60, 1, 0, 0);
	duration_spinner = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
	gtk_signal_connect(GTK_OBJECT(duration_spinner),
		"changed",
		GTK_SIGNAL_FUNC(on_duration_changed),
		GTK_OBJECT(duration_spinner));
	gtk_widget_show(duration_spinner);
	gtk_box_pack_start(GTK_BOX(hbox), duration_spinner, FALSE, FALSE, 5);

	durationUnitsIndex = 0; 
	for (u_int8_t i = 0; 
	  i < sizeof(durationUnitsValues) / sizeof(u_int16_t); i++) {
		if (MyConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS) 
		  == durationUnitsValues[i]) {
			durationUnitsIndex = i;
			break;
		}
	}
	duration_units_menu = CreateOptionMenu(NULL,
		durationUnitsNames, 
		sizeof(durationUnitsNames) / sizeof(char*),
		durationUnitsIndex,
		GTK_SIGNAL_FUNC(on_duration_units_menu_activate));
	gtk_box_pack_start(GTK_BOX(hbox), duration_units_menu, FALSE, FALSE, 5);

	// vertical separator
	GtkWidget* sep = gtk_vseparator_new();
	gtk_widget_show(sep);
	gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 0);

	start_button = gtk_button_new();
	start_button_label = gtk_label_new("  Start  ");
	gtk_misc_set_alignment(GTK_MISC(start_button_label), 0.5, 0.5);
	gtk_container_add(GTK_CONTAINER(start_button), start_button_label);
	gtk_widget_show(start_button_label);
	gtk_box_pack_start(GTK_BOX(hbox), start_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(start_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_start_button),
		NULL);
	gtk_widget_show(start_button);

	gtk_widget_show(frame); // show control frame
}

// Status frame
void LayoutStatusFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Status");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_end(GTK_BOX(box), frame, TRUE, TRUE, 5);

	// first row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	// vbox for labels
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	start_time_label = gtk_label_new(" Start Time:");
	gtk_misc_set_alignment(GTK_MISC(start_time_label), 0.0, 0.5);
	gtk_widget_show(start_time_label);
	gtk_box_pack_start(GTK_BOX(vbox), start_time_label, TRUE, TRUE, 0);

	duration_label = gtk_label_new(" Current Duration:");
	gtk_misc_set_alignment(GTK_MISC(duration_label), 0.0, 0.5);
	gtk_widget_show(duration_label);
	gtk_box_pack_start(GTK_BOX(vbox), duration_label, TRUE, TRUE, 0);

	current_time_label = gtk_label_new(" Current Time:");
	gtk_misc_set_alignment(GTK_MISC(current_time_label), 0.0, 0.5);
	gtk_widget_show(current_time_label);
	gtk_box_pack_start(GTK_BOX(vbox), current_time_label, TRUE, TRUE, 0);

	current_size_label = gtk_label_new(" Current Size:");
	gtk_misc_set_alignment(GTK_MISC(current_size_label), 0.0, 0.5);
	gtk_widget_show(current_size_label);
	gtk_box_pack_start(GTK_BOX(vbox), current_size_label, TRUE, TRUE, 0);

	actual_fps_label = gtk_label_new(" Video Frame Rate:");
	gtk_misc_set_alignment(GTK_MISC(actual_fps_label), 0.0, 0.5);
	gtk_widget_show(actual_fps_label);
	gtk_box_pack_start(GTK_BOX(vbox), actual_fps_label, TRUE, TRUE, 0);

	// vbox for values
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	start_time = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(start_time), 1.0, 0.5);
	gtk_widget_show(start_time);
	gtk_box_pack_start(GTK_BOX(vbox), start_time, TRUE, TRUE, 0);

	duration = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(duration), 1.0, 0.5);
	gtk_widget_show(duration);
	gtk_box_pack_start(GTK_BOX(vbox), duration, TRUE, TRUE, 0);

	current_time = gtk_label_new("                 ");
	gtk_misc_set_alignment(GTK_MISC(current_time), 1.0, 0.5);
	gtk_widget_show(current_time);
	gtk_box_pack_start(GTK_BOX(vbox), current_time, TRUE, TRUE, 0);

	current_size = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_size), 1.0, 0.5);
	gtk_widget_show(current_size);
	gtk_box_pack_start(GTK_BOX(vbox), current_size, TRUE, TRUE, 0);

	actual_fps = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(actual_fps), 1.0, 0.5);
	gtk_widget_show(actual_fps);
	gtk_box_pack_start(GTK_BOX(vbox), actual_fps, TRUE, TRUE, 0);

	// vbox for units
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	start_time_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(start_time_units), 1.0, 0.5);
	gtk_widget_show(start_time_units);
	gtk_box_pack_start(GTK_BOX(vbox), start_time_units, TRUE, TRUE, 0);

	duration_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(duration_units), 1.0, 0.5);
	gtk_widget_show(duration_units);
	gtk_box_pack_start(GTK_BOX(vbox), duration_units, TRUE, TRUE, 0);

	current_time_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_time_units), 1.0, 0.5);
	gtk_widget_show(current_time_units);
	gtk_box_pack_start(GTK_BOX(vbox), current_time_units, TRUE, TRUE, 0);

	current_size_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_size_units), 1.0, 0.5);
	gtk_widget_show(current_size_units);
	gtk_box_pack_start(GTK_BOX(vbox), current_size_units, TRUE, TRUE, 0);

	actual_fps_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(actual_fps_units), 1.0, 0.5);
	gtk_widget_show(actual_fps_units);
	gtk_box_pack_start(GTK_BOX(vbox), actual_fps_units, TRUE, TRUE, 0);

	gtk_widget_show(frame); // show control frame
}

/*
 * Main routine - set up window
 */
int gui_main(int argc, char **argv, CLiveConfig* pConfig)
{
	MyConfig = pConfig;

	AVFlow = new CAVMediaFlow(pConfig);

	gtk_init(&argc, &argv);

	// Setup main window
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy(GTK_WINDOW(main_window), FALSE, TRUE, FALSE);
	char buffer[80];
	snprintf(buffer, sizeof(buffer), "cisco mp4live %s", VERSION);
	gtk_window_set_title(GTK_WINDOW(main_window), buffer);
	gtk_signal_connect(GTK_OBJECT(main_window),
		"delete_event",
		GTK_SIGNAL_FUNC(delete_event),
		NULL);

	// main boxes
	main_hbox = gtk_hbox_new(FALSE, 1);
	gtk_container_set_border_width(GTK_CONTAINER(main_hbox), 4);
	gtk_widget_show(main_hbox);
	gtk_container_add(GTK_CONTAINER(main_window), main_hbox);

	main_vbox1 = gtk_vbox_new(FALSE, 1);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox1), 4);
	gtk_widget_show(main_vbox1);
	gtk_box_pack_start(GTK_BOX(main_hbox), main_vbox1, FALSE, FALSE, 5);

	main_vbox2 = gtk_vbox_new(TRUE, 1);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox2), 4);
	gtk_widget_show(main_vbox2);
	gtk_box_pack_start(GTK_BOX(main_hbox), main_vbox2, TRUE, TRUE, 5);

	// Video Preview
	NewVideoWindow();
	
	// Video Frame
	LayoutVideoFrame(main_vbox2);
	DisplayVideoSettings();

	// Audio Frame
	LayoutAudioFrame(main_vbox2);
	DisplayAudioSettings();

	// Recording Frame
	LayoutRecordingFrame(main_vbox2);
	DisplayRecordingSettings();

	// Transmission Frame
	LayoutTransmitFrame(main_vbox2);
	DisplayTransmitSettings();

	// Status frame
	LayoutStatusFrame(main_vbox1);

	// Control frame
	LayoutControlFrame(main_vbox1);

	gtk_widget_show(main_window);

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)
	  && MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
		AVFlow->StartVideoPreview();
	}

	// "press" start button
	if (MyConfig->m_appAutomatic) {
		on_start_button(start_button, NULL);
	}

	gtk_main();

	delete AVFlow;

	return 0;
}

/* end gui_main.cpp */
