
#ifndef __PROFILE_LIST_H__
#define __PROFILE_LIST_H__ 1
#include "mpeg4ip_config_set.h"
#include "util.h"

class CProfileEntry {
 public:
  CProfileEntry(const char *filename, const char *profile_type,
		CProfileEntry *next) {
    m_filename = strdup(filename);
    m_profile_type = profile_type;
    m_initialized = false;
    m_pProfile = NULL;
    m_next_profile = next;
  };
  ~CProfileEntry(void);
  
  void AddVariables(const SConfigVariable *var_list,
		    const config_index_t num_vars) {
    if (m_pProfile == NULL) {
      m_pProfile = new CConfigSet(var_list, num_vars, m_filename);
    } else {
      m_pProfile->AddConfigVariables(var_list, num_vars);
    }
  };

  void Write(void) {
    Initialize();
    if (m_pProfile != NULL) {
      m_pProfile->WriteDefaultFile();
    };
  };

  const char *GetProfileName(void) {
    Initialize();
    return m_pProfile->GetStringValue(m_profileNameIndex);
  };

  CConfigSet *GetConfiguration(void) {
    Initialize();
    return m_pProfile;
  };

  CProfileEntry *GetNextProfile(void) { return m_next_profile; };
 protected:
  bool m_initialized;
  config_index_t m_profileNameIndex;
  CConfigSet *m_pProfile;
  const char *m_filename;
  const char *m_profile_type;
  CProfileEntry *m_next_profile;
  void Initialize (void) {
    if (m_initialized) return;
    if (m_pProfile == NULL) {
      error_message("Attempt to initialized profile %s, type %s without variables", 
		    m_filename, m_profile_type);
      return;
    }
    m_pProfile->InitializeIndexes();
    m_pProfile->ReadDefaultFile();
    m_profileNameIndex = m_pProfile->FindIndexByName("profileName");
    if (m_profileNameIndex == UINT32_MAX) {
      error_message("No \"profileName\" variable found in configuration variables %s",
		    m_profile_type);
      delete m_pProfile;
      m_pProfile = NULL;
      return;
    }
    const char *pn;
    if ((pn = m_pProfile->GetStringValue(m_profileNameIndex)) == NULL ||
	*pn == '\0') {
      error_message("Profile name for profile type %s in %s is blank", 
		    m_profile_type, m_filename);
      delete m_pProfile;
      m_pProfile = NULL;
      return;
    }
    m_initialized = true;
  };
};


class CProfileList {
 public:
  CProfileList(const char *directory,
	       const char *profile_type);
  virtual ~CProfileList(void);

  bool Load(void);
  CConfigSet *FindProfile(const char *name);

 protected:
  virtual void LoadConfigVariables(CProfileEntry *p) = 0;
  const char *m_profile_type;
 private:
  CProfileEntry *m_profile_list;
  const char *m_directory;
};

#endif
