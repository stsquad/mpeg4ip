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
 * gui_utils.h - various utility programs gotten from the book or
 * other gtk applications
 */
#ifdef __cplusplus
extern "C" {
#endif
GtkWidget *CreateWidgetFromXpm (GtkWidget *widget, gchar **xpm_data);

GtkWidget *CreateMenuItem (GtkWidget *menu,
			   GtkAccelGroup *accel_group,
			   GtkTooltips *tooltips,
                           char *szName, 
                           char *szAccel,
                           char *szTip, 
                           GtkSignalFunc func,
                           gpointer data);

GtkWidget *CreateMenuCheck (GtkWidget *menu, 
                            char *szName, 
                            GtkSignalFunc func, 
                            gpointer data,
			    gboolean initial_data);

GtkWidget *CreateSubMenu (GtkWidget *menubar, char *szName);

GtkWidget *CreateBarSubMenu (GtkWidget *menu, char *szName);

GtkWidget *CreateMenuRadio (GtkWidget *menu, 
                            char *szName, 
                            GSList **group,
                            GtkSignalFunc func, 
                            gpointer data);
void FreeChild(GtkWidget *widget);

  // from gui_showmsg.c
void ShowMessage (const char *szTitle, const char *szMessage);
#define CreateMenuItemSeperator(menu) CreateMenuItem(menu, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

void CreateLogLevelSubmenu(GtkWidget *menu,
			   char *szName,
			   int active,
			   GtkSignalFunc func);

#ifdef __cplusplus
}
#endif
