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

static GtkWidget *dialog;

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
static char* ttlNames[] = {
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

	if (inet_aton(address, &in) != 0) {
		return true;
	}

	// Might have a DNS address...
	if (gethostbyname(address) != NULL) {
		return true;
	}

	ShowMessage("Transmission Address", "Invalid address entered");
	return false;
}

static void on_address_leave(GtkWidget *widget, gpointer *data)
{
	if (!address_modified) {
		return;
	}
	ValidateAddress(widget);
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

static void on_port_leave(GtkWidget *widget, gpointer *data)
{
	if (widget == video_port_entry) {
		if (!video_port_modified) {
			return;
		}
	} else { // widget == audio_port_entry
		if (!audio_port_modified) {
			return;
		}
	}
	
	ValidatePort(widget, NULL);
}

static void on_ttl_menu_activate (GtkWidget *widget, gpointer data)
{
	ttlIndex = (unsigned int)data & 0xFF;
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

	if (GenerateSdpFile(MyConfig)) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), "SDP file %s written",
			MyConfig->GetStringValue(CONFIG_SDP_FILE_NAME));
		ShowMessage("Generate SDP file", buffer); 
	} else {
		ShowMessage("Generate SDP file", 
			"SDP file couldn't be written, check file name");
	}
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

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Transmission Settings");

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
		ttlIndex,
		GTK_SIGNAL_FUNC(on_ttl_menu_activate));
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

/* end transmit_dialog.cpp */
