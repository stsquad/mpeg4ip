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
 * gui_main.cpp - Contains the gtk based gui for this player.
 */
#include <stdlib.h>
#include <stdio.h>
#include "player_session.h"
#include <glib.h>
#include <gtk/gtk.h>
#include "gui_utils.h"
#include "media_utils.h"
#include "player_util.h"
#include "gui_xpm.h"
#include <syslog.h>
#include <rtsp/rtsp_client.h>

/* Local variables */
static GtkWidget *main_window;
static GtkWidget *main_vbox;
static GtkWidget *close_menuitem;
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
static SDL_mutex *command_mutex;
static int master_volume;
static int master_muted;
static uint64_t master_time;
static int time_slider_pressed = 0;
static int master_screen_size = 100;
static const char *last_entry = NULL;
static const gchar *last_file = NULL;
static GtkTargetEntry drop_types[] = 
{
  { "text/plain", 0, 1 }
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
  if (psptr->session_is_seekable()) {
    gtk_widget_set_sensitive(time_slider, 1);
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


/*
 * start a session
 */
static void start_session_from_name (const char *name)
{
  GList *p;
  int in_list = 0;
  GdkWindow *window;
  window = GTK_WIDGET(main_window)->window;
  gint x, y, w, h;
  gdk_window_get_position(window, &x, &y);
  gdk_window_get_size(window, &w, &h);
  y += h + 40;

  // Save the last file, so we can use it next time we do a file open
  if (last_entry != NULL) {
    free((void *)last_entry);
  }
  last_entry = strdup(name);
  if (strncmp("rtsp://", name, strlen("rtsp://")) != 0) {
    if (last_file != NULL) {
      free((void *)last_file);
      last_file = NULL;
    }
    last_file = g_strdup(name);
  }

  // See if entry is already in the list.
  p = g_list_first(playlist);
  while (p != NULL && in_list == 0) {
    if (g_strcasecmp(name, (const gchar *)p->data) == 0) {
      in_list = 1;
    }
    p = g_list_next(p);
  }
  
  // If we're already running a session, close it.
  if (psptr != NULL) {
    close_session();
  }

  // Create a new player session
  SDL_mutexP(command_mutex);
  psptr = new CPlayerSession(&master_queue,
			     NULL,
			     name);
  if (psptr != NULL) {
    const char *errmsg;
    errmsg = NULL;
    // See if we can create media for this session
    int ret = parse_name_for_session(psptr, name, &errmsg);
    if (ret >= 0) {
      // Yup - valid session.  Set volume, set up sync thread, and
      // start the session
      if (ret > 0) {
	ShowMessage("Warning", errmsg);
      }
      if (master_muted == 0)
	psptr->set_audio_volume(master_volume);
      else
	psptr->set_audio_volume(0);
      psptr->set_up_sync_thread();
      psptr->set_screen_location(x, y);
      psptr->set_screen_size(master_screen_size / 50);
      psptr->play_all_media(TRUE);  // check response here...
    } else {
      // Nope - display a message
      delete psptr;
      psptr = NULL;
      char buffer[1024];
      snprintf(buffer, sizeof(buffer), "%s cannot be opened\n%s", name,
	       errmsg);
      ShowMessage("Open error", buffer);
    }
  }

  // Add this file to the drop down list
  if (in_list == 0) {
    gchar *newone = g_strdup(name);
    playlist = g_list_append(playlist, newone);
    gtk_combo_set_popdown_strings (GTK_COMBO(combo), playlist);
    gtk_widget_show(combo);
  }
  // If we're playing, adjust the gui
  if (psptr != NULL) {
    adjust_gui_for_play();
  }
  SDL_mutexV(command_mutex);
}

/*
 * delete_event - called when window closed
 */
void delete_event (GtkWidget *widget, gpointer *data)
{
  //  if (widget == main_window) {
  if (psptr != NULL) {
    delete psptr;
  }
    
  gtk_main_quit();
    //}
}

static void on_main_menu_close (GtkWidget *window, gpointer data)
{
  SDL_mutexP(command_mutex);
  close_session();
  SDL_mutexV(command_mutex);
}

static void on_main_menu_help (GtkWidget *window, gpointer data)
{
}

static void on_main_menu_about (GtkWidget *window, gpointer data)
{
  ShowMessage("About gmp4player",
	      "gmp4player is an open source file/streaming MPEG4 player\n"
	      "Developed by cisco Systems using the\n"
	      "following open source packages:\n"
	      "\n"
	      "SDL, SMPEG audio (MP3) from lokigames\n"
	      "RTP from UCL\n"
	      "ISO reference decoder for MPEG4\n"
	      "FAAC decoder\n"
	      "OpenDivx decore version 0.48\n"
	      "Developed by Bill May, 10/00 to 3/01");
}

static void on_main_menu_debug (GtkWidget *window, gpointer data)
{
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

static GtkWidget *filesel;

static void on_filename_selected (GtkFileSelection *selector, 
				  gpointer user_data) 
{
  const gchar *name;

  name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel));
  start_session_from_name(name);
}

/*
 * This is right out of the book...
 */
static void on_browse_button_clicked (GtkWidget *window, gpointer data)
{
  filesel = gtk_file_selection_new("Open Media file");
  if (last_file != NULL) {
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), last_file);
  }
  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(filesel));
  gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
		      "clicked", 
		      GTK_SIGNAL_FUNC(on_filename_selected), 
		      filesel);
                             
  /* Ensure that the dialog box is destroyed when the user clicks a button. */
     
  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
			    "clicked", 
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(filesel));

  gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
			    "clicked", 
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(filesel));
  
  /* Display that dialog */
     
  gtk_widget_show (filesel);
  
}

static void on_playlist_child_selected (GtkWidget *window, gpointer data)
{
  gchar *entry = 
    gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));
  if (strcmp(entry, "") == 0) 
    return;
  if (last_entry && strcmp(entry, last_entry) == 0) 
    return;
  start_session_from_name(entry);
}

static void on_play_list_selected (GtkWidget *window, gpointer data)
{
  gchar *entry = gtk_entry_get_text(GTK_ENTRY(window));
  start_session_from_name(entry);
}

static void on_play_clicked (GtkWidget *window, gpointer data)
{
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(play_button)) == FALSE) {
    if (play_state == PLAYING) {
      toggle_button_adjust(play_button, TRUE);
    }
    return;
  }

  if (psptr == NULL)
    return;
  SDL_mutexP(command_mutex);
  switch (play_state) {
  case PAUSED:
    psptr->play_all_media(FALSE);
    break;
  case STOPPED:
    psptr->play_all_media(TRUE, 0.0);
    break;
  default:
    break;
  }
  adjust_gui_for_play();
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
  if (master_muted == 0) {
    // mute the puppy
    set_speaker_button(xpm_speaker_muted);
    master_muted = 1;
    vol = 0;
  } else {
    set_speaker_button(xpm_speaker);
    master_muted = 0;
    vol  = master_volume;
  }
  if (psptr && psptr->session_has_audio()) {
    psptr->set_audio_volume(vol);
  }
}

static void on_volume_adjusted (GtkWidget *window, gpointer data)
{
  GtkWidget *vol = (GtkWidget *)window;
  GtkAdjustment *val = gtk_range_get_adjustment(GTK_RANGE(vol));
  master_volume = (int)val->value;
  gtk_range_set_adjustment(GTK_RANGE(vol), val);
  gtk_range_slider_update(GTK_RANGE(vol));
   gtk_range_clear_background(GTK_RANGE(vol));
  //gtk_range_draw_background(GTK_RANGE(vol));
   if (master_muted == 0 && psptr && psptr->session_has_audio()) {
     psptr->set_audio_volume(master_volume);
   }
}

static void on_video_radio (GtkWidget *window, gpointer data)
{
  int newsize = GPOINTER_TO_INT(data);
  if (newsize != master_screen_size) {
    master_screen_size = newsize;
    if (psptr != NULL) {
      psptr->set_screen_size(newsize / 50);
    }
  }
}

static void on_video_fullscreen (GtkWidget *window, gpointer data)
{
  player_debug_message("Fullscreen");
  if (psptr != NULL) {
    psptr->set_screen_size(master_screen_size / 50, 1);
  }
}

static void on_time_slider_pressed (GtkWidget *window, gpointer data)
{
  time_slider_pressed = 1;
}

static void on_time_slider_adjusted (GtkWidget *window, gpointer data)
{
  double maxtime, newtime;
  time_slider_pressed = 0;
  if (psptr == NULL) 
    return;
  maxtime = psptr->get_max_time();
  if (maxtime == 0.0)
    return;
  GtkAdjustment *val = gtk_range_get_adjustment(GTK_RANGE(time_slider));
  newtime = (maxtime * val->value) / 100.0;
  SDL_mutexP(command_mutex);
  if (play_state == PLAYING) {
    psptr->pause_all_media();
  }
  // If we're going all the way back to the beginning, indicate that
  psptr->play_all_media(newtime == 0.0 ? TRUE : FALSE, newtime);
  adjust_gui_for_play();
  SDL_mutexV(command_mutex);
  gtk_range_set_adjustment(GTK_RANGE(time_slider), val);
  gtk_range_slider_update(GTK_RANGE(time_slider));
  gtk_range_clear_background(GTK_RANGE(time_slider));
  //gtk_range_draw_background(GTK_RANGE(vol));
}

/*
 * Main timer routine - runs ~every 500 msec
 * Might be able to adjust this to handle just when playing.
 */
static gint main_timer (gpointer raw)
{
  if (play_state == PLAYING) {
    double val = psptr->get_max_time();
    double playtime = psptr->get_playing_time();
    if (time_slider_pressed == 0 && 
	val > 0.0 &&
	psptr->get_session_state() == SESSION_PLAYING) {
      if (playtime < 0) {
	val = 0.0;
      } else if (playtime > val) {
	val = 100.0;
      } else {
	val = (psptr->get_playing_time() * 100.0) / val;
      }
      GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(time_slider));
      adj->value = val;
      gtk_range_set_adjustment(GTK_RANGE(time_slider), adj);
      gtk_range_slider_update(GTK_RANGE(time_slider));
      gtk_range_clear_background(GTK_RANGE(time_slider));
      gtk_range_draw_background(GTK_RANGE(time_slider));
      gtk_widget_show(time_slider);
    }
    int hr, min, tot;
    tot = (int) playtime;
    hr = tot / 3600;
    tot %= 3600;
    min = tot / 60;
    tot %= 60;
    playtime -= (double)((hr * 3600) + (min * 60));
    gchar buffer[30];
    g_snprintf(buffer, 30, "%d:%02d:%04.1f", hr, min, playtime);
    gtk_label_set_text(GTK_LABEL(time_disp), buffer);
    
  }
  CMsg *newmsg;
  while ((newmsg = master_queue.get_message()) != NULL) {
    switch (newmsg->get_value()) {
    case MSG_RECEIVED_QUIT:
      //player_debug_message("received quit");
      on_main_menu_close(NULL, 0);
      break;
    }
  }
  return (TRUE);  // keep timer going
}

/*
 * Main routine - set up window
 */
int main (int argc, char **argv)
{
  gtk_init(&argc, &argv);

  command_mutex = SDL_CreateMutex();

  rtsp_set_loglevel(LOG_DEBUG);
  /*
   * Set up main window
   */
  main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_policy(GTK_WINDOW(main_window), FALSE, TRUE, FALSE);
  gtk_window_set_title(GTK_WINDOW(main_window), 
		       "cisco Open Source MPEG4 Player");
  gtk_widget_set_uposition(GTK_WIDGET(main_window), 10, 10);
  gtk_widget_set_usize(main_window, 450, 122);
  gtk_window_set_default_size(GTK_WINDOW(main_window), 450, 122);
  gtk_signal_connect(GTK_OBJECT(main_window),
		     "delete_event",
		     GTK_SIGNAL_FUNC(delete_event),
		     NULL);
  main_vbox = gtk_vbox_new(FALSE, 1);
  gtk_widget_show(main_vbox);
  // add stuff
  accel_group = gtk_accel_group_new();
  gtk_accel_group_attach(accel_group, GTK_OBJECT(main_window));
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
  GtkWidget *videoradio;
  videoradio = CreateMenuRadio(videosub,
			       "50 %",
			       &videosizelist,
			       GTK_SIGNAL_FUNC(on_video_radio),
			       GINT_TO_POINTER(50));
  videoradio = CreateMenuRadio(videosub,
			       "100 %",
			       &videosizelist,
			       GTK_SIGNAL_FUNC(on_video_radio),
			       GINT_TO_POINTER(100));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(videoradio), TRUE);
  videoradio = CreateMenuRadio(videosub,
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
  menuitem = CreateMenuItem(menu,
			    accel_group,
			    tooltips, 
			    "Debug",
			    NULL,
			    "Display Debug/Message Window",
			    GTK_SIGNAL_FUNC(on_main_menu_debug),
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
  playlist = g_list_append(playlist, (void *)"");
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

  /*
   * 2nd line  play pause stop speaker volume
   */
  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 5);
  gtk_widget_show(hbox);

  GtkWidget *image;
  
  play_button = gtk_toggle_button_new();
  image = CreateWidgetFromXpm(main_window,xpm_play);
  gtk_container_add(GTK_CONTAINER(play_button), image);
  gdk_pixmap_unref((GdkPixmap *)image);
  gtk_widget_show(play_button);
  gtk_signal_connect(GTK_OBJECT(play_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_play_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox), play_button, FALSE, FALSE, 2);

  pause_button = gtk_toggle_button_new();
  image = CreateWidgetFromXpm(main_window,xpm_pause);
  gtk_container_add(GTK_CONTAINER(pause_button), image);
  gdk_pixmap_unref((GdkPixmap *)image);
  gtk_widget_show(pause_button);
  gtk_signal_connect(GTK_OBJECT(pause_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_pause_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox), pause_button, FALSE, FALSE, 2);

  stop_button = gtk_toggle_button_new();
  image = CreateWidgetFromXpm(main_window,xpm_stop);
  gtk_container_add(GTK_CONTAINER(stop_button), image);
  gdk_pixmap_unref((GdkPixmap *)image);
  gtk_widget_show(stop_button);
  gtk_signal_connect(GTK_OBJECT(stop_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_stop_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox), stop_button, FALSE, FALSE, 2);


  gtk_widget_set_sensitive(play_button, 0);
  gtk_widget_set_sensitive(pause_button, 0);
  gtk_widget_set_sensitive(stop_button, 0);

  GtkWidget *sep = gtk_vseparator_new();
  gtk_widget_show(sep);
  gtk_box_pack_start(GTK_BOX(hbox), sep, FALSE, FALSE, 10);

  master_muted = 0;
  speaker_button = gtk_button_new();
  image = CreateWidgetFromXpm(speaker_button,xpm_speaker);
  gtk_container_add(GTK_CONTAINER(speaker_button), image);
  gdk_pixmap_unref((GdkPixmap *)image);
  gtk_widget_show(speaker_button);
  gtk_signal_connect(GTK_OBJECT(speaker_button),
		     "clicked",
		     GTK_SIGNAL_FUNC(on_speaker_clicked),
		     NULL);
  gtk_box_pack_start(GTK_BOX(hbox), speaker_button, FALSE, FALSE, 0);

  GtkObject *vol_adj;
  master_volume = 75;
  vol_adj = gtk_adjustment_new(75.0,
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
  gtk_range_slider_update ( GTK_RANGE( vol ) );
  gtk_range_clear_background ( GTK_RANGE( vol ) );
  //gtk_range_draw_background ( GTK_RANGE( vol ) );

#if 0
  // We don't need this - I'm not 100% sure why, but we don't...
  gtk_signal_connect(vol_adj,
		     "value_changed",
		     GTK_SIGNAL_FUNC(on_volume_adjusted),
		     vol);
#endif
  gtk_signal_connect(GTK_OBJECT(vol), 
		     "button_release_event",
		     GTK_SIGNAL_FUNC(on_volume_adjusted),
		     vol);
  gtk_widget_show(vol);
  volume_slider = vol;
  gtk_box_pack_start(GTK_BOX(hbox), vol, FALSE, TRUE, 10);

  
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
				  1.0,
				  0.0,
				  0.0);
  
  time_slider = gtk_hscale_new(GTK_ADJUSTMENT(time_slider_adj));
  gtk_widget_ref(time_slider);
  gtk_range_set_update_policy(GTK_RANGE(time_slider), GTK_UPDATE_DELAYED);
  gtk_scale_set_digits(GTK_SCALE(time_slider),0);
  gtk_scale_set_draw_value(GTK_SCALE(time_slider), 0);
  gtk_range_set_adjustment(GTK_RANGE(time_slider), 
			   GTK_ADJUSTMENT(time_slider_adj));
  gtk_range_slider_update(GTK_RANGE(time_slider));
  gtk_range_clear_background(GTK_RANGE( time_slider));
  //gtk_range_draw_background(GTK_RANGE(time_slider));
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
  argc--;
  argv++;
  if (argc > 0) {
    start_session_from_name(*argv);
  }
  gtk_main();
  exit(0);
}

/* end gui_main.cpp */
