/* $Id: vtc_ztq_quant.cpp,v 1.1 2005/05/09 21:29:49 wmaycisco Exp $ */
/****************************************************************************/
/*   MPEG4 Visual Texture Coding (VTC) Mode Software                        */
/*                                                                          */
/*   This software was jointly developed by the following participants:     */
/*                                                                          */
/*   Single-quant,  multi-quant and flow control                            */
/*   are provided by  Sarnoff Corporation                                   */
/*     Iraj Sodagar   (iraj@sarnoff.com)                                    */
/*     Hung-Ju Lee    (hjlee@sarnoff.com)                                   */
/*     Paul Hatrack   (hatrack@sarnoff.com)                                 */
/*     Shipeng Li     (shipeng@sarnoff.com)                                 */
/*     Bing-Bing Chai (bchai@sarnoff.com)                                   */
/*     B.S. Srinivas  (bsrinivas@sarnoff.com)                               */
/*                                                                          */
/*   Bi-level is provided by Texas Instruments                              */
/*     Jie Liang      (liang@ti.com)                                        */
/*                                                                          */
/*   Shape Coding is provided by  OKI Electric Industry Co., Ltd.           */
/*     Zhixiong Wu    (sgo@hlabs.oki.co.jp)                                 */
/*     Yoshihiro Ueda (yueda@hlabs.oki.co.jp)                               */
/*     Toshifumi Kanamaru (kanamaru@hlabs.oki.co.jp)                        */
/*                                                                          */
/*   OKI, Sharp, Sarnoff, TI and Microsoft contributed to bitstream         */
/*   exchange and bug fixing.                                               */
/*                                                                          */
/*                                                                          */
/* In the course of development of the MPEG-4 standard, this software       */
/* module is an implementation of a part of one or more MPEG-4 tools as     */
/* specified by the MPEG-4 standard.                                        */
/*                                                                          */
/* The copyright of this software belongs to ISO/IEC. ISO/IEC gives use     */
/* of the MPEG-4 standard free license to use this  software module or      */
/* modifications thereof for hardware or software products claiming         */
/* conformance to the MPEG-4 standard.                                      */
/*                                                                          */
/* Those intending to use this software module in hardware or software      */
/* products are advised that use may infringe existing  patents. The        */
/* original developers of this software module and their companies, the     */
/* subsequent editors and their companies, and ISO/IEC have no liability    */
/* and ISO/IEC have no liability for use of this software module or         */
/* modification thereof in an implementation.                               */
/*                                                                          */
/* Permission is granted to MPEG members to use, copy, modify,              */
/* and distribute the software modules ( or portions thereof )              */
/* for standardization activity within ISO/IEC JTC1/SC29/WG11.              */
/*                                                                          */
/* Copyright 1995, 1996, 1997, 1998 ISO/IEC                                 */
/****************************************************************************/

/************************************************************/
/*     Sarnoff Very Low Bit Rate Still Image Coder          */
/*     Copyright 1995, 1996, 1997, 1998 Sarnoff Corporation */
/************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "basic.hpp"
#include "dataStruct.hpp"
#include "quant.hpp"


/* sign */
#define SGN(x) (((x)<0) ? -1 : 1)
/* Integer rounding */
#define IROUND(n,d) ( ((n)/(d)) + (((n)%(d) > (d-1)/2) || (n)<(d))) 
/* Integer ceiling */
#define ICEIL(n,d) ( ((n)/(d)) + ((n)%(d)!=0 || (n)<(d)) ) 

/* For partitionType bit field processing */
#define MASK_fromReduced  ((UChar)'\1')
#define MASK_fromDeadZone ((UChar)'\2')

#define SET_fromReduced(x)  ((x) |= MASK_fromReduced)
#define SET_fromDeadZone(x) ((x) |= MASK_fromDeadZone)

#define CLR_fromReduced(x)  ((x) &= ~(MASK_fromReduced))
#define CLR_fromDeadZone(x) ((x) &= ~(MASK_fromDeadZone))

#define fromReduced(x)  ((x) & MASK_fromReduced)
#define fromDeadZone(x) (((x) & MASK_fromDeadZone)>>1)


/*------------------------- QUANTIZATION --------------------------*/


/* 
   Function:
   ---------
   initQuantSingleStage - Initialization of the single-stage quantizer for
     a given value.

   Arguments:
   ----------
   quantState *state - PoInter to the state data structure.
   Int *statePrevQ - PoInter to previous quantized value state.
   Int initialValue - Value which is to be quantized.

   Return Value:
   -------------
   <None>

   Description:
   ------------
   This must be called prior to single-stage quantization. A seperate
   state structure must be kept for each value that is quantized in parallel.
   Single-stage quantization is just successive calls to quantSingleStage.
   For each value,  need only be called once, before the first call to 
   quantSingleStage.
*/

Void CVTCCommon::initQuantSingleStage(quantState *state, Int *statePrevQ, Int initialVal)
{
  state->residualValue = initialVal;
  state->partitionType = 0x2; /* fromReduced = 0 and fromDeadZone  = 1 */
  *statePrevQ          = 0;
}

/* 
   Function:
   ---------
   quantSingleStage - Single-stage quantizer.

   Arguments:
   ----------
   Int Q - Quantization value. Represents desired quantization level size.
   quantState *state - State of quantizer.
   Int *statePrevQ - PoInter to previous quantized value state.
   Int updatePrevQ - 0 means don't update the statePrevQ variable. !0 means
     update it.

   Return Value:
   -------------
   New Q index.

   Description:
   ------------
   initQuantSingleStage must be called prior to using this function the
   first time for a given value. It will compute the new quantization index
   based on the current state associated with the value.
*/

Int CVTCEncoder::quantSingleStage(Int Q, quantState *state, Int *statePrevQ,
		     Int updatePrevQ)
{
  Int refLevs;    /* how many refinement levels in new stage */
  Int QIndex;     /* new quantization index to return for this stage */

  /*--------------- INITIAL QUANTIZATION STAGE -------------------*/
  if (*statePrevQ==0)
  {
    QIndex = state->residualValue/Q;

    /* update state */
    if (QIndex)
      state->residualValue = abs(state->residualValue) - (abs(QIndex)*Q);
    CLR_fromReduced(state->partitionType);
    if (QIndex)
      CLR_fromDeadZone(state->partitionType);
    else
      SET_fromDeadZone(state->partitionType);
 
    if (updatePrevQ)
      *statePrevQ = Q;

    return QIndex;
  }


  /*--------------- NON-INITIAL QUANTIZATION STAGES -------------------*/

  /* get the number of refinement levels from lastQUsed state */
  refLevs = IROUND(*statePrevQ,Q);

  /* Catch condition where there's no refinement being done.
     State information is not changed.
  */
  if (refLevs<=1)
    QIndex = 0;
  else
  {
    Int inDeadZone;  /* are we still in the dead zone */
    Int lastQUsed;   /* The "real" Q value last used */
    Int val;         /* value of number to be quantized */
    Int lastLevSize; /* Size of quantization level used last */
    Int newQUsed, newStateQ;
    Int excess;

    /* Initialize the last quant value used */
    lastQUsed = *statePrevQ;
    
    /* update new Q value state */
    newStateQ = newQUsed = ICEIL(lastQUsed,refLevs);
    if (updatePrevQ)
      *statePrevQ = newStateQ;

    /* Get last level size */
    lastLevSize = lastQUsed-fromReduced(state->partitionType);

    /* check if a reduced level can span the last level */
    if (refLevs*(newQUsed-1) >= lastLevSize)
    {
      --newQUsed;

      /* might overshoot (?) but can't reduce anymore */
      excess=0;
    }
    else
      /* get excess (overshoot) */
      excess=lastLevSize-refLevs*newQUsed;

    /* Set dead zone indicator */
    inDeadZone = fromDeadZone(state->partitionType);
    
    /* Set value of leftovers from last pass */
    val=state->residualValue;

    /*--- Calculate QIndex. Update residualValue and fromReduced states ---*/
    if (excess==0)
    {
      QIndex = val/newQUsed;
      if (newQUsed < newStateQ)
	SET_fromReduced(state->partitionType);
      else
	CLR_fromReduced(state->partitionType);

      
      if (QIndex)
	state->residualValue -= QIndex*newQUsed;
    }
    else
    {
      Int reducedParStart; /* Where the reduced partition starts (magnitude)
			    */
      Int reducedIdx;

      reducedParStart = newQUsed*(refLevs+excess);
      if (abs(val) >= reducedParStart)
      {
	SET_fromReduced(state->partitionType);
	QIndex = SGN(state->residualValue)*(refLevs+excess);
	state->residualValue -= QIndex*newQUsed;

	--newQUsed;
	reducedIdx = 
	  SGN(state->residualValue) * (abs(val)-reducedParStart) / newQUsed;
	QIndex += reducedIdx;
	state->residualValue -= reducedIdx*newQUsed;
      }
      else
      {
	CLR_fromReduced(state->partitionType);
	QIndex = val/newQUsed;
	state->residualValue -= QIndex*newQUsed;
      }
    }
    
    if (inDeadZone && QIndex)
    {
      /* We have the sign info so all residual values from here on in are 
	 positive */
      state->residualValue = abs(state->residualValue);
      CLR_fromDeadZone(state->partitionType);
    }
  }


  return QIndex;
}



/*--------------- INVERSE QUANTIZATION --------------------------*/

/* 
   Function:
   ---------
   initInvQuantSingleStage - Initialization of the single-stage inverse
     quantizer for a given Q index.

   Arguments:
   ----------
   quantState *state - PoInter to the state data structure.
   Int *statePrevQ - PoInter to previous quantized value state.

   Return Value:
   -------------
   <None>

   Description:
   ------------
   This must be called prior to single-stage inverse quantization. A seperate
   state structure must be kept for each value that is quantized in parallel.
   Single-stage inverse quantization is just successive calls to 
   invQuantSingleStage. For each value,  need only be called once, before
   the first call to invQuantSingleStage.
*/

Void CVTCCommon::initInvQuantSingleStage(quantState *state, Int *statePrevQ)
{
  state->residualValue = 0;
  state->partitionType = 0x2; /* fromReduced = 0 and fromDeadZone  = 1 */
  *statePrevQ          = 0;
}

#define QLEVMID(q) (((q))/2)   // 1124


/* Mapping of start of quantization level, sign, and quantization level size
   to specific value.
*/
#define GETVAL(start, sgn, q) ((start) \
			       ? ((start) + (sgn)*QLEVMID((q))) \
			       : 0)

/* 
   Function:
   ---------
   invQuantSingleStage - Single-stage inverse quantizer.

   Arguments:
   ----------
   QIndex - Quantized value for this stage.
   Q     - Quantization value. Represents desired quantization level size.
   state - State of quantizer.
   Int *statePrevQ - PoInter to previous quantized value state.
   Int updatePrevQ - 0 means don't update the statePrevQ variable. !0 means
     update it.

   Return Value:
   -------------
   Inverse quantization value for this stage.

   Description:
   ------------
   initInvQuantSingleStage must be called prior to using this function the
   first time for a given value. It will compute the new, updated value
   based on the current index and state associated with the value.
*/

Int CVTCCommon::invQuantSingleStage(Int QIndex, Int Q, quantState *state, Int *statePrevQ,
			Int updatePrevQ)
{
  Int refLevs; /* how many refinement levels in new stage */
  Int val;     /* new value to return for this stage */
  Int sgn;

  /*--------------- INITIAL QUANTIZATION STAGE -------------------*/
  if (*statePrevQ==0)
  {
    val = QIndex*Q + SGN(QIndex)*(QIndex ? QLEVMID(Q) : 0);

    /* update state */
    state->residualValue = QIndex*Q;
    CLR_fromReduced(state->partitionType);
    if (QIndex)
      CLR_fromDeadZone(state->partitionType);
    else
      SET_fromDeadZone(state->partitionType);

    if (updatePrevQ)
      *statePrevQ = Q;

    return val;
  }


  /*--------------- NON-INITIAL QUANTIZATION STAGES -------------------*/

  /* get the number of refinement levels from lastQUsed state */
  refLevs = IROUND(*statePrevQ,Q);

  /* Catch condition where there's no refinement being done.
     State information is not changed.
  */
  sgn = (state->residualValue < 0 || QIndex < 0) ? -1 : 1;      
  if (refLevs<=1)
    val = GETVAL(state->residualValue,sgn,*statePrevQ);
  else
  {
    Int inDeadZone;  /* are we still in the dead zone */
    Int lastQUsed;   /* The "real" Q value last used */
    Int lastLevSize; /* Size of quantization level used last */
    Int newQUsed, newStateQ;
    Int excess;
    Int absQIndex;

    /* Initialize the last quant value used */
    lastQUsed = *statePrevQ;
    
    /* update new Q value state */
    newStateQ = newQUsed = ICEIL(lastQUsed,refLevs);
    if (updatePrevQ)
      *statePrevQ = newStateQ;

    /* Get last level size */
    lastLevSize = lastQUsed-fromReduced(state->partitionType);

    /* check if a reduced level can span the last level */
    if (refLevs*(newQUsed-1) >= lastLevSize)
    {
      --newQUsed;

      /* might overshoot (?) but can't reduce anymore */
      excess=0;
#if 1
      if (lastLevSize-refLevs*newQUsed)
	fprintf(stderr,"Excess in reduced partition\n");
#endif
    }
    else
      /* get excess (overshoot) */
      excess=lastLevSize-refLevs*newQUsed;

    /* Set dead zone indicator */
    inDeadZone = fromDeadZone(state->partitionType);

    /*--- Calculate val. Update residualValue and fromReduced states ---*/
    absQIndex = abs(QIndex);
    if (excess==0)
    {
      if (newQUsed < newStateQ)
	SET_fromReduced(state->partitionType);
      else
	CLR_fromReduced(state->partitionType);
      state->residualValue += sgn*absQIndex*newQUsed;
    }
    else
    {
      Int reducedIdx;
      Int fullLevs;

      fullLevs = refLevs+excess;
      if (absQIndex >= fullLevs)
      {
	SET_fromReduced(state->partitionType);
	state->residualValue += sgn*fullLevs*newQUsed;

	--newQUsed;
	
	reducedIdx = absQIndex-fullLevs;
	state->residualValue += sgn*reducedIdx*newQUsed;
      }
      else
      {
	CLR_fromReduced(state->partitionType);
	state->residualValue += sgn*absQIndex*newQUsed;
      }
    }

    val = GETVAL(state->residualValue, sgn, newQUsed);
    
    if (inDeadZone && QIndex)
      CLR_fromDeadZone(state->partitionType);
  }

  return val;
}



/*--------- DERIVED QUANTIZATION VALUES AND REFINEMENT LEVELS ----------*/


/* 
   Function:
   ---------
   quantRefLev - Get the number of quantization levels for a given stage and
     the revised Q value.

   Arguments:
   ----------
   Int curQ - Current input Q value.
   Int *lastQUsed - last, revised, Q value. Will be updated.
   Int whichQ - Quantization stage.

   Return Value:
   -------------
   The number of refinement quantization levels.

   Description:
   ------------
   quantRefLev will return the number of refinement quantization levels
   at stage whichQ based on the last Q value used and the current input Q
   value. It will also update the revised Q value by overwriting the argument
   lastQUsed.
*/

Int CVTCCommon::quantRefLev(Int curQ, Int *lastQUsed, Int whichQ)
{
  Int refLevs;
  Int newQUsed;

  /* get the number of refinement levels */
  refLevs = IROUND(*lastQUsed,curQ);
    
  if (whichQ==0 || refLevs>1)
  {
    /* get new level size */
    newQUsed = ICEIL(*lastQUsed,refLevs);

    /* update the last quant value used */
    *lastQUsed = newQUsed;
  }

  return refLevs;
}



