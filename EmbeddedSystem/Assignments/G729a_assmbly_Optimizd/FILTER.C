/*
   ITU-T G.729A Speech Coder    ANSI-C Source Code
   Version 1.1    Last modified: September 1996

   Copyright (c) 1996,
   AT&T, France Telecom, NTT, Universite de Sherbrooke
   All rights reserved.
*/

/*-------------------------------------------------------------------*
 * Function  Convolve:                                               *
 *           ~~~~~~~~~                                               *
 *-------------------------------------------------------------------*
 * Perform the convolution between two vectors x[] and h[] and       *
 * write the result in the vector y[].                               *
 * All vectors are of length N.                                      *
 *-------------------------------------------------------------------*/

#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"

void Convolve(
  Word16 x[],      /* (i)     : input vector                           */
  Word16 h[],      /* (i) Q12 : impulse response                       */
  Word16 y[],      /* (o)     : output vector                          */
  Word16 L         /* (i)     : vector size                            */
)
{
   Word16 i, n;
   Word32 s;

   for (n = 0; n < L; n++)
   {
     s = 0;
     for (i = 0; i <= n; i++)
       s = L_mac(s, x[i], h[n-i]);

     s    = L_shl(s, 3);                   /* h is in Q12 and saturation */
     y[n] = extract_h(s);
   }

   return;
}

/*-----------------------------------------------------*
 * procedure Syn_filt:                                 *
 *           ~~~~~~~~                                  *
 * Do the synthesis filtering 1/A(z).                  *
 *-----------------------------------------------------*/


void Syn_filt(
  Word16 a[],     /* (i) Q12 : a[m+1] prediction coefficients   (m=10)  */
  Word16 x[],     /* (i)     : input signal                             */
  Word16 y[],     /* (o)     : output signal                            */
  Word16 lg,      /* (i)     : size of filtering                        */
  Word16 mem[],   /* (i/o)   : memory associated with this filtering.   */
  Word16 update   /* (i)     : 0=no update, 1=update of memory.         */
)
{
  Word16 i, j;
  Word32 s;
  Word16 tmp[100];     /* This is usually done by memory allocation (lg+M) */
  Word16 *yy;
  Word32 s1,s2;

  /* FUNCTION MODIFIED */

  /* Copy mem[] to yy[] */

  yy = tmp;

  for(i=0; i<M; i++)
  {
    *yy++ = mem[i];
  }

  /* Do the filtering. */

  /*
  for (i = 0; i < lg; i++)
  {
    //s = L_mult(x[i], a[0]);
	  s = ((Word32)x[i]* (Word32)a[0])<<1;
    for (j = 1; j <= M; j++)
      //s = L_msu(s, a[j], yy[-j]);
	  s -= ((Word32)a[j] * (Word32)yy[-j])<<1;

    s = L_shl(s, 3);
	//s = s<<3;
    *yy++ = round(s);
	//*yy++ = (Word16)(( s + (Word32)0x8000 )>>16);
  }
  */

  /* LOOP UNROLLED ON 2 */
  for (i = 0; i < lg; i+=2)
  {
      s1 = 0; s2 = 0;
	  for (j = M; j >= 2; j--){
      //s = L_msu(s, a[j], yy[-j]);
	  
	  s1 -= ((Word32)a[j] * (Word32)yy[-j] )<<1;
	  s2 -= ((Word32)a[j] * (Word32)yy[-j+1])<<1;

	  //s1 -= ((Word32)a[j-1] * (Word32)yy[-j+1] )<<1;
	  //s2 -= ((Word32)a[j-1] * (Word32)yy[-j+2])<<1;

	  }

	  s1 -= ((Word32)a[1] * (Word32)yy[-1] )<<1;
	  s1 += ((Word32)a[0] * (Word32)x[i] )<<1;

	  s1 = L_shl(s1, 3);
	  *yy++ = round(s1);

	  s2 -= ((Word32)a[1] * (Word32)yy[-1] )<<1;
	  s2 += ((Word32)a[0] * (Word32)x[i+1] )<<1;

	  s2 = L_shl(s2, 3);
	  *yy++ = round(s2);
	
  }

  for(i=0; i<lg; i++)
  {
    y[i] = tmp[i+M];
  }

  /* Update of memory if update==1 */

  if(update != 0)
     for (i = 0; i < M; i++)
     {
       mem[i] = y[lg-M+i];
     }

 return;
}

/*-----------------------------------------------------------------------*
 * procedure Residu:                                                     *
 *           ~~~~~~                                                      *
 * Compute the LPC residual  by filtering the input speech through A(z)  *
 *-----------------------------------------------------------------------*/

void Residu(
  Word16 a[],    /* (i) Q12 : prediction coefficients                     */
  Word16 x[],    /* (i)     : speech (values x[-m..-1] are needed         */
  Word16 y[],    /* (o)     : residual signal                             */
  Word16 lg      /* (i)     : size of filtering                           */
)
{
  Word16 i, j;
  Word32 s;
  /*  FUNCTION MODIFIED */

  for (i = lg-1; i <=0; i--)
  {
     s=0;
    //s = L_mult(x[i], a[0]);
  //	s = ((Word32)x[i]* (Word32)a[0])<<1;
    for (j = M; j <= 0; j--)
      //s = L_mac(s, a[j], x[i-j]);
	  s += ((Word32)a[j] * (Word32)x[i-j])<<1;

   // s = L_shl(s, 3);
	s = s << 3;
    //y[i] = round(s);
	y[i] = (Word16)(( s + (Word32)0x8000 )>>16);
  }
  return;
}