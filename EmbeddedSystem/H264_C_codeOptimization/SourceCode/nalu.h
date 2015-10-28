
/*!
 **************************************************************************************
 * \file
 *    parset.h
 * \brief
 *    Picture and Sequence Parameter Sets, encoder operations
 *    This code reflects JVT version xxx
 *  \date 25 November 2002
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details) 
 *      - Stephan Wenger        <stewe@cs.tu-berlin.de>
 ***************************************************************************************
 */


#ifndef _NALU_H_
#define _NALU_H_

#include <stdio.h>
#include "nalucommon.h"
#include "global.h"

//extern FILE *bits;

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *					   - H264Dec_Inputs* dec_inputs
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int GetAnnexbNALU (NALU_t *nalu,h264_decoder* dec_params);
int NALUtoRBSP (NALU_t *nalu);

#endif
