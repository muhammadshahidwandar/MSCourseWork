/*
   ITU-T G.729A Speech Coder    ANSI-C Source Code
   Version 1.1    Last modified: September 1996

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke
   All rights reserved.
*/

/*-------------------------------------------------------------------*
 * Function  Pred_lt_3()                                             *
 *           ~~~~~~~~~~~                                             *
 *-------------------------------------------------------------------*
 * Compute the result of long term prediction with fractional        *
 * interpolation of resolution 1/3.                                  *
 *                                                                   *
 * On return exc[0..L_subfr-1] contains the interpolated signal      *
 *   (adaptive codebook excitation)                                  *
 *-------------------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"
#include "tab_ld8a.h"

extern Word16 asmADD16bit(Word16 var1 , Word16 var2 );

void Pred_lt_3(
  Word16   exc[],       /* in/out: excitation buffer */
  Word16   T0,          /* input : integer pitch lag */
  Word16   frac,        /* input : fraction of lag   */
  Word16   L_subfr      /* input : subframe size     */
)
{
  Word16   i, j, k;
  Word16   *x0, *x1, *x2, *c1, *c2;
  Word32  s;

  x0 = &exc[-T0];

  frac = negate(frac);
  //frac = -(frac);
  if (frac < 0)
  {
    //frac = add(frac, UP_SAMP);
	frac = asmADD16bit(frac, UP_SAMP);
    x0--;
  }

  for (j=0; j<L_subfr; j++)
  {
    x1 = x0++;
    x2 = x0;
    c1 = &inter_3l[frac];
    c2 = &inter_3l[sub(UP_SAMP,frac)];

    s = 0;
    for(i=0, k=0; i< L_INTER10; i++, k+=UP_SAMP)
    {
       s = L_mac(s, x1[-i], c1[k]);
  		//s += (((Word32)x1[-i]*(Word32)c1[k])<<1);
      s = L_mac(s, x2[i],  c2[k]);
  	    //s += (((Word32)x2[i]*(Word32)c2[k])<<1);
    }

    exc[j] = round(s);
  }

  return;
}

