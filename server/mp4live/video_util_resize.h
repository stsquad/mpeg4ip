/*
 *  Filtered Image Rescaling by Dale Schumacher
 */

#ifndef _SCALEIMAGE_H_
#define _SCALEIMAGE_H_

#include <math.h>

/* This type represents one pixel */
typedef unsigned char pixel_t;

/* Helper structure that holds information about image */
typedef struct
{
    int     xsize;      /* horizontal size of the image in pixel_ts */
    int     ysize;      /* vertical size of the image in pixel_ts */
    pixel_t *data;      /* pointer to first scanline of image */
    int     span;       /* byte offset between two scanlines */
    int     pixspan;    /* byte offset between two pixels on line (usually 1) */
} image_t;

/* Initializes image_t structure for given width and height.
   Please do NOT manipulate xsize, span and ysize properties directly,
   scaling code has inverted meaning of x and y axis 
   (i.e. w=ysize, h=xsize)  */
extern void scale_setup_image(image_t *img, int w, int h, int depth, pixel_t *data);


typedef int fixdouble;
#define double2fixdouble(d) ((fixdouble)((d) * 65536))
#define fixdouble2int(d)    (((d) + 32768) >> 16)

typedef union
{
   pixel_t      *pixel;
   fixdouble     weight;
   int           index;
   int           count;
} instruction_t;

/* This structure holds the state of scaling function just after
   initialization and before image processing. The advantage of
   this approach is that you can do as much of (CPU intensive)
   processing as possible only once and use relatively fast
   function for per-frame processing.  */
typedef struct
{
    image_t       *src, *dst;
    pixel_t       *tmp;
    instruction_t *programY, *programX;
}  scaler_t;

extern image_t *scale_new_image(int xsize, int ysize, int depth);

/* Initializes scaling for given destination and source dimensions
   and depths and filter function. */
extern scaler_t *scale_image_init(image_t *dst, image_t *src,
                                 double (*filterf)(double), double fwidth);

/* Processes frame.  */
extern void scale_image_process(scaler_t *scaler);

/* Shuts down scaler, deallocates memory. */
extern void scale_image_done(scaler_t *scaler);

extern void scale_free_image(image_t *image);

/* These are filter functions and their respective fwidth values
   that you can use when filtering. The higher fwidth is, the
   longer processing takes. Filter function's cost is much less
   important because it is computed only once, during scaler_t
   initialization.
*/

#define       Box_support       (0.5)
extern double Box_filter(double t);
#define       Triangle_support  (1.0)
extern double Triangle_filter(double t);
#define       Hermite_support   (1.0)
extern double Hermite_filter(double t);
#define       Bell_support      (1.5)
extern double Bell_filter(double t);        /* box (*) box (*) box */
#define       B_spline_support  (2.0)
extern double B_spline_filter(double t);    /* box (*) box (*) box (*) box */
#define       Mitchell_support  (2.0)
extern double Mitchell_filter(double t);
#define       Lanczos3_support  (3.0)
extern double Lanczos3_filter(double t);

void CopyYuv(const uint8_t *fY, const uint8_t *fU, const uint8_t *fV,
	     uint32_t fyStride, uint32_t fuStride, uint32_t fvStride,
	     uint8_t *tY, uint8_t *tU, uint8_t *fV,
	     uint32_t tyStride, uint32_t tvStride, uint32_t tvStride,
	     uint32_t w, uint32_t h);
#endif
