
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
 *  mom_util.c
 *
 *  Copyright (C) 2001  Project Mayo
 *
 *  Adam Li
 *
 *  DivX Advance Research Center <darc@projectmayo.com>
 *
 **************************************************************************/

/* This file contains some utility functions to manipulate Image data     */
/* structures.                                                            */
/* Some codes of this project come from MoMuSys MPEG-4 implementation.    */
/* Please see seperate acknowledgement file for a list of contributors.   */

#include "mom_util.h"

/* private prototypes */
Char *emalloc(Int n);
Char *ecalloc(Int n, Int s);
Char *erealloc(Char *p, Int n);
Void CopyImageI(ImageI *image_in, ImageI *image_out);
Void CopyImageF(ImageF *image_in, ImageF *image_out);
Void SetConstantImageI(ImageI *image, SInt val);
Void SetConstantImageF(ImageF *image, Float val);
Void SubImageI(ImageI *image_in1, ImageI *image_in2, ImageI *image_out);
Void SubImageF(ImageF *image_in1, ImageF *image_in2, ImageF *image_out);


/* The following is the Image maintance functions. */

/***********************************************************CommentBegin******
 *
 * -- AllocImage -- Allocates memory for an Image structure
 *
 * Purpose :		
 *	Allocates memory for an Image structure, including memory
 *	for the actual image pixels. The image pixels can be of type
 *	UChar, SInt or Float depending on the parameter	type.
 * 
 * Return values :	
 *	A pointer to the allocated Image structure.
 *
 * Description :	
 *	Allocates memory for the Image if possible, otherwise a 
 *	memory allocation error is signalled and the program will
 *	exit.
 *
 ***********************************************************CommentEnd********/

Image *
AllocImage(UInt size_x, UInt size_y, ImageType type)
{
  Image *image;
  
  image = (Image *) emalloc(sizeof(Image));
  
  image->version = IMAGE_VERSION;
  image->x = size_x;
  image->y = size_y;
  image->upperodd = 0;
  image->grid = 's';
  image->type = type;
  image->data = (ImageData *) emalloc(sizeof(ImageData)); 
  
  switch(type)
    {
    case SHORT_TYPE:
      image->data->s = (SInt *) ecalloc(size_x*size_y,sizeof(SInt));
      break;

    case FLOAT_TYPE:
      image->data->f = (Float *) ecalloc(size_x*size_y,sizeof(Float));
      break;

    case UCHAR_TYPE:
      image->data->u = (UChar *) ecalloc(size_x*size_y,sizeof(UChar));
      break;
    }
  
  image->f = image->data->s;	/* For compatibility with KHOROS */
  
  return(image);
}


/***********************************************************CommentBegin******
 *
 * -- FreeImage -- Frees up all the memory associated with an Image structure
 *
 * Purpose :		
 *	Frees up all the memory associated with an Image structure
 * 
 ***********************************************************CommentEnd********/

Void
FreeImage(Image *image)
{
  SInt	*ps;
  Float	*pf;
  UChar 	*pu;

  if (image == NULL) return;

  switch(image->type)
    {
    case SHORT_TYPE:
      ps = (SInt *)GetImageData(image);
      if(ps != NULL) free((Char *)ps);
      free((Char *) image->data);
      free((Char *)image);
      break;

    case FLOAT_TYPE:
      pf = (Float *)GetImageData(image);
      if(pf != NULL) free((Char *)pf);
      free((Char *) image->data);
      free((Char *)image);
      break;

    case UCHAR_TYPE:
      pu = (UChar *)GetImageData(image);
      if(pu != NULL) free((Char *)pu);
      free((Char *) image->data);
      free((Char *)image);
      break;
    }
}


/***********************************************************CommentBegin******
 *
 * -- CopyImage -- Copies the pixel values of one image to another image
 *
 * Purpose :		
 *	Copies the pixel values of one image to another image.
 * 
 * Description :	
 *	Currently, the function exits with an error if
 *	the source and destination images are not of the same allocated
 *	size and type.
 *
 *	Also, the images of type UChar are not yet supported.
 *
 ***********************************************************CommentEnd********/

Void
CopyImage(Image *image_in, Image *image_out)
{
  switch(image_out->type)
    {
    case SHORT_TYPE:
      CopyImageI(image_in,image_out);
      break;

    case FLOAT_TYPE:
      CopyImageF(image_in,image_out);
      break;

    }
}


/***********************************************************CommentBegin******
 *
 * -- CopyImageI -- 
 *
 ***********************************************************CommentEnd********/

Void
CopyImageI(ImageI *image_in, ImageI *image_out)
{
  SInt 	*p_in  = image_in->data->s,
		*p_out = image_out->data->s,
		*p_end;

  UInt	sx_in  = image_in->x,
		sx_out = image_out->x,
		sy_in  = image_in->y,
		sy_out = image_out->y,
		sxy_in = sx_in * sy_in;

  p_end = p_in + sxy_in;

  while (p_in != p_end) 
    {
      *p_out = *p_in;
      p_in++;
      p_out++;
    }

}


/***********************************************************CommentBegin******
 *
 * -- CopyImageF -- Copies image_in to image_out
 *
 ***********************************************************CommentEnd********/

Void
CopyImageF(ImageF *image_in, ImageF *image_out)
{
  Float *p_in  = image_in->data->f,
		*p_out = image_out->data->f,
		*p_end;

  UInt  sx_in = image_in->x,
		sx_out = image_out->x,
		sy_in = image_in->y,
		sy_out = image_out->y,
		sxy_in = sx_in * sy_in;

  p_end = p_in + sxy_in;

  while (p_in != p_end) 
    {
      *p_out = *p_in;
      p_in++;
      p_out++;
    }

}


/***********************************************************CommentBegin******
 *
 * -- SetConstantImage -- Sets each pixel in the Image to val
 *
 * Purpose :		
 *	Sets each pixel in the Image to val.
 * 
 * Side effects :	
 *	Note val is a Float. If image is not a Float image then
 *	val is cast to the appropriate type i.e. SInt, unsigned
 *	Char.
 *
 * Description :	
 *	Images of type UChar are not yet supported.
 *
 ***********************************************************CommentEnd********/

Void
SetConstantImage(Image *image, Float val)
{
  switch(image->type)
    {
    case SHORT_TYPE:
      SetConstantImageI(image,(SInt)val);
      break;

    case FLOAT_TYPE:
      SetConstantImageF(image,val);
      break;
    }
}



/***********************************************************CommentBegin******
 *
 * -- SetConstantImageI -- 
 *
 ***********************************************************CommentEnd********/

Void
SetConstantImageI(ImageI *image, SInt val)
{
  SInt	*p  = image->data->s,
        *p_end;

  UInt	sxy = image->x * image->y;

  if (val == 0)
	  memset (p, 0, sxy * 2);
  else
  {
	p_end = p + sxy;

	while (p != p_end)
	{
		*p = val;
		p++;
    }
  }

}

/***********************************************************CommentBegin******
 *
 * -- SetConstantImageF -- 
 *
 ***********************************************************CommentEnd********/

Void
SetConstantImageF(ImageF *image, Float val)
{
  Float		*p  = image->data->f,
            *p_end;

  UInt	sxy = image->x * image->y;

  p_end = p + sxy;

  while (p != p_end)
    {
      *p = val;
      p++;
    }

}


/***********************************************************CommentBegin******
 *
 * -- SubImage -- Subtracts two images
 *
 * Purpose :		
 *	Subtracts two images and stores the result in the third
 * 	i.e. image_out = image_in1 - image_in2
 * 
 * Description :	
 *	Currently, the function exits with an error if
 *	the source and destination images are not of the same allocated
 *	size and type.
 *
 *	Also, the images of type UChar are not yet supported.
 *
 ***********************************************************CommentEnd********/

Void
SubImage(Image *image_in1, Image *image_in2, Image *image_out)
{
  switch(image_in1->type)
    {
    case SHORT_TYPE:
      SubImageI(image_in1,image_in2,image_out);
      break;

    case FLOAT_TYPE:
      SubImageF(image_in1,image_in2,image_out);
      break;
    }
}

/***********************************************************CommentBegin******
 *
 * -- SubImageI -- 
 *
 ***********************************************************CommentEnd********/

Void
SubImageI(ImageI *image_in1, ImageI *image_in2, ImageI *image_out)
{
  SInt	*p  = image_out->data->s,
		*p1 = image_in1->data->s,
		*p2 = image_in2->data->s,
		*p_end;

  UInt	sx_in1 = image_in1->x,
		sx_in2 = image_in2->x,
		sx_out = image_out->x,
		sy_in1 = image_in1->y,
		sy_in2 = image_in2->y,
		sy_out = image_out->y,
		sxy    = sx_out * sy_out;

  p_end = p + sxy;

  while (p != p_end)
    {
      *p = *p1 - *p2;
      p++;
      p1++;
      p2++;
    }

}

/***********************************************************CommentBegin******
 *
 * -- SubImageF -- 
 *
 ***********************************************************CommentEnd********/

Void
SubImageF(ImageF *image_in1, ImageF *image_in2, ImageF *image_out)
{
  Float	*p  = image_out->data->f,
		*p1 = image_in1->data->f,
		*p2 = image_in2->data->f,
		*p_end;

  UInt	sx_in1 = image_in1->x,
		sx_in2 = image_in2->x,
		sx_out = image_out->x,							
		sy_in1 = image_in1->y,
		sy_in2 = image_in2->y,
		sy_out = image_out->y,
		sxy    = sx_out * sy_out;

  p_end = p + sxy;

  while (p != p_end)
    {
      *p = *p1 - *p2;
      p++;
      p1++;
      p2++;
    }

}

/* The following is Vop maintance functions. */

Vop *
SallocVop()
{
	Vop   *vop;

	vop = (Vop *)ecalloc(1,sizeof(Vop));

	return(vop);
}


Vop *
AllocVop(UInt x, UInt y)
{
  Vop		*vop;

  Image	*y_chan,
		*u_chan,
		*v_chan; 

  /* first allocate memory for the data structure */
  vop = SallocVop();

  vop->width = x;
  vop->height = y;

  /* Allocate image fields */
  y_chan = AllocImage(x,y,SHORT_TYPE);
  u_chan = AllocImage(x/2,y/2,SHORT_TYPE);
  v_chan = AllocImage(x/2,y/2,SHORT_TYPE);

  /* Include image fields in structure */
  FreeImage(vop->y_chan);
  vop->y_chan = y_chan;

  FreeImage(vop->u_chan);
  vop->u_chan = u_chan;

  FreeImage(vop->v_chan);
  vop->v_chan = v_chan;

  return(vop);
}


Void
SfreeVop (Vop *vop)
{
	free ((Char*)vop);
	/* vop = NULL; */
	return;
}


Void
FreeVop(Vop *vop)
{
  Image		*data=NULL; /*SpSc: added the initialization */

  if(vop != NULL) {

    /* Deallocate memory for image fields */

    data = vop->y_chan;
    FreeImage(data);

    data = vop->u_chan;
    FreeImage(data);

    data = vop->v_chan;
    FreeImage(data);

    SfreeVop(vop);
  }

  return;
}


Void
CopyVopNonImageField(Vop *in,Vop *out)
{
/*	out->quant_precision = in->quant_precision;
	out->bits_per_pixel = in->bits_per_pixel;
*/
//	out->mod_time_base = in->mod_time_base;
//	out->time_inc = in->time_inc;
	out->prediction_type = in->prediction_type;
/*	out->rounding_type = in->rounding_type;
	out->width = in->width;
	out->height = in->height;
	out->hor_spat_ref = in->hor_spat_ref;
	out->ver_spat_ref = in->ver_spat_ref;
	out->quantizer = in->quantizer;
	out->intra_quantizer = in->intra_quantizer;
	out->time_increment_resolution = in->time_increment_resolution;

	out->intra_acdc_pred_disable = in->intra_acdc_pred_disable;
	out->fcode_for = in->fcode_for;
	out->sr_for = in->sr_for;

	out->intra_dc_vlc_thr = in->intra_dc_vlc_thr;
*/
}


/* The following is Memory allocation functions. */

/***********************************************************CommentBegin******
 *
 * -- emalloc -- Memory allocation with error handling
 *
 * Purpose :		
 *	Memory allocation with error handling.
 * 
 * Description :	
 *	WARNING: There is a problem when you want to allocate more memory
 *	than size can handle.  Malloc gets as argument an unsigned.  It poses
 *	a problem when sizeof(unsigned) < sizeof(Char *)...
 *
 ***********************************************************CommentEnd********/

Char *
emalloc(Int n)
{
  Char 	*p;
	
  p = (Char *) malloc((UInt)n);
  return p;
}

/***********************************************************CommentBegin******
 *
 * -- ecalloc -- Memory allocation with error handling
 *
 * Purpose :		
 *	Memory allocation with error handling.
 * 
 * Description :	
 *	WARNING: There is a problem when you want to allocate more memory
 *	than size can handle.  Malloc gets as argument an unsigned.  It poses
 *	a problem when sizeof(unsigned) < sizeof(Char *)...
 *
 ***********************************************************CommentEnd********/

Char *
ecalloc(Int n, Int s)
{
  Char 	*p;
	
  p = (Char *) calloc((UInt)n,(UInt)s);
	
  return p;
}

/***********************************************************CommentBegin******
 *
 * -- erealloc -- Memory re-allocation with error handling.
 *
 * Purpose :		
 *	Memory re-allocation with error handling
 * 
 * Description :	
 *	WARNING: There is a problem when you want to allocate more memory
 *	than size can handle.  Malloc gets as argument an unsigned.  It poses
 *	a problem when sizeof(unsigned) < sizeof(Char *)...
 *
 ***********************************************************CommentEnd********/

Char *
erealloc(Char *p, Int n)
{	
  p = (Char *) realloc(p,(UInt)n);
  return p;
}


