#include "../mpeg3private.inc"

#include <stdio.h>
#include <string.h>

int mpeg3_mmx_test()
{
	int result = 0;
	FILE *proc;
	char string[MPEG3_STRLEN];


#ifdef HAVE_MMX
	if(!(proc = fopen(MPEG3_PROC_CPUINFO, "r")))
	{
		fprintf(stderr, "mpeg3_mmx_test: failed to open /proc/cpuinfo\n");
		return 0;
	}
	
	while(!feof(proc))
	{
		fgets(string, MPEG3_STRLEN, proc);
/* Got the flags line */
		if(!strncasecmp(string, "flags", 5))
		{
			char *needle;
			needle = strstr(string, "mmx");
			if(!needle)
            {
            	fclose(proc);
            	return 0;
            }
			if(!strncasecmp(needle, "mmx", 3))
            {
            	fclose(proc);
            	return 1;
            }
		}
	}
   	fclose(proc);
#endif

	return 0;
}
