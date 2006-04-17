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
 * Copyright (C) Cisco Systems Inc. 2000-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 *            Peter Maersk-Moller     peter@maersk-moller.net
 */
/*
 * gui_main.cpp - Contains the gtk based gui for this player.
 */
#include <stdlib.h>
#include <stdio.h>
#include "player_session.h"
#include "player_media.h"
#include <glib.h>
#include <gtk/gtk.h>
#include "gui_utils.h"
#include "media_utils.h"
#include "player_util.h"
#include "gui_xpm.h"
#include <syslog.h>
#include <rtsp/rtsp_client.h>
#include "our_config_file.h"
#include "playlist.h"
#include <libhttp/http.h>
#include <rtp/debug.h>
#include "codec_plugin_private.h"
#include <mpeg2t/mpeg2_transport.h>
#include "mpeg4ip_getopt.h"
#include "mpeg2_ps.h"
/* ??? */
#ifndef LOG_PRI
#define LOG_PRI(p) ((p) & LOG_PRIMASK)
#endif

/* Local variables */
static GtkWidget *main_window;
static GtkWidget *main_vbox;
static GtkWidget *close_menuitem, *seek_menuitem;
static GtkAccelGroup *accel_group = NULL;
static GtkTooltips         *tooltips = NULL;
static GList *playlist = NULL;
static GtkWidget *combo;

static volatile enum {
  PLAYING_NONE,
  PLAYING,
  STOPPED,
  PAUSED,
} play_state = PLAYING_NONE;
static CPlaylist *master_playlist;
static CPlayerSession *psptr = NULL;
static CMsgQueue master_queue;
static GtkWidget *play_button;
static GtkWidget *pause_button;
static GtkWidget *stop_button;
//static GtkWidget *rewind_button;
//static GtkWidget *ff_button;
static GtkWidget *speaker_button;
static GtkWidget *volume_slider;
static GtkWidget *time_slider;
static GtkWidget *time_disp;
static GtkWidget *session_desc[5];
static SDL_mutex *command_mutex;
static bool master_looped;
static int master_volume;
static bool master_muted;
static int master_fullscreen = 0;
static uint64_t master_time;
static int time_slider_pressed = 0;
static int master_screen_size = 100;
static const char *last_entry = NULL;
static const gchar *last_file = NULL;
static int doing_seek = 0;
static GtkWidget *videoradio[3];
static GtkWidget *aspectratio[6];

static GtkTargetEntry drop_types[] = 
{
  { "text/plain", 0, 1 },
};

static void media_list_query (CPlayerSession *psptr,
			      uint num_video, 
			      video_query_t *vq,
			      uint num_audio,
			      audio_query_t *aq,
			      uint num_text,
			      text_query_t *tq)
{
  if (num_video > 0) {
    if (config.get_config_value(CONFIG_PLAY_VIDEO) != 0) {
      vq[0].enabled = 1;
    } 
  }
  if (num_audio > 0) {
    if (config.get_config_value(CONFIG_PLAY_AUDIO) != 0) {
      aq[0].enabled = 1;
    } 
  }
  if (num_text > 0) {
    if (config.get_config_value(CONFIG_PLAY_TEXT) != 0) {
      tq[0].enabled = 1;
    } 
  }
}

static control_callback_vft_t cc_vft = {
  media_list_query,
};
/*
 * toggle_button_adjust - make sure a toggle button reflects the correct
 * state
 */
static void toggle_button_adjust (GtkWidget *button, int state)
{
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == state)
    return;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), state);
}

/*
 * close_session - delete the session, and set the various gui pieces
 */
static void close_session (void)
{
  if (psptr != NULL) {
    delete psptr;
    psptr = NULL;
    play_state = PLAYING_NONE;
    gtk_widget_set_sensitive(close_menuitem, 0);
    gtk_widget_set_sensitive(seek_menuitem, 0);
    toggle_button_adjust(play_button, FALSE);
    toggle_button_adjust(pause_button, FALSE);
    toggle_button_adjust(stop_button, FALSE);
    gtk_widget_set_sensitive(stop_button, 0);
    gtk_widget_set_sensitive(play_button, 0);
    gtk_widget_set_sensitive(pause_button, 0);
    gtk_widget_set_sensitive(time_slider, 0);
  }
}

/*
 * When we hit play, adjust the gui
 */
static void adjust_gui_for_play (void)
{
  gtk_widget_set_sensitive(close_menuitem, 1);
  gtk_widget_set_sensitive(play_button, 1);
  gtk_widget_set_sensitive(pause_button, 1);
  gtk_widget_set_sensitive(stop_button, 1);
#ifdef HAVE_GTK_2_0
  gtk_range_set_value(GTK_RANGE(time_slider), 0.0);
#endif
  if (psptr->session_is_seekable()) {
    gtk_widget_set_sensitive(time_slider, 1);
    gtk_widget_set_sensitive(seek_menuitem, 1);
  }
  play_state = PLAYING;
  toggle_button_adjust(play_button, TRUE);
  toggle_button_adjust(stop_button, FALSE);
  toggle_button_adjust(pause_button, FALSE);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), last_entry);
  gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(combo)->entry), 0, -1);
  gtk_editable_set_position(GTK_EDITABLE(GTK_COMBO(combo)->entry), -1);
  gtk_widget_grab_focus(combo);
}

static void set_aspect_ratio(uint32_t newaspect)
{
  // Unfortunately psptr is NULL upon startup so we won't set aspect ratio
  // correct

  if (config.get_config_value(CONFIG_ASPECT_RATIO) == newaspect) {
    return;
  }

  if (newaspect > 5) newaspect = 0;
  config.set_config_value(CONFIG_ASPECT_RATIO, newaspect);
  if (psptr != NULL) {
    switch (newaspect) {
    case 1 : 
      psptr->set_screen_size(master_screen_size/50,master_fullscreen,4,3);
      break;
    case 2 : 
      psptr->set_screen_size(master_screen_size/50,master_fullscreen,16,9);
      break;
    case 3 : 
      psptr->set_screen_size(master_screen_size/50,master_fullscreen,185,100);
      break;
    case 4 : 
      psptr->set_screen_size(master_screen_size/50,master_fullscreen,235,100);
      break;
    case 5:
      psptr->set_screen_size(master_screen_size/50,master_fullscreen, 1, 1);
      break;
    default: 
      psptr->set_screen_size(master_screen_size/50,master_fullscreen,0,0);
      newaspect = 0;
      break;
    }
  } 
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(aspectratio[newaspect]), TRUE);
  
}

static void create_session_from_name (const char *name)
{
  gint x, y, w, h;
  GdkWindow *window;
  //  int display_err = 0;
    
  window = GTK_WIDGET(main_window)->window;
  gdk_window_get_position(window, &x, &y);
  gdk_window_get_size(window, &w, &h);
  y += h + 40;

  if (config.get_config_value(CONFIG_PLAY_AUDIO) == 0 &&
      config.get_config_value(CONFIG_PLAY_VIDEO) == 0 &&
      config.get_config_value(CONFIG_PLAY_TEXT) == 0) {
    ShowMessage("Hey Dummy", "You have both audio and video disabled");
    return;
  }

  // If we're already running a session, close it.
  if (psptr != NULL) {
    close_session();
  }

  //  bool do_delete = false;

  psptr = start_session(&master_queue, 
			NULL,
			NULL,
			name, 
			&cc_vft,
			master_muted ? 0 : master_volume,
			x, 
			y, 
			master_screen_size / 50);
  if (psptr != NULL) {
    adjust_gui_for_play();
  }
}
/*
 * start a session
 */
static void start_session_from_name (const char *name)
{
  GList *p;
  int in_list = 0;

  // Save the last file, so we can use it next time we do a file open
  if (last_entry != NULL) {
    free((void *)last_entry);
  }
  last_entry = strdup(name);
  if (strstr(name, "://") == NULL) {
    if (last_file != NULL) {
      free((void *)last_file);
      last_file = NULL;
    }
    last_file = g_strdup(name);
    config.set_config_string(CONFIG_PREV_DIRECTORY, last_file);
  }

  // See if entry is already in the list.
  p = g_list_first(playlist);
  while (p != NULL && in_list == 0) {
    if (g_strcasecmp(name, (const gchar *)p->data) == 0) {
      in_list = 1;
    }
    p = g_list_next(p);
  }
  
  SDL_mutexP(command_mutex);
  
  // If we're running a playlist, close it now.
  if (master_playlist != NULL) {
    delete master_playlist;
    master_playlist = NULL;
  }

  // Create a new player session
  const char *suffix = strrchr(name, '.');
  if ((suffix != NULL) && 
      ((strcasecmp(suffix, ".mp4plist") == 0) ||
       (strcasecmp(suffix, ".mxu") == 0) ||
       (strcasecmp(suffix, ".m4u") == 0) ||
       (strcasecmp(suffix, ".gmp4_playlist") == 0))) {
    const char *errmsg = NULL;
    master_playlist = new CPlaylist(name, &errmsg);
    if (errmsg != NULL) {
      ShowMessage("Playlist error", errmsg);
    } else {
      create_session_from_name(master_playlist->get_first());
    }
  } else {
    create_session_from_name(name);
  }

  // Add this file to the drop down list
  // If we're playing, adjust the gui
  if (psptr != NULL) {

    if (in_list == 0) {
      config.set_config_string(CONFIG_PREV_FILE_3, 
				config.get_config_string(CONFIG_PREV_FILE_2));
      config.set_config_string(CONFIG_PREV_FILE_2, 
				config.get_config_string(CONFIG_PREV_FILE_1));
      config.set_config_string(CONFIG_PREV_FILE_1, 
				config.get_config_string(CONFIG_PREV_FILE_0));
      config.set_config_string(CONFIG_PREV_FILE_0, name);
    }
  }
  if (in_list == 0) {
    gchar *newone = g_strdup(name);
    playlist = g_list_append(playlist, newone);
    gtk_combo_set_popdown_strings (GTK_COMBO(combo), playlist);
    gtk_widget_show(combo);
  }
  SDL_mutexV(command_mutex);
}

/*
 * delete_event - called when window closed
 */
void delete_event (GtkWidget *widget, gpointer *data)
{
  if (psptr != NULL) {
    delete psptr;
  }
  if (master_playlist != NULL) {
    delete master_playlist;
  }
  if (config.get_config_string(CONFIG_LOG_FILE) != NULL) {
    close_log_file();
  }
  config.WriteToFile();
  close_plugins();
  gtk_main_quit();
}

static void on_main_menu_close (GtkWidget *window, gpointer data)
{
  SDL_mutexP(command_mutex);
  close_session();
  master_fullscreen = 0;
  SDL_mutexV(command_mutex);
}

static GtkWidget *seek_dialog;

static void on_seek_destroy (GtkWidget *window, gpointer data)
{
  gtk_widget_destroy(window);
  doing_seek = 0;
}
static void on_seek_activate (GtkWidget *window, gpointer data)
{
  const gchar *entry = 
    gtk_entry_get_text(GTK_ENTRY(data));
  double time;
  bool good = true;
  if (strchr(entry, ':') == NULL) {
    if (sscanf(entry, "%lf", &time) != 1) {
      good = false;
    }
  } else {
    int hr, min;
    hr = 0; 
    min = 0;
    if (sscanf(entry, "%u", &hr) == 1) {
      entry = strchr(entry, ':') + 1;
      if (strchr(entry, ':') == NULL) {
	min = hr;
	hr = 0;
      } else {
	if (sscanf(entry, "%u", &min) != 1) {
	  good = false;
	} else 
	  entry = strchr(entry, ':') + 1;
      }
      if (good) {
	if (sscanf(entry, "%lf", &time) == 1) {
	  time += ((hr * 60) + min) * 60;
	} else good = false;
      }
    } else good = false;
  }
	
  if (good && psptr && time < psptr->get_max_time()) {
    if (play_state == PLAYING) {
      psptr->pause_all_media();
    }
    char errmsg[512];
    int ret = 
	psptr->play_all_media(time == 0.0 ? TRUE : FALSE, 
			      time);
    if (ret == 0) {
      adjust_gui_for_play();
    }
    if (ret != 0) {
      char buffer[1024];
      close_session();
      snprintf(buffer, sizeof(buffer), "Error seeking session: %s", 
	       errmsg);
      ShowMessage("Play error", buffer);
    } 
  }
  on_seek_destroy(seek_dialog, data);
}

static void on_seek_cancel (GtkWidget *window, gpointer data)
{
  on_seek_destroy(seek_dialog, data);
}
static void on_menu_seek (GtkWidget *window, gpointer data)
{
  if (master_fullscreen || doing_seek != 0) return;

  seek_dialog = gtk_dialog_new();
  gtk_signal_connect(GTK_OBJECT(seek_dialog),
		     "destroy",
		     GTK_SIGNAL_FUNC(on_seek_destroy),
		     seek_dialog);
  gtk_window_set_title(GTK_WINDOW(seek_dialog), "Seek to");
  gtk_container_border_width(GTK_CONTAINER(seek_dialog), 5);

  GtkWidget *vbox;
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(seek_dialog)->action_area),
		     vbox, 
		     TRUE, 
		     TRUE,
		     0);
  gtk_widget_show(vbox);
  GtkWidget *hbox;
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  GtkWidget *seek_entry;
  seek_entry = gtk_entry_new();
  gtk_signal_connect(GTK_OBJECT(seek_entry),
		     "activate",
		     GTK_SIGNAL_FUNC(on_seek_activate),
		     seek_entry);
  gtk_box_pack_start(GTK_BOX(hbox), seek_entry, TRUE, TRUE, 0);
  gtk_widget_show(seek_entry);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
  GtkWidget *button;
  button = gtk_button_new_with_label("Ok");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(on_seek_activate),
		     seek_entry);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(on_seek_cancel),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_widget_show(button);
 
  gtk_widget_show(seek_dialog);
  doing_seek = 1;
}

static void on_main_menu_help (GtkWidget *window, gpointer data)
{
}

static void on_main_menu_about (GtkWidget *window, gpointer data)
{
  char buffer[1024];
  sprintf(buffer,
	  "gmp4player Version %s.\n"
	  "An open source file/streaming MPEG4 player\n"
	  "Developed by Cisco Systems using the\n"
	  "following open source packages:\n"
	  "\n"
	  "SDL, SMPEG audio (MP3) from lokigames\n"
	  "RTP from UCL\n"
	  "ISO reference decoder for MPEG4\n"
	  "FAAC decoder\n"
	  "Developed by Bill May, 10/00 to present", MPEG4IP_VERSION
	  );

  ShowMessage("About gmp4player",buffer);
}

/*
 * on_drag_data_received - copied from gtv, who copied from someone else
 */
static void on_drag_data_received (GtkWidget *widget,
				   GdkDragContext *context,
				   gint x,
				   gint y,
				   GtkSelectionData *selection_data,
				   guint info,
				   guint time)
{
    gchar *temp, *string;

    string = (gchar *)selection_data->data;

    /* remove newline at end of line, and the file:// url header
       at the begining, copied this code from the xmms source */
    temp = strchr(string, '\n');
    if (temp)
    {
        if (*(temp - 1) == '\r')
            *(temp - 1) = '\0';
        *temp = '\0';
    }
    if (!strncasecmp(string, "file:", 5))
        string = string + 5;
    start_session_from_name(string);
}


static void on_filename_selected (GtkFileSelection *selector, 
				  gpointer user_data) 
{
  const gchar *name;

  name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(user_data));
  start_session_from_name(name);
}

/*
 * This is right out of the book...
 */
static void enable_file_select (GtkSignalFunc func,
				const char *header, 
				const char *file)
{
  GtkWidget *ret;
  ret = gtk_file_selection_new(header);
  if (file != NULL) {
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(ret), file);
  }
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(ret));
  gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(ret)->ok_button),
		      "clicked", 
		      GTK_SIGNAL_FUNC(func), 
		      ret);
                             
  /* Ensure that the dialog box is destroyed when the user clicks a button. */
     
  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(ret)->ok_button),
			    "clicked", 
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(ret));

  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(ret)->cancel_button),
			    "clicked", 
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(ret));
  
  /* Display that dialog */
     
  gtk_widget_show (ret);
  
}

static void on_browse_button_clicked (GtkWidget *window, gpointer data)
{
  enable_file_select(GTK_SIGNAL_FUNC(on_filename_selected),
		     "Open Media File", 
		     last_file);
}

static void on_logfile_selected (GtkFileSelection *selected,
				 gpointer user_data)
{
  const gchar *name;
  name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(user_data));
  config.set_config_string(CONFIG_LOG_FILE, name);
  open_log_file(name);
}

static void on_main_menu_set_log_file (GtkWidget *window, gpointer data)
{
   enable_file_select(GTK_SIGNAL_FUNC(on_logfile_selected),
		     "Select Log File", 
		     config.get_config_string(CONFIG_LOG_FILE));
}

static void on_main_menu_clear_log_file (GtkWidget *window, gpointer data)
{
  if (config.get_config_string(CONFIG_LOG_FILE) != NULL) {
    clear_log_file();
  }
}

static void on_playlist_child_selected (GtkWidget *window, gpointer data)
{
  const gchar *entry = 
    gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
  if (strcmp(entry, "") == 0) 
    return;
  if (last_entry && strcmp(entry, last_entry) == 0) 
    return;
  start_session_from_name(entry);
}

static void on_play_list_selected (GtkWidget *window, gpointer data)
{
  const gchar *entry = gtk_entry_get_text(GTK_ENTRY(window));
  start_session_from_name(entry);
}

static void on_play_clicked (GtkWidget *window, gpointer data)
{
  int ret;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(play_button)) == FALSE) {
    if (play_state == PLAYING) {
      toggle_button_adjust(play_button, TRUE);
    }
    return;
  }

  if (psptr == NULL)
    return;
  ret = 0;
  SDL_mutexP(command_mutex);
  switch (play_state) {
  case PAUSED:
    ret = psptr->play_all_media(FALSE, 0.0);
    break;
  case STOPPED:
    ret = psptr->play_all_media(TRUE, 0.0);
    break;
  default:
    break;
  }
  if (ret == 0)
    adjust_gui_for_play();
  SDL_mutexV(command_mutex);
  if (ret != 0) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Error re-starting session: %s", 
	     psptr->get_message());
    ShowMessage("Play error", buffer);
    close_session();
  }
}

static void do_pause (void)
{
  SDL_mutexP(command_mutex);
  if (psptr != NULL && play_state == PLAYING) {
    play_state = PAUSED;
    toggle_button_adjust(play_button, FALSE);
    toggle_button_adjust(pause_button, TRUE);
    psptr->pause_all_media();
  } else {
    toggle_button_adjust(pause_button, play_state == PAUSED);
  }
  SDL_mutexV(command_mutex);
}
static void on_pause_clicked (GtkWidget *window, gpointer data)
{
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pause_button)) == FALSE) {
    if (play_state == PAUSED) {
      toggle_button_adjust(pause_button, TRUE);
    }
    return;
  }
  do_pause();
}

static void on_stop_clicked (GtkWidget *window, gpointer data)
{
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(stop_button)) == FALSE) {
    if (play_state == STOPPED) {
      toggle_button_adjust(stop_button, TRUE);
    }
    return;
  }
  SDL_mutexP(command_mutex);
  if (psptr != NULL && play_state == PLAYING) {
    play_state = STOPPED;
    toggle_button_adjust(play_button, FALSE);
    toggle_button_adjust(stop_button, TRUE);
    psptr->pause_all_media();
  } else {
    toggle_button_adjust(stop_button, play_state == STOPPED);
  }
  SDL_mutexV(command_mutex);
}

/*
 * Set the speaker button to either mute or playing
 */
static void set_speaker_button (gchar **xpm_data)
{
  GtkWidget *wid;
  wid = CreateWidgetFromXpm(speaker_button, xpm_data);
  FreeChild(speaker_button);
  gtk_container_add(GTK_CONTAINER(speaker_button), wid);
}

static void on_speaker_clicked (GtkWidget *window, gpointer data)
{
  int vol;
  if (master_muted == false) {
    // mute the puppy
    set_speaker_button(xpm_speaker_muted);
    master_muted = true;
    vol = 0;
  } else {
    set_speaker_button(xpm_speaker);
    master_muted = false;
    vol  = master_volume;
  }
  config.SetBoolValue(CONFIG_MUTED, master_muted);
  if (psptr && psptr->session_has_audio()) {
    psptr->set_audio_volume(vol);
  }
}
static void volume_adjusted (int volume)
{
  GtkAdjustment *val = gtk_range_get_adjustment(GTK_RANGE(volume_slider));
  gtk_adjustment_set_value(val, (gfloat)volume);
  master_volume = volume;
  config.set_config_value(CONFIG_VOLUME, master_volume);
  gtk_range_set_adjustment(GTK_RANGE(volume_slider), val);
#ifndef HAVE_GTK_2_0
  gtk_range_slider_update(GTK_RANGE(volume_slider));
  gtk_range_clear_background(GTK_RANGE(volume_slider));
#endif
   if (master_muted == false && psptr && psptr->session_has_audio()) {
     psptr->set_audio_volume(master_volume);
   }
}

static void on_volume_adjusted (GtkWidget *window, gpointer data)
{
  GtkAdjustment *val = gtk_range_get_adjustment(GTK_RANGE(volume_slider));
  volume_adjusted((int)val->value);
}

static void on_debug_display_status (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;
  checkmenu = GTK_CHECK_MENU_ITEM(window);
  config.SetBoolValue(CONFIG_DISPLAY_DEBUG,
		      checkmenu->active == FALSE ? false : true);
}

static void on_debug_mpeg4isoonly (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;
  config_index_t iso_only = config.FindIndexByName("Mpeg4IsoOnly");

  checkmenu = GTK_CHECK_MENU_ITEM(window);

  if (iso_only != UINT32_MAX) {
    config.SetBoolValue(iso_only,
			checkmenu->active == FALSE ? 0 : 1);
  } else {
    ShowMessage("Error", "Mpeg4IsoOnly config variable not loaded");
  }
}
static void on_debug_oldxvid (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;
  config_index_t old_xvid = config.FindIndexByName("UseOldXvid");
  if (old_xvid == UINT32_MAX) {
    ShowMessage("Error", "UseOldXvid config variable not loaded");
  } else {
    checkmenu = GTK_CHECK_MENU_ITEM(window);
    config.SetBoolValue(old_xvid, 
			checkmenu->active == FALSE ? 0 : 1);
  }
}
static void on_debug_usempeg3 (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;
  config_index_t mpeg3 = config.FindIndexByName("UseMpeg3");
  if (mpeg3 == UINT32_MAX) {
    ShowMessage("Error", "UseMpeg3 config variable not loaded");
  } else {
    checkmenu = GTK_CHECK_MENU_ITEM(window);
    config.SetBoolValue(mpeg3, 
			checkmenu->active == FALSE ? 0 : 1);
  }
}

static void on_debug_use_old_mp4_lib (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;

  checkmenu = GTK_CHECK_MENU_ITEM(window);

  config.set_config_value(CONFIG_USE_OLD_MP4_LIB,
			  checkmenu->active == FALSE ? 0 : 1);
}

static void on_debug_http (GtkWidget *window, gpointer data)
{
  int loglevel = GPOINTER_TO_INT(data);

  http_set_loglevel(LOG_PRI(loglevel));
  config.set_config_value(CONFIG_HTTP_DEBUG, LOG_PRI(loglevel));
}

static void on_debug_rtsp (GtkWidget *window, gpointer data)
{
  int loglevel = GPOINTER_TO_INT(data);

  rtsp_set_loglevel(LOG_PRI(loglevel));
  config.set_config_value(CONFIG_RTSP_DEBUG, LOG_PRI(loglevel));
}
  
static void on_debug_rtp (GtkWidget *window, gpointer data)
{
  int loglevel = GPOINTER_TO_INT(data);
  rtp_set_loglevel(LOG_PRI(loglevel));
  config.set_config_value(CONFIG_RTP_DEBUG, LOG_PRI(loglevel));
}

static void on_debug_sdp (GtkWidget *window, gpointer data)
{
  int loglevel = GPOINTER_TO_INT(data);

  sdp_set_loglevel(LOG_PRI(loglevel));
  config.set_config_value(CONFIG_SDP_DEBUG, LOG_PRI(loglevel));
}

static void on_debug_mpeg2t (GtkWidget *window, gpointer data)
{
  int loglevel = GPOINTER_TO_INT(data);

  mpeg2t_set_loglevel(LOG_PRI(loglevel));
  config.set_config_value(CONFIG_MPEG2T_DEBUG, LOG_PRI(loglevel));
}
static void on_debug_mpeg2ps (GtkWidget *window, gpointer data)
{
  int loglevel = GPOINTER_TO_INT(data);

  mpeg2ps_set_loglevel(LOG_PRI(loglevel));
  config.set_config_value(CONFIG_MPEG2PS_DEBUG, LOG_PRI(loglevel));
}

static void on_media_play_audio (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;
  checkmenu = GTK_CHECK_MENU_ITEM(window);

  config.set_config_value(CONFIG_PLAY_AUDIO, checkmenu->active);
  if (psptr != NULL) {
    ShowMessage("Warning", "Play audio will not take effect until next session");
  }
}

static void on_media_play_text (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;
  checkmenu = GTK_CHECK_MENU_ITEM(window);

  config.set_config_value(CONFIG_PLAY_TEXT, checkmenu->active);
  if (psptr != NULL) {
    ShowMessage("Warning", "Play text will not take effect until next session");
  }
}
static void on_media_play_video (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;
  checkmenu = GTK_CHECK_MENU_ITEM(window);

  config.set_config_value(CONFIG_PLAY_VIDEO, checkmenu->active);
  if (psptr != NULL) {
    ShowMessage("Warning", "Play video will not take effect until next session");
  }
}

static void on_network_rtp_over_rtsp (GtkWidget *window, gpointer data)
{
  GtkCheckMenuItem *checkmenu;

  checkmenu = GTK_CHECK_MENU_ITEM(window);


  config.set_config_value(CONFIG_USE_RTP_OVER_RTSP,
			  checkmenu->active);
}
  
static void change_video_size (int newsize)
{
  if (master_screen_size == newsize) return;

  master_screen_size = newsize;
  if (psptr != NULL) {
    psptr->set_screen_size(newsize / 50);
  }
  int index;
  if (newsize == 50) index = 0;
  else if (newsize == 100) index = 1;
  else index = 2;
#ifdef HAVE_GTK_2_0
  if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(videoradio[index])))
#else
    if (!(GTK_CHECK_MENU_ITEM(videoradio[index])->active)) 
#endif
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(videoradio[index]), TRUE);

}

static void on_video_radio (GtkWidget *window, gpointer data)
{
#ifdef HAVE_GTK_2_0
  if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(window)))
#else
    if (!(GTK_CHECK_MENU_ITEM(window)->active)) 
#endif
    return;

  int newsize = GPOINTER_TO_INT(data);
  if (newsize != master_screen_size) {
    change_video_size(newsize);
  }
}

static void on_aspect_ratio (GtkWidget *window, gpointer data)
{
#ifdef HAVE_GTK_2_0
  if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(window)))
#else
    if (!(GTK_CHECK_MENU_ITEM(window)->active)) 
#endif
    return;
  int newaspect = GPOINTER_TO_INT(data);
  if (psptr != NULL) set_aspect_ratio(newaspect);
}

static void on_video_fullscreen (GtkWidget *window, gpointer data)
{
  if (psptr != NULL) {
    master_fullscreen = 1;
    psptr->set_screen_size(master_screen_size / 50, 1);
  }
}

static int on_time_slider_pressed (GtkWidget *window, gpointer data)
{
  time_slider_pressed = 1;
  return (FALSE);
}

static int on_time_slider_adjusted (GtkWidget *window, gpointer data)
{
  double maxtime, newtime;
  time_slider_pressed = 0;
  if (psptr == NULL) 
    return FALSE;
  maxtime = psptr->get_max_time();
  if (maxtime == 0.0)
    return FALSE;
  gdouble val;
#ifdef HAVE_GTK_2_0
  val = gtk_range_get_value(GTK_RANGE(time_slider));
#else
  GtkAdjustment *adj_val = gtk_range_get_adjustment(GTK_RANGE(time_slider));
  val = adj_val->value;
#endif
  newtime = (maxtime * val) / 100.0;
  SDL_mutexP(command_mutex);
  if (play_state == PLAYING) {
    psptr->pause_all_media();
  }
  // If we're going all the way back to the beginning, indicate that
  int ret = psptr->play_all_media(newtime == 0.0 ? TRUE : FALSE, newtime);
  if (ret == 0) 
    adjust_gui_for_play();
  SDL_mutexV(command_mutex);
  if (ret != 0) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "Error re-starting session: %s", 
	     psptr->get_message());
    ShowMessage("Play error", buffer);
    close_session();
  } 
  return FALSE;
}

static void on_loop_enabled_button (GtkWidget *widget, gpointer *data)
{
  master_looped = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  config.SetBoolValue(CONFIG_LOOPED, master_looped);
}

/*
 * Main timer routine - runs ~every 500 msec
 * Might be able to adjust this to handle just when playing.
 */
static bool one_sec = false;
static gint main_timer (gpointer raw)
{
  uint64_t play_time;
  char buffer[1024];
  if (play_state == PLAYING) {
    flush_log_file();

    if (one_sec == false) {
      one_sec = true;
    } else {
      if (config.GetBoolValue(CONFIG_DISPLAY_DEBUG))
	psptr->display_status();
      one_sec = false;
    }
    double max_time = psptr->get_max_time();
    uint64_t pt = psptr->get_playing_time();
    double playtime = ((double)pt) / 1000.0;
    double val;

    val = max_time;
    if (time_slider_pressed == 0 && 
	val > 0.0 &&
	psptr->get_session_state() == SESSION_PLAYING) {
      if (playtime < 0) {
	val = 0.0;
      } else if (playtime > val) {
	val = 100.0;
      } else {
	val = (playtime * 100.0) / val;
      }
#ifdef HAVE_GTK_2_0
      gtk_range_set_value(GTK_RANGE(time_slider), val);
#else
      GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(time_slider));
      adj->value = val;
      gtk_range_set_adjustment(GTK_RANGE(time_slider), adj);
      gtk_range_slider_update(GTK_RANGE(time_slider));
      gtk_range_clear_background(GTK_RANGE(time_slider));
      gtk_range_draw_background(GTK_RANGE(time_slider));
      gtk_widget_show(time_slider);
#endif
    }
    int hr, min, tot;
    tot = (int) playtime;
    hr = tot / 3600;
    tot %= 3600;
    min = tot / 60;
    tot %= 60;
    playtime -= (double)((hr * 3600) + (min * 60));
    if (max_time == 0.0) {
      gchar buffer[30];
      g_snprintf(buffer, 30, "%d:%02d:%04.1f", hr, min, playtime);
      gtk_label_set_text(GTK_LABEL(time_disp), buffer);
    } else {
      int mhr, mmin, msec, mtot;
      mtot = (int)max_time;
      mhr = mtot / 3600;
      mtot %= 3600;
      mmin = mtot / 60;
      msec = mtot % 60;

      gchar buffer[60];
      g_snprintf(buffer, 60, "%d:%02d:%04.1f of %d:%02d:%02d", 
		 hr, min, playtime, mhr, mmin, msec);
      gtk_label_set_text(GTK_LABEL(time_disp), buffer);
    }
    
    for (int ix = 0; ix < 4; ix++) {
      const char *text;
      text = psptr->get_session_desc(ix);
      if (text != NULL) {
	gtk_label_set_text(GTK_LABEL(session_desc[ix]), text);
      } else {
	gtk_label_set_text(GTK_LABEL(session_desc[ix]), "");
      }
    }
  }
  CMsg *newmsg;
  while ((newmsg = master_queue.get_message()) != NULL) {
    switch (newmsg->get_value()) {
    case MSG_SESSION_STARTED:
      adjust_gui_for_play();
      break;
    case MSG_SESSION_ERROR: 
    case MSG_SESSION_WARNING: 
      //
      if (psptr != NULL) {
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "%s cannot be opened\n%s", 
		 psptr->get_session_name(),
		 psptr->get_message());
	ShowMessage("Open error", buffer);
      }
      if (newmsg->get_value() == MSG_SESSION_WARNING) {
	adjust_gui_for_play();
	break;
      }
      // otherwise fall through into quit
    case MSG_RECEIVED_QUIT:
      //player_debug_message("received quit");
      on_main_menu_close(NULL, 0);
      break;
    case MSG_SESSION_FINISHED:
      player_debug_message("gui received finished message");
      if (master_playlist != NULL) {
	SDL_mutexP(command_mutex);
	const char *start;
	do {
	  start = master_playlist->get_next();
	  if (start == NULL) {
	    if (master_looped)
	      start = master_playlist->get_first();
	  }
	  if (start != NULL)
	    create_session_from_name(start);
	  else {
	    on_stop_clicked(NULL, NULL);
	  }
	} while (start != NULL && psptr == NULL);
	SDL_mutexV(command_mutex);
      } else if (master_looped) {
	if (play_state == PLAYING) {
	  if (psptr == NULL)
	    break;
	  SDL_mutexP(command_mutex);
	  psptr->pause_all_media();
	  if (psptr->play_all_media(TRUE, 0.0) < 0) {
	    snprintf(buffer, sizeof(buffer), "Error re-starting session: %s", 
		     psptr->get_message());
	    close_session();
	    ShowMessage("Play error", buffer);
	  } else {
	    adjust_gui_for_play();
	  }
	  SDL_mutexV(command_mutex);
	}
      } else if (psptr != NULL) {
	play_state = STOPPED;
	toggle_button_adjust(play_button, FALSE);
	toggle_button_adjust(stop_button, TRUE);
	psptr->pause_all_media();
	master_fullscreen = 0;
      }

      break;
    case MSG_SDL_KEY_EVENT:
      sdl_event_msg_t *msg;
      uint32_t len;
      msg = (sdl_event_msg_t *)newmsg->get_message(len);
      if (len != sizeof(*msg)) {
	player_error_message("key event message is wrong size %d %d", 
			     len, (int) sizeof(*msg));
	break;
      }
      switch (msg->sym) {
      case SDLK_ESCAPE:
	player_debug_message("Got escape");
	master_fullscreen = 0;
	break;
      case SDLK_RETURN:
	if ((msg->mod & (KMOD_ALT | KMOD_META)) != 0) {
	  master_fullscreen = 1;
	}
	break;
      case SDLK_c:
	if ((msg->mod & (KMOD_LCTRL | KMOD_RCTRL)) != 0) {
	  on_main_menu_close(NULL, 0);
	}
	break;
      case SDLK_n:
	if ((msg->mod & KMOD_CTRL) != 0) {
	  if (master_playlist != NULL) {
	    master_queue.send_message(MSG_SESSION_FINISHED);
	  }
	}
	break;
      case SDLK_x:
	if ((msg->mod & KMOD_CTRL) != 0) {
	  delete_event(NULL, 0);
	}
	break;
      case SDLK_s:
	if ((msg->mod & KMOD_CTRL) != 0) {
	  on_menu_seek(NULL, NULL);
	}
	break;
      case SDLK_UP:
	master_volume += 10;
	if (master_volume > 100) master_volume = 100;
	volume_adjusted(master_volume);
	break;
      case SDLK_DOWN:
	if (master_volume > 10) {
	  master_volume -= 10;
	} else {
	  master_volume = 0;
	}
	volume_adjusted(master_volume);
	break;
	
      case SDLK_SPACE:
	if (play_state == PLAYING) {
	  do_pause();
	} else if (play_state == PAUSED && psptr) {
	  SDL_mutexP(command_mutex);
	  if (psptr->play_all_media(FALSE, 0.0) == 0) {
	    adjust_gui_for_play();
	    SDL_mutexV(command_mutex);
	  } else {
	    SDL_mutexV(command_mutex);
	    snprintf(buffer, sizeof(buffer), "Error re-starting session: %s", 
		     psptr->get_message());
	    close_session();
	    ShowMessage("Play error", buffer);
	  }
	}
	break;
      case SDLK_HOME:
	if (psptr && play_state == PLAYING) {
	  psptr->pause_all_media();
	  if (psptr->play_all_media(TRUE, 0.0) < 0) {
	    snprintf(buffer, sizeof(buffer), "Error re-starting session: %s", 
		     psptr->get_message());
	    close_session();
	    ShowMessage("Play error", buffer);
	  }
	}
	break;
      case SDLK_RIGHT:
	if (psptr && psptr->session_is_seekable() && play_state == PLAYING) {
	  SDL_mutexP(command_mutex);
	  play_time = psptr->get_playing_time();
	  double ptime, maxtime;
	  play_time += TO_U64(10000);
	  ptime = (double)play_time;
	  ptime /= 1000.0;
	  maxtime = psptr->get_max_time();
	  if (ptime < maxtime) {
	    psptr->pause_all_media();
	    if (psptr->play_all_media(FALSE, ptime) == 0) {
	      adjust_gui_for_play();
	    } else {
	      SDL_mutexV(command_mutex);
	      snprintf(buffer, sizeof(buffer), "Error re-starting session: %s", 
		       psptr->get_message());
	      close_session();
	      ShowMessage("Play error", buffer);
	      break;
	    }
	  }
	  SDL_mutexV(command_mutex);
	}
	break;
      case SDLK_LEFT:
	if (psptr && psptr->session_is_seekable() && play_state == PLAYING) {
	  SDL_mutexP(command_mutex);
	  play_time = psptr->get_playing_time();
	  double ptime;
	  if (play_time >= TO_U64(10000)) {
	    play_time -= TO_U64(10000);
	    ptime = (double)play_time;
	    ptime /= 1000.0;
	    psptr->pause_all_media();
	    if (psptr->play_all_media(FALSE, ptime) == 0) {
	      adjust_gui_for_play();
	    } else {
	      SDL_mutexV(command_mutex);
	      snprintf(buffer, sizeof(buffer), "Error starting session: %s", 
		       psptr->get_message());
	      close_session();
	      ShowMessage("Play error", buffer);
	      break;
	    }
	  }
	  SDL_mutexV(command_mutex);
	}
	break;
      case SDLK_PAGEUP:
	if (master_screen_size < 200) {
	  change_video_size (master_screen_size * 2);
	}
	break;
      case SDLK_PAGEDOWN:
	if (master_screen_size > 50) {
	  change_video_size (master_screen_size / 2);
	}
	break;
      case SDLK_0:
      case SDLK_1:
      case SDLK_2:
      case SDLK_3:
      case SDLK_4:
      case SDLK_5:
	if ((msg->mod & KMOD_CTRL) != 0) {
	  set_aspect_ratio(msg->sym - SDLK_0);
	}
        break;
      default:
	break;
      }
      break;
    }
    delete newmsg;
  }
  return (TRUE);  // keep timer going
}

/*
 * Main routine - set up window
 */
int main (int argc, char **argv)
{
  static struct option orig_options[] = {
    { "version", 0, 0, 'v' },
    { "help", 0, 0, 'H'},
    { "config-vars", 0, 0, 'c'},
    { NULL, 0, 0, 0 }
  };
  
  player_error_message("%s version %s", argv[0], MPEG4IP_VERSION);

  bool have_unknown_opts = false;
  char buffer[FILENAME_MAX];
  char *home = getenv("HOME");
  if (home == NULL) {
#ifdef _WIN32
	strcpy(buffer, "gmp4player_rc");
#else
    strcpy(buffer, ".gmp4player_rc");
#endif
  } else {
    strcpy(buffer, home);
    strcat(buffer, "/.gmp4player_rc");
  }
  
  initialize_plugins(&config);
  config.SetFileName(buffer);
  config.InitializeIndexes();
  opterr = 0;
  while (true) {
    int c = -1;
    int option_index = 0;
	  
    c = getopt_long_only(argc, argv, "af:hsvc",
			 orig_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 'H':
      fprintf(stderr, 
	      "Usage: %s [--help] [--version] [--<config variable>=<value>] media-to-play\n",
	      argv[0]);
      fprintf(stderr, "Use [--config-vars] to dump configuration variables\n");
      exit(-1);

    case 'c':
      config.DisplayHelp();
      exit(0);
    case 'v':
      fprintf(stderr, "%s version %s\n", argv[0], MPEG4IP_VERSION);
      exit(0);
    case '?':
    default:
      have_unknown_opts = true;
      break;
    }
  }

  command_mutex = SDL_CreateMutex();
  config.ReadFile();

  if (config.get_config_string(CONFIG_LOG_FILE) != NULL) {
    open_log_file(config.get_config_string(CONFIG_LOG_FILE));
  }
  if (have_unknown_opts) {
    /*
     * Create an struct option that allows all the loaded configuration
     * options
     */
    struct option *long_options;
    uint32_t origo = sizeof(orig_options) / sizeof(*orig_options);
    long_options = create_long_opts_from_config(&config,
						orig_options,
						origo,
						128);
    if (long_options == NULL) {
      player_error_message("Couldn't create options");
      exit(-1);
    }
    optind = 1;
    // command line parsing
    while (true) {
      int c = -1;
      int option_index = 0;
      config_index_t ix;

      c = getopt_long_only(argc, argv, "af:hsv",
			   long_options, &option_index);

      if (c == -1)
	break;

      /*
       * We set a value of 128 to 128 + number of variables
       * we added the original options to the block, but don't
       * process them here.
       */
      if (c >= 128) {
	// we have an option from the config file
	ix = c - 128;
	player_debug_message("setting %s to %s",
		      config.GetNameFromIndex(ix),
		      optarg);
	if (config.GetTypeFromIndex(ix) == CONFIG_TYPE_BOOL &&
	    optarg == NULL) {
	  config.SetBoolValue(ix, true);
	} else
	  if (optarg == NULL) {
	    player_error_message("Missing argument with variable %s",
				 config.GetNameFromIndex(ix));
	  } else 
	    config.SetVariableFromAscii(ix, optarg);
      } else if (c == '?') {
	fprintf(stderr, 
		"Usage: %s [--help] [--version] [--<config variable>=<value>] media-to-play\n",
		argv[0]);
	exit(-1);
      }
    }
    free(long_options);
  }
  gtk_init(&argc, &argv);
  const char *read;
  playlist = g_list_append(playlist, (void *)"");
  read = config.get_config_string(CONFIG_PREV_FILE_0);
  if (read != NULL) {
    gchar *newone = g_strdup(read);
    playlist = g_list_append(playlist, newone);
  }
  read = config.get_config_string(CONFIG_PREV_FILE_1);
  if (read != NULL) {
    gchar *newone = g_strdup(read);
    playlist = g_list_append(playlist, newone);
  }
  read = config.get_config_string(CONFIG_PREV_FILE_2);
  if (read != NULL) {
    gchar *newone = g_strdup(read);
    playlist = g_list_append(playlist, newone);
  }
  read = config.get_config_string(CONFIG_PREV_FILE_3);
  if (read != NULL) {
    gchar *newone = g_strdup(read);
    playlist = g_list_append(playlist, newone);
  }
  
  last_file = g_strdup(config.get_config_string(CONFIG_PREV_DIRECTORY));
  master_looped = config.GetBoolValue(CONFIG_LOOPED);
  master_muted = config.GetBoolValue(CONFIG_MUTED);
  master_volume = config.get_config_value(CONFIG_VOLUME);
  rtsp_set_error_func(library_message);
  rtsp_set_loglevel(config.get_config_value(CONFIG_RTSP_DEBUG));
  rtp_set_error_msg_func(library_message);
  rtp_set_loglevel(config.get_config_value(CONFIG_RTP_DEBUG));
  sdp_set_error_func(library_message);
  sdp_set_loglevel(config.get_config_value(CONFIG_SDP_DEBUG));
  http_set_error_func(library_message);
  http_set_loglevel(config.get_config_value(CONFIG_HTTP_DEBUG));
  mpeg2t_set_error_func(library_message);
  mpeg2t_set_loglevel(config.get_config_value(CONFIG_MPEG2T_DEBUG));
  mpeg2ps_set_error_func(library_message);
  mpeg2ps_set_loglevel(config.get_config_value(CONFIG_MPEG2PS_DEBUG));

  if (config.get_config_value(CONFIG_RX_SOCKET_SIZE) != 0) {
    rtp_set_receive_buffer_default_size(config.get_config_value(CONFIG_RX_SOCKET_SIZE));
  }
  /*
   * Set up main window
   */
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(main_window), FALSE, TRUE, FALSE);
  sprintf(buffer, "Cisco Open Source Streaming Video Player %s", MPEG4IP_VERSION);
  gtk_window_set_title(GTK_WINDOW(main_window), buffer);
  gtk_widget_set_uposition(GTK_WIDGET(main_window), 10, 10);
  gtk_widget_set_usize(main_window, 450, 200);
  gtk_window_set_default_size(GTK_WINDOW(main_window), 460, 210);
  gtk_signal_connect(GTK_OBJECT(main_window),
		     "delete_event",
		     GTK_SIGNAL_FUNC(delete_event),
		     NULL);
  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(main_vbox);
  // add stuff
  accel_group = gtk_accel_group_new();
#ifdef HAVE_GTK_2_0
  gtk_window_add_accel_group(GTK_WINDOW(main_window), accel_group);
#else
  gtk_accel_group_attach(accel_group, GTK_OBJECT(main_window));
#endif
  tooltips = gtk_tooltips_new();

  GtkWidget *menubar, *menu, *menuitem;
  
  menubar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
  gtk_widget_show(menubar);

  menu = CreateBarSubMenu(menubar, "File");
  menuitem = CreateMenuItem(menu,
			    accel_group,
			    tooltips,
			    "Open",
			    "^O",
			    "Open local media file",
			    GTK_SIGNAL_FUNC(on_browse_button_clicked),
			    NULL);

  close_menuitem = CreateMenuItem(menu,
			    accel_group,
			    tooltips,
			    "Close",
			    "^C",
			    "Close file/stream being viewed",
			    GTK_SIGNAL_FUNC(on_main_menu_close),
			    NULL);

  // NULL entry is a seperator
  menuitem = CreateMenuItem(menu, 
			    accel_group, 
			    tooltips, 
			    NULL, NULL, NULL, 
			    NULL, NULL);

  menuitem = CreateMenuItem(menu,
			    accel_group,
			    tooltips,
			    "Exit",
			    "^X",
			    "Exit program",
			    GTK_SIGNAL_FUNC(delete_event),
			    NULL);

  menu = CreateBarSubMenu(menubar, "Media");

  GtkWidget *videosub;
  videosub = CreateSubMenu(menu, "Video");
  GSList *videosizelist = NULL;
  videoradio[0] = CreateMenuRadio(videosub,
			       "50 %",
			       &videosizelist,
			       GTK_SIGNAL_FUNC(on_video_radio),
			       GINT_TO_POINTER(50));
  videoradio[1] = CreateMenuRadio(videosub,
			       "100 %",
			       &videosizelist,
			       GTK_SIGNAL_FUNC(on_video_radio),
			       GINT_TO_POINTER(100));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM((videoradio[1])), TRUE);
  videoradio[2] = CreateMenuRadio(videosub,
			       "200 %",
			       &videosizelist,
			       GTK_SIGNAL_FUNC(on_video_radio),
			       GINT_TO_POINTER(200));

  menuitem = CreateMenuItem(videosub, 
			    accel_group, 
			    tooltips, 
			    NULL, NULL, NULL, 
			    NULL, NULL);

  menuitem = CreateMenuItem(videosub,
			    accel_group,
			    tooltips,
			    "Full Screen",
			    "M-<enter>",
			    "Full Screen",
			    GTK_SIGNAL_FUNC(on_video_fullscreen),
			    NULL);

  menuitem = CreateMenuItem(videosub, 
			    accel_group, 
			    tooltips, 
			    NULL, NULL, NULL, 
			    NULL, NULL);
  GSList *aspectratiolist = NULL;
  int value = config.get_config_value(CONFIG_ASPECT_RATIO);
  set_aspect_ratio(value);
  aspectratio[0] = CreateMenuRadio(videosub,
				   "Aspect Ratio Auto",
				   &aspectratiolist,
				   GTK_SIGNAL_FUNC(on_aspect_ratio),
				   GINT_TO_POINTER(0));
  aspectratio[1] = CreateMenuRadio(videosub,
				   "4:3",
				   &aspectratiolist,
				   GTK_SIGNAL_FUNC(on_aspect_ratio),
				   GINT_TO_POINTER(1));
  aspectratio[2] = CreateMenuRadio(videosub,
				   "16:9",
				   &aspectratiolist,
				   GTK_SIGNAL_FUNC(on_aspect_ratio),
				   GINT_TO_POINTER(2));
  aspectratio[3] = CreateMenuRadio(videosub,
				   "1.85 Letterbox",
				   &aspectratiolist,
				   GTK_SIGNAL_FUNC(on_aspect_ratio),
				   GINT_TO_POINTER(3));
  aspectratio[4] = CreateMenuRadio(videosub,
				   "2.35 Letterbox",
				   &aspectratiolist,
				   GTK_SIGNAL_FUNC(on_aspect_ratio),
				   GINT_TO_POINTER(4));
  aspectratio[5] = CreateMenuRadio(videosub, 
				   "1:1",
				   &aspectratiolist,
				   GTK_SIGNAL_FUNC(on_aspect_ratio),
				   GINT_TO_POINTER(5));

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(aspectratio[value]), TRUE);
  CreateMenuItemSeperator(menu);

  seek_menuitem = CreateMenuItem(menu, 
				 accel_group,
				 tooltips,
				 "Seek",
				 "^S",
				 "Seek",
				 GTK_SIGNAL_FUNC(on_menu_seek),
				 NULL);
  gtk_widget_set_sensitive(seek_menuitem, 0);
  CreateMenuItemSeperator(menu);

  menuitem = CreateMenuCheck(menu, 
			     "Play Audio", 
			     GTK_SIGNAL_FUNC(on_media_play_audio),
			     NULL,
			     config.get_config_value(CONFIG_PLAY_AUDIO));

  menuitem = CreateMenuCheck(menu, 
			     "Play Text", 
			     GTK_SIGNAL_FUNC(on_media_play_text),
			     NULL,
			     config.get_config_value(CONFIG_PLAY_TEXT));
  menuitem = CreateMenuCheck(menu, 
			     "Play Video", 
			     GTK_SIGNAL_FUNC(on_media_play_video),
			     NULL,
			     config.get_config_value(CONFIG_PLAY_VIDEO));
  CreateMenuItemSeperator(menu);

  menu = CreateBarSubMenu(menubar, "Network");
  menuitem = CreateMenuCheck(menu,
			     "Use RTP over RTSP(TCP)",
			     GTK_SIGNAL_FUNC(on_network_rtp_over_rtsp),
			     NULL,
			     config.get_config_value(CONFIG_USE_RTP_OVER_RTSP));

  menu = CreateBarSubMenu(menubar, "Help");
  menuitem = CreateMenuItem(menu,
			    accel_group,
			    tooltips,
			    "Help",
			    NULL,
			    NULL,
			    GTK_SIGNAL_FUNC(on_main_menu_help),
			    NULL);
  CreateMenuItemSeperator(menu);
  GtkWidget *debugsub;
  debugsub = CreateSubMenu(menu, "Debug");
  menuitem = CreateMenuCheck(debugsub,
			     "One Sec Debug Display",
			     GTK_SIGNAL_FUNC(on_debug_display_status),
			     NULL,
			     config.GetBoolValue(CONFIG_DISPLAY_DEBUG));

  config_index_t iso_check;
  iso_check = config.FindIndexByName("Mpeg4IsoOnly");
  if (iso_check != UINT32_MAX) {
    menuitem = CreateMenuCheck(debugsub, 
			       "Mpeg4ISOOnly",
			       GTK_SIGNAL_FUNC(on_debug_mpeg4isoonly),
			       NULL,
			       config.GetBoolValue(iso_check));
  }
  config_index_t old_xvid;
  old_xvid = config.FindIndexByName("UseOldXvid");
  if (old_xvid != UINT32_MAX) {
    menuitem = CreateMenuCheck(debugsub, 
			       "Use Old Xvid",
			       GTK_SIGNAL_FUNC(on_debug_oldxvid),
			       NULL,
			       config.GetBoolValue(old_xvid));
  }
  old_xvid = config.FindIndexByName("UseMpeg3");
  if (old_xvid != UINT32_MAX) {
    menuitem = CreateMenuCheck(debugsub,
			       "Use libmpeg3",
			       GTK_SIGNAL_FUNC(on_debug_usempeg3),
			       NULL,
			       config.GetBoolValue(old_xvid));
  }

  menuitem = CreateMenuCheck(debugsub, 
			     "Use Old mp4 library",
			     GTK_SIGNAL_FUNC(on_debug_use_old_mp4_lib),
			     NULL,
			     config.get_config_value(CONFIG_USE_OLD_MP4_LIB) == 0 ? FALSE : TRUE);

  CreateLogLevelSubmenu(debugsub, 
			"HTTP library", 
			config.get_config_value(CONFIG_HTTP_DEBUG),
			GTK_SIGNAL_FUNC(on_debug_http));
  CreateLogLevelSubmenu(debugsub, 
			"RTSP library", 
			config.get_config_value(CONFIG_RTSP_DEBUG),
			GTK_SIGNAL_FUNC(on_debug_rtsp));
  CreateLogLevelSubmenu(debugsub, 
			"RTP library", 
			config.get_config_value(CONFIG_RTP_DEBUG),
			GTK_SIGNAL_FUNC(on_debug_rtp));
  CreateLogLevelSubmenu(debugsub, 
			"SDP library", 
			config.get_config_value(CONFIG_SDP_DEBUG),
			GTK_SIGNAL_FUNC(on_debug_sdp));
  CreateLogLevelSubmenu(debugsub,
			"Mpeg2 Transport Library",
			config.get_config_value(CONFIG_MPEG2T_DEBUG),
			GTK_SIGNAL_FUNC(on_debug_mpeg2t));
  CreateLogLevelSubmenu(debugsub,
			"Mpeg2 Program Stream Library",
			config.get_config_value(CONFIG_MPEG2PS_DEBUG),
			GTK_SIGNAL_FUNC(on_debug_mpeg2ps));

  CreateMenuItemSeperator(menu);
  menuitem = CreateMenuItem(menu, 
			    accel_group, 
			    tooltips, 
			    "Set Log File", 
			    NULL, 
			    NULL, 
			    GTK_SIGNAL_FUNC(on_main_menu_set_log_file),
			    NULL);
  menuitem = CreateMenuItem(menu, 
			    accel_group, 
			    tooltips, 
			    "Clear Log File", 
			    NULL, 
			    NULL, 
			    GTK_SIGNAL_FUNC(on_main_menu_clear_log_file),
			    NULL);
  CreateMenuItemSeperator(menu);
  menuitem = CreateMenuItem(menu,
			    accel_group,
			    tooltips, 
			    "About",
			    NULL,
			    NULL,
			    GTK_SIGNAL_FUNC(on_main_menu_about),
			    NULL);

  // this is how you turn off the widget
  gtk_widget_set_sensitive(close_menuitem, 0);
  GtkWidget *hbox;

  /*
   * first line - Play: combo box browse button 
   */
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox);

  gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 5);
  
  GtkWidget *label;
  label = gtk_label_new("Play: ");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
  gtk_widget_show(label);

  // combo box uses list playlist.
  combo = gtk_combo_new();
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry),
		     "activate",
		     GTK_SIGNAL_FUNC(on_play_list_selected), 
		     NULL);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->list),
		     "select_child",
		     GTK_SIGNAL_FUNC(on_playlist_child_selected),
		     combo);
  gtk_combo_set_popdown_strings (GTK_COMBO(combo), playlist);

  gtk_combo_set_value_in_list(GTK_COMBO(combo), FALSE, FALSE);
  gtk_combo_disable_activate(GTK_COMBO(combo));
  gtk_combo_set_use_arrows_always(GTK_COMBO(combo), 1);
  gtk_combo_set_case_sensitive(GTK_COMBO(combo), TRUE);
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show(combo);

  /* button for browse */
  GtkWidget *button;
  button = gtk_button_new_with_label("Browse");
  gtk_signal_connect(GTK_OBJECT(button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_browse_button_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);
  gtk_widget_show(button);

  /*************************************************************************
   * 2nd line  play pause stop speaker volume
   *************************************************************************/
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show(hbox);

  GtkWidget *vbox, *hbox2;
  vbox = gtk_vbox_new(TRUE, 1);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);
  gtk_widget_show(vbox);

  hbox2 = gtk_hbox_new(TRUE, 1);
  gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show(hbox2);

  GtkWidget *image;
  
  play_button = gtk_toggle_button_new();
  image = CreateWidgetFromXpm(main_window,xpm_play);
  gtk_container_add(GTK_CONTAINER(play_button), image);
#if 0
  //gdk_pixmap_unref((GdkPixmap *)image);
#endif
  gtk_widget_show(play_button);
  gtk_signal_connect(GTK_OBJECT(play_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_play_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox2), play_button, FALSE, FALSE, 2);

  pause_button = gtk_toggle_button_new();
  image = CreateWidgetFromXpm(main_window,xpm_pause);
  gtk_container_add(GTK_CONTAINER(pause_button), image);
#if 0
  gdk_pixmap_unref((GdkPixmap *)image);
#endif
  gtk_widget_show(pause_button);
  gtk_signal_connect(GTK_OBJECT(pause_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_pause_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox2), pause_button, FALSE, FALSE, 2);

  stop_button = gtk_toggle_button_new();
  image = CreateWidgetFromXpm(main_window,xpm_stop);
  gtk_container_add(GTK_CONTAINER(stop_button), image);
#if 0
  gdk_pixmap_unref((GdkPixmap *)image);
#endif
  gtk_widget_show(stop_button);
  gtk_signal_connect(GTK_OBJECT(stop_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_stop_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox2), stop_button, FALSE, FALSE, 2);


  gtk_widget_set_sensitive(play_button, 0);
  gtk_widget_set_sensitive(pause_button, 0);
  gtk_widget_set_sensitive(stop_button, 0);

  GtkWidget *sep = gtk_vseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 5);

  vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(vbox);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

  hbox2 = gtk_hbox_new(FALSE, 1);
  gtk_widget_show(hbox2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

  speaker_button = gtk_button_new();
  image = CreateWidgetFromXpm(speaker_button,
			      master_muted == false ?
			      xpm_speaker : xpm_speaker_muted);
  gtk_container_add(GTK_CONTAINER(speaker_button), image);
#if 0
  gdk_pixmap_unref((GdkPixmap *)image);
#endif
  gtk_widget_show(speaker_button);
  gtk_signal_connect(GTK_OBJECT(speaker_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_speaker_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox2), speaker_button, FALSE, FALSE, 5);

  GtkObject *vol_adj;
  vol_adj = gtk_adjustment_new(master_volume,
			       0.0,
			       100.0,
			       5.0,
			       25.0,
			       0.0);
  
  GtkWidget *vol;
  vol = gtk_hscale_new(GTK_ADJUSTMENT(vol_adj));
  gtk_scale_set_digits(GTK_SCALE(vol),0);
  gtk_scale_set_draw_value(GTK_SCALE(vol), 0);
  gtk_range_set_adjustment ( GTK_RANGE( vol ), GTK_ADJUSTMENT(vol_adj));
#ifdef HAVE_GTK_2_0
  gtk_widget_set_size_request(GTK_WIDGET(vol), 100, -1);
  gtk_range_set_update_policy(GTK_RANGE(vol), GTK_UPDATE_DISCONTINUOUS);
#else
  gtk_range_slider_update ( GTK_RANGE( vol ) );
  gtk_range_clear_background ( GTK_RANGE( vol ) );
  //gtk_range_draw_background ( GTK_RANGE( vol ) );
#endif

#ifdef HAVE_GTK_2_0
  // We don't need this - I'm not 100% sure why, but we don't...
  gtk_signal_connect(vol_adj,
		     "value_changed",
		     GTK_SIGNAL_FUNC(on_volume_adjusted),
		     vol);
#else
  gtk_signal_connect(GTK_OBJECT(vol), 
		     "button_release_event",
		     GTK_SIGNAL_FUNC(on_volume_adjusted),
		     vol);
#endif
  gtk_widget_show(vol);
  volume_slider = vol;
  gtk_box_pack_start(GTK_BOX(hbox2), vol, TRUE, TRUE, 0);
  
  // boxes for Looped label 
  sep = gtk_vseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 5);

  vbox = gtk_vbox_new(TRUE, 1);
  gtk_widget_show(vbox);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

  hbox2 = gtk_hbox_new(TRUE, 1);
  gtk_widget_show(hbox2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

  GtkWidget *loop_enabled_button;
  loop_enabled_button = gtk_check_button_new_with_label("Looped");
  gtk_box_pack_start(GTK_BOX(hbox2), loop_enabled_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(loop_enabled_button),
		     "toggled",
		     GTK_SIGNAL_FUNC(on_loop_enabled_button),
		     loop_enabled_button);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(loop_enabled_button),
			       master_looped);
  gtk_widget_show(loop_enabled_button);
  
  /* 
   * 3rd line - where we are meter...
   */
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);

  GtkObject *time_slider_adj;
  master_time = 0;
  time_slider_adj = gtk_adjustment_new(0.0,
				  0.0,
				  100.0,
				  0.5,
				  0.5,
				  0.5);
  
  time_slider = gtk_hscale_new(GTK_ADJUSTMENT(time_slider_adj));
  gtk_range_set_update_policy(GTK_RANGE(time_slider), GTK_UPDATE_DELAYED);
  gtk_scale_set_digits(GTK_SCALE(time_slider),0);
  gtk_scale_set_draw_value(GTK_SCALE(time_slider), 0);
  gtk_range_set_adjustment(GTK_RANGE(time_slider), 
			   GTK_ADJUSTMENT(time_slider_adj));
#ifndef HAVE_GTK_2_0
  gtk_range_slider_update(GTK_RANGE(time_slider));
  gtk_range_clear_background(GTK_RANGE( time_slider));
#endif

  gtk_signal_connect(GTK_OBJECT(time_slider), 
		     "button_release_event",
		     GTK_SIGNAL_FUNC(on_time_slider_adjusted),
		     time_slider);
  gtk_signal_connect(GTK_OBJECT(time_slider),
		     "button_press_event",
		     GTK_SIGNAL_FUNC(on_time_slider_pressed),
		     NULL);
  gtk_widget_set_sensitive(time_slider, 0);
  gtk_widget_show(time_slider);
  gtk_box_pack_start(GTK_BOX(hbox), time_slider, TRUE, TRUE, 0);
  /* 
   * 4th line - time display...
   */
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
  
  time_disp = gtk_label_new("0:00:00");
  gtk_widget_ref(time_disp);
  gtk_widget_show(time_disp);
  gtk_box_pack_start(GTK_BOX(hbox), time_disp, TRUE, TRUE,0);

  // session information
  for (int ix = 0; ix < 4; ix++) {
    hbox = gtk_hbox_new(FALSE, 1);
    gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show(hbox);
  
    session_desc[ix] = gtk_label_new("");
    gtk_widget_ref(session_desc[ix]);
    gtk_widget_show(session_desc[ix]);
    gtk_box_pack_start(GTK_BOX(hbox), session_desc[ix], TRUE, TRUE,0);
  }


  gtk_timeout_add(500, main_timer, main_window);
  gtk_container_add(GTK_CONTAINER(main_window), main_vbox);
  gtk_widget_show(main_window);

  /*
   * set up drag and drop
   */
  gtk_drag_dest_set(main_window, 
		    GTK_DEST_DEFAULT_ALL, 
		    drop_types,
		    1, 
		    GDK_ACTION_COPY);
  gtk_signal_connect(GTK_OBJECT(main_window), 
		     "drag_data_received",
		     GTK_SIGNAL_FUNC(on_drag_data_received),
		     main_window);
  /*
   * any args passed ?
   */
  if ((argc - optind) > 0) {
    start_session_from_name(argv[optind++]);
  }
  gtk_main();
  exit(0);
}

/* end gui_main.cpp */
