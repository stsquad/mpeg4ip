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

#ifdef HAVE_LINUX_VIDEODEV2_H
#include "video_v4l2_source.h"
#else
#include "video_v4l_source.h"
#endif


static GtkWidget *dialog = NULL;

static char* source_type;
static char* source_name;
static GtkWidget *source_combo;
static GtkWidget *source_entry;
static GtkWidget *source_list;
static GtkWidget *browse_button;
static bool source_modified;
static bool default_file_audio_dialog = false;
static int32_t default_file_audio_source = -1;
static GtkWidget *input_label;
static GtkWidget *input_menu;
static GtkWidget *signal_label;
static GtkWidget *signal_menu;
static GSList* signal_menu_items;
static GtkWidget *channel_list_label;
static GtkWidget *channel_list_menu;
static GtkWidget *channel_label;
static GtkWidget *channel_combo;
static GtkWidget *track_label;
static GtkWidget *track_menu;
static GtkWidget *encoder_menu;
static GtkWidget *size_menu;
static GtkWidget *aspect_menu;
static GtkObject *frame_rate_ntsc_adjustment;
static GtkObject *frame_rate_pal_adjustment;
static GtkWidget *frame_rate_spinner;
static GtkWidget *bit_rate_spinner;

static CVideoCapabilities* pVideoCaps;

static char** inputNames = NULL;
static u_int8_t inputIndex = 0;
static u_int8_t inputNumber = 0;	// how many inputs total

static char* signalNames[] = {
	"PAL", "NTSC", "SECAM"
};
static u_int8_t signalIndex;

static u_int8_t channelListIndex;

static u_int8_t channelIndex;

static u_int32_t trackIndex;
static u_int32_t trackNumber;	// how many tracks total
static u_int32_t* trackValues = NULL;

static uint32_t encoderIndex;
static char **encoderNames = NULL;
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
static char* aspectNames[] = {
	"Standard 4:3", "Letterbox 2.35", "Letterbox 1.85", "HDTV 16:9"
};
static u_int8_t aspectIndex;

// forward declarations
static void SetAvailableSignals(void);

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
	dialog = NULL;
} 

static bool SourceIsDevice()
{
	return IsDevice(gtk_entry_get_text(GTK_ENTRY(source_entry)));
}

static void ShowSourceSpecificSettings()
{
	if (SourceIsDevice()) {
		gtk_widget_show(input_label);
		gtk_widget_show(input_menu);
		gtk_widget_show(signal_label);
		gtk_widget_show(signal_menu);
		gtk_widget_show(channel_list_label);
		gtk_widget_show(channel_list_menu);
		gtk_widget_show(channel_label);
		gtk_widget_show(channel_combo);

		gtk_widget_hide(track_label);
		gtk_widget_hide(track_menu);
	} else {
		gtk_widget_hide(input_label);
		gtk_widget_hide(input_menu);
		gtk_widget_hide(signal_label);
		gtk_widget_hide(signal_menu);
		gtk_widget_hide(channel_list_label);
		gtk_widget_hide(channel_list_menu);
		gtk_widget_hide(channel_label);
		gtk_widget_hide(channel_combo);

		gtk_widget_show(track_label);
		gtk_widget_show(track_menu);
	}
}

static void EnableChannels()
{
	bool hasTuner = false;

	if (pVideoCaps && pVideoCaps->m_inputHasTuners[inputIndex]) {
		hasTuner = true;
	}

	gtk_widget_set_sensitive(GTK_WIDGET(channel_list_menu), hasTuner);
	gtk_widget_set_sensitive(GTK_WIDGET(channel_combo), hasTuner);
}

static void ChangeInput(u_int8_t newIndex)
{
	inputIndex = newIndex;
	SetAvailableSignals();
	EnableChannels();
}

static void on_input_menu_activate(GtkWidget *widget, gpointer data)
{
	ChangeInput(((unsigned int)data) & 0xFF);
}

void CreateInputMenu(CVideoCapabilities* pNewVideoCaps)
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
	char** newInputNames = (char**)malloc(sizeof(char*) * newInputNumber);

	for (u_int8_t i = 0; i < newInputNumber; i++) {
		char buf[64];
		snprintf(buf, sizeof(buf), "%u - %s",
			i, pNewVideoCaps->m_inputNames[i]);
		newInputNames[i] = stralloc(buf);
	}

	// (re)create the menu
	input_menu = CreateOptionMenu(
		input_menu,
		newInputNames, 
		newInputNumber,
		inputIndex,
		GTK_SIGNAL_FUNC(on_input_menu_activate));

	// free up old names
	for (u_int8_t i = 0; i < inputNumber; i++) {
		free(inputNames[i]);
	}
	free(inputNames);
	inputNames = newInputNames;
	inputNumber = newInputNumber;
}

static void SourceV4LDevice()
{
	const char *newSourceName =
		gtk_entry_get_text(GTK_ENTRY(source_entry));

	// don't probe the already open device!
	if (!strcmp(newSourceName, 
	  MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME))) {
		return;
	}

	// probe new source device
	CVideoCapabilities* pNewVideoCaps = 
		new CVideoCapabilities(newSourceName);
	
	// check for errors
	if (!pNewVideoCaps->IsValid()) {
		if (!pNewVideoCaps->m_canOpen) {
			ShowMessage("Change Video Source",
				"Specified video source can't be opened, check name");
		} else {
			ShowMessage("Change Video Source",
				"Specified video source doesn't support capture");
		}
		delete pNewVideoCaps;
		return;
	}

	// change inputs menu
	CreateInputMenu(pNewVideoCaps);

	if (pVideoCaps != MyConfig->m_videoCapabilities) {
		delete pVideoCaps;
	}
	pVideoCaps = pNewVideoCaps;

	ChangeInput(inputIndex);
}

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

static void ChangeSource()
{
	const char* new_source_name =
		gtk_entry_get_text(GTK_ENTRY(source_entry));

	if (!strcmp(new_source_name, source_name)) {
		source_modified = false;
		return;
	}

	free(source_name);
	source_name = stralloc(new_source_name);

	default_file_audio_source = -1;

	// decide what type of source we have
	if (SourceIsDevice()) {
		source_type = VIDEO_SOURCE_V4L;

		SourceV4LDevice();
	} else {
		if (pVideoCaps != MyConfig->m_videoCapabilities) {
			delete pVideoCaps;
		}
		pVideoCaps = NULL;

		if (IsUrl(source_name)) {
			source_type = URL_SOURCE;
		} else {
			if (access(source_name, R_OK) != 0) {
				ShowMessage("Change Video Source",
					"Specified video source can't be opened, check name");
			}
			source_type = FILE_SOURCE;
		}

		if (!default_file_audio_dialog
		  && FileDefaultAudio(source_name) >= 0) {
			YesOrNo(
				"Change Video Source",
				"Do you want to use this for the audio source also?",
				true,
				GTK_SIGNAL_FUNC(on_yes_default_file_audio_source),
				GTK_SIGNAL_FUNC(on_no_default_file_audio_source));
		}
	}

	track_menu = CreateTrackMenu(
		track_menu,
		'V',
		source_name,
		&trackIndex,
		&trackNumber,
		&trackValues);

	ShowSourceSpecificSettings();

	source_modified = false;
}

static void on_source_browse_button (GtkWidget *widget, gpointer *data)
{
	FileBrowser(source_entry, GTK_SIGNAL_FUNC(ChangeSource));
}

static void on_source_entry_changed(GtkWidget *widget, gpointer *data)
{
	if (widget == source_entry) {
		source_modified = true;
	}
}

static int on_source_leave(GtkWidget *widget, gpointer *data)
{
	if (source_modified) {
		ChangeSource();
	}
	return FALSE;
}

static void on_source_list_changed(GtkWidget *widget, gpointer *data)
{
	if (widget == source_list) {
		ChangeSource();
	}
}


char* GetChannelName(size_t index, void* pUserData)
{
	return ((struct CHANLIST*)pUserData)[index].name;
}

static void CreateChannelCombo()
{
  struct CHANLISTS* pChannelList = chanlists;

	GList* list = NULL;
	for (int i = 0; i < pChannelList[channelListIndex].count; i++) {
		list = g_list_append(list, 
			pChannelList[channelListIndex].list[i].name);
	}

	gtk_combo_set_popdown_strings(GTK_COMBO(channel_combo), list);
	// although we do want to limit the combo choices to the ones we provide
	// this call results is some odd UI behaviors
	// gtk_combo_set_value_in_list(GTK_COMBO(channel_combo), 1, 0);
	gtk_combo_set_use_arrows_always(GTK_COMBO(channel_combo), 1);

	GtkWidget* entry = GTK_COMBO(channel_combo)->entry;
	gtk_entry_set_text(GTK_ENTRY(entry), 
		pChannelList[channelListIndex].list[channelIndex].name);

	gtk_widget_show(channel_combo);
}

void ChangeChannelList(u_int8_t newIndex)
{
	channelListIndex = newIndex;

	// change channel index to zero
	channelIndex = 0;

	// recreate channel menu
	CreateChannelCombo();
}

static void on_channel_list_menu_activate(GtkWidget *widget, gpointer data)
{
	ChangeChannelList((unsigned int)data & 0xFF);
}

char* GetChannelListName(size_t index, void* pUserData)
{
	return ((struct CHANLISTS*)pUserData)[index].name;
}

static void CreateChannelListMenu()
{
  struct CHANLISTS* pChannelList = chanlists;

	channel_list_menu = CreateOptionMenu(
		channel_list_menu,
		GetChannelListName,
		pChannelList,
		0xFF,
		channelListIndex,
		GTK_SIGNAL_FUNC(on_channel_list_menu_activate));
}

static void on_size_menu_activate(GtkWidget *widget, gpointer data)
{
	sizeIndex = ((unsigned int)data) & 0xFF;
}

static void CreateSizeMenu(uint16_t width)
{
  char **names = NULL;
  
  switch (signalIndex) {
  case 0:
    // PAL
    names = video_encoder_table[encoderIndex].sizeNamesPAL;
    sizeWidthValues = video_encoder_table[encoderIndex].widthValuesPAL;
    sizeWidthValueCnt = video_encoder_table[encoderIndex].numSizesPAL;
    sizeHeightValues = video_encoder_table[encoderIndex].heightValuesPAL;
    sizeHeightValueCnt = sizeWidthValueCnt;
    break;
  case 1:
    // NTSC
    names = video_encoder_table[encoderIndex].sizeNamesNTSC;
    sizeWidthValues = video_encoder_table[encoderIndex].widthValuesNTSC;
    sizeWidthValueCnt = video_encoder_table[encoderIndex].numSizesNTSC;
    sizeHeightValues = video_encoder_table[encoderIndex].heightValuesNTSC;
    sizeHeightValueCnt = sizeWidthValueCnt;
    break;
  case 2:
    // Secam
    names = video_encoder_table[encoderIndex].sizeNamesSecam;
    sizeWidthValues = video_encoder_table[encoderIndex].widthValuesSecam;
    sizeWidthValueCnt = video_encoder_table[encoderIndex].numSizesSecam;
    sizeHeightValues = video_encoder_table[encoderIndex].heightValuesSecam;
    sizeHeightValueCnt = sizeWidthValueCnt;
    break;
  };
  if (names == NULL) return;

  sizeMaxIndex = sizeHeightValueCnt;
	u_int8_t i;
	for (i = 0; i < sizeMaxIndex; i++) {
		if (sizeWidthValues[i] >= width) {
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

	size_menu = CreateOptionMenu(
		size_menu,
		names, 
		sizeMaxIndex,
		sizeIndex,
		GTK_SIGNAL_FUNC(on_size_menu_activate));
}
static void on_encoder_menu_activate (GtkWidget *widget, 
				      gpointer data)
{
  encoderIndex = ((uint32_t) data) & 0xff;
  CreateSizeMenu(sizeWidthValues[sizeIndex]);
}

void ChangeSignal(u_int8_t newIndex)
{
	signalIndex = newIndex;

	CreateChannelListMenu();
	ChangeChannelList(0);

	CreateSizeMenu(sizeWidthValues[sizeIndex]);

	// change max frame rate spinner
	if (signalIndex == 1) {
		// NTSC 
		gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(frame_rate_spinner),
			GTK_ADJUSTMENT(frame_rate_ntsc_adjustment));
	} else {
		// PAL or SECAM
		float frameRate = gtk_spin_button_get_value_as_float(
			GTK_SPIN_BUTTON(frame_rate_spinner));

		if (frameRate > VIDEO_PAL_FRAME_RATE) {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(frame_rate_spinner),
				VIDEO_PAL_FRAME_RATE);
		}
		gtk_spin_button_set_adjustment(GTK_SPIN_BUTTON(frame_rate_spinner),
			GTK_ADJUSTMENT(frame_rate_pal_adjustment));
	}
}

static void on_signal_menu_activate(GtkWidget *widget, gpointer data)
{
	ChangeSignal(((unsigned int)data) & 0xFF);
}

static void SetAvailableSignals(void)
{
	GSList* item = signal_menu_items;
	u_int8_t i = 0;
	u_int8_t validSignalIndex = 0xFF;

	// for all menu items in the signal menu
	while (item != NULL) {
		// all signals are enabled
		bool enabled = true;

		// unless input is a tuner
		if (pVideoCaps != NULL
		  && pVideoCaps->m_inputHasTuners[inputIndex]) {

			// this signal type is valid for this tuner
		  	if (pVideoCaps->m_inputTunerSignalTypes[inputIndex] & (1 << i)) {

				// remember first valid signal type for this tuner
				if (validSignalIndex == 0xFF) {
					validSignalIndex = i;
				}

			} else { // this signal type is invalid for this tuner

				// so disable this menu item
				enabled = false;

				// check if our current signal type is invalid for this tuner
				if (signalIndex == i) {
					signalIndex = 0xFF;
				}
			}
		}

		gtk_widget_set_sensitive(GTK_WIDGET(item->data), enabled);

		item = item->next;
		i++;
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

static void on_aspect_menu_activate(GtkWidget *widget, gpointer data)
{
	aspectIndex = ((unsigned int)data) & 0xFF;
}

static bool ValidateAndSave(void)
{
  bool resizewindow;
	// if source has been modified
	if (source_modified) {
		// validate it
		ChangeSource();

		// can't validate
		if (source_modified) {
			return false;
		}
	}

	// if previewing, stop video source
	AVFlow->StopVideoPreview();

	// copy new values to config

	MyConfig->SetStringValue(CONFIG_VIDEO_SOURCE_TYPE,
		source_type);

	MyConfig->SetStringValue(CONFIG_VIDEO_SOURCE_NAME,
		gtk_entry_get_text(GTK_ENTRY(source_entry)));

	MyConfig->UpdateFileHistory(
		gtk_entry_get_text(GTK_ENTRY(source_entry)));

	if (MyConfig->m_videoCapabilities != pVideoCaps) {
		delete MyConfig->m_videoCapabilities;
		MyConfig->m_videoCapabilities = pVideoCaps;
		pVideoCaps = NULL;
	}

	if (strcasecmp(source_type, VIDEO_SOURCE_V4L) 
	  && default_file_audio_source >= 0) {
		MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_TYPE,
			source_type);
		MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_NAME,
			gtk_entry_get_text(GTK_ENTRY(source_entry)));
		MyConfig->SetIntegerValue(CONFIG_AUDIO_SOURCE_TRACK,
			default_file_audio_source);
	}

	MyConfig->SetIntegerValue(CONFIG_VIDEO_INPUT,
		inputIndex);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_SIGNAL,
		signalIndex);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX,
		channelListIndex);

	// extract channel index out of combo (not so simple)
	GtkWidget* entry = GTK_COMBO(channel_combo)->entry;
	const char* channelName = gtk_entry_get_text(GTK_ENTRY(entry));
	struct CHANLISTS* pChannelList = chanlists;
	for (int i = 0; i < pChannelList[channelListIndex].count; i++) {
		if (!strcmp(channelName, 
		  pChannelList[channelListIndex].list[i].name)) {
			channelIndex = i;
			break;
		}
	}
	MyConfig->SetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX,
		channelIndex);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_SOURCE_TRACK,
		trackValues ? trackValues[trackIndex] : 0);

	MyConfig->SetStringValue(CONFIG_VIDEO_ENCODING, 
				 video_encoder_table[encoderIndex].encoding);
	MyConfig->SetStringValue(CONFIG_VIDEO_ENCODER, 
				 video_encoder_table[encoderIndex].encoder);

	resizewindow = sizeWidthValues[sizeIndex] != MyConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
	resizewindow |= sizeHeightValues[sizeIndex] != MyConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	resizewindow |= aspectValues[aspectIndex] != MyConfig->GetFloatValue(CONFIG_VIDEO_ASPECT_RATIO);
	
	MyConfig->SetIntegerValue(CONFIG_VIDEO_RAW_WIDTH,
		sizeWidthValues[sizeIndex]);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT,
		sizeHeightValues[sizeIndex]);

	MyConfig->SetFloatValue(CONFIG_VIDEO_ASPECT_RATIO,
		aspectValues[aspectIndex]);

	MyConfig->SetFloatValue(CONFIG_VIDEO_FRAME_RATE,
		gtk_spin_button_get_value_as_float(
			GTK_SPIN_BUTTON(frame_rate_spinner)));

	MyConfig->SetIntegerValue(CONFIG_VIDEO_BIT_RATE,
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(bit_rate_spinner)));


	// now put the new configuration into effect

	MyConfig->Update();

	if (resizewindow) {
	  NewVideoWindow();
	}

	// restart video source
	if (MyConfig->GetBoolValue(CONFIG_VIDEO_ENABLE)) {
		AVFlow->StartVideoPreview();
	}

	// refresh display of settings in main window
	DisplayVideoSettings();  
	DisplayStatusSettings();  

	return true;
}

static void on_ok_button (GtkWidget *widget, gpointer *data)
{
	// check and save values
	if (!ValidateAndSave()) {
		return;
	}
    on_destroy_dialog(NULL, NULL);
}

static void on_cancel_button (GtkWidget *widget, gpointer *data)
{
	on_destroy_dialog(NULL, NULL);
}

void CreateVideoDialog (void) 
{
	GtkWidget* hbox;
	GtkWidget* vbox;
	GtkWidget* hbox2;
	GtkWidget* label;
	GtkWidget* button;
	GtkObject* adjustment;
	uint8_t i;

	SDL_LockMutex(dialog_mutex);
	if (dialog != NULL) {
	  SDL_UnlockMutex(dialog_mutex);
	  return;
	}
	SDL_UnlockMutex(dialog_mutex);
	pVideoCaps = MyConfig->m_videoCapabilities;
	default_file_audio_source = -1;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Video Settings");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
#ifndef HAVE_GTK_2_0
	gtk_container_set_resize_mode(GTK_CONTAINER(dialog), GTK_RESIZE_IMMEDIATE);
#endif

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
		FALSE, FALSE, 5);

	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	label = gtk_label_new(" Source:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	input_label = gtk_label_new("   Port:");
	gtk_misc_set_alignment(GTK_MISC(input_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), input_label, FALSE, FALSE, 0);

	signal_label = gtk_label_new("   Signal:");
	gtk_misc_set_alignment(GTK_MISC(signal_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), signal_label, FALSE, FALSE, 0);

	channel_list_label = gtk_label_new("   Channel List:");
	gtk_misc_set_alignment(GTK_MISC(channel_list_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), channel_list_label, FALSE, FALSE, 0);

	channel_label = gtk_label_new("   Channel:");
	gtk_misc_set_alignment(GTK_MISC(channel_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), channel_label, FALSE, FALSE, 0);

	track_label = gtk_label_new("   Track:");
	gtk_misc_set_alignment(GTK_MISC(track_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), track_label, FALSE, FALSE, 0);

	label = gtk_label_new(" Output:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Encoder:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Size:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Aspect Ratio:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Frame Rate (fps):");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Bit Rate (kbps):");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);


	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	hbox2 = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

	source_type = MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_TYPE);

	// source entry
	free(source_name);
	source_name = 
		stralloc(MyConfig->GetStringValue(CONFIG_VIDEO_SOURCE_NAME));

	source_modified = false;

	source_combo = CreateFileCombo(source_name);

	source_entry = GTK_COMBO(source_combo)->entry;

	source_list = GTK_COMBO(source_combo)->list;
	gtk_signal_connect(GTK_OBJECT(source_list), "select_child",
		GTK_SIGNAL_FUNC(on_source_list_changed), NULL);

	SetEntryValidator(GTK_OBJECT(source_entry),
		GTK_SIGNAL_FUNC(on_source_entry_changed), 
		GTK_SIGNAL_FUNC(on_source_leave));

	gtk_widget_show(source_combo);
	gtk_box_pack_start(GTK_BOX(hbox2), source_combo, TRUE, TRUE, 0);

	// browse button
	browse_button = gtk_button_new_with_label(" Browse... ");
	gtk_signal_connect(GTK_OBJECT(browse_button),
		 "clicked",
		 GTK_SIGNAL_FUNC(on_source_browse_button),
		 NULL);
	gtk_widget_show(browse_button);
	gtk_box_pack_start(GTK_BOX(hbox2), browse_button, FALSE, FALSE, 5);

	// N.B. because of the dependencies of 
	// input, signal, channel list, channel, and sizes
	// order of operations is important here

	channelIndex = 
		MyConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_INDEX);
	channelListIndex = 
		MyConfig->GetIntegerValue(CONFIG_VIDEO_CHANNEL_LIST_INDEX);
	signalIndex = 
		MyConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL); 
	inputIndex = 
		MyConfig->GetIntegerValue(CONFIG_VIDEO_INPUT);

	channel_combo = NULL;
	channel_combo = gtk_combo_new();
	CreateChannelCombo();

	channel_list_menu = NULL;
	CreateChannelListMenu();

	track_menu = NULL;
	track_menu = CreateTrackMenu(
		track_menu,
		'V',
		gtk_entry_get_text(GTK_ENTRY(source_entry)),
		&trackIndex,
		&trackNumber,
		&trackValues);

	trackIndex = 0; 
	for (i = 0; i < trackNumber; i++) {
		if (MyConfig->GetIntegerValue(CONFIG_VIDEO_SOURCE_TRACK)
		   == trackValues[i]) {
			trackIndex = i;
			break;
		}
	}

	sizeIndex = 0; 
	size_menu = NULL;
	CreateSizeMenu(MyConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH));
	signal_menu = CreateOptionMenu(
		NULL,
		signalNames, 
		sizeof(signalNames) / sizeof(char*),
		signalIndex,
		GTK_SIGNAL_FUNC(on_signal_menu_activate),
		&signal_menu_items);

	input_menu = NULL;
	CreateInputMenu(pVideoCaps);
	ChangeInput(inputIndex);

	gtk_box_pack_start(GTK_BOX(vbox), input_menu, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), signal_menu, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), channel_list_menu, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), channel_combo, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), track_menu, FALSE, FALSE, 0);

	// spacer
	label = gtk_label_new(" ");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	encoderIndex = 0;
	encoderNames = (char **)malloc(video_encoder_table_size * sizeof(char *));
	for (i = 0; i < video_encoder_table_size; i++) {
	  if ((strcasecmp(MyConfig->GetStringValue(CONFIG_VIDEO_ENCODING),
			  video_encoder_table[i].encoding) == 0) &&
	      (strcasecmp(MyConfig->GetStringValue(CONFIG_VIDEO_ENCODER), 
			  video_encoder_table[i].encoder) == 0)) {
	    encoderIndex = i;
	  }
	  encoderNames[i] = video_encoder_table[i].encoding_name;
	}

	encoder_menu = CreateOptionMenu(NULL, 
					encoderNames,
					video_encoder_table_size,
					encoderIndex, 
					GTK_SIGNAL_FUNC(on_encoder_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), encoder_menu, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), size_menu, FALSE, FALSE, 0);

	aspectIndex = 0; 
	for (i = 0; i < sizeof(aspectValues) / sizeof(float); i++) {
		if (MyConfig->GetFloatValue(CONFIG_VIDEO_ASPECT_RATIO)
		  == aspectValues[i]) {
			aspectIndex = i;
			break;
		}
	}
	aspect_menu = CreateOptionMenu(
		NULL,
		aspectNames, 
		sizeof(aspectNames) / sizeof(char*),
		aspectIndex,
		GTK_SIGNAL_FUNC(on_aspect_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), aspect_menu, FALSE, FALSE, 0);

	frame_rate_pal_adjustment = gtk_adjustment_new(
		MyConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE),
		1, VIDEO_PAL_FRAME_RATE, 1, 0, 0);
	gtk_object_ref(frame_rate_pal_adjustment);

	frame_rate_ntsc_adjustment = gtk_adjustment_new(
		MyConfig->GetFloatValue(CONFIG_VIDEO_FRAME_RATE),
		1, VIDEO_NTSC_FRAME_RATE, 1, 0, 0);
	gtk_object_ref(frame_rate_ntsc_adjustment);

	if (signalIndex == 1) {
		frame_rate_spinner = gtk_spin_button_new(
			GTK_ADJUSTMENT(frame_rate_ntsc_adjustment), 1, 2);
	} else {
		frame_rate_spinner = gtk_spin_button_new(
			GTK_ADJUSTMENT(frame_rate_pal_adjustment), 1, 2);
	}
	gtk_widget_show(frame_rate_spinner);
	gtk_box_pack_start(GTK_BOX(vbox), frame_rate_spinner, FALSE, FALSE, 0);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_VIDEO_BIT_RATE),
		25, 4000, 50, 0, 0);
	bit_rate_spinner = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 50, 0);
	gtk_widget_show(bit_rate_spinner);
	gtk_box_pack_start(GTK_BOX(vbox), bit_rate_spinner, FALSE, FALSE, 0);

	// Add standard buttons at bottom
	button = AddButtonToDialog(dialog,
		" OK ", 
		GTK_SIGNAL_FUNC(on_ok_button));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	AddButtonToDialog(dialog,
		" Cancel ", 
		GTK_SIGNAL_FUNC(on_cancel_button));

	ShowSourceSpecificSettings();

	gtk_widget_show(dialog);
}

/* end video_dialog.cpp */
