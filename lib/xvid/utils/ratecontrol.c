#include "ratecontrol.h"

#include <math.h> /* fabs */

#define ABS(X) (X < 0) ? (-X) : (X)

typedef struct {
	int64_t size;
	int32_t count;
} QuantInfo;

typedef struct
{
	int32_t rtn_quant;
	int64_t frames;
	int64_t total_size;
	double framerate;
	int32_t target_rate;
	int16_t max_quant;
	int16_t min_quant;
	int64_t last_change;
	int64_t quant_sum;
	double quant_error[32];
	double avg_framesize;
	double target_framesize;
	int32_t reaction_delay_factor;
} RateControl;

RateControl rate_control;

int get_initial_quant(int bpp) {
	return 5;
}

void RateControlInit(uint32_t target_rate, uint32_t reaction_delay_factor, int framerate,
		     int max_quant, int min_quant)
{
	int i;

	rate_control.frames = 0;
	rate_control.total_size = 0;
	
	rate_control.framerate = framerate / 1000.0;
	rate_control.target_rate = target_rate;

	rate_control.rtn_quant = get_initial_quant(0);
	rate_control.max_quant = max_quant;
	rate_control.min_quant = min_quant;

	for (i=0 ; i<32 ; ++i)
	{
		rate_control.quant_error[i] = 0.0;
	}

	rate_control.target_framesize = target_rate / rate_control.framerate;
	rate_control.avg_framesize = target_rate / rate_control.framerate;

	rate_control.reaction_delay_factor = reaction_delay_factor;
}

int RateControlGetQ(int keyframe)
{
	return rate_control.rtn_quant;
}

void RateControlUpdate(int16_t quant, int frame_size, int keyframe)
{
	int64_t deviation;
	double overflow, cur_target;
	int32_t rtn_quant;

	rate_control.frames++;
	rate_control.total_size += frame_size;

	deviation = (int64_t) ((double) rate_control.total_size - 
			       ((double) ((double) rate_control.target_rate / 8.0 /
					 (double) rate_control.framerate) * (double) rate_control.frames));

	DEBUGCBR((int32_t)(rate_control.frames - 1), rate_control.rtn_quant, (int32_t)deviation);

	rate_control.avg_framesize -= rate_control.avg_framesize / (double)rate_control.reaction_delay_factor;
	rate_control.avg_framesize += frame_size * rate_control.rtn_quant * 0.5 / (double)rate_control.reaction_delay_factor;

	if (rate_control.target_framesize > rate_control.avg_framesize)
		cur_target = rate_control.avg_framesize;
	else
		cur_target = rate_control.target_framesize;

	deviation /= -rate_control.reaction_delay_factor;

	overflow = (double)deviation * rate_control.avg_framesize / rate_control.target_framesize;

	if (overflow > cur_target)
		overflow = cur_target;
	else if (overflow < cur_target * -0.935)
		overflow = cur_target * -0.935;

	rtn_quant = (int32_t)(rate_control.avg_framesize * 2.0 / (cur_target + overflow));

	if (rtn_quant < 31)
	{
		rate_control.quant_error[rtn_quant] += rate_control.avg_framesize * 2.0 / (cur_target + overflow) - rtn_quant;
		if (rate_control.quant_error[rtn_quant] >= 1.0)
		{
			rate_control.quant_error[rtn_quant] -= 1.0;
			rtn_quant++;
		}
	}

	if (rtn_quant > rate_control.rtn_quant + 1)
		rtn_quant = rate_control.rtn_quant + 1;
	else if (rtn_quant < rate_control.rtn_quant - 1)
		rtn_quant = rate_control.rtn_quant - 1;

	if (rtn_quant > rate_control.max_quant)
		rtn_quant = rate_control.max_quant;
	else if (rtn_quant < rate_control.min_quant)
		rtn_quant = rate_control.min_quant;

	rate_control.rtn_quant = rtn_quant;
}
