#ifndef __PLAYLIST_H__
#define __PLAYLIST_H__

#include <stdlib.h>
#include <string.h>

class CPlaylistEntry
{
 public:
  CPlaylistEntry(const char *name) {
    m_file_name = strdup(name);
    m_next = NULL;
    m_prev = NULL;
  };
  ~CPlaylistEntry(void) {
    free((void *)m_file_name);
  }
  const char *get_value (void) { return (m_file_name); };
  CPlaylistEntry *get_next (void) { return (m_next); };
  CPlaylistEntry *get_prev (void) { return (m_prev); };
  void set_next (CPlaylistEntry *next) { m_next = next; };
  void set_prev (CPlaylistEntry *prev) { m_prev = prev; };
 private:
  const char *m_file_name;
  CPlaylistEntry *m_next;
  CPlaylistEntry *m_prev;
};


class CPlaylist {
 public:
  CPlaylist(const char *file_name, const char **errmsg);
  ~CPlaylist(void);
  const char *get_next(void);
  const char *get_prev(void);
  const char *get_first(void);
  const char *get_last(void);
 private:
  CPlaylistEntry *m_first, *m_on;
};

#endif
