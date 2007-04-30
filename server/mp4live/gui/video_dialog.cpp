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

#define __STDC_LIMIT_MACROS
#include "mp4live.h"
#include "mp4live_gui.h"
#include "video_v4l_source.h"
#include "support.h"
// source defines

GtkWidget *VideoSourceDialog;
static const char** inputNames = NULL;
static u_int8_t inputNumber = 0;	// how many inputs total

static CVideoCapabilities* pVideoCaps;
static bool source_modified;
static const char* source_type;

// profile defines
static GtkWidget *VideoProfileDialog = NULL;
static const char **encoderNames = NULL;
static u_int16_t* sizeWidthValues;
static u_int16_t  sizeWidthValueCnt;
static u_int16_t* sizeHeightValues;
static u_int16_t  sizeHeightValueCnt;
static u_int8_t sizeIndex;
static u_int8_t sizeMaxIndex;

static float aspectValues[] = {
	VIDEO_STD_ASPECT_RATIO, VIDEO_LB1_ASPECT_RATIO, 
	VIDEO_LB2_ASPECT_RATIO, VIDEO_LB3_ASPECT_RATIO
}; 
static const char* aspectNames[] = {
	"Standard 4:3", "Letterbox 2.35", "Letterbox 1.85", "HDTV 16:9"
};
static const char *profilefilterNames[] = {
  VIDEO_FILTER_NONE, VIDEO_FILTER_DEINTERLACE,
#ifdef HAVE_FFMPEG
  VIDEO_FILTER_FFMPEG_DEINTERLACE_INPLACE,
#endif
};

static bool ValidateAndSave(void)
{
  GtkWidget *wid;

  // if previewing, stop video source
  //AVFlow->StopVideoPreview();

  wid = lookup_widget(VideoSourceDialog, "VideoSourceFilter");
  
  
  MyConfig->SetStringValue(CONFIG_VIDEO_FILTER, 
			   gtk_option_menu_get_history(GTK_OPTION_MENU(wid)) == 0 ?
			   VIDEO_FILTER_NONE : VIDEO_FILTER_DECIMATE);
  // copy new values to config
  wid = lookup_widget(VideoSourceDialog, "VideoSourceFile");
  const char *source_name = gtk_entry_get_text(GTK_ENTRY(wid));
  if (strcmp(source_name, 
	     MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME)) != 0) {
    MyConfig->SetStringValue(CONFIG_VIDEO_SOURCE_TYPE,
			     source_type);
    
    MyConfig->SetStringValue(CONFIG_VIDEO_SOURCE_NAME,
			     gtk_entry_get_text(GTK_ENTRY(wid)));

#if 0
    MyConfig->UpdateFileHistory(
				gtk_entry_get_text(GTK_ENTRY(source_entry)));
#endif

    if (MyConfig->m_videoCapabilities != pVideoCaps) {
      // don't do this any more - save old delete MyConfig->m_videoCapabilities;
      MyConfig->m_videoCapabilities = pVideoCaps;
      pVideoCaps = NULL;
    }
  
#if 0
    if (strcasecmp(source_type, VIDEO_SOURCE_V4L) 
	&& default_file_audio_source >= 0) {
		MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_TYPE,
			source_type);
		MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_NAME,
			gtk_entry_get_text(GTK_ENTRY(source_entry)));
		MyConfig->SetIntegerValue(CONFIG_AUDIO_SOURCE_TRACK,
			default_file_audio_source);
	}
#endif
  }

    wid = lookup_widget(VideoSourceDialog, "VideoSourcePort");
    MyConfig->SetIntegerValue(CONFIG_VIDEO_INPUT,
			      gtk_option_menu_get_history(GTK_OPTION_MENU(wid)));

    wid = lookup_widget(VideoSourceDialog, "VideoSourceSignal");
    MyConfig->SetIntegerValue(CONFIG_VIDEO_SIGNAL,
			      gtk_option_menu_get_history(GTK_OPTION_MENU(wid)));
    wid = lookup_widget(VideoSourceDialog, "VideoSourceChannelType");
    uint channelListIndex = 
      gtk_option_menu_get_history(GTK_OPTION_MENU(wid));
    MyConfig->SetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX,
			      channelListIndex);
    // extract channel index out of combo (not so simple)
    GtkWidget* combo = lookup_widget(VideoSourceDialog, "VideoSourceChannel");
    GtkWidget* entry = GTK_COMBO(combo)->entry;
    const char* channelName = gtk_entry_get_text(GTK_ENTRY(entry));
    uint channelIndex = MyConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX);
    struct CHANLISTS* pChannelList = chanlists;
    for (uint i = 0; i < pChannelList[channelListIndex].count; i++) {
      if (!strcmp(channelName, 
		  pChannelList[channelListIndex].list[i].name)) {
	channelIndex = i;
	break;
      }
    }
    MyConfig->SetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX,
			      channelIndex);

#if 0
    MyConfig->SetIntegerValue(CONFIG_VIDEO_SOURCE_TRACK,
			      trackValues ? trackValues[trackIndex] : 0);
#endif


    MyConfig->Update();

    // restart video source
    if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
      //AVFlow->StartVideoPreview();
      AVFlow->RestartVideo();
    }

    MainWindowDisplaySources();
    return true;
}

// forward declarations
static void SetAvailableSignals(void)
{
  u_int8_t ix = 0;
  u_int8_t validSignalIndex = 0xFF;
  uint inputIndex;
  uint signalIndex;
  GtkWidget *temp, *signal_menu;
  temp = lookup_widget(VideoSourceDialog, "VideoSourcePort");
  inputIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));
  signal_menu = lookup_widget(VideoSourceDialog, "VideoSourceSignal");
  signalIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(signal_menu));

  
  for (ix = VIDEO_SIGNAL_PAL; ix < VIDEO_SIGNAL_MAX; ix++) {
    // all signals are enabled
    bool enabled = true;
    
    // unless input is a tuner
    if (pVideoCaps != NULL
	&& pVideoCaps->m_inputHasTuners[inputIndex]) {
      
      // this signal type is valid for this tuner
      debug_message("Available signals for tuners %x", 
		    pVideoCaps->m_inputTunerSignalTypes[inputIndex]);

      if (pVideoCaps->m_inputTunerSignalTypes[inputIndex] & (1 << ix)) {
	
	// remember first valid signal type for this tuner
	if (validSignalIndex == 0xFF) {
	  validSignalIndex = ix;
	}
	
      } else { // this signal type is invalid for this tuner
	
	// so disable this menu item
	enabled = false;
	
	// check if our current signal type is invalid for this tuner
	if (signalIndex == ix) {
	  signalIndex = 0xFF;
	}
      }
    }
    
    static const char *signalwidgets[] = {
      "signalpal", 
      "signalntsc", 
      "signalsecam",
    };

    temp = lookup_widget(VideoSourceDialog, signalwidgets[ix]);
    gtk_widget_set_sensitive(temp, enabled);
  }
  
  // try to choose a valid signal type if we don't have one
  if (signalIndex == 0xFF) {
    if (validSignalIndex == 0xFF) {
      // we're in trouble
      debug_message("SetAvailableSignals: no valid signal type!");
      signalIndex = 0;
    } else {
      signalIndex = validSignalIndex;
    }
    
    // cause the displayed value to be updated
    gtk_option_menu_set_history(GTK_OPTION_MENU(signal_menu), signalIndex);
    // TBD check that signal activate is called here
  }
}

static bool SourceIsDevice()
{
  GtkWidget *temp;
  temp = lookup_widget(VideoSourceDialog, "VideoSourceFile");
  return IsDevice(gtk_entry_get_text(GTK_ENTRY(temp)));
}

static void EnableChannels()
{
	bool hasTuner = false;

	uint inputIndex;
	GtkWidget *temp;
	temp = lookup_widget(VideoSourceDialog, "VideoSourcePort");
	inputIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));

	if (pVideoCaps && pVideoCaps->m_inputHasTuners[inputIndex]) {
		hasTuner = true;
	}
	
	temp = lookup_widget(VideoSourceDialog, 
			     "VideoSourceChannelType");
	gtk_widget_set_sensitive(temp, hasTuner);
	temp = lookup_widget(VideoSourceDialog,
			     "VideoSourceChannel");
	gtk_widget_set_sensitive(temp, hasTuner);
}

static void ChangeInput (void)
{
	SetAvailableSignals();
	EnableChannels();
}

static void on_input_menu_activate(GtkOptionMenu *menu, gpointer data)
{
  ChangeInput();
}

void CreateInputMenu(CVideoCapabilities* pNewVideoCaps,
		     uint inputIndex)
{
  u_int8_t newInputNumber;
  if (pNewVideoCaps) {
    newInputNumber = pNewVideoCaps->m_numInputs;
  } else {
    newInputNumber = 0;
  }
  
  if (inputIndex >= newInputNumber) {
    inputIndex = 0; 
  }

  // create new menu item names
  const char** newInputNames = (const char**)malloc(sizeof(char*) * newInputNumber);

  for (u_int8_t i = 0; i < newInputNumber; i++) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%u - %s",
	     i, pNewVideoCaps->m_inputNames[i]);
    newInputNames[i] = strdup(buf);
  }

  // (re)create the menu
  GtkWidget *temp = lookup_widget(VideoSourceDialog, "VideoSourcePort");
  CreateOptionMenu(
		   temp,
		   newInputNames, 
		   newInputNumber,
		   inputIndex);

  // free up old names
  for (u_int8_t i = 0; i < inputNumber; i++) {
    free((void *)inputNames[i]);
  }
  free(inputNames);
  inputNames = newInputNames;
  inputNumber = newInputNumber;
}

static void SourceV4LDevice()
{
  GtkWidget *temp;
  temp = lookup_widget(VideoSourceDialog, "VideoSourceFile");
  const char *newSourceName =
    gtk_entry_get_text(GTK_ENTRY(temp));

  // don't probe the already open device!
  if (!strcmp(newSourceName, 
	      MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME))) {
    return;
  }

  // probe new source device
  CVideoCapabilities* pNewVideoCaps;
  pNewVideoCaps = FindVideoCapabilitiesByDevice(newSourceName);
  if (pNewVideoCaps == NULL) {
    pNewVideoCaps = new CVideoCapabilities(newSourceName);
    if (pNewVideoCaps->IsValid()) {
      MyConfig->m_videoCapabilities = pNewVideoCaps;
      pNewVideoCaps->SetNext(VideoCapabilities);
    }
  }
	
  // check for errors
  if (!pNewVideoCaps->IsValid()) {
    if (!pNewVideoCaps->m_canOpen) {
      ShowMessage("Change Video Source",
		  "Specified video source can't be opened, check name");
    } else {
      ShowMessage("Change Video Source",
		  "Specified video source doesn't support capture");
    }
    gtk_entry_set_text(GTK_ENTRY(temp), 
		       MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME));
    delete pNewVideoCaps;
    return;
  }

  // change inputs menu
  temp = lookup_widget(VideoSourceDialog, "VideoSourcePort");
  
  CreateInputMenu(pNewVideoCaps, 
		  gtk_option_menu_get_history(GTK_OPTION_MENU(temp)));

  if (pVideoCaps != MyConfig->m_videoCapabilities) {
    delete pVideoCaps;
  }
  pVideoCaps = pNewVideoCaps;
  
  ChangeInput();
}
#if 0
static void on_yes_default_file_audio_source (GtkWidget *widget, gpointer *data)
{
	default_file_audio_source = 
		FileDefaultAudio(gtk_entry_get_text(GTK_ENTRY(source_entry)));

	default_file_audio_dialog = false;
}

static void on_no_default_file_audio_source (GtkWidget *widget, gpointer *data)
{
	default_file_audio_dialog = false;
}
#endif

static void ChangeSource()
{
  GtkWidget *wid = lookup_widget(VideoSourceDialog, "VideoSourceFile");

  const char* new_source_name =
    gtk_entry_get_text(GTK_ENTRY(wid));

  if (strcmp(new_source_name, 
	     MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME)) == 0) {
    return;
  }

      //	default_file_audio_source = -1;

      // decide what type of source we have
      if (SourceIsDevice()) {
	source_type = VIDEO_SOURCE_V4L;
	
	SourceV4LDevice();
      } else {
	if (pVideoCaps != MyConfig->m_videoCapabilities) {
	  // don't delete these anymoredelete pVideoCaps;
	}
	pVideoCaps = NULL;
	
	if (IsUrl(new_source_name)) {
	  source_type = URL_SOURCE;
	} else {
	  if (access(new_source_name, R_OK) != 0) {
	    ShowMessage("Change Video Source",
			"Specified video source can't be opened, check name");
	  }
	  source_type = FILE_SOURCE;
	}
	
#if 0
	if (!default_file_audio_dialog
	    && FileDefaultAudio(source_name) >= 0) {
	  YesOrNo(
		  "Change Video Source",
		  "Do you want to use this for the audio source also?",
		  true,
		  GTK_SIGNAL_FUNC(on_yes_default_file_audio_source),
		  GTK_SIGNAL_FUNC(on_no_default_file_audio_source));
	}
#endif
      }

}
static void on_source_entry_changed (GtkWidget *widget, gpointer *data)
{
  source_modified = true;
}

static int on_source_leave (GtkWidget *widget, gpointer *data)
{
	if (source_modified) {
		ChangeSource();
	}
	return FALSE;
}

static void on_source_browse_button (GtkWidget *widget, gpointer *data)
{
  GtkWidget *wid = lookup_widget(VideoSourceDialog, "VideoSourceFile");
  FileBrowser(wid, VideoSourceDialog, GTK_SIGNAL_FUNC(ChangeSource));
}


static void CreateChannelCombo(uint list_index, uint chan_index)
{
  GList *VideoSourceChannel_items = NULL;

  if (chanlists[list_index].count <= chan_index) {
    chan_index = 0;
  }
  for (uint ix = 0; ix < chanlists[list_index].count; ix++) {
    VideoSourceChannel_items = 
      g_list_append(VideoSourceChannel_items, 
		    (gpointer)chanlists[list_index].list[ix].name);
  }

  debug_message("channel list has %d", chanlists[list_index].count);
  GtkWidget *channel_combo = lookup_widget(VideoSourceDialog,
					   "VideoSourceChannel");
  gtk_combo_set_popdown_strings(GTK_COMBO(channel_combo), 
				VideoSourceChannel_items);
  //gtk_combo_set_use_arrows_always(GTK_COMBO(channel_combo), 1);
  g_list_free(VideoSourceChannel_items);

  GtkWidget *entry = GTK_COMBO(channel_combo)->entry;
  gtk_entry_set_text(GTK_ENTRY(entry), 
		     chanlists[list_index].list[chan_index].name);
  gtk_widget_show(channel_combo);
}

void ChangeChannelList(u_int8_t newIndex)
{
  //  channelListIndex = newIndex;

	// recreate channel menu
  CreateChannelCombo(newIndex, 
		     MyConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX));
}

static void on_channel_list_menu_activate(GtkWidget *widget, gpointer data)
{
  uint channel_list_index = 
    gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
  ChangeChannelList(channel_list_index);
}

static const char* GetChannelListName(uint32_t index, void* pUserData)
{
	return ((struct CHANLISTS*)pUserData)[index].name;
}

static void CreateChannelListMenu (void)
{
  struct CHANLISTS* pChannelList = chanlists;
  uint index, on_index = 0;
  on_index = MyConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX);
  
  for (index = 0; chanlists[index].name != NULL; index++) {
  }
  GtkWidget *channel_list_menu = lookup_widget(VideoSourceDialog, 
					       "VideoSourceChannelType");
  CreateOptionMenu(channel_list_menu,
		   GetChannelListName,
		   (void*)pChannelList,
		   index,
		   on_index);
}

static void
on_VideoSourceDialog_response          (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    if (ValidateAndSave() == false) 
      return;
  }

  gtk_widget_destroy(GTK_WIDGET(dialog));
}

void create_VideoSourceDialog (void)
{
  GtkWidget *dialog_vbox7;
  GtkWidget *VideoSourceTable;
  GtkWidget *label150;
  GtkWidget *label151;
  GtkWidget *label152;
  GtkWidget *label153;
  GtkWidget *label154;
  GtkWidget *label155;
  GtkWidget *hbox75;
  GtkWidget *VideoSourceFile;
  GtkWidget *VideoSourceBrowse;
  GtkWidget *alignment20;
  GtkWidget *hbox76;
  GtkWidget *image20;
  GtkWidget *label156;
  GtkWidget *VideoSourcePort;
  GtkWidget *VideoSourceSignal;
  GtkWidget *menu15;
  GtkWidget *signalpal;
  GtkWidget *signalntsc;
  GtkWidget *signalsecam;
  GtkWidget *VideoSourceChannelType;
  GtkWidget *VideoSourceFilter;
  GtkWidget *menu11;
  GtkWidget *none1;
  GtkWidget *decimate1;
  GtkWidget *VideoSourceChannel;

  GtkWidget *dialog_action_area6;
  GtkWidget *VideoSourceCancel;
  GtkWidget *VideoSourceOkButton;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new ();

  VideoSourceDialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (VideoSourceDialog), _("Video Source"));
  gtk_window_set_modal(GTK_WINDOW(VideoSourceDialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(VideoSourceDialog), 
			       GTK_WINDOW(MainWindow));

  dialog_vbox7 = GTK_DIALOG (VideoSourceDialog)->vbox;
  gtk_widget_show (dialog_vbox7);

  VideoSourceTable = gtk_table_new (6, 2, FALSE);
  gtk_widget_show (VideoSourceTable);
  gtk_box_pack_start (GTK_BOX (dialog_vbox7), VideoSourceTable, TRUE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (VideoSourceTable), 2);
  gtk_table_set_col_spacings (GTK_TABLE (VideoSourceTable), 9);

  label150 = gtk_label_new (_("Source:"));
  gtk_widget_show (label150);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), label150, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label150), 0, 0.5);

  label151 = gtk_label_new (_("Port:"));
  gtk_widget_show (label151);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), label151, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label151), 0, 0.5);

  label152 = gtk_label_new (_("Signal:"));
  gtk_widget_show (label152);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), label152, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label152), 0, 0.5);

  label153 = gtk_label_new (_("Channel List:"));
  gtk_widget_show (label153);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), label153, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label153), 0, 0.5);

  label154 = gtk_label_new (_("Channel:"));
  gtk_widget_show (label154);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), label154, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label154), 0, 0.5);

  label155 = gtk_label_new (_("Filter:"));
  gtk_widget_show (label155);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), label155, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label155), 0, 0.5);

  hbox75 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox75);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), hbox75, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  VideoSourceFile = gtk_entry_new ();
  gtk_widget_show (VideoSourceFile);
  gtk_box_pack_start (GTK_BOX (hbox75), VideoSourceFile, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, VideoSourceFile, _("Video Device"), NULL);

  VideoSourceBrowse = gtk_button_new ();
  gtk_widget_show (VideoSourceBrowse);
  gtk_box_pack_start (GTK_BOX (hbox75), VideoSourceBrowse, FALSE, FALSE, 0);

  alignment20 = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_widget_show (alignment20);
  gtk_container_add (GTK_CONTAINER (VideoSourceBrowse), alignment20);

  hbox76 = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox76);
  gtk_container_add (GTK_CONTAINER (alignment20), hbox76);

  image20 = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (image20);
  gtk_box_pack_start (GTK_BOX (hbox76), image20, FALSE, FALSE, 0);

  label156 = gtk_label_new_with_mnemonic (_("Browse"));
  gtk_widget_show (label156);
  gtk_box_pack_start (GTK_BOX (hbox76), label156, FALSE, FALSE, 0);

  VideoSourcePort = gtk_option_menu_new ();
  gtk_widget_show (VideoSourcePort);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), VideoSourcePort, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, VideoSourcePort, _("Video Input to use"), NULL);

  VideoSourceSignal = gtk_option_menu_new ();
  gtk_widget_show (VideoSourceSignal);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), VideoSourceSignal, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, VideoSourceSignal, _("Signal Type"), NULL);

  menu15 = gtk_menu_new ();

  signalpal = gtk_menu_item_new_with_mnemonic (_("PAL"));
  gtk_widget_show (signalpal);
  gtk_container_add (GTK_CONTAINER (menu15), signalpal);

  signalntsc = gtk_menu_item_new_with_mnemonic (_("NTSC"));
  gtk_widget_show (signalntsc);
  gtk_container_add (GTK_CONTAINER (menu15), signalntsc);

  signalsecam = gtk_menu_item_new_with_mnemonic (_("SECAM"));
  gtk_widget_show (signalsecam);
  gtk_container_add (GTK_CONTAINER (menu15), signalsecam);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (VideoSourceSignal), menu15);

  VideoSourceChannelType = gtk_option_menu_new ();
  gtk_widget_show (VideoSourceChannelType);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), VideoSourceChannelType, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_sensitive (VideoSourceChannelType, FALSE);
  gtk_tooltips_set_tip (tooltips, VideoSourceChannelType, _("Channel Type for TV input"), NULL);

  VideoSourceChannel = gtk_combo_new();
  gtk_combo_set_value_in_list(GTK_COMBO(VideoSourceChannel), true, false);
  gtk_widget_show (VideoSourceChannel);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), VideoSourceChannel, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_set_sensitive (VideoSourceChannel, FALSE);
  gtk_tooltips_set_tip (tooltips, VideoSourceChannel, _("Channel number to tune"), NULL);

  VideoSourceFilter = gtk_option_menu_new ();
  gtk_widget_show (VideoSourceFilter);
  gtk_table_attach (GTK_TABLE (VideoSourceTable), VideoSourceFilter, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, VideoSourceFilter, _("Video Input Filter"), NULL);

  menu11 = gtk_menu_new ();

  none1 = gtk_menu_item_new_with_mnemonic (_("None"));
  gtk_widget_show (none1);
  gtk_container_add (GTK_CONTAINER (menu11), none1);

  decimate1 = gtk_menu_item_new_with_mnemonic (_("Decimate"));
  gtk_widget_show (decimate1);
  gtk_container_add (GTK_CONTAINER (menu11), decimate1);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (VideoSourceFilter), menu11);

  dialog_action_area6 = GTK_DIALOG (VideoSourceDialog)->action_area;
  gtk_widget_show (dialog_action_area6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area6), GTK_BUTTONBOX_END);

  VideoSourceCancel = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_show (VideoSourceCancel);
  gtk_dialog_add_action_widget (GTK_DIALOG (VideoSourceDialog), VideoSourceCancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (VideoSourceCancel, GTK_CAN_DEFAULT);

  VideoSourceOkButton = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_show (VideoSourceOkButton);
  gtk_dialog_add_action_widget (GTK_DIALOG (VideoSourceDialog), VideoSourceOkButton, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (VideoSourceOkButton, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) VideoSourceDialog, "response",
                    G_CALLBACK (on_VideoSourceDialog_response),
                    NULL);
  g_signal_connect((gpointer)VideoSourcePort, "changed",
		   G_CALLBACK(on_input_menu_activate),
		   NULL);

  SetEntryValidator(GTK_OBJECT(VideoSourceFile),
		    GTK_SIGNAL_FUNC(on_source_entry_changed), 
		    GTK_SIGNAL_FUNC(on_source_leave));
  g_signal_connect((gpointer)VideoSourceBrowse, "clicked",
		   G_CALLBACK(on_source_browse_button),
		   NULL);
  g_signal_connect((gpointer) VideoSourceChannelType, "changed",
		   G_CALLBACK(on_channel_list_menu_activate),
		   NULL);
		   
  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (VideoSourceDialog, VideoSourceDialog, "VideoSourceDialog");
  GLADE_HOOKUP_OBJECT_NO_REF (VideoSourceDialog, dialog_vbox7, "dialog_vbox7");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceTable, "VideoSourceTable");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, label150, "label150");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, label151, "label151");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, label152, "label152");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, label153, "label153");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, label154, "label154");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, label155, "label155");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, hbox75, "hbox75");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceFile, "VideoSourceFile");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceBrowse, "VideoSourceBrowse");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, alignment20, "alignment20");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, hbox76, "hbox76");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, image20, "image20");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, label156, "label156");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourcePort, "VideoSourcePort");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceSignal, "VideoSourceSignal");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, menu15, "menu15");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, signalpal, "signalpal");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, signalntsc, "signalntsc");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, signalsecam, "signalsecam");

  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceChannelType, "VideoSourceChannelType");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceChannel, "VideoSourceChannel");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceFilter, "VideoSourceFilter");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, menu11, "menu11");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, none1, "none1");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, decimate1, "decimate1");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceChannel, "VideoSourceChannel");

  GLADE_HOOKUP_OBJECT_NO_REF (VideoSourceDialog, dialog_action_area6, "dialog_action_area6");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceCancel, "VideoSourceCancel");
  GLADE_HOOKUP_OBJECT (VideoSourceDialog, VideoSourceOkButton, "VideoSourceOkButton");
  GLADE_HOOKUP_OBJECT_NO_REF (VideoSourceDialog, tooltips, "tooltips");

  //asdf
  source_modified = false;
  gtk_entry_set_text(GTK_ENTRY(VideoSourceFile),
		     MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME));
  
  gtk_option_menu_set_history(GTK_OPTION_MENU(VideoSourceSignal), 
			      MyConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL));
  pVideoCaps = (CVideoCapabilities *)MyConfig->m_videoCapabilities;
  CreateInputMenu(pVideoCaps, MyConfig->GetIntegerValue(CONFIG_VIDEO_INPUT));
  source_type = MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_TYPE);
  CreateChannelListMenu();
  CreateChannelCombo(MyConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX),
		     MyConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX));
  gtk_option_menu_set_history(GTK_OPTION_MENU(VideoSourceFilter),
			      strcmp(MyConfig->GetStringValue(CONFIG_VIDEO_FILTER),
				     VIDEO_FILTER_NONE) == 0 ? 0 : 1);
  gtk_widget_show(VideoSourceDialog);
}

/**************************************************************************
 * Video Profile Dialog
 **************************************************************************/
static void CreateSizeMenu(uint16_t width, uint16_t height)
{
  const char **names = NULL;

  uint32_t encoderIndex;
  GtkWidget *encoderwidget = lookup_widget(VideoProfileDialog,
					   "VideoProfileEncoder");
  encoderIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(encoderwidget));
  uint signal = MyConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL);
  
  if (signal == VIDEO_SIGNAL_PAL) {
    // PAL
    names = video_encoder_table[encoderIndex].sizeNamesPAL;
    sizeWidthValues = video_encoder_table[encoderIndex].widthValuesPAL;
    sizeWidthValueCnt = video_encoder_table[encoderIndex].numSizesPAL;
    sizeHeightValues = video_encoder_table[encoderIndex].heightValuesPAL;
    sizeHeightValueCnt = sizeWidthValueCnt;
  } else if (signal == VIDEO_SIGNAL_NTSC) {
    // NTSC
    names = video_encoder_table[encoderIndex].sizeNamesNTSC;
    sizeWidthValues = video_encoder_table[encoderIndex].widthValuesNTSC;
    sizeWidthValueCnt = video_encoder_table[encoderIndex].numSizesNTSC;
    sizeHeightValues = video_encoder_table[encoderIndex].heightValuesNTSC;
    sizeHeightValueCnt = sizeWidthValueCnt;
  } else {
    // Secam
    names = video_encoder_table[encoderIndex].sizeNamesSecam;
    sizeWidthValues = video_encoder_table[encoderIndex].widthValuesSecam;
    sizeWidthValueCnt = video_encoder_table[encoderIndex].numSizesSecam;
    sizeHeightValues = video_encoder_table[encoderIndex].heightValuesSecam;
    sizeHeightValueCnt = sizeWidthValueCnt;
  }

  if (names == NULL) return;

  sizeMaxIndex = sizeHeightValueCnt;
  u_int8_t i;
  for (i = 0; i < sizeMaxIndex; i++) {
    if (sizeWidthValues[i] >= width &&
	sizeHeightValues[i] >= height) {
      sizeIndex = i;
      break;
    }
  }
  if (i == sizeMaxIndex) {
    sizeIndex = sizeMaxIndex - 1;
  }

  if (sizeIndex >= sizeMaxIndex) {
    sizeIndex = sizeMaxIndex - 1;
  }
  GtkWidget *temp = lookup_widget(VideoProfileDialog, "VideoProfileSize");
  CreateOptionMenu(temp,
		   names, 
		   sizeMaxIndex,
		   sizeIndex);
  temp = lookup_widget(VideoProfileDialog, "VideoEncoderSettingsButton");
  bool enable_settings;
  if (video_encoder_table[encoderIndex].get_gui_options == NULL) {
    enable_settings = false;
  } else {
    enable_settings = 
      (video_encoder_table[encoderIndex].get_gui_options)(NULL, NULL);
  }
  gtk_widget_set_sensitive(temp, enable_settings);
}

void
on_VideoEncoderSettingsButton          (GtkButton *button,
					gpointer user_data)
{
  CVideoProfile *profile = (CVideoProfile *)user_data;
  GtkWidget *temp;
  temp = lookup_widget(VideoProfileDialog, "VideoProfileEncoder");
  encoder_gui_options_base_t **settings_array;
  uint settings_array_count;
  uint index = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));
  
  if ((video_encoder_table[index].get_gui_options)(&settings_array, &settings_array_count) == false) {
    return;
  }
  
  CreateEncoderSettingsDialog(profile, VideoProfileDialog,
			      video_encoder_table[index].encoding_name,
			      settings_array,
			      settings_array_count);
}

static void
on_VideoProfileEncoder_changed         (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  GtkWidget *size = lookup_widget(VideoProfileDialog, "VideoProfileSize");

  sizeIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(size));
  CreateSizeMenu(sizeWidthValues[sizeIndex],
		 sizeHeightValues[sizeIndex]);
}


bool CreateNewProfile (GtkWidget *dialog,
		       CVideoProfile *profile)
{
  GtkWidget *temp;
  temp = lookup_widget(dialog, "VideoProfileName");
  const char *name = gtk_entry_get_text(GTK_ENTRY(temp));
  if (name == NULL || *name == '\0') return false;
  profile->SetConfigName(name);
  if (AVFlow->m_video_profile_list->FindProfile(name) != NULL) {
    return false;
  }
  AVFlow->m_video_profile_list->AddEntryToList(profile);
  return true;
}
  
void
on_VideoProfileDialog_response         (GtkWidget       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  CVideoProfile *profile = (CVideoProfile *)user_data;
  CVideoProfile *pass_profile = NULL;
  GtkWidget *temp;
  bool do_copy = true;
  if (response_id == GTK_RESPONSE_OK) {
    if (profile->GetName() == NULL) {
      do_copy = CreateNewProfile(dialog, profile);
      if (do_copy == false) {
	delete profile;
      } else {
	pass_profile = profile;
      }
    } else {
      debug_message("profile is %s", profile->GetName());
    }
    if (do_copy) {
      temp = lookup_widget(dialog, "VideoProfileEncoder");
      uint index = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));
      profile->SetStringValue(CFG_VIDEO_ENCODER,
			      video_encoder_table[index].encoder);
      profile->SetStringValue(CFG_VIDEO_ENCODING,
			      video_encoder_table[index].encoding);
      temp = lookup_widget(dialog, "VideoProfileSize");
      index = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));
      profile->SetIntegerValue(CFG_VIDEO_WIDTH, sizeWidthValues[index]);
      profile->SetIntegerValue(CFG_VIDEO_HEIGHT, sizeHeightValues[index]);

      temp = lookup_widget(dialog, "VideoProfileBitRate");
      profile->SetIntegerValue(CFG_VIDEO_BIT_RATE,
			       gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(temp)));
      temp = lookup_widget(dialog, "VideoProfileFrameRate");
      profile->SetFloatValue(CFG_VIDEO_FRAME_RATE,
			     gtk_spin_button_get_value_as_float(
								GTK_SPIN_BUTTON(temp)));
      // need to add filter and aspect ratio
      temp = lookup_widget(dialog, "VideoProfileAspectRatio");
      profile->SetFloatValue(CFG_VIDEO_CROP_ASPECT_RATIO,
			     aspectValues[gtk_option_menu_get_history(GTK_OPTION_MENU(temp))]);
      temp = lookup_widget(dialog, "VideoFilterMenu");
      profile->SetStringValue(CFG_VIDEO_FILTER,
			      profilefilterNames[gtk_option_menu_get_history(GTK_OPTION_MENU(temp))]);
      profile->Update();
      profile->WriteToFile();
    }
  } else {
    if (profile->GetName() == NULL) {
      delete profile;
    }
  }
  OnVideoProfileFinished(pass_profile);
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

void CreateVideoProfileDialog(CVideoProfile *profile)
{
  GtkWidget *dialog_vbox4;
  GtkWidget *VideoProfileTable;
  GtkWidget *VideoProfileEncoder;
  GtkWidget *VideoProfileSize;
  GtkWidget *VideoProfileAspectRatio;
  GtkWidget *label87;
  GtkWidget *label88;
  GtkWidget *label89;
  GtkWidget *label93;
  GtkWidget *VideoProfileName;
  GtkWidget *label92;
  GtkWidget *VideoEncoderSettingsButton;
  GtkWidget *alignment11;
  GtkWidget *hbox59;
  GtkWidget *image11;
  GtkWidget *label95;
  GtkWidget *label91;
  GtkObject *VideoProfileBitRate_adj;
  GtkWidget *VideoProfileBitRate;
  GtkWidget *label90;
  GtkObject *VideoProfileFrameRate_adj;
  GtkWidget *VideoProfileFrameRate;
  GtkWidget *label166;
  GtkWidget *VideoFilterMenu;
  GtkWidget *dialog_action_area3;
  GtkWidget *cancelbutton3;
  GtkWidget *okbutton3;
  GtkTooltips *tooltips;
  bool new_profile = profile == NULL;

  if (new_profile) {
    profile = new CVideoProfile(NULL, NULL);
    profile->LoadConfigVariables();
    profile->Initialize(false);
  }

  tooltips = gtk_tooltips_new();

  VideoProfileDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(VideoProfileDialog), _("Video Profile"));
  gtk_window_set_modal(GTK_WINDOW(VideoProfileDialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(VideoProfileDialog), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(VideoProfileDialog), 
			       GTK_WINDOW(MainWindow));

  dialog_vbox4 = GTK_DIALOG(VideoProfileDialog)->vbox;
  gtk_widget_show(dialog_vbox4);

  VideoProfileTable = gtk_table_new(8, 2, FALSE);
  gtk_widget_show(VideoProfileTable);
  gtk_box_pack_start(GTK_BOX(dialog_vbox4), VideoProfileTable, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(VideoProfileTable), 3);

  VideoProfileEncoder = gtk_option_menu_new();
  gtk_widget_show(VideoProfileEncoder);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoProfileEncoder, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, VideoProfileEncoder, _("Select Encoder/Encoding"), NULL);

  uint encoderIndex = 0;
  uint i;
  encoderNames = (const char **)malloc(video_encoder_table_size * sizeof(char *));
  for (i = 0; i < video_encoder_table_size; i++) {
    if ((strcasecmp(profile->GetStringValue(CFG_VIDEO_ENCODING),
		    video_encoder_table[i].encoding) == 0) &&
	(strcasecmp(profile->GetStringValue(CFG_VIDEO_ENCODER), 
		    video_encoder_table[i].encoder) == 0)) {
      encoderIndex = i;
    }
    encoderNames[i] = video_encoder_table[i].encoding_name;
  }

  CreateOptionMenu(VideoProfileEncoder, 
		   encoderNames,
		   video_encoder_table_size,
		   encoderIndex);

  VideoProfileSize = gtk_option_menu_new();
  gtk_widget_show(VideoProfileSize);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoProfileSize, 1, 2, 2, 3,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, VideoProfileSize, _("Select Output Stream Size"), NULL);


  VideoProfileAspectRatio = gtk_option_menu_new();
  gtk_widget_show(VideoProfileAspectRatio);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoProfileAspectRatio, 1, 2, 3, 4,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, VideoProfileAspectRatio, _("Crop to Aspect Ratio"), NULL);

  uint aspectIndex = 0; 
  for (uint i = 0; i < NUM_ELEMENTS_IN_ARRAY(aspectValues); i++) {
    if (profile->GetFloatValue(CFG_VIDEO_CROP_ASPECT_RATIO)
	== aspectValues[i]) {
      aspectIndex = i;
      break;
    }
  }
  CreateOptionMenu(VideoProfileAspectRatio, aspectNames,
		   NUM_ELEMENTS_IN_ARRAY(aspectNames),
		   aspectIndex);

  label87 = gtk_label_new(_("Encoder:"));
  gtk_widget_show(label87);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label87, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label87), 0, 0.5);

  label88 = gtk_label_new(_("Size:"));
  gtk_widget_show(label88);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label88, 0, 1, 2, 3,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label88), 0, 0.5);

  label89 = gtk_label_new(_("Crop to Aspect Ratio:"));
  gtk_widget_show(label89);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label89, 0, 1, 3, 4,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label89), 0, 0.5);

  label93 = gtk_label_new(_("Video Profile:"));
  gtk_widget_show(label93);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label93, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 8);
  gtk_misc_set_alignment(GTK_MISC(label93), 0.05, 0.5);

  VideoProfileName = gtk_entry_new();
  gtk_widget_show(VideoProfileName);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoProfileName, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  if (new_profile == false) {
    gtk_entry_set_text(GTK_ENTRY(VideoProfileName), profile->GetName());
    gtk_widget_set_sensitive(VideoProfileName, false);
  }
    

  label92 = gtk_label_new(_("Encoder Settings:"));
  gtk_widget_show(label92);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label92, 0, 1, 7, 8,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label92), 0, 0.5);

  VideoEncoderSettingsButton = gtk_button_new();
  gtk_widget_show(VideoEncoderSettingsButton);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoEncoderSettingsButton, 1, 2, 7, 8,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_widget_set_sensitive(VideoEncoderSettingsButton, FALSE);
  gtk_tooltips_set_tip(tooltips, VideoEncoderSettingsButton, _("Configure Encoder Specific Settings"), NULL);

  alignment11 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment11);
  gtk_container_add(GTK_CONTAINER(VideoEncoderSettingsButton), alignment11);

  hbox59 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox59);
  gtk_container_add(GTK_CONTAINER(alignment11), hbox59);

  image11 = gtk_image_new_from_stock("gtk-preferences", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image11);
  gtk_box_pack_start(GTK_BOX(hbox59), image11, FALSE, FALSE, 0);

  label95 = gtk_label_new_with_mnemonic(_("Settings"));
  gtk_widget_show(label95);
  gtk_box_pack_start(GTK_BOX(hbox59), label95, FALSE, FALSE, 0);

  label91 = gtk_label_new(_("Bit Rate(kbps):"));
  gtk_widget_show(label91);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label91, 0, 1, 6, 7,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label91), 0, 0.5);

  VideoProfileBitRate_adj = gtk_adjustment_new(profile->GetIntegerValue(CFG_VIDEO_BIT_RATE), 25, 4000, 50, 1000, 1000);
  VideoProfileBitRate = gtk_spin_button_new(GTK_ADJUSTMENT(VideoProfileBitRate_adj), 50, 0);
  gtk_widget_show(VideoProfileBitRate);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoProfileBitRate, 1, 2, 6, 7,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_EXPAND), 0, 0);
  gtk_tooltips_set_tip(tooltips, VideoProfileBitRate, _("Enter Bit Rate"), NULL);

  label90 = gtk_label_new(_("Frame Rate(fps):"));
  gtk_widget_show(label90);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label90, 0, 1, 5, 6,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label90), 0, 0.5);

  VideoProfileFrameRate_adj = 
    gtk_adjustment_new(profile->GetFloatValue(CFG_VIDEO_FRAME_RATE), 
		       0, 
		       MyConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL) ==
		       VIDEO_SIGNAL_NTSC ?
		       VIDEO_NTSC_FRAME_RATE : VIDEO_PAL_FRAME_RATE, 
		       1, 5, 5);
  VideoProfileFrameRate = gtk_spin_button_new(GTK_ADJUSTMENT(VideoProfileFrameRate_adj), 1, 2);
  gtk_widget_show(VideoProfileFrameRate);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoProfileFrameRate, 1, 2, 5, 6,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, VideoProfileFrameRate, _("Enter Frame Rate"), NULL);

  label166 = gtk_label_new(_("Video Filter:"));
  gtk_widget_show(label166);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), label166, 0, 1, 4, 5,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label166), 0, 0.5);

  VideoFilterMenu = gtk_option_menu_new();
  gtk_widget_show(VideoFilterMenu);
  gtk_table_attach(GTK_TABLE(VideoProfileTable), VideoFilterMenu, 1, 2, 4, 5,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  uint filterIndex = 0; 
  for (i = 0; i < NUM_ELEMENTS_IN_ARRAY(profilefilterNames); i++) {
    if (strcasecmp(profile->GetStringValue(CFG_VIDEO_FILTER),
		   profilefilterNames[i]) == 0) {
      filterIndex = i;
      break;
    }
  }
  CreateOptionMenu(VideoFilterMenu,
		   profilefilterNames,
		   NUM_ELEMENTS_IN_ARRAY(profilefilterNames), 
		   filterIndex);

  dialog_action_area3 = GTK_DIALOG(VideoProfileDialog)->action_area;
  gtk_widget_show(dialog_action_area3);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area3), GTK_BUTTONBOX_END);

  cancelbutton3 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton3);
  gtk_dialog_add_action_widget(GTK_DIALOG(VideoProfileDialog), cancelbutton3, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton3, GTK_CAN_DEFAULT);

  okbutton3 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton3);
  gtk_dialog_add_action_widget(GTK_DIALOG(VideoProfileDialog), okbutton3, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton3, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) VideoProfileDialog, "response",
                    G_CALLBACK(on_VideoProfileDialog_response),
                    profile);
  g_signal_connect((gpointer) VideoProfileEncoder, "changed",
                    G_CALLBACK(on_VideoProfileEncoder_changed),
                    NULL);
  g_signal_connect((gpointer) VideoEncoderSettingsButton, "clicked", 
		   G_CALLBACK(on_VideoEncoderSettingsButton),
		   profile);
  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(VideoProfileDialog, VideoProfileDialog, "VideoProfileDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(VideoProfileDialog, dialog_vbox4, "dialog_vbox4");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoProfileTable, "VideoProfileTable");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoProfileEncoder, "VideoProfileEncoder");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoProfileSize, "VideoProfileSize");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoProfileAspectRatio, "VideoProfileAspectRatio");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label87, "label87");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label88, "label88");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label89, "label89");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label93, "label93");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoProfileName, "VideoProfileName");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label92, "label92");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoEncoderSettingsButton, "VideoEncoderSettingsButton");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, alignment11, "alignment11");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, hbox59, "hbox59");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, image11, "image11");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label95, "label95");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label91, "label91");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoProfileBitRate, "VideoProfileBitRate");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label90, "label90");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoProfileFrameRate, "VideoProfileFrameRate");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, label166, "label166");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, VideoFilterMenu, "VideoFilterMenu");
  GLADE_HOOKUP_OBJECT_NO_REF(VideoProfileDialog, dialog_action_area3, "dialog_action_area3");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, cancelbutton3, "cancelbutton3");
  GLADE_HOOKUP_OBJECT(VideoProfileDialog, okbutton3, "okbutton3");
  GLADE_HOOKUP_OBJECT_NO_REF(VideoProfileDialog, tooltips, "tooltips");

  CreateSizeMenu(profile->GetIntegerValue(CFG_VIDEO_WIDTH),
		 profile->GetIntegerValue(CFG_VIDEO_HEIGHT));

  //  return VideoProfileDialog;
  gtk_widget_show(VideoProfileDialog);
}

/* end video_dialog.cpp */
