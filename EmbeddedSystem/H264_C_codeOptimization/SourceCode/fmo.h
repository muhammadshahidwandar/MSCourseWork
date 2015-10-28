
/*!
 ***************************************************************************
 *
 * \file fmo.h
 *
 * \brief
 *    Support for Flexilble Macroblock Ordering (FMO)
 *
 * \date
 *    19 June, 2002
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 **************************************************************************/

#ifndef _FMO_H_
#define _FMO_H_


/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int FmoInit( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int FmoFinit ( h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int FmoGetNumberOfSliceGroup(h264_decoder*dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int FmoGetLastMBOfPicture( h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int FmoGetLastMBInSliceGroup(int SliceGroup, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int FmoGetSliceGroupId (int mb, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int FmoGetNextMBNr (int CurrentMbNr, h264_decoder* dec_params);

#endif
