

/*!
 ****************************************************************************
 * \file errorconcealment.h
 *
 * \brief
 *    Header file for errorconcealment.c
 *
 ****************************************************************************
 */

#ifndef _ERRORCONCEALMENT_H_
#define _ERRORCONCEALMENT_H_
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int set_ec_flag(int se,h264_decoder*dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void reset_ec_flags(h264_decoder*dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int get_concealed_element(SyntaxElement *sym,h264_decoder*dec_params);

#endif

