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
#include "video_source.h"
#include "audio_source.h"
#include "raw_recorder.h"
#include "mp4_recorder.h"
#include "rtp_transmitter.h"

CLiveConfig* MyConfig;

static bool Started = false;
static CVideoSource* VideoSource;
static CAudioSource* AudioSource;
static CRawRecorder* RawRecorder;
static CMp4Recorder* Mp4Recorder;
static CRtpTransmitter* RtpTransmitter;

/* Local variables */
static GtkWidget *main_window;
static GtkWidget *main_vbox;

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

static Timestamp StartTime;

/*
 * delete_event - called when window closed
 */
static void delete_event (GtkWidget *widget, gpointer *data)
{
  gtk_main_quit();
}

static void StartMedia()
{
	if (Started) {
		return;
	}

	if (MyConfig->m_audioEnable) {
		AudioSource = new CAudioSource();
		AudioSource->SetConfig(MyConfig);
		AudioSource->StartThread();
		AudioSource->StartCapture();
	}

	if (MyConfig->m_videoEnable && VideoSource == NULL) {
		VideoSource = new CVideoSource();
		VideoSource->SetConfig(MyConfig);
		VideoSource->StartThread();
		VideoSource->StartCapture();
	}

	if (MyConfig->m_recordEnable) {
		if (MyConfig->m_recordRaw) {
			RawRecorder = new CRawRecorder();
			RawRecorder->SetConfig(MyConfig);	
			RawRecorder->StartThread();
			if (AudioSource) {
				AudioSource->AddSink(RawRecorder);
			}
			if (VideoSource) {
				VideoSource->AddSink(RawRecorder);
			}
		}
		if (MyConfig->m_recordMp4) {
			Mp4Recorder = new CMp4Recorder();
			Mp4Recorder->SetConfig(MyConfig);	
			Mp4Recorder->StartThread();
			if (AudioSource) {
				AudioSource->AddSink(Mp4Recorder);
			}
			if (VideoSource) {
				VideoSource->AddSink(Mp4Recorder);
			}
		}
	}

	if (MyConfig->m_rtpEnable) {
		RtpTransmitter = new CRtpTransmitter();
		RtpTransmitter->SetConfig(MyConfig);	
		RtpTransmitter->StartThread();	
		if (AudioSource) {
			AudioSource->AddSink(RtpTransmitter);
		}
		if (VideoSource) {
			VideoSource->AddSink(RtpTransmitter);
		}
	}

	if (RawRecorder) {
		RawRecorder->StartRecord();
	}
	if (Mp4Recorder) {
		Mp4Recorder->StartRecord();
	}
	if (RtpTransmitter) {
		RtpTransmitter->StartTransmit();
	}
}

static void StopMedia()
{
	if (!Started) {
		return;
	}

	if (AudioSource) {
		AudioSource->StopThread();
		delete AudioSource;
		AudioSource = NULL;
	}
	if (VideoSource) {
		if (MyConfig->m_videoPreview) {
			VideoSource->RemoveAllSinks();
		} else {
			VideoSource->StopThread();
			delete VideoSource;
			VideoSource = NULL;
		}
	}
	if (RawRecorder) {
		RawRecorder->StopThread();
		delete RawRecorder;
		RawRecorder = NULL;
	}
	if (Mp4Recorder) {
		Mp4Recorder->StopThread();
		delete Mp4Recorder;
		Mp4Recorder = NULL;
	}
	if (RtpTransmitter) {
		RtpTransmitter->StopThread();
		delete RtpTransmitter;
		RtpTransmitter = NULL;
	}
}

static void StartVideoPreview()
{
	if (VideoSource == NULL) {
		VideoSource = new CVideoSource();
		VideoSource->SetConfig(MyConfig);
		VideoSource->StartThread();
	}
	VideoSource->StartPreview();
}

static void StopVideoPreview()
{
	if (VideoSource) {
		if (!Started) {
			VideoSource->StopThread();
			delete VideoSource;
			VideoSource = NULL;
		} else {
			VideoSource->StopPreview();
		}
	}
}

void SetAudio(bool mute)
{
	static char* mixerDeviceName = "/dev/mixer";
	static int muted = 0;
	static int lastVolume;

	int mixer = open(mixerDeviceName, O_RDONLY);

	if (mixer < 0) {
debug_message("Couldn't open mixer");
		return;
	}

	// TBD LINE vs VIDEO 
	if (mute) {
		ioctl(mixer, SOUND_MIXER_READ_LINE, &lastVolume);
		ioctl(mixer, SOUND_MIXER_WRITE_LINE, &muted);
debug_message("Muting, last volume %x", lastVolume);
	} else {
		int newVolume;
		ioctl(mixer, SOUND_MIXER_READ_LINE, &newVolume);
debug_message("UnMuting, new volume %x", newVolume);
		if (newVolume == 0) {
			ioctl(mixer, SOUND_MIXER_WRITE_LINE, &lastVolume);
		}
	}

	close(mixer);
}

void DisplayVideoSettings(void)
{
	char buffer[256];

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_enabled_button),
		MyConfig->m_videoEnable);
  
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(video_preview_button),
		MyConfig->m_videoPreview);

	snprintf(buffer, sizeof(buffer), " MPEG-4 at %u Kbps",
		MyConfig->m_videoTargetBitRate);
	gtk_label_set_text(GTK_LABEL(video_settings_label1), buffer);
	gtk_widget_show(video_settings_label1);

	snprintf(buffer, sizeof(buffer), " %ux%u @ %u fps", 
		MyConfig->m_videoWidth,
		MyConfig->m_videoHeight,
		MyConfig->m_videoTargetFrameRate);
	gtk_label_set_text(GTK_LABEL(video_settings_label2), buffer);
	gtk_widget_show(video_settings_label2);
}

void DisplayAudioSettings(void)
{
	char buffer[256];
 
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_enabled_button),
		MyConfig->m_audioEnable);
  
	snprintf(buffer, sizeof(buffer), " MP3 at %u Kbps",
		MyConfig->m_audioTargetBitRate);
	gtk_label_set_text(GTK_LABEL(audio_settings_label), buffer);
	gtk_widget_show(audio_settings_label);
}

void DisplayTransmitSettings (void)
{
	char buffer[256];
	const char *addr = MyConfig->m_rtpDestAddress;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(transmit_enabled_button),
		MyConfig->m_rtpEnable);
  
	if (addr == NULL) {
		gtk_label_set_text(GTK_LABEL(transmit_settings_label), 
			"RTP/UDP to <Automatic Multicast>");
	} else {
		snprintf(buffer, sizeof(buffer), " RTP/UDP to %s", addr); 
		gtk_label_set_text(GTK_LABEL(transmit_settings_label), buffer);
	}
	gtk_widget_show(transmit_settings_label);
}

void DisplayRecordingSettings (void)
{
  char *fileName;
  char buffer[256];

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(record_enabled_button),
	MyConfig->m_recordEnable);
  
  if (MyConfig->m_recordMp4) {
	fileName = strrchr(MyConfig->m_recordMp4FileName, '/');
	if (!fileName) {
		fileName = MyConfig->m_recordMp4FileName;
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
	MyConfig->m_videoEnable = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void on_video_preview_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->m_videoPreview = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	
	if (MyConfig->m_videoPreview) {
		StartVideoPreview();
	} else {
		StopVideoPreview();
	}
}

static void on_video_settings_button (GtkWidget *widget, gpointer *data)
{
  //CreateVideoDialog();
  //DisplayVideoSettings();
}

static void on_audio_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->m_audioEnable = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void on_audio_mute_button (GtkWidget *widget, gpointer *data)
{
	SetAudio(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void on_audio_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateAudioDialog();
}

static void on_record_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->m_recordEnable = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void on_record_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateRecordingDialog();
}

static void on_transmit_enabled_button (GtkWidget *widget, gpointer *data)
{
	MyConfig->m_rtpEnable = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

static void on_transmit_settings_button (GtkWidget *widget, gpointer *data)
{
	CreateTransmitDialog();
}

static guint status_timer_id;

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

	if (MyConfig->m_recordEnable) {
		struct stat stats;
		stat(MyConfig->m_recordMp4FileName, &stats);
		snprintf(buffer, sizeof(buffer), " %lu", stats.st_size / 1000000);
		gtk_label_set_text(GTK_LABEL(current_size), buffer);
		gtk_widget_show(current_size);

		gtk_label_set_text(GTK_LABEL(current_size_units), "MB");
		gtk_widget_show(current_size);
	}

	return (TRUE);  // keep timer going
}

static void on_start_button (GtkWidget *widget, gpointer *data)
{
	if (!Started) {
		if (!MyConfig->m_audioEnable && !MyConfig->m_videoEnable) {
			ShowMessage("Can't record", "Neither audio nor video are enabled");
			return;
		}

		// TBD lock out change to settings while media is running

		StartMedia();

		StartTime = GetTimestamp(); 

		gtk_label_set_text(GTK_LABEL(start_button_label), "  Stop  ");

		status_timer_id = gtk_timeout_add(1000, status_timer, main_window);

		Started = true;
	} else {
		StopMedia();

		// TBD unlock changes to settings

		gtk_label_set_text(GTK_LABEL(start_button_label), "  Start  ");

		gtk_timeout_remove(status_timer_id);

		Started = false;
	}
}

static gfloat frameLabelAlignment = 0.025;

static void LayoutVideoFrame()
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Video");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);

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
	gtk_widget_ref(video_settings_label1);
	gtk_box_pack_start(GTK_BOX(vbox2), video_settings_label1, TRUE, TRUE, 0);

	video_settings_label2 = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(video_settings_label2), 0.0, 0.5);
	gtk_widget_ref(video_settings_label2);
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
	gtk_widget_ref(video_settings_label3);
	gtk_box_pack_start(GTK_BOX(vbox3), video_settings_label3, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_widget_show(frame);
}

static void LayoutAudioFrame()
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Audio");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);

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
	gtk_widget_ref(audio_settings_label);
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

static void LayoutRecordingFrame()
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Recording");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);

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
	gtk_widget_ref(record_settings_label);
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

static void LayoutTransmitFrame()
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Transmission");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 5);

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
	gtk_widget_ref(transmit_settings_label);
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
void LayoutControlFrame()
{
	GtkWidget *frame;
	GtkWidget *vbox, *hbox;

	frame = gtk_frame_new("Status");
	gtk_frame_set_label_align(GTK_FRAME(frame), frameLabelAlignment, 0);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(main_vbox), frame, TRUE, TRUE, 0);

	// first row
	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(frame), hbox);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	start_button = gtk_button_new();
	start_button_label = gtk_label_new("  Start  ");
	gtk_misc_set_alignment(GTK_MISC(start_button_label), 0.5, 0.5);
	gtk_container_add(GTK_CONTAINER(start_button), start_button_label);
	gtk_widget_show(start_button_label);
	gtk_box_pack_start(GTK_BOX(vbox), start_button, TRUE, TRUE, 5);
	gtk_signal_connect(GTK_OBJECT(start_button), 
		"clicked",
		GTK_SIGNAL_FUNC(on_start_button),
		NULL);
	gtk_widget_show(start_button);

	// vertical separator
	GtkWidget* sep = gtk_vseparator_new();
	gtk_widget_show(sep);
	gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 0);

	// vbox for labels
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	start_time_label = gtk_label_new(" Start Time:");
	gtk_misc_set_alignment(GTK_MISC(start_time_label), 0.0, 0.5);
	gtk_widget_ref(start_time_label);
	gtk_widget_show(start_time_label);
	gtk_box_pack_start(GTK_BOX(vbox), start_time_label, TRUE, TRUE, 0);

	duration_label = gtk_label_new(" Duration:");
	gtk_misc_set_alignment(GTK_MISC(duration_label), 0.0, 0.5);
	gtk_widget_ref(duration_label);
	gtk_widget_show(duration_label);
	gtk_box_pack_start(GTK_BOX(vbox), duration_label, TRUE, TRUE, 0);

	current_time_label = gtk_label_new(" Current Time:");
	gtk_misc_set_alignment(GTK_MISC(current_time_label), 0.0, 0.5);
	gtk_widget_ref(current_time_label);
	gtk_widget_show(current_time_label);
	gtk_box_pack_start(GTK_BOX(vbox), current_time_label, TRUE, TRUE, 0);

	current_size_label = gtk_label_new(" Current Size:");
	gtk_misc_set_alignment(GTK_MISC(current_size_label), 0.0, 0.5);
	gtk_widget_ref(current_size_label);
	gtk_widget_show(current_size_label);
	gtk_box_pack_start(GTK_BOX(vbox), current_size_label, TRUE, TRUE, 0);

	// vbox for values
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

	start_time = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(start_time), 1.0, 0.5);
	gtk_widget_ref(start_time);
	gtk_widget_show(start_time);
	gtk_box_pack_start(GTK_BOX(vbox), start_time, TRUE, TRUE, 0);

	duration = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(duration), 1.0, 0.5);
	gtk_widget_ref(duration);
	gtk_widget_show(duration);
	gtk_box_pack_start(GTK_BOX(vbox), duration, TRUE, TRUE, 0);

	current_time = gtk_label_new("                 ");
	gtk_misc_set_alignment(GTK_MISC(current_time), 1.0, 0.5);
	gtk_widget_ref(current_time);
	gtk_widget_show(current_time);
	gtk_box_pack_start(GTK_BOX(vbox), current_time, TRUE, TRUE, 0);

	current_size = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_size), 1.0, 0.5);
	gtk_widget_ref(current_size);
	gtk_widget_show(current_size);
	gtk_box_pack_start(GTK_BOX(vbox), current_size, TRUE, TRUE, 0);

	// vbox for units
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	start_time_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(start_time_units), 1.0, 0.5);
	gtk_widget_ref(start_time_units);
	gtk_widget_show(start_time_units);
	gtk_box_pack_start(GTK_BOX(vbox), start_time_units, TRUE, TRUE, 0);

	duration_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(duration_units), 1.0, 0.5);
	gtk_widget_ref(duration_units);
	gtk_widget_show(duration_units);
	gtk_box_pack_start(GTK_BOX(vbox), duration_units, TRUE, TRUE, 0);

	current_time_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_time_units), 1.0, 0.5);
	gtk_widget_ref(current_time_units);
	gtk_widget_show(current_time_units);
	gtk_box_pack_start(GTK_BOX(vbox), current_time_units, TRUE, TRUE, 0);

	current_size_units = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(current_size_units), 1.0, 0.5);
	gtk_widget_ref(current_size_units);
	gtk_widget_show(current_size_units);
	gtk_box_pack_start(GTK_BOX(vbox), current_size_units, TRUE, TRUE, 0);

	gtk_widget_show(frame); // show control frame
	gtk_container_add(GTK_CONTAINER(main_window), main_vbox);
}

/*
 * Main routine - set up window
 */
int gui_main(int argc, char **argv, CLiveConfig* pConfig)
{
	MyConfig = pConfig;

	gtk_init(&argc, &argv);

	// Setup main window
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy(GTK_WINDOW(main_window), FALSE, TRUE, FALSE);
	gtk_window_set_title(GTK_WINDOW(main_window), "cisco mp4live");
	gtk_signal_connect(GTK_OBJECT(main_window),
		"delete_event",
		GTK_SIGNAL_FUNC(delete_event),
		NULL);

	// and the main vbox
	main_vbox = gtk_vbox_new(FALSE, 1);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 4);
	gtk_widget_show(main_vbox);

	// Video Frame
	LayoutVideoFrame();
	DisplayVideoSettings();

	// Audio Frame
	LayoutAudioFrame();
	DisplayAudioSettings();

	// Recording Frame
	LayoutRecordingFrame();
	DisplayRecordingSettings();

	// Transmission Frame
	LayoutTransmitFrame();
	DisplayTransmitSettings();

	// Control frame
	LayoutControlFrame();

	gtk_widget_show(main_window);

	gtk_main();

	return 0;
}

/* end gui_main.cpp */
