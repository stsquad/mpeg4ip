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
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 */

#ifndef __CONFIG_SET_H__
#define __CONFIG_SET_H__

#include <systems.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef CONFIG_SAFETY
#define CONFIG_SAFETY 1
#endif

typedef u_int32_t config_integer_t;

typedef u_int16_t config_index_t;


enum ConfigException {
	CONFIG_ERR_INAME,
	CONFIG_ERR_TYPE,
	CONFIG_ERR_MEMORY,
};

// TBD type specific exception info and printing utility
class CConfigException {
public:
	CConfigException(ConfigException e) {
		type = e;
	}
	ConfigException	type;
};

#define CONFIG_MAX_STRLEN	255

// TBD weld this in, and throw exception
inline char* stralloc(const char* src) {
	char* dst = (char*)malloc(strlen(src)+1);
	if (dst) {
		strcpy(dst, src);
	}
	return dst;
}

enum ConfigType {
	CONFIG_TYPE_UNDEFINED,
	CONFIG_TYPE_INTEGER,
	CONFIG_TYPE_BOOL,
	CONFIG_TYPE_STRING,
	CONFIG_TYPE_FLOAT
};

union UConfigValue {
	UConfigValue(void) {
		m_svalue = NULL;
	}
	UConfigValue(config_integer_t ivalue) {
		m_ivalue = ivalue;
	}
	UConfigValue(bool bvalue) {
		m_bvalue = bvalue;
	}
	UConfigValue(char* svalue) {
		m_svalue = svalue;
	}
	UConfigValue(float fvalue) {
		m_fvalue = fvalue;
	}

	config_integer_t	m_ivalue;
	bool				m_bvalue;
	char*				m_svalue;
	float				m_fvalue;
};

struct SConfigVariable {
	const config_index_t	m_iName;
	const char* 			m_sName;
	ConfigType				m_type;
	UConfigValue			m_defaultValue;
	UConfigValue			m_value;

	const char* ToAscii() {
		static char sBuf[CONFIG_MAX_STRLEN+3];
		switch (m_type) {
		case CONFIG_TYPE_INTEGER:
			sprintf(sBuf, "%d", m_value.m_ivalue);
			return sBuf;
		case CONFIG_TYPE_BOOL:
			sprintf(sBuf, "%d", m_value.m_bvalue);
			return sBuf;
		case CONFIG_TYPE_STRING:
			if (strchr(m_value.m_svalue, ' ')) {
				sBuf[0] = '"';
				strncpy(&sBuf[1], m_value.m_svalue, CONFIG_MAX_STRLEN);
				strcpy(&sBuf[
					MIN(strlen(m_value.m_svalue), CONFIG_MAX_STRLEN)+1], "\"");
			}
			return m_value.m_svalue;
		case CONFIG_TYPE_FLOAT:
			sprintf(sBuf, "%f", m_value.m_fvalue);
			return sBuf;
		default:
			return "";
		}
	}

	bool FromAscii(const char* s) {
		switch (m_type) {
		case CONFIG_TYPE_INTEGER:
			return (sscanf(s, " %i ", &m_value.m_ivalue) == 1);
		case CONFIG_TYPE_BOOL:
			// OPTION could add "yes/no", "true/false"
			if (sscanf(s, " %u ", &m_value.m_ivalue) != 1) {
				return false;
			}
			m_value.m_bvalue = m_value.m_ivalue ? true : false;
			return true;
		case CONFIG_TYPE_STRING:
			// N.B. assuming m_svalue has been alloc'ed
			free(m_value.m_svalue);
			m_value.m_svalue = (char*)malloc(strlen(s)+1);
			if (m_value.m_svalue == NULL) {
				throw new CConfigException(CONFIG_ERR_MEMORY);
			}
			m_value.m_svalue[0] = '\0';
			if (s[0] == '"') {
				char* end = strchr(&s[1], '"');
				if (end == NULL) {
					return false;
				}
				size_t len = end - &s[1];
				strncpy(m_value.m_svalue, &s[1], len);
				m_value.m_svalue[len] = '\0';
			}
			return (sscanf(s, "%s", m_value.m_svalue) == 1);
		case CONFIG_TYPE_FLOAT:
			return (sscanf(s, " %f ", &m_value.m_fvalue) == 1);
		default:
			return false;
		}
	}

	void SetToDefault(void) {
		switch (m_type) {
		case CONFIG_TYPE_INTEGER:
			m_value.m_ivalue = m_defaultValue.m_ivalue;
			break;
		case CONFIG_TYPE_BOOL:
			m_value.m_bvalue = m_defaultValue.m_bvalue;
			break;
		case CONFIG_TYPE_STRING:
			free(m_value.m_svalue);
			m_value.m_svalue = stralloc(m_defaultValue.m_svalue);
			if (m_value.m_svalue == NULL) {
				throw new CConfigException(CONFIG_ERR_MEMORY);
			}
			break;
		case CONFIG_TYPE_FLOAT:
			m_value.m_fvalue = m_defaultValue.m_fvalue;
			break;
		default:
			break;
		} 
	}

	bool IsValueDefault(void) {
		switch (m_type) {
		case CONFIG_TYPE_INTEGER:
			return m_value.m_ivalue == m_defaultValue.m_ivalue;
		case CONFIG_TYPE_BOOL:
			return m_value.m_bvalue == m_defaultValue.m_bvalue;
		case CONFIG_TYPE_STRING:
			return (strcmp(m_value.m_svalue, m_defaultValue.m_svalue) == 0);
		case CONFIG_TYPE_FLOAT:
			return m_value.m_fvalue == m_defaultValue.m_fvalue;
		default:
			return false;
		} 
	}
};


class CConfigSet {
public:
	CConfigSet(SConfigVariable* variables, 
	  config_index_t numVariables, 
	  const char* defaultFileName) {
		m_debug = false;
		m_variables = variables;
		m_numVariables = numVariables;
		m_defaultFileName = defaultFileName;
		SetToDefaults();
	};

	inline void CheckIName(config_index_t iName) {
		if (iName >= m_numVariables) {
			throw new CConfigException(CONFIG_ERR_INAME);
		}
		if (m_variables[iName].m_iName != iName) {
			throw new CConfigException(CONFIG_ERR_INAME);
		}
	}

	inline void CheckIntegerType(config_index_t iName) {
		if (m_variables[iName].m_type != CONFIG_TYPE_INTEGER) {
			throw new CConfigException(CONFIG_ERR_TYPE);
		}
	}

	inline void CheckBoolType(config_index_t iName) {
		if (m_variables[iName].m_type != CONFIG_TYPE_BOOL) {
			throw new CConfigException(CONFIG_ERR_TYPE);
		}
	}

	inline void CheckStringType(config_index_t iName) {
		if (m_variables[iName].m_type != CONFIG_TYPE_STRING) {
			throw new CConfigException(CONFIG_ERR_TYPE);
		}
	}

	inline void CheckFloatType(config_index_t iName) {
		if (m_variables[iName].m_type != CONFIG_TYPE_FLOAT) {
			throw new CConfigException(CONFIG_ERR_TYPE);
		}
	}

	inline config_integer_t GetIntegerValue(const config_index_t iName) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckIntegerType(iName);
#endif
		return m_variables[iName].m_value.m_ivalue;
	}

	inline void SetIntegerValue(const config_index_t iName, 
	  config_integer_t ivalue) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckIntegerType(iName);
#endif
		m_variables[iName].m_value.m_ivalue = ivalue;
	}

	inline bool GetBoolValue(const config_index_t iName) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckBoolType(iName);
#endif
		return m_variables[iName].m_value.m_bvalue;;
	}

	inline void SetBoolValue(const config_index_t iName, bool bvalue) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckBoolType(iName);
#endif
		m_variables[iName].m_value.m_bvalue = bvalue;
	}

	inline char* GetStringValue(const config_index_t iName) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckStringType(iName);
#endif
		return m_variables[iName].m_value.m_svalue;
	}

	inline void SetStringValue(const config_index_t iName, char* svalue) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckStringType(iName);
#endif
		free(m_variables[iName].m_value.m_svalue);
		m_variables[iName].m_value.m_svalue = stralloc(svalue);
		if (m_variables[iName].m_value.m_svalue == NULL) {
			throw new CConfigException(CONFIG_ERR_MEMORY);
		}
	}

	inline float GetFloatValue(const config_index_t iName) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckFloatType(iName);
#endif
		return m_variables[iName].m_value.m_fvalue;
	}

	inline void SetFloatValue(const config_index_t iName, float fvalue) {
#if CONFIG_SAFETY
		CheckIName(iName);
		CheckFloatType(iName);
#endif
		m_variables[iName].m_value.m_fvalue = fvalue;
	}

	void SetToDefaults(void) {
		for (config_index_t i = 0; i < m_numVariables; i++) {
			m_variables[i].SetToDefault();
		}
	}

	bool ReadFromFile(const char* fileName) {
		FILE* pFile = fopen(fileName, "r");
		if (pFile == NULL) {
			if (m_debug) {
				fprintf(stderr, "couldn't open file %s\n", fileName);
			}
			return false;
		}
		char line[256];
		while (fgets(line, sizeof(line), pFile)) {
			// comment
			if (line[0] == '#') {
				continue;
			}
			char* s = line;
			SConfigVariable* var = FindByName(strsep(&s, "="));
			if (var == NULL || s == NULL) {
				if (m_debug) {
					fprintf(stderr, "bad config line %s\n", s);  
				}
				continue;
			}
			if (!var->FromAscii(s)) {
				if (m_debug) {
					fprintf(stderr, "bad config value in line %s\n", s);  
				}
			}
		}
		fclose(pFile);
		return true;
	}

	bool WriteToFile(const char* fileName, bool allValues = false) {
		FILE* pFile = fopen(fileName, "w");
		if (pFile == NULL) {
			if (m_debug) {
				fprintf(stderr, "couldn't open file %s\n", fileName);
			}
			return false;
		}
		for (config_index_t i = 0; i < m_numVariables; i++) {
			SConfigVariable* var = &m_variables[i];
			if (allValues || !var->IsValueDefault()) {
				fprintf(pFile, "%s=%s\n", var->m_sName, var->ToAscii());
			}
		}
		fclose(pFile);
		return true;
	}

	bool ReadDefaultFile(void) {
		return ReadFromFile(m_defaultFileName);
	}
	bool WriteDefaultFile(void) {
		return WriteToFile(m_defaultFileName);
	}

	void SetDebug(bool debug = true) {
		m_debug = debug;
	}

protected:
	SConfigVariable* FindByName(const char* sName) {
		for (config_index_t i = 0; i < m_numVariables; i++) {
			if (!strcmp(sName, m_variables[i].m_sName)) {
				return &m_variables[i];
			}
		}
		return NULL;
	};

protected:
	SConfigVariable*	m_variables;
	config_index_t		m_numVariables;
	const char*			m_defaultFileName;
	bool 				m_debug;
};

#endif /* __CONFIG_SET_H__ */
