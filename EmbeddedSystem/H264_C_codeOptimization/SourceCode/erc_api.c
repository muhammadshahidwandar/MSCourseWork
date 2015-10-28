
/*!
 *************************************************************************************
 * \file erc_api.c
 *
 * \brief
 *    External (still inside video decoder) interface for error concealment module
 *
 *  \author
 *     - Ari Hourunranta                <ari.hourunranta@nokia.com>
 *     - Viktor Varsa                     <viktor.varsa@nokia.com>
 *     - Ye-Kui Wang                   <wyk@ieee.org>
 *
 *************************************************************************************
 */


#include <stdlib.h>

#include "global.h"
#include "memalloc.h"
#include "erc_api.h"

//objectBuffer_t *erc_object_list = NULL;/*Changed by Saad Bin Shams [Removing Global Variables]*/
//ercVariables_t *erc_errorVar = NULL;	 /*Changed by Saad Bin Shams [Removing Global Variables]*/
//frame erc_recfr;						 /*Changed by Saad Bin Shams [Removing Global Variables]*/	
//int erc_mvperMB;						 /*Changed by Saad Bin Shams [Removing Global Variables]*/			

/*!
 ************************************************************************
 * \brief
 *    Initinize the error concealment module
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
  *		<saad.shams@inforient.com>
 ************************************************************************
 */

void ercInit(int flag,  h264_decoder* dec_params )
{
  ercClose(dec_params);
  //dec_params->erc_object_list = (objectBuffer_t *) calloc((dec_params->img->width * dec_params->img->height) >> 6, sizeof(objectBuffer_t));
  dec_params->erc_object_list = (objectBuffer_t *) h264_malloc(((dec_params->img->width * dec_params->img->height) >> 6) * sizeof(objectBuffer_t));
  if (dec_params->erc_object_list == NULL) 
  {
	  printf("ercInit: erc_object_list");
	  exit(0);
  }
  
  /* the error concelament instance is allocated */
  dec_params->erc_errorVar = ercOpen(dec_params);
  
  /* set error concealment ON */
  ercSetErrorConcealment(dec_params->erc_errorVar, flag);
}

/*!
 ************************************************************************
 * \brief
 *      Allocates data structures used in error concealment.
 *\return
 *      The allocated ercVariables_t is returned.
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
ercVariables_t *ercOpen( h264_decoder* dec_params )
{
  ercVariables_t *errorVar = NULL;
  
  errorVar = (ercVariables_t *)h264_malloc( sizeof(ercVariables_t));
  if ( errorVar == NULL ) 
  {
	  printf("ercOpen: errorVar");
	  exit(0);
  }

  errorVar->nOfMBs = 0;
  errorVar->segments = NULL;
  errorVar->currSegment = 0;
  errorVar->yCondition = NULL;
  errorVar->uCondition = NULL;
  errorVar->vCondition = NULL;
  errorVar->prevFrameYCondition = NULL;
  
  errorVar->concealment = 1;
  
  return errorVar;
}

/*!
 ************************************************************************
 * \brief
 *      Resets the variables used in error detection. 
 *      Should be called always when starting to decode a new frame.
 * \param errorVar
 *      Variables for error concealment
 * \param nOfMBs
 *      Number of macroblocks in a frame
 * \param numOfSegments
 *    Estimated number of segments (memory reserved)
 * \param picSizeX
 *      Width of the frame in pixels.
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

void ercReset( h264_decoder* dec_params )
{
  int *tmp = NULL;
  int i = 0;
  /*********************************************/
  /*Variables added to ease the use of same variable
  names in differnt structures i.e image & ercVariables_t*/
  int nOfMBs=dec_params->img->FrameSizeInMbs;
  int numOfSegments=dec_params->img->FrameSizeInMbs;
  /*********************************************/
  if ( dec_params->erc_errorVar && dec_params->erc_errorVar->concealment ) 
  {
    /* If frame size has been changed */
    if ( nOfMBs != dec_params->erc_errorVar->nOfMBs && dec_params->erc_errorVar->yCondition != NULL ) 
    {
      h264_free( dec_params->erc_errorVar->yCondition );
      dec_params->erc_errorVar->yCondition = NULL;
      h264_free( dec_params->erc_errorVar->prevFrameYCondition );
      dec_params->erc_errorVar->prevFrameYCondition = NULL;
      h264_free( dec_params->erc_errorVar->uCondition );
      dec_params->erc_errorVar->uCondition = NULL;
      h264_free( dec_params->erc_errorVar->vCondition );
      dec_params->erc_errorVar->vCondition = NULL;
      h264_free( dec_params->erc_errorVar->segments );
      dec_params->erc_errorVar->segments = NULL;
    }
    
    /* If the structures are uninitialized (first frame, or frame size is chaned) */
    if ( dec_params->erc_errorVar->yCondition == NULL ) 
    {
      dec_params->erc_errorVar->segments = (ercSegment_t *)h264_malloc( numOfSegments*sizeof(ercSegment_t) );

      if ( dec_params->erc_errorVar->segments == NULL )
      {
          printf("ercReset: erc_errorVar->segments");
		  exit(0);
      }
      memset( dec_params->erc_errorVar->segments, 0, numOfSegments*sizeof(ercSegment_t));

	  dec_params->erc_errorVar->nOfSegments = numOfSegments;
      
      dec_params->erc_errorVar->yCondition = (int *)h264_malloc( 4*nOfMBs*sizeof(int) );
      if ( dec_params->erc_errorVar->yCondition == NULL )
      {
          printf("ercReset: erc_errorVar->yCondition");
		  exit(0);
      }
      dec_params->erc_errorVar->prevFrameYCondition = (int *)h264_malloc( 4*nOfMBs*sizeof(int) );
      if ( dec_params->erc_errorVar->prevFrameYCondition == NULL )
      {
          printf("ercReset: erc_errorVar->prevFrameYCondition");
		  exit(0);
      }
      dec_params->erc_errorVar->uCondition = (int *)h264_malloc( nOfMBs*sizeof(int) );
      if ( dec_params->erc_errorVar->uCondition == NULL )
      {
          printf("ercReset: erc_errorVar->uCondition");
		  exit(0);
      }
      dec_params->erc_errorVar->vCondition = (int *)h264_malloc( nOfMBs*sizeof(int) );
      if ( dec_params->erc_errorVar->vCondition == NULL )
      {
          printf("ercReset: erc_errorVar->vCondition");
		  exit(0);
      }
      dec_params->erc_errorVar->nOfMBs = nOfMBs;
    }
    else 
    {
      /* Store the yCondition struct of the previous frame */
      tmp = dec_params->erc_errorVar->prevFrameYCondition;
      dec_params->erc_errorVar->prevFrameYCondition = dec_params->erc_errorVar->yCondition;
      dec_params->erc_errorVar->yCondition = tmp;
    }
    
    /* Reset tables and parameters */
    memset( dec_params->erc_errorVar->yCondition, 0, 4*nOfMBs*sizeof(*dec_params->erc_errorVar->yCondition));
    memset( dec_params->erc_errorVar->uCondition, 0,   nOfMBs*sizeof(*dec_params->erc_errorVar->uCondition));
    memset( dec_params->erc_errorVar->vCondition, 0,   nOfMBs*sizeof(*dec_params->erc_errorVar->vCondition));

    if (dec_params->erc_errorVar->nOfSegments != numOfSegments) 
    {
      h264_free( dec_params->erc_errorVar->segments );
      dec_params->erc_errorVar->segments = NULL;
      dec_params->erc_errorVar->segments = (ercSegment_t *)h264_malloc( numOfSegments*sizeof(ercSegment_t) );
      if ( dec_params->erc_errorVar->segments == NULL ) 
	  {
		  printf("ercReset: erc_errorVar->segments");
		  exit(0);
	  }
      dec_params->erc_errorVar->nOfSegments = numOfSegments;
    }
    memset( dec_params->erc_errorVar->segments, 0, dec_params->erc_errorVar->nOfSegments*sizeof(ercSegment_t));

    for( i = 0; i < dec_params->erc_errorVar->nOfSegments; i++ ) 
    {
      dec_params->erc_errorVar->segments[i].fCorrupted = 1; //! mark segments as corrupted
      dec_params->erc_errorVar->segments[i].startMBPos = 0;
      dec_params->erc_errorVar->segments[i].endMBPos = nOfMBs - 1;
    }
    
    dec_params->erc_errorVar->currSegment = 0;
    dec_params->erc_errorVar->nOfCorruptedSegments = 0;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Resets the variables used in error detection. 
 *      Should be called always when starting to decode a new frame.
 * \param errorVar
 *      Variables for error concealment
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

void ercClose( h264_decoder* dec_params )
{
  if ( dec_params->erc_errorVar != NULL ) 
  {
    if (dec_params->erc_errorVar->yCondition != NULL) 
    {
      h264_free( dec_params->erc_errorVar->segments );
      h264_free( dec_params->erc_errorVar->yCondition );
      h264_free( dec_params->erc_errorVar->uCondition );
      h264_free( dec_params->erc_errorVar->vCondition );
      h264_free( dec_params->erc_errorVar->prevFrameYCondition );
    }
    h264_free( dec_params->erc_errorVar );
    dec_params->erc_errorVar = NULL;
  }
  
  if (dec_params->erc_object_list)
  {
    h264_free(dec_params->erc_object_list);
    dec_params->erc_object_list=NULL;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Sets error concealment ON/OFF. Can be invoked only between frames, not during a frame
 * \param errorVar
 *      Variables for error concealment
 * \param value
 *      New value
 ************************************************************************
 */
void ercSetErrorConcealment( ercVariables_t *errorVar, int value )
{
  if ( errorVar != NULL )
    errorVar->concealment = value;
}

/*!
 ************************************************************************
 * \brief
 *      Creates a new segment in the segment-list, and marks the start MB and bit position.
 *      If the end of the previous segment was not explicitly marked by "ercStopSegment",
 *      also marks the end of the previous segment.
 *      If needed, it reallocates the segment-list for a larger storage place.
 * \param currMBNum
 *      The MB number where the new slice/segment starts
 * \param segment
 *      Segment/Slice No. counted by the caller
 * \param bitPos
 *      Bitstream pointer: number of bits read from the buffer.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercStartSegment( int currMBNum, int segment, u_int32 bitPos, ercVariables_t *errorVar )
{
  if ( errorVar && errorVar->concealment ) 
  {
    errorVar->currSegmentCorrupted = 0;
        
    errorVar->segments[ segment ].fCorrupted = 0;
    errorVar->segments[ segment ].startMBPos = currMBNum;
    
  }   
}

/*!
 ************************************************************************
 * \brief
 *      Marks the end position of a segment.
 * \param currMBNum
 *      The last MB number of the previous segment
 * \param segment
 *      Segment/Slice No. counted by the caller
 *      If (segment<0) the internal segment counter is used.
 * \param bitPos
 *      Bitstream pointer: number of bits read from the buffer.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercStopSegment( int currMBNum, int segment, u_int32 bitPos, ercVariables_t *errorVar )
{
  if ( errorVar && errorVar->concealment ) 
  {
    errorVar->segments[ segment ].endMBPos = currMBNum; //! Changed TO 12.11.2001
    errorVar->currSegment++;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Marks the current segment (the one which has the "currMBNum" MB in it)
 *      as lost: all the blocks of the MBs in the segment as corrupted.
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercMarkCurrSegmentLost(int32 picSizeX, ercVariables_t *errorVar )
{
  int j = 0;
  int current_segment;
  
  current_segment = errorVar->currSegment-1;
  if ( errorVar && errorVar->concealment ) 
  {
    if (errorVar->currSegmentCorrupted == 0) 
    {
      errorVar->nOfCorruptedSegments++;
      errorVar->currSegmentCorrupted = 1;
    }
     
    for( j = errorVar->segments[current_segment].startMBPos; j <= errorVar->segments[current_segment].endMBPos; j++ ) 
    {
      errorVar->yCondition[MBNum2YBlock (j, 0, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 1, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 2, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->yCondition[MBNum2YBlock (j, 3, picSizeX)] = ERC_BLOCK_CORRUPTED;
      errorVar->uCondition[j] = ERC_BLOCK_CORRUPTED;
      errorVar->vCondition[j] = ERC_BLOCK_CORRUPTED;
    }
    errorVar->segments[current_segment].fCorrupted = 1;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Marks the current segment (the one which has the "currMBNum" MB in it)
 *      as OK: all the blocks of the MBs in the segment as OK.
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercMarkCurrSegmentOK(int32 picSizeX, ercVariables_t *errorVar )
{
  int j = 0;
  int current_segment;
  
  current_segment = errorVar->currSegment-1;
  if ( errorVar && errorVar->concealment ) 
  {
    // mark all the Blocks belonging to the segment as OK */
    for( j = errorVar->segments[current_segment].startMBPos; j <= errorVar->segments[current_segment].endMBPos; j++ ) 
    {
      errorVar->yCondition[MBNum2YBlock (j, 0, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 1, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 2, picSizeX)] = ERC_BLOCK_OK;
      errorVar->yCondition[MBNum2YBlock (j, 3, picSizeX)] = ERC_BLOCK_OK;
      errorVar->uCondition[j] = ERC_BLOCK_OK;
      errorVar->vCondition[j] = ERC_BLOCK_OK;
    }
    errorVar->segments[current_segment].fCorrupted = 0;
  }
}

/*!
 ************************************************************************
 * \brief
 *      Marks the Blocks of the given component (YUV) of the current MB as concealed.
 * \param currMBNum
 *      Selects the segment where this MB number is in.
 * \param comp
 *      Component to mark (0:Y, 1:U, 2:V, <0:All)
 * \param picSizeX
 *      Width of the frame in pixels.
 * \param errorVar
 *      Variables for error detector
 ************************************************************************
 */
void ercMarkCurrMBConcealed( int currMBNum, int comp, int32 picSizeX, ercVariables_t *errorVar )
{
  int setAll = 0;
  
  if ( errorVar && errorVar->concealment ) 
  {
    if (comp < 0) 
    {
      setAll = 1;
      comp = 0;
    }
    
    switch (comp) 
    {
    case 0:
      errorVar->yCondition[MBNum2YBlock (currMBNum, 0, picSizeX)] = ERC_BLOCK_CONCEALED;
      errorVar->yCondition[MBNum2YBlock (currMBNum, 1, picSizeX)] = ERC_BLOCK_CONCEALED;
      errorVar->yCondition[MBNum2YBlock (currMBNum, 2, picSizeX)] = ERC_BLOCK_CONCEALED;
      errorVar->yCondition[MBNum2YBlock (currMBNum, 3, picSizeX)] = ERC_BLOCK_CONCEALED;
      if (!setAll)
        break;
    case 1:
      errorVar->uCondition[currMBNum] = ERC_BLOCK_CONCEALED;
      if (!setAll)
        break;
    case 2:
      errorVar->vCondition[currMBNum] = ERC_BLOCK_CONCEALED;
    }
  }
}
