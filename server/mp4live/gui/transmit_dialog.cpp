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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "rtp_transmitter.h"
#include "support.h"

static CMediaStream *media_stream;
static enum { 
  DO_AUDIO, 
  DO_VIDEO, 
  DO_TEXT
} addr_type = DO_AUDIO;

#ifdef HAVE_SRTP
static bool ValidateKeySalt (const char *value, bool dokey)
{
  uint max_ix = dokey ? 32 : 28;
  uint ix;

  for (ix = 0; ix < max_ix; ix++) {
    if (!isxdigit(value[ix])) {
      return false;
    }
  }
  return value[ix] == '\0';
}
#endif

static bool ValidatePort(GtkWidget* entry, in_port_t *port)
{
  int value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(entry));

	if (value < 1024 || value > 65534 || (value & 1)) {
		ShowMessage("Transmission Port Number",
			"Please enter an even number between 1024 and 65534");
		// TBD gtk_widget_grab_focus(widget);
		return false;
	}
	if (port) {
		*port = value;
	}
	return true;
}

static bool ValidateAddress (GtkWidget* widget)
{
	const char* address = gtk_entry_get_text(GTK_ENTRY(widget));

	struct in_addr in;
	if (inet_pton(AF_INET, address, &in) > 0) {
		return true;
	}

	struct in6_addr in6;
	if (inet_pton(AF_INET6, address, &in6) > 0) {
		return true;
	}

	// Might have a DNS address...
	if (gethostbyname(address) != NULL) {
		return true;
	}

	ShowMessage("Transmission Address", "Invalid address entered");
	return false;
}

#ifdef HAVE_SRTP
static void
on_SRTPEnableButton_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  bool enabled  = gtk_toggle_button_get_active(togglebutton);
  GtkWidget *temp = lookup_widget(GTK_WIDGET(dialog), "SrtpAuthType");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "SrtpEncType");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "EnableRtpEncCheckButton");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "EnableRtpAuthCheckButton");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "EnableRtpAuthCheckButton");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "EnableRtcpEncCheckButton");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "SpecifyKeySaltCheckButton");
  gtk_widget_set_sensitive(temp, enabled);
  if (enabled) {
    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp));
  }
  temp = lookup_widget(GTK_WIDGET(dialog), "SrtpKeyValue");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "SrtpSaltValue");
  gtk_widget_set_sensitive(temp, enabled);
  
}


static void
on_SpecifyKeySaltCheckButton_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  bool enabled  = gtk_toggle_button_get_active(togglebutton);
  GtkWidget *temp = lookup_widget(GTK_WIDGET(dialog), "SrtpKeyValue");
  gtk_widget_set_sensitive(temp, enabled);
  temp = lookup_widget(GTK_WIDGET(dialog), "SrtpSaltValue");
  gtk_widget_set_sensitive(temp, enabled);
}
#endif

static void
on_IpAddrDialog_response               (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  config_index_t fixed, addr, port;
  config_index_t use_srtp, enc_algo, auth_algo, srtp_fixed_keys, srtp_key, srtp_salt;
  config_index_t rtp_enc, rtp_auth, rtcp_enc;
  switch (addr_type) {
  case DO_AUDIO:
    fixed = STREAM_AUDIO_ADDR_FIXED;
    addr = STREAM_AUDIO_DEST_ADDR;
    port = STREAM_AUDIO_DEST_PORT;
    use_srtp = STREAM_AUDIO_USE_SRTP;
    enc_algo = STREAM_AUDIO_SRTP_ENC_ALGO;
    auth_algo = STREAM_AUDIO_SRTP_AUTH_ALGO;
    srtp_fixed_keys = STREAM_AUDIO_SRTP_FIXED_KEYS;
    srtp_key = STREAM_AUDIO_SRTP_KEY;
    srtp_salt = STREAM_AUDIO_SRTP_SALT;
    rtp_enc = STREAM_AUDIO_SRTP_RTP_ENC;
    rtp_auth = STREAM_AUDIO_SRTP_RTP_AUTH;
    rtcp_enc = STREAM_AUDIO_SRTP_RTCP_ENC;
    break;
  case DO_VIDEO:
    fixed = STREAM_VIDEO_ADDR_FIXED;
    addr = STREAM_VIDEO_DEST_ADDR;
    port = STREAM_VIDEO_DEST_PORT;
    use_srtp = STREAM_VIDEO_USE_SRTP;
    enc_algo = STREAM_VIDEO_SRTP_ENC_ALGO;
    auth_algo = STREAM_VIDEO_SRTP_AUTH_ALGO;
    srtp_fixed_keys = STREAM_VIDEO_SRTP_FIXED_KEYS;
    srtp_key = STREAM_VIDEO_SRTP_KEY;
    srtp_salt = STREAM_VIDEO_SRTP_SALT;
    rtp_enc = STREAM_VIDEO_SRTP_RTP_ENC;
    rtp_auth = STREAM_VIDEO_SRTP_RTP_AUTH;
    rtcp_enc = STREAM_VIDEO_SRTP_RTCP_ENC;
    break;
  case DO_TEXT:
  default:
    fixed = STREAM_TEXT_ADDR_FIXED;
    addr = STREAM_TEXT_DEST_ADDR;
    port = STREAM_TEXT_DEST_PORT;
    use_srtp = STREAM_TEXT_USE_SRTP;
    enc_algo = STREAM_TEXT_SRTP_ENC_ALGO;
    auth_algo = STREAM_TEXT_SRTP_AUTH_ALGO;
    srtp_fixed_keys = STREAM_TEXT_SRTP_FIXED_KEYS;
    srtp_key = STREAM_TEXT_SRTP_KEY;
    srtp_salt = STREAM_TEXT_SRTP_SALT;
    rtp_enc = STREAM_TEXT_SRTP_RTP_ENC;
    rtp_auth = STREAM_TEXT_SRTP_RTP_AUTH;
    rtcp_enc = STREAM_TEXT_SRTP_RTCP_ENC;
    break;
  }
      
  if (response_id == GTK_RESPONSE_OK) {
#ifdef HAVE_SRTP
    GtkWidget *temp;
    bool have_srtp;
    temp = lookup_widget(GTK_WIDGET(dialog), "SRTPEnableButton");
    have_srtp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp));
    if (have_srtp) {
      temp = lookup_widget(GTK_WIDGET(dialog), "SpecifyKeySaltCheckButton");
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp))) {
	temp = lookup_widget(GTK_WIDGET(dialog), "SrtpSaltValue");
	if (ValidateKeySalt(gtk_entry_get_text(GTK_ENTRY(temp)), false) == false) {
	  ShowMessage("Transmission Address", "Invalid salt entered");
	  return;
	}
	temp = lookup_widget(GTK_WIDGET(dialog), "SrtpKeyValue");
	if (ValidateKeySalt(gtk_entry_get_text(GTK_ENTRY(temp)), true) == false) {
	  ShowMessage("Transmission Address", "Invalid key entered");
	  return;
	}
      }
    }
#endif
      
    GtkWidget *fixedw = lookup_widget(GTK_WIDGET(dialog), "FixedAddrButton");
    
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fixedw))) {
      in_port_t readport;
      GtkWidget *ipaddr;
      ipaddr = lookup_widget(GTK_WIDGET(dialog), "IpAddr");
      if (ValidateAddress(ipaddr) == false)
	return;
      if (ValidatePort(lookup_widget(GTK_WIDGET(dialog), 
				     "IpPort"), &readport) == false) 
	return;
      media_stream->SetBoolValue(fixed, true);
      media_stream->SetStringValue(addr, gtk_entry_get_text(GTK_ENTRY(ipaddr)));
      media_stream->SetIntegerValue(port, readport);
    } else {
      media_stream->SetBoolValue(fixed, false);
    }
#ifdef HAVE_SRTP
    if (have_srtp) {
      media_stream->SetBoolValue(use_srtp, true);
      temp = lookup_widget(GTK_WIDGET(dialog), "SrtpEncType");
      media_stream->SetIntegerValue(enc_algo, 
				    gtk_combo_box_get_active(GTK_COMBO_BOX(temp)));
      temp = lookup_widget(GTK_WIDGET(dialog), "SrtpAuthType");
      media_stream->SetIntegerValue(auth_algo, 
				    gtk_combo_box_get_active(GTK_COMBO_BOX(temp)));
      temp = lookup_widget(GTK_WIDGET(dialog), "SpecifyKeySaltCheckButton");
      bool specify_key = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp));
      media_stream->SetBoolValue(srtp_fixed_keys, specify_key);
      if (specify_key) {
	temp = lookup_widget(GTK_WIDGET(dialog), "SrtpKeyValue");
	media_stream->SetStringValue(srtp_key, gtk_entry_get_text(GTK_ENTRY(temp)));
	temp = lookup_widget(GTK_WIDGET(dialog), "SrtpSaltValue");
	media_stream->SetStringValue(srtp_salt, gtk_entry_get_text(GTK_ENTRY(temp)));
      }
      temp = lookup_widget(GTK_WIDGET(dialog), "EnableRtpEncCheckButton");
      media_stream->SetBoolValue(rtp_enc, 
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp)));
      temp = lookup_widget(GTK_WIDGET(dialog), "EnableRtpAuthCheckButton");
      media_stream->SetBoolValue(rtp_auth, 
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp)));
      temp = lookup_widget(GTK_WIDGET(dialog), "EnableRtcpEncCheckButton");
      media_stream->SetBoolValue(rtcp_enc, 
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(temp)));
    } else {
      media_stream->SetBoolValue(use_srtp, false);
    }
#endif
    AVFlow->ValidateAndUpdateStreams();
    RefreshCurrentStream();
  }
  gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void
on_FixedAddrButton_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
  GtkWidget *dialog = GTK_WIDGET(user_data);
  GtkWidget *ipaddr, *ipport;
  ipaddr = lookup_widget(dialog, "IpAddr");
  ipport = lookup_widget(dialog, "IpPort");
  debug_message("fixed toggled %p", togglebutton);
  bool test = gtk_toggle_button_get_active(togglebutton);

  gtk_widget_set_sensitive(ipaddr, test);
  gtk_widget_set_sensitive(ipport, test);
}

void create_IpAddrDialog (CMediaStream *ms, 
			  bool do_audio, 
			  bool do_video, 
			  bool do_text)
{
  GtkWidget *IpAddrDialog;
  GtkWidget *dialog_vbox6;
  GtkWidget *vbox35;
  GtkWidget *IpAddrDialogLabel;
  GtkWidget *table4;
  GtkWidget *GenerateAddrButton;
  GSList *GenerateAddrButton_group = NULL;
  GtkWidget *FixedAddrButton;
  GtkWidget *GenerateAddrLable;
  GtkWidget *hbox74;
  GtkWidget *IpAddr;
  GtkWidget *label149;
  GtkObject *IpPort_adj;
  GtkWidget *IpPort;
#ifdef HAVE_SRTP
  GtkWidget *hseparator1;
  GtkWidget *vbox49;
  GtkWidget *alignment37;
  GtkWidget *SRTPEnableButton;
  GtkWidget *SrtpParametersTable;
  GtkWidget *label215;
  GtkWidget *SrtpAuthType;
  GtkWidget *label216;
  GtkWidget *SpecifyKeySaltCheckButton;
  GtkWidget *label217;
  GtkWidget *SrtpKeyValue;
  GtkWidget *label218;
  GtkWidget *SrtpSaltValue;
  GtkWidget *label220;
  GtkWidget *label221;
  GtkWidget *EnableRtpEncCheckButton;
  GtkWidget *EnableRtpAuthCheckButton;
  GtkWidget *EnableRtcpEncCheckButton;
  GtkWidget *label222;
  GtkWidget *SrtpEncType;
  GtkWidget *label223;
#endif
  GtkWidget *dialog_action_area5;
  GtkWidget *cancelbutton5;
  GtkWidget *okbutton5;
  GtkTooltips *tooltips;
  media_stream = ms;


  tooltips = gtk_tooltips_new();

  IpAddrDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(IpAddrDialog), _("Transmit Address and Parameters"));
  gtk_window_set_modal(GTK_WINDOW(IpAddrDialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(IpAddrDialog), FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(IpAddrDialog), 
			       GTK_WINDOW(MainWindow));

  dialog_vbox6 = GTK_DIALOG(IpAddrDialog)->vbox;
  gtk_widget_show(dialog_vbox6);

  vbox35 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox35);
  gtk_box_pack_start(GTK_BOX(dialog_vbox6), vbox35, TRUE, TRUE, 0);

  IpAddrDialogLabel = gtk_label_new(_("label147"));
  gtk_widget_show(IpAddrDialogLabel);
  gtk_box_pack_start(GTK_BOX(vbox35), IpAddrDialogLabel, FALSE, FALSE, 0);

  table4 = gtk_table_new(2, 2, FALSE);
  gtk_widget_show(table4);
  gtk_box_pack_start(GTK_BOX(vbox35), table4, TRUE, TRUE, 0);

  GenerateAddrButton = gtk_radio_button_new_with_mnemonic(NULL, _("Generate Address "));
  gtk_widget_show(GenerateAddrButton);
  gtk_table_attach(GTK_TABLE(table4), GenerateAddrButton, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, GenerateAddrButton, _("Generate Multicast Address Automatically"), NULL);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(GenerateAddrButton), GenerateAddrButton_group);
  GenerateAddrButton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(GenerateAddrButton));

  FixedAddrButton = gtk_radio_button_new_with_mnemonic(NULL, _("Fixed Address"));
  gtk_widget_show(FixedAddrButton);
  gtk_table_attach(GTK_TABLE(table4), FixedAddrButton, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_tooltips_set_tip(tooltips, FixedAddrButton, _("Set Fixed Address"), NULL);
  gtk_radio_button_set_group(GTK_RADIO_BUTTON(FixedAddrButton), GenerateAddrButton_group);
  GenerateAddrButton_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(FixedAddrButton));

  GenerateAddrLable = gtk_label_new("");
  gtk_widget_show(GenerateAddrLable);
  gtk_table_attach(GTK_TABLE(table4), GenerateAddrLable, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(GenerateAddrLable), 0, 0.5);

  hbox74 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox74);
  gtk_table_attach(GTK_TABLE(table4), hbox74, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(GTK_FILL), 0, 0);

  IpAddr = gtk_entry_new();
  gtk_widget_show(IpAddr);
  gtk_box_pack_start(GTK_BOX(hbox74), IpAddr, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, IpAddr, _("Enter IP Address"), NULL);
  gtk_entry_set_width_chars(GTK_ENTRY(IpAddr), 20);

  label149 = gtk_label_new(_(":"));
  gtk_widget_show(label149);
  gtk_box_pack_start(GTK_BOX(hbox74), label149, FALSE, FALSE, 0);

  IpPort_adj = 
    gtk_adjustment_new(1024, 1024, 65534, 2, 10, 10);
  IpPort = gtk_spin_button_new(GTK_ADJUSTMENT(IpPort_adj), 1, 0);
  gtk_widget_show(IpPort);
  gtk_box_pack_start(GTK_BOX(hbox74), IpPort, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tooltips, IpPort, _("Enter IP Port"), NULL);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(IpPort), TRUE);

#ifdef HAVE_SRTP
  // replace with SRTP
  hseparator1 = gtk_hseparator_new();
  gtk_widget_show(hseparator1);
  gtk_box_pack_start(GTK_BOX(vbox35), hseparator1, TRUE, TRUE, 6);

  vbox49 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox49);
  gtk_box_pack_start(GTK_BOX(vbox35), vbox49, TRUE, TRUE, 0);

  alignment37 = gtk_alignment_new(0.5, 0.5, 1, 1);
  gtk_widget_show(alignment37);
  gtk_box_pack_start(GTK_BOX(vbox49), alignment37, FALSE, FALSE, 0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment37), 0, 0, 144, 0);

  SRTPEnableButton = gtk_check_button_new_with_mnemonic(_("Enable SRTP"));
  gtk_widget_show(SRTPEnableButton);
  gtk_container_add(GTK_CONTAINER(alignment37), SRTPEnableButton);

  SrtpParametersTable = gtk_table_new(7, 2, FALSE);
  gtk_widget_show(SrtpParametersTable);
  gtk_box_pack_start(GTK_BOX(vbox49), SrtpParametersTable, TRUE, TRUE, 0);
  gtk_table_set_row_spacings(GTK_TABLE(SrtpParametersTable), 3);

  label223 = gtk_label_new(_("Encryption Algorithm:"));
  gtk_widget_show(label223);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label223, 0, 1, 0, 1,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label223), 0, 0.5);

  SrtpEncType = gtk_combo_box_new_text();
  gtk_widget_show(SrtpEncType);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), SrtpEncType, 1, 2, 0, 1,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_FILL), 0, 0);
  gtk_combo_box_append_text(GTK_COMBO_BOX(SrtpEncType), _("AES_CM_128"));

  label215 = gtk_label_new(_("Authentication Algorithm:"));
  gtk_widget_show(label215);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label215, 0, 1, 1, 2,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label215), 0, 0.5);

  SrtpAuthType = gtk_combo_box_new_text();
  gtk_widget_show(SrtpAuthType);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), SrtpAuthType, 1, 2, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(GTK_FILL), 0, 0);
  gtk_combo_box_append_text(GTK_COMBO_BOX(SrtpAuthType), _("HMAC_SHA1_80"));
  gtk_combo_box_append_text(GTK_COMBO_BOX(SrtpAuthType), _("HMAC_SHA1_32"));

  label220 = gtk_label_new ("");
  gtk_widget_show (label220);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label220, 0, 1, 2, 3,
                      (GtkAttachOptions)(GTK_FILL),
                      (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label220), 0, 0.5);
  
  label221 = gtk_label_new("");
  gtk_widget_show(label221);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label221, 0, 1, 3, 4,
                      (GtkAttachOptions)(GTK_FILL),
                    (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label221), 0, 0.5);

  EnableRtpEncCheckButton = gtk_check_button_new_with_mnemonic(_("Enable RTP Encryption"));
  gtk_widget_show(EnableRtpEncCheckButton);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), EnableRtpEncCheckButton, 1, 2, 3, 4,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  label222 = gtk_label_new("");
  gtk_widget_show(label222);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label222, 0, 1, 4, 5,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label222), 0, 0.5);

  EnableRtpAuthCheckButton = gtk_check_button_new_with_mnemonic(_("Enable RTP Authentication"));
  gtk_widget_show(EnableRtpAuthCheckButton);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), EnableRtpAuthCheckButton, 1, 2, 4, 5,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  label216 = gtk_label_new("");
  gtk_widget_show(label216);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label216, 0, 1, 5, 6,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label216), 0, 0.5);

  EnableRtcpEncCheckButton = gtk_check_button_new_with_mnemonic(_("Enable RTCP Encryption"));
  gtk_widget_show(EnableRtcpEncCheckButton);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), EnableRtcpEncCheckButton, 1, 2, 5, 6,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);


  SpecifyKeySaltCheckButton = gtk_check_button_new_with_mnemonic(_("Specify Key/Salt"));
  gtk_widget_show(SpecifyKeySaltCheckButton);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), SpecifyKeySaltCheckButton, 1, 2, 6, 7,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);

  label217 = gtk_label_new(_("Key Value:"));
  gtk_widget_show(label217);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label217, 0, 1, 7, 8,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label217), 0, 0.5);

  SrtpKeyValue = gtk_entry_new();
  gtk_widget_show(SrtpKeyValue);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), SrtpKeyValue, 1, 2, 7, 8,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_entry_set_max_length(GTK_ENTRY(SrtpKeyValue), 33);
  gtk_entry_set_width_chars(GTK_ENTRY(SrtpKeyValue), 33);

  label218 = gtk_label_new(_("Salt Value:"));
  gtk_widget_show(label218);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), label218, 0, 1, 8, 9,
                   (GtkAttachOptions)(GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_misc_set_alignment(GTK_MISC(label218), 0, 0.5);

  SrtpSaltValue = gtk_entry_new();
  gtk_widget_show(SrtpSaltValue);
  gtk_table_attach(GTK_TABLE(SrtpParametersTable), SrtpSaltValue, 1, 2, 8, 9,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
                   (GtkAttachOptions)(0), 0, 0);
  gtk_entry_set_max_length(GTK_ENTRY(SrtpSaltValue), 29);
  gtk_entry_set_width_chars(GTK_ENTRY(SrtpSaltValue), 29);

#endif
  dialog_action_area5 = GTK_DIALOG(IpAddrDialog)->action_area;
  gtk_widget_show(dialog_action_area5);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(dialog_action_area5), GTK_BUTTONBOX_END);

  cancelbutton5 = gtk_button_new_from_stock("gtk-cancel");
  gtk_widget_show(cancelbutton5);
  gtk_dialog_add_action_widget(GTK_DIALOG(IpAddrDialog), cancelbutton5, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS(cancelbutton5, GTK_CAN_DEFAULT);

  okbutton5 = gtk_button_new_from_stock("gtk-ok");
  gtk_widget_show(okbutton5);
  gtk_dialog_add_action_widget(GTK_DIALOG(IpAddrDialog), okbutton5, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(okbutton5, GTK_CAN_DEFAULT);
  bool fixed;
  const char *addr;
  in_port_t port;
  char buffer[128];
  bool srtp_enabled;
  uint enc_algo, auth_algo;
  bool srtp_fixed;
  const char *srtp_key;
  const char *srtp_salt;
  bool rtp_enc;
  bool rtp_auth;
  bool rtcp_enc;
  if (do_audio) {
    sprintf(buffer, "Stream \"%s\" Audio Destination Address", 
	    media_stream->GetName());
    addr_type = DO_AUDIO;
    addr = media_stream->GetStringValue(STREAM_AUDIO_DEST_ADDR);
    port = media_stream->GetIntegerValue(STREAM_AUDIO_DEST_PORT);
    fixed = media_stream->GetBoolValue(STREAM_AUDIO_ADDR_FIXED);
    srtp_enabled = media_stream->GetBoolValue(STREAM_AUDIO_USE_SRTP);
    enc_algo = media_stream->GetIntegerValue(STREAM_AUDIO_SRTP_ENC_ALGO);
    auth_algo = media_stream->GetIntegerValue(STREAM_AUDIO_SRTP_AUTH_ALGO);
    srtp_fixed = media_stream->GetBoolValue(STREAM_AUDIO_SRTP_FIXED_KEYS);
    srtp_key = media_stream->GetStringValue(STREAM_AUDIO_SRTP_KEY);
    srtp_salt = media_stream->GetStringValue(STREAM_AUDIO_SRTP_SALT);
    rtp_enc = media_stream->GetBoolValue(STREAM_AUDIO_SRTP_RTP_ENC);
    rtp_auth = media_stream->GetBoolValue(STREAM_AUDIO_SRTP_RTP_AUTH);
    rtcp_enc = media_stream->GetBoolValue(STREAM_AUDIO_SRTP_RTCP_ENC);

  } else if (do_video) {
    sprintf(buffer, "Stream \"%s\" Video Destination Address", 
	    media_stream->GetName());
    addr_type = DO_VIDEO;
    addr = media_stream->GetStringValue(STREAM_VIDEO_DEST_ADDR);
    port = media_stream->GetIntegerValue(STREAM_VIDEO_DEST_PORT);
    fixed = media_stream->GetBoolValue(STREAM_VIDEO_ADDR_FIXED);
    srtp_enabled = media_stream->GetBoolValue(STREAM_VIDEO_USE_SRTP);
    enc_algo = media_stream->GetIntegerValue(STREAM_VIDEO_SRTP_ENC_ALGO);
    auth_algo = media_stream->GetIntegerValue(STREAM_VIDEO_SRTP_AUTH_ALGO);
    srtp_fixed = media_stream->GetBoolValue(STREAM_VIDEO_SRTP_FIXED_KEYS);
    srtp_key = media_stream->GetStringValue(STREAM_VIDEO_SRTP_KEY);
    srtp_salt = media_stream->GetStringValue(STREAM_VIDEO_SRTP_SALT);
    rtp_enc = media_stream->GetBoolValue(STREAM_VIDEO_SRTP_RTP_ENC);
    rtp_auth = media_stream->GetBoolValue(STREAM_VIDEO_SRTP_RTP_AUTH);
    rtcp_enc = media_stream->GetBoolValue(STREAM_VIDEO_SRTP_RTCP_ENC);
  } else {
    sprintf(buffer, "Stream \"%s\" Text Destination Address", 
	    media_stream->GetName());
    addr_type = DO_TEXT;
    addr = media_stream->GetStringValue(STREAM_TEXT_DEST_ADDR);
    port = media_stream->GetIntegerValue(STREAM_TEXT_DEST_PORT);
    fixed = media_stream->GetBoolValue(STREAM_TEXT_ADDR_FIXED);
    srtp_enabled = media_stream->GetBoolValue(STREAM_TEXT_USE_SRTP);
    enc_algo = media_stream->GetIntegerValue(STREAM_TEXT_SRTP_ENC_ALGO);
    auth_algo = media_stream->GetIntegerValue(STREAM_TEXT_SRTP_AUTH_ALGO);
    srtp_fixed = media_stream->GetBoolValue(STREAM_TEXT_SRTP_FIXED_KEYS);
    srtp_key = media_stream->GetStringValue(STREAM_TEXT_SRTP_KEY);
    srtp_salt = media_stream->GetStringValue(STREAM_TEXT_SRTP_SALT);
    rtp_enc = media_stream->GetBoolValue(STREAM_TEXT_SRTP_RTP_ENC);
    rtp_auth = media_stream->GetBoolValue(STREAM_TEXT_SRTP_RTP_AUTH);
    rtcp_enc = media_stream->GetBoolValue(STREAM_TEXT_SRTP_RTCP_ENC);
  }
#ifdef HAVE_SRTP
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SRTPEnableButton), srtp_enabled);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(SpecifyKeySaltCheckButton), 
			       srtp_fixed);
  gtk_combo_box_set_active(GTK_COMBO_BOX(SrtpEncType), enc_algo);
  gtk_combo_box_set_active(GTK_COMBO_BOX(SrtpAuthType), auth_algo);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(EnableRtpEncCheckButton), rtp_enc);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(EnableRtpAuthCheckButton), rtp_auth);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(EnableRtcpEncCheckButton), rtcp_enc);
  if (srtp_key != NULL) gtk_entry_set_text(GTK_ENTRY(SrtpKeyValue), srtp_key);
  if (srtp_salt != NULL) gtk_entry_set_text(GTK_ENTRY(SrtpSaltValue), srtp_salt);
  if (srtp_enabled == false) {
    gtk_widget_set_sensitive(SpecifyKeySaltCheckButton, false);
    gtk_widget_set_sensitive(SrtpEncType, false);
    gtk_widget_set_sensitive(SrtpAuthType, false);
    gtk_widget_set_sensitive(EnableRtpEncCheckButton, false);
    gtk_widget_set_sensitive(EnableRtpAuthCheckButton, false);
    gtk_widget_set_sensitive(EnableRtcpEncCheckButton, false);
  }
  if (srtp_enabled == false || srtp_fixed == false) {
    gtk_widget_set_sensitive(SrtpKeyValue, false);
    gtk_widget_set_sensitive(SrtpSaltValue, false);
  }
#endif
   
  gtk_label_set_text(GTK_LABEL(IpAddrDialogLabel), buffer);
  gtk_entry_set_text(GTK_ENTRY(IpAddr), addr);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(IpPort), port);
  if (fixed) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(FixedAddrButton), true);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(GenerateAddrButton), true);
    gtk_widget_set_sensitive(IpAddr, false);
    gtk_widget_set_sensitive(IpPort, false);
  }
    
  g_signal_connect((gpointer) IpAddrDialog, "response",
                    G_CALLBACK(on_IpAddrDialog_response),
                    NULL);
  g_signal_connect((gpointer) FixedAddrButton, "toggled",
                    G_CALLBACK(on_FixedAddrButton_toggled),
                    IpAddrDialog);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF(IpAddrDialog, IpAddrDialog, "IpAddrDialog");
  GLADE_HOOKUP_OBJECT_NO_REF(IpAddrDialog, dialog_vbox6, "dialog_vbox6");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, vbox35, "vbox35");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, IpAddrDialogLabel, "IpAddrDialogLabel");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, table4, "table4");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, GenerateAddrButton, "GenerateAddrButton");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, FixedAddrButton, "FixedAddrButton");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, GenerateAddrLable, "GenerateAddrLable");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, hbox74, "hbox74");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, IpAddr, "IpAddr");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label149, "label149");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, IpPort, "IpPort");
#ifdef HAVE_SRTP
  g_signal_connect((gpointer) SRTPEnableButton, "toggled",
		   G_CALLBACK(on_SRTPEnableButton_toggled),
		   IpAddrDialog);
  g_signal_connect((gpointer) SpecifyKeySaltCheckButton, "toggled",
                    G_CALLBACK(on_SpecifyKeySaltCheckButton_toggled),
		   IpAddrDialog);

  GLADE_HOOKUP_OBJECT(IpAddrDialog, hseparator1, "hseparator1");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, vbox49, "vbox49");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, alignment37, "alignment37");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, SRTPEnableButton, "SRTPEnableButton");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, SrtpParametersTable, "SrtpParametersTable");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label223, "label223");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, SrtpEncType, "SrtpEncType");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label215, "label215");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, SrtpAuthType, "SrtpAuthType");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label216, "label216");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, SpecifyKeySaltCheckButton, "SpecifyKeySaltCheckButton");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label217, "label217");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, SrtpKeyValue, "SrtpKeyValue");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label218, "label218");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, SrtpSaltValue, "SrtpSaltValue");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label220, "label220");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label221, "label221");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, EnableRtpEncCheckButton, "EnableRtpEncCheckButton");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, EnableRtpAuthCheckButton, "EnableRtpAuthCheckButton");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, EnableRtcpEncCheckButton, "EnableRtcpEncCheckButton");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, label222, "label222");
#endif
  GLADE_HOOKUP_OBJECT_NO_REF(IpAddrDialog, dialog_action_area5, "dialog_action_area5");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, cancelbutton5, "cancelbutton5");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, okbutton5, "okbutton5");
  GLADE_HOOKUP_OBJECT_NO_REF(IpAddrDialog, tooltips, "tooltips");

  gtk_widget_show(IpAddrDialog);
}

/* end transmit_dialog.cpp */

