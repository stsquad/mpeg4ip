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
 * mp3_file.cpp - create media structure for mp3 files
 */

#include "player_session.h"
#include "player_media.h"
#include "our_bytestream_file.h"
#include "mp3.h"
#include "mp3_file.h"
#include "player_util.h"

static unsigned char c_read_byte (void *userdata)
{
  COurInByteStreamFile *fd = (COurInByteStreamFile *)userdata;
  return (fd->get());
}

static uint32_t c_read_bytes (unsigned char *b, uint32_t bytes, void *userdata)
{
  COurInByteStreamFile *fd = (COurInByteStreamFile *)userdata;
  return (fd->read(b, bytes));
}


// from mplib-0.6
#define GLL 148
const static char *genre_list[GLL] = 
{ "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk", "Grunge", "Hip-Hop", "Jazz",
  "Metal", "New Age", "Oldies", "Other", "Pop", "R&B", "Rap", "Reggae", "Rock", "Techno",
  "Industrial", "Alternative", "Ska", "Death Metal", "Pranks", "Soundtrack", "Euro-Techno",
  "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk", "Fusion", "Trance", "Classical", "Instrumental",
  "Acid", "House", "Game", "Sound Clip", "Gospel", "Noise", "Alternative Rock", "Bass", "Soul", "Punk",
  "Space", "Meditative", "Instrumental Pop", "Instrumental Rock", "Ethnic", "Gothic", "Darkwave",
  "Techno-Industrial", "Electronic", "Pop-Folk", "Eurodance", "Dream", "Southern Rock", "Comedy", 
  "Cult", "Gangsta Rap", "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
  "New Wave", "Psychedelic", "Rave", "Showtunes", "Trailer", "Lo-Fi", "Tribal", "Acid Punk",
  "Acid Jazz", "Polka", "Retro", "Musical", "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", 
  "National Folk", "Swing", "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass",
  "Avantgarde", "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
  "Big Band", "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson", "Opera",
  "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus", "Porn Groove", "Satire", "Slow Jam",
  "Club", "Tango", "Samba", "Folklore", "Ballad", "Power Ballad", "Rythmic Soul", "Freestyle",
  "Duet", "Punk Rock", "Drum Solo", "A Cappella", "Euro-House", "Dance Hall", "Goa", "Drum & Bass",
  "Club-House", "Hardcore", "Terror", "Indie", "BritPop", "Negerpunk", "Polsk Punk", "Beat",
  "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover", "Contemporary Christian",
  "Christian Rock", "Merengue", "Salsa", "Trash Metal", "Anime", "JPop", "Synthpop" };


static void read_mp3_file_for_tag (CPlayerSession *psptr,
				   const char *name)
{
  FILE *ifile;
  char buffer[128];
  char desc[80];
  char temp;
  int ix;

  ifile = fopen(name, "r");
  if (ifile == NULL) return;
  if (fseek(ifile, -128, SEEK_END) != 0) {
    return;
  }
  fread(buffer, 1, 128, ifile);
  if (strncasecmp(buffer, "tag", 3) != 0) {
    return;
  }
  temp = buffer[33];
  buffer[33] = '\0';
  ix = 32;
  while (isspace(buffer[ix]) && ix > 0) {
    buffer[ix] = '\0';
    ix--;
  }
  snprintf(desc, sizeof(desc), "%s", &buffer[3]);
  psptr->set_session_desc(0, desc);
  buffer[33] = temp;

  temp = buffer[63];
  buffer[63] = '\0';
  ix = 62;
  while (isspace(buffer[ix]) && ix > 33) {
    buffer[ix] = '\0';
    ix--;
  }
  snprintf(desc, sizeof(desc), "By: %s", &buffer[33]);
  psptr->set_session_desc(1, desc);
  buffer[63] = temp;

  temp = buffer[93];
  buffer[93] = '\0';
  ix = 92;
  while (isspace(buffer[ix]) && ix > 63) {
    buffer[ix] = '\0';
    ix--;
  }
  if (buffer[125] == '\0' && buffer[126] != '\0') {
    snprintf(desc, sizeof(desc), "On: %s - track %d (%c%c%c%c)",
	     &buffer[63], buffer[126], temp, buffer[94], buffer[95],
	     buffer[96]);
  } else {
    snprintf(desc, sizeof(desc), "On: %s (%c%c%c%c)",
	     &buffer[63], temp, buffer[94], buffer[95],
	     buffer[96]);
  }
  psptr->set_session_desc(2, desc);
  unsigned char index = (unsigned char)buffer[127];
  if (index <= GLL) {
    snprintf(desc, sizeof(desc), "Genre: %s", genre_list[index]);
    psptr->set_session_desc(3, desc);
  }
}
int create_media_for_mp3_file (CPlayerSession *psptr, 
			       const char *name,
			       const char **errmsg)
{
  CPlayerMedia *mptr;
  COurInByteStreamFile *fbyte;
  int freq = 0, samplesperframe;
  int ret;

  psptr->session_set_seekable(1);
  mptr = new CPlayerMedia;
  if (mptr == NULL) {
    *errmsg = "Couldn't create media";
    return (-1);
  }

  fbyte = new COurInByteStreamFile(name);

  MPEGaudio *mp3 = new MPEGaudio(c_read_byte, c_read_bytes, fbyte);
  try {
    while (mp3->loadheader() == FALSE);
  } catch (int err) {
    *errmsg = "Couldn't read MP3 header";
    delete mp3;
    delete mptr;
    return (-1);
  }

  freq = mp3->getfrequency();
  samplesperframe = 32;
  if (mp3->getlayer() == 3) {
    samplesperframe *= 18;
    if (mp3->getversion() == 0) {
      samplesperframe *= 2;
    }
  } else {
    samplesperframe *= SCALEBLOCK;
    if (mp3->getlayer() == 2) {
      samplesperframe *= 3;
    }
  }
  // Get the number of frames in the file.
  int framecount = 1;
  int havelast = 0;
  while (havelast == 0) {
    try {
      if (mp3->loadheader() != FALSE) {
	framecount++;
      } else {
	havelast = 1;
      }
    } catch (int err) {
      havelast = 1;
    } catch (...) {
      havelast = 1;
    }
  }
  delete mp3;

  mp3_message(LOG_INFO, "freq %d samples %d fps %d", freq, samplesperframe, 
	      freq / samplesperframe);
  double maxtime;
  maxtime = (double) samplesperframe * (double)framecount;
  maxtime /= (double)freq;
  mp3_message(LOG_INFO, "max playtime %g", maxtime);
  fbyte->config_for_file(freq / samplesperframe, maxtime);

  *errmsg = "Can't create thread";
  ret =  mptr->create_from_file(psptr, fbyte, FALSE);
  if (ret != 0) {
    return (-1);
  }

  audio_info_t *audio = (audio_info_t *)malloc(sizeof(audio_info_t));
  audio->freq = freq;
  mptr->set_audio_info(audio);
  mptr->set_codec_type("mp3 ");
  read_mp3_file_for_tag(psptr, name);
  return (0);
}

/* end file mp3_file.cpp */


