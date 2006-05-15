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
 * Copyright (C) Cisco Systems Inc. 2001-2005.  All Rights Reserved.
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
#include "support.h"
#include "profile_video.h"
#include "profile_audio.h"
#include "profile_text.h"
#include "mp4live_common.h"
#include "rtp_transmitter.h"
#include "text_source.h"
#include "video_v4l_source.h"

#define HAVE_TEXT 1
//#define HAVE_TEXT_ENTRY 1
CLiveConfig* MyConfig;
CPreviewAVMediaFlow* AVFlow = NULL;

/* Local variables */
static bool started = false;

GtkWidget *MainWindow;
static GtkTooltips *tooltips;
const char *SelectedStream;
static Timestamp StartTime;
static Timestamp StopTime;
static Duration FlowDuration;
static const u_int32_t durationUnitsValues[] = {
  1, 60, 3600, 3600*24, 3600*24*365
};
//static u_int64_t StartFileSize;
//static u_int64_t StopFileSize;

static void on_VideoPreview(GtkToggleButton *togglebutton, gpointer user_data);

CMediaStream *GetSelectedStream (void)
{
  return AVFlow->m_stream_list->FindStream(SelectedStream);
}


static void DisplayFinishTime (Timestamp t)
{
	time_t secs;
	struct tm local;
	char buffer[128];

	secs = (time_t)GetSecsFromTimestamp(t);
	localtime_r(&secs, &local);
	strftime(buffer, sizeof(buffer), "%l:%M:%S", &local);
	GtkWidget *temp;
	temp = lookup_widget(MainWindow, "EndTimeLabel");
	gtk_label_set_text(GTK_LABEL(temp), buffer);
	strftime(buffer, sizeof(buffer), "%p", &local);
	temp = lookup_widget(MainWindow, "EndTimeSuffix");
	gtk_label_set_text(GTK_LABEL(temp), buffer);
}

#if 0
static void on_audio_mute_button (GtkWidget *widget, gpointer *data)
{
	AVFlow->SetAudioOutput(
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

#endif

static const char *lockouts[] = {
  "new1", 
  "open1", 
  "save1",
  "generate_addresses",
  "generate_sdp",
  "preferences1",
  "AddStreamButton",
  "DeleteStreamButton",
  "StreamCaption",  
  "StreamDescription",
  "VideoEnabled",
  "AudioEnabled",
#ifdef HAVE_TEXT
  "TextEnabled",
  "TextSourceMenu",
#endif
  //"VideoSourceMenu", // not if we want to switch sources mid stream
  "AudioSourceMenu",
  "Duration", 
  "DurationType",
  NULL
};

static const char *lockins[] = {
  "restart_recording",
  NULL,
};

 static void LockoutChanges (bool lockout)
 {
   GtkWidget *temp;
   uint ix;
   for (ix = 0; lockouts[ix] != NULL; ix++) {
     temp = lookup_widget(MainWindow, lockouts[ix]);
     gtk_widget_set_sensitive(GTK_WIDGET(temp), !lockout);
   }
   for (ix = 0; lockins[ix] != NULL; ix++) {
     temp = lookup_widget(MainWindow, lockins[ix]);
     gtk_widget_set_sensitive(GTK_WIDGET(temp), lockout);
   }
}

static guint status_timer_id;

static void status_start()
{
	time_t secs;
	struct tm local;
	char buffer[128];
	GtkWidget *temp;

	// start time
	secs = (time_t)GetSecsFromTimestamp(StartTime);
	localtime_r(&secs, &local);
	strftime(buffer, sizeof(buffer), "%l:%M:%S", &local);
	temp = lookup_widget(MainWindow, "StartTimeLabel");
	gtk_label_set_text(GTK_LABEL(temp), buffer);
	strftime(buffer, sizeof(buffer), "%p", &local);
	temp = lookup_widget(MainWindow, "StartTimeSuffix");
	gtk_label_set_text(GTK_LABEL(temp), buffer);

	// finish time
	if (StopTime) {
		DisplayFinishTime(StopTime);
	}

#if 0
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
#endif
}

/*
 * Status timer routine
 */
static gint status_timer (gpointer raw)
{
	time_t secs;
	struct tm local;
	char buffer[80];
	GtkWidget *temp;

	if (AVFlow->ProcessSDLEvents(false)) {
	  on_VideoPreview(NULL, GINT_TO_POINTER(2));
	}

	if (started == false) {
	  return AVFlow->doingPreview(); 
	}

	Timestamp now = GetTimestamp();
	secs = (time_t)GetSecsFromTimestamp(now);
	localtime_r(&secs, &local);
	strftime(buffer, sizeof(buffer), "%l:%M:%S", &local);
	temp = lookup_widget(MainWindow, "CurrentTimeLabel");
	gtk_label_set_text(GTK_LABEL(temp), buffer);
	strftime(buffer, sizeof(buffer), "%p", &local);
	temp = lookup_widget(MainWindow, "CurrentTimeSuffix");
	gtk_label_set_text(GTK_LABEL(temp), buffer);

	time_t duration_secs = (time_t)GetSecsFromTimestamp(now - StartTime);

	snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", 
		duration_secs / 3600, (duration_secs % 3600) / 60, duration_secs % 60);
	temp = lookup_widget(MainWindow, "CurrentDurationLabel");
	gtk_label_set_text(GTK_LABEL(temp), buffer);
	
	CMediaStream *ms = GetSelectedStream();

	if (ms->GetBoolValue(STREAM_RECORD)) {
	  const char *fname;
	  ms->GetStreamStatus(FLOW_STATUS_FILENAME, &fname);
	  struct stat stats;
	  if (stat(fname, &stats) == 0) {
	    uint64_t size = stats.st_size;
	    size /= TO_U64(1000000);
	    snprintf(buffer, sizeof(buffer), " "U64"MB", size);
	  } else {
	    snprintf(buffer, sizeof(buffer), "BAD");
	  }
	  temp = lookup_widget(MainWindow, "StreamRecording");
	  gtk_label_set_text(GTK_LABEL(temp), buffer);
#if 0
	  if (MyConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS) ==
	      FILE_MP4_CREATE_NEW) {
	    DisplayRecordingSettings();
	  }
#endif
	}

	if (!StopTime) {
		float progress;
		AVFlow->GetStatus(FLOW_STATUS_PROGRESS, &progress);

		if (progress > 0.0) {
			u_int32_t estDuration = (u_int32_t)(duration_secs / progress);
			DisplayFinishTime(StartTime + (estDuration * TimestampTicks));
		}
	}

	if (ms->GetBoolValue(STREAM_VIDEO_ENABLED)) {
	  u_int32_t encodedFrames;
	  ms->GetStreamStatus(FLOW_STATUS_VIDEO_ENCODED_FRAMES, 
			      &encodedFrames);

	  snprintf(buffer, sizeof(buffer), " %.2f fps", 
		   ((float)encodedFrames / (float)(now - StartTime)) * TimestampTicks);
	  temp = lookup_widget(MainWindow, "StreamFps");
	  gtk_label_set_text(GTK_LABEL(temp), buffer);
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
			
			notice = "Completed";

			ShowMessage("Completed", notice, MainWindow);
		}

		return (FALSE);
	}

	return (TRUE);  // keep timer going
}

static void ReadConfigFromWindow (void)
{
  GtkWidget *temp;
  temp = lookup_widget(MainWindow, "Duration");

  MyConfig->SetIntegerValue(CONFIG_APP_DURATION,
			    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(temp)));
  // read info that might not be saved from config
  CMediaStream *ms = GetSelectedStream();
  if (ms != NULL) {
    temp = lookup_widget(MainWindow, "StreamRecordFileEntry");
    ms->SetStringValue(STREAM_RECORD_MP4_FILE_NAME,
		       gtk_entry_get_text(GTK_ENTRY(temp)));
    temp = lookup_widget(MainWindow, "StreamSdpFileEntry");
    ms->SetStringValue(STREAM_SDP_FILE_NAME,
		       gtk_entry_get_text(GTK_ENTRY(temp)));
    temp = lookup_widget(MainWindow, "StreamCaption");
    ms->SetStringValue(STREAM_CAPTION,
		       gtk_entry_get_text(GTK_ENTRY(temp)));
    temp = lookup_widget(MainWindow, "StreamDescription");
    ms->SetStringValue(STREAM_DESCRIPTION, 
		       gtk_entry_get_text(GTK_ENTRY(temp)));
  }
}

/*
 * MainWindowDisplaySources - update the sources labels
 */
void MainWindowDisplaySources (void) 
{
  GtkWidget *temp;
  char buffer[128];

  temp = lookup_widget(MainWindow, "VideoSourceLabel");
  
  if (MyConfig->m_videoCapabilities != NULL)
    MyConfig->m_videoCapabilities->Display(MyConfig, buffer, 128);
  else 
    snprintf(buffer, sizeof(buffer), "no video");

  gtk_label_set_text(GTK_LABEL(temp), buffer);

  temp = lookup_widget(MainWindow, "AudioSourceLabel");
  if (MyConfig->m_audioCapabilities != NULL) {
    MyConfig->m_audioCapabilities->Display(MyConfig, buffer, 128);
  } else {
    snprintf(buffer, sizeof(buffer), "no audio");
  }
  gtk_label_set_text(GTK_LABEL(temp), buffer);
  // need to do text
#ifdef HAVE_TEXT
  temp = lookup_widget(MainWindow, "TextSourceLabel");
  DisplayTextSource(MyConfig, buffer, sizeof(buffer));
  gtk_label_set_text(GTK_LABEL(temp), buffer);
#endif
}
static const char *add_profile_string = "Add Profile";
static const char *customize_profile_string = "Change Profile Settings";

// load_profiles will create a menu of the profiles - in alphabetical
// order.  Should only be called at start, and if a profile is added
static void DisplayProfiles (GtkWidget *option_menu, 
			   CConfigList *clist,
			   const char ***pOptions,
			   uint32_t *pCount)
{
  if (clist == NULL) return;

  uint32_t count = clist->GetCount();
  uint32_t count_on = 0;
  const char **options = (const char **)malloc(sizeof(char *) * (count + 4));
  CConfigEntry *ce = clist->GetHead();

  *pOptions = NULL; // so we don't activate when we create the menu

  while (ce != NULL) {
    const char *name = ce->GetName();
    options[count_on++] = strdup(name);
    ce = ce->GetNext();
  }
  // sort list in alphabetical order
  for (uint32_t ix = 0; ix < count_on - 1; ix++) {
    for (uint32_t jx = ix; jx < count_on; jx++) {
      if (strcmp(options[ix], options[jx]) > 0) {
	const char *temp = options[ix];
	options[ix] = options[jx];
	options[jx] = temp;
      }
    }
  }
  options[count_on++] = NULL; // add seperator
  options[count_on++] = strdup(add_profile_string);
  options[count_on++] = NULL;
  options[count_on++] = strdup(customize_profile_string);
  CreateOptionMenu(option_menu,
		   options, 
		   count_on,
		   0);
  *pOptions = options;
  *pCount = count_on;
}

static const char **audio_profile_names = NULL;
static uint32_t audio_profile_names_count = 0;
static const char **video_profile_names = NULL;
static uint32_t video_profile_names_count = 0;
#ifdef HAVE_TEXT
static const char **text_profile_names = NULL;
static uint32_t text_profile_names_count = 0;
#endif

static void free_profile_names (const char **arr, uint32_t count)
{
  for (uint32_t ix = 0; ix < count; ix++) {
    CHECK_AND_FREE(arr[ix]);
  }
  CHECK_AND_FREE(arr);
}

static uint32_t get_index_from_profile_list (const char **list,
					     uint32_t count, 
					     const char *name)
{
  for (uint32_t ix = 0; ix < count; ix++) {
    if (strcmp(list[ix], name) == 0) {
      return ix;
    }
  }
  return 0;
}

static void DisplayAudioProfiles (void)
{
  free_profile_names(audio_profile_names, audio_profile_names_count);
  DisplayProfiles(lookup_widget(MainWindow, "AudioProfile"),
		(CConfigList *)AVFlow->m_audio_profile_list, 
		&audio_profile_names,
		&audio_profile_names_count);
}

static void DisplayVideoProfiles (void) 
{
  free_profile_names(video_profile_names, video_profile_names_count);
  DisplayProfiles(lookup_widget(MainWindow, "VideoProfile"),
		  (CConfigList *)AVFlow->m_video_profile_list, 
		  &video_profile_names,
		  &video_profile_names_count);
}

#ifdef HAVE_TEXT
static void DisplayTextProfiles (void)
{
  free_profile_names(text_profile_names, text_profile_names_count);
  DisplayProfiles(lookup_widget(MainWindow, "TextProfile"),
		(CConfigList *)AVFlow->m_text_profile_list,
		&text_profile_names,
		&text_profile_names_count);
}
#endif

/*
 * SetStreamTransmit - sets up information about the stream sdp
 */
static void DisplayStreamTransmit (CMediaStream *ms) 
{
  GtkWidget *temp;
  bool is_set;

  temp = lookup_widget(MainWindow, "StreamTransmit");
  is_set = ms->GetBoolValue(STREAM_TRANSMIT);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), is_set);
  is_set = is_set && !started;
  gtk_widget_set_sensitive(temp, !started);

  temp = lookup_widget(MainWindow, "StreamSdpFileEntry");
  gtk_entry_set_text(GTK_ENTRY(temp), ms->GetStringValue(STREAM_SDP_FILE_NAME));

  gtk_widget_set_sensitive(temp, is_set);
  temp = lookup_widget(MainWindow, "SDPFileOpenButton");
  gtk_widget_set_sensitive(temp, is_set);
}

/*
 * SetStreamRecord - set the stream's record settings
 */
static void DisplayStreamRecord (CMediaStream *ms) 
{
  GtkWidget *temp;
  bool is_set;

  temp = lookup_widget(MainWindow, "StreamRecord");
  is_set = ms->GetBoolValue(STREAM_RECORD);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), is_set);
  is_set = is_set && !started;
  gtk_widget_set_sensitive(temp, !started);

  temp = lookup_widget(MainWindow, "StreamRecordFileEntry");
  gtk_entry_set_text(GTK_ENTRY(temp), 
		     ms->GetStringValue(STREAM_RECORD_MP4_FILE_NAME));

  gtk_widget_set_sensitive(temp, is_set);
  temp = lookup_widget(MainWindow, "RecordFileOpenButton");
  gtk_widget_set_sensitive(temp, is_set);
}

/*
 * DisplayStreamsInView - load the streams into the stream view box
 * only occurs at startup, or if a stream is added/deleted
 */
static void DisplayStreamsInView (GtkTreeView *StreamTreeView = NULL,
				  const char *selected_stream = NULL) 
{
  if (StreamTreeView == NULL) {
    StreamTreeView = GTK_TREE_VIEW(lookup_widget(MainWindow, "StreamTreeView"));
  }
  GtkTreeSelection *select = gtk_tree_view_get_selection(StreamTreeView);

  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
  GtkTreeIter iter;
  GtkTreeIter selected_iter;
  bool selected = false;

  CMediaStream *ms = AVFlow->m_stream_list->GetHead();
  while (ms != NULL) {
    // Add the stream name to the list.
    gtk_list_store_append(store, &iter);
    // check if it should be highlighted
    if (selected_stream == NULL) {
      if (selected == false) {
	selected_iter = iter;
	selected = true;
      }
    } else {
      if (strcmp(selected_stream, ms->GetName()) == 0) {
	selected_iter = iter;
	selected = true;
      }
    }
    gtk_list_store_set(store, &iter, 
		       0, ms->GetName(),
		       -1);
    ms = ms->GetNext();
  }
  gtk_tree_view_set_model(StreamTreeView, GTK_TREE_MODEL(store));
  if (selected)
    gtk_tree_selection_select_iter(select, &selected_iter);
}

/*
 * DisplayStreamData - when a stream is selected, we need to change a lot
 * of things
 */
static void DisplayStreamData (const char *stream)
{
  CMediaStream *ms;

  ms = AVFlow->m_stream_list->FindStream(stream);
  if (ms == NULL) {
    ms = AVFlow->m_stream_list->GetHead();
  }

  CHECK_AND_FREE(SelectedStream);
  SelectedStream = strdup(ms->GetName());

  GtkWidget *temp, *temp2;
  const char *ptr;
  char buffer[1024];

  // Name, description
  temp = lookup_widget(MainWindow, "StreamNameLabel");
  gtk_label_set_text(GTK_LABEL(temp), SelectedStream);

  temp = lookup_widget(MainWindow, "StreamCaption");
  ptr = ms->GetStringValue(STREAM_CAPTION);
  gtk_entry_set_text(GTK_ENTRY(temp), ptr == NULL ? "" : ptr);

  temp = lookup_widget(MainWindow, "StreamDescription");
  ptr = ms->GetStringValue(STREAM_DESCRIPTION);
  gtk_entry_set_text(GTK_ENTRY(temp), ptr == NULL ? "" : ptr);

  // transmit and mp4 file
  DisplayStreamTransmit(ms);
  DisplayStreamRecord(ms);

  uint32_t index;

  // Audio information - enabled, profile, change tooltip to reflect settings
  temp = lookup_widget(MainWindow, "AudioEnabled");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), 
			       ms->GetBoolValue(STREAM_AUDIO_ENABLED));
  index = get_index_from_profile_list(audio_profile_names, 
				      audio_profile_names_count,
				      ms->GetStringValue(STREAM_AUDIO_PROFILE));
  temp = lookup_widget(MainWindow, "AudioProfile");
  gtk_option_menu_set_history(GTK_OPTION_MENU(temp), index);
  bool enabled = ms->GetBoolValue(STREAM_AUDIO_ENABLED);
  temp2 = lookup_widget(MainWindow, "StreamAudioInfo");
  if (enabled == false) {
    gtk_label_set_text(GTK_LABEL(temp2), "audio disabled");
    gtk_tooltips_set_tip(tooltips, temp, "Select/Add/Customize Audio Profile", NULL);
  } else {
    CAudioProfile *ap = ms->GetAudioProfile();
    float srate = ((float)ap->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE)) / 1000.0;
    float sbit = ((float)ap->GetIntegerValue(CFG_AUDIO_BIT_RATE)) / 1000.0;
    snprintf(buffer, sizeof(buffer), "%s %gKbps",
	     ap->GetStringValue(CFG_AUDIO_ENCODING),
				sbit);
    gtk_label_set_text(GTK_LABEL(temp2), buffer);
    snprintf(buffer, sizeof(buffer), 
	     "Select/Add/Customize Audio Profile\n"
	     "%s(%s), %s, %gKHz %gKbps", 
	     ap->GetStringValue(CFG_AUDIO_ENCODING),
	     ap->GetStringValue(CFG_AUDIO_ENCODER),
	     ap->GetIntegerValue(CFG_AUDIO_CHANNELS) == 1 ? "mono" : "stereo",
	     srate,
	     sbit);
    gtk_tooltips_set_tip(tooltips, temp, buffer, NULL);

  }
  temp2 = lookup_widget(MainWindow, "AudioTxAddrLabel");
  if (enabled == false || ms->GetBoolValue(STREAM_TRANSMIT) == false) {
    gtk_label_set_text(GTK_LABEL(temp2), "disabled");
  } else {
    snprintf(buffer, sizeof(buffer), "%s:%u", 
	     ms->GetStringValue(STREAM_AUDIO_DEST_ADDR), 
	     ms->GetIntegerValue(STREAM_AUDIO_DEST_PORT));
    gtk_label_set_text(GTK_LABEL(temp2), buffer);
  }
  enabled = enabled && !started;
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(MainWindow, "AudioTxAddrButton");
  gtk_widget_set_sensitive(temp, enabled);

  // Video Information
  temp = lookup_widget(MainWindow, "VideoEnabled");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), 
			       ms->GetBoolValue(STREAM_VIDEO_ENABLED));
  index = get_index_from_profile_list(video_profile_names, 
				      video_profile_names_count,
				      ms->GetStringValue(STREAM_VIDEO_PROFILE));
  temp = lookup_widget(MainWindow, "VideoProfile");
  gtk_option_menu_set_history(GTK_OPTION_MENU(temp), index);
  enabled = ms->GetBoolValue(STREAM_VIDEO_ENABLED);

  temp2 = lookup_widget(MainWindow, "StreamVideoInfo");
  if (enabled == false) {
    gtk_label_set_text(GTK_LABEL(temp2), "video disabled");
    gtk_tooltips_set_tip(tooltips, temp, "Select/Add/Customize Video Profile", NULL);
  } else {
    CVideoProfile *vp = ms->GetVideoProfile();
    snprintf(buffer, sizeof(buffer), "%s %ux%u %uKbps",
	     vp->GetStringValue(CFG_VIDEO_ENCODING),
	     vp->m_videoWidth,
	     vp->m_videoHeight,
	     vp->GetIntegerValue(CFG_VIDEO_BIT_RATE));
    gtk_label_set_text(GTK_LABEL(temp2), buffer);
    snprintf(buffer, sizeof(buffer), "Select/Add/Customize Video Profile\n%s(%s) %uX%u %uKbps@%gfps",
	     vp->GetStringValue(CFG_VIDEO_ENCODING),
	     vp->GetStringValue(CFG_VIDEO_ENCODER),
	     vp->m_videoWidth,
	     vp->m_videoHeight,
	     vp->GetIntegerValue(CFG_VIDEO_BIT_RATE),
	     vp->GetFloatValue(CFG_VIDEO_FRAME_RATE));
    gtk_tooltips_set_tip(tooltips, temp, buffer, NULL);
	     
  }

  temp2 = lookup_widget(MainWindow, "StreamVideoPreview");
  gtk_widget_set_sensitive(temp2, enabled);
  if (MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
    const char *preview = 
      MyConfig->GetStringValue(CONFIG_VIDEO_PREVIEW_STREAM);
    if (preview != NULL) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp2),
				   strcmp(preview, SelectedStream) == 0);
    } else {
      temp2 = lookup_widget(MainWindow, "VideoSourcePreview");
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp2), 
				   true);
    }
  }
  gtk_widget_set_sensitive(temp2, enabled);
  temp2 = lookup_widget(MainWindow, "VideoTxAddrLabel");
  if (enabled == false || ms->GetBoolValue(STREAM_TRANSMIT) == false) {
    gtk_label_set_text(GTK_LABEL(temp2), "disabled");
  } else {
    snprintf(buffer, sizeof(buffer), "%s:%u", 
	     ms->GetStringValue(STREAM_VIDEO_DEST_ADDR), 
	     ms->GetIntegerValue(STREAM_VIDEO_DEST_PORT));
    gtk_label_set_text(GTK_LABEL(temp2), buffer);
  }

  enabled = enabled && !started;
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(MainWindow, "VideoTxAddrButton");
  gtk_widget_set_sensitive(temp, enabled);

#ifdef HAVE_TEXT
  // text information
  temp = lookup_widget(MainWindow, "TextEnabled");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(temp), 
			       ms->GetBoolValue(STREAM_TEXT_ENABLED));
  index = get_index_from_profile_list(text_profile_names, 
				      text_profile_names_count,
				      ms->GetStringValue(STREAM_TEXT_PROFILE));
  temp = lookup_widget(MainWindow, "TextProfile");
  gtk_option_menu_set_history(GTK_OPTION_MENU(temp), index);

  enabled = ms->GetBoolValue(STREAM_TEXT_ENABLED);
  temp2 = lookup_widget(MainWindow, "StreamTextInfo");
  if (enabled == false) {
    gtk_label_set_text(GTK_LABEL(temp2), "text disabled");
    gtk_tooltips_set_tip(tooltips, temp, "Select/Add/Customize Audio Profile", NULL);
  } else {
    CTextProfile *tp = ms->GetTextProfile();
    snprintf(buffer, sizeof(buffer), "%s", tp->GetStringValue(CFG_TEXT_ENCODING));
    gtk_label_set_text(GTK_LABEL(temp2), buffer);
    snprintf(buffer, sizeof(buffer), 
	     "Select/Add/Customize Text Profile\n"
	     "%s", 
	     tp->GetStringValue(CFG_TEXT_ENCODING));
    gtk_tooltips_set_tip(tooltips, temp, buffer, NULL);
  }
  temp2 = lookup_widget(MainWindow, "TextTxAddrLabel");
  if (enabled == false || ms->GetBoolValue(STREAM_TRANSMIT) == false) {
    gtk_label_set_text(GTK_LABEL(temp2), "disabled");
  } else {
    snprintf(buffer, sizeof(buffer), "%s:%u", 
	     ms->GetStringValue(STREAM_TEXT_DEST_ADDR), 
	     ms->GetIntegerValue(STREAM_TEXT_DEST_PORT));
    gtk_label_set_text(GTK_LABEL(temp2), buffer);
  }
  enabled = enabled && !started;
    gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(MainWindow, "TextTxAddrButton");
  gtk_widget_set_sensitive(temp, enabled);
#endif

}

/****************************************************************************
 * Main window routines called from other dialogs
 ****************************************************************************/
/*
 * OnAudioProfileFinished and OnVideoProfileFinished get called
 * when an audio or a video profile are updated.  This will change
 * settings, as well as validating the information for the sources
 */
void OnAudioProfileFinished (CAudioProfile *p)
{
  if (p != NULL) {
    CMediaStream *ms = GetSelectedStream();
    ms->SetAudioProfile(p->GetName());
    DisplayAudioProfiles();
  }
  AVFlow->ValidateAndUpdateStreams();
  MainWindowDisplaySources();
  DisplayStreamData(SelectedStream);

}

void OnVideoProfileFinished (CVideoProfile *p)
{
  if (p != NULL) {
    // new video profile created - set the stream to it
    CMediaStream *ms = GetSelectedStream();
    ms->SetVideoProfile(p->GetName());
    DisplayVideoProfiles(); // have it appear in the list
  }
  AVFlow->ValidateAndUpdateStreams();
  MainWindowDisplaySources();
  DisplayStreamData(SelectedStream);
  if (MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
    AVFlow->StartVideoPreview();
  }}

void OnTextProfileFinished (CTextProfile *p)
{
  if (p != NULL) {
    CMediaStream *ms = GetSelectedStream();
    ms->SetTextProfile(p->GetName());
    DisplayTextProfiles();
  }
  AVFlow->ValidateAndUpdateStreams();
  MainWindowDisplaySources();
  DisplayStreamData(SelectedStream);
}

void RefreshCurrentStream (void)
{
  DisplayStreamData(SelectedStream);
}

// Top level menu handlers

static void onConfigFileOpenResponse (GtkDialog *sel,
				      gint response_id,
				      gpointer user_data)
{
  if (GTK_RESPONSE_OK == response_id) {
    const char *name = 
      gtk_file_selection_get_filename(GTK_FILE_SELECTION(sel));
    
    struct stat statbuf;
    if (GPOINTER_TO_INT(user_data) == false) {
      if (stat(name, &statbuf) != 0 ||
	  !S_ISREG(statbuf.st_mode)) {
	ShowMessage("Open File Error", "File does not exist", MainWindow);
	return;
      }
    } else {
      if (stat(name, &statbuf) == 0) {
	ShowMessage("New File Error", "File exists", MainWindow);
	return;
      }
    }
    MyConfig->SetDebug(true);
    MyConfig->SetToDefaults();
    MyConfig->ReadFile(name);
    MyConfig->Update();
    StartFlowLoadWindow();
      
  }
  gtk_widget_destroy(GTK_WIDGET(sel));
}

static void start_config_file_selection (bool new_one)
{
  GtkWidget *fs, *FileOkayButton, *cancel_button1;
  fs = gtk_file_selection_new(new_one ? 
			      "New global configuration file" :
			      "Open global configuration file");
    gtk_container_set_border_width(GTK_CONTAINER(fs), 10);
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fs));

  if (new_one == false) {
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs),
				    MyConfig->GetFileName());
  }

  FileOkayButton = GTK_FILE_SELECTION(fs)->ok_button;
  gtk_widget_show(FileOkayButton);
  GTK_WIDGET_SET_FLAGS(FileOkayButton, GTK_CAN_DEFAULT);

  cancel_button1 = GTK_FILE_SELECTION(fs)->cancel_button;
  gtk_widget_show(cancel_button1);
  GTK_WIDGET_SET_FLAGS(cancel_button1, GTK_CAN_DEFAULT);
 
  gtk_window_set_modal(GTK_WINDOW(fs), true);
  gtk_window_set_transient_for(GTK_WINDOW(fs), 
			       GTK_WINDOW(MainWindow));
  g_signal_connect((gpointer) fs, "response",
		   G_CALLBACK (onConfigFileOpenResponse),
		   GINT_TO_POINTER(new_one));
  gtk_widget_show(fs);

}

static void
on_new1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  start_config_file_selection(true);
}

static void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  start_config_file_selection(false);
}


static void
on_save1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  if (started == false) {
    // read that info
    ReadConfigFromWindow();
  }

  // need to read the information
  GtkWidget *statusbar = lookup_widget(MainWindow, "statusbar1");
  gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 
		     0,
		     "Writing Configuration and Stream Files");
  MyConfig->WriteToFile();
  CMediaStream *ms = AVFlow->m_stream_list->GetHead();
  while (ms != NULL) {
    ms->WriteToFile();
    ms = ms->GetNext();
  }
    
  gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 
		     0,
		     "Configuration and stream files written");
}


static void on_generate_addresses_yes (void)
{
  CMediaStream *ms = AVFlow->m_stream_list->GetHead();

  while (ms != NULL) {
    struct in_addr in;
    if (ms->GetBoolValue(STREAM_TRANSMIT)) {
      if (ms->GetBoolValue(STREAM_VIDEO_ENABLED) &&
	  ms->GetBoolValue(STREAM_VIDEO_ADDR_FIXED) == false) {
	in.s_addr = CRtpTransmitter::GetRandomMcastAddress();
	ms->SetStringValue(STREAM_VIDEO_DEST_ADDR, inet_ntoa(in));
	ms->SetIntegerValue(STREAM_VIDEO_DEST_PORT,
			    CRtpTransmitter::GetRandomPortBlock());
      }
      if (ms->GetBoolValue(STREAM_AUDIO_ENABLED) &&
	  ms->GetBoolValue(STREAM_AUDIO_ADDR_FIXED) == false) {
	in.s_addr = CRtpTransmitter::GetRandomMcastAddress();
	ms->SetStringValue(STREAM_AUDIO_DEST_ADDR, inet_ntoa(in));
	ms->SetIntegerValue(STREAM_AUDIO_DEST_PORT,
			    CRtpTransmitter::GetRandomPortBlock());
      }
      if (ms->GetBoolValue(STREAM_TEXT_ENABLED) &&
	  ms->GetBoolValue(STREAM_TEXT_ADDR_FIXED) == false) {
	in.s_addr = CRtpTransmitter::GetRandomMcastAddress();
	ms->SetStringValue(STREAM_TEXT_DEST_ADDR, inet_ntoa(in));
	ms->SetIntegerValue(STREAM_TEXT_DEST_PORT,
			    CRtpTransmitter::GetRandomPortBlock());
      }
    }
    ms = ms->GetNext();
  }
  AVFlow->ValidateAndUpdateStreams();
  DisplayStreamData(SelectedStream);
}

static void
on_generate_addresses_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  printf("generate_addresses_activate\n");
  YesOrNo("Generate Random Multicast Addresses",
	  "This will generate new addresses and ports\n"
	  "for all Streams\n"
	  "\nDo you want to continue ?",
	  true,
	  GTK_SIGNAL_FUNC(on_generate_addresses_yes),
	  NULL);
}

static void on_restart_record_yes (void)
{
  AVFlow->RestartFileRecording();
}
static const char *restart1 = 
"This will restart recording for all streams\n"
"\nDo you want to continue ?";
static const char *restart2 = 
"This will restart recording for all streams\n"
"\nIt is configured to wipe out the existing file\n"
"\nDo you want to continue ?";

static void 
on_restart_recording_activate           (GtkMenuItem  *menuitem,
					 gpointer      user_data)
{
  YesOrNo("Restart Recording",
	  MyConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS)
	  == FILE_MP4_CREATE_NEW ? restart1 : restart2,
	  false,
	  GTK_SIGNAL_FUNC(on_restart_record_yes),
	  NULL);
}

static void
on_generate_sdp_activate         (GtkMenuItem     *menuitem,
				  gpointer         user_data)
{
  AVFlow->ValidateAndUpdateStreams();
  GtkWidget *statusbar = lookup_widget(MainWindow, "statusbar1");
  gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 
		     0,
		     "SDP files have been generated");
}
static void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_PreferencesDialog();
}


static void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), 
	   "mp4live %s %s\n"
	   "\nAn open source live broadcaster\n"
	   "\nDeveloped by Cisco Systems\n"
	   "\nUsing open source packages:\n"
	   "lame encoder\n"
	   "faac encoder\n"
#if defined(HAVE_XVID10) || defined(HAVE_XVID_H)
	   "Xvid mpeg-4 encoder\n"
#endif
#ifdef HAVE_FFMPEG
	   "Ffmpeg encoder\n"
#endif
#ifdef HAVE_X264
	   "H.264 encoder from x264\n"
#endif
	   "H.261 encoder from UC\n"
	   "Glade interface designer\n"
	   "\n2000 to present\n"
	   ,
	   get_linux_video_type(),
	   MPEG4IP_VERSION);
  ShowMessage("About", buffer);
}

// Source Menu handlers
static void RestoreSourceMenu (void)
{
  GtkWidget *wid = lookup_widget(MainWindow, "SourceOptionMenu");
  gtk_option_menu_set_history(GTK_OPTION_MENU(wid), 0);
}

static void
on_VideoSourceMenu_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  RestoreSourceMenu();
  create_VideoSourceDialog();
}


static void
on_AudioSourceMenu_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  RestoreSourceMenu();
  create_AudioSourceDialog();
}
static void
on_picture_settings_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  RestoreSourceMenu();
  CreatePictureDialog();
}

#ifdef HAVE_TEXT
static void
on_TextSourceMenu_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  RestoreSourceMenu();
  create_TextSourceDialog();
}
#endif

// Add Stream Dialog
static void
on_AddStreamDialog_response            (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  if (GTK_RESPONSE_OK == response_id) {
    GtkWidget *temp = lookup_widget(GTK_WIDGET(dialog), "AddStreamText");
    
    const char *stream_name = gtk_entry_get_text(GTK_ENTRY(temp));
    if ((stream_name == NULL) || (*stream_name == '\0')) {
      ShowMessage("Add Stream Error", "Must enter stream name", GTK_WIDGET(dialog));
      return;
    }

    if (AVFlow->AddStream(stream_name) == false) {
      ShowMessage("Not Added", "An Error as occurred\nStream has not been added");
    } else {
      DisplayStreamsInView(NULL, stream_name);
      DisplayStreamData(stream_name);
    }
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

// This is if they hit return from the AddStream text window
static void on_AddStreamText_activate (GtkEntry *ent,
				       gpointer user_data)
{
  on_AddStreamDialog_response(GTK_DIALOG(user_data),
			      GTK_RESPONSE_OK,
			      NULL);
}


// create the add stream dialog
static void
on_AddStreamButton_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *AddStreamDialog;
  GtkWidget *dialog_vbox1;
  GtkWidget *vbox20;
  GtkWidget *AddStreamText;
  GtkWidget *dialog_action_area1;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton1;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new();

  AddStreamDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(AddStreamDialog), _("Add Stream"));
  gtk_window_set_modal(GTK_WINDOW(AddStreamDialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(AddStreamDialog), 
			       GTK_WINDOW(MainWindow));
  gtk_window_set_resizable(GTK_WINDOW(AddStreamDialog), FALSE);
  gtk_dialog_set_has_separator(GTK_DIALOG(AddStreamDialog), FALSE);

  dialog_vbox1 = GTK_DIALOG(AddStreamDialog)->vbox;
  gtk_widget_show(dialog_vbox1);

  vbox20 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox20);
  gtk_box_pack_start(GTK_BOX(dialog_vbox1), vbox20, TRUE, TRUE, 0);

  AddStreamText = gtk_entry_new();
  gtk_widget_show(AddStreamText);
  gtk_box_pack_start(GTK_BOX(vbox20), AddStreamText, FALSE, FALSE, 16);
  gtk_tooltips_set_tip(tooltips, AddStreamText, _("Enter Stream Name"), NULL);
  gtk_entry_set_width_chars(GTK_ENTRY(AddStreamText), 40);

  dialog_action_area1 = GTK_DIALOG(AddStreamDialog)->action_area;
  gtk_widget_show(dialog_action_area1);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area1), GTK_BUTTONBOX_END);

  cancelbutton1 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton1);
  gtk_dialog_add_action_widget(GTK_DIALOG(AddStreamDialog), cancelbutton1, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton1, GTK_CAN_DEFAULT);

  okbutton1 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton1);
  gtk_dialog_add_action_widget(GTK_DIALOG(AddStreamDialog), okbutton1, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton1, GTK_CAN_DEFAULT);
  
  g_signal_connect((gpointer) AddStreamText, "activate",
		   G_CALLBACK(on_AddStreamText_activate),
		   AddStreamDialog);
  g_signal_connect((gpointer) AddStreamDialog, "response",
                    G_CALLBACK(on_AddStreamDialog_response),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(AddStreamDialog, AddStreamDialog, "AddStreamDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(AddStreamDialog, dialog_vbox1, "dialog_vbox1");
  GLADE_HOOKUP_OBJECT(AddStreamDialog, vbox20, "vbox20");
  GLADE_HOOKUP_OBJECT(AddStreamDialog, AddStreamText, "AddStreamText");
  GLADE_HOOKUP_OBJECT_NO_REF(AddStreamDialog, dialog_action_area1, "dialog_action_area1");
  GLADE_HOOKUP_OBJECT(AddStreamDialog, cancelbutton1, "cancelbutton1");
  GLADE_HOOKUP_OBJECT(AddStreamDialog, okbutton1, "okbutton1");
  GLADE_HOOKUP_OBJECT_NO_REF(AddStreamDialog, tooltips, "tooltips");

  gtk_widget_grab_focus(AddStreamText);
  gtk_widget_show(AddStreamDialog);

}

/*
 * Delete Stream handlers and dialog
 */
static void
on_DeleteStreamDialog_response         (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  if (GTK_RESPONSE_YES == response_id) {
    if (MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
      const char *stream = 
	MyConfig->GetStringValue(CONFIG_VIDEO_PREVIEW_STREAM);
      if (strcmp(stream, SelectedStream) == 0) {
	AVFlow->StopVideoPreview();
	MyConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW, false);
	MyConfig->SetStringValue(CONFIG_VIDEO_PREVIEW_STREAM, NULL);
      }
    }
    if (AVFlow->DeleteStream(SelectedStream) == false) {
      ShowMessage("Not Deleted", "An Error as occurred\nStream not deleted");
    } else {
      DisplayStreamsInView();
      DisplayStreamData(NULL);
    }
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
on_DeleteStreamButton_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *DeleteStreamDialog;
  GtkWidget *dialog_vbox2;
  GtkWidget *vbox21;
  GtkWidget *label72;
  GtkWidget *DeleteStreamLabel;
  GtkWidget *label74;
  GtkWidget *dialog_action_area2;
  GtkWidget *cancelbutton2;
  GtkWidget *DeleteStreamButton;

  if (started) return;

  if (AVFlow->m_stream_list->GetCount() == 1) {
    ShowMessage("Delete Error", "Can not delete last stream", MainWindow);
    return;
  }

  DeleteStreamDialog = gtk_dialog_new();
  gtk_widget_set_size_request(DeleteStreamDialog, 236, -1);
  gtk_window_set_title(GTK_WINDOW(DeleteStreamDialog), _("Delete Stream ?"));
  gtk_window_set_modal(GTK_WINDOW(DeleteStreamDialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(DeleteStreamDialog), 
			       GTK_WINDOW(MainWindow));
  gtk_window_set_resizable(GTK_WINDOW(DeleteStreamDialog), FALSE);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(DeleteStreamDialog), TRUE);
  gtk_dialog_set_has_separator(GTK_DIALOG(DeleteStreamDialog), FALSE);

  dialog_vbox2 = GTK_DIALOG(DeleteStreamDialog)->vbox;
  gtk_widget_show(dialog_vbox2);

  vbox21 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox21);
  gtk_box_pack_start(GTK_BOX(dialog_vbox2), vbox21, TRUE, TRUE, 0);

  label72 = gtk_label_new(_("You are about to delete stream:"));
  gtk_widget_show(label72);
  gtk_box_pack_start(GTK_BOX(vbox21), label72, FALSE, FALSE, 23);

  DeleteStreamLabel = gtk_label_new(SelectedStream);
  gtk_widget_show(DeleteStreamLabel);
  gtk_box_pack_start(GTK_BOX(vbox21), DeleteStreamLabel, FALSE, FALSE, 1);

  label74 = gtk_label_new(_("This operation cannot be\nreversed."));
  gtk_widget_show(label74);
  gtk_box_pack_start(GTK_BOX(vbox21), label74, FALSE, FALSE, 24);
  gtk_label_set_justify(GTK_LABEL(label74), GTK_JUSTIFY_CENTER);

  dialog_action_area2 = GTK_DIALOG(DeleteStreamDialog)->action_area;
  gtk_widget_show(dialog_action_area2);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area2), GTK_BUTTONBOX_END);

  cancelbutton2 = gtk_button_new_from_stock("gtk-no");
  gtk_widget_show(cancelbutton2);
  gtk_dialog_add_action_widget(GTK_DIALOG(DeleteStreamDialog), cancelbutton2, GTK_RESPONSE_NO);
  GTK_WIDGET_SET_FLAGS(cancelbutton2, GTK_CAN_DEFAULT);

  DeleteStreamButton = gtk_button_new_from_stock("gtk-yes");
  gtk_widget_show(DeleteStreamButton);
  gtk_dialog_add_action_widget(GTK_DIALOG(DeleteStreamDialog), DeleteStreamButton, GTK_RESPONSE_YES);
  GTK_WIDGET_SET_FLAGS(DeleteStreamButton, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) DeleteStreamDialog, "response",
                    G_CALLBACK(on_DeleteStreamDialog_response),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(DeleteStreamDialog, DeleteStreamDialog, "DeleteStreamDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(DeleteStreamDialog, dialog_vbox2, "dialog_vbox2");
  GLADE_HOOKUP_OBJECT(DeleteStreamDialog, vbox21, "vbox21");
  GLADE_HOOKUP_OBJECT(DeleteStreamDialog, label72, "label72");
  GLADE_HOOKUP_OBJECT(DeleteStreamDialog, DeleteStreamLabel, "DeleteStreamLabel");
  GLADE_HOOKUP_OBJECT(DeleteStreamDialog, label74, "label74");
  GLADE_HOOKUP_OBJECT_NO_REF(DeleteStreamDialog, dialog_action_area2, "dialog_action_area2");
  GLADE_HOOKUP_OBJECT(DeleteStreamDialog, cancelbutton2, "cancelbutton2");
  GLADE_HOOKUP_OBJECT(DeleteStreamDialog, DeleteStreamButton, "DeleteStreamButton");
  gtk_widget_grab_focus(cancelbutton2);
  gtk_widget_show(DeleteStreamDialog);
}

// When the transmit enabled gets clicked
static void
on_StreamTransmit_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (started) return;
  bool set = gtk_toggle_button_get_active(togglebutton);
  CMediaStream *ms = GetSelectedStream();
  if (ms == NULL) return;
  ms->SetBoolValue(STREAM_TRANSMIT, set);
  DisplayStreamData(SelectedStream);
}

// When the record gets clicked
static void
on_StreamRecord_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (started) return;
  bool set = gtk_toggle_button_get_active(togglebutton);
  CMediaStream *ms = GetSelectedStream();
  if (ms == NULL) return;
  ms->SetBoolValue(STREAM_RECORD, set);
  DisplayStreamRecord(ms);
}

// List of files, their stream configuration indexes
static const struct {
  config_index_t *pConfigIndex;
  const char *header;
  const char *widget;
} FileOpenIndexes[] = {
  { &STREAM_RECORD_MP4_FILE_NAME,
    "MP4 File Name",
    "StreamRecordFileEntry" },
  { &STREAM_SDP_FILE_NAME,
    "Select Stream SDP Output File",
    "StreamSdpFileEntry"},
};

// The return from the browse button for SDP or mp4 file. 
static void onFileOpenResponse (GtkDialog *sel,
				gint response_id,
				gpointer user_data)
{
  if (GTK_RESPONSE_OK == response_id) {
    CMediaStream *ms = GetSelectedStream();
    uint index = GPOINTER_TO_INT(user_data);
    const char *name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(sel));
    ms->SetStringValue(*FileOpenIndexes[index].pConfigIndex,
		       name);
    GtkWidget *wid = lookup_widget(MainWindow, 
				   FileOpenIndexes[index].widget);
    gtk_entry_set_text(GTK_ENTRY(wid), name);
  }
  gtk_widget_destroy(GTK_WIDGET(sel));
}

// create the file selection dialog for SDP file or mp4 file.
static void
on_FileOpenButton_clicked        (GtkButton       *button,
				  gpointer         user_data)
{
  uint index = GPOINTER_TO_INT(user_data);
  GtkWidget *FileSelection;
  GtkWidget *FileOkayButton;
  GtkWidget *cancel_button1;

  FileSelection = 
    gtk_file_selection_new(FileOpenIndexes[index].header);

  gtk_container_set_border_width(GTK_CONTAINER (FileSelection), 10);
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION (FileSelection));

  CMediaStream *ms = GetSelectedStream();
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(FileSelection),
				  ms->GetStringValue(*FileOpenIndexes[index].pConfigIndex));

  FileOkayButton = GTK_FILE_SELECTION (FileSelection)->ok_button;
  gtk_widget_show(FileOkayButton);
  GTK_WIDGET_SET_FLAGS(FileOkayButton, GTK_CAN_DEFAULT);

  cancel_button1 = GTK_FILE_SELECTION(FileSelection)->cancel_button;
  gtk_widget_show(cancel_button1);
  GTK_WIDGET_SET_FLAGS(cancel_button1, GTK_CAN_DEFAULT);
 
  gtk_window_set_modal(GTK_WINDOW(FileSelection), true);
  gtk_window_set_transient_for(GTK_WINDOW(FileSelection), 
			       GTK_WINDOW(MainWindow));
  g_signal_connect((gpointer) FileSelection, "response",
		   G_CALLBACK (onFileOpenResponse),
		   user_data);
  gtk_widget_show(FileSelection);
}

// Stream video settings handlers
static void
on_VideoEnabled_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (started) 
    return;
  
  CMediaStream *ms = GetSelectedStream();
  if (ms == NULL) return;

  ms->SetBoolValue(STREAM_VIDEO_ENABLED, 
		   gtk_toggle_button_get_active(togglebutton));
  ms->Initialize();
  AVFlow->ValidateAndUpdateStreams();
  DisplayStreamData(SelectedStream);
}

static void
on_VideoProfile_changed                (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  if (started) {
    DisplayStreamData(SelectedStream);
    return;
  }

  if (video_profile_names != NULL) {
    CMediaStream *ms = GetSelectedStream();
    if (ms == NULL) return;
    const char *new_profile = 
      video_profile_names[gtk_option_menu_get_history(optionmenu)];
    if (strcmp(new_profile, add_profile_string) == 0) {
      CreateVideoProfileDialog(NULL);
    } else if (strcmp(new_profile, customize_profile_string) == 0) {
      CreateVideoProfileDialog(ms->GetVideoProfile());
    } else if (strcmp(new_profile, 
		      ms->GetStringValue(STREAM_VIDEO_PROFILE)) != 0) {
      ms->SetVideoProfile(new_profile);
      AVFlow->ValidateAndUpdateStreams();
      MainWindowDisplaySources();
      if (MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
	AVFlow->StartVideoPreview();
      }
      DisplayStreamData(SelectedStream);
    }
  }
}

// inpreview is required, because calling DisplayStreamData may cause
// other signals to be emited, calling this more than 1 time
static bool inpreview = false;

static void
on_VideoPreview                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (inpreview) return;
  inpreview = true;

  int type = GPOINTER_TO_INT(user_data);

  int raw_toggled = (GPOINTER_TO_INT(user_data) == 0);
  GtkWidget *raw, *stream;
  raw = lookup_widget(MainWindow, "VideoSourcePreview");
  stream = lookup_widget(MainWindow, "StreamVideoPreview");
  bool raw_set, stream_set;
  const char *stream_name;

  raw_set = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(raw));
  stream_set = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(stream));

  if (type == 2 || (raw_set == false && stream_set == false)) {
    if (type != 2 && MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW)) {
      stream_name = MyConfig->GetStringValue(CONFIG_VIDEO_PREVIEW_STREAM);
      if (raw_toggled == false &&
	  stream_name && strcmp(stream_name, SelectedStream) != 0) {
	// we're turning off for this preview
	inpreview = false;
	return;
      }
      
    }
    if (raw_set) 
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(raw), false);
    if (stream_set) 
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stream), false);
    MyConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW, false);
    AVFlow->StopVideoPreview();
  } else {
    // one or the other is on
    if (raw_set == stream_set) {
      // both are on - figure from userdata
      if (raw_toggled) {
	// we want raw
	stream_set = false;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stream), false);
      } else {
	raw_set = false;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(raw), false);
      }
    }
    
    if (MyConfig->GetBoolValue(CONFIG_VIDEO_PREVIEW) == true) {
      if (raw_set && raw_toggled == false) {
	if (MyConfig->GetStringValue(CONFIG_VIDEO_PREVIEW_STREAM) == NULL) 
	  inpreview = false;
	  return;
      } else {
	stream_name = MyConfig->GetStringValue(CONFIG_VIDEO_PREVIEW_STREAM);
	if (raw_toggled == false &&
	    stream_name && strcmp(stream_name, SelectedStream) == 0) {
	  inpreview = false;
	  return;
	}
      }
    }
    MyConfig->SetBoolValue(CONFIG_VIDEO_PREVIEW, true);
    MyConfig->SetStringValue(CONFIG_VIDEO_PREVIEW_STREAM,
			     raw_set  ? NULL : SelectedStream);
    AVFlow->StartVideoPreview();
    if (started == false) {
	status_timer_id = gtk_timeout_add(1000, status_timer, MainWindow);
    }
  }
  inpreview = false;
}

static void
on_VideoTxAddrButton_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  create_IpAddrDialog(GetSelectedStream(), false, true, false);
}


// Stream audio settings handlers
static void
on_AudioEnabled_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  CMediaStream *ms = GetSelectedStream();
  if (ms == NULL) return;

  ms->SetBoolValue(STREAM_AUDIO_ENABLED, 
		   gtk_toggle_button_get_active(togglebutton));
  ms->Initialize();
  AVFlow->ValidateAndUpdateStreams();
  DisplayStreamData(SelectedStream);
}


static void
on_AudioProfile_changed                (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  if (started) {
    DisplayStreamData(SelectedStream);
    return;
  }

  if (audio_profile_names != NULL) {
    CMediaStream *ms = GetSelectedStream();
    if (ms == NULL) return;
    const char *new_profile = 
      audio_profile_names[gtk_option_menu_get_history(optionmenu)];
    if (strcmp(new_profile, add_profile_string) == 0) {
      CreateAudioProfileDialog(NULL);
    } else if (strcmp(new_profile, customize_profile_string) == 0) {
      CreateAudioProfileDialog(ms->GetAudioProfile());
    } else if (strcmp(new_profile, 
		      ms->GetStringValue(STREAM_AUDIO_PROFILE)) != 0) {
      ms->SetAudioProfile(new_profile);
      AVFlow->ValidateAndUpdateStreams();
      MainWindowDisplaySources();
      DisplayStreamData(SelectedStream);
    }
  }
}


static void
on_AudioTxAddrButton_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  create_IpAddrDialog(GetSelectedStream(), true, false, false);
}

#ifdef HAVE_TEXT
static void
on_TextEnabled_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  CMediaStream *ms = GetSelectedStream();
  if (ms == NULL) return;

  ms->SetBoolValue(STREAM_TEXT_ENABLED, 
		   gtk_toggle_button_get_active(togglebutton));
  ms->Initialize();
  AVFlow->ValidateAndUpdateStreams();
  DisplayStreamData(SelectedStream);
}


static void
on_TextProfile_changed                 (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  if (started) {
    DisplayStreamData(SelectedStream);
    return;
  }

  if (text_profile_names != NULL) {
    CMediaStream *ms = GetSelectedStream();
    if (ms == NULL) return;
    const char *new_profile = 
      text_profile_names[gtk_option_menu_get_history(optionmenu)];
    if (strcmp(new_profile, add_profile_string) == 0) {
      create_TextProfileDialog(NULL);
    } else if (strcmp(new_profile, customize_profile_string) == 0) {
      create_TextProfileDialog(ms->GetTextProfile());
    } else if (strcmp(new_profile, 
		      ms->GetStringValue(STREAM_TEXT_PROFILE)) != 0) {
      ms->SetTextProfile(new_profile);
      AVFlow->ValidateAndUpdateStreams();
      MainWindowDisplaySources();
      DisplayStreamData(SelectedStream);
    }
  }
}


static void
on_TextTxAddrButton_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  create_IpAddrDialog(GetSelectedStream(), false, false, true);
}

#ifdef HAVE_TEXT_ENTRY
static void
on_TextEntrySend_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{

}
#endif
#endif


// If they change the duration.  Don't know why we care.
static void
on_durationtype_activate               (GtkOptionMenu     *menu,
                                        gpointer         user_data)
{
  MyConfig->SetIntegerValue(CONFIG_APP_DURATION_UNITS, 
			    durationUnitsValues[gtk_option_menu_get_history(menu)]);
}

void DoStart()
{
  GtkWidget *temp;
  ReadConfigFromWindow();
  
  if (!MyConfig->GetBoolValue(CONFIG_AUDIO_ENABLE)
      && !MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE) 
      && !MyConfig->GetBoolValue(CONFIG_TEXT_ENABLE)) {
    ShowMessage("No Media", "Neither audio nor video are enabled", MainWindow);
    return;
  }
  
  // lock out change to settings while media is running
  LockoutChanges(true);
  GtkWidget *statusbar = lookup_widget(MainWindow, "statusbar1");
  gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 
		     0,
		     "Starting");
  
  AVFlow->Start();
  gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
  gtk_statusbar_push(GTK_STATUSBAR(statusbar), 
		     0,
		     "Started");
  if (MyConfig->GetBoolValue(CONFIG_TEXT_ENABLE)) {
    const char *type = MyConfig->GetStringValue(CONFIG_TEXT_SOURCE_TYPE);
    
    bool have_file;
    have_file = strcmp(type, TEXT_SOURCE_FILE_WITH_DIALOG) == 0;
    if (have_file || strcmp(type, TEXT_SOURCE_DIALOG) == 0) {
      GtkWidget *textd = create_TextFileDialog(have_file);
      GLADE_HOOKUP_OBJECT(MainWindow, textd, "TextDialog");
    }
  }

  
  StartTime = GetTimestamp(); 
#if 0
  if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
    if (MyConfig->IsCaptureVideoSource()) {
      AVFlow->GetStatus(FLOW_STATUS_VIDEO_ENCODED_FRAMES, 
			&StartEncodedFrameNumber);
    } else {
      StartEncodedFrameNumber = 0;
    }
  }
#endif

  FlowDuration =
    MyConfig->GetIntegerValue(CONFIG_APP_DURATION) *
    MyConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS) * 
    TimestampTicks;
  debug_message(U64" %u dur %u units",
		FlowDuration, 
		MyConfig->GetIntegerValue(CONFIG_APP_DURATION),
		MyConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS)); 
		

#if 0
  if (MyConfig->IsFileVideoSource() && MyConfig->IsFileAudioSource()
      && !MyConfig->GetBoolValue(CONFIG_RTP_ENABLE)) {
    // no real time constraints
    StopTime = 0;
  } else {
    StopTime = StartTime + FlowDuration;
  }
#endif
  StopTime = StartTime + FlowDuration;
	status_start();

	temp = lookup_widget(MainWindow, "StartButton");
	gtk_button_set_label(GTK_BUTTON(temp), "Stop");
	status_timer_id = gtk_timeout_add(1000, status_timer, MainWindow);

	started = true;
}

void DoStop()
{
  if (AVFlow->doingPreview() == false) {
	gtk_timeout_remove(status_timer_id);
  }
	GtkWidget *statusbar = lookup_widget(MainWindow, "statusbar1");
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), 
			   0,
			   "Stopping - Closing recorded files");
			  
	GtkWidget *temp = lookup_widget(MainWindow, "TextDialog");
	if (temp != NULL) {
	  gtk_widget_destroy(temp);
	}

	AVFlow->Stop();
	gtk_statusbar_pop(GTK_STATUSBAR(statusbar), 0);
	gtk_statusbar_push(GTK_STATUSBAR(statusbar), 
			   0,
			   "Session Finished");

	// unlock changes to settings
	LockoutChanges(false);
	temp = lookup_widget(MainWindow, "StartButton");
	gtk_button_set_label(GTK_BUTTON(temp), "Start");

	started = false;
	DisplayStreamData(SelectedStream);
}


static void
on_StartButton_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  if (started == false) {
    DoStart();
  } else {
    DoStop();
  }
  DisplayStreamData(SelectedStream);
}

// Handler for when a stream is clicked.  Go through, and figure out
// which one is selected.  We should get a valid stream name, which we
// then switch to.
static void on_StreamTree_select (GtkTreeSelection *selection,
				  gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *stream;

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    ReadConfigFromWindow();

    gtk_tree_model_get(model, &iter, 0, &stream, -1);
    DisplayStreamData(stream);
    g_free(stream);
  }
}

static void delete_event (GtkWidget *widget, gpointer *data)
{
  // stop the flow (which gets rid of the preview, before we gtk_main_quit
  AVFlow->Stop();
  ReadConfigFromWindow();
  delete AVFlow;
  //  SDL_DestroyMutex(dialog_mutex);
  gtk_main_quit();
}

#ifndef HAVE_SRTP
static const char *addr_button_label = "Set Address";
#else
static const char *addr_button_label = "Set Addr/SRTP Params";
#endif

static GtkWidget *create_MainWindow (void)
{
  GtkWidget *MainWindow;
  GtkWidget *vbox1;
  GtkWidget *menubar1;
  GtkWidget *menuitem1;
  GtkWidget *menuitem1_menu;
  GtkWidget *new1;
  GtkWidget *open1;
  GtkWidget *save1;
  GtkWidget *separatormenuitem1;
  GtkWidget *quit1;
  GtkWidget *menuitem2;
  GtkWidget *menuitem2_menu;
  GtkWidget *generate_addresses;
  GtkWidget *generate_sdp;
  GtkWidget *separator1;
  GtkWidget *restart_recording;
  GtkWidget *separator3;
  GtkWidget *preferences1;
  GtkWidget *menuitem4;
  GtkWidget *menuitem4_menu;
  GtkWidget *about1;

  GtkWidget *InputFrame;
  GtkWidget *hbox97;
  GtkWidget *vbox41;
  GtkWidget *hbox100;
  GtkWidget *label189;
  GtkWidget *VideoSourceLabel;
  GtkWidget *hbox101;
  GtkWidget *label190;
  GtkWidget *AudioSourceLabel;
  GtkWidget *VideoSourcePreview;
  GtkWidget *hbox98;
  GtkWidget *SourceOptionMenu;
  GtkWidget *menu14;
  GtkWidget *Change;
  GtkWidget *separator2;
  GtkWidget *VideoSourceMenu;
  GtkWidget *image27;
  GtkWidget *AudioSourceMenu;
  GtkWidget *image28;
#ifdef HAVE_TEXT
  GtkWidget *TextSourceMenu;
  GtkWidget *image29;
#endif
  GtkWidget *picture_settings;
  GtkWidget *image32;
  GtkWidget *label184;
  GtkWidget *vbox40;
  GtkWidget *hbox99;


  GtkWidget *StreamInfoFrame;
  GtkWidget *hpaned1;
  GtkWidget *StreamVbox;
  GtkWidget *stream_label;
  GtkWidget *StreamScrolledWindow;
  GtkWidget *StreamTreeView;
  GtkWidget *hbuttonbox1;
  GtkWidget *AddStreamButton;
  GtkWidget *image1;
  GtkWidget *DeleteStreamButton;
  GtkWidget *image2;
  GtkWidget *StreamInfoVbox;
  GtkWidget *StreamVideoInfo;
  GtkWidget *StreamAudioInfo;
  GtkWidget *hbox34;
  GtkWidget *label47;
  GtkWidget *StreamFps;
  GtkWidget *hbox33;
  GtkWidget *label45;
  GtkWidget *StreamRecording;
  GtkWidget *InfoVbox;
  GtkWidget *StreamFrame;
  GtkWidget *StreamFrameVbox;
  GtkWidget *StreamNameHbox;
  GtkWidget *label3;
  GtkWidget *StreamNameLabel;
  GtkWidget *StreamCaptionHbox;
  GtkWidget *label32;
  GtkWidget *StreamCaption;
  GtkWidget *StreamDescHbox;
  GtkWidget *label31;
  GtkWidget *StreamDescription;
  GtkWidget *StreamTransmitHbox;
  GtkWidget *StreamTransmit;
  GtkWidget *StreamSdpFile;
  GtkWidget *StreamSdpFileEntry;
  GtkWidget *label30;
  GtkWidget *SDPFileOpenButton;
  GtkWidget *alignment22;
  GtkWidget *hbox81;
  GtkWidget *image22;
  GtkWidget *label167;
  GtkWidget *StreamRecordHbox;
  GtkWidget *StreamRecord;
  GtkWidget *RecordFileHbox;
  GtkWidget *label4;
  GtkWidget *StreamRecordFileEntry;
  GtkWidget *RecordFileOpenButton;
  GtkWidget *alignment23;
  GtkWidget *hbox82;
  GtkWidget *image23;
  GtkWidget *label168;
  GtkWidget *label2;
  GtkWidget *VideoFrame;
  GtkWidget *VideoFrameVbox;
  GtkWidget *VideoFrameHbox1;
  GtkWidget *VideoEnabled;
  GtkWidget *hbox7;
  GtkWidget *label6;
  GtkWidget *VideoProfile;
  GtkWidget *menu5;
  GtkWidget *StreamVideoPreview;
  GtkWidget *VideoTxTable;
  GtkWidget *label124;
  GtkWidget *VideoTxAddrLabel;
  GtkWidget *VideoTxAddrButton;
  GtkWidget *label5;
  GtkWidget *AudioFrame;
  GtkWidget *AudioFrameVbox;
  GtkWidget *AudioFrameLine1;
  GtkWidget *AudioEnabled;
  GtkWidget *hbox15;
  GtkWidget *label12;
  GtkWidget *AudioProfile;
  GtkWidget *menu6;
  GtkWidget *AudioTxTable;
  GtkWidget *label126;
  GtkWidget *AudioTxAddrLabel;
  GtkWidget *AudioTxAddrButton;
  GtkWidget *label9;
#ifdef HAVE_TEXT
  GtkWidget *hbox102;
  GtkWidget *label192;
  GtkWidget *TextSourceLabel;
  GtkWidget *StreamTextInfo;
  GtkWidget *TextFrame;
  GtkWidget *TextFrameVbox;
  GtkWidget *TextFrameHbox1;
  GtkWidget *TextEnabled;
  GtkWidget *hbox17;
  GtkWidget *label13;
  GtkWidget *TextProfile;
  GtkWidget *menu7;
  GtkWidget *TextTxTable;
  GtkWidget *label128;
  GtkWidget *TextTxAddrLabel;
  GtkWidget *TextTxAddrButton;
  GtkWidget *label16;
#ifdef HAVE_TEXT_ENTRY
  GtkWidget *TextEntryFrame;
  GtkWidget *TextEntryHbox;
  GtkWidget *label51;
  GtkWidget *TextEntry;
  GtkWidget *TextEntrySend;
  GtkWidget *label52;
#endif
#endif
  GtkWidget *label114;
  GtkWidget *StatusFrame;
  GtkWidget *StatusHbox;
  GtkWidget *DurationHbox;
  GtkObject *Duration_adj;
  GtkWidget *Duration;
  GtkWidget *DurationLabel;
  GtkWidget *vbox37;
  GtkWidget *DurationType;
  GtkWidget *DurationMenu;
  GtkWidget *menuitem23;
  GtkWidget *menuitem24;
  GtkWidget *menuitem25;
  GtkWidget *menuitem26;
  GtkWidget *menuitem27;
  GtkWidget *vseparator1;
  GtkWidget *TimeTable;
  GtkWidget *label116;
  GtkWidget *label117;
  GtkWidget *label118;
  GtkWidget *label119;
  GtkWidget *CurrentDurationLabel;
  GtkWidget *CurrentTimeLabel;
  GtkWidget *EndTimeLabel;
  GtkWidget *StartTimeSuffix;
  GtkWidget *StartTimeLabel;
  GtkWidget *label183;
  GtkWidget *CurrentTimeSuffix;
  GtkWidget *EndTimeSuffix;
  GtkWidget *vseparator2;
  GtkWidget *vbox34;
  GtkWidget *StartButton;
  GtkWidget *label164;
  GtkWidget *statusbar1;
  GtkAccelGroup *accel_group;

  tooltips = gtk_tooltips_new();

  accel_group = gtk_accel_group_new();

  MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  vbox1 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox1);
  gtk_container_add(GTK_CONTAINER(MainWindow), vbox1);

  menubar1 = gtk_menu_bar_new();
  gtk_widget_show(menubar1);
  gtk_box_pack_start(GTK_BOX(vbox1), menubar1, FALSE, FALSE, 0);

  menuitem1 = gtk_menu_item_new_with_mnemonic(_("_File"));
  gtk_widget_show(menuitem1);
  gtk_container_add(GTK_CONTAINER(menubar1), menuitem1);

  menuitem1_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem1), menuitem1_menu);

  new1 = gtk_image_menu_item_new_from_stock("gtk-new", accel_group);
  gtk_widget_show(new1);
  gtk_container_add(GTK_CONTAINER(menuitem1_menu), new1);

  open1 = gtk_image_menu_item_new_from_stock("gtk-open", accel_group);
  gtk_widget_show(open1);
  gtk_container_add(GTK_CONTAINER(menuitem1_menu), open1);

  save1 = gtk_image_menu_item_new_from_stock("gtk-save", accel_group);
  gtk_widget_show(save1);
  gtk_container_add(GTK_CONTAINER(menuitem1_menu), save1);

  separatormenuitem1 = gtk_menu_item_new();
  gtk_widget_show(separatormenuitem1);
  gtk_container_add(GTK_CONTAINER(menuitem1_menu), separatormenuitem1);
  gtk_widget_set_sensitive(separatormenuitem1, FALSE);

  quit1 = gtk_image_menu_item_new_from_stock("gtk-quit", accel_group);
  gtk_widget_show(quit1);
  gtk_container_add(GTK_CONTAINER(menuitem1_menu), quit1);

  menuitem2 = gtk_menu_item_new_with_mnemonic(_("_Edit"));
  gtk_widget_show(menuitem2);
  gtk_container_add(GTK_CONTAINER(menubar1), menuitem2);

  menuitem2_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem2), menuitem2_menu);

  generate_addresses = gtk_menu_item_new_with_mnemonic(_("_Generate Addresses"));
  gtk_widget_show(generate_addresses);
  gtk_container_add(GTK_CONTAINER(menuitem2_menu), generate_addresses);

  separator1 = gtk_menu_item_new();
  gtk_widget_show(separator1);
  gtk_container_add(GTK_CONTAINER(menuitem2_menu), separator1);
  gtk_widget_set_sensitive(separator1, FALSE);

  generate_sdp = gtk_menu_item_new_with_mnemonic(_("_Generate SDP Files"));
  gtk_widget_show(generate_sdp);
  gtk_container_add(GTK_CONTAINER(menuitem2_menu), generate_sdp);

  separator1 = gtk_menu_item_new();
  gtk_widget_show(separator1);
  gtk_container_add(GTK_CONTAINER(menuitem2_menu), separator1);
  gtk_widget_set_sensitive(separator1, FALSE);

  restart_recording = gtk_menu_item_new_with_mnemonic(_("_Restart Recording"));
  gtk_widget_show(restart_recording);
  gtk_container_add(GTK_CONTAINER(menuitem2_menu), restart_recording);
  gtk_widget_set_sensitive(restart_recording, FALSE);

  separator3 = gtk_menu_item_new();
  gtk_widget_show(separator3);
  gtk_container_add(GTK_CONTAINER(menuitem2_menu), separator3);
  gtk_widget_set_sensitive(separator3, FALSE);

  preferences1 = gtk_image_menu_item_new_from_stock("gtk-preferences", accel_group);
  gtk_widget_show(preferences1);
  gtk_container_add(GTK_CONTAINER(menuitem2_menu), preferences1);

  menuitem4 = gtk_menu_item_new_with_mnemonic(_("_Help"));
  gtk_widget_show(menuitem4);
  gtk_container_add(GTK_CONTAINER(menubar1), menuitem4);

  menuitem4_menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem4), menuitem4_menu);

  about1 = gtk_menu_item_new_with_mnemonic(_("_About"));
  gtk_widget_show(about1);
  gtk_container_add(GTK_CONTAINER(menuitem4_menu), about1);

  InputFrame = gtk_frame_new(NULL);
  gtk_widget_show(InputFrame);
  gtk_box_pack_start(GTK_BOX(vbox1), InputFrame, TRUE, TRUE, 0);

  hbox97 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox97);
  gtk_container_add(GTK_CONTAINER(InputFrame), hbox97);

  vbox41 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox41);
  gtk_box_pack_start(GTK_BOX(hbox97), vbox41, TRUE, TRUE, 0);

  hbox100 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox100);
  gtk_box_pack_start(GTK_BOX(vbox41), hbox100, FALSE, FALSE, 0);

  label189 = gtk_label_new(_("Video Source: "));
  gtk_widget_show(label189);
  gtk_box_pack_start(GTK_BOX(hbox100), label189, FALSE, FALSE, 5);

  VideoSourceLabel = gtk_label_new("");
  gtk_widget_show(VideoSourceLabel);
  gtk_box_pack_start(GTK_BOX(hbox100), VideoSourceLabel, FALSE, FALSE, 4);

  hbox101 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox101);
  gtk_box_pack_start(GTK_BOX(vbox41), hbox101, FALSE, FALSE, 0);

  label190 = gtk_label_new(_("Audio Source: "));
  gtk_widget_show(label190);
  gtk_box_pack_start(GTK_BOX(hbox101), label190, FALSE, FALSE, 5);

  AudioSourceLabel = gtk_label_new("");
  gtk_widget_show(AudioSourceLabel);
  gtk_box_pack_start(GTK_BOX(hbox101), AudioSourceLabel, FALSE, FALSE, 4);

#ifdef HAVE_TEXT
  hbox102 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox102);
  gtk_box_pack_start(GTK_BOX(vbox41), hbox102, FALSE, FALSE, 0);

  label192 = gtk_label_new(_("Text Source:   "));
  gtk_widget_show(label192);
  gtk_box_pack_start(GTK_BOX(hbox102), label192, FALSE, FALSE, 5);

  TextSourceLabel = gtk_label_new("");
  gtk_widget_show(TextSourceLabel);
  gtk_box_pack_start(GTK_BOX(hbox102), TextSourceLabel, FALSE, FALSE, 6);
#endif

  vbox40 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox40);
  gtk_box_pack_start(GTK_BOX(hbox97), vbox40, FALSE, TRUE, 0);

  hbox99 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox99);
  gtk_box_pack_start(GTK_BOX(vbox40), hbox99, FALSE, FALSE, 0);

  VideoSourcePreview = gtk_check_button_new_with_mnemonic(_("Preview Video Source"));
  gtk_widget_show(VideoSourcePreview);
  gtk_box_pack_start(GTK_BOX(hbox99), VideoSourcePreview, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, VideoSourcePreview, _("Preview Raw Video"), NULL);

  hbox98 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox98);
  gtk_box_pack_start(GTK_BOX(vbox40), hbox98, FALSE, FALSE, 0);

  SourceOptionMenu = gtk_option_menu_new();
  gtk_widget_show(SourceOptionMenu);
  gtk_box_pack_start(GTK_BOX(hbox98), SourceOptionMenu, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, SourceOptionMenu, 
		       _("Change Source Settings"), NULL);

  menu14 = gtk_menu_new();
  Change = gtk_menu_item_new_with_mnemonic(_("Change Source"));
  gtk_widget_show(Change);
  gtk_container_add(GTK_CONTAINER(menu14), Change);

  separator2 = gtk_menu_item_new();
  gtk_widget_show(separator2);
  gtk_container_add(GTK_CONTAINER(menu14), separator2);
  gtk_widget_set_sensitive(separator2, FALSE);

  VideoSourceMenu = gtk_image_menu_item_new_with_mnemonic(_("Video Source"));
  gtk_widget_show(VideoSourceMenu);
  gtk_container_add(GTK_CONTAINER(menu14), VideoSourceMenu);

  image27 = gtk_image_new_from_stock("gtk-preferences", GTK_ICON_SIZE_MENU);
  gtk_widget_show(image27);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(VideoSourceMenu), image27);

  AudioSourceMenu = gtk_image_menu_item_new_with_mnemonic(_("Audio Source"));
  gtk_widget_show(AudioSourceMenu);
  gtk_container_add(GTK_CONTAINER(menu14), AudioSourceMenu);

  image28 = gtk_image_new_from_stock("gtk-preferences", GTK_ICON_SIZE_MENU);
  gtk_widget_show(image28);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(AudioSourceMenu), image28);
  picture_settings = gtk_image_menu_item_new_with_mnemonic (_("Picture Settings"));
  gtk_widget_show (picture_settings);
  gtk_container_add (GTK_CONTAINER (menu14), picture_settings);

  image32 = gtk_image_new_from_stock ("gtk-preferences", GTK_ICON_SIZE_MENU);
  gtk_widget_show (image32);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (picture_settings), image32);

#ifdef HAVE_TEXT
  TextSourceMenu = gtk_image_menu_item_new_with_mnemonic(_("Text Source"));
  gtk_widget_show(TextSourceMenu);
  gtk_container_add(GTK_CONTAINER(menu14), TextSourceMenu);

  image29 = gtk_image_new_from_stock("gtk-preferences", GTK_ICON_SIZE_MENU);
  gtk_widget_show(image29);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(TextSourceMenu), image29);
#endif

  gtk_option_menu_set_menu(GTK_OPTION_MENU(SourceOptionMenu), menu14);

  label184 = gtk_label_new(_("Inputs"));
  gtk_widget_show(label184);
  gtk_frame_set_label_widget(GTK_FRAME(InputFrame), label184);

  // stream frame
  StreamInfoFrame = gtk_frame_new(NULL);
  gtk_widget_show(StreamInfoFrame);
  gtk_box_pack_start(GTK_BOX(vbox1), StreamInfoFrame, FALSE, FALSE, 0);

  hpaned1 = gtk_hpaned_new();
  gtk_widget_show(hpaned1);
  gtk_container_add(GTK_CONTAINER(StreamInfoFrame), hpaned1);
  gtk_paned_set_position(GTK_PANED(hpaned1), 175);

  StreamVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(StreamVbox);
  gtk_paned_pack1(GTK_PANED(hpaned1), StreamVbox, FALSE, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(StreamVbox), 4);

  stream_label = gtk_label_new(_("Streams"));
  gtk_widget_show(stream_label);
  gtk_box_pack_start(GTK_BOX(StreamVbox), stream_label, FALSE, FALSE, 0);

  StreamScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(StreamScrolledWindow);
  gtk_box_pack_start(GTK_BOX(StreamVbox), StreamScrolledWindow, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(StreamScrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(StreamScrolledWindow), GTK_SHADOW_ETCHED_IN);

  // tree view
  StreamTreeView = gtk_tree_view_new();
  gtk_widget_show(StreamTreeView);
  gtk_container_add(GTK_CONTAINER(StreamScrolledWindow), StreamTreeView);
  gtk_tooltips_set_tip(tooltips, StreamTreeView, _("Select stream to view information"), NULL);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(StreamTreeView), FALSE);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(StreamTreeView), TRUE);

  GtkCellRenderer *rend = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column;
  column = gtk_tree_view_column_new_with_attributes("Stream",
						    rend,
						    "text", 0, (void *)NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(StreamTreeView), column);

  hbuttonbox1 = gtk_hbutton_box_new();
  gtk_widget_show(hbuttonbox1);
  gtk_box_pack_start(GTK_BOX(StreamVbox), hbuttonbox1, FALSE, TRUE, 0);

  AddStreamButton = gtk_button_new();
  gtk_widget_show(AddStreamButton);
  gtk_container_add(GTK_CONTAINER(hbuttonbox1), AddStreamButton);
  GTK_WIDGET_SET_FLAGS(AddStreamButton, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip(tooltips, AddStreamButton, _("Add Stream"), NULL);

  image1 = gtk_image_new_from_stock("gtk-add", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image1);
  gtk_container_add(GTK_CONTAINER(AddStreamButton), image1);

  DeleteStreamButton = gtk_button_new();
  gtk_widget_show(DeleteStreamButton);
  gtk_container_add(GTK_CONTAINER(hbuttonbox1), DeleteStreamButton);
  gtk_widget_set_size_request(DeleteStreamButton, 38, 38);
  GTK_WIDGET_SET_FLAGS(DeleteStreamButton, GTK_CAN_DEFAULT);
  gtk_tooltips_set_tip(tooltips, DeleteStreamButton, _("Delete Stream"), NULL);

  image2 = gtk_image_new_from_stock("gtk-delete", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image2);
  gtk_container_add(GTK_CONTAINER(DeleteStreamButton), image2);

  StreamInfoVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(StreamInfoVbox);
  gtk_box_pack_start(GTK_BOX(StreamVbox), StreamInfoVbox, FALSE, TRUE, 10);

  StreamVideoInfo = gtk_label_new(_("video"));
  gtk_widget_show(StreamVideoInfo);
  gtk_box_pack_start(GTK_BOX(StreamInfoVbox), StreamVideoInfo, FALSE, FALSE, 0);

  StreamAudioInfo = gtk_label_new(_("audio"));
  gtk_widget_show(StreamAudioInfo);
  gtk_box_pack_start(GTK_BOX(StreamInfoVbox), StreamAudioInfo, FALSE, FALSE, 0);

#ifdef HAVE_TEXT
  StreamTextInfo = gtk_label_new("");
  gtk_widget_show(StreamTextInfo);
  gtk_box_pack_start(GTK_BOX(StreamInfoVbox), StreamTextInfo, FALSE, FALSE, 0);
#endif

  hbox34 = gtk_hbox_new(TRUE, 0);
  gtk_widget_show(hbox34);
  gtk_box_pack_start(GTK_BOX(StreamInfoVbox), hbox34, FALSE, FALSE, 0);

  label47 = gtk_label_new(_("FPS:"));
  gtk_widget_show(label47);
  gtk_box_pack_start(GTK_BOX(hbox34), label47, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label47), GTK_JUSTIFY_RIGHT);

  StreamFps = gtk_label_new("");
  gtk_widget_show(StreamFps);
  gtk_box_pack_start(GTK_BOX(hbox34), StreamFps, FALSE, FALSE, 0);

  hbox33 = gtk_hbox_new(TRUE, 0);
  gtk_widget_show(hbox33);
  gtk_box_pack_start(GTK_BOX(StreamInfoVbox), hbox33, FALSE, FALSE, 0);

  label45 = gtk_label_new(_("Recording:"));
  gtk_widget_show(label45);
  gtk_box_pack_start(GTK_BOX(hbox33), label45, FALSE, FALSE, 0);
  gtk_label_set_justify(GTK_LABEL(label45), GTK_JUSTIFY_RIGHT);

  StreamRecording = gtk_label_new("");
  gtk_widget_show(StreamRecording);
  gtk_box_pack_start(GTK_BOX(hbox33), StreamRecording, FALSE, FALSE, 0);

  InfoVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(InfoVbox);
  gtk_paned_pack2(GTK_PANED(hpaned1), InfoVbox, TRUE, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(InfoVbox), 4);

  StreamFrame = gtk_frame_new(NULL);
  gtk_widget_show(StreamFrame);
  gtk_box_pack_start(GTK_BOX(InfoVbox), StreamFrame, TRUE, TRUE, 1);

  StreamFrameVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(StreamFrameVbox);
  gtk_container_add(GTK_CONTAINER(StreamFrame), StreamFrameVbox);
  gtk_container_set_border_width(GTK_CONTAINER(StreamFrameVbox), 2);

  StreamNameHbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(StreamNameHbox);
  gtk_box_pack_start(GTK_BOX(StreamFrameVbox), StreamNameHbox, TRUE, TRUE, 1);

  label3 = gtk_label_new(_("Stream:"));
  gtk_widget_show(label3);
  gtk_box_pack_start(GTK_BOX(StreamNameHbox), label3, FALSE, FALSE, 0);

  StreamNameLabel = gtk_label_new(_("StreamName"));
  gtk_widget_show(StreamNameLabel);
  gtk_box_pack_start(GTK_BOX(StreamNameHbox), StreamNameLabel, FALSE, FALSE, 4);

  StreamCaptionHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(StreamCaptionHbox);
  gtk_box_pack_start(GTK_BOX(StreamFrameVbox), StreamCaptionHbox, TRUE, FALSE, 0);

  label32 = gtk_label_new(_("Caption: "));
  gtk_widget_show(label32);
  gtk_box_pack_start(GTK_BOX(StreamCaptionHbox), label32, FALSE, FALSE, 0);

  StreamCaption = gtk_entry_new();
  gtk_widget_show(StreamCaption);
  gtk_box_pack_start(GTK_BOX(StreamCaptionHbox), StreamCaption, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, StreamCaption, _("The caption for this stream, used to set S field in sdp"), NULL);

  StreamDescHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(StreamDescHbox);
  gtk_box_pack_start(GTK_BOX(StreamFrameVbox), StreamDescHbox, TRUE, FALSE, 0);

  label31 = gtk_label_new(_("Description: "));
  gtk_widget_show(label31);
  gtk_box_pack_start(GTK_BOX(StreamDescHbox), label31, FALSE, FALSE, 0);

  StreamDescription = gtk_entry_new();
  gtk_widget_show(StreamDescription);
  gtk_box_pack_start(GTK_BOX(StreamDescHbox), StreamDescription, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, StreamDescription, _("Description for SDP, recorded file"), NULL);

  StreamTransmitHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(StreamTransmitHbox);
  gtk_box_pack_start(GTK_BOX(StreamFrameVbox), StreamTransmitHbox, TRUE, FALSE, 0);

  StreamTransmit = gtk_check_button_new_with_mnemonic(_("Transmit"));
  gtk_widget_show(StreamTransmit);
  gtk_box_pack_start(GTK_BOX(StreamTransmitHbox), StreamTransmit, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, StreamTransmit, _("Transmit Stream"), NULL);

  StreamSdpFile = gtk_hbox_new(FALSE, 6);
  gtk_widget_show(StreamSdpFile);
  gtk_box_pack_start(GTK_BOX(StreamTransmitHbox), StreamSdpFile, TRUE, TRUE, 20);

  label30 = gtk_label_new(_("SDP File:"));
  gtk_widget_show(label30);
  gtk_box_pack_start(GTK_BOX(StreamSdpFile), label30, FALSE, FALSE, 0);

  StreamSdpFileEntry = gtk_entry_new();
  gtk_widget_show(StreamSdpFileEntry);
  gtk_box_pack_start(GTK_BOX(StreamSdpFile), StreamSdpFileEntry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, StreamSdpFileEntry, _("SDP File Name"), NULL);

  SDPFileOpenButton = gtk_button_new();
  gtk_widget_show(SDPFileOpenButton);
  gtk_box_pack_start(GTK_BOX(StreamSdpFile), SDPFileOpenButton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, SDPFileOpenButton, _("Select SDP File"), NULL);

  alignment22 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment22);
  gtk_container_add(GTK_CONTAINER(SDPFileOpenButton), alignment22);

  hbox81 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox81);
  gtk_container_add(GTK_CONTAINER(alignment22), hbox81);

  image22 = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image22);
  gtk_box_pack_start(GTK_BOX(hbox81), image22, FALSE, FALSE, 0);

  label167 = gtk_label_new_with_mnemonic(_("Select"));
  gtk_widget_show(label167);
  gtk_box_pack_start(GTK_BOX(hbox81), label167, FALSE, FALSE, 0);

  StreamRecordHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(StreamRecordHbox);
  gtk_box_pack_start(GTK_BOX(StreamFrameVbox), StreamRecordHbox, TRUE, FALSE, 0);

  StreamRecord = gtk_check_button_new_with_mnemonic(_("Record as MP4"));
  gtk_widget_show(StreamRecord);
  gtk_box_pack_start(GTK_BOX(StreamRecordHbox), StreamRecord, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, StreamRecord, _("Enable Recording for Stream"), NULL);

  RecordFileHbox = gtk_hbox_new(FALSE, 6);
  gtk_widget_show(RecordFileHbox);
  gtk_box_pack_start(GTK_BOX(StreamRecordHbox), RecordFileHbox, TRUE, TRUE, 20);

  label4 = gtk_label_new(_("File:"));
  gtk_widget_show(label4);
  gtk_box_pack_start(GTK_BOX(RecordFileHbox), label4, FALSE, FALSE, 0);

  StreamRecordFileEntry = gtk_entry_new();
  gtk_widget_show(StreamRecordFileEntry);
  gtk_box_pack_start(GTK_BOX(RecordFileHbox), StreamRecordFileEntry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, StreamRecordFileEntry, _("File to record to"), NULL);

  RecordFileOpenButton = gtk_button_new();
  gtk_widget_show(RecordFileOpenButton);
  gtk_box_pack_start(GTK_BOX(RecordFileHbox), RecordFileOpenButton, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, RecordFileOpenButton, _("Select Record File"), NULL);

  alignment23 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment23);
  gtk_container_add(GTK_CONTAINER(RecordFileOpenButton), alignment23);

  hbox82 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox82);
  gtk_container_add(GTK_CONTAINER(alignment23), hbox82);

  image23 = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image23);
  gtk_box_pack_start(GTK_BOX(hbox82), image23, FALSE, FALSE, 0);

  label168 = gtk_label_new_with_mnemonic(_("Select"));
  gtk_widget_show(label168);
  gtk_box_pack_start(GTK_BOX(hbox82), label168, FALSE, FALSE, 0);

  label2 = gtk_label_new(_("Stream Information"));
  gtk_widget_show(label2);
  gtk_frame_set_label_widget(GTK_FRAME(StreamFrame), label2);

  VideoFrame = gtk_frame_new(NULL);
  gtk_widget_show(VideoFrame);
  gtk_box_pack_start(GTK_BOX(InfoVbox), VideoFrame, TRUE, TRUE, 0);

  VideoFrameVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(VideoFrameVbox);
  gtk_container_add(GTK_CONTAINER(VideoFrame), VideoFrameVbox);
  gtk_container_set_border_width(GTK_CONTAINER(VideoFrameVbox), 2);

  VideoFrameHbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(VideoFrameHbox1);
  gtk_box_pack_start(GTK_BOX(VideoFrameVbox), VideoFrameHbox1, TRUE, FALSE, 0);

  VideoEnabled = gtk_check_button_new_with_mnemonic(_("Enabled"));
  gtk_widget_show(VideoEnabled);
  gtk_box_pack_start(GTK_BOX(VideoFrameHbox1), VideoEnabled, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, VideoEnabled, _("Enable Video For Stream"), NULL);

  hbox7 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox7);
  gtk_box_pack_start(GTK_BOX(VideoFrameHbox1), hbox7, TRUE, TRUE, 31);

  label6 = gtk_label_new(_("Profile:"));
  gtk_widget_show(label6);
  gtk_box_pack_start(GTK_BOX(hbox7), label6, FALSE, FALSE, 0);

  VideoProfile = gtk_option_menu_new();
  gtk_widget_show(VideoProfile);
  gtk_box_pack_start(GTK_BOX(hbox7), VideoProfile, FALSE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, VideoProfile, _("Select/Add/Customize Video Profile"), NULL);

  menu5 = gtk_menu_new();

  gtk_option_menu_set_menu(GTK_OPTION_MENU(VideoProfile), menu5);

  StreamVideoPreview = gtk_check_button_new_with_mnemonic(_("Preview"));
  gtk_widget_show(StreamVideoPreview);
  gtk_box_pack_start(GTK_BOX(VideoFrameHbox1), StreamVideoPreview, FALSE, FALSE, 19);
  gtk_tooltips_set_tip(tooltips, StreamVideoPreview, _("Enable Encoded Preview Window"), NULL);

  VideoTxTable = gtk_table_new(1, 3, FALSE);
  gtk_widget_show(VideoTxTable);
  gtk_box_pack_start(GTK_BOX(VideoFrameVbox), VideoTxTable, TRUE, TRUE, 0);

  label124 = gtk_label_new(_("Transmit Address:"));
  gtk_widget_show(label124);
  gtk_table_attach(GTK_TABLE(VideoTxTable), label124, 0, 1, 0, 1,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label124), 0, 0.5);

  VideoTxAddrLabel = gtk_label_new("");
  gtk_widget_show(VideoTxAddrLabel);
  gtk_table_attach(GTK_TABLE(VideoTxTable), VideoTxAddrLabel, 1, 2, 0, 1,
                 (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(VideoTxAddrLabel), 0, 0.5);

  VideoTxAddrButton = gtk_button_new_with_mnemonic(_(addr_button_label));
  gtk_widget_show(VideoTxAddrButton);
  gtk_table_attach(GTK_TABLE(VideoTxTable), VideoTxAddrButton, 2, 3, 0, 1,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 11, 0);
  gtk_tooltips_set_tip(tooltips, VideoTxAddrButton, _("Set Transmission Address and Parameters"), NULL);

  label5 = gtk_label_new(_("Video"));
  gtk_widget_show(label5);
  gtk_frame_set_label_widget(GTK_FRAME(VideoFrame), label5);

  AudioFrame = gtk_frame_new(NULL);
  gtk_widget_show(AudioFrame);
  gtk_box_pack_start(GTK_BOX(InfoVbox), AudioFrame, TRUE, TRUE, 0);

  AudioFrameVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(AudioFrameVbox);
  gtk_container_add(GTK_CONTAINER(AudioFrame), AudioFrameVbox);
  gtk_container_set_border_width(GTK_CONTAINER(AudioFrameVbox), 2);

  AudioFrameLine1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(AudioFrameLine1);
  gtk_box_pack_start(GTK_BOX(AudioFrameVbox), AudioFrameLine1, TRUE, FALSE, 0);

  AudioEnabled = gtk_check_button_new_with_mnemonic(_("Enabled"));
  gtk_widget_show(AudioEnabled);
  gtk_box_pack_start(GTK_BOX(AudioFrameLine1), AudioEnabled, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, AudioEnabled, _("Enable Audio For Stream"), NULL);

  hbox15 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox15);
  gtk_box_pack_start(GTK_BOX(AudioFrameLine1), hbox15, TRUE, TRUE, 31);

  label12 = gtk_label_new(_("Profile:"));
  gtk_widget_show(label12);
  gtk_box_pack_start(GTK_BOX(hbox15), label12, FALSE, FALSE, 0);

  AudioProfile = gtk_option_menu_new();
  gtk_widget_show(AudioProfile);
  gtk_box_pack_start(GTK_BOX(hbox15), AudioProfile, FALSE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, AudioProfile, _("Select/Add/Customize Audio Profile"), NULL);

  menu6 = gtk_menu_new();

  gtk_option_menu_set_menu(GTK_OPTION_MENU(AudioProfile), menu6);

  AudioTxTable = gtk_table_new(1, 3, FALSE);
  gtk_widget_show(AudioTxTable);
  gtk_box_pack_start(GTK_BOX(AudioFrameVbox), AudioTxTable, TRUE, TRUE, 2);

  label126 = gtk_label_new(_("Transmit Address:"));
  gtk_widget_show(label126);
  gtk_table_attach(GTK_TABLE(AudioTxTable), label126, 0, 1, 0, 1,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label126), 0, 0.5);

  AudioTxAddrLabel = gtk_label_new("");
  gtk_widget_show(AudioTxAddrLabel);
  gtk_table_attach(GTK_TABLE(AudioTxTable), AudioTxAddrLabel, 1, 2, 0, 1,
                 (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(AudioTxAddrLabel), 0, 0.5);

  AudioTxAddrButton = gtk_button_new_with_mnemonic(_(addr_button_label));
  gtk_widget_show(AudioTxAddrButton);
  gtk_table_attach(GTK_TABLE(AudioTxTable), AudioTxAddrButton, 2, 3, 0, 1,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 11, 0);
  gtk_tooltips_set_tip(tooltips, AudioTxAddrButton, _("Set Transmission Address and Parameters"), NULL);

  label9 = gtk_label_new(_("Audio"));
  gtk_widget_show(label9);
  gtk_frame_set_label_widget(GTK_FRAME(AudioFrame), label9);

#ifdef HAVE_TEXT
  TextFrame = gtk_frame_new(NULL);
  gtk_widget_show(TextFrame);
  gtk_box_pack_start(GTK_BOX(InfoVbox), TextFrame, TRUE, TRUE, 0);

  TextFrameVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(TextFrameVbox);
  gtk_container_add(GTK_CONTAINER(TextFrame), TextFrameVbox);
  gtk_container_set_border_width(GTK_CONTAINER(TextFrameVbox), 2);

  TextFrameHbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(TextFrameHbox1);
  gtk_box_pack_start(GTK_BOX(TextFrameVbox), TextFrameHbox1, TRUE, FALSE, 0);

  TextEnabled = gtk_check_button_new_with_mnemonic(_("Enabled"));
  gtk_widget_show(TextEnabled);
  gtk_box_pack_start(GTK_BOX(TextFrameHbox1), TextEnabled, FALSE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, TextEnabled, _("Enable Text for Stream"), NULL);

  hbox17 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox17);
  gtk_box_pack_start(GTK_BOX(TextFrameHbox1), hbox17, TRUE, TRUE, 31);

  label13 = gtk_label_new(_("Profile:"));
  gtk_widget_show(label13);
  gtk_box_pack_start(GTK_BOX(hbox17), label13, FALSE, FALSE, 0);

  TextProfile = gtk_option_menu_new();
  gtk_widget_show(TextProfile);
  gtk_box_pack_start(GTK_BOX(hbox17), TextProfile, FALSE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, TextProfile, _("Select/Add/Customize Text Profile"), NULL);

  menu7 = gtk_menu_new();

  gtk_option_menu_set_menu(GTK_OPTION_MENU(TextProfile), menu7);

  TextTxTable = gtk_table_new(1, 3, FALSE);
  gtk_widget_show(TextTxTable);
  gtk_box_pack_start(GTK_BOX(TextFrameVbox), TextTxTable, TRUE, TRUE, 2);

  label128 = gtk_label_new(_("Transmit Address:"));
  gtk_widget_show(label128);
  gtk_table_attach(GTK_TABLE(TextTxTable), label128, 0, 1, 0, 1,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label128), 0, 0.5);

  TextTxAddrLabel = gtk_label_new("");
  gtk_widget_show(TextTxAddrLabel);
  gtk_table_attach(GTK_TABLE(TextTxTable), TextTxAddrLabel, 1, 2, 0, 1,
                 (GtkAttachOptions)(GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(TextTxAddrLabel), 0, 0.5);

  TextTxAddrButton = gtk_button_new_with_mnemonic(_(addr_button_label));
  gtk_widget_show(TextTxAddrButton);
  gtk_table_attach(GTK_TABLE(TextTxTable), TextTxAddrButton, 2, 3, 0, 1,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 11, 0);
  gtk_tooltips_set_tip(tooltips, TextTxAddrButton, _("Set Transmission Address and Parameters"), NULL);

  label16 = gtk_label_new(_("Text"));
  gtk_widget_show(label16);
  gtk_frame_set_label_widget(GTK_FRAME(TextFrame), label16);
#ifdef HAVE_TEXT_ENTRY
  TextEntryFrame = gtk_frame_new(NULL);
  gtk_widget_show(TextEntryFrame);
  gtk_box_pack_start(GTK_BOX(InfoVbox), TextEntryFrame, TRUE, TRUE, 0);

  TextEntryHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(TextEntryHbox);
  gtk_container_add(GTK_CONTAINER(TextEntryFrame), TextEntryHbox);

  label51 = gtk_label_new(_("Text:"));
  gtk_widget_show(label51);
  gtk_box_pack_start(GTK_BOX(TextEntryHbox), label51, FALSE, FALSE, 0);

  TextEntry = gtk_entry_new();
  gtk_widget_show(TextEntry);
  gtk_box_pack_start(GTK_BOX(TextEntryHbox), TextEntry, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, TextEntry, _("Enter Text to send"), NULL);
  gtk_widget_set_sensitive(TextEntry, FALSE);

  TextEntrySend = gtk_button_new_with_mnemonic(_("Send"));
  gtk_widget_show(TextEntrySend);
  gtk_box_pack_start(GTK_BOX(TextEntryHbox), TextEntrySend, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(TextEntrySend, FALSE);
  gtk_tooltips_set_tip(tooltips, TextEntrySend, _("Send Text"), NULL);

  label52 = gtk_label_new(_("Text Transmission"));
  gtk_widget_show(label52);
  gtk_frame_set_label_widget(GTK_FRAME(TextEntryFrame), label52);
#endif
#endif
  label114 = gtk_label_new(_("Outputs"));
  gtk_widget_show(label114);
  gtk_frame_set_label_widget(GTK_FRAME(StreamInfoFrame), label114);

  StatusFrame = gtk_frame_new(NULL);
  gtk_widget_show(StatusFrame);
  gtk_box_pack_start(GTK_BOX(vbox1), StatusFrame, TRUE, TRUE, 0);

  StatusHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(StatusHbox);
  gtk_container_add(GTK_CONTAINER(StatusFrame), StatusHbox);

  DurationHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(DurationHbox);
  gtk_box_pack_start(GTK_BOX(StatusHbox), DurationHbox, FALSE, TRUE, 0);

  DurationLabel = gtk_label_new(_("Duration:"));
  gtk_widget_show(DurationLabel);
  gtk_box_pack_start(GTK_BOX(DurationHbox), DurationLabel, FALSE, FALSE, 0);

  Duration_adj = 
    gtk_adjustment_new(MyConfig->GetIntegerValue(CONFIG_APP_DURATION), 
		       1, 1e+11, 1, 10, 10);
  Duration = gtk_spin_button_new(GTK_ADJUSTMENT(Duration_adj), 1, 0);
  gtk_widget_show(Duration);
  gtk_box_pack_start(GTK_BOX(DurationHbox), Duration, FALSE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, Duration, _("Enter Duration of Session"), NULL);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(Duration), TRUE);

  vbox37 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox37);
  gtk_box_pack_start(GTK_BOX(DurationHbox), vbox37, FALSE, FALSE, 2);

  DurationType = gtk_option_menu_new();
  gtk_widget_show(DurationType);
  gtk_box_pack_start(GTK_BOX(vbox37), DurationType, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, DurationType, _("Select Duration Units"), NULL);

  DurationMenu = gtk_menu_new();

  menuitem23 = gtk_menu_item_new_with_mnemonic(_("Seconds"));
  gtk_widget_show(menuitem23);
  gtk_container_add(GTK_CONTAINER(DurationMenu), menuitem23);

  menuitem24 = gtk_menu_item_new_with_mnemonic(_("Minutes"));
  gtk_widget_show(menuitem24);
  gtk_container_add(GTK_CONTAINER(DurationMenu), menuitem24);

  menuitem25 = gtk_menu_item_new_with_mnemonic(_("Hours"));
  gtk_widget_show(menuitem25);
  gtk_container_add(GTK_CONTAINER(DurationMenu), menuitem25);

  menuitem26 = gtk_menu_item_new_with_mnemonic(_("Days"));
  gtk_widget_show(menuitem26);
  gtk_container_add(GTK_CONTAINER(DurationMenu), menuitem26);

  menuitem27 = gtk_menu_item_new_with_mnemonic(_("Years"));
  gtk_widget_show(menuitem27);
  gtk_container_add(GTK_CONTAINER(DurationMenu), menuitem27);

  gtk_option_menu_set_menu(GTK_OPTION_MENU(DurationType), DurationMenu);

  vseparator1 = gtk_vseparator_new();
  gtk_widget_show(vseparator1);
  gtk_box_pack_start(GTK_BOX(StatusHbox), vseparator1, FALSE, FALSE, 0);

  TimeTable = gtk_table_new(4, 3, FALSE);
  gtk_widget_show(TimeTable);
  gtk_box_pack_start(GTK_BOX(StatusHbox), TimeTable, TRUE, TRUE, 0);

  label116 = gtk_label_new(_("Start Time"));
  gtk_widget_show(label116);
  gtk_table_attach(GTK_TABLE(TimeTable), label116, 0, 1, 0, 1,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label116), 0, 0.5);

  label117 = gtk_label_new(_("Current Duration:"));
  gtk_widget_show(label117);
  gtk_table_attach(GTK_TABLE(TimeTable), label117, 0, 1, 1, 2,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label117), 0, 0.5);

  label118 = gtk_label_new(_("Current Time:"));
  gtk_widget_show(label118);
  gtk_table_attach(GTK_TABLE(TimeTable), label118, 0, 1, 2, 3,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label118), 0, 0.5);

  label119 = gtk_label_new(_("End Time:"));
  gtk_widget_show(label119);
  gtk_table_attach(GTK_TABLE(TimeTable), label119, 0, 1, 3, 4,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label119), 0, 0.5);

  CurrentDurationLabel = gtk_label_new("");
  gtk_widget_show(CurrentDurationLabel);
  gtk_table_attach(GTK_TABLE(TimeTable), CurrentDurationLabel, 1, 2, 1, 2,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_label_set_justify(GTK_LABEL(CurrentDurationLabel), GTK_JUSTIFY_RIGHT);

  CurrentTimeLabel = gtk_label_new("");
  gtk_widget_show(CurrentTimeLabel);
  gtk_table_attach(GTK_TABLE(TimeTable), CurrentTimeLabel, 1, 2, 2, 3,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_label_set_justify(GTK_LABEL(CurrentTimeLabel), GTK_JUSTIFY_RIGHT);

  EndTimeLabel = gtk_label_new("");
  gtk_widget_show(EndTimeLabel);
  gtk_table_attach(GTK_TABLE(TimeTable), EndTimeLabel, 1, 2, 3, 4,
                 (GtkAttachOptions)(GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_label_set_justify(GTK_LABEL(EndTimeLabel), GTK_JUSTIFY_RIGHT);

  StartTimeSuffix = gtk_label_new(_(""));
  gtk_widget_show(StartTimeSuffix);
  gtk_table_attach(GTK_TABLE(TimeTable), StartTimeSuffix, 2, 3, 0, 1,
		 (GtkAttachOptions)(0),
		 (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(StartTimeSuffix), 0, 0.5);

  StartTimeLabel = gtk_label_new("");
  gtk_widget_show(StartTimeLabel);
  gtk_table_attach(GTK_TABLE(TimeTable), StartTimeLabel, 1, 2, 0, 1,
                 (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                 (GtkAttachOptions)(0), 0, 0);
  gtk_label_set_justify(GTK_LABEL(StartTimeLabel), GTK_JUSTIFY_RIGHT);

  label183 = gtk_label_new("");
  gtk_widget_show(label183);
  gtk_table_attach(GTK_TABLE(TimeTable), label183, 2, 3, 1, 2,
                  (GtkAttachOptions)(GTK_FILL),
                  (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label183), 0, 0.5);

  CurrentTimeSuffix = gtk_label_new("");
  gtk_widget_show(CurrentTimeSuffix);
  gtk_table_attach(GTK_TABLE(TimeTable), CurrentTimeSuffix, 2, 3, 2, 3,
                  (GtkAttachOptions)(GTK_FILL),
                  (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(CurrentTimeSuffix), 0, 0.5);

  EndTimeSuffix = gtk_label_new("");
  gtk_widget_show(EndTimeSuffix);
  gtk_table_attach(GTK_TABLE(TimeTable), EndTimeSuffix, 2, 3, 3, 4,
                  (GtkAttachOptions)(GTK_FILL),
                  (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(EndTimeSuffix), 0, 0.5);


  vseparator2 = gtk_vseparator_new();
  gtk_widget_show(vseparator2);
  gtk_box_pack_start(GTK_BOX(StatusHbox), vseparator2, FALSE, TRUE, 0);

  vbox34 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox34);
  gtk_box_pack_start(GTK_BOX(StatusHbox), vbox34, TRUE, TRUE, 0);

  StartButton = gtk_toggle_button_new_with_mnemonic(_("Start"));
  gtk_widget_show(StartButton);
  gtk_box_pack_start(GTK_BOX(vbox34), StartButton, TRUE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(StartButton), 3);
  gtk_tooltips_set_tip(tooltips, StartButton, _("Start all Streams"), NULL);

  label164 = gtk_label_new(_("Status"));
  gtk_widget_show(label164);
  gtk_frame_set_label_widget(GTK_FRAME(StatusFrame), label164);

  statusbar1 = gtk_statusbar_new();
  gtk_widget_show(statusbar1);
  gtk_box_pack_start(GTK_BOX(vbox1), statusbar1, FALSE, FALSE, 0);

  g_signal_connect((gpointer) MainWindow, "delete_event",
                    G_CALLBACK(delete_event),
                    NULL);
  g_signal_connect((gpointer) new1, "activate",
                    G_CALLBACK(on_new1_activate),
                    NULL);
  g_signal_connect((gpointer) open1, "activate",
                    G_CALLBACK(on_open1_activate),
                    NULL);
  g_signal_connect((gpointer) save1, "activate",
                    G_CALLBACK(on_save1_activate),
                    NULL);
  g_signal_connect((gpointer) quit1, "activate",
                    G_CALLBACK(delete_event),
                    NULL);
  g_signal_connect((gpointer) generate_addresses, "activate",
                    G_CALLBACK(on_generate_addresses_activate),
                    NULL);
  g_signal_connect((gpointer)restart_recording, "activate",
		   G_CALLBACK(on_restart_recording_activate),
		   NULL);

  g_signal_connect((gpointer) generate_sdp, "activate",
                    G_CALLBACK(on_generate_sdp_activate),
                    NULL);
  g_signal_connect((gpointer) preferences1, "activate",
                    G_CALLBACK(on_preferences1_activate),
                    NULL);
  g_signal_connect((gpointer) about1, "activate",
                    G_CALLBACK(on_about1_activate),
                    NULL);
  g_signal_connect((gpointer) VideoSourcePreview, "toggled",
                    G_CALLBACK(on_VideoPreview),
                    GINT_TO_POINTER(0));

  g_signal_connect((gpointer) VideoSourceMenu, "activate",
                    G_CALLBACK(on_VideoSourceMenu_activate),
                    NULL);
  g_signal_connect((gpointer) AudioSourceMenu, "activate",
                    G_CALLBACK(on_AudioSourceMenu_activate),
                    NULL);
  g_signal_connect((gpointer) picture_settings, "activate", 
		   G_CALLBACK(on_picture_settings_activate), 
		   NULL);
#ifdef HAVE_TEXT
  g_signal_connect((gpointer) TextSourceMenu, "activate",
                    G_CALLBACK(on_TextSourceMenu_activate),
                    NULL);
#endif

  g_signal_connect((gpointer) AddStreamButton, "clicked",
                    G_CALLBACK(on_AddStreamButton_clicked),
                    NULL);
  g_signal_connect((gpointer) DeleteStreamButton, "clicked",
                    G_CALLBACK(on_DeleteStreamButton_clicked),
                    NULL);
  g_signal_connect((gpointer) StreamTransmit, "toggled",
                    G_CALLBACK(on_StreamTransmit_toggled),
                    NULL);
  g_signal_connect((gpointer) StreamRecord, "toggled",
                    G_CALLBACK(on_StreamRecord_toggled),
                    NULL);
  g_signal_connect((gpointer) RecordFileOpenButton, "clicked",
                    G_CALLBACK(on_FileOpenButton_clicked),
                    GINT_TO_POINTER(0));
  g_signal_connect((gpointer) SDPFileOpenButton, "clicked",
                    G_CALLBACK(on_FileOpenButton_clicked),
                    GINT_TO_POINTER(1));
  g_signal_connect((gpointer) VideoEnabled, "toggled",
                    G_CALLBACK(on_VideoEnabled_toggled),
                    NULL);
  g_signal_connect((gpointer) VideoProfile, "changed",
                    G_CALLBACK(on_VideoProfile_changed),
                    NULL);
  g_signal_connect((gpointer) StreamVideoPreview, "toggled",
                            G_CALLBACK(on_VideoPreview),
                            GINT_TO_POINTER(1));
  g_signal_connect((gpointer) VideoTxAddrButton, "clicked",
                    G_CALLBACK(on_VideoTxAddrButton_clicked),
                    NULL);
  g_signal_connect((gpointer) AudioEnabled, "toggled",
                    G_CALLBACK(on_AudioEnabled_toggled),
                    NULL);
  g_signal_connect((gpointer) AudioProfile, "changed",
                    G_CALLBACK(on_AudioProfile_changed),
                    NULL);
  g_signal_connect((gpointer) AudioTxAddrButton, "clicked",
                    G_CALLBACK(on_AudioTxAddrButton_clicked),
                    NULL);
#ifdef HAVE_TEXT
  g_signal_connect((gpointer) TextEnabled, "toggled",
                    G_CALLBACK(on_TextEnabled_toggled),
                    NULL);
  g_signal_connect((gpointer) TextProfile, "changed",
                    G_CALLBACK(on_TextProfile_changed),
                    NULL);
  g_signal_connect((gpointer) TextTxAddrButton, "clicked",
                    G_CALLBACK(on_TextTxAddrButton_clicked),
                    NULL);
#ifdef HAVE_TEXT_ENTRY
  g_signal_connect((gpointer) TextEntrySend, "clicked",
                    G_CALLBACK(on_TextEntrySend_clicked),
                    NULL);
#endif
#endif
  g_signal_connect((gpointer)DurationType, "changed", 
		   G_CALLBACK(on_durationtype_activate), NULL);
  g_signal_connect((gpointer) StartButton, "toggled",
                    G_CALLBACK(on_StartButton_toggled),
                    NULL);

  GtkTreeSelection *select;
  select = gtk_tree_view_get_selection(GTK_TREE_VIEW(StreamTreeView));
  gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(select), "changed",
		   G_CALLBACK(on_StreamTree_select), NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(MainWindow, MainWindow, "MainWindow");
  GLADE_HOOKUP_OBJECT(MainWindow, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT(MainWindow, menubar1, "menubar1");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem1, "menuitem1");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem1_menu, "menuitem1_menu");
  GLADE_HOOKUP_OBJECT(MainWindow, new1, "new1");
  GLADE_HOOKUP_OBJECT(MainWindow, open1, "open1");
  GLADE_HOOKUP_OBJECT(MainWindow, save1, "save1");
  GLADE_HOOKUP_OBJECT(MainWindow, separatormenuitem1, "separatormenuitem1");
  GLADE_HOOKUP_OBJECT(MainWindow, quit1, "quit1");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem2, "menuitem2");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem2_menu, "menuitem2_menu");
  GLADE_HOOKUP_OBJECT(MainWindow, generate_addresses, "generate_addresses");
  GLADE_HOOKUP_OBJECT(MainWindow, generate_sdp, "generate_sdp");
  GLADE_HOOKUP_OBJECT(MainWindow, separator1, "separator1");
  GLADE_HOOKUP_OBJECT(MainWindow, preferences1, "preferences1");
  GLADE_HOOKUP_OBJECT(MainWindow, restart_recording, "restart_recording");
  GLADE_HOOKUP_OBJECT(MainWindow, separator3, "separator3");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem4, "menuitem4");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem4_menu, "menuitem4_menu");
  GLADE_HOOKUP_OBJECT(MainWindow, about1, "about1");

  GLADE_HOOKUP_OBJECT(MainWindow, InputFrame, "InputFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox97, "hbox97");
  GLADE_HOOKUP_OBJECT(MainWindow, vbox41, "vbox41");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox100, "hbox100");
  GLADE_HOOKUP_OBJECT(MainWindow, label189, "label189");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoSourceLabel, "VideoSourceLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox101, "hbox101");
  GLADE_HOOKUP_OBJECT(MainWindow, label190, "label190");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioSourceLabel, "AudioSourceLabel");
#ifdef HAVE_TEXT
  GLADE_HOOKUP_OBJECT(MainWindow, hbox102, "hbox102");
  GLADE_HOOKUP_OBJECT(MainWindow, label192, "label192");
  GLADE_HOOKUP_OBJECT(MainWindow, TextSourceLabel, "TextSourceLabel");
#endif
  GLADE_HOOKUP_OBJECT(MainWindow, vbox40, "vbox40");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox99, "hbox99");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoSourcePreview, "VideoSourcePreview");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox98, "hbox98");
  GLADE_HOOKUP_OBJECT(MainWindow, SourceOptionMenu, "SourceOptionMenu");
  GLADE_HOOKUP_OBJECT(MainWindow, menu14, "menu14");
  GLADE_HOOKUP_OBJECT(MainWindow, Change, "Change");
  GLADE_HOOKUP_OBJECT(MainWindow, separator2, "separator2");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoSourceMenu, "VideoSourceMenu");
  GLADE_HOOKUP_OBJECT(MainWindow, image27, "image27");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioSourceMenu, "AudioSourceMenu");
  GLADE_HOOKUP_OBJECT(MainWindow, image28, "image28");
#ifdef HAVE_TEXT
  GLADE_HOOKUP_OBJECT(MainWindow, TextSourceMenu, "TextSourceMenu");
  GLADE_HOOKUP_OBJECT(MainWindow, image29, "image29");
#endif

  GLADE_HOOKUP_OBJECT(MainWindow, label184, "label184");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamInfoFrame, "StreamInfoFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, hpaned1, "hpaned1");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamVbox, "StreamVbox");
  GLADE_HOOKUP_OBJECT(MainWindow, stream_label, "stream_label");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamScrolledWindow, "StreamScrolledWindow");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamTreeView, "StreamTreeView");
  GLADE_HOOKUP_OBJECT(MainWindow, hbuttonbox1, "hbuttonbox1");
  GLADE_HOOKUP_OBJECT(MainWindow, AddStreamButton, "AddStreamButton");
  GLADE_HOOKUP_OBJECT(MainWindow, image1, "image1");
  GLADE_HOOKUP_OBJECT(MainWindow, DeleteStreamButton, "DeleteStreamButton");
  GLADE_HOOKUP_OBJECT(MainWindow, image2, "image2");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamInfoVbox, "StreamInfoVbox");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamVideoInfo, "StreamVideoInfo");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamAudioInfo, "StreamAudioInfo");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox34, "hbox34");
  GLADE_HOOKUP_OBJECT(MainWindow, label47, "label47");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamFps, "StreamFps");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox33, "hbox33");
  GLADE_HOOKUP_OBJECT(MainWindow, label45, "label45");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamRecording, "StreamRecording");
  GLADE_HOOKUP_OBJECT(MainWindow, InfoVbox, "InfoVbox");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamFrame, "StreamFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamFrameVbox, "StreamFrameVbox");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamNameHbox, "StreamNameHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, label3, "label3");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamNameLabel, "StreamNameLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamCaptionHbox, "StreamCaptionHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, label32, "label32");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamCaption, "StreamCaption");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamDescHbox, "StreamDescHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, label31, "label31");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamDescription, "StreamDescription");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamTransmitHbox, "StreamTransmitHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamTransmit, "StreamTransmit");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamSdpFile, "StreamSdpFile");
  // GLADE_HOOKUP_OBJECT(MainWindow, label30, "label30");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamSdpFileEntry, "StreamSdpFileEntry");
  GLADE_HOOKUP_OBJECT(MainWindow, SDPFileOpenButton, "SDPFileOpenButton");
  GLADE_HOOKUP_OBJECT(MainWindow, alignment22, "alignment22");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox81, "hbox81");
  GLADE_HOOKUP_OBJECT(MainWindow, image22, "image22");
  GLADE_HOOKUP_OBJECT(MainWindow, label167, "label167");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamRecordHbox, "StreamRecordHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamRecord, "StreamRecord");
  GLADE_HOOKUP_OBJECT(MainWindow, RecordFileHbox, "RecordFileHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, label4, "label4");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamRecordFileEntry, "StreamRecordFileEntry");
  GLADE_HOOKUP_OBJECT(MainWindow, RecordFileOpenButton, "RecordFileOpenButton");
  GLADE_HOOKUP_OBJECT(MainWindow, alignment23, "alignment23");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox82, "hbox82");
  GLADE_HOOKUP_OBJECT(MainWindow, image23, "image23");
  GLADE_HOOKUP_OBJECT(MainWindow, label168, "label168");
  GLADE_HOOKUP_OBJECT(MainWindow, label2, "label2");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoFrame, "VideoFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoFrameVbox, "VideoFrameVbox");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoFrameHbox1, "VideoFrameHbox1");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoEnabled, "VideoEnabled");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox7, "hbox7");
  GLADE_HOOKUP_OBJECT(MainWindow, label6, "label6");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoProfile, "VideoProfile");
  GLADE_HOOKUP_OBJECT(MainWindow, menu5, "menu5");
  GLADE_HOOKUP_OBJECT(MainWindow, StreamVideoPreview, "StreamVideoPreview");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoTxTable, "VideoTxTable");
  GLADE_HOOKUP_OBJECT(MainWindow, label124, "label124");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoTxAddrLabel, "VideoTxAddrLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, VideoTxAddrButton, "VideoTxAddrButton");
  GLADE_HOOKUP_OBJECT(MainWindow, label5, "label5");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioFrame, "AudioFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioFrameVbox, "AudioFrameVbox");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioFrameLine1, "AudioFrameLine1");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioEnabled, "AudioEnabled");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox15, "hbox15");
  GLADE_HOOKUP_OBJECT(MainWindow, label12, "label12");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioProfile, "AudioProfile");
  GLADE_HOOKUP_OBJECT(MainWindow, menu6, "menu6");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioTxTable, "AudioTxTable");
  GLADE_HOOKUP_OBJECT(MainWindow, label126, "label126");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioTxAddrLabel, "AudioTxAddrLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, AudioTxAddrButton, "AudioTxAddrButton");
  GLADE_HOOKUP_OBJECT(MainWindow, label9, "label9");
#ifdef HAVE_TEXT
  GLADE_HOOKUP_OBJECT(MainWindow, StreamTextInfo, "StreamTextInfo");
  GLADE_HOOKUP_OBJECT(MainWindow, TextSourceLabel, "TextSourceLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, TextFrame, "TextFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, TextFrameVbox, "TextFrameVbox");
  GLADE_HOOKUP_OBJECT(MainWindow, TextFrameHbox1, "TextFrameHbox1");
  GLADE_HOOKUP_OBJECT(MainWindow, TextEnabled, "TextEnabled");
  GLADE_HOOKUP_OBJECT(MainWindow, hbox17, "hbox17");
  GLADE_HOOKUP_OBJECT(MainWindow, label13, "label13");
  GLADE_HOOKUP_OBJECT(MainWindow, TextProfile, "TextProfile");
  GLADE_HOOKUP_OBJECT(MainWindow, menu7, "menu7");
  GLADE_HOOKUP_OBJECT(MainWindow, TextTxTable, "TextTxTable");
  GLADE_HOOKUP_OBJECT(MainWindow, label128, "label128");
  GLADE_HOOKUP_OBJECT(MainWindow, TextTxAddrLabel, "TextTxAddrLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, TextTxAddrButton, "TextTxAddrButton");
  GLADE_HOOKUP_OBJECT(MainWindow, label16, "label16");
#ifdef HAVE_TEXT_ENTRY
  GLADE_HOOKUP_OBJECT(MainWindow, TextEntryFrame, "TextEntryFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, TextEntryHbox, "TextEntryHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, label51, "label51");
  GLADE_HOOKUP_OBJECT(MainWindow, TextEntry, "TextEntry");
  GLADE_HOOKUP_OBJECT(MainWindow, TextEntrySend, "TextEntrySend");
  GLADE_HOOKUP_OBJECT(MainWindow, label52, "label52");
#endif
#endif
  GLADE_HOOKUP_OBJECT(MainWindow, label114, "label114");
  GLADE_HOOKUP_OBJECT(MainWindow, StatusFrame, "StatusFrame");
  GLADE_HOOKUP_OBJECT(MainWindow, StatusHbox, "StatusHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, DurationHbox, "DurationHbox");
  GLADE_HOOKUP_OBJECT(MainWindow, DurationLabel, "DurationLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, Duration, "Duration");
  GLADE_HOOKUP_OBJECT(MainWindow, vbox37, "vbox37");
  GLADE_HOOKUP_OBJECT(MainWindow, DurationType, "DurationType");
  GLADE_HOOKUP_OBJECT(MainWindow, DurationMenu, "DurationMenu");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem23, "menuitem23");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem24, "menuitem24");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem25, "menuitem25");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem26, "menuitem26");
  GLADE_HOOKUP_OBJECT(MainWindow, menuitem27, "menuitem27");
  GLADE_HOOKUP_OBJECT(MainWindow, vseparator1, "vseparator1");
  GLADE_HOOKUP_OBJECT(MainWindow, TimeTable, "TimeTable");
  GLADE_HOOKUP_OBJECT(MainWindow, label116, "label116");
  GLADE_HOOKUP_OBJECT(MainWindow, label117, "label117");
  GLADE_HOOKUP_OBJECT(MainWindow, label118, "label118");
  GLADE_HOOKUP_OBJECT(MainWindow, label119, "label119");
  GLADE_HOOKUP_OBJECT(MainWindow, CurrentDurationLabel, "CurrentDurationLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, CurrentTimeLabel, "CurrentTimeLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, EndTimeLabel, "EndTimeLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, StartTimeSuffix, "StartTimeSuffix");
  GLADE_HOOKUP_OBJECT(MainWindow, StartTimeLabel, "StartTimeLabel");
  GLADE_HOOKUP_OBJECT(MainWindow, label183, "label183");
  GLADE_HOOKUP_OBJECT(MainWindow, CurrentTimeSuffix, "CurrentTimeSuffix");
  GLADE_HOOKUP_OBJECT(MainWindow, EndTimeSuffix, "EndTimeSuffix");
  GLADE_HOOKUP_OBJECT(MainWindow, vseparator2, "vseparator2");
  GLADE_HOOKUP_OBJECT(MainWindow, vbox34, "vbox34");
  GLADE_HOOKUP_OBJECT(MainWindow, StartButton, "StartButton");
  GLADE_HOOKUP_OBJECT(MainWindow, label164, "label164");
  GLADE_HOOKUP_OBJECT(MainWindow, statusbar1, "statusbar1");
  GLADE_HOOKUP_OBJECT_NO_REF(MainWindow, tooltips, "tooltips");

  gtk_widget_grab_focus(StartButton);
  gtk_window_add_accel_group(GTK_WINDOW(MainWindow), accel_group);

  return MainWindow;
}

void StartFlowLoadWindow (void)
{
  if (AVFlow != NULL) {
    delete AVFlow;
    AVFlow = NULL;
  }
  AVFlow = new CPreviewAVMediaFlow(MyConfig);
  // main window created - fill in the settings

  GtkWidget *wid;
  wid = lookup_widget(MainWindow, "Duration");
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(wid), 
			    MyConfig->GetIntegerValue(CONFIG_APP_DURATION));

  uint32_t dur = MyConfig->GetIntegerValue(CONFIG_APP_DURATION_UNITS);
  for(uint ix = 0; ix < NUM_ELEMENTS_IN_ARRAY(durationUnitsValues); ix++) {
    if(dur == durationUnitsValues[ix]) {
      wid = lookup_widget(MainWindow, "DurationType");
      gtk_option_menu_set_history(GTK_OPTION_MENU(wid), 
				  ix);
    }
  }
  MainWindowDisplaySources();
  DisplayAudioProfiles();
  DisplayVideoProfiles();
#ifdef HAVE_TEXT
  DisplayTextProfiles();
#endif
  DisplayStreamsInView();
  DisplayStreamData(NULL);
  // "press" start button
  if (MyConfig->m_appAutomatic) {
    DoStart();
  }

  AVFlow->StartVideoPreview();
}

int gui_main(int argc, char **argv, CLiveConfig* pConfig)
{
  MyConfig = pConfig;

#if 0
  argv =(char **)malloc(3 * sizeof(char *));
  argv[0] = "mp4live";
  argv[1] ="--sync";
  argv[2] = NULL;
  argc = 2;
#endif
#ifdef ENABLE_NLS
  bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
#endif

  gtk_init(&argc, &argv);

  //  add_pixmap_directory(PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");

  MainWindow = create_MainWindow();

  char buffer[80];
  snprintf(buffer, sizeof(buffer), "cisco %s %s %s", argv[0], MPEG4IP_VERSION,
	   get_linux_video_type());
  gtk_window_set_title(GTK_WINDOW(MainWindow), buffer);
  gtk_widget_show(MainWindow);

  StartFlowLoadWindow();

  gtk_main();
  return 0;
}

/* end gui_main.cpp */
