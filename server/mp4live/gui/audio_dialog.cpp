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
#include "audio_alsa_source.h"
#include "audio_oss_source.h"
#include "profile_audio.h"
#include "audio_encoder.h"
#include "support.h"

// Source dialog variables
static const char* inputValues[] = {
	"cd", "line", "mic", "mix"
};
static const char* inputNames[] = {
	"CD", "Line In", "Microphone", "Via Mixer"
};
static const char* inputTypes[] = {
	AUDIO_SOURCE_OSS, 
#ifdef HAVE_ALSA
	AUDIO_SOURCE_ALSA
#endif
};
static bool source_modified;
static CAudioCapabilities* pAudioCaps;
static const char* source_type;

// Profile dialog variables
static const char **encodingNames;
static GtkWidget *AudioProfileDialog = NULL,
  *AudioSourceDialog = NULL;
static const char** samplingRateNames = NULL;
static u_int32_t* samplingRateValues = NULL;
static u_int32_t samplingRateNumber = 0;	// how many sampling rates

static const char* channelNames[] = {
	"1 - Mono", "2 - Stereo"
};
static u_int32_t *bitRateValues;
static const char **bitRateNames;
static u_int32_t bitRateNumber = 0; // how many bit rates

/*static bool SourceIsDevice()
{
  GtkWidget *temp;
  temp = lookup_widget(AudioSourceDialog, "AudioSourceFile");
  return IsDevice(gtk_entry_get_text(GTK_ENTRY(temp)));
}*/

#if 0
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
#endif

static void SourceDevice()
{
  GtkWidget *temp;
  temp = lookup_widget(AudioSourceDialog, "AudioSourceFile");
  const char *newSourceName =
    gtk_entry_get_text(GTK_ENTRY(temp));

  // don't probe the already open device!
#if 0
  if (!strcmp(newSourceName, 
	      MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME))) {
    return;
  }
#endif

  // probe new device
  GtkWidget* wid = lookup_widget(GTK_WIDGET(AudioSourceDialog), "AudioTypeMenu");
  const char* source_type = inputTypes[gtk_option_menu_get_history(GTK_OPTION_MENU(wid))];
  CAudioCapabilities* pNewAudioCaps;
  debug_message("trying source %s", source_type);
  pNewAudioCaps = (CAudioCapabilities *)FindAudioCapabilitiesByDevice(newSourceName);
  if (pNewAudioCaps == NULL) {
    if (!strcasecmp(source_type, AUDIO_SOURCE_OSS)) {
      pNewAudioCaps = new CAudioCapabilities(newSourceName);
#ifdef HAVE_ALSA
    } else if (!strcasecmp(source_type, AUDIO_SOURCE_ALSA)) {
      pNewAudioCaps = new CALSAAudioCapabilities(newSourceName);
#endif
    } else {
      return;
    }
  }
  
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
}

static void ChangeSource()
{
  debug_message("In ChangeSource");

  GtkWidget *wid = lookup_widget(AudioSourceDialog, "AudioSourceFile");
  const char* new_source_name = gtk_entry_get_text(GTK_ENTRY(wid));
  wid = lookup_widget(GTK_WIDGET(AudioSourceDialog), "AudioTypeMenu");
  const char* new_source_type = 
    inputTypes[gtk_option_menu_get_history(GTK_OPTION_MENU(wid))];

  if (strcmp(new_source_name, 
	     MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME)) == 0 &&
      strcmp(new_source_type,
	     MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE)) == 0) {
    return;
  }
  

  if (!strcmp(new_source_type, AUDIO_SOURCE_OSS)) {
    source_type = AUDIO_SOURCE_OSS;
    SourceDevice();
#ifdef HAVE_ALSA
  } else if (!strcmp(new_source_type, AUDIO_SOURCE_ALSA)) {
    source_type = AUDIO_SOURCE_ALSA;
    SourceDevice();
#endif
  } else {
    if (pAudioCaps != MyConfig->m_audioCapabilities) {
      delete pAudioCaps;
    }
    pAudioCaps = NULL;

    if (IsUrl(new_source_name)) {
      source_type = URL_SOURCE;
    } else {
      if (access(new_source_name, R_OK) != 0) {
        ShowMessage("Change Audio Source", "Specified audio source can't be opened, check name");
      }
      source_type = FILE_SOURCE;
    }
  }
#if 0
	track_menu = CreateTrackMenu(
		track_menu,
		'A',
		gtk_entry_get_text(GTK_ENTRY(source_entry)),
		&trackIndex,
		&trackNumber,
		&trackValues);

	ShowSourceSpecificSettings();
#endif

}

static void on_source_browse_button (GtkWidget *widget, gpointer *data)
{
  GtkWidget *wid = lookup_widget(AudioSourceDialog, "AudioSourceFile");
  FileBrowser(wid, AudioSourceDialog, GTK_SIGNAL_FUNC(ChangeSource));
}

static void on_source_entry_changed(GtkWidget *widget, gpointer *data)
{
  source_modified = true;
}

static int on_source_leave(GtkWidget *widget, gpointer *data)
{
	if (source_modified) {
		ChangeSource();
	}
	return FALSE;
}

static void
on_AudioSourceDialog_response          (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  if (response_id == GTK_RESPONSE_OK) {
    GtkWidget *wid;

    wid = lookup_widget(GTK_WIDGET(dialog), "AudioSourceFile");
    const char *source_name = gtk_entry_get_text(GTK_ENTRY(wid));
    if (strcmp(source_name, 
	       MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME)) != 0) {
      MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_NAME, source_name);
      MyConfig->SetStringValue(CONFIG_AUDIO_SOURCE_TYPE, source_type);
      if (MyConfig->m_audioCapabilities != pAudioCaps) {
        // never do this any more delete MyConfig->m_audioCapabilities;
        MyConfig->m_audioCapabilities = pAudioCaps;
	pAudioCaps->SetNext(AudioCapabilities);
	AudioCapabilities = pAudioCaps;

        pAudioCaps = NULL;
      }
    }
    wid = lookup_widget(GTK_WIDGET(dialog), "AudioPortMenu");
    MyConfig->SetStringValue(CONFIG_AUDIO_INPUT_NAME,
			     inputValues[gtk_option_menu_get_history(GTK_OPTION_MENU(wid))]);
    
    MyConfig->Update();
    MainWindowDisplaySources();
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));

}

void create_AudioSourceDialog (void)
{
  GtkWidget *dialog_vbox8;
  GtkWidget *table6;
  GtkWidget *audioTypeLabel;
  GtkWidget *label157;
  GtkWidget *label158;
  GtkWidget *hbox77;
  GtkWidget *AudioSourceFile;
  GtkWidget *AudioSourceBrowse;
  GtkWidget *alignment21;
  GtkWidget *hbox78;
  GtkWidget *image21;
  GtkWidget *label163;
  GtkWidget *AudioTypeMenu;
  GtkWidget *AudioPortMenu;
  GtkWidget *dialog_action_area7;
  GtkWidget *AudioSourceCancel;
  GtkWidget *okbutton7;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new();

  AudioSourceDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(AudioSourceDialog), _("Select Audio Source"));
  gtk_window_set_modal(GTK_WINDOW(AudioSourceDialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(AudioSourceDialog), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(AudioSourceDialog), 
			       GTK_WINDOW(MainWindow));

  dialog_vbox8 = GTK_DIALOG(AudioSourceDialog)->vbox;
  gtk_widget_show(dialog_vbox8);

  table6 = gtk_table_new(2, 2, FALSE);
  gtk_widget_show(table6);
  gtk_box_pack_start(GTK_BOX(dialog_vbox8), table6, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(table6), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table6), 9);

  audioTypeLabel = gtk_label_new(_("Type:"));
  gtk_widget_show(audioTypeLabel);
  gtk_table_attach(GTK_TABLE(table6), audioTypeLabel, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(audioTypeLabel), 0, 0.5);

  label157 = gtk_label_new(_("Source:"));
  gtk_widget_show(label157);
  gtk_table_attach(GTK_TABLE(table6), label157, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label157), 0, 0.5);

  label158 = gtk_label_new(_("Input Port:"));
  gtk_widget_show(label158);
  gtk_table_attach(GTK_TABLE(table6), label158, 0, 1, 2, 3,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label158), 0, 0.5);

  AudioTypeMenu = gtk_option_menu_new();
  gtk_widget_show(AudioTypeMenu);
  gtk_table_attach(GTK_TABLE(table6), AudioTypeMenu, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, AudioTypeMenu, _("Audio Device Type"), NULL);

  hbox77 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox77);
  gtk_table_attach(GTK_TABLE(table6), hbox77, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_FILL), 0, 0);

  AudioSourceFile = gtk_entry_new();
  gtk_widget_show(AudioSourceFile);
  gtk_box_pack_start(GTK_BOX(hbox77), AudioSourceFile, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, AudioSourceFile, _("Select Audio Device"), NULL);

  AudioSourceBrowse = gtk_button_new();
  gtk_widget_show(AudioSourceBrowse);
  gtk_box_pack_start(GTK_BOX(hbox77), AudioSourceBrowse, FALSE, FALSE, 0);

  alignment21 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment21);
  gtk_container_add(GTK_CONTAINER(AudioSourceBrowse), alignment21);

  hbox78 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox78);
  gtk_container_add(GTK_CONTAINER(alignment21), hbox78);

  image21 = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image21);
  gtk_box_pack_start(GTK_BOX(hbox78), image21, FALSE, FALSE, 0);

  label163 = gtk_label_new_with_mnemonic(_("Browse"));
  gtk_widget_show(label163);
  gtk_box_pack_start(GTK_BOX(hbox78), label163, FALSE, FALSE, 0);

  AudioPortMenu = gtk_option_menu_new();
  gtk_widget_show(AudioPortMenu);
  gtk_table_attach(GTK_TABLE(table6), AudioPortMenu, 1, 2, 2, 3,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, AudioPortMenu, _("Audio Source Device"), NULL);

  dialog_action_area7 = GTK_DIALOG(AudioSourceDialog)->action_area;
  gtk_widget_show(dialog_action_area7);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area7), GTK_BUTTONBOX_END);

  AudioSourceCancel = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(AudioSourceCancel);
  gtk_dialog_add_action_widget(GTK_DIALOG(AudioSourceDialog), AudioSourceCancel, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(AudioSourceCancel, GTK_CAN_DEFAULT);

  okbutton7 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton7);
  gtk_dialog_add_action_widget(GTK_DIALOG(AudioSourceDialog), okbutton7, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton7, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) AudioSourceDialog, "response",
                    G_CALLBACK(on_AudioSourceDialog_response),
                    NULL);
  g_signal_connect((gpointer) AudioSourceBrowse, "clicked", 
		   G_CALLBACK(on_source_browse_button),
		   NULL);
  g_signal_connect((gpointer) AudioTypeMenu, "changed",
		   G_CALLBACK(ChangeSource),
		   NULL);
  SetEntryValidator(GTK_OBJECT(AudioSourceFile),
		    GTK_SIGNAL_FUNC(on_source_entry_changed),
		    GTK_SIGNAL_FUNC(on_source_leave));

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(AudioSourceDialog, AudioSourceDialog, "AudioSourceDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioSourceDialog, dialog_vbox8, "dialog_vbox8");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, table6, "table6");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, audioTypeLabel, "audioTypeLabel");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, label157, "label157");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, label158, "label158");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, hbox77, "hbox77");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, AudioSourceFile, "AudioSourceFile");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, AudioSourceBrowse, "AudioSourceBrowse");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, alignment21, "alignment21");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, hbox78, "hbox78");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, image21, "image21");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, label163, "label163");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, AudioTypeMenu, "AudioTypeMenu");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, AudioPortMenu, "AudioPortMenu");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioSourceDialog, dialog_action_area7, "dialog_action_area7");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, AudioSourceCancel, "AudioSourceCancel");
  GLADE_HOOKUP_OBJECT(AudioSourceDialog, okbutton7, "okbutton7");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioSourceDialog, tooltips, "tooltips");

  gtk_entry_set_text(GTK_ENTRY(AudioSourceFile),
		     MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_NAME));
  uint inputIndex = 0;
  for (u_int8_t i = 0; i < sizeof(inputValues) / sizeof(u_int8_t); i++) {
    if (!strcasecmp(MyConfig->GetStringValue(CONFIG_AUDIO_INPUT_NAME),
		    inputValues[i])) {
      inputIndex = i;
      break;
    }
  }

  CreateOptionMenu(AudioPortMenu,
		   inputNames, 
		   sizeof(inputNames) / sizeof(char*),
		   inputIndex);

  source_modified = false;
  source_type = MyConfig->GetStringValue(CONFIG_AUDIO_SOURCE_TYPE);
  pAudioCaps = (CAudioCapabilities *)MyConfig->m_audioCapabilities;
  inputIndex = 0;
  for (u_int8_t i = 0; i < sizeof(inputTypes) / sizeof(u_int8_t); i++) {
    if (!strcasecmp(source_type,
		    inputTypes[i])) {
      inputIndex = i;
      break;
    }
  }
  CreateOptionMenu(AudioTypeMenu,
		   inputTypes, 
		   sizeof(inputTypes) / sizeof(char*),
		   inputIndex);

  gtk_widget_show(AudioSourceDialog);
}

/***************************************************************************
 * Audio Profile Dialog
 ***************************************************************************/
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
    for (uint32_t iy = 0; found == false && iy < checkSampleRatesSize; iy++) {
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
  if (bitRateValues != NULL && haverate == false) {
    i = gtk_option_menu_get_history(GTK_OPTION_MENU(menu));
    if (i >= bitRateNumber) i = 0;
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

static void on_SamplingRateMenu_changed (GtkOptionMenu *optionmenu,
					 gpointer user_data)
{
  CreateBitRateMenu();
}

void
on_AudioEncoderSettingsButton          (GtkButton *button,
					gpointer user_data)
{
  CAudioProfile *profile = (CAudioProfile *)user_data;
  GtkWidget *temp;
  temp = lookup_widget(AudioProfileDialog, "AudioProfileEncoder");
  encoder_gui_options_base_t **settings_array;
  uint settings_array_count;
  audio_encoder_table_t *aenct;
	
  aenct = audio_encoder_table[GetAudioEncoderIndex()];
  
  if ((aenct->get_gui_options)(&settings_array, &settings_array_count) == false) {
    return;
  }
  
  CreateEncoderSettingsDialog(profile, AudioProfileDialog,
			      aenct->dialog_selection_name,
			      settings_array,
			      settings_array_count);
}

static void
on_AudioProfileEncoder_changed         (GtkOptionMenu   *optionmenu,
                                        gpointer         user_data)
{
  GtkWidget *temp;
  audio_encoder_table_t *aenct;
	
  aenct = audio_encoder_table[GetAudioEncoderIndex()];
  CreateChannelMenu();
  CreateSamplingRateMenu((CAudioCapabilities *)MyConfig->m_audioCapabilities);
  CreateBitRateMenu();
  bool enable_settings;
  if (aenct->get_gui_options == NULL) {
    enable_settings = false;
  } else {
    enable_settings = (aenct->get_gui_options)(NULL, NULL);
  }
  temp = lookup_widget(AudioProfileDialog, "AudioEncoderSettingsButton");
  gtk_widget_set_sensitive(temp, enable_settings);
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

      profile->Update();
      profile->WriteToFile(); // set up profile
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
  GtkWidget *AudioEncoderSettingsButton;
  GtkWidget *alignment19;
  GtkWidget *hbox73;
  GtkWidget *image19;
  GtkWidget *label146;
  GtkWidget *AudioProfileEncoder;
  GtkWidget *label98;
  GtkWidget *label99;
  GtkWidget *label100;
  GtkWidget *label101;
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
  gtk_window_set_transient_for(GTK_WINDOW(AudioProfileDialog), 
			       GTK_WINDOW(MainWindow));

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

  AudioEncoderSettingsButton = gtk_button_new();
  gtk_widget_show(AudioEncoderSettingsButton);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), AudioEncoderSettingsButton, 1, 2, 5, 6,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_widget_set_sensitive(AudioEncoderSettingsButton, FALSE);
  gtk_tooltips_set_tip(tooltips, AudioEncoderSettingsButton, _("Audio Encoder Specific Settings"), NULL);

  alignment19 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment19);
  gtk_container_add(GTK_CONTAINER(AudioEncoderSettingsButton), alignment19);


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

  bool enable_settings;
  if (audio_encoder_table[table_index]->get_gui_options == NULL) {
    enable_settings = false;
  } else {
    enable_settings = (audio_encoder_table[table_index]->get_gui_options)(NULL, NULL);
  }
  gtk_widget_set_sensitive(AudioEncoderSettingsButton, enable_settings);

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

  label103 = gtk_label_new(_("Encoder Settings:"));
  gtk_widget_show(label103);
  gtk_table_attach(GTK_TABLE(AudioProfileTable), label103, 0, 1, 5, 6,
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
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioEncoderSettingsButton, "AudioEncoderSettingsButton");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, alignment19, "alignment19");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, hbox73, "hbox73");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, image19, "image19");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label146, "label146");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioProfileEncoder, "AudioProfileEncoder");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label98, "label98");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label99, "label99");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label100, "label100");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label101, "label101");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, label103, "label103");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioProfileName, "AudioProfileName");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioProfileDialog, dialog_action_area4, "dialog_action_area4");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, cancelbutton4, "cancelbutton4");
  GLADE_HOOKUP_OBJECT(AudioProfileDialog, AudioProfileOk, "AudioProfileOk");
  GLADE_HOOKUP_OBJECT_NO_REF(AudioProfileDialog, tooltips, "tooltips");

  CreateChannelMenu(ChannelMenu, true, 
		    profile->GetIntegerValue(CFG_AUDIO_CHANNELS));
  CreateSamplingRateMenu((CAudioCapabilities *)MyConfig->m_audioCapabilities, true, 
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
  g_signal_connect((gpointer)AudioEncoderSettingsButton, "clicked", 
		   G_CALLBACK(on_AudioEncoderSettingsButton),
		   profile);
  gtk_widget_show(AudioProfileDialog);
}


/* end audio_dialog.cpp */
