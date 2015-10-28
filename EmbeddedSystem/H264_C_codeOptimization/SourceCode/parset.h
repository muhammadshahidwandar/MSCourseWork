
/*!
 **************************************************************************************
 * \file
 *    parset.h
 * \brief
 *    Picture and Sequence Parameter Sets, decoder operations
 *    This code reflects JVT version xxx
 * \date 25 November 2002
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details) 
 *      - Stephan Wenger        <stewe@cs.tu-berlin.de>
 ***************************************************************************************
 */
#ifndef _PARSET_H_
#define _PARSET_H_

#include "global.h"
#include "parsetcommon.h"
#include "nalucommon.h"

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void Scaling_List(int *scalingList, int sizeOfScalingList, Boolean *UseDefaultScalingMatrix, 
				  Bitstream *s, h264_decoder* dec_params);

void InitVUI(seq_parameter_set_rbsp_t *sps);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  ReadVUI(DataPartition *p, seq_parameter_set_rbsp_t *sps, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  ReadHRDParameters(DataPartition *p, hrd_parameters_t *hrd, h264_decoder* dec_params);

void PPSConsistencyCheck (pic_parameter_set_rbsp_t *pps);
void SPSConsistencyCheck (seq_parameter_set_rbsp_t *sps);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void MakePPSavailable (int id, pic_parameter_set_rbsp_t *pps,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void MakeSPSavailable (int id, seq_parameter_set_rbsp_t *sps,h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void ProcessSPS_baseline(NALU_t *nalu, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void ProcessPPS_baseline (NALU_t *nalu,  h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void UseParameterSet_baseline( int PicParsetId,   h264_decoder* dec_params );

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void activate_sps (seq_parameter_set_rbsp_t *sps,h264_decoder* dec_params);
void activate_sps_baseline(seq_parameter_set_rbsp_t *sps,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void activate_pps_baseline(pic_parameter_set_rbsp_t *pps ,   h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
pic_parameter_set_rbsp_t* AllocPPS ();
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
seq_parameter_set_rbsp_t* AllocSPS ();

void FreePPS (pic_parameter_set_rbsp_t *pps);
void FreeSPS (seq_parameter_set_rbsp_t *sps);

#endif
