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

#ifndef __GUIUTILS_H__
#define __GUIUTILS_H__

#include "mpeg4ip.h"

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
                            gpointer data);

GtkWidget *CreateSubMenu (GtkWidget *menubar, char *szName);

GtkWidget *CreateBarSubMenu (GtkWidget *menu, char *szName);

GtkWidget *CreateMenuRadio (GtkWidget *menu, 
                            char *szName, 
                            GSList **group,
                            GtkSignalFunc func, 
                            gpointer data);

GtkWidget *CreateOptionMenu(GtkWidget *old,
			    const char **names,
			    uint32_t max,
			    uint32_t current_index);

GtkWidget *CreateOptionMenu(GtkWidget *old,
			    const char* (*gather_func)(uint32_t index, void* pUserData),
			    void* pUserData,
			    uint32_t max,
			    uint32_t current_index);
  
int GetNumberEntryValue(GtkWidget *entry, uint32_t *result);
  
void SetNumberEntryValue(GtkWidget *entry, uint32_t value);
  
GtkWidget *AddNumberEntryBoxWithLabel(GtkWidget *vbox,
				      const char *label_name,
				      uint32_t value,
				      uint32_t value_len);
  
GtkWidget *AddButtonToDialog(GtkWidget *dialog,
			     const char *name,
			     GtkSignalFunc on_click);

GtkWidget *AddEntryBoxWithLabel(GtkWidget *vbox,
				const char *label_name,
				const char *initial_value,
				uint32_t value_len);
  
void FreeChild(GtkWidget *widget);

void SetEntryValidator(GtkObject* object, 
	GtkSignalFunc changed_func, GtkSignalFunc leave_func);

// from gui_showmsg.cpp
GtkWidget* ShowMessage (
	const char *szTitle, 
	const char *szMessage,
	GtkWidget *infront = NULL,
	bool userDismiss = true);

void CloseShowMessage(
	GtkWidget* parent, 
	gpointer widget);

#define CreateMenuItemSeperator(menu) \
	CreateMenuItem(menu, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

void YesOrNo (const char *szTitle,
	      const char *szMessage,
	      int yes_as_default,
	      GtkSignalFunc yesroutine,
	      GtkSignalFunc noroutine);

#endif /* __GUIUTILS_H__ */

