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

/* end transmit_dialog.cpp */
