#define DECLARE_CONFIG_VARIABLES 1
#include "profile_audio.h"

void CAudioProfile::LoadConfigVariables (CProfileEntry *e)
{
  e->AddVariables(AudioProfileConfigVariables, 
		  NUM_ELEMENTS_IN_ARRAY(AudioProfileConfigVariables));
  // eventually will add interface to read each encoder's variables
}
