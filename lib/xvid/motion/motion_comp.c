#include "../encoder.h"
#include "../utils/mbfunctions.h"
#include "../image/interpolate8x8.h"
#include "../utils/timer.h"

#define ABS(X) (((X)>0)?(X):-(X))
#define SIGN(X) (((X)>0)?1:-1)

static __inline void compensate8x8_halfpel(
				int16_t * const dct_codes,
				uint8_t * const cur,
				const uint8_t * const ref,
				const uint8_t * const refh,
				const uint8_t * const refv,
				const uint8_t * const refhv,
				const uint32_t x, const uint32_t y,
				const int32_t dx,  const int dy,
				const uint32_t stride)
{
	int32_t ddx,ddy;

	switch ( ((dx&1)<<1) + (dy&1) )   // ((dx%2)?2:0)+((dy%2)?1:0)
    {
    case 0 :
		ddx = dx/2;
		ddy = dy/2;
		transfer_8to16sub(dct_codes, cur + y*stride + x, 
				ref + (y+ddy)*stride + x+ddx, stride);
		break;

    case 1 :
		ddx = dx/2;
		ddy = (dy-1)/2;
		transfer_8to16sub(dct_codes, cur + y*stride + x, 
				refv + (y+ddy)*stride + x+ddx, stride);
		break;

    case 2 :
		ddx = (dx-1)/2;
		ddy = dy/2;
		transfer_8to16sub(dct_codes, cur + y*stride + x, 
				refh + (y+ddy)*stride + x+ddx, stride);
		break;

	default :	// case 3:
		ddx = (dx-1)/2;
		ddy = (dy-1)/2;
		transfer_8to16sub(dct_codes, cur + y*stride + x, 
				refhv + (y+ddy)*stride + x+ddx, stride);
		break;
    }
}



void MBMotionCompensation(
	MACROBLOCK * const mb,
	const uint32_t i,
	const uint32_t j,
	const IMAGE * const ref,
	const IMAGE * const refh,
	const IMAGE * const refv,
	const IMAGE * const refhv,
	IMAGE * const cur,
	int16_t *dct_codes,
	const uint32_t width, 
	const uint32_t height,
	const uint32_t edged_width,
	const uint32_t rounding)
{
	static const uint32_t roundtab[16] =
		{ 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 };


	if (mb->mode == MODE_INTER || mb->mode == MODE_INTER_Q)
	{
		int32_t dx = mb->mvs[0].x;
		int32_t dy = mb->mvs[0].y;

		compensate8x8_halfpel(&dct_codes[0*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i,     16*j,     dx, dy, edged_width);
		compensate8x8_halfpel(&dct_codes[1*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i + 8, 16*j,     dx, dy, edged_width);
		compensate8x8_halfpel(&dct_codes[2*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i,     16*j + 8, dx, dy, edged_width);
		compensate8x8_halfpel(&dct_codes[3*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i + 8, 16*j + 8, dx, dy, edged_width);

		dx = (dx & 3) ? (dx >> 1) | 1 : dx / 2;
		dy = (dy & 3) ? (dy >> 1) | 1 : dy / 2;

		/* uv-image-based compensation
		   compensate8x8_halfpel(dct_codes[4], cur->u, ref->u, refh->u, refv->u, refhv->u,
		   8*i, 8*j, dx, dy, edged_width/2);
		   compensate8x8_halfpel(dct_codes[5], cur->v, ref->v, refh->v, refv->v, refhv->v,
		   8*i, 8*j, dx, dy, edged_width/2);		*/

		/* uv-block-based compensation */
		interpolate8x8_switch(refv->u, ref->u, 8*i, 8*j, dx, dy, edged_width/2, rounding);
		transfer_8to16sub(&dct_codes[4*64], 
				  cur->u + 8*j*edged_width/2 + 8*i, 
				  refv->u + 8*j*edged_width/2 + 8*i, edged_width/2);

		interpolate8x8_switch(refv->v, ref->v, 8*i, 8*j, dx, dy, edged_width/2, rounding);
		transfer_8to16sub(&dct_codes[5*64], 
				  cur->v + 8*j*edged_width/2 + 8*i, 
				  refv->v + 8*j*edged_width/2 + 8*i, edged_width/2);

	}
	else	// mode == MODE_INTER4V
	{
		int32_t sum, dx, dy;

		compensate8x8_halfpel(&dct_codes[0*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i,     16*j,     mb->mvs[0].x, mb->mvs[0].y, edged_width);
		compensate8x8_halfpel(&dct_codes[1*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i + 8, 16*j,     mb->mvs[1].x, mb->mvs[1].y, edged_width);
		compensate8x8_halfpel(&dct_codes[2*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i,     16*j + 8, mb->mvs[2].x, mb->mvs[2].y, edged_width);
		compensate8x8_halfpel(&dct_codes[3*64], cur->y, ref->y, refh->y, refv->y, refhv->y,
				      16*i + 8, 16*j + 8, mb->mvs[3].x, mb->mvs[3].y, edged_width);

		sum = mb->mvs[0].x + mb->mvs[1].x + mb->mvs[2].x + mb->mvs[3].x;
		dx = (sum ? SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2) : 0);

		sum = mb->mvs[0].y + mb->mvs[1].y + mb->mvs[2].y + mb->mvs[3].y;
		dy = (sum ? SIGN(sum) * (roundtab[ABS(sum) % 16] + (ABS(sum) / 16) * 2) : 0);

		/* uv-image-based compensation
		   compensate8x8_halfpel(dct_codes[4], cur->u, ref->u, refh->u, refv->u, refhv->u,
		   8*i, 8*j, dx, dy, edged_width/2);
		   compensate8x8_halfpel(dct_codes[5], cur->v, ref->v, refh->v, refv->v, refhv->v,
		   8*i, 8*j, dx, dy, edged_width/2);		*/

		/* uv-block-based compensation */
		interpolate8x8_switch(refv->u, ref->u, 8*i, 8*j, dx, dy, edged_width/2, rounding);
		transfer_8to16sub(&dct_codes[4*64], 
				  cur->u + 8*j*edged_width/2 + 8*i, 
				  refv->u + 8*j*edged_width/2 + 8*i, edged_width/2);

		interpolate8x8_switch(refv->v, ref->v, 8*i, 8*j, dx, dy, edged_width/2, rounding);
		transfer_8to16sub(&dct_codes[5*64], 
				  cur->v + 8*j*edged_width/2 + 8*i, 
				  refv->v + 8*j*edged_width/2 + 8*i, edged_width/2);

	}
}
