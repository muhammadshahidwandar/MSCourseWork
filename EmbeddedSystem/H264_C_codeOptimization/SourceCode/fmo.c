
/*!
*****************************************************************************
*
* \file fmo.c
*
* \brief
*    Support for Flexible Macroblock Ordering (FMO)
*
* \author
*    Main contributors (see contributors.h for copyright, address and affiliation details)
*    - Stephan Wenger      stewe@cs.tu-berlin.de
*    - Karsten Suehring    suehring@hhi.de
******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "global.h"
#include "elements.h"
#include "defines.h"
#include "header.h"
#include "fmo.h"

//#define PRINT_FMO_MAPS

//int *MbToSliceGroupMap = NULL;		/*Changed by Saad Bin Shams [Removing Global Variables]*/
//int *MapUnitToSliceGroupMap = NULL;	/*Changed by Saad Bin Shams [Removing Global Variables]*/
//static int NumberOfSliceGroups;       /*Changed by Saad Bin Shams [Removing Global Variables]*/

/*****************/
/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
/*****************/
/**/static void FmoGenerateType0MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params );
/**/static void FmoGenerateType1MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params );
/**/static void FmoGenerateType2MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params);
/**/static void FmoGenerateType3MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params );
/**/static void FmoGenerateType4MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params );
/**/static void FmoGenerateType5MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params );
/**/static void FmoGenerateType6MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params );
/*
************************************************************************
* \brief
*    Generates MapUnitToSliceGroupMap
*    Has to be called every time a new Picture Parameter Set is used
*
* \param pps
*    Picture Parameter set to be used for map generation
* \param sps
*    Sequence Parameter set to be used for map generation
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

static int FmoGenerateMapUnitToSliceGroupMap( h264_decoder* dec_params )
{
	unsigned int NumSliceGroupMapUnits;
	
	NumSliceGroupMapUnits = (dec_params->active_sps->pic_height_in_map_units_minus1+1)* (dec_params->active_sps->pic_width_in_mbs_minus1+1);
	
	if (dec_params->active_pps->slice_group_map_type == 6)
	{
		if ((dec_params->active_pps->num_slice_group_map_units_minus1+1) != NumSliceGroupMapUnits)
		{
			printf("wrong pps->num_slice_group_map_units_minus1 for used SPS and FMO type 6" );
			exit(0);
		}
	}
	
	// allocate memory for MapUnitToSliceGroupMap
	if (dec_params->MapUnitToSliceGroupMap)
		h264_free (dec_params->MapUnitToSliceGroupMap);
	if ((dec_params->MapUnitToSliceGroupMap = (int *)h264_malloc ((NumSliceGroupMapUnits) * sizeof (int))) == NULL)
	{
		printf ("cannot allocated %d bytes for MapUnitToSliceGroupMap, exit\n", (dec_params->active_pps->num_slice_group_map_units_minus1+1) * sizeof (int));
		exit (-1);
	}
	
	if (dec_params->active_pps->num_slice_groups_minus1 == 0)    // only one slice group
	{
		
		memset(dec_params->MapUnitToSliceGroupMap, 0, NumSliceGroupMapUnits * sizeof (int));
		
		return 0;
	}
	
	switch (dec_params->active_pps->slice_group_map_type)
	{
	case 0:
		FmoGenerateType0MapUnitMap (NumSliceGroupMapUnits,dec_params);
		break;
	case 1:
		FmoGenerateType1MapUnitMap (NumSliceGroupMapUnits,dec_params);
		break;
	case 2:
		FmoGenerateType2MapUnitMap ( NumSliceGroupMapUnits,dec_params);
		break;
	case 3:
		FmoGenerateType3MapUnitMap ( NumSliceGroupMapUnits,dec_params);
		break;
	case 4:
		FmoGenerateType4MapUnitMap ( NumSliceGroupMapUnits,dec_params);
		break;
	case 5:
		FmoGenerateType5MapUnitMap ( NumSliceGroupMapUnits,dec_params);
		break;
	case 6:
		FmoGenerateType6MapUnitMap ( NumSliceGroupMapUnits,dec_params);
		break;
	default:
		printf ("Illegal slice_group_map_type %d , exit \n", dec_params->active_pps->slice_group_map_type);
		exit (-1);
	}
	return 0;
}


/*!
************************************************************************
* \brief
*    Generates MbToSliceGroupMap from MapUnitToSliceGroupMap
*
* \param pps
*    Picture Parameter set to be used for map generation
* \param sps
*    Sequence Parameter set to be used for map generation
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

static int FmoGenerateMbToSliceGroupMap (h264_decoder* dec_params)
{
	unsigned i;
	
	// allocate memory for MbToSliceGroupMap
	if (dec_params->MbToSliceGroupMap)
		h264_free (dec_params->MbToSliceGroupMap);
	
	if ((dec_params->MbToSliceGroupMap = (int *)h264_malloc ((dec_params->img->FrameSizeInMbs) * sizeof (int))) == NULL)
	{
		printf ("cannot allocated %d bytes for MbToSliceGroupMap, exit\n", (dec_params->img->FrameSizeInMbs) * sizeof (int));
		exit (-1);
	}
	for (i=0; i<dec_params->img->FrameSizeInMbs; i++)
	{
		dec_params->MbToSliceGroupMap[i] = dec_params->MapUnitToSliceGroupMap[i];
	}
	return 0;
}


/*!
************************************************************************
* \brief
*    FMO initialization: Generates MapUnitToSliceGroupMap and MbToSliceGroupMap.
*
* \param pps
*    Picture Parameter set to be used for map generation
* \param sps
*    Sequence Parameter set to be used for map generation
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
int FmoInit( h264_decoder* dec_params )
{
#ifdef PRINT_FMO_MAPS
	unsigned i,j;
#endif
	
	FmoGenerateMapUnitToSliceGroupMap( dec_params );
	FmoGenerateMbToSliceGroupMap(dec_params);
	
	dec_params->NumberOfSliceGroups = dec_params->active_pps->num_slice_groups_minus1+1;
	
#ifdef PRINT_FMO_MAPS
	printf("\n");
	printf("FMO Map (Units):\n");
	
	for (j=0; j<img->PicHeightInMapUnits; j++)
	{
		for (i=0; i<img->PicWidthInMbs; i++)
		{
			printf("%c",48+dec_params->MapUnitToSliceGroupMap[i+j*img->PicWidthInMbs]);
		}
		printf("\n");
	}
	printf("\n");
	printf("FMO Map (Mb):\n");
	
	for (j=0; j<img->PicHeightInMbs; j++)
	{
		for (i=0; i<img->PicWidthInMbs; i++)
		{
			printf("%c",48+dec_params->MbToSliceGroupMap[i+j*img->PicWidthInMbs]);
		}
		printf("\n");
	}
	printf("\n");
	
#endif
	
	return 0;
}


/*!
************************************************************************
* \brief
*    Free memory allocated by FMO functions
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

int FmoFinit(  h264_decoder* dec_params)
{
	if (dec_params->MbToSliceGroupMap)
	{
		h264_free (dec_params->MbToSliceGroupMap);
		dec_params->MbToSliceGroupMap = NULL;
	}
	if (dec_params->MapUnitToSliceGroupMap)
	{
		h264_free (dec_params->MapUnitToSliceGroupMap);
		dec_params->MapUnitToSliceGroupMap = NULL; 
	}
	return 0;
}


/*!
************************************************************************
* \brief
*    FmoGetNumberOfSliceGroup() 
*
* \par Input:
*    None
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

int FmoGetNumberOfSliceGroup(h264_decoder*dec_params)
{
	return dec_params->NumberOfSliceGroups;
}


/*!
************************************************************************
* \brief
*    FmoGetLastMBOfPicture() 
*    returns the macroblock number of the last MB in a picture.  This
*    mb happens to be the last macroblock of the picture if there is only
*    one slice group
*
* \par Input:
*    None
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
int FmoGetLastMBOfPicture(  h264_decoder* dec_params)
{
	return FmoGetLastMBInSliceGroup (FmoGetNumberOfSliceGroup(dec_params)-1,dec_params);
}


/*!
************************************************************************
* \brief
*    FmoGetLastMBInSliceGroup: Returns MB number of last MB in SG
*
* \par Input:
*    SliceGroupID (0 to 7)
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
int FmoGetLastMBInSliceGroup (int SliceGroup,  h264_decoder* dec_params)
{
	int i;
	for (i=dec_params->img->FrameSizeInMbs-1; i>=0; i--)
	{
		if (FmoGetSliceGroupId (i,dec_params) == SliceGroup)
		{
			return i;
		}
		return -1;
	}
	return -1;
};


/*!
************************************************************************
* \brief
*    Returns SliceGroupID for a given MB
*
* \param mb
*    Macroblock number (in scan order)
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
int FmoGetSliceGroupId (int mb,  h264_decoder* dec_params)
{
	assert (mb < (int)dec_params->img->FrameSizeInMbs);
	assert (dec_params->MbToSliceGroupMap != NULL);
	return dec_params->MbToSliceGroupMap[mb];
}
/*!
************************************************************************
* \brief
*    Generate interleaved slice group map type MapUnit map (type 0)
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void FmoGenerateType0MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params )
{
	unsigned iGroup, j;
	unsigned i = 0;
	do
	{
		for(iGroup = 0; 
		(iGroup <= dec_params->active_pps->num_slice_groups_minus1) && (i < PicSizeInMapUnits); 
		i += dec_params->active_pps->run_length_minus1[iGroup++] + 1 )
		{
			for( j = 0; j <= dec_params->active_pps->run_length_minus1[ iGroup ] && i + j < PicSizeInMapUnits; j++ )
			{
				dec_params->MapUnitToSliceGroupMap[i+j] = iGroup;
			}
		}
	}
	while( i < PicSizeInMapUnits );
}


/*!
************************************************************************
* \brief
*    Generate dispersed slice group map type MapUnit map (type 1)
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void FmoGenerateType1MapUnitMap (unsigned PicSizeInMapUnits ,  h264_decoder* dec_params)
{
	unsigned i;
	for( i = 0; i < PicSizeInMapUnits; i++ )
	{
		dec_params->MapUnitToSliceGroupMap[i] = ((i%dec_params->img->FrameWidthInMbs)+(((i/dec_params->img->FrameWidthInMbs)*(dec_params->active_pps->num_slice_groups_minus1+1))/2))
			%(dec_params->active_pps->num_slice_groups_minus1+1);
	}
}

/*!
************************************************************************
* \brief
*    Generate foreground with left-over slice group map type MapUnit map (type 2)
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void FmoGenerateType2MapUnitMap ( unsigned PicSizeInMapUnits ,  h264_decoder* dec_params)
{
	int iGroup;
	unsigned i, x, y;
	unsigned yTopLeft, xTopLeft, yBottomRight, xBottomRight;
	
	for( i = 0; i < PicSizeInMapUnits; i++ )
	{
		dec_params->MapUnitToSliceGroupMap[ i ] = dec_params->active_pps->num_slice_groups_minus1;
	}
	
	for( iGroup = dec_params->active_pps->num_slice_groups_minus1 - 1 ; iGroup >= 0; iGroup-- ) 
	{
		yTopLeft = dec_params->active_pps->top_left[ iGroup ] / dec_params->img->FrameWidthInMbs;
		xTopLeft = dec_params->active_pps->top_left[ iGroup ] % dec_params->img->FrameWidthInMbs;
		yBottomRight = dec_params->active_pps->bottom_right[ iGroup ] / dec_params->img->FrameWidthInMbs;
		xBottomRight = dec_params->active_pps->bottom_right[ iGroup ] % dec_params->img->FrameWidthInMbs;
		for( y = yTopLeft; y <= yBottomRight; y++ )
		{
			for( x = xTopLeft; x <= xBottomRight; x++ )
			{
				dec_params->MapUnitToSliceGroupMap[ y * dec_params->img->FrameWidthInMbs + x ] = iGroup;
			}
		}
	}
}


/*!
************************************************************************
* \brief
*    Generate box-out slice group map type MapUnit map (type 3)
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

static void FmoGenerateType3MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params )
{
	unsigned i, k;
	int leftBound, topBound, rightBound, bottomBound;
	int x, y, xDir, yDir;
	int mapUnitVacant;
	
	unsigned mapUnitsInSliceGroup0 = min((dec_params->active_pps->slice_group_change_rate_minus1 + 1) * dec_params->img->slice_group_change_cycle, PicSizeInMapUnits);
	
	for( i = 0; i < PicSizeInMapUnits; i++ )
	{
		dec_params->MapUnitToSliceGroupMap[ i ] = 2;
	}
	
	x = ( dec_params->img->FrameWidthInMbs - dec_params->active_pps->slice_group_change_direction_flag ) / 2;
	y = ( dec_params->img->FrameHeightInMbs - dec_params->active_pps->slice_group_change_direction_flag ) / 2;
	
	leftBound   = x;
	topBound    = y;
	rightBound  = x;
	bottomBound = y;
	
	xDir =  dec_params->active_pps->slice_group_change_direction_flag - 1;
	yDir =  dec_params->active_pps->slice_group_change_direction_flag;
	
	for( k = 0; k < PicSizeInMapUnits; k += mapUnitVacant ) 
	{
		mapUnitVacant = ( dec_params->MapUnitToSliceGroupMap[ y * dec_params->img->FrameWidthInMbs + x ]  ==  2 );
		if( mapUnitVacant )
		{
			dec_params->MapUnitToSliceGroupMap[ y * dec_params->img->FrameWidthInMbs + x ] = ( k >= mapUnitsInSliceGroup0 );
		}
		
		if( xDir  ==  -1  &&  x  ==  leftBound ) 
		{
			leftBound = max( leftBound - 1, 0 );
			x = leftBound;
			xDir = 0;
			yDir = 2 * dec_params->active_pps->slice_group_change_direction_flag - 1;
		} 
		else
		{
			if( xDir  ==  1  &&  x  ==  rightBound ) 
			{
				rightBound = min( rightBound + 1, (int)dec_params->img->FrameWidthInMbs - 1 );
				x = rightBound;
				xDir = 0;
				yDir = 1 - 2 * dec_params->active_pps->slice_group_change_direction_flag;
			} 
			else
			{
				if( yDir  ==  -1  &&  y  ==  topBound ) 
				{
					topBound = max( topBound - 1, 0 );
					y = topBound;
					xDir = 1 - 2 * dec_params->active_pps->slice_group_change_direction_flag;
					yDir = 0;
				} 
				else
				{
					if( yDir  ==  1  &&  y  ==  bottomBound ) 
					{
						bottomBound = min( bottomBound + 1, (int)dec_params->img->FrameHeightInMbs- 1 );
						y = bottomBound;
						xDir = 2 * dec_params->active_pps->slice_group_change_direction_flag - 1;
						yDir = 0;
					} 
					else
					{
						x = x + xDir;
						y = y + yDir;
					}
				}
			}
		}
	}	
}

/*!
************************************************************************
* \brief
*    Generate raster scan slice group map type MapUnit map (type 4)
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void FmoGenerateType4MapUnitMap (unsigned PicSizeInMapUnits ,  h264_decoder* dec_params)
{
	
	unsigned mapUnitsInSliceGroup0 = min((dec_params->active_pps->slice_group_change_rate_minus1 + 1) * dec_params->img->slice_group_change_cycle, PicSizeInMapUnits);
	unsigned sizeOfUpperLeftGroup = dec_params->active_pps->slice_group_change_direction_flag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;
	
	unsigned i;
	
	for( i = 0; i < PicSizeInMapUnits; i++ )
	{
		if( i < sizeOfUpperLeftGroup )
		{
			dec_params->MapUnitToSliceGroupMap[ i ] = dec_params->active_pps->slice_group_change_direction_flag;
		}
		else
		{
			dec_params->MapUnitToSliceGroupMap[ i ] = 1 - dec_params->active_pps->slice_group_change_direction_flag;
		}
	}
}

/*!
************************************************************************
* \brief
*    Generate wipe slice group map type MapUnit map (type 5)
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void FmoGenerateType5MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params )
{
	
	unsigned mapUnitsInSliceGroup0 = min((dec_params->active_pps->slice_group_change_rate_minus1 + 1) * dec_params->img->slice_group_change_cycle, PicSizeInMapUnits);
	unsigned sizeOfUpperLeftGroup = dec_params->active_pps->slice_group_change_direction_flag ? ( PicSizeInMapUnits - mapUnitsInSliceGroup0 ) : mapUnitsInSliceGroup0;
	
	unsigned i,j, k = 0;
	
	for( j = 0; j < dec_params->img->FrameWidthInMbs; j++ )
	{
		for( i = 0; i < dec_params->img->FrameHeightInMbs; i++ )
		{
			if( k++ < sizeOfUpperLeftGroup )
			{
				dec_params->MapUnitToSliceGroupMap[ i * dec_params->img->FrameWidthInMbs + j ] = 1 - dec_params->active_pps->slice_group_change_direction_flag;
			}
			else
			{
				dec_params->MapUnitToSliceGroupMap[ i * dec_params->img->FrameWidthInMbs + j ] = dec_params->active_pps->slice_group_change_direction_flag;
			}
		}
	}
}

/*!
************************************************************************
* \brief
*    Generate explicit slice group map type MapUnit map (type 6)
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void FmoGenerateType6MapUnitMap (unsigned PicSizeInMapUnits,  h264_decoder* dec_params )
{
	unsigned i;
	for (i=0; i<PicSizeInMapUnits; i++)
	{
		dec_params->MapUnitToSliceGroupMap[i] = dec_params->active_pps->slice_group_id[i];
	}
}