
/*!
 *************************************************************************************
 * \file annexb.c
 *
 * \brief
 *    Annex B Byte Stream format
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *************************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "annexb.h"
#include "memalloc.h"

static int FindStartCode (unsigned char *Buf, int zeros_in_startcode);

int GetAnnexbNALU( NALU_t *nalu,  h264_decoder* dec_params )
{
  
  int info2, info3, pos = 0;
  int StartCodeFound, rewind;
  unsigned char *Buf;
  int LeadingZero8BitsCount=0, TrailingZero8Bits=0;
  
 
    if ((Buf = (unsigned char*) h264_malloc (nalu->max_size * sizeof(char))) == NULL)
    {
		printf("GetAnnexbNALU: Buf");
		exit(0);
    }
    
    while(!feof(dec_params->bits_Annexb) && (Buf[pos++]=fgetc(dec_params->bits_Annexb))==0);
  
  //do
  //{
  //	Buf[pos] = fgetc(dec_params->bits_Annexb);
  //	pos++;
  //}
  //while( !feof(dec_params->bits_Annexb) && (Buf[pos-1]==0) );
  		
  if(feof(dec_params->bits_Annexb))
  {
        if(pos==0)
        {
            h264_free(Buf);
            return 0;
        }
    else
    {
      printf( "GetAnnexbNALU can't read start code\n");
      h264_free(Buf);
      return -1;
    }
  }

  if(Buf[pos-1]!=1)
  {
    printf ("GetAnnexbNALU: no Start Code at the begin of the NALU, return -1\n");
    h264_free(Buf);
    return -1;
  }

  if(pos<3)
  {
    printf ("GetAnnexbNALU: no Start Code at the begin of the NALU, return -1\n");
    h264_free(Buf);
    return -1;
  }
  else if(pos==3)
  {
    nalu->startcodeprefix_len = 3;
    LeadingZero8BitsCount = 0;
  }
  else
  {
    LeadingZero8BitsCount = pos-4;
    nalu->startcodeprefix_len = 4;
  }

  //the 1st byte stream NAL unit can has leading_zero_8bits, but subsequent ones are not
  //allowed to contain it since these zeros(if any) are considered trailing_zero_8bits
  //of the previous byte stream NAL unit.
  if(!dec_params->IsFirstByteStreamNALU && LeadingZero8BitsCount>0)
  {
    printf ("GetAnnexbNALU: The leading_zero_8bits syntax can only be present in the first byte stream NAL unit, return -1\n");
    h264_free(Buf);
    return -1;
  }
  dec_params->IsFirstByteStreamNALU=0;

  StartCodeFound = 0;
  info2 = 0;
  info3 = 0;

  while (!StartCodeFound)
  {
    if (feof (dec_params->bits_Annexb))
    {
      //Count the trailing_zero_8bits
      while(Buf[pos-2-TrailingZero8Bits]==0)
            {
                TrailingZero8Bits++;
            }
      nalu->len = (pos-1)-nalu->startcodeprefix_len-LeadingZero8BitsCount-TrailingZero8Bits;
      memcpy(nalu->buf, &Buf[LeadingZero8BitsCount+nalu->startcodeprefix_len], nalu->len);     
      nalu->forbidden_bit = (nalu->buf[0]>>7) & 1;
      nalu->nal_reference_idc = (nalu->buf[0]>>5) & 3;
      nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;

// printf ("GetAnnexbNALU, eof case: pos %d nalu->len %d, nalu->reference_idc %d, nal_unit_type %d \n", pos, nalu->len, nalu->nal_reference_idc, nalu->nal_unit_type);

#if TRACE
//  fprintf (dec_params->p_trace, "\n\nLast NALU in File\n\n");
//  fprintf (dec_params->p_trace, "Annex B NALU w/ %s startcode, len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n",
//    nalu->startcodeprefix_len == 4?"long":"short", nalu->len, nalu->forbidden_bit, nalu->nal_reference_idc, nalu->nal_unit_type);
//  fflush (dec_params->p_trace);

  fprintf (p_trace, "\n\nLast NALU in File\n\n");
  fprintf (p_trace, "Annex B NALU w/ %s startcode, len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n",
    nalu->startcodeprefix_len == 4?"long":"short", nalu->len, nalu->forbidden_bit, nalu->nal_reference_idc, nalu->nal_unit_type);
  fflush (p_trace);

#endif
      h264_free(Buf);
      return pos-1;
    }

    Buf[pos++] = fgetc (dec_params->bits_Annexb);
	//Buf[pos] = fgetc (dec_params->bits_Annexb);
	//pos++;

	// Scan for Start Code
    //info3 = FindStartCode(&Buf[pos-4], 3);
    //if(info3 != 1)
    //  info2 = FindStartCode(&Buf[pos-3], 2);

	info2 = info3 = 0;
	if (Buf[pos-1] == 1)
	{		
		if(pos >= 3)
		{
			// START CODE = 001 
			if ( (Buf[pos-2] == 0) && (Buf[pos-3] == 0) )
			{
				info2 = 1;
				// START CODE = 0001 
				if (  Buf[pos-4] == 0 )
				{
					info3 = 1;
				}
			}
			
		}		
	}    

    //StartCodeFound = (info2 == 1 || info3 == 1);
	StartCodeFound = (info2  || info3 );
  }

  //Count the trailing_zero_8bits
  if(info3==1)	//if the detected start code is 00 00 01, trailing_zero_8bits is sure not to be present
  {
    while(Buf[pos-5-TrailingZero8Bits]==0)
      TrailingZero8Bits++;
  }
  // Here, we have found another start code (and read length of startcode bytes more than we should
  // have.  Hence, go back in the file
  rewind = 0;
  if(info3 == 1)
    rewind = -4;
  else if (info2 == 1)
    rewind = -3;
  else
    printf(" Panic: Error in next start code search \n");

  if (0 != fseek (dec_params->bits_Annexb, rewind, SEEK_CUR))
  {
    //snprintf (dec_params->errortext, ET_SIZE, "GetAnnexbNALU: Cannot fseek %d in the bit stream file", rewind);
    printf ("GetAnnexbNALU: Cannot fseek %d in the bit stream file\n", rewind);
    h264_free(Buf);
    //error(dec_params->errortext, 600,dec_params,dec_outputs);
	printf("GetAnnexbNALU: Cannot fseek %d in the bit stream file\n");
  }

  // Here the leading zeros(if any), Start code, the complete NALU, trailing zeros(if any)
  // and the next start code is in the Buf.
  // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
  // start code, and (pos+rewind)-startcodeprefix_len-LeadingZero8BitsCount-TrailingZero8Bits
  // is the size of the NALU.

  nalu->len = (pos+rewind)-nalu->startcodeprefix_len-LeadingZero8BitsCount-TrailingZero8Bits;
  
  memcpy (nalu->buf, &Buf[LeadingZero8BitsCount+nalu->startcodeprefix_len], nalu->len);
  
  nalu->forbidden_bit = (nalu->buf[0]>>7) & 1;
  nalu->nal_reference_idc = (nalu->buf[0]>>5) & 3;
  nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;


//printf ("GetAnnexbNALU, regular case: pos %d nalu->len %d, nalu->reference_idc %d, nal_unit_type %d \n", pos, nalu->len, nalu->nal_reference_idc, nalu->nal_unit_type);
#if TRACE
  //fprintf (dec_params->p_trace, "\n\nAnnex B NALU w/ %s startcode, len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n",
  //  nalu->startcodeprefix_len == 4?"long":"short", nalu->len, nalu->forbidden_bit, nalu->nal_reference_idc, nalu->nal_unit_type);
  //fflush (dec_params->p_trace);

  fprintf (p_trace, "\n\nAnnex B NALU w/ %s startcode, len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n",
    nalu->startcodeprefix_len == 4?"long":"short", nalu->len, nalu->forbidden_bit, nalu->nal_reference_idc, nalu->nal_unit_type);
  fflush (p_trace);
#endif
  
  h264_free(Buf);
 
  return (pos+rewind);
}
//*/

/***********************************************************************************\
* FUNCTION     : OpenBitstreamFile.                                                 *
*-----------------------------------------------------------------------------------*
* DISCRIPTION  : Opens the bit stream file named fn.                                *
*-----------------------------------------------------------------------------------*
* ARGUMENTS    : Bitstream File,                                                    *
*              : Decoder Parameters,                                                *
*              : Decoder Inputs,                                                    *
*              : Decoder Outputs.                                                   *
*-----------------------------------------------------------------------------------*
* RETURN VALUE : NONE                                                               *
*-----------------------------------------------------------------------------------*
* OUTPUT       : Error message if file cannot be opened.                            *
*-----------------------------------------------------------------------------------*
* CHANGE LOG   : Below are the changes done to the function                         *
*-----------------------------------------------------------------------------------*
* DATE         :                                                                    *
*-----------------------------------------------------------------------------------*
*              : Function Argument List Changed [Removing Global Variables]         *
*              : Input parameters added are                                         *
*              : - h264_decoder* dec_params                                       *
*              : - H264Dec_Inputs* dec_inputs                                       *
*                                                                                   *
*              : <saad.shams@inforient.com>                                         *
\***********************************************************************************/
void OpenBitstreamFile (char *fn, h264_decoder* dec_params)
{
	if (NULL == (dec_params->bits_Annexb = fopen(fn, "rb")))
	{
		printf ("Cannot open Annex B ByteStream file '%s'\n", dec_params->input->infile);
		exit(0);
	}  
}

/***********************************************************************************\
* FUNCTION     : CloseBitstreamFile.                                                *
*-----------------------------------------------------------------------------------*
* DISCRIPTION  : Closes the bit stream file.                                        *
*-----------------------------------------------------------------------------------*
* ARGUMENTS    : Decoder Inputs.                                                    *
*-----------------------------------------------------------------------------------*
* RETURN VALUE : NONE                                                               *
*-----------------------------------------------------------------------------------*
* OUTPUT       : NONE                                                               *
*-----------------------------------------------------------------------------------*
* CHANGE LOG   : Below are the changes done to the function                         *
*-----------------------------------------------------------------------------------*
* DATE         :                                                                    *
*-----------------------------------------------------------------------------------*
*              : Function Argument List Changed [Removing Global Variables]         *
*              : Input parameters added are                                         *
*              : - H264Dec_Inputs* dec_inputs                                       *
*                                                                                   *
*              : <saad.shams@inforient.com>                                         *
\***********************************************************************************/
void CloseBitstreamFile(h264_decoder* dec_params)
{
  fclose (dec_params->bits_Annexb);
}

/***********************************************************************************\
* FUNCTION     : FindStartCode.                                                     *
*-----------------------------------------------------------------------------------*
* DISCRIPTION  : returns if new start code is found at byte aligned position buf.   *
*              : new-startcode is of form N 0x00 bytes, followed by a 0x01 byte.    *
*-----------------------------------------------------------------------------------*
* ARGUMENTS    : Data Buffer,                                                       *
*              : Leading zeros in Start Code.                                       *
*-----------------------------------------------------------------------------------*
* RETURN VALUE : Start Code found return 1                                          *
*              : No Sart Code id indicated by a 0                                   *
*-----------------------------------------------------------------------------------*
* OUTPUT       : NONE                                                               *
*-----------------------------------------------------------------------------------*
* CHANGE LOG   : Below are the changes done to the function                         *
*-----------------------------------------------------------------------------------*
* DATE         :                                                                    *
*-----------------------------------------------------------------------------------*
*                                                                                   *
\***********************************************************************************/
static int FindStartCode (unsigned char *Buf, int zeros_in_startcode)
{
  int info;
  int i;

  info = 1;
  for (i = 0; i < zeros_in_startcode; i++)
    if(Buf[i] != 0)
      info = 0;

  if(Buf[i] != 1)
    info = 0;
  return info;
}

/***********************************************************************************\
* FUNCTION     : CheckZeroByteNonVCL.                                               *
*-----------------------------------------------------------------------------------*
* DISCRIPTION  : check the possibility of the current NALU to be the start of a new *
*              : access unit, according to 7.4.1.2.3                                *
*-----------------------------------------------------------------------------------*
* ARGUMENTS    : NAL unit,                                                          *
*              : Decoder Parameters.                                                *
*-----------------------------------------------------------------------------------*
* RETURN VALUE : NONE.                                                              *
*-----------------------------------------------------------------------------------*
* OUTPUT       : NONE                                                               *
*-----------------------------------------------------------------------------------*
* CHANGE LOG   : Below are the changes done to the function                         *
*-----------------------------------------------------------------------------------*
* DATE         :                                                                    *
*-----------------------------------------------------------------------------------*
*              : Function Argument List Changed [Removing Global Variables]         *
*              : Input parameters added are                                         *
*              : - H264Dec_Inputs* dec_params                                       *
*                                                                                   *
*              : <saad.shams@inforient.com>                                         *
\***********************************************************************************/
void CheckZeroByteNonVCL(NALU_t *nalu,  h264_decoder* dec_params)
{
  int CheckZeroByte=0;

  //This function deals only with non-VCL NAL units
  if(nalu->nal_unit_type>=1&&nalu->nal_unit_type<=5)
    return;

  //for SPS and PPS, zero_byte shall exist
  if(nalu->nal_unit_type==NALU_TYPE_SPS || nalu->nal_unit_type==NALU_TYPE_PPS)
    CheckZeroByte=1;
  //check the possibility of the current NALU to be the start of a new access unit, according to 7.4.1.2.3
  if(nalu->nal_unit_type==NALU_TYPE_AUD  || nalu->nal_unit_type==NALU_TYPE_SPS ||
     nalu->nal_unit_type==NALU_TYPE_PPS || nalu->nal_unit_type==NALU_TYPE_SEI ||
    (nalu->nal_unit_type>=13 && nalu->nal_unit_type<=18))
  {
    if(dec_params->LastAccessUnitExists)
    {
      dec_params->LastAccessUnitExists=0;		//deliver the last access unit to decoder
      dec_params->NALUCount=0;
    }
  }
  dec_params->NALUCount++;
  //for the first NAL unit in an access unit, zero_byte shall exists
  if(dec_params->NALUCount==1)
    CheckZeroByte=1;
  if(CheckZeroByte && nalu->startcodeprefix_len==3)
  {
    printf("warning: zero_byte shall exist\n");
    //because it is not a very serious problem, we may not indicate an error by setting ret to -1
    //*ret=-1;
  }
}

/***********************************************************************************\
* FUNCTION     : CheckZeroByteVCL.                                                  *
*-----------------------------------------------------------------------------------*
* DISCRIPTION  : Checks for the first VCL NAL unit that is the first NAL unit after *
*              : last VCL NAL unit indicates the start of a new access unit and     *
*              : hence the first NAL unit of the new access unit.                   *
*-----------------------------------------------------------------------------------*
* ARGUMENTS    : NAL unit,                                                          *
*              : Decoder Parameters.                                                *
*-----------------------------------------------------------------------------------*
* RETURN VALUE : NONE.                                                              *
*-----------------------------------------------------------------------------------*
* OUTPUT       : NONE                                                               *
*-----------------------------------------------------------------------------------*
* CHANGE LOG   : Below are the changes done to the function                         *
*-----------------------------------------------------------------------------------*
* DATE         :                                                                    *
*-----------------------------------------------------------------------------------*
*              : Function Argument List Changed [Removing Global Variables]         *
*              : Input parameters added are                                         *
*              : - H264Dec_Inputs* dec_params                                       *
*                                                                                   *
*              : <saad.shams@inforient.com>                                         *
\***********************************************************************************/
void CheckZeroByteVCL(NALU_t *nalu,  h264_decoder* dec_params)
{
  int CheckZeroByte=0;

  //This function deals only with VCL NAL units
  if(!(nalu->nal_unit_type>=1&&nalu->nal_unit_type<=5))
  {
    return;
  }

  if(dec_params->LastAccessUnitExists)
  {
    dec_params->NALUCount=0;
  }
  dec_params->NALUCount++;
  //the first VCL NAL unit that is the first NAL unit after last VCL NAL unit indicates 
  //the start of a new access unit and hence the first NAL unit of the new access unit.						(sounds like a tongue twister :-)
  if(dec_params->NALUCount==1)
    CheckZeroByte=1;
  dec_params->LastAccessUnitExists=1;
  if(CheckZeroByte && nalu->startcodeprefix_len==3)
  {
	  printf("warning: zero_byte shall exist\n");
	  //because it is not a very serious problem, we may not indicate an error by setting ret to -1
	  //*ret=-1;
  }
}
