#ifndef _ENCORE_TIMER_H
#define _ENCORE_TIMER_H

#if defined(_PROFILING_)

#include "../portab.h"

uint64_t count_frames;

extern void start_timer();
extern void start_global_timer();
extern void stop_dct_timer();
extern void stop_idct_timer();
extern void stop_motion_timer();
extern void stop_comp_timer();
extern void stop_edges_timer();
extern void stop_inter_timer();
extern void stop_quant_timer();
extern void stop_iquant_timer();
extern void stop_conv_timer();
extern void stop_transfer_timer();
extern void stop_coding_timer();
extern void stop_prediction_timer();
extern void stop_interlacing_timer();
extern void stop_global_timer();
extern void init_timer();
extern void write_timer();

#else

static __inline void start_timer() {}
static __inline void start_global_timer() {}
static __inline void stop_dct_timer() {}
static __inline void stop_idct_timer() {}
static __inline void stop_motion_timer() {}
static __inline void stop_comp_timer() {}
static __inline void stop_edges_timer() {}
static __inline void stop_inter_timer() {}
static __inline void stop_quant_timer() {}
static __inline void stop_iquant_timer() {}
static __inline void stop_conv_timer() {}
static __inline void stop_transfer_timer() {}
static __inline void init_timer() {}
static __inline void write_timer() {}
static __inline void stop_coding_timer() {}
static __inline void stop_interlacing_timer() {}
static __inline void stop_prediction_timer() {}
static __inline void stop_global_timer() {}

#endif

#endif /* _TIMER_H_ */
