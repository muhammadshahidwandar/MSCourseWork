
/*!
 ************************************************************************
 * \file  nalu.c
 *
 * \brief
 *    Decoder NALU support functions
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Stephan Wenger   <stewe@cs.tu-berlin.de>
 ************************************************************************
 */

#include <assert.h>

#include "global.h"
#include "nalu.h"




/*! 
 *************************************************************************************
 * \brief
 *    Converts a NALU to an RBSP
 *
 * \param 
 *    nalu: nalu structure to be filled
 *
 * \return
 *    length of the RBSP in bytes
 *************************************************************************************
 */

int NALUtoRBSP (NALU_t *nalu)
{
  int i, j, count;
  int begin_bytepos , end_bytepos;
  unsigned int copyflag = 0;
  count = 0;

  assert (nalu != NULL);
  //nalu->len = EBSPtoRBSP (nalu->buf, nalu->len, 1) ; 

  begin_bytepos = 1;
  end_bytepos   = nalu->len;
  
  if(end_bytepos < begin_bytepos)
    return end_bytepos;
  
  j = begin_bytepos;
  
  //for(i = begin_bytepos; i < end_bytepos; i++) 
  //{ //starting from begin_bytepos to avoid header information
  //  if(count == ZEROBYTES_SHORTSTARTCODE && nalu->buf[i] == 0x03) 
  //  {
  //    i++;
  //    count = 0;
  //  }
  //  nalu->buf[j] = nalu->buf[i];
  //  if(nalu->buf[i] == 0x00)
  //    count++;
  //  else
  //    count = 0;
  //  j++;
  //}

  for(i = begin_bytepos; i < end_bytepos; i++) 
  { //starting from begin_bytepos to avoid header information
    if( nalu->buf[i] == 0x00) 
    {
      if( nalu->buf[i+1] == 0x00) 
	  {
		if( nalu->buf[i+2] == 0x03) 
		{
			j = i + 2;
			begin_bytepos = i+3;
			copyflag = 1;
			break;
		}
	  }      
    }
    //nalu->buf[j] = nalu->buf[i];
    //if(nalu->buf[i] == 0x00)
    //  count++;
    //else
    //  count = 0;
    j++;
  }
  
  if (copyflag) 
  {
	  //j = begin_bytepos;
	  for(i = begin_bytepos; i < end_bytepos; i++) 
	  { //starting from begin_bytepos to avoid header information
		  if( nalu->buf[i] == 0x00) 
		  {
			  if( nalu->buf[i+1] == 0x00) 
			  {
				  if( nalu->buf[i+2] == 0x03) 
				  {
					  //begin_bytepos = i + 2;
					  nalu->buf[j++] = nalu->buf[i++];
					  nalu->buf[j++] = nalu->buf[i++];
					  i++;					  
				  }
			  }      
		  }
		  nalu->buf[j] = nalu->buf[i];
		  //if(nalu->buf[i] == 0x00)
		  //  count++;
		  //else
		  //  count = 0;
		  j++;
	  }
  }

  nalu->len = j;

  return nalu->len ;
}

