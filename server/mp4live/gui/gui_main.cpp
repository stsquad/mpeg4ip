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
#include "preview_flow.h"
#include "gdk/gdkx.h"

CLiveConfig* MyConfig;
CPreviewAVMediaFlow* AVFlow;

/* Local variables */
static bool started = false;

static GtkWidget *main_window;
static GtkWidget *main_hbox;
static GtkWidget *main_vbox1;
static GtkWidget *main_vbox2;
static GtkWidget *video_preview;

static GtkWidget *video_enabled_button;
static GSList	 *video_preview_radio_group;
static GtkWidget *video_none_preview_button;
static GtkWidget *video_raw_preview_button;
static GtkWidget *video_encoded_preview_button;
static GtkWidget *video_settings_label1;
static GtkWidget *video_settings_label2;
static GtkWidget *video_settings_label3;
static GtkWidget *video_settings_button;
static GtkWidget *picture_settings_button;

static GtkWidget *audio_enabled_button;
static GtkWidget *audio_mute_button;
static GtkWidget *audio_settings_label1;
static GtkWidget *audio_settings_label2;
static GtkWidget *audio_settings_label3;
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

static GtkWidget *config_file_entry;
static GtkWidget *load_config_button;
static GtkWidget *save_config_button;

static u_int32_t durationUnitsValues[] = {
	1, 60, 3600, 3600*24, 3600*24*365
};
static char* durationUnitsNames[] = {
	"Seconds", "Minutes", "Hours", "Days", "Years"
};
static u_int8_t durationUnitsIndex = 1;

static GtkWidget *media_source_label;
static GtkWidget *media_source;

static GtkWidget *start_time_label;
static GtkWidget *start_time;
static GtkWidget *start_time_units;

static GtkWidget *duration_label;
static GtkWidget *duration;
static GtkWidget *duration_units;

static GtkWidget *current_time_label;
static GtkWidget *current_time;
static GtkWidget *current_time_units;

static GtkWidget *finish_time_label;
static GtkWidget *finish_time;
static GtkWidget *finish_time_units;

static GtkWidget *current_size_label;
static GtkWidget *current_size;
static GtkWidget *current_size_units;

static GtkWidget *final_size_label;
static GtkWidget *final_size;
static GtkWidget *final_size_units;

static GtkWidget *actual_fps_label;
static GtkWidget *actual_fps;
static GtkWidget *actual_fps_units;

static Timestamp StartTime;
static Timestamp StopTime;
static Duration FlowDuration;
static u_int32_t StartEncodedFrameNumber;
static u_int64_t StartFileSize;
static u_int64_t StopFileSize;
SDL_mutex *dialog_mutex;
/*
 * delete_event - called when window closed
 */
static void delete_event (GtkWidget *widget, gpointer *data)
{
  // stop the flow (which gets rid of the preview, before we gtk_main_quit
  AVFlow->Stop();
  delete AVFlow;
  SDL_DestroyMutex(dialog_mutex);
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
  
	gtk_widget_set_sensitive(GTK_WIDGET(picture_settings_button),
		MyConfig->IsCaptureVideoSource());
  
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(video_none_preview_button), 
		!MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW));

	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(video_raw_preview_button), 
		MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)
		&& MyConfig->GetBoolValue(CONFIG_VIDEO_RAW_PREVIEW));

	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(video_encoded_preview_button), 
		MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)
		&& MyConfig->GetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW));

	snprintf(buffer, sizeof(buffer), " %s(%s) at %u kbps",
		 MyConfig->GetStringValue(CONFIG_VIDEO_ENCODING),
		 MyConfig->GetStringValue(CONFIG_VIDEO_ENCODER),
		 MyConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE));
	gtk_label_set_text(GTK_LABEL(video_settings_label1), buffer);
	gtk_widget_show(video_settings_label1);

	snprintf(buffer, sizeof(buffer), " %u x %u @ %.2f fps", 
		MyConfig->m_videoWidth,
		MyConfig->m_videoHeight,
		MyConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE));
	gtk_label_set_text(GTK_LABEL(video_settings_label2), buffer);
	gtk_widget_show(video_settings_label2);
}

void DisplayAudioSettings(void)
{
	char buffer[256];
 
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_enabled_button),
		MyConfig->GetBoolValue(CONFIG_AUDIO_ENABLE));
  
	snprintf(buffer, sizeof(buffer), " %s(%s) at %u bps",
		 MyConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
		 MyConfig->GetStringValue(CONFIG_AUDIO_ENCODER),
		MyConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE));
	gtk_label_set_text(GTK_LABEL(audio_settings_label1), buffer);
	gtk_widget_show(audio_settings_label1);

	snprintf(buffer, sizeof(buffer), " %s @ %u Hz",
		(MyConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) == 2
			? "Stereo" : "Mono"),
		MyConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));
	gtk_label_set_text(GTK_LABEL(audio_settings_label2), buffer);
	gtk_widget_show(audio_settings_label2);
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
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(record_enabled_button),
		MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE));

	const char *fname;
	AVFlow->GetStatus(FLOW_STATUS_FILENAME, &fname);
	if (fname == NULL) {
	  fname = MyConfig->GetStringValue(CONFIG_RECORD_MP4_FILE_NAME);
	}
	const char* fileName = strrchr(fname,'/');
	if (!fileName) {
	  fileName = fname;
	} else {
		fileName++;
	}

	char buffer[256];
    snprintf(buffer, sizeof(buffer), " %s", fileName);
	gtk_label_set_text(GTK_LABEL(record_settings_label), buffer);
	gtk_widget_show(record_settings_label);
}

void DisplayControlSettings(void)
{
	// duration
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(duration_spinner),
		(gfloat)MyConfig->GetIntegerValue(CONFIG_APP_DURATION));

	// duration units
	for (u_int8_t i = 0; 
	  i < sizeof(durationUnitsValues) / sizeof(*durationUnitsValues); i++) {
		if (MyConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS) 
		  == durationUnitsValues[i]) {
			durationUnitsIndex = i;
			break;
		}
	}
	gtk_option_menu_set_history(
		GTK_OPTION_MENU(duration_units_menu), 
		durationUnitsIndex);
	gtk_widget_show(duration_units_menu);
}

void DisplayStatusSettings(void)
{
	// media source
	char buffer[128];

	buffer[0] = '\0';

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)
	  && MyConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
	  && strcmp(MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME),
		MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME))) {

		snprintf(buffer, sizeof(buffer), "%s & %s",
			MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME),
			MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));
		gtk_label_set_text(GTK_LABEL(media_source), buffer);

	} else if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		gtk_label_set_text(GTK_LABEL(media_source),
			MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME));

	} else if (MyConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)) {
		gtk_label_set_text(GTK_LABEL(media_source),
			MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));
	}
}

void DisplayAllSettings()
{
	DisplayVideoSettings();
	DisplayAudioSettings();
	DisplayRecordingSettings();
	DisplayTransmitSettings();
	DisplayControlSettings();
	DisplayStatusSettings();
}

void DisplayFinishTime(Timestamp t)
{
	time_t secs;
	const struct tm *local;
	char buffer[128];

	secs = (time_t)GetSecsFromTimestamp(t);
	local = localtime(&secs);
	strftime(buffer, sizeof(buffer), "%l:%M:%S", local);
	gtk_label_set_text(GTK_LABEL(finish_time), buffer);
	gtk_widget_show(finish_time);

	strftime(buffer, sizeof(buffer), "%p", local);
	gtk_label_set_text(GTK_LABEL(finish_time_units), buffer);
	gtk_widget_show(finish_time_units);
}

static void on_video_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->SetBoolValue(CONFIG_VIDEO_ENABLE,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		AVFlow->StartVideoPreview();
	} else {
		AVFlow->StopVideoPreview();
	}
}

static void on_video_preview_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW,
		!gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(video_none_preview_button)));

	MyConfig->SetBoolValue(CONFIG_VIDEO_RAW_PREVIEW,
		gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(video_raw_preview_button)));

	MyConfig->SetBoolValue(CONFIG_VIDEO_ENCODED_PREVIEW,
		gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(video_encoded_preview_button)));

	MyConfig->UpdateVideo();
}

static void on_video_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateVideoDialog();
}

static void on_picture_settings_button (GtkWidget *widget, gpointer *data)
{
	CreatePictureDialog();
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
	gtk_widget_set_sensitive(GTK_WIDGET(config_file_entry), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(load_config_button), !lockout);
	gtk_widget_set_sensitive(GTK_WIDGET(save_config_button), !lockout);
}

static guint status_timer_id;

// forward declaration
static void on_start_button (GtkWidget *widget, gpointer *data);

static void status_start()
{
	time_t secs;
	const struct tm *local;
	char buffer[128];

	// start time
	secs = (time_t)GetSecsFromTimestamp(StartTime);
	local = localtime(&secs);
	strftime(buffer, sizeof(buffer), "%l:%M:%S", local);
	gtk_label_set_text(GTK_LABEL(start_time), buffer);
	gtk_widget_show(start_time);

	strftime(buffer, sizeof(buffer), "%p", local);
	gtk_label_set_text(GTK_LABEL(start_time_units), buffer);
	gtk_widget_show(start_time_units);

	// finish time
	if (StopTime) {
		DisplayFinishTime(StopTime);
	}

	// file size
	StartFileSize = 0;
	StopFileSize = 0;

	if (MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
		snprintf(buffer, sizeof(buffer), " "U64,
			StartFileSize / TO_U64(1000000));
		gtk_label_set_text(GTK_LABEL(current_size), buffer);
		gtk_widget_show(current_size);

		gtk_label_set_text(GTK_LABEL(current_size_units), "MB");
		gtk_widget_show(current_size_units);

		StopFileSize = MyConfig->m_recordEstFileSize;
		snprintf(buffer, sizeof(buffer), " "U64,
			StopFileSize / TO_U64(1000000));
		gtk_label_set_text(GTK_LABEL(final_size), buffer);
		gtk_widget_show(final_size);

		gtk_label_set_text(GTK_LABEL(final_size_units), "MB");
		gtk_widget_show(final_size_units);
	}
}

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

	snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", 
		duration_secs / 3600, (duration_secs % 3600) / 60, duration_secs % 60);
	gtk_label_set_text(GTK_LABEL(duration), buffer);
	gtk_widget_show(duration);

	if (MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
	  const char *fname;
	  AVFlow->GetStatus(FLOW_STATUS_FILENAME, &fname);
		struct stat stats;
		if (stat(fname, &stats) == 0) {
		  uint64_t size = stats.st_size;
		  size /= TO_U64(1000000);
		  snprintf(buffer, sizeof(buffer), " "U64, size);
		} else {
		  snprintf(buffer, sizeof(buffer), "BAD");
		}
		gtk_label_set_text(GTK_LABEL(current_size), buffer);
		gtk_widget_show(current_size);
		if (MyConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS) ==
		    FILE_MP4_CREATE_NEW) {
		  DisplayRecordingSettings();
		}
	}

	if (!StopTime) {
		float progress;
		AVFlow->GetStatus(FLOW_STATUS_PROGRESS, &progress);

		if (progress > 0.0) {
			u_int32_t estDuration = (u_int32_t)(duration_secs / progress);
			DisplayFinishTime(StartTime + (estDuration * TimestampTicks));
		}
	}

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		u_int32_t encodedFrames;
		AVFlow->GetStatus(FLOW_STATUS_VIDEO_ENCODED_FRAMES, &encodedFrames);
		u_int32_t totalFrames = encodedFrames - StartEncodedFrameNumber;

		snprintf(buffer, sizeof(buffer), " %.2f", 
			((float)totalFrames / (float)(now - StartTime)) * TimestampTicks);
		gtk_label_set_text(GTK_LABEL(actual_fps), buffer);
		gtk_widget_show(actual_fps);

		gtk_label_set_text(GTK_LABEL(actual_fps_units), "fps");
		gtk_widget_show(actual_fps_units);
	}

	bool stop = false;

	if (StopTime) {
		stop = (now >= StopTime);
	} 

	if (!stop && duration_secs > 10) {
		AVFlow->GetStatus(FLOW_STATUS_DONE, &stop);
	}
	if (!stop) {
		Duration elapsedDuration;
		if (AVFlow->GetStatus(FLOW_STATUS_DURATION, &elapsedDuration)) {
			if (elapsedDuration >= FlowDuration) {
				stop = true;
			}
		}
	}

	if (stop) {
		// automatically "press" stop button
		DoStop();

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
			ShowMessage("Completed", notice);
		}

		return (FALSE);
	}

	return (TRUE);  // keep timer going
}

void DoStart()
{
	if (!MyConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
	  && !MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		ShowMessage("No Media", "Neither audio nor video are enabled");
		return;
	}

	// lock out change to settings while media is running
	LockoutChanges(true);

	AVFlow->Start();

	StartTime = GetTimestamp(); 

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		if (MyConfig->IsCaptureVideoSource()) {
			AVFlow->GetStatus(FLOW_STATUS_VIDEO_ENCODED_FRAMES, 
				&StartEncodedFrameNumber);
		} else {
			StartEncodedFrameNumber = 0;
		}
	}

	FlowDuration =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(duration_spinner))
		* durationUnitsValues[durationUnitsIndex] * TimestampTicks;

	if (MyConfig->IsFileVideoSource() && MyConfig->IsFileAudioSource()
	  && !MyConfig->GetBoolValue(CONFIG_RTP_ENABLE)) {
		// no real time constraints
		StopTime = 0;
	} else {
		StopTime = StartTime + FlowDuration;
	}

	status_start();

	gtk_label_set_text(GTK_LABEL(start_button_label), "  Stop  ");

	status_timer_id = gtk_timeout_add(1000, status_timer, main_window);

	started = true;
}

void DoStop()
{
	gtk_timeout_remove(status_timer_id);

	if (MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
		char* notice = "CLOSING";

		if (MyConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS)) {
			notice = "HINTING";
		}

		// borrow finish time field
		gtk_label_set_text(GTK_LABEL(finish_time), notice);
		gtk_widget_show_now(finish_time);

		gtk_label_set_text(GTK_LABEL(finish_time_units), "");
		gtk_widget_show_now(finish_time_units);
	}

	AVFlow->Stop();

	if (MyConfig->GetBoolValue(CONFIG_RECORD_ENABLE)) {
		DisplayFinishTime(GetTimestamp());
	}

	// unlock changes to settings
	LockoutChanges(false);

	gtk_label_set_text(GTK_LABEL(start_button_label), "  Start  ");

	started = false;
}

static void on_start_button (GtkWidget *widget, gpointer *data)
{
	if (!started) {
		DoStart();
	} else {
		DoStop();
	}
}

#ifdef HAVE_GTK_2_0
static void on_duration_changed(GtkWidget* widget, gpointer* data)
{
  gtk_spin_button_update(GTK_SPIN_BUTTON(duration_spinner));

	MyConfig->SetIntegerValue(CONFIG_APP_DURATION,
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(duration_spinner)));
	MyConfig->UpdateRecord();
}
#endif

static void on_duration_units_menu_activate(GtkWidget *widget, gpointer data)
{
	durationUnitsIndex =  GPOINTER_TO_UINT(data) & 0xFF;;
	MyConfig->SetIntegerValue(CONFIG_APP_DURATION_UNITS,
		durationUnitsValues[durationUnitsIndex]);
	MyConfig->UpdateRecord();
}

static void LoadConfig()
{
	AVFlow->StopVideoPreview();

	const char* configFileName =
		gtk_entry_get_text(GTK_ENTRY(config_file_entry));

	MyConfig->ReadFromFile(configFileName);

	MyConfig->Update();

	DisplayAllSettings();

	NewVideoWindow();

	// restart video source
	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		AVFlow->StartVideoPreview();
	}
}

static void on_load_config_button (GtkWidget *widget, gpointer *data)
{
	FileBrowser(config_file_entry, GTK_SIGNAL_FUNC(LoadConfig));
}

static void on_save_config_button (GtkWidget *widget, gpointer *data)
{
	const char* configFileName =
		gtk_entry_get_text(GTK_ENTRY(config_file_entry));

	MyConfig->WriteToFile(configFileName);

	char notice[512];
	sprintf(notice,
		"Current configuration saved to %s", configFileName);

	ShowMessage("Configuration Saved", notice);
}

static gfloat frameLabelAlignment = 0.025;

static void LayoutVideoFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox, *vbox1, *vbox2;
	GtkWidget *label;

	frame = gtk_frame_new("Video");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 5);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);

	// create first row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  
	// enabled button
	video_enabled_button = gtk_check_button_new_with_label("Enabled");
	gtk_box_pack_start(GTK_BOX(hbox), video_enabled_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(video_enabled_button), 
		"toggled",
		GTK_SIGNAL_FUNC(on_video_enabled_button),
		NULL);
	gtk_widget_show(video_enabled_button);

	// picture controls button
	picture_settings_button = gtk_button_new_with_label(" Picture... ");
	gtk_box_pack_start(GTK_BOX(hbox), picture_settings_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(picture_settings_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_picture_settings_button),
		NULL);
	gtk_widget_show(picture_settings_button);

	// create second row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);

	// preview label
	label = gtk_label_new(" Preview:");
	gtk_widget_show(label);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	// none preview radio button
	video_none_preview_button = gtk_radio_button_new_with_label(NULL, "None");
	gtk_widget_show(video_none_preview_button);
	gtk_box_pack_start(GTK_BOX(hbox), video_none_preview_button,
		FALSE, FALSE, 0);

	// raw preview radio button
	video_preview_radio_group = 
		gtk_radio_button_group(GTK_RADIO_BUTTON(video_none_preview_button));
	video_raw_preview_button = 
		gtk_radio_button_new_with_label(video_preview_radio_group, "Raw");
	gtk_widget_show(video_raw_preview_button);
	gtk_box_pack_start(GTK_BOX(hbox), video_raw_preview_button,
		FALSE, FALSE, 0);

	// encoded preview radio button
	video_preview_radio_group = 
		gtk_radio_button_group(GTK_RADIO_BUTTON(video_none_preview_button));
	video_encoded_preview_button = 
		gtk_radio_button_new_with_label(video_preview_radio_group, "Encoded");
	gtk_widget_show(video_encoded_preview_button);
	gtk_box_pack_start(GTK_BOX(hbox), video_encoded_preview_button,
		FALSE, FALSE, 0);

	gtk_signal_connect(GTK_OBJECT(video_none_preview_button), 
		"toggled",
		 GTK_SIGNAL_FUNC(on_video_preview_button),
		 NULL);
	gtk_signal_connect(GTK_OBJECT(video_raw_preview_button), 
		"toggled",
		 GTK_SIGNAL_FUNC(on_video_preview_button),
		 NULL);
	gtk_signal_connect(GTK_OBJECT(video_encoded_preview_button), 
		"toggled",
		 GTK_SIGNAL_FUNC(on_video_preview_button),
		 NULL);

	// create third row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	// secondary vbox for two labels
	vbox1 = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox1);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 5);

	video_settings_label1 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(video_settings_label1), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox1), video_settings_label1, TRUE, TRUE, 0);

	video_settings_label2 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(video_settings_label2), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox1), video_settings_label2, TRUE, TRUE, 0);

	// secondary vbox to match stacked labels
	vbox2 = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox2);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 5);

	// settings button
	video_settings_button = gtk_button_new_with_label(" Settings... ");
	gtk_box_pack_start(GTK_BOX(vbox2), video_settings_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(video_settings_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_video_settings_button),
		NULL);
	gtk_widget_show(video_settings_button);

	// empty label to get sizing correct
	video_settings_label3 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox2), video_settings_label3, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(frame);
}

static void LayoutAudioFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox, *vbox1, *vbox2;

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

	// secondary vbox for two labels
	vbox1 = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox1);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, TRUE, TRUE, 5);

	audio_settings_label1 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(audio_settings_label1), 0.0, 0.5);
	gtk_widget_show(audio_settings_label1);
	gtk_box_pack_start(GTK_BOX(vbox1), audio_settings_label1, TRUE, TRUE, 0);

	audio_settings_label2 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(audio_settings_label2), 0.0, 0.5);
	gtk_widget_show(audio_settings_label2);
	gtk_box_pack_start(GTK_BOX(vbox1), audio_settings_label2, TRUE, TRUE, 0);

	// secondary vbox to match stacked labels
	vbox2 = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox2);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 5);

	// settings button
	audio_settings_button = gtk_button_new_with_label(" Settings... ");
	gtk_box_pack_start(GTK_BOX(vbox2), audio_settings_button, FALSE, FALSE, 5);
	gtk_signal_connect(GTK_OBJECT(audio_settings_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_audio_settings_button),
		NULL);
	gtk_widget_show(audio_settings_button);

	// empty label to get sizing correct
	audio_settings_label3 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox2), audio_settings_label3, TRUE, TRUE, 0);

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
	gtk_box_pack_start(GTK_BOX(hbox), 
		transmit_settings_button, FALSE, FALSE, 5);
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
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *separator;
	GtkObject* adjustment;

	frame = gtk_frame_new("Control");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_end(GTK_BOX(box), frame, FALSE, FALSE, 5);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	label = gtk_label_new(" Duration:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_APP_DURATION),
		1, 24 * 60 * 60, 1, 0, 0);
	duration_spinner = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
#ifdef HAVE_GTK_2_0
	gtk_signal_connect(GTK_OBJECT(duration_spinner),
		"value-changed",
		GTK_SIGNAL_FUNC(on_duration_changed),
		GTK_OBJECT(duration_spinner));
#endif
	gtk_widget_show(duration_spinner);
	gtk_box_pack_start(GTK_BOX(hbox), duration_spinner, FALSE, FALSE, 5);

	durationUnitsIndex = 0; // temporary
	duration_units_menu = CreateOptionMenu(NULL,
		durationUnitsNames, 
		sizeof(durationUnitsNames) / sizeof(char*),
		durationUnitsIndex,
		GTK_SIGNAL_FUNC(on_duration_units_menu_activate));
	gtk_box_pack_start(GTK_BOX(hbox), duration_units_menu, FALSE, FALSE, 5);

	// vertical separator
	separator = gtk_vseparator_new();
	gtk_widget_show(separator);
	gtk_box_pack_start(GTK_BOX(hbox), separator, FALSE, FALSE, 0);

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

	// separator
	separator = gtk_hseparator_new();
	gtk_widget_show(separator);
	gtk_box_pack_start(GTK_BOX(vbox), separator, TRUE, TRUE, 0);

	// second row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

	label = gtk_label_new(" Configuration File:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

	config_file_entry = gtk_entry_new_with_max_length(256);
	gtk_entry_set_text(GTK_ENTRY(config_file_entry), 
		MyConfig->GetFileName());
	gtk_widget_show(config_file_entry);
	gtk_box_pack_start(GTK_BOX(hbox), config_file_entry, TRUE, TRUE, 5);
	
	// config file load button
	load_config_button = gtk_button_new_with_label(" Load... ");
	gtk_signal_connect(GTK_OBJECT(load_config_button),
		 "clicked",
		 GTK_SIGNAL_FUNC(on_load_config_button),
		 NULL);
	gtk_widget_show(load_config_button);
	gtk_box_pack_start(GTK_BOX(hbox), load_config_button, FALSE, FALSE, 5);

	// config file save button
	save_config_button = gtk_button_new_with_label(" Save ");
	gtk_signal_connect(GTK_OBJECT(save_config_button),
		 "clicked",
		 GTK_SIGNAL_FUNC(on_save_config_button),
		 NULL);
	gtk_widget_show(save_config_button);
	gtk_box_pack_start(GTK_BOX(hbox), save_config_button, FALSE, FALSE, 5);

	gtk_widget_show(frame); // show control frame
}

// Status frame
void LayoutStatusFrame(GtkWidget* box)
{
	GtkWidget *frame;
	GtkWidget *frame_vbox;
	GtkWidget *vbox, *hbox;
	GtkWidget *separator;

	frame = gtk_frame_new("Status");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_end(GTK_BOX(box), frame, FALSE, FALSE, 5);

	// frame vbox
	frame_vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(frame_vbox);
	gtk_container_add(GTK_CONTAINER(frame), frame_vbox);

	// first row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 2);

	media_source_label = gtk_label_new(" Source:");
	gtk_misc_set_alignment(GTK_MISC(media_source_label), 0.0, 0.5);
	gtk_widget_show(media_source_label);
	gtk_box_pack_start(GTK_BOX(hbox), media_source_label, TRUE, TRUE, 5);

	media_source = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(media_source), 1.0, 0.5);
	gtk_widget_show(media_source);
	gtk_box_pack_start(GTK_BOX(hbox), media_source, TRUE, TRUE, 5);

	// separator
	separator = gtk_hseparator_new();
	gtk_widget_show(separator);
	gtk_box_pack_start(GTK_BOX(frame_vbox), separator, TRUE, TRUE, 0);

	// second row

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 2);

	// vbox for time labels
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

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

	finish_time_label = gtk_label_new(" Finish Time:");
	gtk_misc_set_alignment(GTK_MISC(finish_time_label), 0.0, 0.5);
	gtk_widget_show(finish_time_label);
	gtk_box_pack_start(GTK_BOX(vbox), finish_time_label, TRUE, TRUE, 0);

	// vbox for time values
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

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

	finish_time = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(finish_time), 1.0, 0.5);
	gtk_widget_show(finish_time);
	gtk_box_pack_start(GTK_BOX(vbox), finish_time, TRUE, TRUE, 0);

	// vbox for time units
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

	finish_time_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(finish_time_units), 1.0, 0.5);
	gtk_widget_show(finish_time_units);
	gtk_box_pack_start(GTK_BOX(vbox), finish_time_units, TRUE, TRUE, 0);

	// separator
	separator = gtk_hseparator_new();
	gtk_widget_show(separator);
	gtk_box_pack_start(GTK_BOX(frame_vbox), separator, TRUE, TRUE, 0);

	// third row

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 2);

	// vbox for size labels
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	current_size_label = gtk_label_new(" Current Size:");
	gtk_misc_set_alignment(GTK_MISC(current_size_label), 0.0, 0.5);
	gtk_widget_show(current_size_label);
	gtk_box_pack_start(GTK_BOX(vbox), current_size_label, TRUE, TRUE, 0);

	final_size_label = gtk_label_new(" Estimated Final Size:");
	gtk_misc_set_alignment(GTK_MISC(final_size_label), 0.0, 0.5);
	gtk_widget_show(final_size_label);
	gtk_box_pack_start(GTK_BOX(vbox), final_size_label, TRUE, TRUE, 0);

	// vbox for size values
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	current_size = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_size), 1.0, 0.5);
	gtk_widget_show(current_size);
	gtk_box_pack_start(GTK_BOX(vbox), current_size, TRUE, TRUE, 0);

	final_size = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(final_size), 1.0, 0.5);
	gtk_widget_show(final_size);
	gtk_box_pack_start(GTK_BOX(vbox), final_size, TRUE, TRUE, 0);

	// vbox for size units
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	current_size_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_size_units), 1.0, 0.5);
	gtk_widget_show(current_size_units);
	gtk_box_pack_start(GTK_BOX(vbox), current_size_units, TRUE, TRUE, 0);

	final_size_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(final_size_units), 1.0, 0.5);
	gtk_widget_show(final_size_units);
	gtk_box_pack_start(GTK_BOX(vbox), final_size_units, TRUE, TRUE, 0);

	// separator
	separator = gtk_hseparator_new();
	gtk_widget_show(separator);
	gtk_box_pack_start(GTK_BOX(frame_vbox), separator, TRUE, TRUE, 0);

	// fourth row

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(frame_vbox), hbox, FALSE, FALSE, 2);

	actual_fps_label = gtk_label_new(" Video Frame Rate:");
	gtk_misc_set_alignment(GTK_MISC(actual_fps_label), 0.0, 0.5);
	gtk_widget_show(actual_fps_label);
	gtk_box_pack_start(GTK_BOX(hbox), actual_fps_label, TRUE, TRUE, 5);

	actual_fps = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(actual_fps), 1.0, 0.5);
	gtk_widget_show(actual_fps);
	gtk_box_pack_start(GTK_BOX(hbox), actual_fps, FALSE, FALSE, 0);

	actual_fps_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(actual_fps_units), 1.0, 0.5);
	gtk_widget_show(actual_fps_units);
	gtk_box_pack_start(GTK_BOX(hbox), actual_fps_units, FALSE, FALSE, 5);

	gtk_widget_show(frame); // show control frame
}

/*
 * Main routine - set up window
 */
int gui_main(int argc, char **argv, CLiveConfig* pConfig)
{
	MyConfig = pConfig;

	dialog_mutex = SDL_CreateMutex();
	AVFlow = new CPreviewAVMediaFlow(pConfig);

#if 0
	argv = (char **)malloc(3 * sizeof(char *));
	argv[0] = "mp4live";
	argv[1] ="--sync";
	argv[2] = NULL;
	argc = 2;
#endif
	gtk_init(&argc, &argv);

	// Setup main window
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy(GTK_WINDOW(main_window), FALSE, FALSE, TRUE);

	char buffer[80];
	snprintf(buffer, sizeof(buffer), "cisco %s %s %s", argv[0], MPEG4IP_VERSION,
#ifdef HAVE_LINUX_VIDEODEV2_H
		 "V4L2"
#else
		 "V4L"
#endif
		 );
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

	main_vbox2 = gtk_vbox_new(FALSE, 1);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox2), 4);
	gtk_widget_show(main_vbox2);
	gtk_box_pack_start(GTK_BOX(main_hbox), main_vbox2, FALSE, FALSE, 5);

	// Video Preview
	NewVideoWindow();
	
	// Video Frame
	LayoutVideoFrame(main_vbox2);

	// Audio Frame
	LayoutAudioFrame(main_vbox2);

	// Recording Frame
	LayoutRecordingFrame(main_vbox2);

	// Transmission Frame
	LayoutTransmitFrame(main_vbox2);

	// Status frame
	LayoutStatusFrame(main_vbox1);

	// Control frame
	LayoutControlFrame(main_vbox1);

	DisplayAllSettings();

	gtk_widget_show(main_window);

	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		AVFlow->StartVideoPreview();
	}

	// "press" start button
	if (MyConfig->m_appAutomatic) {
		on_start_button(start_button, NULL);
	}

	gtk_main();
	return 0;
}

/* end gui_main.cpp */
