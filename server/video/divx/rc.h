
#ifndef RC_INC
#define RC_INC

#include "momusys.h"

/* ------------------------------------------------------------------------
   RC_HIST: Data structure to store the history of a VOL
   ------------------------------------------------------------------------ */

#define RC_WINDOW_LENGTH 200

typedef struct
   {
   Int  size;           /* Storage capacity */
      
   Int  n;              /* Number of actually stored values */
   Int  prev;           /* 1 if first set of data already stored */
   Int  ptr;            /* Position where next data are going to be stored */
//   Int  proc_stat;      /* Flag for process status:
//                                 0 = processed
//                                 1 = currently in process 
//                                 2 = not yet processed */
   Int    *bits_text;   /* Bits for texture */
   Int    *pixels;      /* Number of coded pixels */
   Double *mad_text;    /* Mean average prediction error for texture */
   Double *dist;        /* Distortion for texture */
   Int    *bits_vop;    /* Total bits */ 
   Int    *qp;          /* Employed QP */

//   Double  total_dist;      /* Total texture distortion */
//   Int     total_bits_text; /* Total bits texture */
//   Int     total_bits;      /* Total bits */
//   Int     total_frames;    /* Total frames */

//   Int     last_pred_bits_text;   /* Predicted bits for texture */

   Double  rate_history;
   Int  history_length;

   } RC_HIST;

#define rch_get_size(rch)       ((rch)->size)
#define rch_get_n(rch)          ((rch)->n)
#define rch_get_ptr(rch)        ((rch)->ptr)
//#define rch_proc_stat(rch)      ((rch)->proc_stat)

#define rch_get_bits_text(rch)  ((rch)->bits_text)
#define rch_get_pixels(rch)     ((rch)->pixels)
#define rch_get_mad_text(rch)   ((rch)->mad_text)
#define rch_get_dist(rch)       ((rch)->dist)
#define rch_get_bits_total(rch) ((rch)->bits_vop) 
#define rch_get_qp(rch)         ((rch)->qp)

//#define rch_get_total_bits(rch) ((rch)->total_bits) 
//#define rch_get_total_frames(rch) ((rch)->total_frames) 

#define rch_first(rch)         ((rch)->n==0)

//#define rch_get_pred(rch)  ((rch)->last_pred_bits_text)
//#define rch_get_beta(rch)  ((rch)->beta)

 /* ------------------------------------------------------------------------
   RCQ2_DATA: Q2 algorithm data and parameters
   ------------------------------------------------------------------------ */

#define START_MODEL_UPDATING   1
#define MAX_SLIDING_WINDOW     20
#define PAST_PERCENT           0.05

/* VOL-level parameters*/
typedef struct
   {
      Int     bit_rate; /* in bits/second */
      Int     numFrame; /* # of frames remaining of each type in this GOP */
      Int     num_I_Frame; /* # of I frames remaining in this GOP */
      Int     num_B_Frame; /* # of B frames remaining in this GOP */
      Int     num_P_Frame; /* # of P frames remaining in this GOP */
      Double  alpha_I;  /* factor for bit amount provided in relation to P frame */
      Double  alpha_B;  /* factor for bit amount provided in relation to P frame */
      Double  bits_P_ave;  /* average bit amount for P frames */
      Int     T;        /* target # of bits for the next frame of each type */
      Int     R;        /* remaining number of bits assigned to the GOP. */
      Double  Bpp;      /* bit_rate / picture_rate */
      Int     VBV_size;
      Int     VBV_fullness;    /* buffer occupancy */
      Double  *X1;       /* second order complexity measure */
      Double  *X2;       /* second order complexity measure */
      Int     skip;
      Int     totalskipped;
      Int     codedFr;
      Int     dataNumber;
      Int     headerBitCount;
      Int     *QPfirst;  

	  Double  decay;
	  Int    rc_period;
	  Int	target_rate;
   } RCQ2_DATA;

#define rcQ2_get_QPfirst(rcc, n)         ((rcc)->QPfirst[n])

#endif
