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
#include "profile_audio.h"
#include "audio_encoder.h"
#include "support.h"

static const char **encodingNames;
static GtkWidget *AudioProfileDialog = NULL;
static const char** samplingRateNames = NULL;
static u_int32_t* samplingRateValues = NULL;
static u_int32_t samplingRateNumber = 0;	// how many sampling rates

static const char* channelNames[] = {
	"1 - Mono", "2 - Stereo"
};
static u_int32_t *bitRateValues;
static const char **bitRateNames;
static u_int32_t bitRateNumber = 0; // how many bit rates
#if 0
static GtkWidget *dialog = NULL;

static const char* source_type;
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

static const char* inputValues[] = {
	"cd", "line", "mic", "mix"
};
static const char* inputNames[] = {
	"CD", "Line In", "Microphone", "Via Mixer"
};
static u_int8_t inputIndex;

static u_int32_t trackIndex;
static u_int32_t trackNumber;	// how many tracks total
static u_int32_t* trackValues = NULL;

static u_int8_t channelValues[] = {
	1, 2
};
static u_int8_t channelIndex;

static const char **encodingNames;
static u_int32_t encodingIndex;

static u_int32_t samplingRateIndex;

static u_int32_t bitRateIndex;

// forward function declarations
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
	CreateBitRateMenu(bitRateValues == NULL ? 
			  0 : bitRateValues[bitRateIndex]);
}

static void ChangeSource()
{
	const gchar* new_source_name =
		gtk_entry_get_text(GTK_ENTRY(source_entry));

	if (!strcmp(new_source_name, source_name)) {
		source_modified = false;
		return;
	}

	CHECK_AND_FREE(source_name);
	source_name = strdup(new_source_name);

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
	inputIndex = GPOINTER_TO_UINT(data) & 0xFF;
}

static void on_channel_menu_activate (GtkWidget *widget, gpointer data)
{
	channelIndex = GPOINTER_TO_UINT(data) & 0xFF;
	CreateBitRateMenu(bitRateValues[bitRateIndex]);
}

#endif

static uint GetAudioEncoderIndex (void)
{
  GtkWidget *encoder_widget = lookup_widget(AudioProfileDialog, 
					    "AudioProfileEncoder");
  return gtk_option_menu_get_history(GTK_OPTION_MENU(encoder_widget));
}
  
static void CreateChannelMenu (GtkWidget *menu = NULL,
			       bool haveChans = false,
			       uint chans = 0)
{
  audio_encoder_table_t *aenct;
  
  aenct = audio_encoder_table[GetAudioEncoderIndex()];
	
  if (menu == NULL) {
    menu = lookup_widget(AudioProfileDialog, "ChannelMenu");
  }
  
  uint channelIndex = haveChans ? chans - 1 : 
    gtk_option_menu_get_history(GTK_OPTION_MENU(menu));

  if (channelIndex + 1 > aenct->max_channels) {
    channelIndex = 0;
  } 
  
  CreateOptionMenu(menu,
		   channelNames, 
		   aenct->max_channels,
		   channelIndex);
}

static void CreateSamplingRateMenu(CAudioCapabilities* pNewAudioCaps,
				   bool haveSrate = false,
				   uint32_t oldSamplingRate = 0)
{
	// remember current sampling rate
  uint32_t ix;
  uint samplingRateIndex;
  uint encoderIndex = GetAudioEncoderIndex();
  GtkWidget *menu = lookup_widget(AudioProfileDialog, "SampleRateMenu");
  if (haveSrate == false) {
    samplingRateIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(menu));
    oldSamplingRate = samplingRateValues[samplingRateIndex];
  }     
      
  
  if (samplingRateNames != NULL) {
    for (ix = 0; ix < samplingRateNumber; ix++) {
      free((void *)samplingRateNames[ix]);
    }
    free(samplingRateNames);
    free(samplingRateValues);
  }
  audio_encoder_table_t *aenct;
	
  aenct = audio_encoder_table[encoderIndex];
  samplingRateNumber = aenct->num_sample_rates;

  samplingRateNames = (const char **)malloc(samplingRateNumber * sizeof(char *));
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
      samplingRateNames[newSampleRates] = strdup(buffer);
      if (oldSamplingRate == aenct->sample_rates[ix]) {
	samplingRateIndex = newSampleRates;
      }
      newSampleRates++;
    }
  }
  
  CreateOptionMenu(menu,
		   samplingRateNames, 
		   newSampleRates,
		   samplingRateIndex);

  samplingRateNumber = newSampleRates;
}

static void CreateBitRateMenu(GtkWidget *menu = NULL,
			      bool haverate = false,
			      uint32_t oldBitRate = 0)
{
  u_int8_t i;
  GtkWidget *temp;
  temp = lookup_widget(AudioProfileDialog, "SampleRateMenu");
  u_int32_t samplingRate = 
    samplingRateValues[gtk_option_menu_get_history(GTK_OPTION_MENU(temp))];

  if (menu == NULL) {
    menu = lookup_widget(AudioProfileDialog, "AudioBitRateMenu");
  }
  if (haverate == false) {
    i = gtk_option_menu_get_history(GTK_OPTION_MENU(menu));
    oldBitRate = bitRateValues[i];
  }
  // free up old names
  if (bitRateNames != NULL) {
    for (i = 0; i < bitRateNumber; i++) {
      free((void *)bitRateNames[i]);
      bitRateNames[i] = NULL;
    }
    free(bitRateNames);
  }
  bitRateNumber = 0;
	
  audio_encoder_table_t *aenct;
	
  aenct = audio_encoder_table[GetAudioEncoderIndex()];
  temp = lookup_widget(AudioProfileDialog, "ChannelMenu");
  uint channelIndex = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));
  bitRateValues = aenct->bitrates_for_samplerate(samplingRate, 
						 channelIndex + 1,
						 &bitRateNumber);

  // make current bitrate index invalid, will fixup below
  bitRateNames = (const char **)malloc(sizeof(char *) * bitRateNumber);

  uint bitRateIndex = 0;
  for (uint32_t ix = 0; ix < bitRateNumber; ix++) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%u",
	     bitRateValues[ix]);
    bitRateNames[ix] = strdup(buf);
    
    // preserve user's current choice if we can
    if (oldBitRate == bitRateValues[ix]) {
      bitRateIndex = ix;
    }
  }

	// (re)create the menu
  CreateOptionMenu(menu,
		   bitRateNames, 
		   bitRateNumber,
		   bitRateIndex);
}

#if 0
void CreateAudioDialog (void) 
{
	GtkWidget* hbox;
	GtkWidget* vbox;
	GtkWidget* hbox2;
	GtkWidget* label;
	GtkWidget* button;
	const char *audioEncoder;

	SDL_LockMutex(dialog_mutex);
	if (dialog != NULL) {
	  SDL_UnlockMutex(dialog_mutex);
	  return;
	}
	SDL_UnlockMutex(dialog_mutex);
	pAudioCaps = MyConfig->m_audioCapabilities;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Audio Settings");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

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
		strdup(MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));

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
				       inputIndex);
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

	encodingNames = (const char **)malloc(sizeof(char *) * audio_encoder_table_size);
	audioEncoder = 
	  MyConfig->GetStringValue(CONFIG_AUDIO_ENCODER);
	for (uint32_t ix = 0; ix < audio_encoder_table_size; ix++) {
	  encodingNames[ix] = strdup(audio_encoder_table[ix]->dialog_selection_name);
	  if ((strcasecmp(audioEncoder, 
			 audio_encoder_table[ix]->audio_encoder) == 0) &&
	      (strcasecmp(MyConfig->GetStringValue(CONFIG_AUDIO_ENCODING), 
			  audio_encoder_table[ix]->audio_encoding) == 0)) {
	    encodingIndex = ix;
	  }
	}
	encoding_menu = CreateOptionMenu (NULL,
		encodingNames, 
					  audio_encoder_table_size,
					  encodingIndex);
	gtk_box_pack_start(GTK_BOX(vbox), encoding_menu, TRUE, TRUE, 0);

	// channel menu
	channel_menu = NULL;
	CreateChannelMenu(MyConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS));
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
#endif

static void on_SamplingRateMenu_changed (GtkOptionMenu *optionmenu,
					 gpointer user_data)
{
  CreateBitRateMenu();
}

void
on_AudioProfileEncoder_changed         (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  GtkWidget *temp;
  temp = lookup_widget(AudioProfileDialog, "Mp3Use14");
  audio_encoder_table_t *aenct;
	
  aenct = audio_encoder_table[GetAudioEncoderIndex()];
  gtk_widget_set_sensitive(temp,
			   strcmp(aenct->audio_encoding,
				  AUDIO_ENCODING_MP3) == 0);

  CreateChannelMenu();
  CreateSamplingRateMenu(MyConfig->m_audioCapabilities);
  CreateBitRateMenu();
}

// This will create a new audio profile (if it doesn't already
// exist).
static bool CreateNewProfile (GtkWidget *dialog,
			      CAudioProfile *profile)
{
  GtkWidget *temp;
  temp = lookup_widget(dialog, "AudioProfileName");
  const char *name = gtk_entry_get_text(GTK_ENTRY(temp));
  if (name == NULL || *name == '\0') return false;
  profile->SetConfigName(name);
  if (AVFlow->m_audio_profile_list->FindProfile(name) != NULL) {
    return false;
  }
  AVFlow->m_audio_profile_list->AddEntryToList(profile);
  return true;
}

// This is called when the audio profile dialog is changed
static void 
on_AudioProfileDialog_response (GtkWidget *dialog,
				gint response_id,
				gpointer user_data)
{
  CAudioProfile *profile = (CAudioProfile *)user_data;
  CAudioProfile *pass_profile = NULL;
  GtkWidget *temp;
  bool do_copy = true;
  if (response_id == GTK_RESPONSE_OK) {
    // we had created a profile with no attachments if we started as Add
    if (profile->GetName() == NULL) {
      do_copy = CreateNewProfile(dialog, profile);
      if (do_copy == false) {
	delete profile;
      } else {
	pass_profile = profile;
      }
    }

    if (do_copy) {
      // read the settings out fo the dialog

      temp = lookup_widget(dialog, "AudioProfileEncoder");
      uint index = gtk_option_menu_get_history(GTK_OPTION_MENU(temp));

      profile->SetStringValue(CFG_AUDIO_ENCODER,
			      audio_encoder_table[index]->audio_encoder);
      profile->SetStringValue(CFG_AUDIO_ENCODING,
			      audio_encoder_table[index]->audio_encoding);
      temp = lookup_widget(dialog, "ChannelMenu");
      profile->SetIntegerValue(CFG_AUDIO_CHANNELS, 
			       gtk_option_menu_get_history(GTK_OPTION_MENU(temp)) + 1);
      temp = lookup_widget(dialog, "AudioBitRateMenu");
      profile->SetIntegerValue(CFG_AUDIO_BIT_RATE,
			       bitRateValues[gtk_option_menu_get_history(GTK_OPTION_MENU(temp))]);
      temp = lookup_widget(dialog, "SampleRateMenu");
      profile->SetIntegerValue(CFG_AUDIO_SAMPLE_RATE,
			       samplingRateValues[gtk_option_menu_get_history(GTK_OPTION_MENU(temp))]);

      temp = lookup_widget(dialog, "Mp3Use14");
      profile->SetBoolValue(CFG_RTP_USE_MP3_PAYLOAD_14,
			    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp)));
      profile->WriteDefaultFile(); // set up profile
    } 
  } else {
    if (profile->GetName() == NULL) {
      delete profile;
    }
  }
  if (pass_profile == NULL) {
    debug_message("No pass profile");
  } else {
    debug_message("pass profile %s", pass_profile->GetName());
  }
  // tell the mainwindow what we've done.  If they said "Add", we select
  // the new profile.
  OnAudioProfileFinished(pass_profile);
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

void CreateAudioProfileDialog (CAudioProfile *profile)
{
  GtkWidget *dialog_vbox5;
  GtkWidget *AudioProfileTable;
  GtkWidget *label96;
  GtkWidget *ChannelMenu;
  GtkWidget *SampleRateMenu;
  GtkWidget *AudioBitRateMenu;
  GtkWidget *Mp3Use14;
  GtkWidget *button16;
  GtkWidget *alignment19;
  GtkWidget *hbox73;
  GtkWidget *image19;
  GtkWidget *label146;
  GtkWidget *AudioProfileEncoder;
  GtkWidget *label98;
  GtkWidget *label99;
  GtkWidget *label100;
  GtkWidget *label101;
  GtkWidget *label102;
  GtkWidget *label103;
  GtkWidget *AudioProfileName;
  GtkWidget *dialog_action_area4;
  GtkWidget *cancelbutton4;
  GtkWidget *AudioProfileOk;
  GtkTooltips *tooltips;
  bool new_profile = profile == NULL;

  if (new_profile) {
    profile = new CAudioProfile(NULL, NULL);
    profile->LoadConfigVariables();
    profile->Initialize(false);
  }

  tooltips = gtk_tooltips_new();

  AudioProfileDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(AudioProfileDialog), _("Audio Profile Settings"));
  gtk_window_set_modal(GTK_WINDOW(AudioProfileDialog), TRUE);

  dialog_vbox5 = GTK_DIALOG(AudioProfileDialog)->vbox;
  gtk_widget_show(dialog_vbox5);

  AudioProfileTable = gtk_table_new(7, 2, FALSE);
  gtk_widget_show(AudioProfileTable);
  gtk_box_pack_start(GTK_BOX(dialog_vbox5), AudioProfileTable, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(AudioProfileTable), 3);

  label96 = gtk_label_new(_("Audio Profile"));
  gtk_widget_show(label96);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label96, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 8);
  gtk_misc_set_alignment(GTK_MISC(label96), 0, 0.5);

  ChannelMenu = gtk_option_menu_new();
  gtk_widget_show(ChannelMenu);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), ChannelMenu, 1, 2, 2, 3,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, ChannelMenu, _("Number of Channels"), NULL);

  SampleRateMenu = gtk_option_menu_new();
  gtk_widget_show(SampleRateMenu);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), SampleRateMenu, 1, 2, 3, 4,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, SampleRateMenu, _("Output Sample Rate"), NULL);

  AudioBitRateMenu = gtk_option_menu_new();
  gtk_widget_show(AudioBitRateMenu);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), AudioBitRateMenu, 1, 2, 4, 5,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, AudioBitRateMenu, _("Output Bit Rate"), NULL);

  Mp3Use14 = gtk_check_button_new_with_mnemonic(_("MP3 RTP payload 14"));
  gtk_widget_show(Mp3Use14);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), Mp3Use14, 1, 2, 5, 6,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_widget_set_sensitive(Mp3Use14, FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Mp3Use14),
			       profile->GetBoolValue(CFG_RTP_USE_MP3_PAYLOAD_14));
  gtk_tooltips_set_tip(tooltips, Mp3Use14, _("Transmit MP3 using RFC-2250"), NULL);

  button16 = gtk_button_new();
  gtk_widget_show(button16);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), button16, 1, 2, 6, 7,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_widget_set_sensitive(button16, FALSE);
  gtk_tooltips_set_tip(tooltips, button16, _("Audio Encoder Specific Settings"), NULL);

  alignment19 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment19);
  gtk_container_add(GTK_CONTAINER(button16), alignment19);

  hbox73 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox73);
  gtk_container_add(GTK_CONTAINER(alignment19), hbox73);

  image19 = gtk_image_new_from_stock("gtk-preferences", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image19);
  gtk_box_pack_start(GTK_BOX(hbox73), image19, FALSE, FALSE, 0);

  label146 = gtk_label_new_with_mnemonic(_("Settings"));
  gtk_widget_show(label146);
  gtk_box_pack_start(GTK_BOX(hbox73), label146, FALSE, FALSE, 0);

  AudioProfileEncoder = gtk_option_menu_new();
  gtk_widget_show(AudioProfileEncoder);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), AudioProfileEncoder, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, AudioProfileEncoder, _("Audio Encoder/Encoding"), NULL);

  //  uint encoderIndex = 0;
  //uint i;
  encodingNames = (const char **)malloc(sizeof(char *) * audio_encoder_table_size);
  const char *audioEncoder =
    profile->GetStringValue(CFG_AUDIO_ENCODER);
  uint table_index = 0;
  for (uint32_t ix = 0; ix < audio_encoder_table_size; ix++) {
    encodingNames[ix] = strdup(audio_encoder_table[ix]->dialog_selection_name);
    debug_message("%s %s cmp %s %s", 
		  audioEncoder, profile->GetStringValue(CFG_AUDIO_ENCODING),
		  audio_encoder_table[ix]->audio_encoder, 
		  audio_encoder_table[ix]->audio_encoding);
    if ((strcasecmp(audioEncoder, 
		    audio_encoder_table[ix]->audio_encoder) == 0) &&
	(strcasecmp(profile->GetStringValue(CFG_AUDIO_ENCODING), 
		    audio_encoder_table[ix]->audio_encoding) == 0)) {
      table_index = ix;
    }
  }
  CreateOptionMenu(AudioProfileEncoder,
		   encodingNames, 
		   audio_encoder_table_size,
		   table_index);


  label98 = gtk_label_new(_("Encoding:"));
  gtk_widget_show(label98);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label98, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label98), 0, 0.5);

  label99 = gtk_label_new(_("Channels:"));
  gtk_widget_show(label99);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label99, 0, 1, 2, 3,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label99), 0, 0.5);

  label100 = gtk_label_new(_("Sample Rate(Hz):"));
  gtk_widget_show(label100);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label100, 0, 1, 3, 4,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label100), 0, 0.5);

  label101 = gtk_label_new(_("Bit Rate(bps):"));
  gtk_widget_show(label101);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label101, 0, 1, 4, 5,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label101), 0, 0.5);

  label102 = gtk_label_new("");
  gtk_widget_show(label102);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label102, 0, 1, 5, 6,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label102), 0, 0.5);

  label103 = gtk_label_new(_("Encoder Settings:"));
  gtk_widget_show(label103);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label103, 0, 1, 6, 7,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label103), 0, 0.5);

  AudioProfileName = gtk_entry_new();
  gtk_widget_show(AudioProfileName);
  if (new_profile == false) {
    gtk_entry_set_text(GTK_ENTRY(AudioProfileName), profile->GetName());
    gtk_widget_set_sensitive(AudioProfileName, false);
  }

  gtk_table_attach(GTK_TABLE(AudioProfileTable), AudioProfileName, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  dialog_action_area4 = GTK_DIALOG(AudioProfileDialog)->action_area;
  gtk_widget_show(dialog_action_area4);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area4), GTK_BUTTONBOX_END);

  cancelbutton4 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton4);
  gtk_dialog_add_action_widget(GTK_DIALOG(AudioProfileDialog), cancelbutton4, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton4, GTK_CAN_DEFAULT);

  AudioProfileOk = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(AudioProfileOk);
  gtk_dialog_add_action_widget(GTK_DIALOG(AudioProfileDialog), AudioProfileOk, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(AudioProfileOk, GTK_CAN_DEFAULT);


  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(AudioProfileDialog, AudioProfileDialog, "AudioProfileDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioProfileDialog, dialog_vbox5, "dialog_vbox5");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioProfileTable, "AudioProfileTable");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label96, "label96");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, ChannelMenu, "ChannelMenu");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, SampleRateMenu, "SampleRateMenu");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioBitRateMenu, "AudioBitRateMenu");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, Mp3Use14, "Mp3Use14");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, button16, "button16");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, alignment19, "alignment19");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, hbox73, "hbox73");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, image19, "image19");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label146, "label146");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioProfileEncoder, "AudioProfileEncoder");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label98, "label98");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label99, "label99");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label100, "label100");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label101, "label101");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label102, "label102");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label103, "label103");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioProfileName, "AudioProfileName");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioProfileDialog, dialog_action_area4, "dialog_action_area4");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, cancelbutton4, "cancelbutton4");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioProfileOk, "AudioProfileOk");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioProfileDialog, tooltips, "tooltips");

  if (strcmp(profile->GetStringValue(CFG_AUDIO_ENCODING), 
	     AUDIO_ENCODING_MP3) == 0) {
    gtk_widget_set_sensitive(Mp3Use14, true);
  }
  CreateChannelMenu(ChannelMenu, true, 
		    profile->GetIntegerValue(CFG_AUDIO_CHANNELS));
  CreateSamplingRateMenu(MyConfig->m_audioCapabilities, true, 
			 profile->GetIntegerValue(CFG_AUDIO_SAMPLE_RATE));
  CreateBitRateMenu(AudioBitRateMenu, true,
		    profile->GetIntegerValue(CFG_AUDIO_BIT_RATE));
  g_signal_connect((gpointer) AudioProfileDialog, "response",
		   G_CALLBACK(on_AudioProfileDialog_response),
		   profile);
  g_signal_connect((gpointer) AudioProfileEncoder, "changed",
		   G_CALLBACK(on_AudioProfileEncoder_changed),
		   NULL);
  g_signal_connect((gpointer) SampleRateMenu, "changed",
		   G_CALLBACK(on_SamplingRateMenu_changed), 
		   NULL);
  g_signal_connect((gpointer) ChannelMenu, "changed",
		   G_CALLBACK(on_SamplingRateMenu_changed), 
		   NULL);
  gtk_widget_show(AudioProfileDialog);
}


/* end audio_dialog.cpp */
