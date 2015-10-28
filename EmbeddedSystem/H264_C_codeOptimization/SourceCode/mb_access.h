
/*!
 *************************************************************************************
 * \file mb_access.h
 *
 * \brief
 *    Functions for macroblock neighborhoods
 *
 * \author
 *     Main contributors (see contributors.h for copyright, address and affiliation details)
 *     - Karsten Sühring          <suehring@hhi.de>
 *************************************************************************************
 */

#ifndef _MB_ACCESS_H_
#define _MB_ACCESS_H_
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void CheckAvailabilityOfNeighbors_baseline(  ImageParameters* img);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void getNeighbour(int curr_mb_nr, int xN, int yN, int luma, PixelPos *pix,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void getLuma4x4Neighbour (int block_x, int block_y, int rel_x, int rel_y, PixelPos *pix,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void getChroma4x4Neighbour (int curr_mb_nr, int block_x, int block_y, int rel_x, int rel_y, PixelPos *pix,   h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
//int  mb_is_available(int mbAddr, int currMbAddr,  H264Dec_Outputs* dec_outputs,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void get_mb_pos_baseline (int mb_addr, int *x, int*y,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void get_mb_block_pos (int mb_addr, int *x, int*y);



#endif
