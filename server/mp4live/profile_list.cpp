
#include "profile_list.h"
#include <sys/types.h>
#include <dlfcn.h>
#include <dirent.h>

CProfileList::CProfileList (const char *directory,
			    const char *profile_type)
{
  m_directory = strdup(directory);
  m_profile_type = profile_type;
  m_profile_list = NULL;
}

CProfileList::~CProfileList (void)
{
  CProfileEntry *e;

  while (m_profile_list != NULL) {
    e = m_profile_list->GetNextProfile();
    delete m_profile_list;
    m_profile_list = e;
  };
}

bool CProfileList::Load (void)
{
  DIR *dptr;
  struct dirent *fptr;

  dptr = opendir(m_directory);
  if (dptr == NULL) {
    error_message("Can't open directory %s", m_directory);
    return false;
  }
  
  while ((fptr = readdir(dptr)) != NULL) {
    char fname[PATH_MAX];
    sprintf(fname, "%s/%s", m_directory, fptr->d_name);
    m_profile_list = new CProfileEntry(fname, m_profile_type, m_profile_list);
    LoadConfigVariables(m_profile_list);
  }
  closedir(dptr);
  return true;
}

CConfigSet *CProfileList::FindProfile (const char *name)
{
  CProfileEntry *p = m_profile_list;

  while (p != NULL) {
    if (strcmp(name, p->GetProfileName()) == 0) {
      return p->GetConfiguration();
    }
    p = p->GetNextProfile();
  }
  return NULL;
}
