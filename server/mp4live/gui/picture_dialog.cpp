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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#define __STDC_LIMIT_MACROS
#include "mp4live.h"
#include "mp4live_gui.h"


static GtkWidget *dialog = NULL;

static void on_destroy_dialog (GtkWidget *widget, gpointer *data)
{
	gtk_grab_remove(dialog);
	gtk_widget_destroy(dialog);
	dialog = NULL;
} 

static void on_ok_button (GtkWidget *widget, gpointer *data)
{
    on_destroy_dialog(NULL, NULL);
}

static void on_brightness_changed(GtkWidget *widget, gpointer *data)
{
	GtkAdjustment* adjustment = GTK_ADJUSTMENT(widget);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_BRIGHTNESS, 
		(config_integer_t)adjustment->value);

	AVFlow->SetPictureControls();
}

static void on_hue_changed(GtkWidget *widget, gpointer *data)
{
	GtkAdjustment* adjustment = GTK_ADJUSTMENT(widget);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_HUE, 
		(config_integer_t)adjustment->value);

	AVFlow->SetPictureControls();
}

static void on_color_changed(GtkWidget *widget, gpointer *data)
{
	GtkAdjustment* adjustment = GTK_ADJUSTMENT(widget);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_COLOR, 
		(config_integer_t)adjustment->value);

	AVFlow->SetPictureControls();
}

static void on_contrast_changed(GtkWidget *widget, gpointer *data)
{
	GtkAdjustment* adjustment = GTK_ADJUSTMENT(widget);

	MyConfig->SetIntegerValue(CONFIG_VIDEO_CONTRAST, 
		(config_integer_t)adjustment->value);

	AVFlow->SetPictureControls();
}

void CreatePictureDialog (void) 
{
	GtkWidget* hbox;
	GtkWidget* vbox;
	GtkWidget* label;
	GtkWidget* hscale;
	GtkWidget* button;
	GtkObject* adjustment;

	dialog = gtk_dialog_new();
	gtk_signal_connect(GTK_OBJECT(dialog),
		"destroy",
		GTK_SIGNAL_FUNC(on_destroy_dialog),
		&dialog);

	gtk_window_set_title(GTK_WINDOW(dialog), "Picture Settings");
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), 
				     GTK_WINDOW(MainWindow));
#ifndef HAVE_GTK_2_0
	gtk_container_set_resize_mode(GTK_CONTAINER(dialog), GTK_RESIZE_IMMEDIATE);
#endif

	hbox = gtk_hbox_new(FALSE, 1);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
		FALSE, FALSE, 5);

	// first column
	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

	label = gtk_label_new(" Brightness:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	label = gtk_label_new(" Hue:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	label = gtk_label_new(" Color:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	label = gtk_label_new(" Contrast:");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	// second column
	vbox = gtk_vbox_new(TRUE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_VIDEO_BRIGHTNESS),
		0, 100, 1, 1, 1);
	gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		GTK_SIGNAL_FUNC(on_brightness_changed), NULL);
	hscale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
	gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(hscale), 0);
#ifdef HAVE_GTK_2_0
	gtk_widget_set_size_request(GTK_WIDGET(hscale), 200, -1);
#endif
	gtk_widget_show(hscale);
	gtk_box_pack_start(GTK_BOX(vbox), hscale, FALSE, FALSE, 0);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_VIDEO_HUE),
		0, 100, 1, 1, 1);
	gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		GTK_SIGNAL_FUNC(on_hue_changed), NULL);
	hscale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
	gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(hscale), 0);
	gtk_widget_show(hscale);
	gtk_box_pack_start(GTK_BOX(vbox), hscale, FALSE, FALSE, 0);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_VIDEO_COLOR),
		0, 100, 1, 1, 1);
	gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		GTK_SIGNAL_FUNC(on_color_changed), NULL);
	hscale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
	gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(hscale), 0);
	gtk_widget_show(hscale);
	gtk_box_pack_start(GTK_BOX(vbox), hscale, FALSE, FALSE, 0);

	adjustment = gtk_adjustment_new(
		MyConfig->GetIntegerValue(CONFIG_VIDEO_CONTRAST),
		0, 100, 1, 1, 1);
	gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		GTK_SIGNAL_FUNC(on_contrast_changed), NULL);
	hscale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
	gtk_scale_set_value_pos(GTK_SCALE(hscale), GTK_POS_RIGHT);
	gtk_scale_set_digits(GTK_SCALE(hscale), 0);
	gtk_widget_show(hscale);
	gtk_box_pack_start(GTK_BOX(vbox), hscale, FALSE, FALSE, 0);

	// Add standard buttons at bottom
	button = AddButtonToDialog(dialog,
		" OK ", 
		GTK_SIGNAL_FUNC(on_ok_button));
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);

	gtk_widget_show(dialog);
}

