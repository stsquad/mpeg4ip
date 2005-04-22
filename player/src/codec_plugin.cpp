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
 * Copyright (C) Cisco Systems Inc. 2002-2005.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 */
/*
 * codec_plugin.cpp - read and process for plugins
 */

#ifndef _WIN32
#include <sys/types.h>
#include <dlfcn.h>
#include <dirent.h>
#endif

#include "codec_plugin_private.h"
#include "mpeg4ip_utils.h"
#if 0
#include "player_util.h"
#include "our_config_file.h"
#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_file.h"
#include "audio.h"
#include "video.h"
#endif

#include "rtp_plugin.h"
/*
 * Portability hacks
 */
#ifdef _WIN32
#define LIBRARY_HANDLE    HINSTANCE
#define FILE_HANDLE       void *
#define PLAYER_PLUGIN_DIR "."
typedef struct dir_list_t {
  WIN32_FIND_DATA dptr;
  HANDLE shandle;
} dir_list_t;

#else
#ifdef HAVE_MACOSX
#include <mach-o/dyld.h>
#define LIBRARY_HANDLE NSModule
#define FILE_HANDLE NSObjectFileImage
#else
#define LIBRARY_HANDLE    void *
#define FILE_HANDLE       void *
#endif
// PLAYER_PLUGIN_DIR is set via configure.in
typedef struct dir_list_t {
  DIR *dptr;
  char fname[PATH_MAX];
} dir_list_t;
#endif

/*
 * codec_plugin_list_t is how we store the codec links
 */
typedef enum {
  CODEC_TYPE_AUDIO, 
  CODEC_TYPE_VIDEO,
  CODEC_TYPE_TEXT
} codec_type_t;

typedef struct plugin_list_t {
  struct plugin_list_t *next_plugin;
  FILE_HANDLE    file_handle;
  LIBRARY_HANDLE dl_handle;
  codec_type_t codec_type;
  codec_plugin_t *codec;
  rtp_plugin_t *rtp_plugin;
} plugin_list_t;

static plugin_list_t *plugins;

static void close_file_search (dir_list_t *ptr)
{
#ifndef _WIN32
  if (ptr->dptr != NULL)
    closedir(ptr->dptr);
#endif
}
/*
 * portable way to find the next file.  In unix, we're looking for
 * any .so files.  In windows, we've already started looking for .dll
 */
static const char *find_next_file (dir_list_t *ptr,
				   const char *dirname)
{
#ifndef _WIN32
  struct dirent *fptr;

  while ((fptr = readdir(ptr->dptr)) != NULL) {
    int len = strlen(fptr->d_name);
    if (len > 3 && strcmp(fptr->d_name + len - 3, ".so") == 0) {
      sprintf(ptr->fname, "%s/%s", 
	      dirname, fptr->d_name);
#ifdef HAVE_MACOSX
      struct stat statbuf;
      if (lstat(ptr->fname, &statbuf) == 0 &&
	  S_ISREG(statbuf.st_mode) &&
	  !S_ISLNK(statbuf.st_mode))
	return (ptr->fname);
#else
	return (ptr->fname);
#endif
    }
  }
  return NULL;
#else
  if (FindNextFile(ptr->shandle, &ptr->dptr) == FALSE)
    return NULL;
  return ptr->dptr.cFileName;
#endif
}

/*
 * get_first_file
 * start directory search for plugin files
 */
static const char *get_first_file (dir_list_t *ptr, 
				   const char *dirname)
{
#ifndef _WIN32
  ptr->dptr = opendir(dirname);
  if (ptr->dptr == NULL) 
    return NULL;
  return (find_next_file(ptr, dirname));
#else
  ptr->shandle = FindFirstFile("*.dll", &ptr->dptr);
  if (ptr->shandle == INVALID_HANDLE_VALUE) return NULL;
  return ptr->dptr.cFileName;
#endif
}
static bool open_library (plugin_list_t *p, const char *fname)
{
#ifdef _WIN32
    p->dl_handle = LoadLibrary(fname);
#else
#ifdef HAVE_MACOSX
    NSObjectFileImageReturnCode ret;
    ret = NSCreateObjectFileImageFromFile(fname, &p->file_handle);
    if (ret != NSObjectFileImageSuccess) {
      message(LOG_ERR, "plugin", "error on file %s", fname);
      return false;
    }
    p->dl_handle = NSLinkModule(p->file_handle, fname, 
				NSLINKMODULE_OPTION_BINDNOW |
				NSLINKMODULE_OPTION_PRIVATE |
				NSLINKMODULE_OPTION_RETURN_ON_ERROR);
    // see if this works for xvid
    if (p->dl_handle == NULL) {
      int err;
      const char *fname, *errt;
      NSLinkEditErrors c;
      NSLinkEditError(&c, &err, &fname, &errt);
      message(LOG_ERR, "plugin", "link error %d %s %s",
			   err, fname, errt);
    }
    NSDestroyObjectFileImage(p->file_handle);
    p->file_handle = NULL;
#else
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif
    p->dl_handle = dlopen(fname, RTLD_NOW | RTLD_LOCAL);
#endif
#endif
    return p->dl_handle != NULL;
}

static const char *get_open_error (plugin_list_t *p) 
{
#ifdef _WIN32
  return "stupid, stupid windows";
#else
  return dlerror();
#endif
}

static void *get_symbol_address (plugin_list_t *p, const char *symbol)
{
#ifdef _WIN32
  return GetProcAddress(p->dl_handle, symbol);
#else 
#ifdef HAVE_MACOSX
  NSSymbol sym;
  sym = NSLookupSymbolInModule(p->dl_handle, symbol);
  if (sym == NULL) return NULL;
  return NSAddressOfSymbol(sym);
#else
    
  return dlsym(p->dl_handle, symbol);
#endif
#endif
}

static void close_library (plugin_list_t *p)
{
#ifdef _WIN32
  FreeLibrary(p->dl_handle);
#else
#ifdef HAVE_MACOSX
  if (p->dl_handle)
    NSUnLinkModule(p->dl_handle, NSUNLINKMODULE_OPTION_NONE);
  if (p->file_handle)
    NSDestroyObjectFileImage(p->file_handle);
#else
  dlclose(p->dl_handle);
#endif
#endif
}

/*
 * initialize_plugins
 * Start search for plugins - classify them as audio or video
 */
void initialize_plugins (CConfigSet *pConfig)
{
  codec_plugin_t *cptr;
  rtp_plugin_t *rptr;
  dir_list_t dir;
  const char *fname;

  plugins = NULL;
  fname = get_first_file(&dir, PLAYER_PLUGIN_DIR);
  plugin_list_t *p;
  
  p = NULL;
  while (fname != NULL) {
    if (p == NULL) {
      p = MALLOC_STRUCTURE(plugin_list_t);
      memset(p, 0, sizeof(*p));
      if (p == NULL) exit(-1);
    }

    if (open_library(p, fname)) {
      cptr = (codec_plugin_t *)get_symbol_address(p, "mpeg4ip_codec_plugin");
      if (cptr == NULL) {
	cptr = (codec_plugin_t *)get_symbol_address(p, "_mpeg4ip_codec_plugin");
      }
      if (cptr != NULL) {
	if (strcmp(cptr->c_version, PLUGIN_VERSION) == 0) {
	  bool add = true;
	  if (strcmp(cptr->c_type, "audio") == 0) {
	    p->codec = cptr;
	    p->codec_type = CODEC_TYPE_AUDIO;
	    message(LOG_INFO, "plugin", "Adding audio plugin %s %s", 
		    cptr->c_name, fname);
	  } else if (strcmp(cptr->c_type, "video") == 0) {
	    p->codec = cptr;
	    p->codec_type = CODEC_TYPE_VIDEO;
	    message(LOG_INFO, "plugin", "Adding video plugin %s %s", 
		    cptr->c_name, fname);
	  } else if (strcmp(cptr->c_type, "text") == 0) {
	    p->codec = cptr;
	    p->codec_type = CODEC_TYPE_TEXT;
	    message(LOG_INFO, "plugin", "Adding text plugin %s %s", 
		    cptr->c_name, fname);
	  } else {
	    add = false;
	    message(LOG_CRIT, "plugin", "Unknown plugin type %s in plugin %s", 
		    cptr->c_type, fname);
	  }

	      
	} else {
	  message(LOG_ERR, "plugin", "Plugin %s(%s) has wrong version %s", 
		  fname, cptr->c_type, cptr->c_version);
	}
      }
      rptr = (rtp_plugin_t *)get_symbol_address(p, RTP_PLUGIN_EXPORT_NAME_STR);
      if (rptr == NULL) {
	rptr = (rtp_plugin_t *)get_symbol_address(p, "_" RTP_PLUGIN_EXPORT_NAME_STR);
      }
      if (rptr != NULL) {
	if (strcmp(rptr->version, RTP_PLUGIN_VERSION) == 0) {
	  p->rtp_plugin = rptr;
	  message(LOG_INFO, "plugin", "Adding RTP plugin %s %s", 
		  rptr->name, fname);
	} else {
	  message(LOG_CRIT, "plugin", "Plugin %s (%s) has wrong version %s", 
		  rptr->name, fname, rptr->version);
	}
      }
      if (rptr == NULL && cptr == NULL) {
	message(LOG_ERR, "plugin", "Can't find export point in plugin %s", 
		fname);
      }
    } else {
      message(LOG_ERR, "plugin", "Can't dlopen plugin %s - %s", fname,
	      get_open_error(p));
    }
    if (p->codec != NULL || p->rtp_plugin != NULL) {
      if (p->codec != NULL) {
	if (p->codec->c_variable_list != NULL &&
	    p->codec->c_variable_list_count > 0) {
	  if (pConfig != NULL) {
	    pConfig->AddConfigVariables(p->codec->c_variable_list,
					p->codec->c_variable_list_count);
	  }
	}
      }
      p->next_plugin = plugins;
      plugins = p;
      p = NULL; // so we add a new one    } 
    } else {
      close_library(p);
    }
    fname = find_next_file(&dir, PLAYER_PLUGIN_DIR);
  }
  close_file_search(&dir);
}

/*
 * check_for_audio_codec
 * search the list of audio codecs for one that matches the parameters
 */
codec_plugin_t *check_for_audio_codec (const char *stream_type,
				       const char *compressor,
				       format_list_t *fptr,
				       int audio_type,
				       int profile, 
				       const uint8_t *userdata,
				       uint32_t userdata_size,
				       CConfigSet *pConfig)
{
  plugin_list_t *aptr;
  int best_value = 0;
  codec_plugin_t *ret;

  ret = NULL;
  aptr = plugins;
  while (aptr != NULL) {
    if (aptr->codec != NULL &&
	aptr->codec_type == CODEC_TYPE_AUDIO &&
	aptr->codec->c_compress_check != NULL) {
      int temp;
      temp = (aptr->codec->c_compress_check)(message,
					     stream_type,
					     compressor,
					     audio_type,
					     profile, 
					     fptr,
					     userdata,
					     userdata_size,
					     pConfig);
      if (temp > best_value) {
	best_value = temp;
	ret = aptr->codec;
      }
    }
    aptr = aptr->next_plugin;
  }
  if (ret != NULL) {
    message(LOG_DEBUG, "plugin", 
	    "Found matching audio plugin %s", ret->c_name);
  }
  return (ret);
}

/*
 * check_for_video_codec
 * search the list of video plugins  for one that matches the parameters
 */
codec_plugin_t *check_for_text_codec (const char *stream_type,
				      const char *compressor,
				      format_list_t *fptr,
				      const uint8_t *userdata,
				      uint32_t userdata_size,
				      CConfigSet *pConfig)
{
  plugin_list_t *vptr;
  int best_value = 0;
  codec_plugin_t *ret;

  ret = NULL;
  vptr = plugins;
  while (vptr != NULL) {
    if (vptr->codec && 
	vptr->codec_type == CODEC_TYPE_TEXT &&
	vptr->codec->c_compress_check != NULL) {
      int temp;
      temp = (vptr->codec->c_compress_check)(message,
					     stream_type,
					     compressor,
					     0,
					     0, 
					     fptr,
					     userdata,
					     userdata_size,
					     pConfig);
      if (temp > best_value) {
	best_value = temp;
	ret = vptr->codec;
      }
    }
    vptr = vptr->next_plugin;
  }
  if (ret != NULL) {
    message(LOG_DEBUG, "plugin", 
	    "Found matching text plugin %s", ret->c_name);
  }
  return (ret);
}
/*
 * check_for_video_codec
 * search the list of video plugins  for one that matches the parameters
 */
codec_plugin_t *check_for_video_codec (const char *stream_type,
				       const char *compressor,
				       format_list_t *fptr,
				       int type,
				       int profile, 
				       const uint8_t *userdata,
				       uint32_t userdata_size,
				       CConfigSet *pConfig)
{
  plugin_list_t *vptr;
  int best_value = 0;
  codec_plugin_t *ret;

  ret = NULL;
  vptr = plugins;
  while (vptr != NULL) {
    if (vptr->codec && 
	vptr->codec_type == CODEC_TYPE_VIDEO &&
	vptr->codec->c_compress_check != NULL) {
      int temp;
      temp = (vptr->codec->c_compress_check)(message,
					     stream_type,
					     compressor,
					     type,
					     profile, 
					     fptr,
					     userdata,
					     userdata_size,
					     pConfig);
      if (temp > best_value) {
	best_value = temp;
	ret = vptr->codec;
      }
    }
    vptr = vptr->next_plugin;
  }
  if (ret != NULL) {
    message(LOG_DEBUG, "plugin", 
	    "Found matching video plugin %s", ret->c_name);
  }
  return (ret);
}

rtp_check_return_t check_for_rtp_plugins (format_list_t *fptr,
					  uint8_t rtp_payload_type,
					  rtp_plugin_t **rtp_plugin,
					  CConfigSet *pConfig)
{
  plugin_list_t *r;
  rtp_plugin_t *rptr;
  rtp_check_return_t ret;

  *rtp_plugin = NULL;
  r = plugins;

  while (r != NULL) {
    rptr = r->rtp_plugin;

    if (rptr && rptr->rtp_plugin_check != NULL) {
      ret = (rptr->rtp_plugin_check)(message, fptr, rtp_payload_type, pConfig);
      if (ret != RTP_PLUGIN_NO_MATCH) {
	*rtp_plugin = rptr;
	return ret;
      }
    }
    r = r->next_plugin;
  }

  return RTP_PLUGIN_NO_MATCH;
}

/*
 * video_codec_check_for_raw_file
 * goes through list of video codecs to see if "name" has a raw file
 * match
 */
codec_data_t *video_codec_check_for_raw_file (const char *name,
					      codec_plugin_t **codec,
					      double *maxtime,
					      char **desc,
					      CConfigSet *pConfig)
{
  plugin_list_t *vptr;
  codec_data_t *cifptr;

  vptr = plugins;
  
  while (vptr != NULL) {
    if (vptr->codec != NULL &&
	vptr->codec_type == CODEC_TYPE_VIDEO &&
	vptr->codec->c_raw_file_check != NULL) {
      message(LOG_DEBUG, "plugin", 
	      "Trying raw file codec %s", vptr->codec->c_name);
      cifptr = vptr->codec->c_raw_file_check(message,
					     name,
					     maxtime,
					     desc,
					     pConfig);
      if (cifptr != NULL) {
	*codec = vptr->codec;
	message(LOG_DEBUG, "plugin", "Found raw file codec %s", vptr->codec->c_name);
#if 0

	return 0;
#endif
	return cifptr;
      }
    } 
    vptr = vptr->next_plugin;
  }
  return NULL;
}

/*
 * audio_codec_check_for_raw_file
 * goes through the list of audio codecs, looking for raw file
 * support
 */
codec_data_t *audio_codec_check_for_raw_file (const char *name,
					      codec_plugin_t **codec,
					      double *maxtime,
					      char **desc,
					      CConfigSet *pConfig)
{
  plugin_list_t *aptr;
  codec_data_t *cifptr;


  aptr = plugins;
  while (aptr != NULL) {

    if (aptr->codec != NULL &&
	aptr->codec_type == CODEC_TYPE_AUDIO &&
	aptr->codec->c_raw_file_check != NULL) {
      message(LOG_DEBUG, "plugin", 
	      "Trying raw file codec %s", aptr->codec->c_name);
      cifptr = aptr->codec->c_raw_file_check(message,
					     name,
					     maxtime,
					     desc,
					     pConfig);
      if (cifptr != NULL) {
	*codec = aptr->codec;
	message(LOG_DEBUG, "plugin", "Found raw file codec %s", aptr->codec->c_name);
#if 0
	CPlayerMedia *mptr;
      
	mptr = new CPlayerMedia(psptr);

	COurInByteStreamFile *fbyte;
	fbyte = new COurInByteStreamFile(aptr->codec,
					 cifptr,
					 maxtime);
	mptr->create(fbyte, FALSE);
	mptr->set_plugin_data(aptr->codec, cifptr, NULL, get_audio_vft());

	for (int ix = 0; ix < 4; ix++) 
	  if (desc[ix] != NULL) 
	    psptr->set_session_desc(ix, desc[ix]);

	if (maxtime != 0.0) {
	  psptr->session_set_seekable(1);
	}

	return 0;
#endif
	return cifptr;
      }
    }
					   
    aptr = aptr->next_plugin;
  }
  return NULL;
}

/*
 * close_plugins
 * closes and frees plugin lists
 */
void close_plugins (void) 
{
  plugin_list_t *p;

  while (plugins != NULL) {
    p = plugins;
    close_library(p);
    plugins = plugins->next_plugin;
    free(p);
  }
}

/* end file codec_plugin.cpp */
