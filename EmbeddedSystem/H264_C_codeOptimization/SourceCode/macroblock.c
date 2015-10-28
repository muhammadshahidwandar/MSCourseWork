   
/*!
***********************************************************************
* \file macroblock.c
*
* \brief
*     Decode a Macroblock
*
* \author
*    Main contributors (see contributors.h for copyright, address and affiliation details)
*    - Inge Lille-Langy               <inge.lille-langoy@telenor.com>
*    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
*    - Jani Lainema                    <jani.lainema@nokia.com>
*    - Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
*    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
*    - Detlev Marpe                    <marpe@hhi.de>
*    - Gabi Blaettermann               <blaetter@hhi.de>
*    - Ye-Kui Wang                     <wyk@ieee.org>
*    - Lowell Winger                   <lwinger@lsil.com>
***********************************************************************
*/

#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
//#include <WINDOWS.H>
#include "global.h"
#include "mbuffer.h"
#include "elements.h"
#include "errorconcealment.h"
#include "macroblock.h"
#include "fmo.h"
#include "cabac.h"
#include "vlc.h"
#include "image.h"
#include "mb_access.h"
#include "parset.h"
#include "memalloc.h"
#include "mc.h"
#include "predict.h"
//#include "transform8x8.h"
extern int assignSE2partition[][SE_MAX_ELEMENTS];

extern predict_intra_16x16 predict_16x16[4];

extern predict_intra_4x4   predict_4x4[9];

extern predict_intra_8x8 predict_8x8[4];

#if TRACE
#define TRACE_STRING(s) strncpy(currSE.tracestring, s, TRACESTRING_SIZE)
#else
#define TRACE_STRING(s) // do nothing
#endif

const int size2pixel[5][5] = {
    { 0, },
    { 0, PIXEL_4x4, PIXEL_8x4, 0, 0 },
    { 0, PIXEL_4x8, PIXEL_8x8, 0, PIXEL_16x8 },
    { 0, },
    { 0, 0,        PIXEL_8x16, 0, PIXEL_16x16 }
};

char mb_type_map_slice_I[] = 
{ 
	I_4x4,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_PCM 
};

char mb_type_map_slice_P[] = 
{ 
	P_L0, P_L0, P_L0, P_8x8, P_8x8_ref0,
	I_4x4,  I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_PCM 
};

char part_pred_dir_map_slice_P[5][2] = 
{ 
	{ PRED_DIR_L0, PRED_DIR_L0 },
	{ PRED_DIR_L0, PRED_DIR_L0 },
	{ PRED_DIR_L0, PRED_DIR_L0 },
	{-1,-1},
	{-1,-1}
};
char partition_map_slice_P[] = { D_16x16 ,D_16x8 ,D_8x16 ,D_8x8 ,D_8x8 };

char num_partition_map_slice_P[] = { 1 ,2 ,2 ,4 ,4 };
char sub_mb_type_map_slice_P[] = { D_L0_8x8,D_L0_8x4,D_L0_4x8,D_L0_4x4 };

char sub_part_pred_dir_map_slice_P[] = { PRED_DIR_L0,PRED_DIR_L0,PRED_DIR_L0,PRED_DIR_L0 };

char num_sub_mb_type_map_slice_P[] = { 1,2,2,4 };


char mb_type_map_slice_B[] = 
{ 
	B_DIRECT,B_L0_L0,B_L1_L1,B_BI_BI,B_L0_L0,B_L0_L0,B_L1_L1,B_L1_L1,
	B_L0_L1, B_L0_L1,B_L1_L0,B_L1_L0,B_L0_BI,B_L0_BI,B_L1_BI,B_L1_BI,
	B_BI_L0, B_BI_L0,B_BI_L1,B_BI_L1,B_BI_BI,B_BI_BI,B_8x8,
	I_4x4,  I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,I_16x16,
	I_16x16,I_PCM 
};

char part_pred_dir_map_slice_B[24][2] = 
{ 
	{ -1, -1 },
	{ PRED_DIR_L0, PRED_DIR_L0 }, { PRED_DIR_L1, PRED_DIR_L1 },
	{ PRED_DIR_BI, PRED_DIR_BI }, { PRED_DIR_L0, PRED_DIR_L0 },
	{ PRED_DIR_L0, PRED_DIR_L0 }, { PRED_DIR_L1, PRED_DIR_L1 },
	{ PRED_DIR_L1, PRED_DIR_L1 }, { PRED_DIR_L0, PRED_DIR_L1 },
	{ PRED_DIR_L0, PRED_DIR_L1 }, { PRED_DIR_L1, PRED_DIR_L0 },
	{ PRED_DIR_L1, PRED_DIR_L0 }, { PRED_DIR_L0, PRED_DIR_BI },
	{ PRED_DIR_L0, PRED_DIR_BI }, { PRED_DIR_L1, PRED_DIR_BI },
	{ PRED_DIR_L1, PRED_DIR_BI }, { PRED_DIR_BI, PRED_DIR_L0 },
	{ PRED_DIR_BI, PRED_DIR_L0 }, { PRED_DIR_BI, PRED_DIR_L1 },
	{ PRED_DIR_BI, PRED_DIR_L1 }, { PRED_DIR_BI, PRED_DIR_BI },
	{ PRED_DIR_BI, PRED_DIR_BI },
	{ -1, -1 }
};



char partition_map_slice_B[] = 
{ 
	D_8x8 , D_16x16,D_16x16,D_16x16,D_16x8,D_8x16,D_16x8,D_8x16,
	D_16x8, D_8x16, D_16x8, D_8x16, D_16x8,D_8x16,D_16x8,D_8x16,
	D_16x8, D_8x16, D_16x8, D_8x16, D_16x8,D_8x16,D_8x8
};

char num_partition_map_slice_B[] = 
{ 
	4,1,1,1,2,2,2,2,
	2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,4
};


char sub_mb_type_map_slice_B[] = 
{ 
	D_DIRECT_8x8,D_L0_8x8,D_L1_8x8,D_BI_8x8,
	D_L0_8x4,D_L0_4x8,D_L1_8x4,D_L1_4x8,D_BI_8x4,
	D_BI_4x8,D_L0_4x4,D_L1_4x4,D_BI_4x4 
};

char sub_part_pred_dir_map_slice_B[] = 
{ 
	-1,
	PRED_DIR_L0,PRED_DIR_L1,PRED_DIR_BI,
	PRED_DIR_L0,PRED_DIR_L0,PRED_DIR_L1,
	PRED_DIR_L1,PRED_DIR_BI,PRED_DIR_BI,
	PRED_DIR_L0,PRED_DIR_L1,PRED_DIR_BI
};

char num_sub_mb_type_map_slice_B[] = 
{ 
	1,1,1,1,
	2,2,2,2,2,2,
	4,4,4 
};

char cache_scan_array[16] = 
{
	6*1+1, 6*1+2, 6*1+3, 6*1+4,
	6*2+1, 6*2+2, 6*2+3, 6*2+4,
	6*3+1, 6*3+2, 6*3+3, 6*3+4,
	6*4+1, 6*4+2, 6*4+3, 6*4+4
};


void (*mc_avg[10])( imgpel *dst, int, imgpel *src, int ) = {
	pixel_avg_16x16,
    pixel_avg_16x8,
    pixel_avg_8x16,
    pixel_avg_8x8,
    pixel_avg_8x4,
    pixel_avg_4x8,
    pixel_avg_4x4,
    pixel_avg_4x2,
    pixel_avg_2x4,
    pixel_avg_2x2
};



static const int chroma_qp_table[52] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39
};

//extern int last_dquant;				/*Changed by Saad Bin Shams [Removing Global Variables]*/
//extern ColocatedParams *Co_located;	/*Changed by Saad Bin Shams [Removing Global Variables]*/
static __inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}

// SLICE function pointers
int  (*nal_startcode_follows) (int temp, h264_decoder* dec_params);

void SetMotionVectorPredictor_baseline(short           *pmv_x,
											  short           *pmv_y,
											  char            ref_frame,
											  byte            list,
											  int             block_x,
											  int             block_y,
											  int             blockshape_x,
											  int             blockshape_y,
											  h264_decoder* dec_params);
void SetMotionVectorPredictor_n(short           *pmv_x,
											  short           *pmv_y,
											  char            ref_frame,
											  byte            list,
											  PixelPos        block_a,
											  PixelPos        block_b,
											  PixelPos        block_c,
											  PixelPos        block_d,
											  h264_decoder* dec_params);


///****************************************************************************///
///****************************************************************************///



///****************************************************************************///
///****************************************************************************///

static __inline int median( int a, int b, int c )
{
    int min = a, max =a;
    if( b < min )
        min = b;
    else
        max = b;    /* no need to do 'b > max' (more consuming than always doing affectation) */

    if( c < min )
        min = c;
    else if( c > max )
        max = c;

    return a + b + c - min - max;
}

static void mb_predict_mv( char* ref_idx_cache, int* mv_cache, int idx, int i_width, short mvp[2], int partition )
{
    const int i8 = cache_scan_array[idx];
    const int i_ref= ref_idx_cache[i8];
    int     i_refa = ref_idx_cache[i8 - 1];
    short *mv_a  = (short*) &mv_cache[i8 - 1];
    int     i_refb = ref_idx_cache[i8 - 6];
    short *mv_b  = (short*) &mv_cache[i8 - 6];
    int     i_refc = ref_idx_cache[i8 - 6 + i_width ];
    short *mv_c  = (short*) &mv_cache[i8 - 6 + i_width];

    int i_count;

    //if( (idx&0x03) == 3 || ( i_width == 2 && (idx&0x3) == 2 )|| i_refc == -1 )
	if( (idx&0x05) == 5 || ((idx & 0x04) == 4 && i_width == 2) || i_refc == -2 )
    {
        i_refc = ref_idx_cache[i8 - 6 - 1];
        mv_c   = (short*) &mv_cache[i8 - 6 - 1];
    }

    if( partition == D_16x8 )
    {
        if( idx == 0 && i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
            return;
        }
        else if( idx != 0 && i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
            return;
        }
    }
    else if( partition == D_8x16 )
    {
        if( idx == 0 && i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
            return;
        }
        else if( idx != 0 && i_refc == i_ref )
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
            return;
        }
    }

    i_count = 0;
    if( i_refa == i_ref ) i_count++;
    if( i_refb == i_ref ) i_count++;
    if( i_refc == i_ref ) i_count++;

    if( i_count > 1 )
    {
        mvp[0] = median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = median( mv_a[1], mv_b[1], mv_c[1] );
    }
    else if( i_count == 1 )
    {
        if( i_refa == i_ref )
        {
            mvp[0] = mv_a[0];
            mvp[1] = mv_a[1];
        }
        else if( i_refb == i_ref )
        {
            mvp[0] = mv_b[0];
            mvp[1] = mv_b[1];
        }
        else
        {
            mvp[0] = mv_c[0];
            mvp[1] = mv_c[1];
        }
    }
    else if( i_refb == -2 && i_refc == -2 && i_refa != -2 )
    {
        mvp[0] = mv_a[0];
        mvp[1] = mv_a[1];
    }
    else
    {
        mvp[0] = median( mv_a[0], mv_b[0], mv_c[0] );
        mvp[1] = median( mv_a[1], mv_b[1], mv_c[1] );
    }
}

static void fill_structure_mv_idx(ImageParameters* img, char *ref_idx_struct, int *mvpred_struct, char *curr_ref_idx, int* mv_start)
{
	int * mv = mv_start + (img->current_mb_nr<<4);
	unsigned int mvWidth = img->width>>2;
	int index1;
	int i;	

	// initialize with 0;
	for(i=0; i<30; i++)
	{
		ref_idx_struct[i] = -2;
		mvpred_struct[i]  = 0;
	}

	// copy ref indexes of current macroblock
//  *((int*)(ref_idx_struct+7))     = *((int*)(curr_ref_idx));
//  *((int*)(ref_idx_struct+13))    = *((int*)(curr_ref_idx+4));
//  *((int*)(ref_idx_struct+19))    = *((int*)(curr_ref_idx+8));
//  *((int*)(ref_idx_struct+25))    = *((int*)(curr_ref_idx+12));

    *(ref_idx_struct+7)     = *(curr_ref_idx+0);
    *(ref_idx_struct+8)     = *(curr_ref_idx+1);
    *(ref_idx_struct+9)     = *(curr_ref_idx+2);
    *(ref_idx_struct+10)    = *(curr_ref_idx+3);
    *(ref_idx_struct+13)    = *(curr_ref_idx+4);
    *(ref_idx_struct+14)    = *(curr_ref_idx+5);
    *(ref_idx_struct+15)    = *(curr_ref_idx+6);
    *(ref_idx_struct+16)    = *(curr_ref_idx+7);
    *(ref_idx_struct+19)    = *(curr_ref_idx+8);
    *(ref_idx_struct+20)    = *(curr_ref_idx+9);
    *(ref_idx_struct+21)    = *(curr_ref_idx+10);
    *(ref_idx_struct+22)    = *(curr_ref_idx+11);
    *(ref_idx_struct+25)    = *(curr_ref_idx+12);
    *(ref_idx_struct+26)    = *(curr_ref_idx+13);
    *(ref_idx_struct+27)    = *(curr_ref_idx+14);
    *(ref_idx_struct+28)    = *(curr_ref_idx+15);
	if(img->currMB.mbAvailA)//left
	{
		
		ref_idx_struct[6]  = *(curr_ref_idx - 13);
		ref_idx_struct[12] = *(curr_ref_idx - 9);
		ref_idx_struct[18] = *(curr_ref_idx - 5);
		ref_idx_struct[24] = *(curr_ref_idx - 1);

		mvpred_struct[6]  = *(mv - 13);
		mvpred_struct[12] = *(mv - 9);
		mvpred_struct[18] = *(mv - 5);
		mvpred_struct[24] = *(mv - 1);
		
	}
	
	if(img->currMB.mbAvailB)//top
	{
		index1 = (img->FrameWidthInMbs*16) - 12;
		
        // *((int*)(&ref_idx_struct[1])) = *((int*)(curr_ref_idx - index1));
		ref_idx_struct[1] = *(curr_ref_idx - index1 + 0);
		ref_idx_struct[2] = *(curr_ref_idx - index1 + 1);
		ref_idx_struct[3] = *(curr_ref_idx - index1 + 2);
		ref_idx_struct[4] = *(curr_ref_idx - index1 + 3);

		mvpred_struct[1] = *(mv - index1);
		mvpred_struct[2] = *(mv - index1 + 1 );
		mvpred_struct[3] = *(mv - index1 + 2 );
		mvpred_struct[4] = *(mv - index1 + 3 );
	}
	
	if(img->currMB.mbAvailC)//top right
	{
		index1 = ((img->FrameWidthInMbs-1)*16) - 12;
		ref_idx_struct[5] = *(curr_ref_idx - index1);
		mvpred_struct[5] = *(mv - index1);
	}
	
	if(img->currMB.mbAvailD)//top left
	{
		index1 = ((img->FrameWidthInMbs)*16) + 1;
		ref_idx_struct[0] = *(curr_ref_idx - index1);
		mvpred_struct[0] = *(mv - index1);
	}	
}

int readSyntaxElement_UVLC(SyntaxElement *sym, DataPartition *dP)
{
    Bitstream   *currStream = dP->bitstream;
    return (readSyntaxElement_VLC(sym, currStream));
}

/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*///code related to CABAC added
void UseParameterSet_baseline( int PicParsetId,   h264_decoder* dec_params )
{
	Slice *currentSlice = dec_params->img->currentSlice;
	seq_parameter_set_rbsp_t *sps = &dec_params->SeqParSet[dec_params->PicParSet[PicParsetId].seq_parameter_set_id];
	pic_parameter_set_rbsp_t *pps = &dec_params->PicParSet[PicParsetId];
	
	if (dec_params->PicParSet[PicParsetId].Valid != TRUE)
		printf ("Trying to use an invalid (uninitialized) Picture Parameter Set with ID %d, expect the unexpected...\n", PicParsetId);
	if (dec_params->SeqParSet[dec_params->PicParSet[PicParsetId].seq_parameter_set_id].Valid != TRUE)
		printf ("PicParset %d references an invalid (uninitialized) Sequence Parameter Set with ID %d, expect the unexpected...\n", PicParsetId, dec_params->PicParSet[PicParsetId].seq_parameter_set_id);
	
	sps =  &dec_params->SeqParSet[dec_params->PicParSet[PicParsetId].seq_parameter_set_id];
	
	if ((int) sps->pic_order_cnt_type < 0 || sps->pic_order_cnt_type > 2)  // != 1
	{
		printf ("invalid sps->pic_order_cnt_type = %d\n", sps->pic_order_cnt_type);
		exit(0);
	}
	
	if (sps->pic_order_cnt_type == 1)
	{
		if(sps->num_ref_frames_in_pic_order_cnt_cycle >= MAXnum_ref_frames_in_pic_order_cnt_cycle)
		{
			printf("num_ref_frames_in_pic_order_cnt_cycle too large");
			exit(0);
		}
	}
	
	activate_sps_baseline(sps,dec_params);
	activate_pps_baseline(pps, dec_params);
	
    if (pps->entropy_coding_mode_flag == UVLC)
    {
        nal_startcode_follows = uvlc_startcode_follows;
        currentSlice->partArr[0].readSyntaxElement = readSyntaxElement_UVLC;
    }
    else
    {
        nal_startcode_follows = cabac_startcode_follows;
        currentSlice->partArr[0].readSyntaxElement = readSyntaxElement_CABAC;
    }
}
/*!
 ************************************************************************
 * \brief
 *    returns 1 if the macroblock at the given address is available
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
///??? checking conditions for deblocking filter in each call
//__inline int mb_is_available(int mbAddr, int currMbAddr, h264_decoder*dec_params)
__inline int mb_is_available(int mbAddr, int currMbAddr, ImageParameters* img)
{
  if ((mbAddr < 0) || (mbAddr > ((int)img->FrameSizeInMbs - 1)))
    return 0;
  // the following line checks both: slice number and if the mb has been decoded
//  if (!img->DeblockCall)
//  {
	  // faisal changes er
   if (img->slice_nr[mbAddr] != img->slice_nr[currMbAddr])
      return 0;
//  }
  
  return 1;
}
/*!
************************************************************************
* \brief
*    initializes the current macroblock
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
*-----------------------------------------------------------------------
*   Changes:
*   1. Unnecessary referencing was removed
*   2. Some unnecessary initializations were removed
*
*   Humza Shahid
/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/

void start_macroblock( h264_decoder* dec_params )
{
	ImageParameters *img = dec_params->img;
	const int mb_nr = img->current_mb_nr;
	
//	assert (img->current_mb_nr < img->FrameSizeInMbs);
	
	//printf(" Mb Number %d\n",mb_nr);

	img->mb_x = (img->current_mb_nr)%(img->width>>4);
	img->mb_y = (img->current_mb_nr)/(img->width>>4);

	img->slice_nr[mb_nr] = img->current_slice_nr;
/*	
	if (img->current_slice_nr >= MAX_NUM_SLICES)
	{
		printf("maximum number of supported slices exceeded, please recompile with increased value for MAX_NUM_SLICES");
		exit(0);
	}
*/	
	dec_params->dec_picture->slice_id[img->current_mb_nr] = img->current_slice_nr;
/*
	if (img->current_slice_nr > dec_params->dec_picture->max_slice_id)
	{
		dec_params->dec_picture->max_slice_id=img->current_slice_nr;
	}
*/	
	//CheckAvailabilityOfNeighbors_baseline(img);
	//const int mb_nr = img->current_mb_nr;

    img->currMB.mbAddrA = mb_nr - 1;
    img->currMB.mbAddrB = mb_nr - img->FrameWidthInMbs;
    img->currMB.mbAddrC = mb_nr - img->FrameWidthInMbs + 1;
    img->currMB.mbAddrD = mb_nr - img->FrameWidthInMbs - 1;

	img->currMB.mbAvailD = mb_is_available(img->currMB.mbAddrD, mb_nr,img) && ((mb_nr % img->FrameWidthInMbs)!=0);
	//img->currMB.mbAvailD = img->mb_x != 0;
    img->currMB.mbAvailB = mb_is_available(img->currMB.mbAddrB, mb_nr,img);
	//img->currMB.mbAvailB = img->mb_y != 0;
    img->currMB.mbAvailC = mb_is_available(img->currMB.mbAddrC, mb_nr,img) && (((mb_nr+1) % img->FrameWidthInMbs)!=0);
	//img->currMB.mbAvailC = ((mb_nr+1) % img->FrameWidthInMbs)!=0;
    img->currMB.mbAvailA = mb_is_available(img->currMB.mbAddrA, mb_nr,img) && ((mb_nr % img->FrameWidthInMbs)!=0);
	//img->currMB.mbAvailA = img->mb_x != 0;
	
	// Reset syntax element entries in MB struct
	img->currMB.qp          = img->qp ;
	img->currMB.mb_type     = 0;
	img->currMB.cbp         = 0;
	img->currMB.cbp_blk     = 0;
	img->currMB.c_ipred_mode= DC_PRED_8; //GB

	// store filtering parameters for this MB 
	img->currMB.LFDisableIdc	= img->currentSlice->LFDisableIdc;
	img->currMB.LFAlphaC0Offset = img->currentSlice->LFAlphaC0Offset;
	img->currMB.LFBetaOffset	= img->currentSlice->LFBetaOffset;
}

/*!
************************************************************************
* \brief
*    set coordinates of the next macroblock
*    check end_of_slice condition 
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

int exit_macroblock(int eos_bit,  h264_decoder* dec_params)
{
	ImageParameters* img = dec_params->img;
	int mb_nr = img->current_mb_nr;
	

	img->mb_data1.mb_type[mb_nr] = img->currMB.mb_type;
	img->mb_data1.partition[mb_nr] = img->currMB.partition;
	img->mb_data1.qp[mb_nr] = img->currMB.qp;
	img->mb_data1.mbAvailA[mb_nr] = img->currMB.mbAvailA;
	img->mb_data1.mbAvailB[mb_nr] = img->currMB.mbAvailB;
	img->mb_data1.NoMbPartLessThan8x8Flag[mb_nr] = img->currMB.NoMbPartLessThan8x8Flag;
	img->mb_data1.LFAlphaC0Offset[mb_nr] = img->currMB.LFAlphaC0Offset;
	img->mb_data1.LFBetaOffset[mb_nr] = img->currMB.LFBetaOffset;
	img->mb_data1.LFDisableIdc[mb_nr] = img->currMB.LFDisableIdc;
	img->mb_data1.cbp_blk[mb_nr] = img->currMB.cbp_blk;
	
	img->num_dec_mb++;
	
	if (img->num_dec_mb == img->FrameSizeInMbs)
	{
		return TRUE;
	}
	else
	{
		// IF FMO FLAG PRESENT IN THE STREAM
		if (dec_params->active_pps->num_slice_groups_minus1) 
		{
			int SliceGroup = FmoGetSliceGroupId (img->current_mb_nr,dec_params);
			while (++img->current_mb_nr<(int)dec_params->img->FrameSizeInMbs && dec_params->MbToSliceGroupMap [img->current_mb_nr] != SliceGroup);
		}
		else
		{
			img->current_mb_nr++;
		}		
		
		if (img->current_mb_nr >= (int)dec_params->img->FrameSizeInMbs) // End of Slice group, MUST be end of slice
		{
			return TRUE;
		}
		else
		{
			//DataPartition *dP = &(img->currentSlice->partArr[0]);
			Bitstream   *currStream = img->currentSlice->partArr[0].bitstream;
			long byteoffset;      // byte from start of buffer
			int bitoffset;      // bit from start of byte
			int ctr_bit=0;      // control bit for current bit posision
			int more_rbsp_data;
			int cnt = 0;
			
			byteoffset= currStream->frame_bitoffset>>3;
			bitoffset=7-(currStream->frame_bitoffset & 7);
			
			// there is more until we're in the last byte
			if (byteoffset<(currStream->bitstream_length-1)) 
				more_rbsp_data = TRUE;
			else
			{
				bitoffset--;
				
				while (bitoffset>=0)
				{
					ctr_bit = (currStream->streamBuffer[byteoffset] & (0x01<<bitoffset));   // set up control bit
					if (ctr_bit>0) cnt++;
					bitoffset--;
				}
				more_rbsp_data = (0!=cnt);
			}
			
			if((!more_rbsp_data) == FALSE) 
				return FALSE;
			
			if(img->type == I_SLICE  || img->type == SI_SLICE || dec_params->active_pps->entropy_coding_mode_flag == CABAC)
				return TRUE;
			
			if(img->cod_counter<=0)
				return TRUE;	
			
			return FALSE;
		}
	}
}
/*!
************************************************************************
* \brief
*    Get the syntax elements from the NAL
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
*-----------------------------------------------------------------------
*   Changes:
*   1. Unnecessary referencing was removed
*
*   Humza Shahid
/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
*	 "Nabeel Iqbal" <nabeel.iqbal@inforient.com>
*   -Changes
*	  code removed , improved the conditional execution
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/

int read_one_macroblock( h264_decoder* dec_params )
{
	ImageParameters*			img			= dec_params->img;
	pic_parameter_set_rbsp_t*	active_pps	= dec_params->active_pps;
	StorablePicture*			dec_picture	= dec_params->dec_picture;
	SyntaxElement				currSE;
	DataPartition*				dP;
	Slice*						currSlice	= img->currentSlice;
	
	int i;
	int mb_nr = img->current_mb_nr;
	int mb_x  = img->mb_x;
	int mb_y  = img->mb_y;
	
	int		FrameWidthInMBs = img->FrameWidthInMbs;
	int		mvWidth			= img->width>>1;

	char*	mb_type_map;
	char*	mb_partition_map;
	char*	sub_mb_type_map;
	char*	num_sub_mb_type_map;
	char*	num_mb_partition_map;
	char*	sub_mb_pred_dir;
		
	char*	curr_ref_idxl0 = dec_picture->ref_idx_l0 + (mb_nr*16);
	//char *curr_ref_idxl1 = dec_picture->ref_idx_l1 + (mb_nr*16);
	short*  mvL0 =  dec_picture->mvL0 + (mb_nr<<5);
	int		cbp_index,cbp_index_sub;

	currSE.type		= SE_MBTYPE;
	dP				= &(currSlice->partArr[0]);
	currSE.mapping	= linfo_ue;
	
	if(img->type == I_SLICE)
	{
		mb_type_map				= mb_type_map_slice_I;
		cbp_index_sub = 1;
	}
	else if(img->type == P_SLICE)
	{
		mb_type_map				= mb_type_map_slice_P;
		mb_partition_map		= partition_map_slice_P;
		sub_mb_type_map			= sub_mb_type_map_slice_P;
		num_sub_mb_type_map		= num_sub_mb_type_map_slice_P;
		num_mb_partition_map	= num_partition_map_slice_P;
		sub_mb_pred_dir			= sub_part_pred_dir_map_slice_P;
		cbp_index_sub = 7;
	}
	else if(img->type == B_SLICE)
	{
		mb_type_map				= mb_type_map_slice_B;
		mb_partition_map		= partition_map_slice_B;
		sub_mb_type_map			= sub_mb_type_map_slice_B;
		num_sub_mb_type_map		= num_sub_mb_type_map_slice_B;
		num_mb_partition_map	= num_partition_map_slice_B;
		sub_mb_pred_dir			= sub_part_pred_dir_map_slice_B;
		cbp_index_sub = 24;
	}
		
	if(img->type == I_SLICE)
	{
		//  read MB type
		readSyntaxElement_VLC(&currSE, dP->bitstream);
		cbp_index				= currSE.value1;
		img->currMB.mb_type		= mb_type_map[currSE.value1];
		img->currMB.partition	= -1;

		if(!dP->bitstream->ei_flag)
			img->ei_flag[mb_nr] = 0; // faisal changes er
	}
	else
	{
		if(img->cod_counter == -1)
		{
			readSyntaxElement_VLC(&currSE, dP->bitstream);
			img->cod_counter = currSE.value1;
		}
		
		if (img->cod_counter == 0)
		{
			// read MB type
			readSyntaxElement_VLC(&currSE, dP->bitstream);

			img->currMB.mb_type			= mb_type_map[currSE.value1];
			img->currMB.mb_num_of_part	= num_mb_partition_map[currSE.value1];
		
			if(img->type == P_SLICE )
			{
				if(currSE.value1 < 5)
				{
					img->currMB.part_pred_dir[0] = part_pred_dir_map_slice_P[currSE.value1][0];
					img->currMB.part_pred_dir[1] = part_pred_dir_map_slice_P[currSE.value1][1];
					img->currMB.partition		 = mb_partition_map[currSE.value1];
				}
				else
				{
					img->currMB.partition = -1;
				}
				currSE.value1++;
			}
			else
			{
				if(currSE.value1 < 22)
				{
					img->currMB.part_pred_dir[0] = part_pred_dir_map_slice_B[currSE.value1][0];
					img->currMB.part_pred_dir[1] = part_pred_dir_map_slice_B[currSE.value1][1];
					img->currMB.partition = mb_partition_map[currSE.value1];
				}
			}
			cbp_index = currSE.value1;
			
			if(!dP->bitstream->ei_flag)
				img->ei_flag[mb_nr] = 0; // faisal changes er

			img->cod_counter--;
		} 
		else
		{
			img->cod_counter--;
			img->ei_flag[mb_nr]		= 0; // faisal changes er

			img->currMB.mb_type		= P_SKIP;
			img->currMB.partition	= D_16x16;
		}
	}
	
	if(img->currMB.mb_type == I_PCM)
	{
		img->currMB.cbp		= -1;
		img->currMB.i16mode	= 0;
	}

	if(img->currMB.mb_type == I_16x16)	
	{
		img->currMB.cbp		= ICBPTAB[(cbp_index-cbp_index_sub)>>2];
		img->currMB.i16mode	= (cbp_index-cbp_index_sub) & 0x03;
	}
	
	img->currMB.NoMbPartLessThan8x8Flag = (IS_DIRECT_1(img->currMB) && !(dec_params->active_sps->direct_8x8_inference_flag))? 0: 1;
	
	//Macroblock Sub Partition Type 
	if( img->currMB.mb_type == P_8x8 || img->currMB.mb_type == P_8x8_ref0 || 
		img->currMB.mb_type == B_8x8 )
	{
		dP = &(currSlice->partArr[0]);
		for (i=0; i<4; i++)
		{
			currSE.mapping = linfo_ue;
			readSyntaxElement_VLC(&currSE, dP->bitstream);
	
			img->currMB.mb_num_of_sub_part[i]	= num_sub_mb_type_map[currSE.value1];
			img->currMB.sub_partition[i]		= sub_mb_type_map[currSE.value1];
			img->currMB.sub_part_pred_dir[i]	= sub_mb_pred_dir[currSE.value1];

			img->currMB.NoMbPartLessThan8x8Flag	&= (img->currMB.sub_partition[i]==D_DIRECT_8x8 && dec_params->active_sps->direct_8x8_inference_flag) || 
				(img->currMB.mb_num_of_sub_part[i]==1);
		}
	}	
	
	if(active_pps->constrained_intra_pred_flag && (img->type==P_SLICE|| img->type==B_SLICE))        // inter frame
	{
		if( !IS_INTRA_1(img->currMB) )
		{
			img->intra_block[img->current_mb_nr] = 0;
		}		
	}

	if(IS_INTRA_1 (img->currMB) && dP->bitstream->ei_flag && img->number)
	{
		img->ei_flag[mb_nr] = 1;// faisal changes er
	}
	
	if (IS_DIRECT_1 (img->currMB) && img->cod_counter >= 0)
	{
		img->currMB.cbp = 0;
        memset(dec_params->img->cof_s,0,sizeof(dec_params->img->cof_s));
		return DECODE_MB;
	}
	
	if( img->currMB.mb_type == P_SKIP)
	{
		int		zeroMotionAbove = 1;
		int		zeroMotionLeft  = 1;
		int		a_ref_idx		= 0; 
		int		b_ref_idx		= 0;

		img->intra_block[mb_nr] = 0;
		if (img->currMB.mbAvailA)//(mb_a.available)
		{
			a_ref_idx = *(curr_ref_idxl0 - 13);
			zeroMotionLeft  = a_ref_idx == 0 && *(mvL0-26) == 0 && *(mvL0 - 25) == 0 ? 1 : 0;
		}
		
		if (img->currMB.mbAvailB)//(mb_a.available)
		{	
			b_ref_idx = *(curr_ref_idxl0 - (img->FrameWidthInMbs*16)+12);
			zeroMotionAbove = b_ref_idx == 0 && *(mvL0-(img->FrameWidthInMbs*32)+24) == 0 && *(mvL0-(img->FrameWidthInMbs*32)+25) == 0 ? 1 : 0;
		}

		memset( curr_ref_idxl0, 0, 16*sizeof(unsigned char)  );
		if (zeroMotionAbove || zeroMotionLeft)
		{
			img->currMB.mb_type = P_SKIP_MV_0;
			memset( mvL0, 0, 32*sizeof(short)  );
		}
		else
		{
			char	ref_idx_cache[30];
			int		mv_cache[30];
			short	mvp[2];

			fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl0, (int *)dec_picture->mvL0 );
			mb_predict_mv(ref_idx_cache, mv_cache, 0, 4, mvp, D_16x16);
			for(i=0; i<16; i++)
			{
				
				//*(((int*)mvL0)++) = *((int*)mvp);
				*((mvL0)++) = mvp[0];
				*((mvL0)++) = mvp[1];

			}
		}
		return DECODE_MB;
	}
	else if(img->currMB.mb_type != I_PCM)
	{
		if (!IS_INTRA_1 (img->currMB) )
		{
			img->intra_block[mb_nr] = 0;
			readMotionInfoFromNAL(dec_params);
		}
		else //intra prediction modes for a macroblock 4x4
		{
			img->intra_block[mb_nr] = 1;
			memset( curr_ref_idxl0, -1, 16*sizeof(unsigned char)  );
			memset( mvL0, 0, 32*sizeof(short)  );
			read_ipred_modes(dec_params);
		}
		// read CBP and Coeffs  
		readCBPandCoeffsFromNAL(dec_params);
		
	}
	else
	{
		//read pcm_alignment_zero_bit and pcm_byte[i] 
		dP = &(currSlice->partArr[0]);
		img->intra_block[mb_nr] = 1;
		memset( mvL0, 0, 32*sizeof(short)  );
		readIPCMcoeffsFromNAL(dP,dec_params);
	}
	return DECODE_MB;
}

void printf_SyntaxElement(SyntaxElement currSE)
{
        printf("currSE.type  %d\n",currSE.type);
        printf("currSE.value1  %d\n",currSE.value2);
        printf("currSE.value2  %d\n",currSE.len);
        printf("currSE.len  %d\n",currSE.len);
        printf("currSE.inf  %d\n",currSE.inf);
        printf("currSE.bitpattern  %d\n",currSE.bitpattern);
        printf("currSE.context  %d\n",currSE.context);
        printf("currSE.k  %d\n",currSE.k);
        printf("currSE.mapping  %d \n",currSE.mapping);
        printf("currSE.reading  %d \n\n",currSE.reading);
        //getch();
}

void readIPCMcoeffsFromNAL(DataPartition *dP, h264_decoder* dec_params)
{
	ImageParameters*img = dec_params->img;
	InputParameters*inp = dec_params->input;
	SyntaxElement currSE;
	int i,j;
//    printf(" FUNCTION CALLED :readIPCMcoeffsFromNAL\n");
	//For CABAC, we don't need to read bits to let stream byte aligned
	//  because we have variable for integer bytes position
	
	//if(dec_params->active_pps->entropy_coding_mode_flag  == CABAC)	
	//else
	{ 
		//read bits to let stream byte aligned
		
		if((dP->bitstream->frame_bitoffset)%8!=0)
		{
			//TRACE_STRING("pcm_alignment_zero_bit");
			currSE.len=8-(dP->bitstream->frame_bitoffset)%8;
			readSyntaxElement_FLC(&currSE, dP->bitstream);
		}
		
		//read luma and chroma IPCM coefficients
		currSE.len=8;
		//TRACE_STRING("pcm_byte luma");
		
		for(i=0;i<MB_BLOCK_SIZE;i++)
		{
			for(j=0;j<MB_BLOCK_SIZE;j++)
			{
				readSyntaxElement_FLC(&currSE, dP->bitstream);
//				img->cof_s[i/4][j/4][i%4][j%4]=currSE.value1;
			}
		} 
		currSE.len=8;
		//TRACE_STRING("pcm_byte chroma");
		for(i=0;i<MB_CR_SIZE_Y;i++)
		{
			for(j=0;j<MB_CR_SIZE_X;j++)
			{
				readSyntaxElement_FLC(&currSE, dP->bitstream);
//				img->cof_s[i/4][j/4+4][i%4][j%4]=currSE.value1;
			}
		} 
		for(i=0;i<MB_CR_SIZE_Y;i++)
		{
			for(j=0;j<MB_CR_SIZE_X;j++)
			{
				readSyntaxElement_FLC(&currSE, dP->bitstream);
//				img->cof_s[i/4+2][j/4+4][i%4][j%4]=currSE.value1;
			}
		}
	}
}

void fill_ipred_struct(h264_decoder* dec_params, char ipred_struct[25])
{
	ImageParameters* img = dec_params->img;
		
    char *i_predmode = NULL;
    int left_offset = 13;
    int top_offset = (img->FrameWidthInMbs<<4)-12;
	int i;	
	i_predmode = &(img->ipredmode[0][0]) + (img->current_mb_nr<<4);
	ipred_struct[0] = -1;

	// initialize with -1;
	for(i=1; i<5; i++)
	{
		ipred_struct[i] = -1;
		ipred_struct[i*5] = -1;
	}

	if(img->currMB.mbAvailA)//left
	{
		if(!dec_params->active_pps->constrained_intra_pred_flag)// && img->intra_block[currMB->mbAddrB])
		{
			if(img->intra_block[img->currMB.mbAddrA])
			{
				for(i=1; i<5; i++)
				{
					ipred_struct[i*5] = *(i_predmode-left_offset + ((i-1)*4));
				}
			}
			else
			{
				for(i=1; i<5; i++)
				{
					ipred_struct[i*5] = DC_PRED;
				}
			}
		}
		else
		{

			if(img->intra_block[img->currMB.mbAddrA])
			{
				for(i=1; i<5; i++)
				{
					ipred_struct[i*5] = *(i_predmode-left_offset + ((i-1)*4));
				}
			}
		}
	}

	if(img->currMB.mbAvailB)//top
	{
		if(!dec_params->active_pps->constrained_intra_pred_flag)// && img->intra_block[currMB->mbAddrA])
		{
			if(img->intra_block[img->currMB.mbAddrB])
			{
				for(i=1; i<5; i++)
				{
					ipred_struct[i] = *(i_predmode-top_offset + (i-1));
				}
			}
			else
			{
				for(i=1; i<5; i++)
				{
					ipred_struct[i] = DC_PRED;
				}
			}
		}
		else
		{
			if(img->intra_block[img->currMB.mbAddrB])
			{
				for(i=1; i<5; i++)
				{
					ipred_struct[i] = *(i_predmode-top_offset + (i-1));
				}
			}
		}
	}
}

/***********************************************************************			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
/***********************************************************************
*	- The code has been restructured as such that the number of calls to
*	  the function getLuma4x4Neighbour() are reduced to two.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 5-12-2005
***********************************************************************
*/
void read_ipred_modes(h264_decoder* dec_params)
{
	ImageParameters* img = dec_params->img;
	InputParameters* inp = dec_params->input;
	int i,dec;
	SyntaxElement currSE;
	Slice *currSlice;
	DataPartition *dP;
	int mostProbableIntraPredMode;
	int upIntraPredMode;
	int leftIntraPredMode;
	int IntraChromaPredModeFlag;
	int temp_value;
	int currSE_value1=0;
	
    char *i_predmode = NULL;
	char ipred_struct[25];
	char i_predmode_t[16];
	char * temp_ipred;

	const int index_table[]={
		  0, 1, 4, 5, 2, 3, 6, 7,8,9,12,13,10,11,14,15
	};
	const int index_table_t[]={
		0, 1, 5, 6, 2, 3, 7, 8, 10, 11, 15, 16, 12, 13, 17, 18
	};

	temp_ipred = ipred_struct+6;
    i_predmode = &(img->ipredmode[0][0]) + (img->current_mb_nr<<4);
    
    //////////////////////////////////////////////////////
    //////////////////////////////////////////////////////

	IntraChromaPredModeFlag = IS_INTRA_1(img->currMB);
	
	currSlice	= img->currentSlice;
	currSE.type = SE_INTRAPREDMODE;
	dP			= &(currSlice->partArr[0]);
	
	if(img->currMB.mb_type == I_4x4)
	{
		fill_ipred_struct(dec_params, ipred_struct);
		
		for(i=0; i<16; i++)
		{
			readSyntaxElement_Intra4x4PredictionMode(&currSE,img,inp,dP);
			temp_value = currSE.value1;
			
			upIntraPredMode            = temp_ipred[index_table_t[i]-5];
			leftIntraPredMode          = temp_ipred[index_table_t[i]-1];
			
			mostProbableIntraPredMode  = (upIntraPredMode < 0 || leftIntraPredMode < 0) ? DC_PRED : upIntraPredMode < leftIntraPredMode ? upIntraPredMode : leftIntraPredMode;
			
			dec = (temp_value == -1) ? mostProbableIntraPredMode : temp_value + (temp_value >= mostProbableIntraPredMode);
			
			i_predmode[index_table[i]] = temp_ipred[index_table_t[i]] = dec;
		}
	}
	else
	{
		memset(&(img->ipredmode[0][0]) + (img->current_mb_nr<<4), DC_PRED, 16);
	}
	
	if (IntraChromaPredModeFlag)
	{
		currSE.type = SE_INTRAPREDMODE;
		currSE.mapping = linfo_ue;
		readSyntaxElement_VLC(&currSE, dP->bitstream);
		
		img->currMB.c_ipred_mode = currSE.value1;
		
		if (img->currMB.c_ipred_mode < DC_PRED_8 || img->currMB.c_ipred_mode > PLANE_8)
		{
			printf("illegal chroma intra pred mode!\n");
			exit(0);
		}
	}
}

/*!
************************************************************************
* \brief
*    Set motion vector predictor
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/
void SetMotionVectorPredictor_baseline(short           *pmv_x,
											  short           *pmv_y,
											  char            ref_frame,
											  byte            list,
											  int             block_x,
											  int             block_y,
											  int             blockshape_x,
											  int             blockshape_y,
											  h264_decoder* dec_params)
{
	ImageParameters  *img=dec_params->img;
	short           *tmp_mv;
	
	char            ***refPic=dec_params->dec_picture->ref_idx;
	int mb_x                 = BLOCK_SIZE*block_x;
	int mb_y                 = BLOCK_SIZE*block_y;
	int mb_nr                = img->current_mb_nr;	
	int mv_a, mv_b, mv_c, pred_vec=0;
	int mvPredType, rFrameL, rFrameU, rFrameUR;
	int hv;	
	PixelPos block_a, block_b, block_c, block_d;	
	Macroblock *currMB = &img->mb_data[mb_nr];
	unsigned int posx,posy; 
	unsigned int mvWidth = img->width>>1;

	// faisal ref_idx
	char * curr_ref_idx;
	int mb_nr_a,mb_nr_b, mb_nr_c;
	int a_add, b_add, c_add;
	int dummy_rFrameL, dummy_rFrameU, dummy_rFrameUR;
	
	//tmp_mv = list?dec_outputs->dec_picture->mvL1:dec_outputs->dec_picture->mvL0;

	if(!list)
	{
		tmp_mv = dec_params->dec_picture->mvL0;
		curr_ref_idx = dec_params->dec_picture->ref_idx_l0;
	}
	else
	{
		tmp_mv = dec_params->dec_picture->mvL1;
		curr_ref_idx = dec_params->dec_picture->ref_idx_l1;
	}
	
	
	//getLuma4x4Neighbour( block_x, block_y,           -1,  0, &block_a,dec_params,dec_outputs);
	//getLuma4x4Neighbour( block_x, block_y,            0, -1, &block_b,dec_params,dec_outputs);
	//getLuma4x4Neighbour( block_x, block_y, blockshape_x, -1, &block_c,dec_params,dec_outputs);
	//getLuma4x4Neighbour( block_x, block_y,           -1, -1, &block_d,dec_params,dec_outputs);
	///??? Try to put the conditions on posx , and posy ,
	///     working fine but slicing mechanism how works need to understand
	posx = ((mb_nr % img->FrameWidthInMbs)<<2)+(block_x);
	posy = ((mb_nr / img->FrameWidthInMbs)<<2)+(block_y);
	
	if(block_x !=0 && block_y !=0)
	{		
		block_a.available = 1;
		block_b.available = 1;
		block_d.available = 1;
		
		// position of block A
		block_a.pos_x = posx-1;
		block_a.pos_y = posy;
		// position of block B
		block_b.pos_x = posx;
		block_b.pos_y = posy-1;
		// position of block D
		block_d.pos_x = posx-1;
		block_d.pos_y = posy-1;
		
		if(block_x <3)
		{
			block_c.available = 1;
			// position of block C
			block_c.pos_x = posx+1;
			block_c.pos_y = posy-1;
		}
		else
		{
			block_c.available =0;
		}
		//////////////////////////////////////////////////////////////////////////////
	}else if(block_y !=0 && block_x ==0)
	{	
		block_c.available = 1;
		block_c.pos_x = (blockshape_x == 8) ?  posx+2:posx+1;
		block_c.pos_y = posy-1;
		
		block_b.available =1;
		// position of block B
		block_b.pos_x = posx;
		block_b.pos_y = posy-1;
		
		if(currMB->mbAvailA)
		{
			block_a.available =1;
			block_a.pos_x = posx-1;
			block_a.pos_y = posy;
			
			// position of block D
			block_d.available =1;
			block_d.pos_x = posx-1;
			block_d.pos_y = posy-1;
		}else
		{
			block_a.available =0;
			block_d.available =0;
			
		}
		///////////////////////////////////////////////////////////////////////////////////
	}else if(block_y ==0 && block_x !=0)
	{
		block_a.available = 1;
		block_a.pos_x = posx-1;//blockshape_x;//block_a.pos_x;
		block_a.pos_y = posy;//block_a.pos_y;
		
		
		if(currMB->mbAvailB)
		{
			block_d.available =1;
			block_d.pos_x = posx-1;
			block_d.pos_y = posy-1;
			
			
			block_b.available =1;
			// position of block B
			block_b.pos_x = posx;//block_a.pos_x;
			block_b.pos_y = posy-1;//block_a.pos_y;
			
		}else
		{
			block_b.available =0;
			block_c.available =0;
			block_d.available =0;
			
		}
		
		/*
		if(currMB->mbAvailC)
		{
		// position of block C
		block_c.available =1;
		//block_c.pos_x = posx+1;
		block_c.pos_x = (blockshape_x == 8) ?  posx+2:posx+1;
		block_c.pos_y = posy-1;
		}else
		{
		block_c.available = 0;
		getLuma4x4Neighbour( block_x, block_y, blockshape_x, -1, &block_c,dec_params,dec_outputs);
		}*/
		
		getLuma4x4Neighbour( block_x, block_y, blockshape_x, -1, &block_c,dec_params);
		//////////////////////////////////////////////////////////////////////////////
	}else if(block_y ==0 && block_x ==0)
	{		
		if(currMB->mbAvailA)
		{
			block_a.available = 1;
			block_a.pos_x = posx-1;//blockshape_x;//block_a.pos_x;
			block_a.pos_y = posy;//block_a.pos_y;			
		}
		else
		{
			block_a.available = 0;
		}
		
		if(currMB->mbAvailB)
		{			
			block_b.available =1;
			// position of block B
			block_b.pos_x = posx;//block_a.pos_x;
			block_b.pos_y = posy-1;//block_a.pos_y;			
		}	
		else
		{
			block_b.available = 0;
			//block_c.available = 0;
		}
		
		if(currMB->mbAvailD)
		{
			block_d.available =1;
			// position of block B
			block_d.pos_x = posx-1;//block_a.pos_x;
			block_d.pos_y = posy-1;//block_a.pos_y;			
		}
		else
		{
			block_d.available = 0;
		}
		
		/*
		if(currMB->mbAvailC)
		{
		// position of block C
		block_c.available =1;
		//block_c.pos_x = posx+1;//block_a.pos_x;
		block_c.pos_x = posx+blockshape_x/4;//(blockshape_x == 16) ?  posx+4:posx+2;
		block_c.pos_y = posy-1;//block_a.pos_y;
		}else
		{
		block_c.available =0;
		//block_c = block_d;
		getLuma4x4Neighbour(0,0, blockshape_x, -1, &block_c,dec_params,dec_outputs);
		
		}*/
		getLuma4x4Neighbour(0,0, blockshape_x, -1, &block_c,dec_params);
	}	
	
	////////////////Calls after getLuma4x4Neighbour///////////////////////
	if (mb_y > 0)
	{
		if (mb_x < 8)  // first column of 8x8 blocks
		{
			if (mb_y==8)
			{
				if (blockshape_x == 16)      block_c.available  = 0;
			}
			else
			{
				if (mb_x+blockshape_x == 8)  block_c.available  = 0;
			}
		}
		else
		{
			if (mb_x+blockshape_x == 16)   block_c.available  = 0;
		}
	}
	
	if (!block_c.available)
	{
		block_c=block_d;
	}
	
	mvPredType = MVPRED_MEDIAN;
	
	/*rFrameL    = block_a.available    ? refPic[list][block_a.pos_y][block_a.pos_x] : -1;
	rFrameU    = block_b.available    ? refPic[list][block_b.pos_y][block_b.pos_x] : -1;
	rFrameUR   = block_c.available    ? refPic[list][block_c.pos_y][block_c.pos_x] : -1;
	*///faisal ref_idx

	// faisal ref_idx to be replaced by above (swap)
	mb_nr_a = ((block_a.pos_y>>2) * img->FrameWidthInMbs) + (block_a.pos_x>>2);
	mb_nr_b = ((block_b.pos_y>>2) * img->FrameWidthInMbs) + (block_b.pos_x>>2);
	mb_nr_c = ((block_c.pos_y>>2) * img->FrameWidthInMbs) + (block_c.pos_x>>2);

	a_add = ((block_a.pos_y%4) << 2) + (block_a.pos_x%4);
	b_add = ((block_b.pos_y%4) << 2) + (block_b.pos_x%4);
	c_add = ((block_c.pos_y%4) << 2) + (block_c.pos_x%4);

	rFrameL    = block_a.available    ? *(curr_ref_idx + (mb_nr_a<<4) + a_add) : -1;
	rFrameU    = block_b.available    ? *(curr_ref_idx + (mb_nr_b<<4) + b_add) : -1;
	rFrameUR   = block_c.available    ? *(curr_ref_idx + (mb_nr_c<<4) + c_add) : -1;

		
	/* Prediction if only one of the neighbors uses the reference frame
	* we are checking
	*/
	if(rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
		mvPredType = MVPRED_L;
	else if(rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
		mvPredType = MVPRED_U;
	else if(rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
		mvPredType = MVPRED_UR;
	// Directional predictions 
	if(blockshape_x == 8 && blockshape_y == 16)
	{
		if(mb_x == 0)
		{
			if(rFrameL == ref_frame)
				mvPredType = MVPRED_L;
		}
		else
		{
			if( rFrameUR == ref_frame)
				mvPredType = MVPRED_UR;
		}
	}
	else if(blockshape_x == 16 && blockshape_y == 8)
	{
		if(mb_y == 0)
		{
			if(rFrameU == ref_frame)
				mvPredType = MVPRED_U;
		}
		else
		{
			if(rFrameL == ref_frame)
				mvPredType = MVPRED_L;
		}
	}
	
	for (hv=0; hv < 2; hv++)
	{
		
		
			
		mv_a = block_a.available  ? *(tmp_mv+block_a.pos_y*mvWidth+(block_a.pos_x<<1)+hv) : 0;
		mv_b = block_b.available  ? *(tmp_mv+block_b.pos_y*mvWidth+(block_b.pos_x<<1)+hv) : 0;
		mv_c = block_c.available  ? *(tmp_mv+block_c.pos_y*mvWidth+(block_c.pos_x<<1)+hv) : 0;
				
		
		switch (mvPredType)
		{
		case MVPRED_MEDIAN:
			if(!(block_b.available || block_c.available))
				pred_vec = mv_a;
			else
				pred_vec = mv_a+mv_b+mv_c-min(mv_a,min(mv_b,mv_c))-max(mv_a,max(mv_b,mv_c));
			break;
		case MVPRED_L:
			pred_vec = mv_a;
			break;
		case MVPRED_U:
			pred_vec = mv_b;
			break;
		case MVPRED_UR:
			pred_vec = mv_c;
			break;
		default:
			break;
		}
		
		if (hv==0)  *pmv_x = pred_vec;
		else        *pmv_y = pred_vec;
		
	}
}




void SetMotionVectorPredictor_n(short           *pmv_x,
											  short           *pmv_y,
											  char            ref_frame,
											  byte            list,
											  PixelPos        block_a,
											  PixelPos        block_b,
											  PixelPos        block_c,
											  PixelPos        block_d,
											  h264_decoder* dec_params)
{
	ImageParameters  *img=dec_params->img;
	short           *tmp_mv;
	char            ***refPic=dec_params->dec_picture->ref_idx;
	int mv_a, mv_b, mv_c, pred_vec=0;
	int mvPredType, rFrameL, rFrameU, rFrameUR;
	int hv;	
	unsigned int mvWidth = img->width>>1;

	// faisal ref_idx
	char * curr_ref_idx;
	int mb_nr_a,mb_nr_b, mb_nr_c;
	int a_add, b_add, c_add;
	int dummy_rFrameL, dummy_rFrameU, dummy_rFrameUR;
	if(list == 0)
	{
		curr_ref_idx = dec_params->dec_picture->ref_idx_l0;
	}
	else
	{
		curr_ref_idx = dec_params->dec_picture->ref_idx_l1;
	}

	tmp_mv = list?dec_params->dec_picture->mvL1:dec_params->dec_picture->mvL0;
	
	if (!block_c.available)
	{
		block_c=block_d;
	}
	
	mvPredType = MVPRED_MEDIAN;
	
	/*rFrameL    = block_a.available    ? refPic[list][block_a.pos_y][block_a.pos_x] : -1;
	rFrameU    = block_b.available    ? refPic[list][block_b.pos_y][block_b.pos_x] : -1;
	rFrameUR   = block_c.available    ? refPic[list][block_c.pos_y][block_c.pos_x] : -1;
	*/ // faisal ref_idx

	// faisal ref_idx to be replaced by above (swap)
	mb_nr_a = ((block_a.pos_y>>2) * img->FrameWidthInMbs) + (block_a.pos_x>>2);
	mb_nr_b = ((block_b.pos_y>>2) * img->FrameWidthInMbs) + (block_b.pos_x>>2);
	mb_nr_c = ((block_c.pos_y>>2) * img->FrameWidthInMbs) + (block_c.pos_x>>2);
	a_add = ((block_a.pos_y%4) * 4) + (block_a.pos_x%4);
	b_add = ((block_b.pos_y%4) * 4) + (block_b.pos_x%4);
	c_add = ((block_c.pos_y%4) * 4) + (block_c.pos_x%4);

	rFrameL    = block_a.available    ? *(curr_ref_idx + (16*mb_nr_a) + a_add) : -1;
	rFrameU    = block_b.available    ? *(curr_ref_idx + (16*mb_nr_b) + b_add) : -1;
	rFrameUR   = block_c.available    ? *(curr_ref_idx + (16*mb_nr_c) + c_add) : -1;


		
	/* Prediction if only one of the neighbors uses the reference frame
	* we are checking
	*/
	if(rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
		mvPredType = MVPRED_L;
	else if(rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
		mvPredType = MVPRED_U;
	else if(rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
		mvPredType = MVPRED_UR;

	
	for (hv=0; hv < 2; hv++)
	{

		mv_a = block_a.available  ? *(tmp_mv+block_a.pos_y*mvWidth+(block_a.pos_x<<1)+hv) : 0;
		mv_b = block_b.available  ? *(tmp_mv+block_b.pos_y*mvWidth+(block_b.pos_x<<1)+hv) : 0;
		mv_c = block_c.available  ? *(tmp_mv+block_c.pos_y*mvWidth+(block_c.pos_x<<1)+hv) : 0;
		
		switch (mvPredType)
		{
		case MVPRED_MEDIAN:
			if(!(block_b.available || block_c.available))
				pred_vec = mv_a;
			else
				pred_vec = mv_a+mv_b+mv_c-min(mv_a,min(mv_b,mv_c))-max(mv_a,max(mv_b,mv_c));
			break;
		case MVPRED_L:
			pred_vec = mv_a;
			break;
		case MVPRED_U:
			pred_vec = mv_b;
			break;
		case MVPRED_UR:
			pred_vec = mv_c;
			break;
		default:
			break;
		}
		
		if (hv==0)  *pmv_x = pred_vec;
		else        *pmv_y = pred_vec;
		
	}
}

/*!
************************************************************************
* \brief
*    Read motion info
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
*    "Nabeel Iqbal" <nabeel.iqbal@inforient.com>
*	-Changes
*	  Un-necessarty code related to list 1 and B frames removed
************************************************************************
*	 Changes till 21-11-2005
************************************************************************
*/


static __inline short read_idx_from_NAL_mb(int part_pred_dir, int num_ref_idx_active,int LIST, SyntaxElement *currSE, DataPartition *dP,h264_decoder* dec_params)
{
	//if(part_pred_dir !=1) //if L0 or BiPred
	if( (part_pred_dir + 1) & (LIST + 1) ) //if L0 or BiPred
	{
			
		if( num_ref_idx_active == 2)
		{
			currSE->len = 1;
			readSyntaxElement_FLC(currSE, dP->bitstream);
			currSE->value1 = 1 - currSE->value1;
		}
		else 
		{
			currSE->value2 = LIST;
			readSyntaxElement_VLC(currSE, dP->bitstream);
		}
						
		return(currSE->value1);
	}
	else
		return -1;
}

static __inline void set_ref_idx_mb (int partition, short *refidx, char* curr_ref_idx)
{
	int i0;
	if(partition == D_16x16)
	{
        // int ref = ((char)refidx[0])*0x01010101;
        for(i0=0; i0<4;i0++)
        {
            // *((int*)(curr_ref_idx) + i0) = ref;
            *(curr_ref_idx + 4*i0+0) = (char)refidx[0];
            *(curr_ref_idx + 4*i0+1) = (char)refidx[0];
            *(curr_ref_idx + 4*i0+2) = (char)refidx[0];
            *(curr_ref_idx + 4*i0+3) = (char)refidx[0];
        }
	}
	else if(partition == D_16x8)
	{
        // int ref = ((char)refidx[0])*0x01010101;
        for(i0=0; i0<2;i0++)
        {
            // *((int*)(curr_ref_idx) + i0) = ref;
            *(curr_ref_idx + 4*i0+0) = (char)refidx[0];
            *(curr_ref_idx + 4*i0+1) = (char)refidx[0];
            *(curr_ref_idx + 4*i0+2) = (char)refidx[0];
            *(curr_ref_idx + 4*i0+3) = (char)refidx[0];
        }
        
        // ref = ((char)refidx[1])*0x01010101;
        for(i0=2; i0<4;i0++)
        {
            // *((int*)(curr_ref_idx) + i0) = ref;
            *(curr_ref_idx + 4*i0+0) = (char)refidx[1];
            *(curr_ref_idx + 4*i0+1) = (char)refidx[1];
            *(curr_ref_idx + 4*i0+2) = (char)refidx[1];
            *(curr_ref_idx + 4*i0+3) = (char)refidx[1];
        }
	}
	else if(partition == D_8x16)
	{
		refidx[0] = ((char)refidx[0])*0x0101;
		refidx[1] = ((char)refidx[1])*0x0101;
        for(i0=0; i0<4;i0++)
        {
            // *((int*)(curr_ref_idx) + i0) = *((int*)refidx);
            *(curr_ref_idx + 4*i0+0) = (char)(refidx[0]);
            *(curr_ref_idx + 4*i0+1) = (char)(refidx[0]);
            *(curr_ref_idx + 4*i0+2) = (char)(refidx[1]);
            *(curr_ref_idx + 4*i0+3) = (char)(refidx[1]);
        }
	}
}

void set_ref_idx_part(short *refidx, char* curr_ref_idx)
{
	refidx[0] = ((char)refidx[0])*0x0101;
	refidx[1] = ((char)refidx[1])*0x0101;
	refidx[2] = ((char)refidx[2])*0x0101;
	refidx[3] = ((char)refidx[3])*0x0101;

//  *((int*)(curr_ref_idx ))   = *((int*)refidx);
//  *((int*)(curr_ref_idx+4))  = *((int*)refidx);
//  *((int*)(curr_ref_idx+8))  = *((int*)(refidx+2));
//  *((int*)(curr_ref_idx+12)) = *((int*)(refidx+2));
    *(curr_ref_idx+0)  = (char)(refidx[0]);
    *(curr_ref_idx+1)  = (char)(refidx[0]);
    *(curr_ref_idx+2)  = (char)(refidx[1]);
    *(curr_ref_idx+3)  = (char)(refidx[1]);
    *(curr_ref_idx+4)  = (char)(refidx[0]);
    *(curr_ref_idx+5)  = (char)(refidx[0]);
    *(curr_ref_idx+6)  = (char)(refidx[1]);
    *(curr_ref_idx+7)  = (char)(refidx[1]);
    *(curr_ref_idx+8)  = (char)(refidx[2]);
    *(curr_ref_idx+9)  = (char)(refidx[2]);
    *(curr_ref_idx+10) = (char)(refidx[3]);
    *(curr_ref_idx+11) = (char)(refidx[3]);
    *(curr_ref_idx+12) = (char)(refidx[2]);
    *(curr_ref_idx+13) = (char)(refidx[2]);
    *(curr_ref_idx+14) = (char)(refidx[3]);
    *(curr_ref_idx+15) = (char)(refidx[3]);
}

static __inline void get_motion_vector(SyntaxElement* se, DataPartition* dp, h264_decoder* dec_params, short* mv)
{
	se->type = SE_MVD;
	se->mapping = linfo_se;
	//read mvd_x
	readSyntaxElement_VLC(se, dp->bitstream);
	mv[0] += se->value1;

	//read mvd_y
	readSyntaxElement_VLC(se, dp->bitstream);
	mv[1] += se->value1;
}

void readMotionInfoFromNAL(h264_decoder* dec_params)
{
	ImageParameters* img = dec_params->img;
	int mb_nr = img->current_mb_nr;
	int mb_x = img->mb_x;
	int mb_y = img->mb_y;
	
	int FrameWidthInMBs = img->FrameWidthInMbs;

	SyntaxElement currSE;
	Slice *currSlice    = img->currentSlice;
	DataPartition *dP;
	StorablePicture* dec_picture = dec_params->dec_picture;

	int bframe          = (img->type==B_SLICE);
	int i,i0, j0;

	unsigned int mvWidth = img->width>>1;
	
	char *curr_ref_idxl0 = dec_params->dec_picture->ref_idx_l0 + (mb_nr<<4);
	char *curr_ref_idxl1 = dec_params->dec_picture->ref_idx_l1 + (mb_nr<<4);

	if(img->currMB.mb_num_of_part == 4) // 8x8 and onwards
	{
		short refidx[4];
		
		refidx[0] = -1;
		refidx[1] = -1;
		refidx[2] = -1;
		refidx[3] = -1;
		
		currSE.type = SE_REFFRAME;
		dP = &(currSlice->partArr[0]);
		currSE.mapping = linfo_ue;
		// !Direct
		if(img->num_ref_idx_l0_active > 1)
		{
			for(i0=0; i0<4; i0++)
			{
				if((bframe || img->currMB.mb_type != P_8x8_ref0) && img->currMB.sub_partition[i0] != D_DIRECT_8x8)
				{					
					refidx[i0] = read_idx_from_NAL_mb(img->currMB.sub_part_pred_dir[i0],img->num_ref_idx_l0_active,0,&currSE,dP,dec_params);
				}
				else
				{
					refidx[i0] = 0;
				}
			}
			set_ref_idx_part(refidx, curr_ref_idxl0);
		}
		else
		{
			for(i0=0; i0<4;i0++)
				*((int*)(curr_ref_idxl0) + i0) = 0;
		}

		// ref_idx_1
		if(img->profile_idc>66)
		{
			if(img->num_ref_idx_l1_active > 1)
			{
				for(i0=0; i0<4; i0++)
				{
					if(bframe && img->currMB.sub_partition[i0] != D_DIRECT_8x8)
					{
						refidx[i0] = read_idx_from_NAL_mb(img->currMB.sub_part_pred_dir[i0],img->num_ref_idx_l1_active,1,&currSE,dP,dec_params);					
					}
					else
					{
						refidx[i0] = 0;
					}
				}
				set_ref_idx_part(refidx, curr_ref_idxl1);
			}
			else
			{	
				for(i0=0; i0<4;i0++)
					*((int*)(curr_ref_idxl0) + i0) = 0;
				//memset(curr_ref_idxl0, 0, 16);
			}
		}
		{
			char	ref_idx_cache[30];
			int		mv_cache[30];
			short	mvp[2];
			int		mv_width = img->width >>2; // no of motion vectors in array
			int*	mvL0 =  ((int*)dec_picture->mvL0)  + (mb_nr<<4);
			int*	mvL1 =  ((int*)dec_picture->mvL1)  + (mb_nr<<4);
			int		index8x8[] = { 0,2,8,10 };
			int		index4x4[] = { 0,1,4,5 };

			dP = &(currSlice->partArr[0]);

			fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl0, (int *)dec_picture->mvL0 );
			for(i0=0; i0<4; i0++)
			{
				if( img->currMB.sub_partition[i0] != D_DIRECT_8x8 && 
					img->currMB.sub_part_pred_dir[i0] != PRED_DIR_L1 )
				{
					switch( img->currMB.sub_partition[i0] )
					{
					case D_L0_4x4:			// 4x4
					case D_BI_4x4:
						for( j0 = 0; j0 < 4; j0++ )
						{
							int idx = index8x8[i0] + index4x4[j0];
							mb_predict_mv(ref_idx_cache, mv_cache, idx, 1, mvp, -1);
							get_motion_vector(&currSE, dP, dec_params, mvp);

							// update the mv cache
							mv_cache[cache_scan_array[idx]] = *(int*)mvp;
						}
						break;
					case D_L0_8x4:			// 8x4
					case D_BI_8x4:
						for( j0 = 0; j0 < 2; j0++ )
						{
							int idx = index8x8[i0] + (j0*4);
							mb_predict_mv(ref_idx_cache, mv_cache, idx, 2, mvp, -1);
							get_motion_vector(&currSE, dP, dec_params, mvp);

							// update the mv cache
							mv_cache[cache_scan_array[idx]] = *(int*)mvp;
							mv_cache[cache_scan_array[idx+1]] = *(int*)mvp;
						}
						break;
					case D_L0_4x8:			// 4x8
					case D_BI_4x8:
						for( j0 = 0; j0 < 2; j0++ )
						{
							int idx = index8x8[i0] + j0;
							mb_predict_mv(ref_idx_cache, mv_cache, idx, 1, mvp, -1);
							get_motion_vector(&currSE, dP, dec_params, mvp);

							// update the mv cache
							mv_cache[cache_scan_array[idx]] = *(int*)mvp;
							mv_cache[cache_scan_array[idx+4]] = *(int*)mvp;
						}
						break;
					case D_L0_8x8:			// 8x8
					case D_BI_8x8:
						{
							int idx = index8x8[i0];
							mb_predict_mv(ref_idx_cache, mv_cache, idx, 2, mvp, -1);
							get_motion_vector(&currSE, dP, dec_params, mvp);
							
							// update the mv cache
							mv_cache[cache_scan_array[idx+0]] = *(int*)mvp;
							mv_cache[cache_scan_array[idx+1]] = *(int*)mvp;
							mv_cache[cache_scan_array[idx+4]] = *(int*)mvp;
							mv_cache[cache_scan_array[idx+5]] = *(int*)mvp;
						}
						break;
					}
				}
			}
			// save the motion vectors
			for(i = 0; i < 4; i++)
			{
				*(mvL0 ++) = mv_cache[ cache_scan_array[(i*4) + 0] ];
				*(mvL0 ++) = mv_cache[ cache_scan_array[(i*4) + 1] ];
				*(mvL0 ++) = mv_cache[ cache_scan_array[(i*4) + 2] ];
				*(mvL0 ++) = mv_cache[ cache_scan_array[(i*4) + 3] ];
			}

			// Read forward motion vectors
			if(img->profile_idc > 66)
			{
				fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl1, (int *)dec_picture->mvL1 );
				for(i0=0; i0<4; i0++)
				{
					if( img->currMB.sub_partition[i0] != D_DIRECT_8x8 && 
						img->currMB.sub_part_pred_dir[i0] != PRED_DIR_L0)
					{
						switch( img->currMB.sub_partition[i0] )
						{
						case D_L1_4x4:			// 4x4
						case D_BI_4x4:
							for( j0 = 0; j0 < 4; j0++ )
							{
								int idx = index8x8[i0] + index4x4[j0];
								mb_predict_mv(ref_idx_cache, mv_cache, idx, 1, mvp, -1);
								get_motion_vector(&currSE, dP, dec_params, mvp);
								
								// update the mv cache
								mv_cache[cache_scan_array[idx]] = *(int*)mvp;
							}
							break;
						case D_L1_8x4:			// 8x4
						case D_BI_8x4:
							for( j0 = 0; j0 < 2; j0++ )
							{
								int idx = index8x8[i0] + (j0*4);
								mb_predict_mv(ref_idx_cache, mv_cache, idx, 2, mvp, -1);
								get_motion_vector(&currSE, dP, dec_params, mvp);
								
								// update the mv cache
								mv_cache[cache_scan_array[idx]] = *(int*)mvp;
								mv_cache[cache_scan_array[idx+1]] = *(int*)mvp;
							}
							break;
						case D_L1_4x8:			// 4x8
						case D_BI_4x8:
							for( j0 = 0; j0 < 2; j0++ )
							{
								int idx = index8x8[i0] + j0;
								mb_predict_mv(ref_idx_cache, mv_cache, idx, 1, mvp, -1);
								get_motion_vector(&currSE, dP, dec_params, mvp);
								
								// update the mv cache
								mv_cache[cache_scan_array[idx]] = *(int*)mvp;
								mv_cache[cache_scan_array[idx+4]] = *(int*)mvp;
							}
							break;
						case D_L1_8x8:			// 8x8
						case D_BI_8x8:
							{
								int idx = index8x8[i0];
								mb_predict_mv(ref_idx_cache, mv_cache, idx, 2, mvp, -1);
								get_motion_vector(&currSE, dP, dec_params, mvp);
								
								// update the mv cache
								mv_cache[cache_scan_array[idx+0]] = *(int*)mvp;
								mv_cache[cache_scan_array[idx+1]] = *(int*)mvp;
								mv_cache[cache_scan_array[idx+4]] = *(int*)mvp;
								mv_cache[cache_scan_array[idx+5]] = *(int*)mvp;
							}
							break;
						}
					}
				}
				// save the motion vectors
				for(i = 0; i < 4; i++)
				{
					*(mvL1 ++) = mv_cache[ cache_scan_array[(i*4) + 0] ];
					*(mvL1 ++) = mv_cache[ cache_scan_array[(i*4) + 1] ];
					*(mvL1 ++) = mv_cache[ cache_scan_array[(i*4) + 2] ];
					*(mvL1 ++) = mv_cache[ cache_scan_array[(i*4) + 3] ];
				}
			}
		}
	}
	else// not 8x8 (max partition num = 2
	{
		// ref_idx0
		// !Direct
		if(img->currMB.mb_type != B_DIRECT)
		{
			short refidx[2];
			currSE.type = SE_REFFRAME;
			dP = &(currSlice->partArr[0]);
			currSE.mapping = linfo_ue;

			if(img->num_ref_idx_l0_active > 1)
			{
				for(i0 = 0; i0 < img->currMB.mb_num_of_part; i0++)
				{
					refidx[i0] = read_idx_from_NAL_mb(img->currMB.part_pred_dir[i0],img->num_ref_idx_l0_active,0,&currSE,dP,dec_params);
				}
				set_ref_idx_mb (img->currMB.partition, refidx, curr_ref_idxl0);
			}
			else
			{
				for(i0=0; i0<4;i0++)
					*((int*)(curr_ref_idxl0) + i0) = 0;
			}

			// ref_idx_1
			if(img->profile_idc > 66)
			{
				if( img->num_ref_idx_l1_active > 1)
				{
					for(i0 = 0; i0 < img->currMB.mb_num_of_part; i0++)
					{					
						refidx[i0] = read_idx_from_NAL_mb(img->currMB.part_pred_dir[i0],img->num_ref_idx_l1_active,1,&currSE,dP,dec_params);					
					}
					set_ref_idx_mb (img->currMB.partition, refidx, curr_ref_idxl1);
				}
				else
				{
					for(i0=0; i0<4;i0++)
						*((int*)(curr_ref_idxl1) + i0) = 0;
				}
			}
			{
				char  ref_idx_cache[30];
				int mv_cache[30];
				int mv_width = img->width >>2; // no of motion vectors in array
				int*  mvL0 =  ((int*)dec_picture->mvL0)  + (mb_nr<<4);
				dP = &(currSlice->partArr[0]);
							
				// read the motion vectors
				switch(img->currMB.partition)
				{
				case D_16x16:
					{
						short mvp[2];
						// read motion vector for list 0
						if(img->currMB.part_pred_dir[0] != PRED_DIR_L1)
						{
							fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl0, (int *)dec_picture->mvL0 );
							mb_predict_mv(ref_idx_cache, mv_cache, 0, 4, mvp, D_16x16);
							get_motion_vector(&currSE, dP, dec_params, mvp);

							for(i = 0; i < 4; i++)
							{
								*(mvL0 ++) = *(int*)mvp;
								*(mvL0 ++) = *(int*)mvp;
								*(mvL0 ++) = *(int*)mvp;
								*(mvL0 ++) = *(int*)mvp;
							}
						}
						
						// read motion vector for list 1
						if(img->currMB.part_pred_dir[0] != PRED_DIR_L0)
						{
							int* mvL1 = ((int*)dec_picture->mvL1)+ (mb_nr<<4);
							fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl1, (int *)dec_picture->mvL1 );
							mb_predict_mv(ref_idx_cache, mv_cache, 0, 4, mvp, D_16x16);
							get_motion_vector(&currSE, dP, dec_params, mvp);
							for(i = 0; i < 4; i++)
							{
								*(mvL1 ++) = *(int*)mvp;
								*(mvL1 ++) = *(int*)mvp;
								*(mvL1 ++) = *(int*)mvp;
								*(mvL1 ++) = *(int*)mvp;
							}
						}
					}
					break;
				case D_16x8:
					{
						short mvp[2][2] = { {0,0} , {0,0} };
						// read the motion vector for list 0
						fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl0, (int *)dec_picture->mvL0 );
						for(i = 0; i < 2; i++)
						{
							if(img->currMB.part_pred_dir[i] != PRED_DIR_L1)
							{
								mb_predict_mv(ref_idx_cache, mv_cache, i*8, 4, mvp[i], D_16x8);
								get_motion_vector(&currSE, dP, dec_params, mvp[i]);
								
								// update the mv cache
								mv_cache[cache_scan_array[i*8 + 4]] = *(int*)mvp[i];
							}
						}
						
						for(i = 0; i < 2; i++)
						{
							*(mvL0 ++) = *(int*)mvp[0];
							*(mvL0 ++) = *(int*)mvp[0];
							*(mvL0 ++) = *(int*)mvp[0];
							*(mvL0 ++) = *(int*)mvp[0];
						}
						for(i = 0; i < 2; i++)
						{
							*(mvL0 ++) = *(int*)mvp[1];
							*(mvL0 ++) = *(int*)mvp[1];
							*(mvL0 ++) = *(int*)mvp[1];
							*(mvL0 ++) = *(int*)mvp[1];
						}
						
						// read the motion vector for list 1
						if(img->profile_idc > 66)
						{
							int* mvL1 = ((int*)dec_picture->mvL1)+ (mb_nr<<4);
							fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl1, (int *)dec_picture->mvL1 );
							for(i = 0; i < 2; i++)
							{
								if(img->currMB.part_pred_dir[i] != PRED_DIR_L0)
								{
									mb_predict_mv(ref_idx_cache, mv_cache, i*8, 4, mvp[i], D_16x8);
									get_motion_vector(&currSE, dP, dec_params, mvp[i]);
									
									// update the mv cache
									mv_cache[cache_scan_array[i*8 + 6]] = *(int*)mvp[i];
								}
							}
							for(i = 0; i < 2; i++)
							{
								*(mvL1 ++) = *(int*)mvp[0];
								*(mvL1 ++) = *(int*)mvp[0];
								*(mvL1 ++) = *(int*)mvp[0];
								*(mvL1 ++) = *(int*)mvp[0];
							}
							for(i = 0; i < 2; i++)
							{
								*(mvL1 ++) = *(int*)mvp[1];
								*(mvL1 ++) = *(int*)mvp[1];
								*(mvL1 ++) = *(int*)mvp[1];
								*(mvL1 ++) = *(int*)mvp[1];
							}
						}
					}
					break;
				case D_8x16:
					// read the motion vector for list 0
					{
						short mvp[2][2] = { {0,0} , {0,0} };
						fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl0, (int *)dec_picture->mvL0 );
						for(i = 0; i < 2; i++)
						{
							if(img->currMB.part_pred_dir[i] != PRED_DIR_L1)
							{
								mb_predict_mv(ref_idx_cache, mv_cache, i*2, 2, mvp[i], D_8x16);
								get_motion_vector(&currSE, dP, dec_params, mvp[i]);
								
								// update the mv cache
								mv_cache[cache_scan_array[i*2 + 1]] = *(int*)mvp[i];
							}
						}
						
						for(i = 0; i < 4; i++)
						{
							*(mvL0 ++) = *(int*)mvp[0];
							*(mvL0 ++) = *(int*)mvp[0];
							*(mvL0 ++) = *(int*)mvp[1];
							*(mvL0 ++) = *(int*)mvp[1];
						}
						
						if(img->profile_idc > 66)
						{
							int* mvL1 = ((int*)dec_picture->mvL1)+ (mb_nr<<4);
							// read the motion vector for list 1
							fill_structure_mv_idx(img, ref_idx_cache, (int *)mv_cache, curr_ref_idxl1, (int *)dec_picture->mvL1 );
							for(i = 0; i < 2; i++)
							{
								if(img->currMB.part_pred_dir[i] != PRED_DIR_L0)
								{
									mb_predict_mv(ref_idx_cache, mv_cache, i*2, 4, mvp[i], D_8x16);
									get_motion_vector(&currSE, dP, dec_params, mvp[i]);
									
									// update the mv cache
									mv_cache[cache_scan_array[i*2 + 1]] = *(int*)mvp[i];
								}
							}
							for(i = 0; i < 4; i++)
							{
								*(mvL1 ++) = *(int*)mvp[0];
								*(mvL1 ++) = *(int*)mvp[0];
								*(mvL1 ++) = *(int*)mvp[1];
								*(mvL1 ++) = *(int*)mvp[1];
							}
						}
					}
					break;
				}				
			}
		}
	}
}

/*!
************************************************************************
* \brief
*    Get coded block pattern and coefficients (run/level)
*    from the NAL
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
*    "Nabeel Iqbal" <nabeel.iqbal@inforient.com>
*   -Changes: Generated the row column bimap for variable complexity inverse
*     Discrete Cosine Transform. coeffiecients for IDCT are transposed so that
*     all data should be in row major order at the image formation step.
*  
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/

unsigned char block_scan[] = {0,1,4,5, 2,3,6,7 ,8,9,12,13, 10,11,14,15};

int cache_scan_5[16] = 
{
	 6,  7,  8,  9,
	11, 12, 13, 14,
	16, 17, 18, 19,
	21, 22, 23, 24
};

int cache_scan_3[4] = 
{
	4, 5,
	7, 8
};

#define RS1(r) ((r) >> 1)

void readCBPandCoeffsFromNAL(h264_decoder* dec_params)
{
	ImageParameters* img = dec_params->img;
	int i,j,k ;
	
	int cbp;
	SyntaxElement currSE;
	Slice *currSlice = img->currentSlice;
	DataPartition *dP;

	//int chroma_qp_index_offset = dec_params->active_pps->chroma_qp_index_offset;
	
	int coef_ctr, i0, j0;
	int ll;
	int block_x,block_y;
	
	int levarr[16], runarr[16], numcoeff;
	
	int intra     = IS_INTRA_1 (img->currMB);
	int is_newintra = IS_NEWINTRA_1(img->currMB);
	int mb_nnz ;
	int init_done = 0;

	int mb_cbp_luma;
	
	int mf;
	int qbits;
	
	memset(img->run_idct,0,sizeof(img->run_idct));
	// read CBP if not new intra mode ( !I_16X16 , !IPCM )
	if (!is_newintra)
	{
		//=====   C B P   =====
		//---------------------
		if (IS_OLDINTRA_1 (img->currMB) )		// I_4x4
		{
			currSE.type = SE_CBP_INTRA;
			currSE.mapping = linfo_cbp_intra;			
		}
		else
		{
			currSE.type = SE_CBP_INTER;
			currSE.mapping = linfo_cbp_inter;			
		}

		dP = &(currSlice->partArr[0]);
		
		readSyntaxElement_VLC(&currSE, dP->bitstream);
		img->currMB.cbp = cbp = currSE.value1;
		//=====   DQUANT   =====
		//----------------------
		// Delta quant only if nonzero coeffs
		if (cbp !=0)
		{
			if (IS_INTER_1 (img->currMB))
				currSE.type = SE_DELTA_QUANT_INTER;
			else 
				currSE.type = SE_DELTA_QUANT_INTRA;
			
			currSE.mapping = linfo_se;
			
			dP = &(currSlice->partArr[0]);
			readSyntaxElement_VLC(&currSE, dP->bitstream);		// decode qp
			
			img->qp= ((img->qp + currSE.value1 + 52 )%(52));						
		}		
	}
	else // NEW_INTRA (I_16x16 , IPCM )
	{
		///ppp pragma frequency hit never
		cbp = img->currMB.cbp;				// no cbp comes for NEW_INTRA
		currSE.type = SE_DELTA_QUANT_INTRA;
		dP = &(currSlice->partArr[0]);
		
		currSE.mapping = linfo_se;
#if TRACE
		snprintf(currSE.tracestring, TRACESTRING_SIZE, "Delta quant ");
#endif
		readSyntaxElement_VLC(&currSE, dP->bitstream);		// decode qp		
	
		img->qp = (img->qp + currSE.value1+ 52 )%(52) ;
		
		if(img->currMB.mbAvailA && img->currMB.mbAvailB)
		{
			mb_nnz = ( *(img->nz_coeff1 + (img->currMB.mbAddrA*24) +  3 ) + 
					   *(img->nz_coeff1 + (img->currMB.mbAddrB*24) + 12 ) + 1 )>>1;
		}
		else if(img->currMB.mbAvailA)
		{
			mb_nnz = *(img->nz_coeff1 + (img->currMB.mbAddrA*24) +  3 );
		}
		else if(img->currMB.mbAvailB)
		{
			mb_nnz = *(img->nz_coeff1 + (img->currMB.mbAddrB*24) + 12 ) ;
		}
		else
		{
			mb_nnz = 0;		
		}

		numcoeff = 16; // luma DC maximum coefficients to read
		readCoeff4x4_CAVLC_AC(img, 0, mb_nnz, levarr, runarr, &numcoeff);

		memset( img->dc_cof_luma, 0, sizeof(img->dc_cof_luma) );
		memset( img->cof_s, 0, sizeof(img->cof_s) );
		init_done = 1;

		qbits = img->qp/6 - 6;
	
		coef_ctr=-1;

		for(k = 0; k < numcoeff; k++)
		{
			coef_ctr=coef_ctr+runarr[k]+1;
			
			i0 = SNGL_SCAN_t[coef_ctr][0];
			j0 = SNGL_SCAN_t[coef_ctr][1];
			
			img->dc_cof_luma[i0][j0] = levarr[k];// add new intra DC coeff
		}

		if(numcoeff)
		{
			//if(!img->currMB.cbp)
			img->currMB.cbp |= 1;
			for(i0 = 0; i0 < 16; i0++)
			{
				img->run_idct[i0] = 1;
			}
		}
	}

	img->currMB.qp = img->qp;
	mf = img->qp%6;
	qbits = img->qp/6 - 4;

	if(cbp !=0) //many macro blocks contain no coefficient 
	{
		int mb_nr = img->current_mb_nr << 5;
		int block_idx;

		if(!init_done)
			memset( img->cof_s, 0, sizeof(img->cof_s) );
		
		mb_cbp_luma   = cbp & 0x0f;
	
		if (mb_cbp_luma) 
		{
			short nz_coeff;
			char nz_coeff_cache[25];
			
			memset(nz_coeff_cache, 0, 25);

			if(img->currMB.mbAvailA && img->currMB.mbAvailB)
			{
                //*(int*)(&nz_coeff_cache[1]) = *(int*)(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12);
                nz_coeff_cache[1] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 0);
                nz_coeff_cache[2] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 1);
                nz_coeff_cache[3] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 2);
                nz_coeff_cache[4] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 3);
                nz_coeff_cache[5] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 3);
                nz_coeff_cache[10] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 7);
                nz_coeff_cache[15] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 11);
                nz_coeff_cache[20] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 15);
			}
			else if(img->currMB.mbAvailA)
			{
                //*(int*)(&nz_coeff_cache[1]) = 0x40404040;
                nz_coeff_cache[1]  = 0x40;
                nz_coeff_cache[2]  = 0x40;
                nz_coeff_cache[3]  = 0x40;
                nz_coeff_cache[4]  = 0x40;
                nz_coeff_cache[5]  = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 3);
                nz_coeff_cache[10] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 7);
                nz_coeff_cache[15] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 11);
                nz_coeff_cache[20] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 15);
			}
			else if(img->currMB.mbAvailB)
			{
                //*(int*)(&nz_coeff_cache[1]) = *(int*)(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12);
                *(&nz_coeff_cache[1]) = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 0);
                *(&nz_coeff_cache[2]) = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 1);
                *(&nz_coeff_cache[3]) = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 2);
                *(&nz_coeff_cache[4]) = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 12 + 3);
                nz_coeff_cache[ 5] = 0x40;
                nz_coeff_cache[10] = 0x40;
                nz_coeff_cache[15] = 0x40;
                nz_coeff_cache[20] = 0x40;
			}
			else
			{
                //*(int*)(&nz_coeff_cache[1]) = 0x40404040;
                nz_coeff_cache[1]  = 0x40;
                nz_coeff_cache[2]  = 0x40;
                nz_coeff_cache[3]  = 0x40;
                nz_coeff_cache[4]  = 0x40;
                nz_coeff_cache[ 5] = 0x40;
                nz_coeff_cache[10] = 0x40;
                nz_coeff_cache[15] = 0x40;
                nz_coeff_cache[20] = 0x40;
			}
			
			///////////// changes added for getNeighbor removel end/////
			// luma AC coefficients
			if(dec_params->img->number == 1 && dec_params->img->current_mb_nr ==379)
					dec_params->img->current_mb_nr = dec_params->img->current_mb_nr;
			for(block_x =0; (block_x <16) ;block_x++)		
			{
				if(!(cbp & (1<<(block_x>>2))))  // are there any coeff in current block at all 
				{
					block_x += 3; // no coefficient in block then remaining 3 are also empty
					continue;
				}				

				block_idx = block_scan[block_x];

				if (!is_newintra)
				{
					numcoeff = 16;
					coef_ctr = -1;					
				}
				else
				{
					///ppp pragma frequency hit never
					numcoeff = 15;
					coef_ctr = 0;
				}				
				nz_coeff = nz_coeff_cache[cache_scan_5[block_idx] - 5] + 
						   nz_coeff_cache[cache_scan_5[block_idx] - 1];
				if( nz_coeff < 0x40 )
				{
					nz_coeff = ( nz_coeff + 1) >> 1;
				}
				mb_nnz = nz_coeff & 0x3f;

				readCoeff4x4_CAVLC_AC(img, 0, mb_nnz, levarr, runarr, &numcoeff);

				nz_coeff_cache[cache_scan_5[block_idx]] = numcoeff;

				//coef_ctr = 0;
				if( qbits >= 0 )
				{
					for (k = 0; k < numcoeff; k++)
					{
						coef_ctr  += runarr[k]+1;
						
						i0=SNGL_SCAN_t[coef_ctr][0];
						j0=SNGL_SCAN_t[coef_ctr][1];
			
						img->cof_s[block_idx][j0][i0] = (levarr[k] * dequant_coef[mf][i0][j0]) << qbits;
					}
				}
				else
				{
					const int f = 1 << (-qbits-1);
					for (k = 0; k < numcoeff; k++)
					{
						coef_ctr  += runarr[k]+1;
						
						i0=SNGL_SCAN_t[coef_ctr][0];
						j0=SNGL_SCAN_t[coef_ctr][1];
						
						img->cof_s[block_idx][j0][i0] = (levarr[k] * dequant_coef[mf][i0][j0] + f) >>(-qbits);
					}
				}
				if (numcoeff) 
				{
					img->currMB.cbp_blk |= 1 << (((block_idx>>2)<<2) + (block_idx&3)); // cbp_blk for loop filer 
					img->run_idct[block_idx] = 1;
				}
			}
            
            
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 )    = *(int*)(&nz_coeff_cache[6]);
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 + 4) = *(int*)(&nz_coeff_cache[11]);
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 + 8) = *(int*)(&nz_coeff_cache[16]);
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 +12) = *(int*)(&nz_coeff_cache[21]);
            
            
            
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 )    = *(int*)(&nz_coeff_cache[6]);
            *(img->nz_coeff1 + img->current_mb_nr*24 )      =  nz_coeff_cache[6];
            *(img->nz_coeff1 + img->current_mb_nr*24 +1)    =  nz_coeff_cache[7];
            *(img->nz_coeff1 + img->current_mb_nr*24 +2)    =  nz_coeff_cache[8];
            *(img->nz_coeff1 + img->current_mb_nr*24 +3)    =  nz_coeff_cache[9];
            
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 + 4) = *(int*)(&nz_coeff_cache[11]);
            *(img->nz_coeff1 + img->current_mb_nr*24 + 4)       =  nz_coeff_cache[11];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 4 +1)    =  nz_coeff_cache[12];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 4 +2)    =  nz_coeff_cache[13];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 4 +3)    =  nz_coeff_cache[14];
            
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 + 8) = *(int*)(&nz_coeff_cache[16]);
            *(img->nz_coeff1 + img->current_mb_nr*24 + 8)       =  nz_coeff_cache[16];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 8 +1)    =  nz_coeff_cache[17];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 8 +2)    =  nz_coeff_cache[18];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 8 +3)    =  nz_coeff_cache[19];
            
            //*(int*)(img->nz_coeff1 + img->current_mb_nr*24 +12) = *(int*)(&nz_coeff_cache[21]);
            *(img->nz_coeff1 + img->current_mb_nr*24 + 12)       =  nz_coeff_cache[21];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 12 +1)    =  nz_coeff_cache[22];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 12 +2)    =  nz_coeff_cache[23];
            *(img->nz_coeff1 + img->current_mb_nr*24 + 12 +3)    =  nz_coeff_cache[24];          
			
		} // if (mb_cbp_luma)
		
		
		//========================== CHROMA DC ============================
		//-----------------------------------------------------------------
		// chroma DC coeff
		
		if(cbp>15)
		{
			memset( img->dc_cof_chroma, 0, sizeof(img->dc_cof_chroma) );
			for (ll=0;ll<3;ll+=2)
			{
				//===================== CHROMA DC YUV420 ======================
				numcoeff = 4;
				
				readCoeff4x4_CAVLC_AC(img, 1, 0, levarr, runarr, &numcoeff);
				
				coef_ctr=-1;
				
				for(k = 0; k < numcoeff; k++)
				{
					coef_ctr=coef_ctr+runarr[k]+1;
					img->dc_cof_chroma[ll>>1][coef_ctr] = levarr[k];
				}
				if(numcoeff)
				{
					for(k = 0; k < 4; k++)
					{
						img->run_idct[16 + (ll<<1) + k] = 1;
					}
				}
			}//for (ll=0;ll<3;ll+=2)
		}
		//========================== CHROMA AC ============================
		//-----------------------------------------------------------------
		// chroma AC coeff, all zero from start_scan
		
		if (cbp>31)
		{
			short nz_coeff;
			char nz_coeff_cache[2][9];	// for U and V
			int qp = chroma_qp_table[Clip3( 0, 51, img->currMB.qp + dec_params->active_pps->chroma_qp_index_offset )];

			mf = qp%6;
			qbits = qp/6 -4;
			
			memset(nz_coeff_cache, 0, 18);

			if(img->currMB.mbAvailA && img->currMB.mbAvailB)
			{
                        //*(short*)(&nz_coeff_cache[0][1]) = *(short*)(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 2);
                        nz_coeff_cache[0][1] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 2 + 0);
                        nz_coeff_cache[0][2] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 2 + 1);
                        
                        //*(short*)(&nz_coeff_cache[1][1]) = *(short*)(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 4 + 2);
                        nz_coeff_cache[1][1] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 4 + 2 + 0);
                        nz_coeff_cache[1][2] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 4 + 2 + 1);
				nz_coeff_cache[0][3] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 16 + 1);
				nz_coeff_cache[0][6] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 16 + 3);
				nz_coeff_cache[1][3] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 +(16+4)+1);
				nz_coeff_cache[1][6] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 +(16+4)+3);
			}
			else if(img->currMB.mbAvailA)
			{
                        //*(short*)(&nz_coeff_cache[0][1]) = 0x4040;
                        nz_coeff_cache[0][1] = 0x40;
                        nz_coeff_cache[0][2] = 0x40;
                        
                        //*(short*)(&nz_coeff_cache[1][1]) = 0x4040;
                        nz_coeff_cache[1][1] = 0x40;
                        nz_coeff_cache[1][2] = 0x40;
				nz_coeff_cache[0][3] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 16 + 1);
				nz_coeff_cache[0][6] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 + 16 + 3);
				nz_coeff_cache[1][3] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 +(16+4)+1);
				nz_coeff_cache[1][6] = *(img->nz_coeff1 + img->currMB.mbAddrA*24 +(16+4)+3);
			}
			else if(img->currMB.mbAvailB)
			{	
                        //*(short*)(&nz_coeff_cache[0][1]) = *(short*)(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 2);
                        nz_coeff_cache[0][1] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 2 + 0);
                        nz_coeff_cache[0][2] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 2 + 1);
                        
                        //*(short*)(&nz_coeff_cache[1][1]) = *(short*)(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 4 + 2);
                        nz_coeff_cache[1][1] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 4 + 2 + 0);
                        nz_coeff_cache[1][2] = *(img->nz_coeff1 + img->currMB.mbAddrB*24 + 16 + 4 + 2 + 1);

				nz_coeff_cache[0][3] = 0x40;
				nz_coeff_cache[0][6] = 0x40;
				nz_coeff_cache[1][3] = 0x40;
				nz_coeff_cache[1][6] = 0x40;
			}
			else
			{
                        //*(short*)(&nz_coeff_cache[0][1]) = 0x4040;
                        nz_coeff_cache[0][1] = 0x40;
                        nz_coeff_cache[0][2] = 0x40;
                        
                        //*(short*)(&nz_coeff_cache[1][1]) = 0x4040;
                        nz_coeff_cache[1][1] = 0x40;
                        nz_coeff_cache[1][2] = 0x40;

				nz_coeff_cache[0][3] = 0x40;
				nz_coeff_cache[0][6] = 0x40;
				nz_coeff_cache[1][3] = 0x40;
				nz_coeff_cache[1][6] = 0x40;
			}
			
			for (i =16; i < 24; i++)
			{
				numcoeff = 15;
				nz_coeff = nz_coeff_cache[(i&4)>>2][ cache_scan_3[i%4] - 1 ] + 
						   nz_coeff_cache[(i&4)>>2][ cache_scan_3[i%4] - 3 ];

				if( nz_coeff < 0x40 )
				{
					nz_coeff = (nz_coeff + 1) >> 1;
				}
				mb_nnz = nz_coeff & 0x3F;

				readCoeff4x4_CAVLC_AC(img, 2, mb_nnz, levarr, runarr, &numcoeff);
				nz_coeff_cache[(i&4)>>2][ cache_scan_3[i%4] ] = numcoeff;

				coef_ctr=0;
				for(k = 0; k < numcoeff;k++)
				{
					coef_ctr=coef_ctr+runarr[k]+1;
					i0=SNGL_SCAN_t[coef_ctr][0];
					j0=SNGL_SCAN_t[coef_ctr][1];
					
					if( qbits >= 0 )
					{
						img->cof_s[i][j0][i0] = (levarr[k] * dequant_coef[mf][i0][j0]) << qbits;
					}
					else
					{
						const int f = 1 << (-qbits-1);
						img->cof_s[i][j0][i0] = (levarr[k] * dequant_coef[mf][i0][j0] + f) >>(-qbits);
					}
				}
				if (numcoeff) 
				{
					img->run_idct[i] = 1;
				}
			}
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+16 )        = *(short*)(&nz_coeff_cache[0][4]);
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+16 + 2) = *(short*)(&nz_coeff_cache[0][7]);
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+20 )        = *(short*)(&nz_coeff_cache[1][4]);
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+20 + 2) = *(short*)(&nz_coeff_cache[1][7]);
                    
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+16 )        = *(short*)(&nz_coeff_cache[0][4]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+16 )        = *(&nz_coeff_cache[0][4]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+16 +1)      = *(&nz_coeff_cache[0][5]);
                    
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+16 + 2) = *(short*)(&nz_coeff_cache[0][7]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+18 )        = *(&nz_coeff_cache[0][7]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+18 +1)      = *(&nz_coeff_cache[0][8]);
                    
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+20 )        = *(short*)(&nz_coeff_cache[1][4]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+20 )        = *(&nz_coeff_cache[1][4]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+20 +1)      = *(&nz_coeff_cache[1][5]);
                    
                    //*(short*)(img->nz_coeff1 + img->current_mb_nr*24+20 + 2) = *(short*)(&nz_coeff_cache[1][7]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+22 )        = *(&nz_coeff_cache[1][7]);
                    *(img->nz_coeff1 + img->current_mb_nr*24+22 +1)      = *(&nz_coeff_cache[1][8]);
		}//if(cbp>31)
		
	}// if(cbp !=0)
}


/*!
************************************************************************
* \brief
*    Copy IPCM coefficients to decoded picture buffer and set parameters for this MB
*    (for IPCM CABAC and IPCM CAVLC  28/11/2003)
*
* \author
*    Dong Wang <Dong.Wang@bristol.ac.uk>
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void decode_ipcm_mb( h264_decoder* dec_params )
{
    ImageParameters *img=dec_params->img;
    int i,j;
    int stride = img->width+64;
    int stride_cr = img->width_cr+32;
    Macroblock *currMb = &img->mb_data[img->current_mb_nr];
    imgpel* imgY;//  = dec_params->dec_picture->imgY + (img->pix_y + 32)*(stride) + img->pix_x + 32;
    imgpel* imgUV;// = NULL;
    stride-=16;
    //Copy coefficients to decoded picture buffer
    //IPCM coefficients are stored in img->cof which is set in function readIPCMcoeffsFromNAL()
    
    for(i=0;i<16;i++)
    {
        for(j=0;j<16;j++)
        {
            //dec_outputs->dec_picture->imgY[img->pix_y+i + 32][img->pix_x+j + 32] = img->cof_s[(i>>2)][(j>>2)][i%4][j%4];
//            *(imgY) = img->cof_s[(i>>2)][(j>>2)][i%4][j%4];
            imgY++;
        }
        imgY+=stride;
    }
    //imgUV = dec_outputs->dec_picture->imgU + (img->pix_y + 16)*(stride_cr) + img->pix_x + 16;
//	imgUV = dec_params->dec_picture->imgU + (img->pix_c_y + 16)*(stride_cr) + img->pix_c_x + 16;
    stride_cr-=MB_CR_SIZE_X;
    for(i=0;i<MB_CR_SIZE_Y;i++)
    {
        for(j=0;j<MB_CR_SIZE_X;j++)
        {
            //dec_outputs->dec_picture->imgUV[0][img->pix_c_y+i + 16][img->pix_c_x+j + 16] = img->cof_s[(i>>2)][(j>>2)+4][i%4][j%4];   //TODO-VG
//            *(imgUV) = img->cof_s[(i>>2)][(j>>2)+4][i%4][j%4];   //TODO-VG
            imgUV++;
        }
        imgUV+=stride_cr;
    }
    stride_cr+=MB_CR_SIZE_X;
//    imgUV = dec_params->dec_picture->imgV + (img->pix_c_y + 16)*(stride_cr) + img->pix_c_x + 16;
    stride_cr-=MB_CR_SIZE_X;
    for(i=0;i<MB_CR_SIZE_Y;i++)
    {
        for(j=0;j<MB_CR_SIZE_X;j++)
        {
            //dec_outputs->dec_picture->imgUV[1][img->pix_c_y+i + 16][img->pix_c_x+j + 16] = img->cof_s[(i>>2)+2][(j>>2)+4][i%4][j%4]; //TODO-VG
//            *(imgUV) = img->cof_s[(i>>2)+2][(j>>2)+4][i%4][j%4]; //TODO-VG
            imgUV++;
        }
        imgUV+=stride_cr;
    }
    // for deblocking filter
    currMb->qp=0;
    
    // for CAVLC: Set the nz_coeff to 16. 
    // These parameters are to be used in CAVLC decoding of neighbour blocks
    for(i=0;i<4;i++)
    {
        for (j=0;j<6;j++)
        {

            *(img->nz_coeff+(img->current_mb_nr*32)+(i*8)+j)=16;
        }
    }
    
    // for CABAC decoding of MB skip flag 
    currMb->skip_flag = 0;
    //for deblocking filter CABAC
    currMb->cbp_blk=0xFFFF;
    //For CABAC decoding of Dquant
    dec_params->last_dquant=0;
}

/*!
************************************************************************
* \brief
*    decodes one macroblock
*
* \author
*    Dong Wang <Dong.Wang@bristol.ac.uk>
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
*-----------------------------------------------------------------------
*   Changes:
*   1. Unnecessary referencing was removed
*   2. Removed copying from img->cof_s array to dec_picture only where
*
*   Humza Shahid
*---------------------------------------------------------------------
*	- Code not related to baseline removed.
*		- img->profile_idc != 66
*		- active_pps->weighted_pred_flag,
*		- active_pps->weighted_bipred_idc
*		- img->residue_transform_flag
*		- active_sps->frame_mbs_only_flag
*		- sps->chroma_format_idc != YUV420 || YUV400,
*		- currMB->luma_transform_size_8x8_flag,
*		- pps->transform_8x8_mode_flag
*		- active_pps->entropy_coding_mode_flag == CABAC
*		- img->MbaffFrameFlag
*		- currMB->mb_field
*		- img->field_pic_flag 
*		- img->type == SP_SLICE
*		- img->type == SI_SLICE
*		- img->type == B_SLICE
*	 (if the above flags are available than the related code is not of
*	  baseline so was removed)
*	- Changed the arithmatic operation in the code to utilize shifts 
*	  operations.
*	- Code restructured so that itrans_baseline()/itrans_baseline_t() is
*	  called for every mv_mode seperately.(this has been done to facilitate
*	  the handling of the matrix img->mpr which in the case of 
*	  intrapred_luma_16x16() and intrapred_chroma() is transposed; so 
*	  intrans_baseline() is called otherwise intrans_baseline_t() is called.)
*	
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
*   - Code restructured and rewritten to handle B-Frames <imran.munir@inforient.com>
****************************
*	 Changes till 28-11-2005
***********************************************************************
*/

#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))
 

static __inline void mb_mc_0xywh( h264_decoder* dec_params,StorablePicture* dec_picture, int x, int y, int width, int height )
{
	int i,j;
	imgpel* temp1;
    ImageParameters* img = dec_params->img;

    const int stride_luma = dec_picture->stride_luma;
    const int stride_chroma = dec_picture->stride_chroma;
    const int i_ref = *(dec_picture->ref_idx_l0 + (img->current_mb_nr*16) + (y*4) + x);
	const short mvx = *(dec_picture->mvL0 + (img->current_mb_nr<<5) +(y<<3) + (x<<1));
    const short mvy = *(dec_picture->mvL0 + (img->current_mb_nr<<5) +(y<<3) + (x<<1) + 1);
	
	imgpel* planeY	= ( dec_params->listX[LIST_0][i_ref]->plane[0] + 
					    ((img->mb_y*16)*dec_params->listX[LIST_0][i_ref]->stride_luma) +
					    (img->mb_x*16) ) ;
	imgpel* planeU	= ( dec_params->listX[LIST_0][i_ref]->plane[1] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_0][i_ref]->stride_chroma) +
					    (img->mb_x*8) ) ;
	imgpel* planeV	= ( dec_params->listX[LIST_0][i_ref]->plane[2] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_0][i_ref]->stride_chroma) +
					    (img->mb_x*8) ) ;
	imgpel* imgYUV[3];    
	imgYUV[0] = dec_params->dec_picture->plane[0] + (img->mb_y*16*stride_luma) + (img->mb_x*16);
	imgYUV[1] = dec_params->dec_picture->plane[1] + (img->mb_y*8*stride_chroma) + (img->mb_x*8);
	imgYUV[2] = dec_params->dec_picture->plane[2] + (img->mb_y*8*stride_chroma) + (img->mb_x*8);                        
                        
    mc_luma( planeY, dec_params->listX[LIST_0][i_ref]->stride_luma,
        imgYUV[0] + (4*y*stride_luma)+(4*x), stride_luma,
        mvx + 4*4*x, mvy + 4*4*y, 4*width, 4*height );

    mc_chroma( planeU, dec_params->listX[LIST_0][i_ref]->stride_chroma,
               imgYUV[1] + (2*y*stride_chroma)+(2*x), stride_chroma,
               mvx + 4*4*x, mvy + 4*4*y, 2*width, 2*height );

	mc_chroma( planeV, dec_params->listX[LIST_0][i_ref]->stride_chroma,
               imgYUV[2] + (2*y*stride_chroma) + (2*x), stride_chroma,
               mvx + 4*4*x, mvy + 4*4*y, 2*width, 2*height );

}


static __inline void mb_mc_1xywh( h264_decoder* dec_params,StorablePicture* dec_picture, int x, int y, int width, int height )
{
	ImageParameters* img = dec_params->img;
    const int i_ref = *(dec_picture->ref_idx_l1 + (img->current_mb_nr*16) + (y*4) + x);
	const short mvx   = *(dec_picture->mvL1 + (img->current_mb_nr<<5) +(y<<3) + (x<<1));
    const short mvy   = *(dec_picture->mvL1 + (img->current_mb_nr<<5) +(y<<3) + (x<<1) + 1);
	
	imgpel* planeY	= ( dec_params->listX[LIST_1][i_ref]->plane[0] + 
					    ((img->mb_y*16)*dec_params->listX[LIST_1][i_ref]->stride_luma) +
					    (img->mb_x*16) ) ;
	imgpel* planeU	= ( dec_params->listX[LIST_1][i_ref]->plane[1] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_1][i_ref]->stride_chroma) +
					    (img->mb_x*8) ) ;
	imgpel* planeV	= ( dec_params->listX[LIST_1][i_ref]->plane[2] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_1][i_ref]->stride_chroma) +
					    (img->mb_x*8) ) ;
	
    mc_luma( planeY, dec_params->listX[LIST_1][i_ref]->stride_luma,
             (unsigned char *)(img->mpr1[0] + (4*y*16)+(4*x)), 16,
             mvx + 4*4*x, mvy + 4*4*y, 4*width, 4*height );

    mc_chroma( planeU, dec_params->listX[LIST_1][i_ref]->stride_chroma,
               (unsigned char *)(img->mpr1[16]+(2*y*8)+(2*x)), 8,
               mvx, mvy, 2*width, 2*height );

	mc_chroma( planeV, dec_params->listX[LIST_1][i_ref]->stride_chroma,
               (unsigned char *)(img->mpr1[20]+(2*y*8)+(2*x)), 8,
               mvx, mvy, 2*width, 2*height );
}

static __inline void mb_mc_01xywh( h264_decoder* dec_params,StorablePicture* dec_picture, int x, int y, int width, int height )
{
    ImageParameters* img = dec_params->img;
    const int i_ref1 = *(dec_picture->ref_idx_l1 + (img->current_mb_nr*16) + (y*4) + x);

	const short mvx1   = *(dec_picture->mvL1 + (img->current_mb_nr<<5) +(y<<3) + (x<<1));
    const short mvy1   = *(dec_picture->mvL1 + (img->current_mb_nr<<5) +(y<<3) + (x<<1) + 1);

	imgpel* planeY	= ( dec_params->listX[LIST_1][i_ref1]->plane[0] + 
					    ((img->mb_y*16)*dec_params->listX[LIST_1][i_ref1]->stride_luma) +
					    (img->mb_x*16) ) ;
	imgpel* planeU	= ( dec_params->listX[LIST_1][i_ref1]->plane[1] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_1][i_ref1]->stride_chroma) +
					    (img->mb_x*8) ) ;
	imgpel* planeV	= ( dec_params->listX[LIST_1][i_ref1]->plane[2] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_1][i_ref1]->stride_chroma) +
					    (img->mb_x*8) ) ;

    imgpel tmp[16*16];
    int mode = size2pixel[height][width];

    mb_mc_0xywh( dec_params, dec_picture, x, y, width, height );

	mc_luma( planeY, dec_params->listX[LIST_1][i_ref1]->stride_luma,
             tmp, 16, mvx1 + 4*4*x, mvy1 + 4*4*y, 4*width, 4*height );
/*    
    if( h->param.analyse.b_weighted_bipred )
    {
        const int i_ref0 = h->mb.cache.ref[0][i8];
        const int weight = h->mb.bipred_weight[i_ref0][i_ref1];

        h->mc.avg_weight[i_mode]( &h->mb.pic.p_fdec[0][4*y*FDEC_STRIDE+4*x], FDEC_STRIDE, tmp, 16, weight );

        h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref1][4][2*y*h->mb.pic.i_stride[1]+2*x], h->mb.pic.i_stride[1],
                          tmp, 16, mvx1, mvy1, 2*width, 2*height );
        h->mc.avg_weight[i_mode+3]( &h->mb.pic.p_fdec[1][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE, tmp, 16, weight );

        h->mc.mc_chroma( &h->mb.pic.p_fref[1][i_ref1][5][2*y*h->mb.pic.i_stride[2]+2*x], h->mb.pic.i_stride[2],
                          tmp, 16, mvx1, mvy1, 2*width, 2*height );
        h->mc.avg_weight[i_mode+3]( &h->mb.pic.p_fdec[2][2*y*FDEC_STRIDE+2*x], FDEC_STRIDE, tmp, 16, weight );
    }
    else
    {
	*/
		mc_avg[mode]( (unsigned char *)(img->mpr1[0] + (4*y*16)+(4*x)), 16, tmp, 16 );

        mc_chroma( planeU, dec_params->listX[LIST_1][i_ref1]->stride_chroma,
				   tmp, 8, mvx1, mvy1, 2*width, 2*height );
        
		mc_avg[mode+3]( (unsigned char *) (img->mpr1[16]+(2*y*8)+(2*x)), 8, tmp, 16 );

		mc_chroma( planeV, dec_params->listX[LIST_1][i_ref1]->stride_chroma,
				   tmp, 8, mvx1, mvy1, 2*width, 2*height );
        
		mc_avg[mode+3]( (unsigned char *)(img->mpr1[20]+(2*y*8)+(2*x)), 8, tmp, 16 );

   // }
}


static void mb_mc_direct8x8( h264_decoder *dec_params, int x, int y )
{
    //const int i8 = x264_scan8[0] + x + 8*y;
	ImageParameters* img = dec_params->img;
    const int ref0 = *(dec_params->dec_picture->ref_idx_l0 + (img->current_mb_nr*16) + (y*4) + x);
	const int ref1 = *(dec_params->dec_picture->ref_idx_l1 + (img->current_mb_nr*16) + (y*4) + x);

    // FIXME: optimize based on current block size, not global settings? 
    if( dec_params->sps->direct_8x8_inference_flag )
    {
        if( ref0 >= 0 )
            if( ref1 >= 0 )
                mb_mc_01xywh( dec_params, dec_params->dec_picture, x, y, 2, 2 );
            else
                mb_mc_0xywh( dec_params, dec_params->dec_picture, x, y, 2, 2 );
        else
            mb_mc_1xywh( dec_params, dec_params->dec_picture, x, y, 2, 2 );
    }
    else
    {
        if( ref0 >= 0 )
        {
            if( ref1 >= 0 )
            {
                mb_mc_01xywh( dec_params, dec_params->dec_picture, x+0, y+0, 1, 1 );
                mb_mc_01xywh( dec_params, dec_params->dec_picture, x+1, y+0, 1, 1 );
                mb_mc_01xywh( dec_params, dec_params->dec_picture, x+0, y+1, 1, 1 );
                mb_mc_01xywh( dec_params, dec_params->dec_picture, x+1, y+1, 1, 1 );
            }
            else
            {
                mb_mc_0xywh( dec_params, dec_params->dec_picture, x+0, y+0, 1, 1 );
                mb_mc_0xywh( dec_params, dec_params->dec_picture, x+1, y+0, 1, 1 );
                mb_mc_0xywh( dec_params, dec_params->dec_picture, x+0, y+1, 1, 1 );
                mb_mc_0xywh( dec_params, dec_params->dec_picture, x+1, y+1, 1, 1 );
            }
        }
        else
        {
            mb_mc_1xywh( dec_params, dec_params->dec_picture, x+0, y+0, 1, 1 );
            mb_mc_1xywh( dec_params, dec_params->dec_picture, x+1, y+0, 1, 1 );
            mb_mc_1xywh( dec_params, dec_params->dec_picture, x+0, y+1, 1, 1 );
            mb_mc_1xywh( dec_params, dec_params->dec_picture, x+1, y+1, 1, 1 );
        }
    }
}


__inline void mb_mc_8x8( h264_decoder* dec_params,StorablePicture* dec_picture, int i8 )
{
	
    const int x = 2*(i8&1);
    const int y = 2*(i8>>1);
    switch( dec_params->img->currMB.sub_partition[i8] )
    {
        case D_L0_8x8:
            mb_mc_0xywh( dec_params, dec_picture, x, y, 2, 2 );
            break;
        case D_L0_8x4:
            mb_mc_0xywh( dec_params, dec_picture, x, y+0, 2, 1 );
            mb_mc_0xywh( dec_params, dec_picture, x, y+1, 2, 1 );
            break;
        case D_L0_4x8:
            mb_mc_0xywh( dec_params, dec_picture, x+0, y, 1, 2 );
            mb_mc_0xywh( dec_params, dec_picture, x+1, y, 1, 2 );
            break;
        case D_L0_4x4:
            mb_mc_0xywh( dec_params, dec_picture, x+0, y+0, 1, 1 );
            mb_mc_0xywh( dec_params, dec_picture, x+1, y+0, 1, 1 );
            mb_mc_0xywh( dec_params, dec_picture, x+0, y+1, 1, 1 );
            mb_mc_0xywh( dec_params, dec_picture, x+1, y+1, 1, 1 );
            break;
        case D_L1_8x8:
            mb_mc_1xywh( dec_params, dec_picture, x, y, 2, 2 );
            break;
        case D_L1_8x4:
            mb_mc_1xywh( dec_params, dec_picture, x, y+0, 2, 1 );
            mb_mc_1xywh( dec_params, dec_picture, x, y+1, 2, 1 );
            break;
        case D_L1_4x8:
            mb_mc_1xywh( dec_params, dec_picture, x+0, y, 1, 2 );
            mb_mc_1xywh( dec_params, dec_picture, x+1, y, 1, 2 );
            break;
        case D_L1_4x4:
            mb_mc_1xywh( dec_params, dec_picture, x+0, y+0, 1, 1 );
            mb_mc_1xywh( dec_params, dec_picture, x+1, y+0, 1, 1 );
            mb_mc_1xywh( dec_params, dec_picture, x+0, y+1, 1, 1 );
            mb_mc_1xywh( dec_params, dec_picture, x+1, y+1, 1, 1 );
            break;
        case D_BI_8x8:
            mb_mc_01xywh( dec_params, dec_picture, x, y, 2, 2 );
            break;
        case D_BI_8x4:
            mb_mc_01xywh( dec_params, dec_picture, x, y+0, 2, 1 );
            mb_mc_01xywh( dec_params, dec_picture, x, y+1, 2, 1 );
            break;
        case D_BI_4x8:
            mb_mc_01xywh( dec_params, dec_picture, x+0, y, 1, 2 );
            mb_mc_01xywh( dec_params, dec_picture, x+1, y, 1, 2 );
            break;
        case D_BI_4x4:
            mb_mc_01xywh( dec_params, dec_picture, x+0, y+0, 1, 1 );
            mb_mc_01xywh( dec_params, dec_picture, x+1, y+0, 1, 1 );
            mb_mc_01xywh( dec_params, dec_picture, x+0, y+1, 1, 1 );
            mb_mc_01xywh( dec_params, dec_picture, x+1, y+1, 1, 1 );
            break;
        case D_DIRECT_8x8:
            mb_mc_direct8x8( dec_params, x, y );
            break;
    }
}


void mb_mc( h264_decoder* dec_params )
{
	ImageParameters* img = dec_params->img;

    if( img->currMB.mb_type == P_L0 )
    {
        if( img->currMB.partition == D_16x16 )
        {
            mb_mc_0xywh( dec_params, dec_params->dec_picture, 0, 0, 4, 4 );
        }
        else if( img->currMB.partition == D_16x8 )
        {
			
            mb_mc_0xywh( dec_params, dec_params->dec_picture, 0, 0, 4, 2 );
            mb_mc_0xywh( dec_params, dec_params->dec_picture, 0, 2, 4, 2 );
			
        }
        else if( img->currMB.partition == D_8x16 )
        {
			mb_mc_0xywh( dec_params, dec_params->dec_picture, 0, 0, 2, 4 );
            mb_mc_0xywh( dec_params, dec_params->dec_picture, 2, 0, 2, 4 );
        }
    }
//#if 0
    else if( img->currMB.mb_type == P_8x8 || img->currMB.mb_type == P_8x8_ref0 || 
			 img->currMB.mb_type == B_8x8 )
    {
		
        int i;

		mb_mc_8x8( dec_params, dec_params->dec_picture, 0 );
		mb_mc_8x8( dec_params, dec_params->dec_picture, 1 );
		mb_mc_8x8( dec_params, dec_params->dec_picture, 2 );
		mb_mc_8x8( dec_params, dec_params->dec_picture, 3 );
        //for( i = 0; i < 4; i++ )
		//{
        //   mb_mc_8x8( dec_params, dec_params->dec_picture, i );
		//}
		
    }
//#endif	
    else if( img->currMB.mb_type == B_SKIP || img->currMB.mb_type == B_DIRECT )
    {
		mb_mc_direct8x8( dec_params, 0, 0 );
        mb_mc_direct8x8( dec_params, 2, 0 );
        mb_mc_direct8x8( dec_params, 0, 2 );
        mb_mc_direct8x8( dec_params, 2, 2 );
    }
    else    /* B_*x* */
    {
        int b_list0[2];
        int b_list1[2];

        int i;

        /* init ref list utilisations */
        for( i = 0; i < 2; i++ )
        {
            //b_list0[i] = x264_mb_type_list0_table[h->mb.i_type][i];
            //b_list1[i] = x264_mb_type_list1_table[h->mb.i_type][i];
        }

        if( img->currMB.partition == D_16x16 )
        {
			if( img->currMB.part_pred_dir[0] == PRED_DIR_BI ) 
				mb_mc_01xywh( dec_params, dec_params->dec_picture, 0, 0, 4, 4 );
            else if( img->currMB.part_pred_dir[0] == PRED_DIR_L0 )
				mb_mc_0xywh ( dec_params, dec_params->dec_picture, 0, 0, 4, 4 );
            else if( img->currMB.part_pred_dir[0] == PRED_DIR_L1 )
				mb_mc_1xywh ( dec_params, dec_params->dec_picture, 0, 0, 4, 4 );
        }
        else if( img->currMB.partition == D_16x8 )
        {
			if( img->currMB.part_pred_dir[0] == PRED_DIR_BI ) 
				mb_mc_01xywh( dec_params, dec_params->dec_picture, 0, 0, 4, 2 );
            else if( img->currMB.part_pred_dir[0] == PRED_DIR_L0 )
				mb_mc_0xywh ( dec_params, dec_params->dec_picture, 0, 0, 4, 2 );
            else if( img->currMB.part_pred_dir[0] == PRED_DIR_L1 )
				mb_mc_1xywh ( dec_params, dec_params->dec_picture, 0, 0, 4, 2 );

            if( img->currMB.part_pred_dir[1] == PRED_DIR_BI )
				mb_mc_01xywh( dec_params, dec_params->dec_picture, 0, 2, 4, 2 );
            else if( img->currMB.part_pred_dir[1] == PRED_DIR_L0 )
				mb_mc_0xywh ( dec_params, dec_params->dec_picture, 0, 2, 4, 2 );
            else if( img->currMB.part_pred_dir[1] == PRED_DIR_L1 )
				mb_mc_1xywh ( dec_params, dec_params->dec_picture, 0, 2, 4, 2 );
        }
        else if( img->currMB.partition == D_8x16 )
        {
			if( img->currMB.part_pred_dir[0] == PRED_DIR_BI ) 
				mb_mc_01xywh( dec_params, dec_params->dec_picture, 0, 0, 2, 4 );
            else if( img->currMB.part_pred_dir[0] == PRED_DIR_L0 )
				mb_mc_0xywh ( dec_params, dec_params->dec_picture, 0, 0, 2, 4 );
            else if( img->currMB.part_pred_dir[0] == PRED_DIR_L1 )
				mb_mc_1xywh ( dec_params, dec_params->dec_picture, 0, 0, 2, 4 );

            if( img->currMB.part_pred_dir[1] == PRED_DIR_BI )
				mb_mc_01xywh( dec_params, dec_params->dec_picture, 2, 0, 2, 4 );
            else if( img->currMB.part_pred_dir[1] == PRED_DIR_L0 )
				mb_mc_0xywh ( dec_params, dec_params->dec_picture, 2, 0, 2, 4 );
            else if( img->currMB.part_pred_dir[1] == PRED_DIR_L1 )
				mb_mc_1xywh ( dec_params, dec_params->dec_picture, 2, 0, 2, 4 );
        }

    }

}


static int decode_macroblock_P_SKIP(h264_decoder* dec_params)
{
	int i;
    ImageParameters *img = dec_params->img;
    StorablePicture* dec_picture = dec_params->dec_picture;

	imgpel* dst_imgY, *dst_imgU , *dst_imgV;
	imgpel* src_imgY, *src_imgU , *src_imgV;

	imgpel* tmp;

    int stride_luma = dec_picture->stride_luma;
	int stride_cr = dec_picture->stride_chroma;
	
    dst_imgY = dec_picture->plane[0] + (img->mb_y*16*stride_luma) + (img->mb_x*16);
	dst_imgU = dec_picture->plane[1] + (img->mb_y*8*stride_cr) + (img->mb_x*8);	
	dst_imgV = dec_picture->plane[2] + (img->mb_y*8*stride_cr) + (img->mb_x*8);

	src_imgY = dec_params->listX[LIST_0][0]->plane[0] + (img->mb_y*16*stride_luma) + (img->mb_x*16);
	src_imgU = dec_params->listX[LIST_0][0]->plane[1] + (img->mb_y*8*stride_cr) + (img->mb_x*8);
	src_imgV = dec_params->listX[LIST_0][0]->plane[2] + (img->mb_y*8*stride_cr) + (img->mb_x*8);
    

	for (i = 0; i < 16;i ++)
	{
		*(unsigned int*)(dst_imgY  )  = *(unsigned int*)(src_imgY   );
		*(unsigned int*)(dst_imgY+4)  = *(unsigned int*)(src_imgY+4 );
		*(unsigned int*)(dst_imgY+8)  = *(unsigned int*)(src_imgY+8 );
		*(unsigned int*)(dst_imgY+12) = *(unsigned int*)(src_imgY+12);
		
		dst_imgY += stride_luma;
		src_imgY += stride_luma;
	}

	for (i = 0; i < 8;i ++)
	{
			*(unsigned int*)(dst_imgU)    = *(unsigned int*)(src_imgU);
			*(unsigned int*)(dst_imgU+4)  = *(unsigned int*)(src_imgU+4);
		
			dst_imgU += stride_cr;
			src_imgU += stride_cr;
	}  

	for (i = 0; i < 8;i ++)
	{
			*(unsigned int*)(dst_imgV)    = *(unsigned int*)(src_imgV);
			*(unsigned int*)(dst_imgV+4)  = *(unsigned int*)(src_imgV+4);
		
			dst_imgV += stride_cr;
			src_imgV += stride_cr;
	}

    return 0;
}

void idct4x4dc( short d[4][4] )
{
    short tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = d[i][0] + d[i][1];
        d01 = d[i][0] - d[i][1];
        s23 = d[i][2] + d[i][3];
        d23 = d[i][2] - d[i][3];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        d[0][i] = s01 + s23;
        d[1][i] = s01 - s23;
        d[2][i] = d01 - d23;
        d[3][i] = d01 + d23;
    }
}

static __inline void idct2x2dc( short d[2][2] )
{
    int tmp[2][2];

    tmp[0][0] = d[0][0] + d[0][1];
    tmp[1][0] = d[0][0] - d[0][1];
    tmp[0][1] = d[1][0] + d[1][1];
    tmp[1][1] = d[1][0] - d[1][1];

    d[0][0] = tmp[0][0] + tmp[0][1];
    d[0][1] = tmp[1][0] + tmp[1][1];
    d[1][0] = tmp[0][0] - tmp[0][1];
    d[1][1] = tmp[1][0] - tmp[1][1];
}

void add4x4_idct( imgpel *p_dst, unsigned int p_dst_stride, short dct[4][4] )
{
    short tmp[4][4];
    int x, y;
    int i;

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  dct[0][i]     +  dct[2][i];
        const int d02 =  dct[0][i]     -  dct[2][i];
        const int s13 =  dct[1][i]     + (dct[3][i]>>1);
        const int d13 = (dct[1][i]>>1) -  dct[3][i];

        tmp[i][0] = s02 + s13;
        tmp[i][1] = d02 + d13;
        tmp[i][2] = d02 - d13;
        tmp[i][3] = s02 - s13;
    }

    for( i = 0; i < 4; i++ )
    {
        const int s02 =  tmp[0][i]     +  tmp[2][i];
        const int d02 =  tmp[0][i]     -  tmp[2][i];
        const int s13 =  tmp[1][i]     + (tmp[3][i]>>1);
        const int d13 = (tmp[1][i]>>1) -  tmp[3][i];

        dct[0][i] = s02 + s13;
        dct[1][i] = d02 + d13;
        dct[2][i] = d02 - d13;
        dct[3][i] = s02 - s13;
    }


	for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
			p_dst[x] = allowed_pixel_value[(( dct[y][x]+( (int)p_dst[x] << DQ_BITS ) + DQ_ROUND )>>DQ_BITS) +128];
        }
        p_dst += p_dst_stride;
    }
}

static __inline void add4x4_idct_2( imgpel *p_dst, short dct[4][4], int bitmap[2] )
{
	int i,y,x;
    unsigned char *rowcolmap = (unsigned char *)bitmap;
			
	if(rowcolmap[4] == 0)
		return;
	
	for(i = 0; i < 4; i++)
	{
		if(rowcolmap[i])
		{
			switch(rowcolmap[i])
			{
			case 1: // call for InvXformRow0x1
				dct[i][1] = dct[i][2] = dct[i][3] = dct[i][0];
				break;
				
			case 2: // call for InvXformRow0x2
				dct[i][0] =  dct[i][1];
				dct[i][2] = -RS1(dct[i][1]);
				dct[i][3] = -dct[i][1];
				dct[i][1] =  RS1(dct[i][1]);
				break;
				
			case 3: // call for InvXformRow0x3
				{
					const int s01 =  dct[i][0] +  dct[i][1];
					const int d01 =  dct[i][0] -  dct[i][1];
					const int s12 =  dct[i][0] + (dct[i][1]>>1);
					const int d12 =  dct[i][0] - (dct[i][1]>>1);
					
					dct[i][0] = s01;
					dct[i][1] = s12;
					dct[i][2] = d12;
					dct[i][3] = d01;
				}
				break;
			case 4: // call for InvXformRow0x4
				dct[i][0] =  dct[i][2];
				dct[i][1] = -dct[i][2];
				dct[i][3] =  dct[i][2];
				dct[i][2] = -dct[i][2];
				break;
			case 5: // call for InvXformRow0x5
				{
					const int s02 =  dct[i][0] +  dct[i][2];
					const int d02 =  dct[i][0] -  dct[i][2];
					
					dct[i][0] = s02;
					dct[i][3] = s02;
					dct[i][1] = d02;
					dct[i][2] = d02;
				}
				break;
				
			case 8: // call for InvXformRow0x8
				dct[i][0] =  RS1(dct[i][3]);
				dct[i][1] = -dct[i][3];
				dct[i][2] =  dct[i][3];
				dct[i][3] = -RS1(dct[i][3]);
				break;
			default:
				{
					const int s02 =  dct[i][0]     +  dct[i][2];
					const int d02 =  dct[i][0]     -  dct[i][2];
					const int d13 = (dct[i][1]>>1) -  dct[i][3];
					const int s13 =  dct[i][1]     + (dct[i][3]>>1);
					
					dct[i][0] = s02 + s13;
					dct[i][3] = s02 - s13;
					dct[i][1] = d02 + d13;
					dct[i][2] = d02 - d13;
				}
				break;
			}					
		}
	}
	
	for(i = 0; i < 4; i++)
	{
		switch(rowcolmap[4])
		{
		case 1: // call for InvXformCol0x1
			dct[1][i] = dct[2][i] = dct[3][i] = dct[0][i];
			break;
			
		case 2: // call for InvXformCol0x2
			dct[0][i] =  dct[1][i];
			dct[2][i] = -RS1(dct[1][i]);
			dct[3][i] = -dct[1][i];
			dct[1][i] =  RS1(dct[1][i]);
			break;
			
		case 3: // call for InvXformCol0x3
			{
				const int s01 =  dct[0][i] +  dct[1][i];
				const int d01 =  dct[0][i] -  dct[1][i];
				const int s12 =  dct[0][i] + (dct[1][i]>>1);
				const int d12 =  dct[0][i] - (dct[1][i]>>1);
				
				dct[0][i] = s01;
				dct[1][i] = s12;
				dct[2][i] = d12;
				dct[3][i] = d01;
			}
			break;
			
		case 4: // call for InvXformCol0x4
			dct[0][i] =  dct[2][i];
			dct[1][i] = -dct[2][i];
			dct[3][i] =  dct[2][i];
			dct[2][i] = -dct[2][i];
			break;
			
		case 5: // call for InvXformCol0x5
			{
				const int s02 =  dct[0][i] +  dct[2][i];
				const int d02 =  dct[0][i] -  dct[2][i];
				
				dct[0][i] = s02;
				dct[3][i] = s02;
				dct[1][i] = d02;
				dct[2][i] = d02;
			}
			break;
			
		case 8: // call for InvXformCol0x8
			dct[0][i] =  RS1(dct[3][i]);
			dct[1][i] = -dct[3][i];
			dct[2][i] =  dct[3][i];
			dct[3][i] = -RS1(dct[3][i]);
			break;
			
		default:
			{
				const int s02 =  dct[0][i]     +  dct[2][i];
				const int d02 =  dct[0][i]     -  dct[2][i];
				const int d13 = (dct[1][i]>>1) -  dct[3][i];
				const int s13 =  dct[1][i]     + (dct[3][i]>>1);
				
				dct[0][i] = s02 + s13;
				dct[3][i] = s02 - s13;
				dct[1][i] = d02 + d13;
				dct[2][i] = d02 - d13;
			}
			break;
		}
	}
	for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
			p_dst[x] = allowed_pixel_value[((dct[y][x]+((int)p_dst[x] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS) +128];
        }
        p_dst += 16;
    }
}

static __inline void dequant_2x2_dc( short dct[2][2], int dequant_mf[6][4][4], int i_qp )
{
    const int i_qbits = i_qp/6 - 5;

    if( i_qbits >= 0 )
    {
        const int i_dmf = dequant_mf[i_qp%6][0][0] << i_qbits;
		dct[0][0] = ( dct[0][0] * i_dmf ) << i_qbits;
        dct[0][1] = ( dct[0][1] * i_dmf ) << i_qbits;
        dct[1][0] = ( dct[1][0] * i_dmf ) << i_qbits;
        dct[1][1] = ( dct[1][1] * i_dmf ) << i_qbits;
    }
    else
    {
        const int i_dmf = dequant_mf[i_qp%6][0][0];
        dct[0][0] = ( dct[0][0] * i_dmf ) >> (-i_qbits);
        dct[0][1] = ( dct[0][1] * i_dmf ) >> (-i_qbits);
        dct[1][0] = ( dct[1][0] * i_dmf ) >> (-i_qbits);
        dct[1][1] = ( dct[1][1] * i_dmf ) >> (-i_qbits);
    }
}

static __inline void dequant_4x4_dc( short dct[4][4], int dequant_mf[6][4][4], int i_qp )
{
    const int i_qbits = i_qp/6 - 6;
    int y;
    if( i_qbits >= 0 )
    {
        const int i_dmf = dequant_mf[i_qp%6][0][0] << i_qbits;
        for( y = 0; y < 4; y++ )
        {
            dct[y][0] *= i_dmf;
            dct[y][1] *= i_dmf;
            dct[y][2] *= i_dmf;
            dct[y][3] *= i_dmf;
        }
    }    
	else
    {
        const int i_dmf = dequant_mf[i_qp%6][0][0];
        const int f = 1 << (-i_qbits-1);
        for( y = 0; y < 4; y++ )
        {
            dct[y][0] = ( dct[y][0] * i_dmf + f ) >> (-i_qbits);
            dct[y][1] = ( dct[y][1] * i_dmf + f ) >> (-i_qbits);
            dct[y][2] = ( dct[y][2] * i_dmf + f ) >> (-i_qbits);
            dct[y][3] = ( dct[y][3] * i_dmf + f ) >> (-i_qbits);
        }
    }
}

static void decode_macroblock_i16x16( h264_decoder* dec_params, int qp )
{
	int i;
    ImageParameters* img = dec_params->img;
    const int stride_luma = dec_params->dec_picture->stride_luma;
	imgpel* dst = dec_params->dec_picture->plane[0] + (img->mb_y*16*stride_luma) + (img->mb_x*16);

	img->p_idct4x4dc( img->dc_cof_luma );
	dequant_4x4_dc( img->dc_cof_luma, dequant_coef, qp );
	
    for( i = 0; i < 16; i++ )
    {
		// copy dc coeff 
		img->cof_s[i][0][0] = img->dc_cof_luma[i>>2][i%4];

		if( img->run_idct[i] )
			img->p_add4x4_idct( dst + (i>>2)*stride_luma*4 + (i&3)*4, stride_luma, img->cof_s[i] );
	}
}

static void decode_macroblock_8x8_chroma( h264_decoder* dec_params, int qp )
{
    int i, ch;
    ImageParameters *img = dec_params->img;
    const  int stride_chroma = dec_params->dec_picture->stride_chroma;
	
    //imgpel *dst[2] = { dec_params->dec_picture->plane[1] + (img->mb_y*8*stride_chroma) + (img->mb_x*8),
    //    dec_params->dec_picture->plane[2] + (img->mb_y*8*stride_chroma) + (img->mb_x*8)
    //};

	imgpel *dst[2];
	dst[0] = dec_params->dec_picture->plane[1] + (img->mb_y*8*stride_chroma) + (img->mb_x*8); 
	dst[1] = dec_params->dec_picture->plane[2] + (img->mb_y*8*stride_chroma) + (img->mb_x*8); 


    for( ch = 0; ch < 2; ch++ )
    {
		// Modified to remove compiler error <Tahir>	
		short temp[2][2];

		temp[0][0] = img->dc_cof_chroma[ch][0];
		temp[0][1] = img->dc_cof_chroma[ch][1];
		temp[1][0] = img->dc_cof_chroma[ch][2];
		temp[1][1] = img->dc_cof_chroma[ch][3];

        //idct2x2dc(  (img->dc_cof_chroma[ch]) );	// forward and inverse hadamard transform are same
		idct2x2dc( temp );	// forward and inverse hadamard transform are same
        //dequant_2x2_dc(  (img->dc_cof_chroma[ch]), dequant_coef, qp );
        dequant_2x2_dc( temp, dequant_coef, qp );

		img->dc_cof_chroma[ch][0] = temp[0][0] ;
		img->dc_cof_chroma[ch][1] = temp[0][1] ;
		img->dc_cof_chroma[ch][2] = temp[1][0] ;
  	    img->dc_cof_chroma[ch][3] = temp[1][1] ;

        for( i = 0; i < 4; i++ )
		{
			// copy the dc coefficient
            //img->cof_s[ch*4 + i + 16][0][0] = img->dc_cof_chroma[ch][i];
			img->cof_s[ch*4 + i + 16][0][0] = img->dc_cof_chroma[ch][i];

			if(img->run_idct[16+ch*4 + i])
			{
				img->p_add4x4_idct( dst[ch] + (i&2)*stride_chroma*2 + (i&1)*4, stride_chroma, img->cof_s[ch*4 + i + 16] );
			}
		}
    }
}
void decode_macroblock_pskip( h264_decoder* dec_params, StorablePicture* dec_picture )
{
	ImageParameters* img = dec_params->img;
    const int i_ref = *(dec_picture->ref_idx_l0 + (img->current_mb_nr*16));
	const short mvx   = *(dec_picture->mvL0 + (img->current_mb_nr<<5) );
    const short mvy   = *(dec_picture->mvL0 + (img->current_mb_nr<<5) + 1);
	imgpel* planeY	= ( dec_params->listX[LIST_0][0]->plane[0] + 
					    ((img->mb_y*16)*dec_params->listX[LIST_0][0]->stride_luma) +
					    (img->mb_x*16) ) ;
	imgpel* planeU	= ( dec_params->listX[LIST_0][0]->plane[1] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_0][0]->stride_chroma) +
					    (img->mb_x*8) ) ;
	imgpel* planeV	= ( dec_params->listX[LIST_0][0]->plane[2] + 
					    ((img->mb_y*8)*dec_params->listX[LIST_0][0]->stride_chroma) +
					    (img->mb_x*8) ) ;

	imgpel* planeY1	= ( dec_picture->plane[0] + 
					    ((img->mb_y*16)*dec_picture->stride_luma) +
					    (img->mb_x*16) ) ;
	imgpel* planeU1	= ( dec_picture->plane[1] + 
					    ((img->mb_y*8)*dec_picture->stride_chroma) +
					    (img->mb_x*8) ) ;
	imgpel* planeV1	= ( dec_picture->plane[2] + 
					    ((img->mb_y*8)*dec_picture->stride_chroma) +
					    (img->mb_x*8) ) ;


    mc_luma( planeY, dec_params->listX[LIST_0][0]->stride_luma,
             planeY1 , dec_picture->stride_luma,
             mvx , mvy , 16, 16);

    mc_chroma( planeU, dec_params->listX[LIST_0][0]->stride_chroma,
               planeU1 , dec_picture->stride_chroma,
               mvx, mvy, 8, 8);

	mc_chroma( planeV, dec_params->listX[LIST_0][0]->stride_chroma,
               planeV1 , dec_picture->stride_chroma,
               mvx, mvy, 8, 8 );
}


int decode_one_macroblock( h264_decoder* dec_params )
{
	ImageParameters* img = dec_params->img;
	int i,j;
	int stride_luma = dec_params->dec_picture->stride_luma;
	int stride_cr = dec_params->dec_picture->stride_chroma;
	int qp = img->currMB.qp;
	imgpel* imgYUV[3];
    imgpel *dst=NULL;

	
	imgYUV[0] = dec_params->dec_picture->plane[0] + (img->mb_y*16*stride_luma) + (img->mb_x*16);
	imgYUV[1] = dec_params->dec_picture->plane[1] + (img->mb_y*8*stride_cr) + (img->mb_x*8);
	imgYUV[2] = dec_params->dec_picture->plane[2] + (img->mb_y*8*stride_cr) + (img->mb_x*8);

	if( img->currMB.mb_type == P_SKIP_MV_0 )
	{
		// to decode the P Skip mb when the mv is 0
		decode_macroblock_P_SKIP(dec_params);
		return;
	}

	if( img->currMB.mb_type == P_SKIP )
    {
        // to decode general P Skip mb 
        decode_macroblock_pskip( dec_params, dec_params->dec_picture );
        return;
    }
    if( img->currMB.mb_type == B_SKIP )
    {
        // XXX motion compensation is probably unneeded 
        mb_mc( dec_params );
        //x264_macroblock_encode_skip( h );
        return;
    }
    if( img->currMB.mb_type == I_16x16 )
    {
		int neighbour = 0;

        if (!dec_params->active_pps->constrained_intra_pred_flag)
        {
            neighbour = (img->currMB.mbAvailA << 1) + img->currMB.mbAvailB;
        }
        else
        {
            neighbour = ( img->currMB.mbAvailB ? img->intra_block[img->currMB.mbAddrB] : 0 )
                + ( ( img->currMB.mbAvailA ? img->intra_block[img->currMB.mbAddrA] : 0 ) << 1 );
        }
        predict_16x16[img->currMB.i16mode]( imgYUV[0], stride_luma, imgYUV[0], stride_luma, neighbour);
        // decode the 16x16 macroblock 
        if(img->currMB.cbp%16){
            decode_macroblock_i16x16( dec_params, img->currMB.qp);
        }
    }
    else if( img->currMB.mb_type == I_4x4 )
    {
		int k;
		
		int neighbour = 0;
		char* i_predmode = &(img->ipredmode[0][0]) + (img->current_mb_nr<<4);

		if (!dec_params->active_pps->constrained_intra_pred_flag)
		{
			neighbour = (img->currMB.mbAvailA << 1) + img->currMB.mbAvailB
				+ ( img->currMB.mbAvailC << 2 );
		}
		else
		{
			neighbour = ( img->currMB.mbAvailB ? img->intra_block[img->currMB.mbAddrB] : 0 )
				+ ( ( img->currMB.mbAvailA ? img->intra_block[img->currMB.mbAddrA] : 0 ) << 1 )
				+ ( ( img->currMB.mbAvailC ? img->intra_block[img->currMB.mbAddrC] : 0 ) << 2 );
		}
        for( k = 0; k < 16; k++ )
        {
			imgpel* src = imgYUV[0];

			
			//imgpel* dst = img->mpr1[0];			
			imgpel* dst = (imgpel *)img->mpr1[0][0][0];
			
			int check_availability = neighbour;
			int   mode;
			i = k & 3;
			j = ((k >> 2) );
			src += ((j<<2) )*stride_luma + (i<<2) ;
			dst += (j<<2)*16 + (i<<2);
			
			mode = i_predmode[k]  ;
			
			check_availability |= ( i << 4 ) | ( j << 8 );
			predict_4x4[mode]( src, stride_luma, src, stride_luma, check_availability );

            if(img->currMB.cbp%16 && img->run_idct[k])
			{
				//img->p_add4x4_idct( src, stride_luma, &img->cof_s[k][0][0] );
				img->p_add4x4_idct( src, stride_luma, img->cof_s[k] );
			}
        }
    }
    else    // Inter MB
    {
		// Motion compensation 
        mb_mc( dec_params );

		if(img->currMB.cbp%16)
		{
			//imgpel* dst = img->mpr1[0];
			imgpel* dst = (imgpel *)img->mpr1[0][0][0];

			int bitmap[2];
			
			for( i = 0; i < 16; i++ )
			{
				if(img->run_idct[i])
                {
                    //img->p_add4x4_idct( imgYUV[0] + (i>>2)*stride_luma*4 + (i&3)*4, stride_luma, &img->cof_s[i][0][0] );
					img->p_add4x4_idct( imgYUV[0] + (i>>2)*stride_luma*4 + (i&3)*4, stride_luma, img->cof_s[i] );
                }
			}
		}
    }

    // decode chroma 
    if( img->currMB.mb_type == I_4x4 ||  img->currMB.mb_type == I_16x16 )
    {
        int neighbour = 0;

		if (!dec_params->active_pps->constrained_intra_pred_flag)
        {
            neighbour = (img->currMB.mbAvailA << 1) + img->currMB.mbAvailB;
        }
        else
        {
            neighbour = ( img->currMB.mbAvailB ? img->intra_block[img->currMB.mbAddrB] : 0 )
                + ( ( img->currMB.mbAvailA ? img->intra_block[img->currMB.mbAddrA] : 0 ) << 1 );
        }
        if(img->current_mb_nr == 0)
        {
            img->current_mb_nr = img->current_mb_nr ;
        }
        predict_8x8[img->currMB.c_ipred_mode](imgYUV[1], stride_cr, imgYUV[1], stride_cr, neighbour);
		predict_8x8[img->currMB.c_ipred_mode](imgYUV[2], stride_cr, imgYUV[2], stride_cr, neighbour);
	}
  


	if(img->currMB.cbp/16)
	{
		qp = chroma_qp_table[Clip3( 0, 51, qp + dec_params->active_pps->chroma_qp_index_offset )];
		decode_macroblock_8x8_chroma(dec_params, qp);
	}

	//save_current_mb(img);
	

    return 0;
}
