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
static GtkWidget *mono_button;
static GtkWidget *stereo_button;
static GtkWidget *sampling_rate_menu;
static GtkWidget *bit_rate_menu;

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

static void on_sampling_rate_menu_activate (GtkWidget *widget, gpointer data)
{
	u_int8_t newIndex = (u_int8_t)data;

	if (samplingRateIndex == newIndex) {
		return;
	}
	samplingRateIndex = newIndex;

	// ensure that bit rate is consistent with new sampling rate
	if (samplingRateIndex < 6) {
		// MPEG 2 or 2.5 mode only goes up to 160 Kbps
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
	u_int8_t newIndex = (u_int8_t)data;

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

	free(MyConfig->m_audioDeviceName);
	MyConfig->m_audioDeviceName = stralloc(
		gtk_entry_get_text(GTK_ENTRY(device_entry)));

	bool mono = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mono_button));
	if (mono) {
		MyConfig->m_audioChannels = 1;
	} else {
		MyConfig->m_audioChannels = 2;
	}

	MyConfig->m_audioSamplingRate = samplingRateValues[samplingRateIndex];
	MyConfig->m_audioTargetBitRate = bitRateValues[bitRateIndex];

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
	GSList* radioGroup;
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

	mono_button = gtk_radio_button_new_with_label(NULL, "Mono");
	gtk_widget_show(mono_button);
	gtk_box_pack_start(GTK_BOX(vbox), mono_button, TRUE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mono_button), 
		MyConfig->m_audioChannels == 1);

	label = gtk_label_new(" Sampling Rate (Hz):");
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
	gtk_entry_set_text(GTK_ENTRY(device_entry), MyConfig->m_audioDeviceName);
	gtk_widget_show(device_entry);
	gtk_box_pack_start(GTK_BOX(vbox), device_entry, TRUE, TRUE, 0);

	radioGroup = gtk_radio_button_group(GTK_RADIO_BUTTON(mono_button));
	stereo_button = gtk_radio_button_new_with_label(radioGroup, "Stereo");
	gtk_widget_show(stereo_button);
	gtk_box_pack_start(GTK_BOX(vbox), stereo_button, TRUE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stereo_button), 
		MyConfig->m_audioChannels == 2);

	samplingRateIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(samplingRateValues) / sizeof(u_int32_t); i++) {
		if (MyConfig->m_audioSamplingRate == samplingRateValues[i]) {
			samplingRateIndex = i;
			break;
		}
	}
	sampling_rate_menu = CreateOptionMenu (NULL,
		(const char**) samplingRateNames, 
		sizeof(samplingRateNames) / sizeof(char*),
		samplingRateIndex,
		GTK_SIGNAL_FUNC(on_sampling_rate_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), sampling_rate_menu, TRUE, TRUE, 0);

	bitRateIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(bitRateValues) / sizeof(u_int16_t); i++) {
		if (MyConfig->m_audioTargetBitRate == bitRateValues[i]) {
			bitRateIndex = i;
			break;
		}
	}
	bit_rate_menu = CreateOptionMenu (NULL,
		(const char**) bitRateNames, 
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
