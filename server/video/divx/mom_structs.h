
#ifndef _MOM_STRUCTS_H_
#define _MOM_STRUCTS_H_


#include "momusys.h"

#define VERSION		1	/* Image Structure Version */

/*****
*
*     Basic and generic image structure
*
*****/
enum image_type {SHORT_TYPE,FLOAT_TYPE,UCHAR_TYPE};
typedef enum image_type ImageType;

union image_data
{
  SInt *s;		/* SHORT data */
  Float *f;		/* FLOAT data */
  UChar *u;		/* UCHAR data */
};
typedef union image_data ImageData;

struct image
{
  Int version;		/* Version number */
  UInt x,y;		/* Image size */
  Char upperodd;	/* Flag to tell if the top line is considered
			   as even or odd (used for hex grids) */
  Char grid;		/* Grid type: s = square, h = hexagonal */
  SInt *f;		/* Image data with short int values */
  ImageData *data;	/* NEW: pointer to image data */
  ImageType type;	/* NEW: type of the image */
};
typedef struct image Image;

typedef struct image ImageI;	/* For compatibility with old source code */
typedef struct image ImageF;	/* For compatibility with old source code */

/*****
*
*     Recommended structures for VOPs, VOLs and VOs (now compliant with VM3.0)
*     (in the VOP structure *all* entries from the VOL syntax which
*     are necessary for decoding the VOP are duplicated. These entries
*     are marked as "VOL ...")
*
*****/
struct vop
{
  /* Actual syntax elements for VOP (standard) */
  Int prediction_type;		/* VOP prediction type */
  Int mod_time_base;		/* VOP modulo time base (absolute) */
  Float time_inc;		/* VOP time increment (relative to last mtb) */
  Int rounding_type;

  Int width;			/* VOP height (smallest rectangle) */
  Int height;			/* VOP width  (smallest rectangle) */
  Int hor_spat_ref;		/* VOP horizontal ref. (for composition) */
  Int ver_spat_ref;		/* VOP vertical ref.   (for composition) */
  
  Int intra_dc_vlc_thr;

  Int quantizer;		/* VOP quantizer for P-VOPs */
  Int intra_quantizer;		/* VOP quantizer for I-VOPs */
 
  /* Syntax elements copied from VOL (standard) */
  Int time_increment_resolution;

  Int intra_acdc_pred_disable;	/* VOL disable INTRA DC prediction */
  Int sr_for;			/* VOP search range of motion vectors */
  Int fcode_for;		/* VOP dynamic range of motion vectors */
  Int quant_precision;
  Int bits_per_pixel;

  /* Pointers to the images (YUVA) and to related VOPs */
  Image *y_chan;		/* Y component of the VOP texture */
  Image *u_chan;		/* U component of the VOP texture */
  Image *v_chan;		/* V component of the VOP texture */
};
typedef struct vop Vop;

struct object_layer_cfg
{
	Float	frame_rate;			/* VOL frame rate */
    Int 	M;				    /* M-1 number of B-VOPs between consecutive P-VOPs*/
	Int	start_frame;			/* Frame at which to start coding */
	Int	end_frame;			    /* Frame at which to end coding */
	Int	bit_rate;			    /* Target bitrate */
	Int 	frame_skip;			/* Number of frames to skip between codings */

	Int	quantizer;			/* Initial value for H.263 INTRA quantizer */
	Int	intra_quantizer;		/* Initial value for H.263 INTER quantizer */

	Int	intra_period;	/* Regular rate of INTRA VOP */

/* Automatically calculated coding fields */
	Int	modulo_time_base[2];				/* Modulo time base must be maintained for VOL */
};
typedef struct object_layer_cfg VolConfig;


#include "mom_util.h"
#include "mom_access.h"


#endif /* _MOM_STRUCTS_H_ */
