/**************************************************************************

 *                                                                        *

 * This code is developed by Adam Li.  This software is an                *

 * implementation of a part of one or more MPEG-4 Video tools as          *

 * specified in ISO/IEC 14496-2 standard.  Those intending to use this    *

 * software module in hardware or software products are advised that its  *

 * use may infringe existing patents or copyrights, and any such use      *

 * would be at such party's own risk.  The original developer of this     *

 * software module and his/her company, and subsequent editors and their  *

 * companies (including Project Mayo), will have no liability for use of  *

 * this software or modifications or derivatives thereof.                 *

 *                                                                        *

 * Project Mayo gives users of the Codec a license to this software       *

 * module or modifications thereof for use in hardware or software        *

 * products claiming conformance to the MPEG-4 Video Standard as          *

 * described in the Open DivX license.                                    *

 *                                                                        *

 * The complete Open DivX license can be found at                         *

 * http://www.projectmayo.com/opendivx/license.php .                      *

 *                                                                        *

 **************************************************************************/



/**************************************************************************

 *

 *  putvlc.c

 *

 *  Copyright (C) 2001  Project Mayo

 *

 *  Adam Li

 *

 *  DivX Advance Research Center <darc@projectmayo.com>

 *

 **************************************************************************/

#ifndef _ENCORE_RATE_CTL_H

#define _ENCORE_RATE_CTL_H

#include "encoder.h"



void RateCtlInit(RateCtlParam * pCtl, double quant, double target_rate,

		 long rc_period, long rc_reaction_period,



		 long rc_reaction_ratio);

int RateCtlGetQ(RateCtlParam * pCtl, double MAD);

void RateCtlUpdate(RateCtlParam * pCtl, int current_frame);



#endif /*

          _RATE_CTL_H  

        */

