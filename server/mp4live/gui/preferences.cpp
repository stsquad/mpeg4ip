/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1(the "License"); you may not use this file
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
 * Copyright(C) Cisco Systems Inc. 2001-2005.  All Rights Reserved.
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
#include "mp4live_common.h"
#include "support.h"


static void on_stream_directory_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wid = GTK_WIDGET(data), *entry;
  entry = lookup_widget(wid, "StreamDirectory");
  FileBrowser(entry, wid, NULL);
}

static void on_profile_directory_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *wid = GTK_WIDGET(data), *entry;
  entry = lookup_widget(wid, "ProfileDirectory");
  FileBrowser(entry, wid, NULL);
}

static void on_raw_file_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog = GTK_WIDGET(data);
  GtkWidget *entry = lookup_widget(dialog, "RawFileName");
  FileBrowser(entry, dialog, NULL);
}
static bool check_dir (const char *newdir,
		       const char *old,
		       const char *suffix)
{
    char olddirname[PATH_MAX], newdirname[PATH_MAX];
    if (old != NULL) {
      strcpy(olddirname, old);
    } else {
      GetHomeDirectory(olddirname);
      strcat(olddirname, suffix);
    }
    char *temp = olddirname + strlen(olddirname);
    while (*temp == '/' && temp >= olddirname) {
      *temp = '\0';
      temp--;
    }
    if (temp < olddirname) return false;
    strcpy(newdirname, newdir);
    temp = newdirname + strlen(newdirname);
    while (*temp == '/' && temp >= newdirname) {
      *temp = '\0';
      temp--;
    }
    if (temp < newdirname) return false;
    if (strcmp(olddirname, newdirname) == 0) return false;
    return true;
}
static void
on_PreferencesDialog_response         (GtkWidget       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  GtkWidget *w;
  if(GTK_RESPONSE_OK == response_id) {
    w = lookup_widget(dialog, "LogLevelSpinner");
    MyConfig->SetIntegerValue(CONFIG_APP_LOGLEVEL, 
			      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w)));

    w = lookup_widget(dialog, "MtuSpinButton");
    MyConfig->SetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE,
			      gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w)));

    w = lookup_widget(dialog, "DebugCheckButton");
    MyConfig->SetBoolValue(CONFIG_APP_DEBUG, 
			   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));

    PrintDebugMessages =
      MyConfig->GetBoolValue(CONFIG_APP_DEBUG);
    w = lookup_widget(dialog, "MulticastTtlMenu");
    uint ix = gtk_option_menu_get_history(GTK_OPTION_MENU(w));
    static const uint ttls[] = { 1, 15, 64, 127};
    MyConfig->SetIntegerValue(CONFIG_RTP_MCAST_TTL, ttls[ix]);

    w = lookup_widget(dialog, "AllowRtcpCheck");
    MyConfig->SetBoolValue(CONFIG_RTP_NO_B_RR_0,
			   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
    w = lookup_widget(dialog, "AllowSSMCheck");
    MyConfig->SetBoolValue(CONFIG_RTP_USE_SSM,
			   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));


    config_integer_t value = FILE_MP4_OVERWRITE;
    w = lookup_widget(dialog, "FileStatus2");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
      value = FILE_MP4_APPEND;
    } else {
      w = lookup_widget(dialog, "FileStatus3");
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
	value = FILE_MP4_CREATE_NEW;
      }
    }
    MyConfig->SetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS, value);

    w = lookup_widget(dialog, "RecordRawDataButton");
    MyConfig->SetBoolValue(CONFIG_RECORD_RAW_IN_MP4,
			   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
    w = lookup_widget(dialog, "RawFileName");
    MyConfig->SetStringValue(CONFIG_RECORD_RAW_MP4_FILE_NAME,
			     gtk_entry_get_text(GTK_ENTRY(w)));

    w = lookup_widget(dialog, "RecordHintTrackButton");
    MyConfig->SetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS,
			   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
    w = lookup_widget(dialog, "OptimizeMP4FileButton");
    MyConfig->SetBoolValue(CONFIG_RECORD_MP4_OPTIMIZE,
			   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));

    bool have_dir_change = false;
    w = lookup_widget(dialog, "StreamDirectory");
    if (check_dir(gtk_entry_get_text(GTK_ENTRY(w)),
		  MyConfig->GetStringValue(CONFIG_APP_STREAM_DIRECTORY),
		  ".mp4live_d/Streams/")) {
      MyConfig->SetStringValue(CONFIG_APP_STREAM_DIRECTORY,
			       gtk_entry_get_text(GTK_ENTRY(w)));
      have_dir_change = true;
    }
    w = lookup_widget(dialog, "ProfileDirectory");
    if (check_dir(gtk_entry_get_text(GTK_ENTRY(w)),
		  MyConfig->GetStringValue(CONFIG_APP_PROFILE_DIRECTORY),
		  ".mp4live_d/")) {
      MyConfig->SetStringValue(CONFIG_APP_PROFILE_DIRECTORY,
			       gtk_entry_get_text(GTK_ENTRY(w)));
      have_dir_change = true;
    }
    if (have_dir_change) {
      StartFlowLoadWindow();
    }

  }
  gtk_widget_destroy(GTK_WIDGET(dialog));

}

void create_PreferencesDialog(void)
{
  GtkWidget *PreferencesDialog;
  GtkWidget *dialog_vbox9;
  GtkWidget *notebook1;
  GtkWidget *GenericTable;
  GtkWidget *label173;
  GtkObject *LogLevelSpinner_adj;
  GtkWidget *LogLevelSpinner;
  GtkWidget *DebugCheckButton;
  GtkWidget *label174;
  GtkWidget *StreamDirectoryHbox;
  GtkWidget *StreamDirectory;
  GtkWidget *StreamDirectoryButton;
  GtkWidget *alignment24;
  GtkWidget *hbox84;
  GtkWidget *image24;
  GtkWidget *label175;
  GtkWidget *label176;
  GtkWidget *ProfileDirectoryHbox;
  GtkWidget *ProfileDirectory;
  GtkWidget *ProfileDirectoryButton;
  GtkWidget *alignment25;
  GtkWidget *hbox86;
  GtkWidget *image25;
  GtkWidget *label177;
  GtkWidget *label169;
  GtkWidget *NetworkVbox;
  GtkWidget *NetworkTable;
  GtkWidget *label178;
  GtkWidget *label179;
  GtkWidget *MulticastTtlMenu;
  GtkWidget *menu16;
  GtkWidget *ttl_lan;
  GtkWidget *ttl_organization;
  GtkWidget *ttl_regional;
  GtkWidget *ttl_worldwide;
  GtkObject *MtuSpinButton_adj;
  GtkWidget *MtuSpinButton;
  GtkWidget *vbox39;
  GtkWidget *hbox89;
  GtkWidget *AllowRtcpCheck;
  GtkWidget *hbox87;
  GtkWidget *AllowSSMCheck;
  GtkWidget *label170;
  GtkWidget *MP4FileSettingsVbox;
  GtkWidget *FileStatusFrame;
  GtkWidget *FileStatusVbox;
  GtkWidget *hbox90;
  GtkWidget *FileStatus1;
  GSList *FileStatus1_group = NULL;
  GtkWidget *hbox91;
  GtkWidget *FileStatus2;
  GtkWidget *hbox92;
  GtkWidget *FileStatus3;
  GtkWidget *label180;
  GtkWidget *hbox93;
  GtkWidget *RecordHintTrackButton;
  GtkWidget *hbox94;
  GtkWidget *OptimizeMP4FileButton;
  GtkWidget *hbox95;
  GtkWidget *RecordRawDataButton;
  GtkWidget *RawFileName;
  GtkWidget *RawFileNameBrowse;
  GtkWidget *alignment26;
  GtkWidget *hbox96;
  GtkWidget *image26;
  GtkWidget *label181;
  GtkWidget *label171;
  GtkWidget *dialog_action_area8;
  GtkWidget *cancelbutton6;
  GtkWidget *okbutton8;
  GtkTooltips *tooltips;

  tooltips = gtk_tooltips_new();

  PreferencesDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(PreferencesDialog), _("Preferences"));
  gtk_window_set_modal(GTK_WINDOW(PreferencesDialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(PreferencesDialog), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(PreferencesDialog), 
			       GTK_WINDOW(MainWindow));

  dialog_vbox9 = GTK_DIALOG(PreferencesDialog)->vbox;
  gtk_widget_show(dialog_vbox9);

  notebook1 = gtk_notebook_new();
  gtk_widget_show(notebook1);
  gtk_box_pack_start(GTK_BOX(dialog_vbox9), notebook1, TRUE, TRUE, 0);

  GenericTable = gtk_table_new(4, 2, FALSE);
  gtk_widget_show(GenericTable);
  gtk_container_add(GTK_CONTAINER(notebook1), GenericTable);
  gtk_table_set_row_spacings(GTK_TABLE(GenericTable), 9);
  gtk_table_set_col_spacings(GTK_TABLE(GenericTable), 36);

  label173 = gtk_label_new(_("Log Level"));
  gtk_widget_show(label173);
  gtk_table_attach(GTK_TABLE(GenericTable), label173, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label173), 0, 0.5);

  LogLevelSpinner_adj = gtk_adjustment_new(MyConfig->GetIntegerValue(CONFIG_APP_LOGLEVEL),
					    0, 7, 1, 10, 10);
  LogLevelSpinner = gtk_spin_button_new(GTK_ADJUSTMENT(LogLevelSpinner_adj), 1, 0);
  gtk_widget_show(LogLevelSpinner);
  gtk_table_attach(GTK_TABLE(GenericTable), LogLevelSpinner, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  DebugCheckButton = gtk_check_button_new_with_mnemonic(_("Debug"));
  gtk_widget_show(DebugCheckButton);
  gtk_table_attach(GTK_TABLE(GenericTable), DebugCheckButton, 0, 2, 0, 1,
                   (GtkAttachOptions)(0),
                   (GtkAttachOptions)(0), 0, 0);

  label174 = gtk_label_new(_("Stream Directory"));
  gtk_widget_show(label174);
  gtk_table_attach(GTK_TABLE(GenericTable), label174, 0, 1, 2, 3,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label174), 0, 0.5);

  StreamDirectoryHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(StreamDirectoryHbox);
  gtk_table_attach(GTK_TABLE(GenericTable), StreamDirectoryHbox, 1, 2, 2, 3,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(GTK_FILL), 0, 0);

  StreamDirectory = gtk_entry_new();
  gtk_widget_show(StreamDirectory);
  gtk_box_pack_start(GTK_BOX(StreamDirectoryHbox), StreamDirectory, TRUE, TRUE, 0);

  StreamDirectoryButton = gtk_button_new();
  gtk_widget_show(StreamDirectoryButton);
  gtk_box_pack_start(GTK_BOX(StreamDirectoryHbox), StreamDirectoryButton, FALSE, FALSE, 0);

  alignment24 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment24);
  gtk_container_add(GTK_CONTAINER(StreamDirectoryButton), alignment24);

  hbox84 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox84);
  gtk_container_add(GTK_CONTAINER(alignment24), hbox84);

  image24 = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image24);
  gtk_box_pack_start(GTK_BOX(hbox84), image24, FALSE, FALSE, 0);

  label175 = gtk_label_new_with_mnemonic(_("Browse"));
  gtk_widget_show(label175);
  gtk_box_pack_start(GTK_BOX(hbox84), label175, FALSE, FALSE, 0);

  label176 = gtk_label_new(_("Profiles Directory"));
  gtk_widget_show(label176);
  gtk_table_attach(GTK_TABLE(GenericTable), label176, 0, 1, 3, 4,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label176), 0, 0.5);

  ProfileDirectoryHbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(ProfileDirectoryHbox);
  gtk_table_attach(GTK_TABLE(GenericTable), ProfileDirectoryHbox, 1, 2, 3, 4,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(GTK_FILL), 0, 0);

  ProfileDirectory = gtk_entry_new();
  gtk_widget_show(ProfileDirectory);
  gtk_box_pack_start(GTK_BOX(ProfileDirectoryHbox), ProfileDirectory, TRUE, TRUE, 0);

  ProfileDirectoryButton = gtk_button_new();
  gtk_widget_show(ProfileDirectoryButton);
  gtk_box_pack_start(GTK_BOX(ProfileDirectoryHbox), ProfileDirectoryButton, FALSE, FALSE, 0);

  alignment25 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment25);
  gtk_container_add(GTK_CONTAINER(ProfileDirectoryButton), alignment25);

  hbox86 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox86);
  gtk_container_add(GTK_CONTAINER(alignment25), hbox86);

  image25 = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image25);
  gtk_box_pack_start(GTK_BOX(hbox86), image25, FALSE, FALSE, 0);

  label177 = gtk_label_new_with_mnemonic(_("Browse"));
  gtk_widget_show(label177);
  gtk_box_pack_start(GTK_BOX(hbox86), label177, FALSE, FALSE, 0);

  label169 = gtk_label_new(_("Generic"));
  gtk_widget_show(label169);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook1), 0), label169);

  NetworkVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(NetworkVbox);
  gtk_container_add(GTK_CONTAINER(notebook1), NetworkVbox);

  NetworkTable = gtk_table_new(2, 2, TRUE);
  gtk_widget_show(NetworkTable);
  gtk_box_pack_start(GTK_BOX(NetworkVbox), NetworkTable, FALSE, TRUE, 6);
  gtk_table_set_row_spacings(GTK_TABLE(NetworkTable), 7);

  label178 = gtk_label_new(_("Multicast TTL"));
  gtk_widget_show(label178);
  gtk_table_attach(GTK_TABLE(NetworkTable), label178, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label178), 0, 0.5);

  label179 = gtk_label_new(_("MTU"));
  gtk_widget_show(label179);
  gtk_table_attach(GTK_TABLE(NetworkTable), label179, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label179), 0, 0.5);

  MulticastTtlMenu = gtk_option_menu_new();
  gtk_widget_show(MulticastTtlMenu);
  gtk_table_attach(GTK_TABLE(NetworkTable), MulticastTtlMenu, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  menu16 = gtk_menu_new();

  ttl_lan = gtk_menu_item_new_with_mnemonic(_("LAN - 1"));
  gtk_widget_show(ttl_lan);
  gtk_container_add(GTK_CONTAINER(menu16), ttl_lan);

  ttl_organization = gtk_menu_item_new_with_mnemonic(_("Organization - 15"));
  gtk_widget_show(ttl_organization);
  gtk_container_add(GTK_CONTAINER(menu16), ttl_organization);

  ttl_regional = gtk_menu_item_new_with_mnemonic(_("Regional - 63"));
  gtk_widget_show(ttl_regional);
  gtk_container_add(GTK_CONTAINER(menu16), ttl_regional);

  ttl_worldwide = gtk_menu_item_new_with_mnemonic(_("Worldwide -127"));
  gtk_widget_show(ttl_worldwide);
  gtk_container_add(GTK_CONTAINER(menu16), ttl_worldwide);

  gtk_option_menu_set_menu(GTK_OPTION_MENU(MulticastTtlMenu), menu16);

  MtuSpinButton_adj = 
    gtk_adjustment_new(MyConfig->GetIntegerValue(CONFIG_RTP_PAYLOAD_SIZE), 
		       256, 65535, 1, 100, 100);
  MtuSpinButton = gtk_spin_button_new(GTK_ADJUSTMENT (MtuSpinButton_adj), 1, 0);
  gtk_widget_show(MtuSpinButton);
  gtk_table_attach(GTK_TABLE (NetworkTable), MtuSpinButton, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (MtuSpinButton), TRUE);

  vbox39 = gtk_vbox_new(FALSE, 4);
  gtk_widget_show(vbox39);
  gtk_box_pack_start(GTK_BOX(NetworkVbox), vbox39, FALSE, FALSE, 5);

  hbox89 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox89);
  gtk_box_pack_start(GTK_BOX(vbox39), hbox89, TRUE, TRUE, 4);

  AllowRtcpCheck = gtk_check_button_new_with_mnemonic(_("Allow Clients to Send RTCP"));
  gtk_widget_show(AllowRtcpCheck);
  gtk_box_pack_start(GTK_BOX(hbox89), AllowRtcpCheck, FALSE, FALSE, 94);
  gtk_tooltips_set_tip(tooltips, AllowRtcpCheck, _("Allow clients to send RTCP - default off"), NULL);

  hbox87 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox87);
  gtk_box_pack_start(GTK_BOX(vbox39), hbox87, TRUE, TRUE, 0);

  AllowSSMCheck = gtk_check_button_new_with_mnemonic(_("Use Single Source Multicast"));
  gtk_widget_show(AllowSSMCheck);
  gtk_box_pack_start(GTK_BOX(hbox87), AllowSSMCheck, TRUE, FALSE, 0);
  gtk_widget_set_sensitive(AllowSSMCheck, FALSE);

  label170 = gtk_label_new(_("Network Settings"));
  gtk_widget_show(label170);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook1), 1), label170);

  MP4FileSettingsVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(MP4FileSettingsVbox);
  gtk_container_add(GTK_CONTAINER(notebook1), MP4FileSettingsVbox);
  gtk_notebook_set_tab_label_packing(GTK_NOTEBOOK(notebook1), MP4FileSettingsVbox,
                                      FALSE, FALSE, GTK_PACK_START);

  FileStatusFrame = gtk_frame_new(NULL);
  gtk_widget_show(FileStatusFrame);
  gtk_box_pack_start(GTK_BOX(MP4FileSettingsVbox), FileStatusFrame, TRUE, TRUE, 0);

  FileStatusVbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(FileStatusVbox);
  gtk_container_add(GTK_CONTAINER(FileStatusFrame), FileStatusVbox);

  hbox90 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox90);
  gtk_box_pack_start(GTK_BOX(FileStatusVbox), hbox90, TRUE, TRUE, 0);

  FileStatus1 = gtk_radio_button_new_with_mnemonic(NULL, _("Always overwrite existing file"));
  gtk_widget_show(FileStatus1);
  gtk_box_pack_start(GTK_BOX(hbox90), FileStatus1, TRUE, TRUE, 46);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(FileStatus1), FileStatus1_group);
  FileStatus1_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileStatus1));

  hbox91 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox91);
  gtk_box_pack_start(GTK_BOX(FileStatusVbox), hbox91, FALSE, FALSE, 0);

  FileStatus2 = gtk_radio_button_new_with_mnemonic(NULL, _("Append to file"));
  gtk_widget_show(FileStatus2);
  gtk_box_pack_start(GTK_BOX(hbox91), FileStatus2, TRUE, TRUE, 46);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(FileStatus2), FileStatus1_group);
  FileStatus1_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileStatus2));

  hbox92 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox92);
  gtk_box_pack_start(GTK_BOX(FileStatusVbox), hbox92, FALSE, FALSE, 0);

  FileStatus3 = gtk_radio_button_new_with_mnemonic(NULL, _("Create new file based on name"));
  gtk_widget_show(FileStatus3);
  gtk_box_pack_start(GTK_BOX(hbox92), FileStatus3, FALSE, FALSE, 46);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(FileStatus3), FileStatus1_group);
  FileStatus1_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(FileStatus3));

  label180 = gtk_label_new(_("File Status"));
  gtk_widget_show(label180);
  gtk_frame_set_label_widget(GTK_FRAME(FileStatusFrame), label180);

  hbox93 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox93);
  gtk_box_pack_start(GTK_BOX(MP4FileSettingsVbox), hbox93, TRUE, TRUE, 4);

  RecordHintTrackButton = gtk_check_button_new_with_mnemonic(_("Record Hint Tracks when Finished"));
  gtk_widget_show(RecordHintTrackButton);
  gtk_box_pack_start(GTK_BOX(hbox93), RecordHintTrackButton, TRUE, FALSE, 0);
  gtk_tooltips_set_tip(tooltips, RecordHintTrackButton, _("If set, this can cause application to appear to freeze when finished"), NULL);

  hbox94 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox94);
  gtk_box_pack_start(GTK_BOX(MP4FileSettingsVbox), hbox94, TRUE, TRUE, 4);

  OptimizeMP4FileButton = gtk_check_button_new_with_mnemonic(_("Optimize When Finished"));
  gtk_widget_show(OptimizeMP4FileButton);
  gtk_box_pack_start(GTK_BOX(hbox94), OptimizeMP4FileButton, FALSE, FALSE, 79);

  hbox95 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox95);
  gtk_box_pack_start(GTK_BOX(MP4FileSettingsVbox), hbox95, TRUE, TRUE, 0);

  RecordRawDataButton = gtk_check_button_new_with_mnemonic(_("Record Raw Data "));
  gtk_widget_show(RecordRawDataButton);
  gtk_box_pack_start(GTK_BOX(hbox95), RecordRawDataButton, FALSE, FALSE, 0);

  RawFileName = gtk_entry_new();
  gtk_widget_show(RawFileName);
  gtk_box_pack_start(GTK_BOX(hbox95), RawFileName, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, RawFileName, _("Raw Data MP4 File Name"), NULL);

  RawFileNameBrowse = gtk_button_new();
  gtk_widget_show(RawFileNameBrowse);
  gtk_box_pack_start(GTK_BOX(hbox95), RawFileNameBrowse, FALSE, FALSE, 0);

  alignment26 = gtk_alignment_new(0.5, 0.5, 0, 0);
  gtk_widget_show(alignment26);
  gtk_container_add(GTK_CONTAINER(RawFileNameBrowse), alignment26);

  hbox96 = gtk_hbox_new(FALSE, 2);
  gtk_widget_show(hbox96);
  gtk_container_add(GTK_CONTAINER(alignment26), hbox96);

  image26 = gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show(image26);
  gtk_box_pack_start(GTK_BOX(hbox96), image26, FALSE, FALSE, 0);

  label181 = gtk_label_new_with_mnemonic(_("Browse"));
  gtk_widget_show(label181);
  gtk_box_pack_start(GTK_BOX(hbox96), label181, FALSE, FALSE, 0);

  label171 = gtk_label_new(_("MP4 File Settings"));
  gtk_widget_show(label171);
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook1), gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook1), 2), label171);

  dialog_action_area8 = GTK_DIALOG(PreferencesDialog)->action_area;
  gtk_widget_show(dialog_action_area8);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area8), GTK_BUTTONBOX_END);

  cancelbutton6 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton6);
  gtk_dialog_add_action_widget(GTK_DIALOG(PreferencesDialog), cancelbutton6, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton6, GTK_CAN_DEFAULT);

  okbutton8 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton8);
  gtk_dialog_add_action_widget(GTK_DIALOG(PreferencesDialog), okbutton8, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton8, GTK_CAN_DEFAULT);

  g_signal_connect((gpointer) PreferencesDialog, "response",
                    G_CALLBACK(on_PreferencesDialog_response),
                    NULL);
  g_signal_connect((gpointer)StreamDirectoryButton, "clicked",
		   G_CALLBACK(on_stream_directory_clicked), 
		   PreferencesDialog);
  g_signal_connect((gpointer)ProfileDirectoryButton, "clicked",
		   G_CALLBACK(on_profile_directory_clicked), 
		   PreferencesDialog);

  g_signal_connect((gpointer)RawFileNameBrowse, "clicked",
		   G_CALLBACK(on_raw_file_clicked),
		   PreferencesDialog);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(PreferencesDialog, PreferencesDialog, "PreferencesDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(PreferencesDialog, dialog_vbox9, "dialog_vbox9");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, notebook1, "notebook1");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, GenericTable, "GenericTable");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label173, "label173");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, LogLevelSpinner, "LogLevelSpinner");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, DebugCheckButton, "DebugCheckButton");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label174, "label174");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, StreamDirectoryHbox, "StreamDirectoryHbox");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, StreamDirectory, "StreamDirectory");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, StreamDirectoryButton, "StreamDirectoryButton");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, alignment24, "alignment24");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox84, "hbox84");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, image24, "image24");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label175, "label175");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label176, "label176");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, ProfileDirectoryHbox, "ProfileDirectoryHbox");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, ProfileDirectory, "ProfileDirectory");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, ProfileDirectoryButton, "ProfileDirectoryButton");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, alignment25, "alignment25");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox86, "hbox86");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, image25, "image25");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label177, "label177");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label169, "label169");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, NetworkVbox, "NetworkVbox");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, NetworkTable, "NetworkTable");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label178, "label178");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label179, "label179");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, MulticastTtlMenu, "MulticastTtlMenu");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, menu16, "menu16");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, ttl_lan, "ttl_lan");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, ttl_organization, "ttl_organization");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, ttl_regional, "ttl_regional");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, ttl_worldwide, "ttl_worldwide");
  GLADE_HOOKUP_OBJECT (PreferencesDialog, MtuSpinButton, "MtuSpinButton");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, vbox39, "vbox39");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox89, "hbox89");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, AllowRtcpCheck, "AllowRtcpCheck");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox87, "hbox87");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, AllowSSMCheck, "AllowSSMCheck");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label170, "label170");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, MP4FileSettingsVbox, "MP4FileSettingsVbox");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, FileStatusFrame, "FileStatusFrame");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, FileStatusVbox, "FileStatusVbox");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox90, "hbox90");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, FileStatus1, "FileStatus1");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox91, "hbox91");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, FileStatus2, "FileStatus2");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox92, "hbox92");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, FileStatus3, "FileStatus3");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label180, "label180");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox93, "hbox93");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, RecordHintTrackButton, "RecordHintTrackButton");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox94, "hbox94");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, OptimizeMP4FileButton, "OptimizeMP4FileButton");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox95, "hbox95");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, RecordRawDataButton, "RecordRawDataButton");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, RawFileName, "RawFileName");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, RawFileNameBrowse, "RawFileNameBrowse");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, alignment26, "alignment26");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, hbox96, "hbox96");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, image26, "image26");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label181, "label181");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, label171, "label171");
  GLADE_HOOKUP_OBJECT_NO_REF(PreferencesDialog, dialog_action_area8, "dialog_action_area8");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, cancelbutton6, "cancelbutton6");
  GLADE_HOOKUP_OBJECT(PreferencesDialog, okbutton8, "okbutton8");
  GLADE_HOOKUP_OBJECT_NO_REF(PreferencesDialog, tooltips, "tooltips");

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(DebugCheckButton),
			       MyConfig->GetBoolValue(CONFIG_APP_DEBUG));

  const char *dir;
  char dirname[PATH_MAX];
  dir = MyConfig->GetStringValue(CONFIG_APP_STREAM_DIRECTORY);
  if (dir != NULL) {
    gtk_entry_set_text(GTK_ENTRY(StreamDirectory), dir);
  } else {
    GetHomeDirectory(dirname);
    strcat(dirname, ".mp4live_d/Streams/");
    gtk_entry_set_text(GTK_ENTRY(StreamDirectory), dirname);
  }
  dir = MyConfig->GetStringValue(CONFIG_APP_PROFILE_DIRECTORY);
  if (dir != NULL) {
    gtk_entry_set_text(GTK_ENTRY(ProfileDirectory), dir);
  } else {
    GetHomeDirectory(dirname);
    strcat(dirname, ".mp4live_d/");
    gtk_entry_set_text(GTK_ENTRY(ProfileDirectory), dirname);
  }

  uint ttl = MyConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL);
  uint index;
  if (ttl <= 1) {
    index = 0;
  } else if (ttl <= 15) {
    index = 1;
  } else if (ttl <= 64) {
    index = 2;
  } else 
    index = 3;
  gtk_option_menu_set_history(GTK_OPTION_MENU(MulticastTtlMenu), index);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(AllowRtcpCheck),
			       MyConfig->GetBoolValue(CONFIG_RTP_NO_B_RR_0));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(AllowSSMCheck),
			       MyConfig->GetBoolValue(CONFIG_RTP_USE_SSM));

  uint fstatus = MyConfig->GetIntegerValue(CONFIG_RECORD_MP4_FILE_STATUS);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileStatus1),
			       fstatus == FILE_MP4_OVERWRITE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileStatus2),
			       fstatus == FILE_MP4_APPEND);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FileStatus3),
			       fstatus == FILE_MP4_CREATE_NEW);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RecordHintTrackButton),
			       MyConfig->GetBoolValue(CONFIG_RECORD_MP4_HINT_TRACKS));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(OptimizeMP4FileButton),
			       MyConfig->GetBoolValue(CONFIG_RECORD_MP4_OPTIMIZE));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(RecordRawDataButton),
			       MyConfig->GetBoolValue(CONFIG_RECORD_RAW_IN_MP4));
  gtk_entry_set_text(GTK_ENTRY(RawFileName), 
		     MyConfig->GetStringValue(CONFIG_RECORD_RAW_MP4_FILE_NAME));

  gtk_widget_show(PreferencesDialog);
}
