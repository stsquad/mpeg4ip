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
#include "audio_oss_source.h"
#include "audio_lame.h"

static GtkWidget *dialog;

static char* source_type;
static GtkWidget *source_entry;
static GtkWidget *source_combo;
static bool source_modified;
static GtkWidget *browse_button;
static GtkWidget *input_label;
static GtkWidget *input_menu;
static GtkWidget *track_label;
static GtkWidget *track_menu;
static GtkWidget *channel_menu;
static GtkWidget *encoding_menu;
static GtkWidget *sampling_rate_menu;
static GtkWidget *bit_rate_menu;

static CAudioCapabilities* pAudioCaps;

static char* inputValues[] = {
	"cd", "line", "mic", "mix"
};
static char* inputNames[] = {
	"CD", "Line In", "Microphone", "Via Mixer"
};
static u_int8_t inputIndex;

static u_int32_t trackIndex;
static u_int32_t trackNumber;	// how many tracks total
static u_int32_t* trackValues = NULL;

static u_int8_t channelValues[] = {
	1, 2
};
static char* channelNames[] = {
	"1 - Mono", "2 - Stereo"
};
static u_int8_t channelIndex;

static char* encodingNames[] = {
	"MP3", "AAC"
};
static u_int8_t encodingIndex;

static char** samplingRateNames = NULL;
static u_int32_t* samplingRateValues = NULL;
static u_int8_t samplingRateIndex;
static u_int8_t samplingRateNumber = 0;	// how many sampling rates

// union of valid sampling rates for MP3 and AAC
static const u_int32_t samplingRateAllValues[] = {
	7350, 8000, 11025, 12000, 16000, 22050, 
	24000, 32000, 44100, 48000, 64000, 88200, 96000
};

// union of valid bit rates for MP3 and AAC
static const u_int16_t bitRateAllValues[] = {
	8, 16, 24, 32, 40, 48, 
	56, 64, 80, 96, 112, 128, 
	144, 160, 192, 224, 256, 320
};
static const u_int8_t bitRateAllNumber =
	sizeof(bitRateAllValues) / sizeof(u_int16_t);

static u_int16_t bitRateValues[bitRateAllNumber];
static char* bitRateNames[bitRateAllNumber];
static u_int8_t bitRateIndex;
static u_int8_t bitRateNumber = 0; // how many bit rates

// forward function declarations
static void CreateSamplingRateMenu(CAudioCapabilities* pNewAudioCaps);
static void CreateBitRateMenu();
static void SetSamplingRate(u_int32_t samplingRate);


static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
	dialog = NULL;
} 

static bool SourceIsDevice()
{
	const char* source_name =
		gtk_entry_get_text(GTK_ENTRY(source_entry));
	return !strncmp(source_name, "/dev/", 5);
}

static void ShowSourceSpecificSettings()
{
	if (SourceIsDevice()) {
		gtk_widget_show(input_label);
		gtk_widget_show(input_menu);

		gtk_widget_hide(track_label);
		gtk_widget_hide(track_menu);
	} else {
		gtk_widget_hide(input_label);
		gtk_widget_hide(input_menu);

		gtk_widget_show(track_label);
		gtk_widget_show(track_menu);
	}
}

static void SourceOssDevice()
{
	char *newSourceName =
		gtk_entry_get_text(GTK_ENTRY(source_entry));

	// don't probe the already open device!
	if (!strcmp(newSourceName, 
	  MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME))) {
		return;
	}

	// probe new device
	CAudioCapabilities* pNewAudioCaps = 
		new CAudioCapabilities(newSourceName);
	
	// check for errors
	if (!pNewAudioCaps->IsValid()) {
		ShowMessage("Change Audio Source",
			"Specified audio source can't be opened, check name");
		delete pNewAudioCaps;
		return;
	}

	if (pAudioCaps != MyConfig->m_audioCapabilities) {
		delete pAudioCaps;
	}
	pAudioCaps = pNewAudioCaps;

	// change sampling rate menu
	CreateSamplingRateMenu(pNewAudioCaps);

	// change bit rate menu
	CreateBitRateMenu();
}

static void ChangeSource()
{
	if (SourceIsDevice()) {
		source_type = AUDIO_SOURCE_OSS;
	
		SourceOssDevice();

	} else {
		if (pAudioCaps != MyConfig->m_audioCapabilities) {
			delete pAudioCaps;
		}
		pAudioCaps = NULL;

		const char* source_name =
			gtk_entry_get_text(GTK_ENTRY(source_entry));

		if (access(source_name, R_OK) != 0) {
			ShowMessage("Change Audio Source",
				"Specified audio source can't be opened, check name");
		}

		if (IsMp4File(source_name)) {
			source_type = FILE_SOURCE_MP4;
		} else {
			source_type = FILE_SOURCE_MPEG2;
		}
	}

	track_menu = CreateTrackMenu(
		track_menu,
		'A',
		gtk_entry_get_text(GTK_ENTRY(source_entry)),
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

static void on_source_changed(GtkWidget *widget, gpointer *data)
{
	if (widget == source_entry) {
		source_modified = true;
	}
}

static void on_source_leave(GtkWidget *widget, gpointer *data)
{
	if (!source_modified) {
		return;
	}

	ChangeSource();
}

static void on_input_menu_activate (GtkWidget *widget, gpointer data)
{
	inputIndex = (unsigned int)data & 0xFF;
}

static void on_channel_menu_activate (GtkWidget *widget, gpointer data)
{
	channelIndex = (unsigned int)data & 0xFF;
}

static void on_encoding_menu_activate (GtkWidget *widget, gpointer data)
{
	encodingIndex = (unsigned int)data & 0xFF;

	CreateSamplingRateMenu(pAudioCaps);
	CreateBitRateMenu();
}

static void on_sampling_rate_menu_activate (GtkWidget *widget, gpointer data)
{
	samplingRateIndex = (unsigned int)data & 0xFF;

	CreateBitRateMenu();
}

static void on_bit_rate_menu_activate (GtkWidget *widget, gpointer data)
{
	bitRateIndex = (unsigned int)data & 0xFF;
}

void CreateSamplingRateMenu(CAudioCapabilities* pNewAudioCaps)
{
	// remember current sampling rate
	u_int32_t oldSamplingRate = 0;
	if (samplingRateValues) {
		oldSamplingRate = samplingRateValues[samplingRateIndex];
	}

	// invalidate index, will fix up below
	samplingRateIndex = 255;

	u_int8_t maxSamplingRateNumber;
	const u_int32_t* samplingRates = NULL;

	if (pNewAudioCaps) {
		maxSamplingRateNumber = pNewAudioCaps->m_numSamplingRates;
		samplingRates = &pNewAudioCaps->m_samplingRates[0];
	} else {
		maxSamplingRateNumber = 
			sizeof(samplingRateAllValues) / sizeof(u_int32_t);
		samplingRates = &samplingRateAllValues[0];
	}

	// create new menu item names and values
	char** newSamplingRateNames = 
		(char**)malloc(sizeof(char*) * maxSamplingRateNumber);
	u_int32_t* newSamplingRateValues =
		(u_int32_t*)malloc(sizeof(u_int32_t) * maxSamplingRateNumber);

	u_int8_t i;
	u_int8_t newSamplingRateNumber = 0;

	for (i = 0; i < maxSamplingRateNumber; i++) {

		// MP3 can't use all the possible sampling rates
		if (encodingIndex == 0) {
			// skip the ones it can't handle
			// MP3 can't handle anything less than 8000
			// LAME MP3 encoder has additional lower bound at 16000
			if (samplingRates[i] < 16000 || samplingRates[i] > 48000) {
				continue;
			}
		}

		char buf[64];
		snprintf(buf, sizeof(buf), "%u", samplingRates[i]);
		newSamplingRateNames[newSamplingRateNumber] = 
			stralloc(buf);

		newSamplingRateValues[newSamplingRateNumber] = 
			samplingRates[i];

		if (oldSamplingRate == newSamplingRateValues[newSamplingRateNumber]) {
			samplingRateIndex = newSamplingRateNumber;
		}

		newSamplingRateNumber++;
	}

	if (samplingRateIndex >= newSamplingRateNumber) {
		samplingRateIndex = newSamplingRateNumber - 1; 
	}

	// (re)create the menu
	sampling_rate_menu = CreateOptionMenu(
		sampling_rate_menu,
		newSamplingRateNames, 
		newSamplingRateNumber,
		samplingRateIndex,
		GTK_SIGNAL_FUNC(on_sampling_rate_menu_activate));

	// free up old names
	for (i = 0; i < samplingRateNumber; i++) {
		free(samplingRateNames[i]);
	}
	free(samplingRateNames);
	samplingRateNames = newSamplingRateNames;
	samplingRateValues = newSamplingRateValues;
	samplingRateNumber = newSamplingRateNumber;
}

static void SetSamplingRate(u_int32_t samplingRate)
{
	u_int8_t i;
	for (i = 0; i < samplingRateNumber; i++) {
		if (samplingRate == samplingRateValues[i]) {
			samplingRateIndex = i;
			break;
		}
	}
	if (i == samplingRateNumber) {
		debug_message("invalid sampling rate %u\n", samplingRate);
	}

	gtk_option_menu_set_history(
		GTK_OPTION_MENU(sampling_rate_menu), samplingRateIndex);
}

void CreateBitRateMenu()
{
	u_int8_t i;
	u_int16_t oldBitRate = bitRateValues[bitRateIndex];
	u_int32_t samplingRate = samplingRateValues[samplingRateIndex];

	// free up old names
	for (i = 0; i < bitRateNumber; i++) {
		free(bitRateNames[i]);
		bitRateNames[i] = NULL;
	}
	bitRateNumber = 0;
	
	// make current bitrate index invalid, will fixup below
	bitRateIndex = 255;

	// for all possible bitrates
	for (i = 0; i < bitRateAllNumber; i++) {

		// MP3 can't use all the possible bit rates
		// LAME imposes additional constraints
		if (encodingIndex == 0) {
			if (samplingRate >= 32000) {
				// MPEG-1

				if (bitRateAllValues[i] < 40
				  || bitRateAllValues[i] == 144) {
					continue;
				}
				if (samplingRate >= 44100 && bitRateAllValues[i] < 56) {
					continue;
				}
				if (samplingRate >= 48000 && bitRateAllValues[i] < 64) {
					continue;
				}

			} else {
				// MPEG-2 or MPEG-2.5

				if (samplingRate > 16000) {
					if (bitRateAllValues[i] < 32) {
						continue;
					}
				}

				if (bitRateAllValues[i] > 160) {
					continue;
				}
			}
		}

		char buf[64];
		snprintf(buf, sizeof(buf), "%u",
			bitRateAllValues[i]);
		bitRateNames[bitRateNumber] = stralloc(buf);

		bitRateValues[bitRateNumber] = bitRateAllValues[i];

		// preserve user's current choice if we can
		if (oldBitRate == bitRateValues[bitRateNumber]) {
			bitRateIndex = bitRateNumber;
		}

		bitRateNumber++;
	}

	if (bitRateIndex >= bitRateNumber) {
		bitRateIndex = bitRateNumber - 1; 
	}

	// (re)create the menu
	bit_rate_menu = CreateOptionMenu(
		bit_rate_menu,
		bitRateNames, 
		bitRateNumber,
		bitRateIndex,
		GTK_SIGNAL_FUNC(on_bit_rate_menu_activate));
}

static bool ValidateAndSave(void)
{
	// if source has been modified
	// and isn't validated, then don't proceed
	if (source_modified) {
		return false;
	}

	// copy new values to config

	MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_TYPE,
		source_type);

	MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_NAME,
		gtk_entry_get_text(GTK_ENTRY(source_entry)));

	MyConfig->UpdateFileHistory(
		gtk_entry_get_text(GTK_ENTRY(source_entry)));

	if (MyConfig->m_audioCapabilities != pAudioCaps) {
		delete MyConfig->m_audioCapabilities;
		MyConfig->m_audioCapabilities = pAudioCaps;
	}

	MyConfig->SetStringValue(CONFIG_AUDIO_INPUT_NAME,
		inputValues[inputIndex]);

	MyConfig->SetIntegerValue(CONFIG_AUDIO_SOURCE_TRACK,
		trackValues ? trackValues[trackIndex] : 0);

	MyConfig->SetIntegerValue(CONFIG_AUDIO_CHANNELS, 
		channelValues[channelIndex]);

	if (encodingIndex == 1) {
		MyConfig->SetStringValue(CONFIG_AUDIO_ENCODING, AUDIO_ENCODING_AAC);
		MyConfig->SetStringValue(CONFIG_AUDIO_ENCODER, AUDIO_ENCODER_FAAC);
	} else {
		MyConfig->SetStringValue(CONFIG_AUDIO_ENCODING, AUDIO_ENCODING_MP3);
		MyConfig->SetStringValue(CONFIG_AUDIO_ENCODER, AUDIO_ENCODER_LAME);
	}

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
	GtkWidget* hbox2;
	GtkWidget* label;
	GtkWidget* button;

	pAudioCaps = MyConfig->m_audioCapabilities;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Audio Settings");

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
		FALSE, FALSE, 5);

	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	label = gtk_label_new(" Source:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	input_label = gtk_label_new("   Input Port:");
	gtk_misc_set_alignment(GTK_MISC(input_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), input_label, FALSE, FALSE, 0);

	track_label = gtk_label_new("   Track:");
	gtk_misc_set_alignment(GTK_MISC(track_label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), track_label, FALSE, FALSE, 0);

	label = gtk_label_new(" Output:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Encoding :");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Channels:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	label = gtk_label_new("   Sampling Rate (Hz):");
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

	// source entry
	char* type = MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE);
	if (!strcasecmp(type, AUDIO_SOURCE_OSS)) {
		source_type = AUDIO_SOURCE_OSS;
	} else if (!strcasecmp(type, FILE_SOURCE_MP4)) {
		source_type = FILE_SOURCE_MP4;
	} else if (!strcasecmp(type, FILE_SOURCE_MPEG2)) {
		source_type = FILE_SOURCE_MPEG2;
	} else {
		source_type = "";
	}

	// source entry
	source_modified = false;

	source_combo = CreateFileCombo(
		MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));

	source_entry = GTK_COMBO(source_combo)->entry;

	SetEntryValidator(GTK_OBJECT(source_entry),
		GTK_SIGNAL_FUNC(on_source_changed),
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

	// input menu
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

	// track menu
	track_menu = NULL;
	track_menu = CreateTrackMenu(
		track_menu,
		'A',
		gtk_entry_get_text(GTK_ENTRY(source_entry)),
		&trackIndex,
		&trackNumber,
		&trackValues);

	trackIndex = 0; 
	for (u_int8_t i = 0; i < trackNumber; i++) {
		if (MyConfig->GetIntegerValue(CONFIG_AUDIO_SOURCE_TRACK)
		   == trackValues[i]) {
			trackIndex = i;
			break;
		}
	}
	gtk_box_pack_start(GTK_BOX(vbox), track_menu, FALSE, FALSE, 0);

	// spacer
	label = gtk_label_new(" ");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	if (!strcasecmp(MyConfig->GetStringValue(CONFIG_AUDIO_ENCODING),
	  AUDIO_ENCODING_AAC)) {
		encodingIndex = 1;
	} else {
		encodingIndex = 0;
	}
	encoding_menu = CreateOptionMenu (NULL,
		encodingNames, 
		sizeof(encodingNames) / sizeof(char*),
		encodingIndex,
		GTK_SIGNAL_FUNC(on_encoding_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), encoding_menu, TRUE, TRUE, 0);

	// channel menu
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

	sampling_rate_menu = NULL;
	CreateSamplingRateMenu(pAudioCaps);
	gtk_box_pack_start(GTK_BOX(vbox), sampling_rate_menu, TRUE, TRUE, 0);

	// set sampling rate value based on MyConfig
	SetSamplingRate(MyConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

	bit_rate_menu = NULL;
	CreateBitRateMenu();
	gtk_box_pack_start(GTK_BOX(vbox), bit_rate_menu, TRUE, TRUE, 0);

	// set bit rate value based on MyConfig
	for (u_int8_t i = 0; i < bitRateNumber; i++) {
		if (MyConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE)
		  == bitRateValues[i]) {
			bitRateIndex = i;
			break;
		}
	}
	gtk_option_menu_set_history(
		GTK_OPTION_MENU(bit_rate_menu), bitRateIndex);

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

/* end audio_dialog.cpp */
