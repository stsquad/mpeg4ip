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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 * 		Bill May        wmay@cisco.com
 * 		Dave Mackie 	dmackie@cisco.com
 */

#include "mp4live.h"
#include "mp4live_gui.h"

#include <mp4.h>
#include "mpeg2_ps.h"

GtkWidget* CreateFileCombo(const char* entryText)
{
	GList* list = NULL;

	list = g_list_append(list, strdup(entryText));

	for (int i = 0; i < NUM_FILE_HISTORY; i++) {
	  const char* fileName =
			MyConfig->GetStringValue(CONFIG_APP_FILE_0 + i);
		if (fileName == NULL || fileName[0] == '\0'
		  || !strcmp(fileName, entryText)) {
			continue;
		}
		list = g_list_append(list, strdup(fileName));
	}

	GtkWidget* combo = gtk_combo_new();
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);
	gtk_combo_set_use_arrows_always(GTK_COMBO(combo), 1);

	GtkWidget* entry = GTK_COMBO(combo)->entry;
	gtk_entry_set_text(GTK_ENTRY(entry), entryText);
	return combo;
}

static GtkWidget* FileSelection;
static GtkWidget* FileEntry;
static GtkSignalFunc FileOkFunc;

static void on_filename_selected(
	GtkFileSelection *widget, 
	gpointer data)
{
	const gchar *name =
		gtk_file_selection_get_filename(GTK_FILE_SELECTION(FileSelection));

	gtk_entry_set_text(GTK_ENTRY(FileEntry), name);
	gtk_widget_show(FileEntry);

	gtk_grab_remove(FileSelection);
	gtk_widget_destroy(FileSelection);

	if (FileOkFunc) {
		(*FileOkFunc)();
	}
}

void FileBrowser(
	GtkWidget* fileEntry, 
	GtkSignalFunc okFunc)
{
	FileEntry = fileEntry;
	FileOkFunc = okFunc;

	FileSelection = gtk_file_selection_new("Select File");
	gtk_file_selection_hide_fileop_buttons(
		GTK_FILE_SELECTION(FileSelection));

	gtk_signal_connect(
		GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->ok_button),
		"clicked",
		GTK_SIGNAL_FUNC(on_filename_selected),
		FileSelection);

	gtk_signal_connect_object(
		GTK_OBJECT(GTK_FILE_SELECTION(FileSelection)->cancel_button),
		"clicked",
		GTK_SIGNAL_FUNC(gtk_widget_destroy),
		GTK_OBJECT(FileSelection));

	gtk_widget_show(FileSelection);
	gtk_grab_add(FileSelection);
}

bool IsDevice(const char* fileName)
{
	// LATER might also want to stat()
	return !strncmp(fileName, "/dev/", 5);
}

bool IsUrl(const char* name)
{
	return (strchr(name, ':') != NULL);
}

bool IsMp4File(const char* fileName)
{
	if (IsUrl(fileName)) {
		return false;
	}
	if (strlen(fileName) <= 4) {
		return false;
	} 
	return !strcmp(&fileName[strlen(fileName) - 4], ".mp4");
}

bool IsMpeg2File(const char* fileName)
{
	if (IsUrl(fileName)) {
		return false;
	}
	if (strlen(fileName) <= 4) {
		return false;
	} 

	const char* ext3 = &fileName[strlen(fileName) - 4];

	return !strcmp(ext3, ".mpg")
		|| !strcmp(ext3, ".vob");
}

int32_t Mp4FileDefaultAudio(const char* fileName)
{
	MP4FileHandle mp4File = MP4Read(fileName);

	if (mp4File == MP4_INVALID_FILE_HANDLE) {
		return -1;
	}

	MP4TrackId trackId = 
		MP4FindTrackId(mp4File, 0, MP4_AUDIO_TRACK_TYPE);

	MP4Close(mp4File);

	if (trackId == MP4_INVALID_TRACK_ID) {
		return -1;
	}
	return trackId;
}

int32_t Mpeg2FileDefaultAudio(const char* fileName)
{
  mpeg2ps_t *mpeg2File = mpeg2ps_init(fileName);

	if (!mpeg2File) {
		return -1;
	}

	int32_t audioTracks = mpeg2ps_get_audio_stream_count(mpeg2File);

	mpeg2ps_close(mpeg2File);

	if (audioTracks <= 0) {
		return -1;
	}
	return 0; 
}

int32_t FileDefaultAudio(const char* fileName)
{
	if (IsMp4File(fileName)) {
		return Mp4FileDefaultAudio(fileName);
	} else if (IsMpeg2File(fileName)) {
		return Mpeg2FileDefaultAudio(fileName);
	} else {
		return 0;
	}
}

#if 0
static char** trackNames;
static u_int32_t* pTrackIndex;

static void on_track_menu_activate(GtkWidget *widget, gpointer data)
{
	*pTrackIndex = GPOINTER_TO_UINT(data) & 0xFF;
}

static GtkWidget* CreateNullTrackMenu(
	GtkWidget* menu,
	char type,
	const char* source,
	u_int32_t* pIndex,
	u_int32_t* pNumber,
	u_int32_t** ppValues)
{
	u_int32_t* newTrackValues = 
		(u_int32_t*)malloc(sizeof(u_int32_t));
	newTrackValues[0] = 0;

	char** newTrackNames = 
		(char**)malloc(sizeof(char*));
	newTrackNames[0] = strdup("");

	// (re)create the menu
	menu = CreateOptionMenu(
		menu,
		newTrackNames, 
		1,
		0,
		GTK_SIGNAL_FUNC(on_track_menu_activate));

	// free up old names
	for (u_int8_t i = 0; i < *pNumber; i++) {
		free(trackNames[i]);
	}
	free(*ppValues);
	free(trackNames);

	*pIndex = 0;
	*pNumber = 1;
	*ppValues = newTrackValues;
	trackNames = newTrackNames;

	return menu;
}

static GtkWidget* CreateMp4TrackMenu(
	GtkWidget* menu,
	char type,
	const char* source,
	u_int32_t* pIndex,
	u_int32_t* pNumber,
	u_int32_t** ppValues)
{
	*pIndex = 0;

	u_int32_t newTrackNumber = 1;

	MP4FileHandle mp4File = MP4Read(source);

	char* trackType = NULL;

	if (mp4File) {
		if (type == 'V') {
			trackType = MP4_VIDEO_TRACK_TYPE;
		} else {
			trackType = MP4_AUDIO_TRACK_TYPE;
		}
		newTrackNumber = 
			MP4GetNumberOfTracks(mp4File, trackType);
	}

	u_int32_t* newTrackValues = 
		(u_int32_t*)malloc(sizeof(u_int32_t) * newTrackNumber);

	char** newTrackNames = 
		(char**)malloc(sizeof(char*) * newTrackNumber);

	if (!mp4File) {
		newTrackValues[0] = 0;
		newTrackNames[0] = strdup("");
	} else {
		for (u_int8_t i = 0; i < newTrackNumber; i++) {
			MP4TrackId trackId =
				MP4FindTrackId(mp4File, i, trackType);

			char* trackName = "Unknown";
			char buf[64];

			if (trackType == MP4_VIDEO_TRACK_TYPE) {
				u_int8_t videoType =
					MP4GetTrackEsdsObjectTypeId(mp4File, 
								    trackId);

				switch (videoType) {
				case MP4_MPEG1_VIDEO_TYPE:
					trackName = "MPEG1";
					break;
				case MP4_MPEG2_SIMPLE_VIDEO_TYPE:
				case MP4_MPEG2_MAIN_VIDEO_TYPE:
				case MP4_MPEG2_SNR_VIDEO_TYPE:
				case MP4_MPEG2_SPATIAL_VIDEO_TYPE:
				case MP4_MPEG2_HIGH_VIDEO_TYPE:
				case MP4_MPEG2_442_VIDEO_TYPE:
					trackName = "MPEG2";
					break;
				case MP4_MPEG4_VIDEO_TYPE:
					trackName = "MPEG4";
					break;
				case MP4_YUV12_VIDEO_TYPE:
					trackName = "YUV12";
					break;
				case MP4_H263_VIDEO_TYPE:
					trackName = "H263";
					break;
				case MP4_H261_VIDEO_TYPE:
					trackName = "H261";
					break;
				}
			
				snprintf(buf, sizeof(buf), 
					"%u - %s %u x %u %.2f fps %u kbps", 
					trackId, 
					trackName,
					MP4GetTrackVideoWidth(mp4File, trackId),
					MP4GetTrackVideoHeight(mp4File, trackId),
					MP4GetTrackVideoFrameRate(mp4File, trackId),
					(MP4GetTrackBitRate(mp4File, trackId) + 500) / 1000);

			} else { // audio
				u_int8_t audioType =
					MP4GetTrackEsdsObjectTypeId(mp4File, 
								    trackId);

				switch (audioType) {
				case MP4_MPEG1_AUDIO_TYPE:
				case MP4_MPEG2_AUDIO_TYPE:
					trackName = "MPEG (MP3)";
					break;
				case MP4_MPEG2_AAC_MAIN_AUDIO_TYPE:
				case MP4_MPEG2_AAC_LC_AUDIO_TYPE:
				case MP4_MPEG2_AAC_SSR_AUDIO_TYPE:
				case MP4_MPEG4_AUDIO_TYPE:
					trackName = "AAC";
					break;
				case MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE:
					trackName = "PCM16 LITTLE ENDIAN";
					break;
				case MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE:
				  trackName = "PCM16 BIG ENDIAN";
				case MP4_AC3_AUDIO_TYPE:
					trackName = "AC3";
					break;
				case MP4_VORBIS_AUDIO_TYPE:
					trackName = "Ogg Vorbis";
					break;
				case MP4_ALAW_AUDIO_TYPE:
					trackName = "G711 aLaw";
					break;
				case MP4_ULAW_AUDIO_TYPE:
					trackName = "G711 uLaw";
					break;
				}

				snprintf(buf, sizeof(buf), 
					"%u - %s %u kbps", 
					trackId, 
					trackName,
					(MP4GetTrackBitRate(mp4File, trackId) + 500) / 1000);
			}

			newTrackValues[i] = trackId;
			newTrackNames[i] = strdup(buf);
		}

		MP4Close(mp4File);
	}

	// (re)create the menu
	menu = CreateOptionMenu(
		menu,
		newTrackNames, 
		newTrackNumber,
		*pIndex,
		GTK_SIGNAL_FUNC(on_track_menu_activate));

	// free up old names
	for (u_int8_t i = 0; i < *pNumber; i++) {
		free(trackNames[i]);
	}
	free(trackNames);
	free(*ppValues);

	*pNumber = newTrackNumber;
	trackNames = newTrackNames;
	*ppValues = newTrackValues;

	return menu;
}

static GtkWidget* CreateMpeg2TrackMenu(
	GtkWidget* menu,
	char type,
	const char* source,
	u_int32_t* pIndex,
	u_int32_t* pNumber,
	u_int32_t** ppValues)
{
	*pIndex = 0;

	u_int32_t newTrackNumber = 1;

	mpeg2ps_t* mpeg2File = mpeg2ps_init(source);

	if (mpeg2File) {
		if (type == 'V') {
		  newTrackNumber = mpeg2ps_get_video_stream_count(mpeg2File);
		} else {
		  newTrackNumber = mpeg2ps_get_audio_stream_count(mpeg2File);
		}
	}

	u_int32_t* newTrackValues = 
		(u_int32_t*)malloc(sizeof(u_int32_t) * newTrackNumber);

	char** newTrackNames = 
		(char**)malloc(sizeof(char*) * newTrackNumber);

	if (!mpeg2File) {
		newTrackValues[0] = 0;
		newTrackNames[0] = strdup("");
	} else {
		for (u_int8_t i = 0; i < newTrackNumber; i++) {
			newTrackValues[i] = i;

			char buf[64];
			if (type == 'V') {
			  snprintf(buf, sizeof(buf), 
				   "%u - %u x %u @ %.2f fps", 
				   i + 1,
				   mpeg2ps_get_video_stream_width(mpeg2File, i),
				   mpeg2ps_get_video_stream_height(mpeg2File, i),
				   mpeg2ps_get_video_stream_framerate(mpeg2File, i));
			} else {
			  const char* afmt =
			    mpeg2ps_get_audio_stream_name(mpeg2File, i);
			  
			  // use more familar though less accurate name
			  snprintf(buf, sizeof(buf), 
				   "%u - %s  %u channels @ %u Hz", 
				   i + 1,
				   afmt,
				   mpeg2ps_get_audio_stream_channels(mpeg2File, i),
				   mpeg2ps_get_audio_stream_sample_freq(mpeg2File, i));
			}
			newTrackNames[i] = strdup(buf);
		}
		mpeg2ps_close(mpeg2File);
	}

	// (re)create the menu
	menu = CreateOptionMenu(
		menu,
		newTrackNames, 
		newTrackNumber,
		*pIndex,
		GTK_SIGNAL_FUNC(on_track_menu_activate));

	// free up old names
	for (u_int8_t i = 0; i < *pNumber; i++) {
		free(trackNames[i]);
	}
	free(trackNames);
	free(*ppValues);

	*pNumber = newTrackNumber;
	trackNames = newTrackNames;
	*ppValues = newTrackValues;
	return menu;
}

GtkWidget* CreateTrackMenu(
	GtkWidget* menu,
	char type, 
	const char* source,
	u_int32_t* pMenuIndex,
	u_int32_t* pMenuNumber,
	u_int32_t** ppMenuValues)
{
	pTrackIndex = pMenuIndex;

	if (IsMp4File(source)) {
		return CreateMp4TrackMenu(
			menu, type, source, pMenuIndex, pMenuNumber, ppMenuValues);	

	} else if (IsMpeg2File(source)) {
		return CreateMpeg2TrackMenu(
			menu, type, source, pMenuIndex, pMenuNumber, ppMenuValues);	

	} else {
		return CreateNullTrackMenu(
			menu, type, source, pMenuIndex, pMenuNumber, ppMenuValues);	
	}
}

#endif
