#include "mpeg4ip.h"
#include "mpeg4ip_getopt.h"
#include "mpeg4ip_config_set.h"


struct option *create_long_opts_from_config (CConfigSet *pConfig,
					     struct option *orig_opts,
					     uint32_t orig_opts_size,
					     uint32_t base_offset)
{
  struct option *long_options;
  config_index_t max_vars, ix;


  max_vars = pConfig->GetNumVariables();
  uint32_t opts_size = (max_vars + orig_opts_size + 1) * sizeof(*long_options);

  long_options = (struct option *)malloc(opts_size);
  if (long_options == NULL) {
    return NULL;
  }
  memset(long_options, 0, opts_size);
  
  for (ix = 0; ix < max_vars; ix++) {
    long_options[ix].name = pConfig->GetNameFromIndex(ix);
    long_options[ix].has_arg = pConfig->GetTypeFromIndex(ix) == CONFIG_TYPE_BOOL ? 2 : 1;
    long_options[ix].val = base_offset + ix;
  }
  for (uint32_t jx = 0; jx < orig_opts_size; jx++) {
    long_options[ix + jx].name = orig_opts[jx].name;
    long_options[ix + jx].val = orig_opts[jx].val;
    long_options[ix + jx].has_arg = orig_opts[jx].has_arg;
  }

  return long_options;
}
