#ifndef WORKAROUNDS_H
#define WORKAROUNDS_H

#include "mpeg3io.h"
#include "mpeg3title.h"

int64_t mpeg3io_tell_gcc(mpeg3_fs_t *fs);
double mpeg3_add_double_gcc(double x, double y);
double mpeg3_divide_double_gcc(double x, double y);
int64_t mpeg3_total_bytes_gcc(mpeg3_title_t *title);


#endif
