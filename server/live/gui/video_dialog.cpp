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
#include "video_source.h"

static GtkWidget *dialog;

static GtkWidget *device_entry;
static bool device_modified;
static GtkWidget *signal_menu;
static GtkWidget *size_menu;
static GtkWidget *aspect_menu;
static GtkWidget *frame_rate_spinner;
static GtkWidget *bit_rate_spinner;

static u_int16_t signalValues[] = {
	VIDEO_MODE_NTSC, VIDEO_MODE_PAL, VIDEO_MODE_SECAM
};
static char* signalNames[] = {
	"NTSC", "PAL", "SECAM"
};
static u_int8_t signalIndex;

static u_int16_t sizeWidthValues[] = {
	160, 176, 320, 352
};
static char* sizeNames[] = {
	"160 x 120", "176 x 144", "320 x 240", "352 x 288"
};
static u_int8_t sizeIndex;

static float aspectValues[] = {
	VIDEO_STD_ASPECT_RATIO, VIDEO_LB1_ASPECT_RATIO, VIDEO_LB2_ASPECT_RATIO
}; 
static char* aspectNames[] = {
	"Standard 4:3", "Letterbox 2.35", "Letterbox 1.75"
};
static u_int8_t aspectIndex;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
	dialog = NULL;
} 

static void on_changed(GtkWidget *widget, gpointer *data)
{
	if (widget == device_entry) {
		device_modified = true;
	}
}

static void on_device_leave(GtkWidget *widget, gpointer *data)
{
	if (!device_modified) {
		return;
	}
	// TBD probe new device
	CVideoCapabilities* pVideoCaps = new CVideoCapabilities(
		gtk_entry_get_text(GTK_ENTRY(device_entry)));
}

static void on_signal_menu_activate(GtkWidget *widget, gpointer data)
{
}

static void on_size_menu_activate(GtkWidget *widget, gpointer data)
{
}

static void on_aspect_menu_activate(GtkWidget *widget, gpointer data)
{
}

static bool ValidateAndSave(void)
{
	// copy new values to config

	free(MyConfig->m_videoDeviceName);
	MyConfig->m_videoDeviceName = stralloc(
		gtk_entry_get_text(GTK_ENTRY(device_entry)));

	// TBD 

	MyConfig->m_videoTargetFrameRate =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(frame_rate_spinner));

	MyConfig->m_videoTargetBitRate =
		gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(bit_rate_spinner));

	DisplayVideoSettings();  // display settings in main window

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

void CreateVideoDialog (void) 
{
	GtkWidget* hbox;
	GtkWidget* vbox;
	GtkWidget* label;
	GtkWidget* button;
	GtkObject* adjustment;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Video Settings");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

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

	label = gtk_label_new(" Input Port:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Signal:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Channel List:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Channel:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Size:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Aspect Ratio:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" Frame Rate (fps):");
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
	gtk_entry_set_text(GTK_ENTRY(device_entry), MyConfig->m_videoDeviceName);
	device_modified = false;
	SetEntryValidator(GTK_OBJECT(device_entry),
		GTK_SIGNAL_FUNC(on_changed),
		GTK_SIGNAL_FUNC(on_device_leave));
	gtk_widget_show(device_entry);
	gtk_box_pack_start(GTK_BOX(vbox), device_entry, TRUE, TRUE, 0);

	label = gtk_label_new(" <Option Menu Here>");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	signalIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(signalValues) / sizeof(u_int16_t); i++) {
		if (MyConfig->m_videoSignal == signalValues[i]) {
			signalIndex = i;
			break;
		}
	}
	signal_menu = CreateOptionMenu (NULL,
		(const char**) signalNames, 
		sizeof(signalNames) / sizeof(char*),
		signalIndex,
		GTK_SIGNAL_FUNC(on_signal_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), signal_menu, TRUE, TRUE, 0);

	label = gtk_label_new(" <Option Menu Here>");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	label = gtk_label_new(" <Option Menu Here>");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	sizeIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(sizeWidthValues) / sizeof(u_int16_t); i++) {
		if (MyConfig->m_videoWidth == sizeWidthValues[i]) {
			sizeIndex = i;
			break;
		}
	}
	size_menu = CreateOptionMenu (NULL,
		(const char**) sizeNames, 
		sizeof(sizeNames) / sizeof(char*),
		sizeIndex,
		GTK_SIGNAL_FUNC(on_size_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), size_menu, TRUE, TRUE, 0);

	aspectIndex = 0; 
	for (u_int8_t i = 0; i < sizeof(aspectValues) / sizeof(float); i++) {
		if (MyConfig->m_videoAspectRatio == aspectValues[i]) {
			aspectIndex = i;
			break;
		}
	}
	aspect_menu = CreateOptionMenu (NULL,
		(const char**) aspectNames, 
		sizeof(aspectNames) / sizeof(char*),
		aspectIndex,
		GTK_SIGNAL_FUNC(on_aspect_menu_activate));
	gtk_box_pack_start(GTK_BOX(vbox), aspect_menu, TRUE, TRUE, 0);

	adjustment = gtk_adjustment_new(MyConfig->m_videoTargetFrameRate,
		1, 30, 1, 0, 0);
	frame_rate_spinner = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1, 0);
	gtk_widget_show(frame_rate_spinner);
	gtk_box_pack_start(GTK_BOX(vbox), frame_rate_spinner, TRUE, TRUE, 0);

	adjustment = gtk_adjustment_new(MyConfig->m_videoTargetBitRate,
		50, 1500, 50, 0, 0);
	bit_rate_spinner = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 50, 0);
	gtk_widget_show(bit_rate_spinner);
	gtk_box_pack_start(GTK_BOX(vbox), bit_rate_spinner, TRUE, TRUE, 0);


	// Add standard buttons at bottom
	button = AddButtonToDialog(dialog,
		" OK ", 
		GTK_SIGNAL_FUNC(on_ok_button));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	AddButtonToDialog(dialog,
		" Cancel ", 
		GTK_SIGNAL_FUNC(on_cancel_button));

	gtk_widget_show(dialog);
}

/* end video_dialog.cpp */
