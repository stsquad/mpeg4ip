#define DECLARE_CONFIG_VARIABLES 1
#include "profile_video.h"

void CVideoProfile::LoadConfigVariables (CProfileEntry *e)
{
  e->AddVariables(VideoProfileConfigVariables, 
		  NUM_ELEMENTS_IN_ARRAY(VideoProfileConfigVariables));
  // eventually will add interface to read each encoder's variables
}
