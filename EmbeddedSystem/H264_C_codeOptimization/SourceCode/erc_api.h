
/*!
 ************************************************************************
 * \file  erc_api.h
 *
 * \brief
 *      External (still inside video decoder) interface for error concealment module
 *
 * \author
 *      - Ari Hourunranta                <ari.hourunranta@nokia.com>
 *      - Ye-Kui Wang                   <wyk@ieee.org>
 *
 ************************************************************************
 */


#ifndef _ERC_API_H_
#define _ERC_API_H_

#include "erc_globals.h"

/*
* Defines
*/

/* If the average motion vector of the correctly received macroblocks is less than the 
threshold, concealByCopy is used, otherwise concealByTrial is used. */
#define MVPERMB_THR 8 

/* used to determine the size of the allocated memory for a temporal Region (MB) */
#define DEF_REGION_SIZE 384  /* 8*8*6 */ 

#define ERC_BLOCK_OK                3
#define ERC_BLOCK_CONCEALED         2
#define ERC_BLOCK_CORRUPTED         1
#define ERC_BLOCK_EMPTY             0

#define mabs(a) ( (a) < 0 ? -(a) : (a) )
#define mmax(a,b) ((a) > (b) ? (a) : (b))
#define mmin(a,b) ((a) < (b) ? (a) : (b))

/*
* Functions to convert MBNum representation to blockNum
*/

#define xPosYBlock(currYBlockNum,picSizeX) \
((currYBlockNum)%((picSizeX)>>3))

#define yPosYBlock(currYBlockNum,picSizeX) \
((currYBlockNum)/((picSizeX)>>3))

#define xPosMB(currMBNum,picSizeX) \
((currMBNum)%((picSizeX)>>4))

#define yPosMB(currMBNum,picSizeX) \
((currMBNum)/((picSizeX)>>4))

#define MBxy2YBlock(currXPos,currYPos,comp,picSizeX) \
((((currYPos)<<1)+((comp)>>1))*((picSizeX)>>3)+((currXPos)<<1)+((comp)&1))

#define MBNum2YBlock(currMBNum,comp,picSizeX) \
MBxy2YBlock(xPosMB((currMBNum),(picSizeX)),yPosMB((currMBNum),(picSizeX)),(comp),(picSizeX))


/*
* typedefs
*/




/*
* External function interface
*/
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com>
 *-----------------------------------------------------------------------*/			
void ercInit(int flag,  h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com>
 *-----------------------------------------------------------------------*/			
ercVariables_t *ercOpen( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com>
 *-----------------------------------------------------------------------*/			
void ercReset( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params
 *   <saad.shams@inforient.com>
 *-----------------------------------------------------------------------*/			
//void ercClose( ercVariables_t *errorVar ,h264_decoder* dec_params);
void ercClose( h264_decoder* dec_params );

void ercSetErrorConcealment( ercVariables_t *errorVar, int value );
void ercStartSegment( int currMBNum, int segment, u_int32 bitPos, ercVariables_t *errorVar );
void ercStopSegment( int currMBNum, int segment, u_int32 bitPos, ercVariables_t *errorVar );
void ercMarkCurrSegmentLost(int32 picSizeX, ercVariables_t *errorVar );
void ercMarkCurrSegmentOK(int32 picSizeX, ercVariables_t *errorVar );
void ercMarkCurrMBConcealed( int currMBNum, int comp, int32 picSizeX, ercVariables_t *errorVar );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params
 *   <saad.shams@inforient.com>
 *-----------------------------------------------------------------------*/			
int ercConcealIntraFrame( frame *recfr, int32 picSizeX, int32 picSizeY, ercVariables_t *errorVar ,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com>
 *-----------------------------------------------------------------------*/			
int ercConcealInterFrame( frame *recfr, objectBuffer_t *object_list,int32 picSizeX, 
						 int32 picSizeY, ercVariables_t *errorVar, h264_decoder* dec_params);

#endif

