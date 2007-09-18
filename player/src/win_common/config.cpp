#include <mpeg4ip_config_set.h>
#undef read

#include <atlbase.h>
//#include <atlimpl.cpp>

int CConfigSet::ReadVariablesFromRegistry (const char *reg_name, 
					   const char *config_section)
{
	LONG result;
	char buff[1024];
	DWORD buflen;

	CRegKey newrk;

	snprintf(buff, sizeof(buff), "%s\\\\%s", reg_name, config_section);
	result = newrk.Open(HKEY_CURRENT_USER, buff);
	if (result != ERROR_SUCCESS) return -1;

	for (config_index_t ix = 0; ix < m_numVariables; ix++) {
	  switch (m_variables[ix].m_type) {
	  case CONFIG_TYPE_INTEGER:
	  case CONFIG_TYPE_BOOL:
	    DWORD temp;
	    result = newrk.QueryValue(temp, m_variables[ix].m_sName);
	    if (result == ERROR_SUCCESS) {
	      if (m_variables[ix].m_type == CONFIG_TYPE_BOOL) {
		m_variables[ix].m_value.m_bvalue = (temp != 0);
	      } else {
		m_variables[ix].m_value.m_ivalue = temp;
	      }
	    }
	    break;
	  case CONFIG_TYPE_STRING:
	    buflen = sizeof(buff);
	    result = newrk.QueryValue(buff, m_variables[ix].m_sName,
				      &buflen);
	    if (result == ERROR_SUCCESS && *buff != 0) {
		  if (buff[0] == '\"' &&  buff[1] == '\"') break;
	      this->SetStringValue(ix, buff);
	    }
	    break;

	  case CONFIG_TYPE_FLOAT:
	    buflen = sizeof(buff) - 1;
	    buff[0] = ' ';
	    result = newrk.QueryValue(buff + 1, m_variables[ix].m_sName,
				      &buflen);
	    if (result == ERROR_SUCCESS) {
	      FromAscii(&m_variables[ix], buff);
	    }
	    break;
	  }
	}
	return 0;
}

void CConfigSet::WriteVariablesToRegistry (const char *reg_name, const char *config_section)
{
	LONG result;
	char buff[1024];
	CRegKey newrk;

	snprintf(buff, sizeof(buff), "%s\\\\%s", reg_name, config_section);
	result = newrk.Create(HKEY_CURRENT_USER, buff);
	if (result != ERROR_SUCCESS) return;

	for (config_index_t ix = 0; ix < m_numVariables; ix++) {
	  switch (m_variables[ix].m_type) {
	  case CONFIG_TYPE_INTEGER:
	    newrk.SetValue(m_variables[ix].m_value.m_ivalue,
			   m_variables[ix].m_sName);
	    break;
	  case CONFIG_TYPE_BOOL:
	    newrk.SetValue(m_variables[ix].m_value.m_bvalue ? 1 : 0,
			   m_variables[ix].m_sName);
	    break;
	  case CONFIG_TYPE_STRING:
	    newrk.SetValue(ToAscii(&m_variables[ix]),
			   m_variables[ix].m_sName);
	    break;
	  case CONFIG_TYPE_FLOAT:
	    
	    newrk.SetValue(ToAscii(&m_variables[ix]), 
			   m_variables[ix].m_sName);
	    break;
			   
	  }
	}
	newrk.Close();

}

