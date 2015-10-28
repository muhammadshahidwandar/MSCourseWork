
/*!
 ************************************************************************
 * \file  nalucommon.c
 *
 * \brief
 *    Common NALU support functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Stephan Wenger   <stewe@cs.tu-berlin.de>
 ************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "global.h"
#include "nalu.h"
#include "memalloc.h"


/*! 
 *************************************************************************************
 * \brief
 *    Allocates memory for a NALU
 *
 * \param buffersize
 *     size of NALU buffer 
 *
 * \return
 *    pointer to a NALU
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 *************************************************************************************
 */
 

NALU_t *AllocNALU(int buffersize,  h264_decoder* dec_params)
{
  NALU_t *n;

  if ((n = (NALU_t*)h264_malloc (1 * sizeof (NALU_t))) == NULL)
  {
	  printf("AllocNALU: n");
	  exit(0);
  }

  n->max_size=buffersize;

  if ((n->buf = (byte*)h264_malloc (buffersize * sizeof (byte))) == NULL)
  {
	  printf("AllocNALU: n->buf");
	  exit(0);
  }
  
  return n;
}


/*! 
 *************************************************************************************
 * \brief
 *    Frees a NALU
 *
 * \param n 
 *    NALU to be freed
 *
 *************************************************************************************
 */

void FreeNALU(NALU_t *n)
{
  if (n)
  {
    if (n->buf)
    {
      h264_free(n->buf);
      n->buf=NULL;
    }
    h264_free (n);
  }
}

