
#ifndef _RATE_CTL_H
#define _RATE_CTL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void RateCtlInit(double quant, double target_rate, 
	long rc_period, long rc_reaction_period, long rc_reaction_ratio);
int RateCtlGetQ(double MAD);
void RateCtlUpdate(int current_frame);

#ifdef __cplusplus
}
#endif /* __cplusplus  */
 
#endif /* _RATE_CTL_H  */ 
