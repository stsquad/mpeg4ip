#include "mpeg4ip.h"
#include "playlist.h"

CPlaylist::CPlaylist (const char *filename, const char **errmsg)
{
  #ifndef _WIN32
  struct stat statbuf;
  if (stat(filename, &statbuf) != 0) {
    *errmsg = "File not found";
    return;
  }
  if (!S_ISREG(statbuf.st_mode)) {
    *errmsg = "Not a file";
    return;
  }
#else
  OFSTRUCT ReOpenBuff;
  if (OpenFile(filename, &ReOpenBuff,OF_READ) == HFILE_ERROR) {
    *errmsg = "File not found";
    return;
  }

#endif
  FILE *ifile;

  ifile = fopen(filename, "r");
  if (ifile == NULL) {
    *errmsg = "Can't open file";
    return;
  }
  char buffer[1024];
  CPlaylistEntry *prev = NULL;
  while (fgets(buffer, sizeof(buffer), ifile) != NULL) {
    char *temp;
    temp = strchr(buffer, '\n');
    if (temp) {
        if (*(temp - 1) == '\r')
            *(temp - 1) = '\0';
        *temp = '\0';
    }
    CPlaylistEntry *newone;
    newone = new CPlaylistEntry(buffer);
    if (prev == NULL) {
      m_first = m_on = prev = newone;
    } else {
      prev->set_next(newone);
      newone->set_prev(prev);
      prev = newone;
    }
  }
  fclose(ifile);
}

CPlaylist::~CPlaylist (void) 
{
  CPlaylistEntry *p;
  p = m_first;
  while (p != NULL) {
    m_first = m_first->get_next();
    delete p;
    p = m_first;
  }
}

const char *CPlaylist::get_next (void)
{
  if (m_on == NULL) {
    return (NULL);
  }

  m_on = m_on->get_next();
  if (m_on == NULL) {
    return (NULL);
  }
  return (m_on->get_value());
}

const char *CPlaylist::get_prev (void)
{
  if (m_on == NULL) {
    return NULL;
  }

  m_on = m_on->get_prev();
  if (m_on == NULL) {
    return (NULL);
  }
  return (m_on->get_value());
}

const char *CPlaylist::get_first (void) 
{
  m_on = m_first;
  if (m_on == NULL) 
    return (NULL);
  
  return m_on->get_value();
}

const char *CPlaylist::get_last (void)
{
  CPlaylistEntry *p;

  p = m_first;
  if (p == NULL) 
    return (NULL);
  while (p->get_next() != NULL) {
    p = p->get_next();
  }

  return p->get_value();
}
