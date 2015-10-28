/*!
 *************************************************************************************
 * \file annexb.h
 *
 * \brief
 *    Annex B byte stream buffer handling.
 *
 *************************************************************************************
 */

#ifndef _ANNEXB_H_
#define _ANNEXB_H_

//#include "nalucommon.h"
#include "global.h"

//extern int IsFirstByteStreamNALU;/*Changed by Saad Bin Shams [Removing Global Variables]*/
//extern int LastAccessUnitExists; /*Changed by Saad Bin Shams [Removing Global Variables]*/
//extern int NALUCount;			   /*Changed by Saad Bin Shams [Removing Global Variables]*/	

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: h264_decoder* dec_params,
 *-----------------------------------------------------------------------*/			

int  GetAnnexbNALU( NALU_t *nalu,  h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: h264_decoder* dec_params,
 *-----------------------------------------------------------------------*/			

void OpenBitstreamFile (char *fn,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: H264Dec_Inputs* dec_inputs,
 *-----------------------------------------------------------------------*/			

void CloseBitstreamFile(h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: h264_decoder* dec_params,
 *-----------------------------------------------------------------------*/			
//void CheckZeroByteNonVCL(NALU_t *nalu, int * ret,  h264_decoder* dec_params);
void CheckZeroByteNonVCL(NALU_t *nalu,  h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: h264_decoder* dec_params,
 *-----------------------------------------------------------------------*/			
//void CheckZeroByteVCL(NALU_t *nalu, int * ret,  h264_decoder* dec_params);
void CheckZeroByteVCL(NALU_t *nalu,  h264_decoder* dec_params);

#endif

