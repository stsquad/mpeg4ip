
#ifndef _MOM_IMAGE_H_
#define _MOM_IMAGE_H_

#   ifdef __cplusplus
    extern "C" {
#   endif /* __cplusplus */

#include "momusys.h"

Image	*AllocImage(UInt size_x, UInt size_y, ImageType type);
Void	FreeImage(Image *image);
Void	CopyImage(Image *image_in, Image *image_out);
Void	SetConstantImage(Image *image, Float val);
Void	SubImage(Image *image_in1, Image *image_in2, Image *image_out);

Vop * 	SallocVop (void);
Vop * 	AllocVop (UInt x, UInt y);
Void  	SfreeVop (Vop *vop);
Void  	FreeVop (Vop *vop);
Void  	CopyVopNonImageField (Vop *in, Vop *out);

#   ifdef __cplusplus
    }
#   endif /* __cplusplus  */ 

#endif


#ifndef _TUH_TOOLBOX_H
#define _TUH_TOOLBOX_H

   #   ifdef __cplusplus
       extern "C" {
   #   endif /* __cplusplus */

#define MAX_UCHAR_VAL	255
#define MAX_SHORT_VAL	32767
#define MAX_FLOAT_VAL	2000000000

#ifndef SIGN0
#define SIGN0(a)    	( ((a)<0) ? -1 : (((a)>0) ? 1  : 0) )
#endif

#ifndef SIGN
#define SIGN(a)         ((a)<0 ? (-1) : (1))
#endif

#ifndef ROUND
#define ROUND(x)        ( (Int) ((x) + SIGN0(x)*0.5) )
#endif

#include "mom_structs.h"


   #   ifdef __cplusplus
       }
   #   endif /* __cplusplus  */ 

#endif  /* ifndef _TUH_TOOLBOX_H */
