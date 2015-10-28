
/*!
 **************************************************************************************
 * \file
 *    output.h
 * \brief
 *    Picture writing routine headers
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details) 
 *      - Karsten Suehring        <suehring@hhi.de>
 ***************************************************************************************
 */
#ifndef _OUTPUT_H_
#define _OUTPUT_H_

int testEndian();

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
//void write_stored_frame(FrameStore *fs, int p_out,  h264_decoder* dec_params);
void write_stored_frame(FrameStore *fs, FILE *f_out,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void direct_output(h264_decoder* dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *					   - H264Dec_Inputs* dec_inputs
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
//void init_out_buffer(  h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			

#ifdef PAIR_FIELDS_IN_OUTPUT
void flush_pending_output(int p_out);
#endif

#endif //_OUTPUT_H_
