#include "stdlib.h"
#include "stdio.h"
#include "math.h"
#include "basic.hpp"
#include "dataStruct.hpp"


Void CVTCEncoder::read_image(Char *img_path, 
							Int img_width, 
							Int img_height, 
							Int img_colors, 
							Int img_bit_depth,
							PICTURE *img)
{
  FILE *infptr = NULL;
  int  img_size;
   /* chroma  width and height  to handle odd image size */
  int img_cwidth  = (img_width+1)/2, 
      img_cheight = (img_height+1)/2; 
  int wordsize = (img_bit_depth>8)?2:1;
  unsigned char *srcimg; /* SL: to handle 1-16 bit */
  int  i,k,status;
  
  if (img_colors==1) { /*SL: image size for mono */
     img_size = img_width*img_height; /* bit_depth>8 occupies 2 bytes */
  }
  else { /* only for 420 color for now */
    img_size = img_width*img_height+2*img_cwidth*img_cheight; 
  }
  
  srcimg = new UChar[img_size*wordsize];
  if ((infptr = fopen(img_path,"rb")) == NULL) 
    exit(fprintf(stderr,"Unable to open image_file: %s\n",img_path));

  if ((status=fread(srcimg, sizeof(unsigned char)*wordsize, img_size, infptr))
       != img_size)
    exit(fprintf(stderr,"Error in reading image file: %s\n",img_path));

  fclose(infptr);
  
  /* put into VM structure */
  img[0].width  = img_width;
  img[0].height = img_height;
  if(img_colors != 1) { /* SL: only for color image */
    img[1].width  = img_cwidth; /* to handle odd image size */
    img[1].height = img_cheight;
    img[2].width  = img_cwidth;
    img[2].height = img_cheight;
  }

  img[0].data = (void *)new UChar[img_width*img_height*wordsize];
  if (img_colors==3) {
	img[1].data = (void *)new UChar[img_cwidth*img_cheight*wordsize];
    img[2].data = (void *)new UChar[img_cwidth*img_cheight*wordsize];
  }

  k=0;
  for (i=0; i<img_width*img_height*wordsize; i++) { 
    if (img_bit_depth > 8){ /* mask the input */
      if (i%2==0)
        ((UChar *)img[0].data)[i] =
	      (UChar) (srcimg[k++]&((1<<(img_bit_depth-8))-1));
      else
        ((unsigned char *)img[0].data)[i] = srcimg[k++];
    }
    else {
      ((unsigned char *)img[0].data)[i] = 
	(unsigned char) (srcimg[k++]&((1<<img_bit_depth)-1));
    }
  }
  if (img_colors != 1) { /* SL: only for color image */
    for (i=0; i<img_cwidth*img_cheight*wordsize; i++) {
      if (img_bit_depth > 8) { /* mask the input */
        if (i%2==0) 
           ((unsigned char *)img[1].data)[i] = 
	     (unsigned char) (srcimg[k++]&((1<<(img_bit_depth-8))-1));
        else
           ((unsigned char *)img[1].data)[i] = srcimg[k++];
      }
      else {
        ((unsigned char *)img[1].data)[i] = 
	  (unsigned char) (srcimg[k++]&((1<<img_bit_depth)-1));
      }
    }
    for (i=0; i<img_cwidth*img_cheight*wordsize; i++){
      if (img_bit_depth > 8) { /* mask the input */
        if (i%2==0) 
           ((unsigned char *)img[2].data)[i] = 
	     (unsigned char) (srcimg[k++]&((1<<(img_bit_depth-8))-1));
        else
          ((unsigned char *)img[2].data)[i] = srcimg[k++];
      }
      else {
        ((unsigned char *)img[2].data)[i] = 
	  (unsigned char) (srcimg[k++]&((1<<img_bit_depth)-1));
      }
    }
  }
  if (srcimg) delete (srcimg);

}

