
#ifndef __PROFILE_LIST_H__
#define __PROFILE_LIST_H__ 1
#include "mpeg4ip_config_set.h"
#include "util.h"

class CConfigEntry : public CConfigSet {
 public:
  CConfigEntry(const char *filename, const char *config_type,
		CConfigEntry *next) : CConfigSet(NULL, 0, filename) {
    m_filename = filename == NULL ? NULL : strdup(filename);
    m_config_type = config_type;
    m_initialized = false;
    m_next_config = next;
    m_valid = false;
  };
  virtual ~CConfigEntry(void) {
    WriteToFile();
    CHECK_AND_FREE(m_filename);
  };

  virtual void Initialize (bool check_config_name = true) {
    if (m_initialized) return;
    m_initialized = true;
    InitializeIndexes();
    ReadFile();
    m_configNameIndex = FindIndexByName("name");
    if (m_configNameIndex == UINT32_MAX) {
      error_message("No \"name\" variable found in configuration file type %s %s",
		    m_config_type, GetFileName());
      return;
    }
    const char *pn;
    if (check_config_name &&
	((pn = GetStringValue(m_configNameIndex)) == NULL ||
	 *pn == '\0')) {
      error_message("Config name for config type %s in %s is blank", 
		    m_config_type, m_filename);
      return;
    }
    m_valid = true;
  };

  bool IsValid(void) { return m_valid; } ;
  void Write(void) {
    WriteToFile();
  };

  const char *GetName(void) {
    return GetStringValue(m_configNameIndex);
  };
  // if the first thing they do is set the config name, we have 
  // a new config...  Don't check the config name
  bool SetConfigName(const char *name) {
    SetStringValue(m_configNameIndex, name);
    return true;
  };

  CConfigEntry *GetNext(void) { return m_next_config; };
  void SetNext(CConfigEntry *next) {
    m_next_config = next;
  };
  const char *GetFileName(void) { return m_filename; };
  virtual void LoadConfigVariables(void) = 0;
  virtual void Update(void) { };
 protected:
  bool m_initialized, m_valid;
  config_index_t m_configNameIndex;
  const char *m_filename;
  const char *m_config_type;
  CConfigEntry *m_next_config;
};


class CConfigList {
 public:
  CConfigList(const char *directory,
	       const char *config_type);
  virtual ~CConfigList(void);

  bool Load(void);
  uint32_t GetCount(void) { return m_config_count; };
  bool CreateConfig(const char *name);
  CConfigEntry *GetHead(void) { return m_config_list; } ;
  void AddEntryToList(CConfigEntry *e);
  void RemoveEntryFromList(CConfigEntry *e);
 protected:
  CConfigEntry *FindConfigInt(const char *name);
  const char *m_config_type;
  virtual CConfigEntry *CreateConfigInt(const char *fname, 
					CConfigEntry *next) = 0;
  CConfigEntry *m_config_list;
 private:
  void CreateFileName(const char *name, char *fpathandname);
  const char *m_directory;
  uint32_t m_config_count;
};

#endif
