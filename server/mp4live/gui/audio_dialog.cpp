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

static GtkWidget *dialog;

static GtkWidget *device_entry;
static GtkWidget *input_menu;
static GtkWidget *channel_menu;
static GtkWidget *sampling_rate_menu;
static GtkWidget *bit_rate_menu;

static char* inputValues[] = {
	"cd", "line", "mic"
};
static char* inputNames[] = {
	"CD", "Line In", "Microphone"
};
static u_int8_t inputIndex;

static u_int8_t channelValues[] = {
	1, 2
};
static char* channelNames[] = {
	"1 - Mono", "2 - Stereo"
};
static u_int8_t channelIndex;

// from mp3.cpp

static u_int16_t bitRateValues[] = {
	8, 16, 24, 32, 40, 48, 
	56, 64, 80, 96, 112, 128, 
	144, 160, 192, 224, 256, 320
};
static char* bitRateNames[] = {
	"8", "16", "24", "32", "40", "48", 
	"56", "64", "80", "96", "112", "128", 
	"144", "160", "192", "224", "256", "320"
};
static u_int8_t bitRateIndex;

static u_int32_t samplingRateValues[] = {
	8000, 11025, 12000, 16000, 22050, 
	24000, 32000, 44100, 48000
};
static char* samplingRateNames[] = {
	"8000", "11025", "12000", "16000", "22050", 
	"24000", "32000", "44100", "48000"
};
static u_int8_t samplingRateIndex;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
	dialog = NULL;
} 

static void on_input_menu_activate (GtkWidget *widget, gpointer data)
{
	inputIndex = (unsigned int)data & 0xFF;
}

static void on_channel_menu_activate (GtkWidget *widget, gpointer data)
{
	channelIndex = (unsigned int)data & 0xFF;
}

static void on_sampling_rate_menu_activate (GtkWidget *widget, gpointer data)
{
	u_int8_t newIndex = (unsigned int)data & 0xFF;

	if (samplingRateIndex == newIndex) {
		return;
	}
	samplingRateIndex = newIndex;

	// ensure that bit rate is consistent with new sampling rate
	if (samplingRateIndex < 6) {
		// MPEG 2 or 2.5 mode only goes up to 160 kbps
		if (bitRateIndex >= 14) {
			ShowMessage("Change Sampling Rate",
				"New sampling rate requires that bit rate be lowered");
			bitRateIndex = 13;
			gtk_option_menu_set_history(GTK_OPTION_MENU(bit_rate_menu),
				 bitRateIndex);
		}
	} else {
		// MPEG 1
		if (bitRateIndex < 3) {
			ShowMessage("Change Sampling Rate",
				"New sampling rate requires that bit rate be raised");
			bitRateIndex = 3;
			gtk_option_menu_set_history(GTK_OPTION_MENU(bit_rate_menu),
				 bitRateIndex);
		} else if (bitRateIndex == 12) {
			ShowMessage("Change Sampling Rate",
				"New sampling rate requires that bit rate be changed");
			bitRateIndex = 13;
			gtk_option_menu_set_history(GTK_OPTION_MENU(bit_rate_menu),
				 bitRateIndex);
		}
	}
}

static void on_bit_rate_menu_activate (GtkWidget *widget, gpointer data)
{
	u_int8_t newIndex = (unsigned int)data & 0xFF;

	if (bitRateIndex == newIndex) {
		return;
	}
	bitRateIndex = newIndex;

	// ensure that sampling rate is consistent with new bit rate
	if (bitRateIndex < 3 || bitRateIndex == 12) {
		if (samplingRateIndex > 5) {
			ShowMessage("Change Bit Rate",
				"New bit rate requires that sampling rate be lowered");
			samplingRateIndex = 5;
			gtk_option_menu_set_history(GTK_OPTION_MENU(sampling_rate_menu),
				 samplingRateIndex);
		}
	} else if (bitRateIndex > 13) {
		if (samplingRateIndex < 6) {
			ShowMessage("Change Bit Rate",
				"New bit rate requires that sampling rate be raised");
			samplingRateIndex = 6;
			gtk_option_menu_set_history(GTK_OPTION_MENU(sampling_rate_menu),
				 samplingRateIndex);
		}
	}
}

static bool ValidateAndSave(void)
{
	// copy new values to config

	MyConfig->SetStringValue(CONFIG_AUDIO_DEVICE_NAME,
		gtk_entry_get_text(GTK_ENTRY(device_entry)));

	MyConfig->SetStringValue(CONFIG_AUDIO_INPUT_NAME,
		inputValues[inputIndex]);

	MyConfig->SetIntegerValue(CONFIG_AUDIO_CHANNELS, 
		channelValues[channelIndex]);

	MyConfig->SetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE, 
		samplingRateValues[samplingRateIndex]);

	MyConfig->SetIntegerValue(CONFIG_AUDIO_BIT_RATE,
		bitRateValues[bitRateIndex]);

	MyConfig->Update();

	DisplayAudioSettings();  // display settings in main window

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

void CreateAudioDialog (void) 
{
	GtkWidget* hbox;
	GtkWidget* vbox;
	GtkWidget* label;
	GtkWidget* button;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Audio Settings");

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

	label = gtk_label_new(" Input:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Channels:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Sampling Rate (Hz):");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Bit Rate (kbps):");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);


	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	device_entry = gtk_entry_new_with_max_length(128);
	gtk_entry_set_text(GTK_ENTRY(device_entry), 
		MyConfig->GetStringValue(CONFIG_AUDIO_DEVICE_NAME));
	gtk_widget_show(device_entry);
	gtk_box_pack_start(GTK_BOX(vbox), device_entry, TRUE, TRUE, 0);

	inputIndex = 0;
	for (u_int8_t i = 0; i < sizeof(inputValues) / sizeof(u_int8_t); i++) {
		if (!strcasecmp(MyConfig->GetStringValue(CONFIG_AUDIO_INPUT_NAME),
		  inputValues[i])) {
			inputIndex = i;
			break;
		}
	}
	input_menu = CreateOptionMenu (NULL,
		inputNames, 
		sizeof(inputNames) / sizeof(char*),
		inputIndex,
		GTK_SIGNAL_FUNC(on_input_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), input_menu, TRUE, TRUE, 0);

	channelIndex = 0;
	for (u_int8_t i = 0; i < sizeof(channelValues) / sizeof(u_int8_t); i++) {
		if (MyConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS)
		  == channelValues[i]) {
			channelIndex = i;
			break;
		}
	}
	channel_menu = CreateOptionMenu (NULL,
		channelNames, 
		sizeof(channelNames) / sizeof(char*),
		channelIndex,
		GTK_SIGNAL_FUNC(on_channel_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), channel_menu, TRUE, TRUE, 0);

	samplingRateIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(samplingRateValues) / sizeof(u_int32_t); i++) {
		if (MyConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE)
		  == samplingRateValues[i]) {
			samplingRateIndex = i;
			break;
		}
	}
	sampling_rate_menu = CreateOptionMenu (NULL,
		samplingRateNames, 
		sizeof(samplingRateNames) / sizeof(char*),
		samplingRateIndex,
		GTK_SIGNAL_FUNC(on_sampling_rate_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), sampling_rate_menu, TRUE, TRUE, 0);

	bitRateIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(bitRateValues) / sizeof(u_int16_t); i++) {
		if (MyConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE)
		  == bitRateValues[i]) {
			bitRateIndex = i;
			break;
		}
	}
	bit_rate_menu = CreateOptionMenu (NULL,
		bitRateNames, 
		sizeof(bitRateNames) / sizeof(char*),
		bitRateIndex,
		GTK_SIGNAL_FUNC(on_bit_rate_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), bit_rate_menu, TRUE, TRUE, 0);


	// Add standard buttons at bottom
	button = AddButtonToDialog(dialog,
		" OK ", 
		GTK_SIGNAL_FUNC(on_ok_button));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	AddButtonToDialog(dialog,
		" Cancel ", 
		GTK_SIGNAL_FUNC(on_cancel_button));

	gtk_widget_show(dialog);
	gtk_grab_add(dialog);
}

/* end audio_dialog.cpp */
