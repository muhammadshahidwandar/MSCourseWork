
/*!
 ************************************************************************
 * \file image.h
 *
 * \brief
 *    prototypes for image.c
 *
 ************************************************************************
 */

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "mbuffer.h"

extern const unsigned char allowed_pixel_value[512];

//extern StorablePicture *dec_picture;

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void find_snr( StorablePicture *p,     //!< picture to be compared
               h264_decoder* dec_params );

int  picture_order(ImageParameters *img);
void set_ref_pic_num_baseline(  h264_decoder* dec_params);
void ercWriteMBMODEandMV_baseline( h264_decoder* dec_params );
void exit_slice_baseline(  h264_decoder* dec_params);

#endif

