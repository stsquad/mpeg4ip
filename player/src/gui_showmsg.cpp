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
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * Showmessage.c  from Developing Linux Applications
 */

#include <gtk/gtk.h>
#include "gui_utils.h"

/*
 * CloseShowMessage
 *
 * Routine to close the about dialog window.
 */
static void CloseShowMessage (GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog_widget = (GtkWidget *) data;

    gtk_grab_remove (dialog_widget);

    /* --- Close the widget --- */
    gtk_widget_destroy (dialog_widget);
}



/*
 * ClearShowMessage
 *
 * Release the window "grab" 
 * Clear out the global dialog_window since that
 * is checked when the dialog is brought up.
 */
static void ClearShowMessage (GtkWidget *widget, gpointer data)
{
    gtk_grab_remove (widget);
}


/*
 * ShowMessage
 *
 * Show a popup message to the user.
 */
void ShowMessage (const char *szTitle, const char *szMessage)
{
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *dialog_window;

    /* --- Create a dialog window --- */
    dialog_window = gtk_dialog_new ();

    gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy",
              GTK_SIGNAL_FUNC (ClearShowMessage),
              NULL);

    /* --- Set the title and add a border --- */
    gtk_window_set_title (GTK_WINDOW (dialog_window), szTitle);
    gtk_container_border_width (GTK_CONTAINER (dialog_window), 0);

    /* --- Create an "Ok" button with the focus --- */
    button = gtk_button_new_with_label ("OK");

    gtk_signal_connect (GTK_OBJECT (button), "clicked",
              GTK_SIGNAL_FUNC (CloseShowMessage),
              dialog_window);

    /* --- Default the "Ok" button --- */
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), 
              button, TRUE, TRUE, 0);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);

    /* --- Create a descriptive label --- */
    label = gtk_label_new (szMessage);

    /* --- Put some room around the label text --- */
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);

    /* --- Add label to designated area on dialog --- */
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), 
              label, TRUE, TRUE, 0);

    /* --- Show the label --- */
    gtk_widget_show (label);


    /* --- Show the dialog --- */
    gtk_widget_show (dialog_window);

    /* --- Only this window can have actions done. --- */
    gtk_grab_add (dialog_window);
}

