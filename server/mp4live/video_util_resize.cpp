/*
 *      Filtered Image Rescaling
 *
	Additional changes by Ray Gardener, Daylon Graphics Ltd.
	December 4, 1999

	Summary:

	    - Horizontal filter contributions are calculated on the fly,
	      as each column is mapped from src to dst image. This lets 
	      us omit having to allocate a temporary full horizontal stretch 
	      of the src image.

	    - If none of the src pixels within a sampling region differ, 
	      then the output pixel is forced to equal (any of) the source pixel.
	      This ensures that filters do not corrupt areas of constant color.

	    - Filter weight contribution results, after summing, are 
	      rounded to the nearest pixel color value instead of 
	      being casted to Pixel (usually an int or char). Otherwise, 
	      artifacting occurs. 

	    - All memory allocations checked for failure; scale() returns 
	      error code. new_image() returns NULL if unable to allocate 
	      pixel storage, even if Image struct can be allocated.
	      Some assertions added.

	    - load_image(), save_image() take filenames, not file handles.

	    - TGA bitmap format available. If you want to add a filetype, 
	      extend the gImageHandlers array, and factor in your load_image_xxx()
	      and save_image_xxx() functions. Search for string 'add your' 
	      to find appropriate code locations.

	    - The 'input' and 'output' command-line arguments do not have 
	      to specify .bm files; any supported filetype is okay.

	    - Added implementation of getopt() if compiling under Windows.
*/

#include "mp4live.h"
#include "video_util_resize.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WHITE_PIXEL (255)
#define BLACK_PIXEL (0)
#define CLAMP(v) (((v) < (BLACK_PIXEL)) ? \
                  (BLACK_PIXEL) :  \
                  (((v) > (WHITE_PIXEL)) ? (WHITE_PIXEL) : (v)))


#define get_pixel(image, y, x) ((image)->data[(x) + (y) * image->span])
#define put_pixel(image, y, x, p) ((image)->data[(x) + (y) * image->span] = (p))


/* Helper data structures: */

typedef struct {
    int pixel;
    fixdouble   weight;
} CONTRIB;

typedef struct {
    int n;      /* number of contributors */
    CONTRIB *p;     /* pointer to list of contributions */
} CLIST;

void scale_setup_image(image_t *img, int w, int h, int depth, pixel_t *data)
{
    img->xsize = h;
    img->ysize = w;
    img->pixspan = depth;
    img->span = w * depth;
    img->data = data;
}

image_t *scale_new_image(int w, int h, int depth)  /* create a blank image */
{
    image_t *image;

    if((image = (image_t *)malloc(sizeof(image_t))))
    {
		image->xsize = h;
		image->ysize = w;
		image->span = w * depth;
	    image->pixspan = depth;
		image->data = NULL;
    }
    return image;
}

void scale_free_image(image_t *image)
{
    free(image);
}



/*
 *  filter function definitions
 */

double Hermite_filter(double t)
{
    // f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 
    if(t < 0.0) t = -t;
    if(t < 1.0) return((2.0 * t - 3.0) * t * t + 1.0);
    return(0.0);
}


double Box_filter(double t)
{
    if((t > -0.5) && (t <= 0.5)) return(1.0);
    return(0.0);
}

double Triangle_filter(double t)
{
    if(t < 0.0) t = -t;
    if(t < 1.0) return(1.0 - t);
    return(0.0);
}

double Bell_filter(double t)        /* box (*) box (*) box */
{
    if(t < 0) t = -t;
    if(t < .5) return(.75 - (t * t));
    if(t < 1.5) {
        t = (t - 1.5);
        return(.5 * (t * t));
    }
    return(0.0);
}

double B_spline_filter(double t)    /* box (*) box (*) box (*) box */
{
    double tt;

    if(t < 0) t = -t;
    if(t < 1) {
        tt = t * t;
        return((.5 * tt * t) - tt + (2.0 / 3.0));
    } else if(t < 2) {
        t = 2 - t;
        return((1.0 / 6.0) * (t * t * t));
    }
    return(0.0);
}

static double sinc(double x)
{
    x *= M_PI;

    if(x != 0) return(sin(x) / x);
    return(1.0);
}

double Lanczos3_filter(double t)
{
    if(t < 0) t = -t;
    if(t < 3.0) return(sinc(t) * sinc(t/3.0));
    return(0.0);
}

#define B   (1.0 / 3.0)
#define C   (1.0 / 3.0)

double Mitchell_filter(double t)
{
    double tt;

    tt = t * t;
    if(t < 0) t = -t;
    if(t < 1.0) {
        t = (((12.0 - 9.0 * B - 6.0 * C) * (t * tt))
           + ((-18.0 + 12.0 * B + 6.0 * C) * tt)
           + (6.0 - 2 * B));
        return(t / 6.0);
    } else if(t < 2.0) {
        t = (((-1.0 * B - 6.0 * C) * (t * tt))
           + ((6.0 * B + 30.0 * C) * tt)
           + ((-12.0 * B - 48.0 * C) * t)
           + (8.0 * B + 24 * C));
        return(t / 6.0);
    }
    return(0.0);
}

/*
 *  image rescaling routine
 */

/* 
  calc_x_contrib(
    CLIST* contribX;            * Receiver of contrib info
    double xscale;              * Horizontal scaling
    double fwidth;              * Filter sampling width
    int dstwidth;               * Target bitmap width
    int srcwidth;               * Source bitmap width
    double (*filterf)(double);  * Filter proc
    int i;                      * pixel_t column in source bitmap being processed
  )
    Calculates the filter weights for a single target column.
    contribX->p must be freed afterwards.

    Returns -1 if error, 0 otherwise.
*/
static int calc_x_contrib(CLIST* contribX, double xscale, double fwidth, int dstwidth, int srcwidth, 
                   double (*filterf)(double), int i)
{
    double width;
    double fscale;
    double center, left, right;
    double weight;
    int j, k, n;

    if(xscale < 1.0)
    {
        /* Shrinking image */
        width = fwidth / xscale;
        fscale = 1.0 / xscale;

        contribX->n = 0;
        contribX->p = (CONTRIB *)calloc((int) (width * 2 + 1),
                sizeof(CONTRIB));
        if(contribX->p == NULL)
            return -1;

        center = (double) i / xscale;
        left = ceil(center - width);
        right = floor(center + width);
        for(j = (int)left; j <= right; ++j)
        {
            weight = center - (double) j;
            weight = (*filterf)(weight / fscale) / fscale;
            if(j < 0)
                n = -j;
            else if(j >= srcwidth)
                n = (srcwidth - j) + srcwidth - 1;
            else
                n = j;
            
            k = contribX->n++;
            contribX->p[k].pixel = n;
            contribX->p[k].weight = double2fixdouble(weight);
        }
    
    }
    else
    {
        /* Expanding image */
        contribX->n = 0;
        contribX->p = (CONTRIB *)calloc((int) (fwidth * 2 + 1),
                sizeof(CONTRIB));
        if(contribX->p == NULL)
            return -1;
        center = (double) i / xscale;
        left = ceil(center - fwidth);
        right = floor(center + fwidth);

        for(j = (int)left; j <= right; ++j)
        {
            weight = center - (double) j;
            weight = (*filterf)(weight);
            if(j < 0) {
                n = -j;
            } else if(j >= srcwidth) {
                n = (srcwidth - j) + srcwidth - 1;
            } else {
                n = j;
            }
            k = contribX->n++;
            contribX->p[k].pixel = n;
            contribX->p[k].weight = double2fixdouble(weight);
        }
    }
    return 0;
} /* calc_x_contrib */


scaler_t *
scale_image_init(image_t *dst, image_t *src, double (*filterf)(double), double fwidth)
{
    scaler_t *scaler;
    int i, j, k;            /* loop variables */
    int n;              /* pixel number */
    int xx;
    double center = 0.0, left, right;   /* filter calculation variables */
    double width, fscale, weight;   /* filter calculation variables */
    double xscale, yscale;
    double maxwidth;
    CLIST  contribX;
    CLIST *contribY;
    instruction_t *prg;
    
    scaler = (scaler_t*)malloc(sizeof(scaler_t));
    scaler->src = src;
    scaler->dst = dst;

    /* create intermediate column to hold horizontal dst column scale */
    scaler->tmp = (pixel_t*)malloc(src->ysize * sizeof(pixel_t));
    if(scaler->tmp == NULL)
    {
        free(scaler);
        return 0;
    }

    xscale = (double) dst->xsize / (double) src->xsize;

    /* Build y weights */
    /* pre-calculate filter contributions for a column */
    contribY = (CLIST *)calloc(dst->ysize, sizeof(CLIST));
    if(contribY == NULL)
    {
        free(scaler->tmp);
        free(scaler);
        return 0;
    }

    yscale = (double) dst->ysize / (double) src->ysize;

    if(yscale < 1.0)
    {
        width = fwidth / yscale;
        fscale = 1.0 / yscale;
        for(i = 0; i < dst->ysize; ++i)
        {
            contribY[i].n = 0;
            contribY[i].p = (CONTRIB *)calloc((int) (width * 2 + 1), sizeof(CONTRIB));
            if(contribY[i].p == NULL)
            {
                free(scaler->tmp);
                free(contribY);
                free(scaler);
                return 0;
            }
            center = (double) i / yscale;
            left = ceil(center - width);
            right = floor(center + width);
            for(j = (int)left; j <= right; ++j) {
                weight = center - (double) j;
                weight = (*filterf)(weight / fscale) / fscale;
                if(j < 0) {
                    n = -j;
                } else if(j >= src->ysize) {
                    n = (src->ysize - j) + src->ysize - 1;
                } else {
                    n = j;
                }
                k = contribY[i].n++;
                contribY[i].p[k].pixel = n;
                contribY[i].p[k].weight = double2fixdouble(weight);
            }
        }
    } else {
        for(i = 0; i < dst->ysize; ++i) {
            contribY[i].n = 0;
            contribY[i].p = (CONTRIB *)calloc((int) (fwidth * 2 + 1), sizeof(CONTRIB));
            if(contribY[i].p == NULL)
            {
                free(scaler->tmp);
                free(scaler);
                return 0;
            }
            center = (double) i / yscale;
            left = ceil(center - fwidth);
            right = floor(center + fwidth);
            for(j = (int)left; j <= right; ++j) {
                weight = center - (double) j;
                weight = (*filterf)(weight);
                if(j < 0) {
                    n = -j;
                } else if(j >= src->ysize) {
                    n = (src->ysize - j) + src->ysize - 1;
                } else {
                    n = j;
                }
                k = contribY[i].n++;
                contribY[i].p[k].pixel = n;
                contribY[i].p[k].weight = double2fixdouble(weight);
            }
        }
    }

    /* -------------------------------------------------------------
       streamline contributions into two simple bytecode programs. This
       single optimalization nearly doubles the performance! */

    maxwidth = fwidth;
    if (xscale < 1.0 || yscale < 1.0)
       maxwidth = fwidth / (xscale < yscale ? xscale : yscale);

    prg = scaler->programX = (instruction_t*)calloc(scaler->dst->xsize * 
                                    (2 + 2*(int)(maxwidth*2+1)),
                                    sizeof(instruction_t));

    for(xx = 0; xx < scaler->dst->xsize; xx++)
    {
        calc_x_contrib(&contribX, xscale, fwidth, 
                       scaler->dst->xsize, scaler->src->xsize,
                       filterf, xx);

        /*pel = get_pixel(scaler->src, contribX.p[0].pixel, k);*/
        (prg++)->index = contribX.p[0].pixel * scaler->src->span;
        (prg++)->count = contribX.n;
        for(j = 0; j < contribX.n; ++j)
        {
            /*pel2 = get_pixel(scaler->src, contribX.p[j].pixel, k);*/
            (prg++)->index = contribX.p[j].pixel * scaler->src->span;
            /*weight += pel2 * contribX.p[j].weight;*/
            (prg++)->weight = contribX.p[j].weight;
        }
        free(contribX.p);
    }

    prg = scaler->programY = (instruction_t*)calloc(scaler->dst->ysize * 
                                    (2 + 2*(int)(maxwidth*2+1)),
                                    sizeof(instruction_t));
    
    /* The temp column has been built. Now stretch it 
     vertically into dst column. */
    for(i = 0; i < scaler->dst->ysize; ++i)
    {
        /**(prg++) = scaler->contribY[i].p[0].pixel;*/
        (prg++)->pixel = scaler->tmp + contribY[i].p[0].pixel;
        (prg++)->count = contribY[i].n;
        for(j = 0; j < contribY[i].n; ++j)
        {
            /*(prg++) = scaler->contribY[i].p[j].pixel;*/
            (prg++)->pixel = scaler->tmp + contribY[i].p[j].pixel;
            (prg++)->weight = contribY[i].p[j].weight;
        }
    } /* next dst row */

    /* ---------------------------------------------------
       free the memory allocated for vertical filter weights -- no 
       longer needed, we have programX and programY */
    for(i = 0; i < scaler->dst->ysize; ++i)
        free(contribY[i].p);
    free(contribY);

    return scaler;
}


void scale_image_process(scaler_t *scaler)
{
    int x;
    int i = 0, j, k;            /* loop variables */
    pixel_t pel, pel2;
    int bPelDelta;
    fixdouble weight;
    pixel_t *in;
    pixel_t *out = scaler->dst->data;
    pixel_t *tmpOut;
    instruction_t *prgY, *prgX = NULL, *prgXa;

    prgXa = scaler->programX;
    
    for(x = scaler->dst->xsize; x; --x) {

		/* Apply horz filter to make dst column in tmp. */
		for(in = scaler->src->data, tmpOut = scaler->tmp, 
		  k = scaler->src->ysize; k; --k, in++) {
			prgX = prgXa;
			weight = 0;
			bPelDelta = false;
			pel = in[(prgX++)->index];
			for(j = (prgX++)->count; j; --j) {
				pel2 = in[(prgX++)->index];
				if(pel2 != pel) {
					bPelDelta = true;
				}
				weight += pel2 * (prgX++)->weight;
			}
			*(tmpOut++) = 
				bPelDelta ? (pixel_t)CLAMP(fixdouble2int(weight)) : pel;
		} /* next row in temp column */

		prgXa = prgX;

		/*
		 * The temp column has been built. 
		 * Now stretch it vertically into dst column. 
		 */
		prgY = scaler->programY;
		for(i = scaler->dst->ysize; i; --i) {
			weight = 0;
			bPelDelta = false;
			pel = *((prgY++)->pixel);

			for(j = (prgY++)->count; j; --j) {
				pel2 = *((prgY++)->pixel);
				if(pel2 != pel) {
					bPelDelta = true;
				}
				weight += pel2 * (prgY++)->weight;
			}
			*(out++) = 
				bPelDelta ? (pixel_t)CLAMP(fixdouble2int(weight)) : pel;
		} /* next dst row */

    } /* next dst column */
}

void scale_image_done(scaler_t *scaler)
{
    free(scaler->tmp);
    free(scaler->programY);
    free(scaler->programX);
    free(scaler);
}

void CopyYuv (const uint8_t *fY, const uint8_t *fU, const uint8_t *fV,
	      uint32_t fyStride, uint32_t fuStride, uint32_t fvStride,
	      uint8_t *tY, uint8_t *tU, uint8_t *tV,
	      uint32_t tyStride, uint32_t tuStride, uint32_t tvStride,
	      uint32_t w, uint32_t h)
{
  const uint8_t *from;
  uint8_t *to;
  uint ix;
  if (fY == NULL || fU == NULL || fV == NULL ||
      tY == NULL || tU == NULL || tV == NULL) return;
  if (fyStride == tyStride) {
    memcpy(tY, fY, fyStride * h);
  } else {
    to = tY;
    from = fY;
    for (ix = 0; ix < h; ix++) {
      memcpy(to, from, w);
      to += tyStride;
      from += fyStride;
    }
  }
  h /= 2;
  w /= 2;

  if (fuStride == tuStride) {
    memcpy(tU, fU, fuStride * h);
  } else {
    to = tU;
    from = fU;
    for (ix = 0; ix < h; ix++) {
      memcpy(to, from, w);
      to += tuStride;
      from += fuStride;
    }
  } 
  if (fvStride == tvStride) {
    memcpy(tV, fV, fvStride * h);
  } else {
    to = tV;
    from = fV;
    for (ix = 0; ix < h; ix++) {
      memcpy(to, from, w);
      to += tuStride;
      from += fuStride;
    }
  }
}
