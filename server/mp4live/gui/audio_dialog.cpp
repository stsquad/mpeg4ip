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
static char* source_name;
static GtkWidget *source_combo;
static GtkWidget *source_entry;
static GtkWidget *source_list;
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

static char **encodingNames;
static u_int32_t encodingIndex;

static char** samplingRateNames = NULL;
static u_int32_t* samplingRateValues = NULL;
static u_int32_t samplingRateIndex;
static u_int32_t samplingRateNumber = 0;	// how many sampling rates

static u_int32_t *bitRateValues;
static char **bitRateNames;
static u_int32_t bitRateIndex;
static u_int32_t bitRateNumber = 0; // how many bit rates

// forward function declarations
static void CreateSamplingRateMenu(CAudioCapabilities* pNewAudioCaps);
static void CreateChannelMenu (uint32_t oldChannelNo);
static void CreateBitRateMenu(uint32_t oldBitRate);
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
	const gchar *newSourceName =
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

	CreateChannelMenu(channelValues[channelIndex]);
	// change bit rate menu
	CreateBitRateMenu(bitRateValues[bitRateIndex]);
}

static void ChangeSource()
{
	const gchar* new_source_name =
		gtk_entry_get_text(GTK_ENTRY(source_entry));

	if (!strcmp(new_source_name, source_name)) {
		source_modified = false;
		return;
	}

	free(source_name);
	source_name = stralloc(new_source_name);

	if (SourceIsDevice()) {
		source_type = AUDIO_SOURCE_OSS;
	
		SourceOssDevice();

	} else {
		if (pAudioCaps != MyConfig->m_audioCapabilities) {
			delete pAudioCaps;
		}
		pAudioCaps = NULL;

		if (IsUrl(source_name)) {
			source_type = URL_SOURCE;
		} else {
			if (access(source_name, R_OK) != 0) {
				ShowMessage("Change Audio Source",
					"Specified audio source can't be opened, check name");
			}
			source_type = FILE_SOURCE;
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

static void on_input_menu_activate (GtkWidget *widget, gpointer data)
{
	inputIndex = (unsigned int)data & 0xFF;
}

static void on_channel_menu_activate (GtkWidget *widget, gpointer data)
{
	channelIndex = (unsigned int)data & 0xFF;
	CreateBitRateMenu(bitRateValues[bitRateIndex]);
}

static void on_encoding_menu_activate (GtkWidget *widget, gpointer data)
{
	encodingIndex = (unsigned int)data & 0xFF;

	CreateSamplingRateMenu(pAudioCaps);
	CreateChannelMenu(channelValues[channelIndex]);
	CreateBitRateMenu(bitRateValues[bitRateIndex]);
}

static void on_sampling_rate_menu_activate (GtkWidget *widget, gpointer data)
{
	samplingRateIndex = (unsigned int)data & 0xFF;

	CreateBitRateMenu(bitRateValues[bitRateIndex]);
}

static void on_bit_rate_menu_activate (GtkWidget *widget, gpointer data)
{
	bitRateIndex = (unsigned int)data & 0xFF;
}

void CreateChannelMenu (uint32_t oldChannelNo)
{

  audio_encoder_table_t *aenct;
  
  aenct = audio_encoder_table[encodingIndex];
	
  if (oldChannelNo > aenct->max_channels) {
    channelIndex = 0;
  } else {
    channelIndex = oldChannelNo - 1;
  }
  channel_menu = CreateOptionMenu(channel_menu,
				  channelNames, 
				  aenct->max_channels,
				  channelIndex,
				  GTK_SIGNAL_FUNC(on_channel_menu_activate));
}

void CreateSamplingRateMenu(CAudioCapabilities* pNewAudioCaps)
{
	// remember current sampling rate
	u_int32_t oldSamplingRate = 0;
	uint32_t ix;
	if (samplingRateValues) {
		oldSamplingRate = samplingRateValues[samplingRateIndex];
	}

	if (samplingRateNames != NULL) {
	  for (ix = 0; ix < samplingRateNumber; ix++) {
	    free(samplingRateNames[ix]);
	  }
	  free(samplingRateNames);
	  free(samplingRateValues);
	}
	audio_encoder_table_t *aenct;
	
	aenct = audio_encoder_table[encodingIndex];
	samplingRateNumber = aenct->num_sample_rates;

	samplingRateNames = (char **)malloc(samplingRateNumber * sizeof(char *));
	samplingRateValues = (uint32_t *)malloc(samplingRateNumber * sizeof(uint32_t));

	uint32_t newSampleRates = 0;
	const uint32_t *checkSampleRates;
	uint32_t checkSampleRatesSize;

	if (pNewAudioCaps == NULL) {
	  checkSampleRates = allSampleRateTable;
	  checkSampleRatesSize = allSampleRateTableSize;
	} else {
	  checkSampleRates = pNewAudioCaps->m_samplingRates;
	  checkSampleRatesSize = pNewAudioCaps->m_numSamplingRates;
	}
	samplingRateIndex = 0; // start with default
	for (ix = 0; ix < aenct->num_sample_rates; ix++) {
	  bool found = false;
	  for (uint32_t iy = 0; 
	       found == false && iy < checkSampleRatesSize;
	       iy++) {
	    if (aenct->sample_rates[ix] == checkSampleRates[iy]) {
	      found = true;
	    }
	  }
	  if (found == true) {
	    samplingRateValues[newSampleRates] = aenct->sample_rates[ix];
	    char buffer[20];
	    sprintf(buffer, "%d", aenct->sample_rates[ix]);
	    samplingRateNames[newSampleRates] = stralloc(buffer);
	    if (oldSamplingRate == aenct->sample_rates[ix]) {
	      samplingRateIndex = newSampleRates;
	    }
	    newSampleRates++;
	  }
	}


	// (re)create the menu
	sampling_rate_menu = CreateOptionMenu(
		sampling_rate_menu,
		samplingRateNames, 
		newSampleRates,
		samplingRateIndex,
		GTK_SIGNAL_FUNC(on_sampling_rate_menu_activate));

	samplingRateNumber = newSampleRates;
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

void CreateBitRateMenu(uint32_t oldBitRate)
{
	u_int8_t i;
	u_int32_t samplingRate = samplingRateValues[samplingRateIndex];

	// free up old names
	for (i = 0; i < bitRateNumber; i++) {
		free(bitRateNames[i]);
		bitRateNames[i] = NULL;
	}
	free(bitRateNames);
	bitRateNumber = 0;
	
	audio_encoder_table_t *aenct;
	
	aenct = audio_encoder_table[encodingIndex];
	
	bitRateValues = aenct->bitrates_for_samplerate(samplingRate, 
						       channelValues[channelIndex],
						       &bitRateNumber);

	// make current bitrate index invalid, will fixup below
	bitRateNames = (char **)malloc(sizeof(char *) * bitRateNumber);

	bitRateIndex = 0;
	for (uint32_t ix = 0; ix < bitRateNumber; ix++) {
	  char buf[64];
	  snprintf(buf, sizeof(buf), "%u",
		   bitRateValues[ix]);
	  bitRateNames[ix] = stralloc(buf);

	  // preserve user's current choice if we can
	  if (oldBitRate == bitRateValues[ix]) {
	    bitRateIndex = ix;
	  }
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
	if (source_modified) {
		// validate it
		ChangeSource();

		// can't validate
		if (source_modified) {
			return false;
		}
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

	audio_encoder_table_t *aenct;
	
	aenct = audio_encoder_table[encodingIndex];
	MyConfig->SetStringValue(CONFIG_AUDIO_ENCODING, 
				 aenct->audio_encoding);
	MyConfig->SetStringValue(CONFIG_AUDIO_ENCODER, 
				 aenct->audio_encoder);

	MyConfig->SetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE, 
		samplingRateValues[samplingRateIndex]);

	MyConfig->SetIntegerValue(CONFIG_AUDIO_BIT_RATE,
		bitRateValues[bitRateIndex]);

	MyConfig->Update();

	DisplayAudioSettings();  // display settings in main window
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

void CreateAudioDialog (void) 
{
	GtkWidget* hbox;
	GtkWidget* vbox;
	GtkWidget* hbox2;
	GtkWidget* label;
	GtkWidget* button;
	const char *audioEncoder;

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

	label = gtk_label_new("   Bit Rate (bps):");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);


	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	hbox2 = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

	source_type = MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE);

	// source entry
	free(source_name);
	source_name =
		stralloc(MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));

	source_modified = false;

	source_combo = CreateFileCombo(source_name);

	source_entry = GTK_COMBO(source_combo)->entry;

	SetEntryValidator(GTK_OBJECT(source_entry),
		GTK_SIGNAL_FUNC(on_source_entry_changed),
		GTK_SIGNAL_FUNC(on_source_leave));

	source_list = GTK_COMBO(source_combo)->list;
	gtk_signal_connect(GTK_OBJECT(source_list), "select_child",
		GTK_SIGNAL_FUNC(on_source_list_changed), NULL);

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

	encodingNames = (char **)malloc(sizeof(char *) * audio_encoder_table_size);
	audioEncoder = 
	  MyConfig->GetStringValue(CONFIG_AUDIO_ENCODER);
	for (uint32_t ix = 0; ix < audio_encoder_table_size; ix++) {
	  encodingNames[ix] = strdup(audio_encoder_table[ix]->dialog_selection_name);
	  if (strcasecmp(audioEncoder, 
			 audio_encoder_table[ix]->audio_encoder) == 0) {
	    encodingIndex = ix;
	  }
	}
	encoding_menu = CreateOptionMenu (NULL,
		encodingNames, 
					  audio_encoder_table_size,
		encodingIndex,
		GTK_SIGNAL_FUNC(on_encoding_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), encoding_menu, TRUE, TRUE, 0);

	// channel menu
	channel_menu = NULL;
	CreateChannelMenu(MyConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS) - 1);
	gtk_box_pack_start(GTK_BOX(vbox), channel_menu, TRUE, TRUE, 0);

	sampling_rate_menu = NULL;
	CreateSamplingRateMenu(pAudioCaps);
	gtk_box_pack_start(GTK_BOX(vbox), sampling_rate_menu, TRUE, TRUE, 0);

	// set sampling rate value based on MyConfig
	SetSamplingRate(MyConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

	bit_rate_menu = NULL;
	CreateBitRateMenu(MyConfig->GetIntegerValue(CONFIG_AUDIO_BIT_RATE));
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
