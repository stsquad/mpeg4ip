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
#include "video_source.h"

static GtkWidget *dialog;

static GtkWidget *device_entry;
static bool device_modified;
static GtkWidget *input_menu;
static GtkWidget *signal_menu;
static GSList* signal_menu_items;
static GtkWidget *channel_list_menu;
static GtkWidget *channel_menu;
static GtkWidget *size_menu;
static GtkWidget *aspect_menu;
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

static u_int16_t sizeWidthValues[] = {
	160, 176, 320, 352
};
static u_int16_t sizeHeightValues[] = {
	120, 144, 240, 288
};
static char* sizeNames[] = {
	"160 x 120", "176 x 144", "320 x 240", "352 x 288"
};
static u_int8_t sizeIndex;

static float aspectValues[] = {
	VIDEO_STD_ASPECT_RATIO, VIDEO_LB1_ASPECT_RATIO, VIDEO_LB2_ASPECT_RATIO
}; 
static char* aspectNames[] = {
	"Standard 4:3", "Letterbox 2.35", "Letterbox 1.85"
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

static void on_input_menu_activate(GtkWidget *widget, gpointer data)
{
	inputIndex = ((unsigned int)data) & 0xFF;

	SetAvailableSignals();
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

	// get signal types sync'ed up with new input
	SetAvailableSignals();
}

static void on_changed(GtkWidget *widget, gpointer *data)
{
	if (widget == device_entry) {
		device_modified = true;
	}
}

static void on_device_leave(GtkWidget *widget, gpointer *data)
{
	if (!device_modified) {
		return;
	}

	// probe new device
	CVideoCapabilities* pNewVideoCaps = new CVideoCapabilities(
		gtk_entry_get_text(GTK_ENTRY(device_entry)));
	
	// check for errors
	if (!pNewVideoCaps->IsValid()) {
		if (!pNewVideoCaps->m_canOpen) {
			ShowMessage("Change Video Device",
				"Specified video device can't be opened, check name");
		} else {
			ShowMessage("Change Video Device",
				"Specified video device doesn't support capture");
		}
		delete pNewVideoCaps;
		return;
	}

	// change inputs menu
	CreateInputMenu(pNewVideoCaps);

	pVideoCaps = pNewVideoCaps;

	device_modified = false;
}

static void on_channel_menu_activate(GtkWidget *widget, gpointer data)
{
	channelIndex = ((unsigned int)data) & 0xFF;
}

char* GetChannelName(size_t index, void* pUserData)
{
	return ((struct CHANNEL*)pUserData)[index].name;
}

static void CreateChannelMenu()
{
	struct CHANNEL_LIST* pChannelList =
		ListOfChannelLists[signalIndex];

	channel_menu = CreateOptionMenu(
		channel_menu,
		GetChannelName,
		pChannelList[channelListIndex].list,
		pChannelList[channelListIndex].count,
		channelIndex,
		GTK_SIGNAL_FUNC(on_channel_menu_activate));

	if (pVideoCaps == NULL
	  || !pVideoCaps->m_inputHasTuners[inputIndex]) {
		gtk_widget_set_sensitive(GTK_WIDGET(channel_menu), false);
	}
}

static void on_channel_list_menu_activate(GtkWidget *widget, gpointer data)
{
	channelListIndex = ((unsigned int)data) & 0xFF;

	// change channel index to zero
	channelIndex = 0;

	// recreate channel menu
	CreateChannelMenu();
}

char* GetChannelListName(size_t index, void* pUserData)
{
	return ((struct CHANNEL_LIST*)pUserData)[index].name;
}

static void CreateChannelListMenu()
{
	struct CHANNEL_LIST* pChannelList =
		ListOfChannelLists[signalIndex];

	channel_list_menu = CreateOptionMenu(
		channel_list_menu,
		GetChannelListName,
		pChannelList,
		0xFF,
		channelListIndex,
		GTK_SIGNAL_FUNC(on_channel_list_menu_activate));

	if (pVideoCaps == NULL
	  || !pVideoCaps->m_inputHasTuners[inputIndex]) {
		gtk_widget_set_sensitive(GTK_WIDGET(channel_list_menu), false);
	}
}

static void on_signal_menu_activate(GtkWidget *widget, gpointer data)
{
	signalIndex = ((unsigned int)data) & 0xFF;

	// change channel list index to 0
	on_channel_list_menu_activate(GTK_WIDGET(channel_list_menu), 0);

	CreateChannelListMenu();
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
	}
}

static void on_size_menu_activate(GtkWidget *widget, gpointer data)
{
	sizeIndex = ((unsigned int)data) & 0xFF;
}

static void on_aspect_menu_activate(GtkWidget *widget, gpointer data)
{
	aspectIndex = ((unsigned int)data) & 0xFF;
}

static bool ValidateAndSave(void)
{
	// if device has been modified
	// and isn't validated, then don't proceed
	if (device_modified) {
		return false;
	}

	// if previewing, stop video source
	StopVideoPreview();

	// copy new values to config

	free(MyConfig->m_videoDeviceName);
	MyConfig->m_videoDeviceName = stralloc(
		gtk_entry_get_text(GTK_ENTRY(device_entry)));

	MyConfig->m_videoInput =
		inputIndex;

	MyConfig->m_videoSignal =
		signalIndex;

	MyConfig->m_videoChannelListIndex =
		channelListIndex;

	MyConfig->m_videoChannelIndex =
		channelIndex;

	MyConfig->m_videoRawWidth =
		sizeWidthValues[sizeIndex];

	MyConfig->m_videoRawHeight =
		sizeHeightValues[sizeIndex];

	MyConfig->m_videoAspectRatio =
		aspectValues[aspectIndex];

	MyConfig->m_videoTargetFrameRate =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(frame_rate_spinner));

	MyConfig->m_videoTargetBitRate =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(bit_rate_spinner));


	// now put the new configuration into effect

	GenerateMpeg4VideoConfig(MyConfig);

	// if previewing, restart video source
	if (MyConfig->m_videoEnable && MyConfig->m_videoPreview) {
		StartVideoPreview();
	}

	// refresh display of settings in main window
	DisplayVideoSettings();  

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
	GtkWidget* label;
	GtkWidget* button;
	GtkObject* adjustment;

	pVideoCaps = MyConfig->m_videoCapabilities;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Video Settings");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

	hbox = gtk_hbox_new(TRUE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
		FALSE, FALSE, 5);

	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	label = gtk_label_new(" Device:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Input Port:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Signal:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Channel List:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Channel:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Size:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Aspect Ratio:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Frame Rate (fps):");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Bit Rate (Kbps):");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);


	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	device_entry = gtk_entry_new_with_max_length(128);
	gtk_entry_set_text(GTK_ENTRY(device_entry), MyConfig->m_videoDeviceName);
	device_modified = false;
	SetEntryValidator(GTK_OBJECT(device_entry),
		GTK_SIGNAL_FUNC(on_changed),
		GTK_SIGNAL_FUNC(on_device_leave));
	gtk_widget_show(device_entry);
	gtk_box_pack_start(GTK_BOX(vbox), device_entry, TRUE, TRUE, 0);

	// N.B. because of the dependencies of 
	// input, signal, channel list, and channel
	// order of operations is important here

	channelIndex = MyConfig->m_videoChannelIndex;
	channelListIndex = MyConfig->m_videoChannelListIndex;
	signalIndex = MyConfig->m_videoSignal; 
	inputIndex = MyConfig->m_videoInput;

	channel_menu = NULL;
	CreateChannelMenu();

	channel_list_menu = NULL;
	CreateChannelListMenu();

	signal_menu = CreateOptionMenu(
		NULL,
		signalNames, 
		sizeof(signalNames) / sizeof(char*),
		signalIndex,
		GTK_SIGNAL_FUNC(on_signal_menu_activate),
		&signal_menu_items);

	input_menu = NULL;
	CreateInputMenu(pVideoCaps);

	gtk_box_pack_start(GTK_BOX(vbox), input_menu, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), signal_menu, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), channel_list_menu, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), channel_menu, TRUE, TRUE, 0);

	sizeIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(sizeWidthValues) / sizeof(u_int16_t); i++) {
		if (MyConfig->m_videoWidth == sizeWidthValues[i]) {
			sizeIndex = i;
			break;
		}
	}
	size_menu = CreateOptionMenu(
		NULL,
		sizeNames, 
		sizeof(sizeNames) / sizeof(char*),
		sizeIndex,
		GTK_SIGNAL_FUNC(on_size_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), size_menu, TRUE, TRUE, 0);

	aspectIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(aspectValues) / sizeof(float); i++) {
		if (MyConfig->m_videoAspectRatio == aspectValues[i]) {
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
	gtk_box_pack_start(GTK_BOX(vbox), aspect_menu, TRUE, TRUE, 0);

	adjustment = gtk_adjustment_new(MyConfig->m_videoTargetFrameRate,
		1, 30, 1, 0, 0);
	frame_rate_spinner = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
	gtk_widget_show(frame_rate_spinner);
	gtk_box_pack_start(GTK_BOX(vbox), frame_rate_spinner, TRUE, TRUE, 0);

	adjustment = gtk_adjustment_new(MyConfig->m_videoTargetBitRate,
		50, 1500, 50, 0, 0);
	bit_rate_spinner = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 50, 0);
	gtk_widget_show(bit_rate_spinner);
	gtk_box_pack_start(GTK_BOX(vbox), bit_rate_spinner, TRUE, TRUE, 0);


	// Add standard buttons at bottom
	button = AddButtonToDialog(dialog,
		" OK ", 
		GTK_SIGNAL_FUNC(on_ok_button));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	AddButtonToDialog(dialog,
		" Cancel ", 
		GTK_SIGNAL_FUNC(on_cancel_button));

	gtk_widget_show(dialog);
}

/* end video_dialog.cpp */
