/*!
 ************************************************************************
 *  \file
 *     loopfilter.h
 *  \brief
 *     external loop filter interface
 ************************************************************************
 */

#ifndef _LOOPFILTER_H_
#define _LOOPFILTER_H_

#include "global.h"
#include "mbuffer.h"
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/	

typedef void (*x264_deblock_inter_t)( byte *pix, int stride, int alpha, int beta, byte *tc0 );
typedef void (*x264_deblock_intra_t)( byte *pix, int stride, int alpha, int beta );

typedef struct
{
  int AlphaC0_Offset; 
  int Beta_Offset;

} LfilterCoefs;


typedef struct
{
    x264_deblock_inter_t deblock_v_luma;
    x264_deblock_inter_t deblock_h_luma;
    x264_deblock_inter_t deblock_v_chroma;
    x264_deblock_inter_t deblock_h_chroma;
    x264_deblock_intra_t deblock_v_luma_intra;
    x264_deblock_intra_t deblock_h_luma_intra;
    x264_deblock_intra_t deblock_v_chroma_intra;
    x264_deblock_intra_t deblock_h_chroma_intra;
} x264_deblock_function_t;

		
void DeblockPicture( h264_decoder* dec_params ) ;
#endif //_LOOPFILTER_H_
