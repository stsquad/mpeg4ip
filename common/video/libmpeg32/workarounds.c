// GCC 3.0 workarounds


#include "workarounds.h"


int64_t mpeg3io_tell_gcc(mpeg3_fs_t *fs)
{
 	return fs->current_byte;
}

double mpeg3_add_double_gcc(double x, double y)
{
	return x + y;
}

double mpeg3_divide_double_gcc(double x, double y)
{
	return x / y;
}

int64_t mpeg3_total_bytes_gcc(mpeg3_title_t *title)
{
	return title->total_bytes;
}
