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

static void
on_IpAddrDialog_response               (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data)
{
  config_index_t fixed, addr, port;
  switch (addr_type) {
  case DO_AUDIO:
    fixed = STREAM_AUDIO_ADDR_FIXED;
    addr = STREAM_AUDIO_DEST_ADDR;
    port = STREAM_AUDIO_DEST_PORT;
    break;
  case DO_VIDEO:
    fixed = STREAM_VIDEO_ADDR_FIXED;
    addr = STREAM_VIDEO_DEST_ADDR;
    port = STREAM_VIDEO_DEST_PORT;
    break;
  case DO_TEXT:
  default:
    fixed = STREAM_TEXT_ADDR_FIXED;
    addr = STREAM_TEXT_DEST_ADDR;
    port = STREAM_TEXT_DEST_PORT;
  }
      
  if (response_id == GTK_RESPONSE_OK) {
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
  GtkWidget *dialog_action_area5;
  GtkWidget *cancelbutton5;
  GtkWidget *okbutton5;
  GtkTooltips *tooltips;
  media_stream = ms;


  tooltips = gtk_tooltips_new();

  IpAddrDialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(IpAddrDialog), _("IP Address"));
  gtk_window_set_modal(GTK_WINDOW(IpAddrDialog), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(IpAddrDialog), FALSE);

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
  if (do_audio) {
    sprintf(buffer, "Stream %s Audio Destination Address", 
	    media_stream->GetName());
    addr_type = DO_AUDIO;
    addr = media_stream->GetStringValue(STREAM_AUDIO_DEST_ADDR);
    port = media_stream->GetIntegerValue(STREAM_AUDIO_DEST_PORT);
    fixed = media_stream->GetBoolValue(STREAM_AUDIO_ADDR_FIXED);
  } else if (do_video) {
    sprintf(buffer, "Stream %s Video Destination Address", 
	    media_stream->GetName());
    addr_type = DO_VIDEO;
    addr = media_stream->GetStringValue(STREAM_VIDEO_DEST_ADDR);
    port = media_stream->GetIntegerValue(STREAM_VIDEO_DEST_PORT);
    fixed = media_stream->GetBoolValue(STREAM_VIDEO_ADDR_FIXED);
  } else {
    sprintf(buffer, "Stream %s Text Destination Address", 
	    media_stream->GetName());
    addr_type = DO_TEXT;
    addr = media_stream->GetStringValue(STREAM_TEXT_DEST_ADDR);
    port = media_stream->GetIntegerValue(STREAM_TEXT_DEST_PORT);
    fixed = media_stream->GetBoolValue(STREAM_TEXT_ADDR_FIXED);
  }
   
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
  GLADE_HOOKUP_OBJECT_NO_REF(IpAddrDialog, dialog_action_area5, "dialog_action_area5");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, cancelbutton5, "cancelbutton5");
  GLADE_HOOKUP_OBJECT(IpAddrDialog, okbutton5, "okbutton5");
  GLADE_HOOKUP_OBJECT_NO_REF(IpAddrDialog, tooltips, "tooltips");

  gtk_widget_show(IpAddrDialog);
}

#if 0
static GtkWidget *dialog = NULL;

static GtkWidget *unicast_button;
static GtkWidget *mcast_button;
static GtkWidget *address_entry;
static bool address_modified;
static GtkWidget *video_port_entry;
static bool video_port_modified;
static GtkWidget *audio_port_entry;
static bool audio_port_modified;
static GtkWidget *mcast_ttl_menu;
static GtkWidget *sdp_file_entry;
static GtkWidget *address_generate_button;
static GtkWidget *sdp_generate_button;

static u_int8_t ttlValues[] = {
	1, 15, 63, 127
};
static const char* ttlNames[] = {
	"LAN - 1", "Organization - 15", "Regional - 63", "Worldwide - 127"
};
static u_int8_t ttlIndex;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
	dialog = NULL;
} 

static void on_unicast (GtkWidget *widget, gpointer *data)
{
	bool enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(GTK_WIDGET(address_generate_button), !enabled);
	gtk_widget_set_sensitive(GTK_WIDGET(mcast_ttl_menu), !enabled);
}

static void on_changed (GtkWidget *widget, gpointer *data)
{
	if (widget == address_entry) {
		address_modified = true;
	} else if (widget == video_port_entry) {
		video_port_modified = true;
	} else if (widget == audio_port_entry) {
		audio_port_modified = true;
	}
}

bool ValidateAddress(GtkWidget* widget)
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

static int on_address_leave(GtkWidget *widget, gpointer *data)
{
	if (!address_modified) {
		return FALSE;
	}
	ValidateAddress(widget);
	return FALSE;
}

bool ValidatePort(GtkWidget* entry, u_int16_t* port)
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

static int on_port_leave(GtkWidget *widget, gpointer *data)
{
	if (widget == video_port_entry) {
		if (!video_port_modified) {
			return FALSE;
		}
	} else { // widget == audio_port_entry
		if (!audio_port_modified) {
			return FALSE;
		}
	}
	
	ValidatePort(widget, NULL);
	return FALSE;
}

static void on_ttl_menu_activate (GtkWidget *widget, gpointer data)
{
	ttlIndex = GPOINTER_TO_UINT(data) & 0xFF;
}

static void on_address_generate (GtkWidget *widget, gpointer *data)
{
	struct in_addr in;
	in.s_addr = CRtpTransmitter::GetRandomMcastAddress();
	gtk_entry_set_text(GTK_ENTRY(address_entry), inet_ntoa(in));

	u_int16_t portBlock = CRtpTransmitter::GetRandomPortBlock();
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(video_port_entry), portBlock);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(audio_port_entry), portBlock + 2);
}

static bool ValidateAndSave(void)
{
	if (!ValidateAddress(address_entry)) {
		return false;
	}

	u_int16_t videoPort;
 	if (!ValidatePort(video_port_entry, &videoPort)) {
		return false;
	}

	u_int16_t audioPort;
 	if (!ValidatePort(audio_port_entry, &audioPort)) {
		return false;
	}

	if (videoPort == audioPort) {
		ShowMessage("Port Error", 
			"Video and audio ports must be different");
		return false;
	}

	// copy new values to config
	MyConfig->SetStringValue(CONFIG_RTP_DEST_ADDRESS, 
		gtk_entry_get_text(GTK_ENTRY(address_entry)));
	MyConfig->SetStringValue(CONFIG_RTP_AUDIO_DEST_ADDRESS,
		gtk_entry_get_text(GTK_ENTRY(address_entry)));
				 
	MyConfig->SetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT, videoPort);
	MyConfig->SetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT, audioPort);
	MyConfig->SetIntegerValue(CONFIG_RTP_MCAST_TTL, ttlValues[ttlIndex]);

	MyConfig->SetStringValue(CONFIG_SDP_FILE_NAME,
		gtk_entry_get_text(GTK_ENTRY(sdp_file_entry)));

	DisplayTransmitSettings();  // display settings in main window

	return true;
}

static void on_sdp_generate (GtkWidget *widget, gpointer *data)
{
	// check and save values
	if (!ValidateAndSave()) {
		return;
	}

#if 0
	if (GenerateSdpFile(MyConfig)) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "SDP file %s written",
			MyConfig->GetStringValue(CONFIG_SDP_FILE_NAME));
		ShowMessage("Generate SDP file", buffer); 
	} else {
		ShowMessage("Generate SDP file", 
			"SDP file couldn't be written, check file name");
	}
#endif
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

void CreateTransmitDialog (void) 
{
	GtkWidget* hbox;
	GtkWidget* vbox;
	GSList* radioGroup;
	GtkWidget* label;
	GtkWidget* button;
	GtkObject* adjustment;

	SDL_LockMutex(dialog_mutex);
	if (dialog != NULL) {
	  SDL_UnlockMutex(dialog_mutex);
	  return;
	}
	SDL_UnlockMutex(dialog_mutex);

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Transmission Settings");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

	hbox = gtk_hbox_new(TRUE, 1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
		FALSE, FALSE, 5);
	gtk_widget_show(hbox);

	// Unicast/Multicast radio
	bool isMcast = true;
	if (MyConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS) != NULL) {
		struct in_addr in;
		if (inet_aton(MyConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS), &in)) {
			isMcast = IN_MULTICAST(ntohl(in.s_addr));
		} else {
			isMcast = false;
		}
	}

	unicast_button = gtk_radio_button_new_with_label(NULL, "Unicast");
	gtk_widget_show(unicast_button);
	gtk_signal_connect(GTK_OBJECT(unicast_button), 
		"toggled",
		 GTK_SIGNAL_FUNC(on_unicast),
		 NULL);
	gtk_box_pack_start(GTK_BOX(hbox), unicast_button,
		FALSE, FALSE, 5);

	radioGroup = gtk_radio_button_group(GTK_RADIO_BUTTON(unicast_button));
	mcast_button = gtk_radio_button_new_with_label(radioGroup, "Multicast");
	gtk_widget_show(mcast_button);
	gtk_box_pack_start(GTK_BOX(hbox), mcast_button,
		FALSE, FALSE, 5);

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
		FALSE, FALSE, 5);

	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox,
		FALSE, FALSE, 5);

	label = gtk_label_new(" Address:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Video Port:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Audio Port:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" TTL:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" SDP File:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox,
		TRUE, TRUE, 5);

	address_entry = gtk_entry_new_with_max_length(128);
	gtk_entry_set_text(GTK_ENTRY(address_entry), 
		MyConfig->GetStringValue(CONFIG_RTP_DEST_ADDRESS));
	address_modified = false;
	SetEntryValidator(GTK_OBJECT(address_entry),
		GTK_SIGNAL_FUNC(on_changed),
		GTK_SIGNAL_FUNC(on_address_leave));
	gtk_widget_show(address_entry);
	gtk_box_pack_start(GTK_BOX(vbox), address_entry, TRUE, TRUE, 0);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_RTP_VIDEO_DEST_PORT),
		1024, 65534, 2, 0, 0);
	video_port_entry = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 2, 0);
	video_port_modified = false;
	SetEntryValidator(GTK_OBJECT(video_port_entry),
		GTK_SIGNAL_FUNC(on_changed),
		GTK_SIGNAL_FUNC(on_port_leave));
	gtk_widget_show(video_port_entry);
	gtk_box_pack_start(GTK_BOX(vbox), video_port_entry, FALSE, FALSE, 0);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_RTP_AUDIO_DEST_PORT),
		1024, 65534, 2, 0, 0);
	audio_port_entry = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 2, 0);
	audio_port_modified = false;
	SetEntryValidator(GTK_OBJECT(audio_port_entry),
		GTK_SIGNAL_FUNC(on_changed),
		GTK_SIGNAL_FUNC(on_port_leave));
	gtk_widget_show(audio_port_entry);
	gtk_box_pack_start(GTK_BOX(vbox), audio_port_entry, TRUE, TRUE, 0);

	ttlIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(ttlValues) / sizeof(u_int8_t); i++) {
		if (MyConfig->GetIntegerValue(CONFIG_RTP_MCAST_TTL) == ttlValues[i]) {
			ttlIndex = i;
			break;
		}
	}
	mcast_ttl_menu = CreateOptionMenu (NULL,
		ttlNames, 
		sizeof(ttlNames) / sizeof(char*),
					   ttlIndex);
	gtk_box_pack_start(GTK_BOX(vbox), mcast_ttl_menu, TRUE, TRUE, 0);

	sdp_file_entry = gtk_entry_new_with_max_length(128);
	gtk_entry_set_text(GTK_ENTRY(sdp_file_entry), 
		MyConfig->GetStringValue(CONFIG_SDP_FILE_NAME));
	gtk_widget_show(sdp_file_entry);
	gtk_box_pack_start(GTK_BOX(vbox), sdp_file_entry, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	address_generate_button = gtk_button_new_with_label(" Generate ");
	gtk_signal_connect(GTK_OBJECT(address_generate_button), 
		"clicked",
		 GTK_SIGNAL_FUNC(on_address_generate),
		 NULL);
	gtk_widget_show(address_generate_button);
	gtk_box_pack_start(GTK_BOX(vbox), address_generate_button, FALSE, FALSE, 0);

	sdp_generate_button = gtk_button_new_with_label(" Generate ");
	gtk_signal_connect(GTK_OBJECT(sdp_generate_button), 
		"clicked",
		 GTK_SIGNAL_FUNC(on_sdp_generate),
		 NULL);
	gtk_widget_show(sdp_generate_button);
	gtk_box_pack_end(GTK_BOX(vbox), sdp_generate_button, FALSE, FALSE, 0);

	// do these now so other widget sensitivies will be changed appropriately
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(unicast_button), !isMcast);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mcast_button), isMcast);

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
#endif
/* end transmit_dialog.cpp */
