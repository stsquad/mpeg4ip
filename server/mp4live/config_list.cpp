
#include "config_list.h"
#include <sys/types.h>
#include <dlfcn.h>
#include <dirent.h>

CConfigList::CConfigList (const char *directory,
			  const char *config_type)
{
  m_directory = strdup(directory);
  m_config_type = config_type;
  m_config_list = NULL;
  m_config_count = 0;
}

CConfigList::~CConfigList (void)
{
  CConfigEntry *e;

  while (m_config_list != NULL) {
    e = m_config_list->GetNext();
    delete m_config_list;
    m_config_list = e;
  };
  CHECK_AND_FREE(m_directory);
}

bool CConfigList::Load (void)
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
    struct stat buf;
    if (stat(fname, &buf) >= 0 &&
	S_ISREG(buf.st_mode)) {
      debug_message("trying %s", fname);
      m_config_list = CreateConfigInt(fname, m_config_list);
      m_config_list->Initialize();
      if (m_config_list->IsValid()) {
	m_config_count++;
	debug_message("loaded %s config %s", 
		      m_config_type, 
		      m_config_list->GetName());
	m_config_list->Update();
      }
    }
  }
  closedir(dptr);
  return true;
}

CConfigEntry *CConfigList::FindConfigInt (const char *name)
{
  CConfigEntry *p = m_config_list;
  if (name == NULL) return NULL;
  while (p != NULL) {
    const char *val = p->GetName();
    if (val != NULL && strcmp(name, p->GetName()) == 0 &&
	p->IsValid()) {
      return p;
    }
    p = p->GetNext();
  }
  return NULL;
}

void CConfigList::CreateFileName (const char *proname,
				  char *fpathandname)
{
  char *file_name = strdup(proname);
  char *conv = file_name;
  while (*conv != '\0') {
    if (!isalnum(*conv) && *conv != '.') {
      *conv = '_';
    }
    conv++;
  }

  snprintf(fpathandname, PATH_MAX, "%s/%s", m_directory, file_name);
  struct stat buf;
  while (stat(fpathandname, &buf) == 0) {
    error_message("file %s exists", fpathandname);
    char temp[2];
    temp[0] = (random() % 26) + 'a';
    temp[1] = '\0';
    strcat(fpathandname, temp);
  }
  free(file_name);
}

bool CConfigList::CreateConfig (const char *proname)
{
  char fpathandname[PATH_MAX];
  if (FindConfigInt(proname) != NULL) {
    error_message("Config %s already exists", proname);
    return false;
  }
  CreateFileName(proname, fpathandname);
  m_config_list = CreateConfigInt(fpathandname, m_config_list);
  m_config_list->Initialize(false);
  if (m_config_list->SetConfigName(proname) == false) {
    error_message("can't set config name %s", proname);
    return false;
  }
  if (m_config_list->IsValid()) {
    m_config_count++;
    m_config_list->Update();
  }
  return true;
}

void CConfigList::AddEntryToList(CConfigEntry *e) 
{
  char fname[PATH_MAX];

  CreateFileName(e->GetName(), fname);
  e->SetFileName(fname);
  e->SetNext(m_config_list);
  m_config_list = e;
  m_config_count++;
}

void CConfigList::RemoveEntryFromList (CConfigEntry *e)
{
  CConfigEntry *p, *q;

  if (m_config_list == e) {
    m_config_list = e->GetNext();
    m_config_count--;
  } else {
    q = m_config_list;
    p = q->GetNext();
    while (p != e && p != NULL) {
      q = p;
      p = p->GetNext();
    }
    if (p != NULL) {
      q->SetNext(p->GetNext());
      m_config_count--;
    }
  }
}

