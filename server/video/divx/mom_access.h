

#ifndef _MOM_ACCESS_H_
#define _MOM_ACCESS_H_


#include "momusys.h"
//#include "mom_access.p"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Char *GetImageData(Image *image);
UInt GetImageSize(Image *image);
UInt GetImageSizeX(Image *image);
UInt GetImageSizeY(Image *image);
Int GetImageVersion(Image *image);
ImageType GetImageType(Image *image);


/* -- GetVop{xxx} -- Functions to access components of the Vop structure */
Int GetVopNot8Bit(Vop *vop);
Int GetVopQuantPrecision(Vop *vop);
Int GetVopBitsPerPixel(Vop *vop);
Int GetVopMidGrey(Vop *vop);
Int GetVopBrightWhite(Vop *vop);
Int GetVopTimeIncrementResolution(Vop *vop);
Int GetVopModTimeBase(Vop *vop);
Int GetVopTimeInc(Vop *vop);
Int GetVopPredictionType(Vop *vop);
Int GetVopIntraDCVlcThr(Vop *vop);
Int GetVopRoundingType(Vop *vop);
Int GetVopWidth(Vop *vop);
Int GetVopHeight(Vop *vop);
Int GetVopHorSpatRef(Vop *vop);
Int GetVopVerSpatRef(Vop *vop);
Int GetVopQuantizer(Vop *vop);
Int GetVopIntraQuantizer(Vop *vop);
Int GetVopIntraACDCPredDisable(Vop *vop);
Int GetVopFCodeFor(Vop *vop);
Int GetVopSearchRangeFor(Vop *vop);
Image *GetVopY(Vop *vop);
Image *GetVopU(Vop *vop);
Image *GetVopV(Vop *vop);

/* -- PutVop{xxx} -- Functions to write to components of the Vop structure */
Void PutVopQuantPrecision(Int quant_precision,Vop *vop);
Void PutVopBitsPerPixel(Int bits_per_pixel,Vop *vop);
Void PutVopTimeIncrementResolution(Int time_incre_res, Vop *vop);
Void PutVopModTimeBase(Int mod_time_base, Vop *vop);
Void PutVopTimeInc(Int time_inc, Vop *vop);
Void PutVopPredictionType(Int prediction_type, Vop *vop);
Void PutVopIntraDCVlcThr(Int intra_dc_vlc_thr,Vop *vop);
Void PutVopRoundingType(Int rounding_type, Vop *vop);
Void PutVopWidth(Int width, Vop *vop);
Void PutVopHeight(Int height, Vop *vop);
Void PutVopHorSpatRef(Int hor_spat_ref, Vop *vop);
Void PutVopVerSpatRef(Int ver_spat_ref, Vop *vop);
Void PutVopQuantizer(Int quantizer, Vop *vop);
Void PutVopIntraACDCPredDisable(Int intra_acdc_pred_disable, Vop *vop);
Void PutVopFCodeFor(Int fcode_for, Vop *vop);
Void PutVopSearchRangeFor(Int sr_for, Vop *vop);
Void PutVopY(Image *y_chan, Vop *vop);
Void PutVopU(Image *u_chan, Vop *vop);
Void PutVopV(Image *v_chan, Vop *vop);
Void PutVopIntraQuantizer(Int Q,Vop *vop);

/* VolConfig Put functions */
Void PutVolConfigFrameRate _P_((Float fr, VolConfig *cfg));
Void PutVolConfigM _P_((Int M, VolConfig *cfg));
Void PutVolConfigStartFrame _P_((Int frame, VolConfig *cfg));
Void PutVolConfigEndFrame _P_((Int frame, VolConfig *cfg));
Void PutVolConfigBitrate  _P_((Int bit_rate,VolConfig *cfg));
Void PutVolConfigIntraPeriod _P_((Int ir,VolConfig *cfg));
Void PutVolConfigQuantizer _P_((Int Q,VolConfig *cfg));
Void PutVolConfigIntraQuantizer _P_((Int Q,VolConfig *cfg));
Void PutVolConfigFrameSkip _P_((Int frame_skip,VolConfig *cfg));
Void PutVolConfigModTimeBase _P_((Int time,VolConfig *cfg));

/* VolConfig Get functions */
Float GetVolConfigFrameRate _P_((VolConfig *cfg));
Int GetVolConfigM _P_((VolConfig *cfg));
Int GetVolConfigStartFrame _P_((VolConfig *cfg));
Int GetVolConfigEndFrame _P_((VolConfig *cfg));
Int GetVolConfigBitrate _P_((VolConfig *cfg));
Int GetVolConfigIntraPeriod _P_((VolConfig *cfg));
Int GetVolConfigQuantizer _P_((VolConfig *cfg));
Int GetVolConfigIntraQuantizer _P_((VolConfig *cfg));
Int GetVolConfigFrameSkip _P_((VolConfig *cfg));
Int GetVolConfigModTimeBase _P_((VolConfig *cfg,Int i));

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _MOM_ACCESS_H_ */

