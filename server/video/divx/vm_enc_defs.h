
#ifndef _VM_ENC_DEFS_H_
#define _VM_ENC_DEFS_H_

#include "vm_common_defs.h"

#define I_VOP		0		/* vop coding modes */
#define P_VOP		1
#define B_VOP		2
#define SPRITE_VOP	3	

#define VOP_VISIBLE 		1		

#define SEP_MOT_TEXT	1

#define UNRESTRICTED_MV_RANGE	16 	/* unrestricted MVs can poin
					outside VOP by this number of
 					pixels */

#define QUANT_TYPE_SEL		    0	    /* = 1 for MPEG quantization */
#define LOAD_INTRA_QUANT_MAT  	    1       /* Added, 12-JUN-1996, MW */
#define LOAD_NONINTRA_QUANT_MAT     1    

#define DEFAULT_COMP_ORDER 		TRUE	/* Vop Layer Defaults */
#define DEFAULT_VISIBILITY 		1
#define DEFAULT_SCALING 		0
#define DEFAULT_PRED_TYPE 		P_VOP
#define DEFAULT_DBQUANT			0	/* Not used */

#define BUFFER_EMPTY 			0	/* Initial state of buffer */


#define NON_ERROR_RESILIENT             1     /* non error resilient mode: 26.04.97 LDS */

#endif /* _VM_ENC_DEFS_H_ */
