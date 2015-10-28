
/*!
 *************************************************************************************
 * \file header.c
 *
 * \brief
 *    H.264 Slice headers
 *
 *************************************************************************************
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "global.h"
#include "elements.h"
#include "defines.h"
#include "fmo.h"
#include "vlc.h"
#include "mbuffer.h"
#include "header.h"

//#include "ctx_tables.h"

//extern StorablePicture *dec_picture;	/*Changed by Saad Bin Shams [Removing Global Variables]*/

#if TRACE
#define SYMTRACESTRING(s) strncpy(sym.tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // to nothing
#endif

//extern int UsedBits;					/*Changed by Saad Bin Shams [Removing Global Variables]*/

/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
static void ref_pic_list_reordering( h264_decoder* dec_params );

/*Function Argument List Changed [Removing Global Variables] <saad.shams@inforient.com>*/
static void pred_weight_table( h264_decoder* dec_params );


/*!
 ************************************************************************
 * \brief
 *    calculate Ceil(Log2(uiVal))
 ************************************************************************
 */
unsigned CeilLog2( unsigned uiVal)
{
  unsigned uiTmp = uiVal-1;
  unsigned uiRet = 0;

  while( uiTmp != 0 )
  {
    uiTmp >>= 1;
    uiRet++;
  }
  return uiRet;
}


/*!
 ************************************************************************
 * \brief
 *    read the first part of the header (only the pic_parameter_set_id)
 * \return
 *    Length of the first part of the slice header (in bits)
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
int FirstPartOfSliceHeader(   h264_decoder* dec_params)
{
  Slice *currSlice = dec_params->img->currentSlice;
  //int dP_nr = assignSE2partition[PAR_DP_1][SE_HEADER];
  //int dP_nr = 0;
  DataPartition *partition = &(currSlice->partArr[0]);
  Bitstream *currStream = partition->bitstream;
  int tmp;

  dec_params->UsedBits= partition->bitstream->frame_bitoffset; // was hardcoded to 31 for previous start-code. This is better.

  // Get first_mb_in_slice
  currSlice->start_mb_nr = ue_v ("SH: first_mb_in_slice", currStream,dec_params);

  tmp = ue_v ("SH: slice_type", currStream,dec_params);
  
  if (tmp>4) tmp -=5;

  dec_params->img->type = currSlice->picture_type = (SliceType) tmp;

  currSlice->pic_parameter_set_id = ue_v ("SH: pic_parameter_set_id", currStream,dec_params);
  
  return dec_params->UsedBits;
}

/***********************************************************************
*\brief Header_parsing
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/
int RestOfSliceHeader_baseline( h264_decoder* dec_params )
{
	Slice *currSlice = dec_params->img->currentSlice;
	//int dP_nr = assignSE2partition[PAR_DP_1][SE_HEADER];
	//int dP_nr = 0;
	DataPartition *partition = &(currSlice->partArr[0]);
	Bitstream *currStream = partition->bitstream;
	
	int val, len;
	// reading frame number (7.3.3)
	dec_params->img->frame_num = u_v (dec_params->active_sps->log2_max_frame_num_minus4 + 4, "SH: frame_num", currStream,dec_params);
	
	/* Tian Dong: frame_num gap processing, if found */
	if (dec_params->img->idr_flag)
	{
		dec_params->img->pre_frame_num = dec_params->img->frame_num;
		assert(dec_params->img->frame_num == 0);
	}
	
	//{
	//  dec_params->img->structure = FRAME;
	//}
	
	//currSlice->structure = FRAME;
	
	//assert (dec_params->img->field_pic_flag == 0);
	
	if (dec_params->img->idr_flag)
	{
		dec_params->img->idr_pic_id = ue_v("SH: idr_pic_id", currStream,dec_params);
	}
	
	if (dec_params->active_sps->pic_order_cnt_type == 0)
	{
		dec_params->img->pic_order_cnt_lsb = u_v(dec_params->active_sps->log2_max_pic_order_cnt_lsb_minus4 + 4, "SH: pic_order_cnt_lsb", currStream,dec_params);
		if( dec_params->active_pps->pic_order_present_flag  ==  1)
			dec_params->img->delta_pic_order_cnt_bottom = se_v("SH: delta_pic_order_cnt_bottom", currStream,dec_params);
		else
			dec_params->img->delta_pic_order_cnt_bottom = 0;  
	}
	if( dec_params->active_sps->pic_order_cnt_type == 1 && !dec_params->active_sps->delta_pic_order_always_zero_flag ) 
	{
		dec_params->img->delta_pic_order_cnt[ 0 ] = se_v("SH: delta_pic_order_cnt[0]", currStream,dec_params);
		if( dec_params->active_pps->pic_order_present_flag  ==  1 )
			dec_params->img->delta_pic_order_cnt[ 1 ] = se_v("SH: delta_pic_order_cnt[1]", currStream,dec_params);
	}else
	{
		if (dec_params->active_sps->pic_order_cnt_type == 1)
		{
			dec_params->img->delta_pic_order_cnt[ 0 ] = 0;
			dec_params->img->delta_pic_order_cnt[ 1 ] = 0;
		}
	}
	
	//! redundant_pic_cnt is missing here
	if (dec_params->active_pps->redundant_pic_cnt_present_flag)
	{
		//dec_params->img->redundant_pic_cnt = ue_v ("SH: redundant_pic_cnt", currStream,dec_params);
	}
	
	
	if(dec_params->img->type==B_SLICE)
	{
		dec_params->img->direct_spatial_mv_pred_flag = u_1 ("SH: direct_spatial_mv_pred_flag", currStream,dec_params);
	}
	
	dec_params->img->num_ref_idx_l0_active = dec_params->active_pps->num_ref_idx_l0_active_minus1 + 1;
	dec_params->img->num_ref_idx_l1_active = dec_params->active_pps->num_ref_idx_l1_active_minus1 + 1;
	
	if(dec_params->img->type==P_SLICE || dec_params->img->type==B_SLICE)// added by Faisal Abdullah for B frames
	{
		val = u_1 ("SH: num_ref_idx_override_flag", currStream,dec_params);
		if (val)
		{
			dec_params->img->num_ref_idx_l0_active = 1 + ue_v ("SH: num_ref_idx_l0_active_minus1", currStream,dec_params);
			
			if(dec_params->img->type==B_SLICE)
			{
				dec_params->img->num_ref_idx_l1_active = 1 + ue_v ("SH: num_ref_idx_l1_active_minus1", currStream,dec_params);
			}
			
		}
	}
	
	
	if (dec_params->img->type!=B_SLICE)
	{
		dec_params->img->num_ref_idx_l1_active = 0;
	}
	
	ref_pic_list_reordering( dec_params );
	
	
	// for weighted prediction in B and P frames
	dec_params->img->apply_weights = ((dec_params->active_pps->weighted_pred_flag && (currSlice->picture_type == P_SLICE ) )
	|| ((dec_params->active_pps->weighted_bipred_idc > 0 ) && (currSlice->picture_type == B_SLICE)));
	/*dec_params->img->apply_weights = ((dec_params->active_pps->weighted_pred_flag && (currSlice->picture_type == P_SLICE ) )
		|| ((dec_params->active_pps->weighted_bipred_idc == 1 ) && (currSlice->picture_type == B_SLICE)));*/
	dec_params->img->apply_weights_bi = 0;
    dec_params->img->apply_weights_luma = 0;
	dec_params->img->apply_weights_chr = 0;
	
	if ((dec_params->active_pps->weighted_pred_flag&&(dec_params->img->type==P_SLICE))||
		(dec_params->active_pps->weighted_bipred_idc==1 && (dec_params->img->type==B_SLICE)))
	{
		pred_weight_table(dec_params);
	}
	dec_params->img->apply_weights_bi = (dec_params->active_pps->weighted_bipred_idc==2 && (dec_params->img->type==B_SLICE));
	
	
	if (dec_params->img->nal_reference_idc)
		dec_ref_pic_marking(currStream,dec_params);

    //__CABAC__
    if (dec_params->active_pps->entropy_coding_mode_flag && dec_params->img->type!=I_SLICE && dec_params->img->type!=SI_SLICE)
    {
        dec_params->img->model_number = ue_v("SH: cabac_init_idc", currStream, dec_params);
    }
    else 
    {
        dec_params->img->model_number = 0;
    }
    //__CABAC__
	
	val = se_v("SH: slice_qp_delta", currStream,dec_params);
	//currSlice->qp = dec_params->img->qp = 26 + dec_params->active_pps->pic_init_qp_minus26 + val;
	dec_params->img->qp = 26 + dec_params->active_pps->pic_init_qp_minus26 + val;
	
	
	//currSlice->slice_qp_delta = val;  
	
	
	if (dec_params->active_pps->deblocking_filter_control_present_flag)
	{
		currSlice->LFDisableIdc = ue_v ("SH: disable_deblocking_filter_idc", currStream,dec_params);
		
		if (currSlice->LFDisableIdc!=1)
		{
			currSlice->LFAlphaC0Offset = 2 * se_v("SH: slice_alpha_c0_offset_div2", currStream,dec_params);
			currSlice->LFBetaOffset = 2 * se_v("SH: slice_beta_offset_div2", currStream,dec_params);
		}
		else
		{
			currSlice->LFAlphaC0Offset = currSlice->LFBetaOffset = 0;
		}
	}
	else 
	{
		currSlice->LFDisableIdc = currSlice->LFAlphaC0Offset = currSlice->LFBetaOffset = 0;
	}
	
	if (dec_params->active_pps->num_slice_groups_minus1>0 && dec_params->active_pps->slice_group_map_type>=3 &&
		dec_params->active_pps->slice_group_map_type<=5)
	{
		len = (dec_params->active_sps->pic_height_in_map_units_minus1+1)*(dec_params->active_sps->pic_width_in_mbs_minus1+1)/ 
			(dec_params->active_pps->slice_group_change_rate_minus1+1);
		if (((dec_params->active_sps->pic_height_in_map_units_minus1+1)*(dec_params->active_sps->pic_width_in_mbs_minus1+1))% 
			(dec_params->active_pps->slice_group_change_rate_minus1+1))
			len +=1;
		
		len = CeilLog2(len+1);
		
		dec_params->img->slice_group_change_cycle = u_v (len, "SH: slice_group_change_cycle", currStream,dec_params);
	}
	dec_params->img->FrameSizeInMbs = dec_params->img->FrameWidthInMbs * dec_params->img->FrameHeightInMbs;
	
	return dec_params->UsedBits;
}



/*!
 ************************************************************************
 * \brief
 *    read the reference picture reordering information
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
  *
 *		<saad.shams@inforient.com>
 *-----------------------------------------------------------------------
 *	Added compatablility for B_Slice 
 
 ************************************************************************
 */

static void ref_pic_list_reordering( h264_decoder* dec_params )
{
  Slice *currSlice = dec_params->img->currentSlice;
  //int dP_nr = assignSE2partition[PAR_DP_1][SE_HEADER];
  //int dP_nr = 0;
  DataPartition *partition = &(currSlice->partArr[0]);
  Bitstream *currStream = partition->bitstream;
  int i, val;

  alloc_ref_pic_list_reordering_buffer( currSlice, dec_params );
  
  if (dec_params->img->type!=I_SLICE) {
    val = currSlice->ref_pic_list_reordering_flag_l0 = u_1 ("SH: ref_pic_list_reordering_flag_l0", currStream,dec_params);    
    if (val) {
      i=0;
      do {
        val = currSlice->remapping_of_pic_nums_idc_l0[i] = ue_v("SH: remapping_of_pic_nums_idc_l0", currStream,dec_params);
        if (val==0 || val==1) {
          currSlice->abs_diff_pic_num_minus1_l0[i] = ue_v("SH: abs_diff_pic_num_minus1_l0", currStream,dec_params);
        } else {
          if (val==2) {
            currSlice->long_term_pic_idx_l0[i] = ue_v("SH: long_term_pic_idx_l0", currStream,dec_params);
          }
        }
        i++;
        // assert (i>dec_params->img->num_ref_idx_l0_active);
      } while (val != 3);
    }
  } 

  
  if (dec_params->img->type==B_SLICE) {
    val = currSlice->ref_pic_list_reordering_flag_l1 = u_1 ("SH: ref_pic_list_reordering_flag_l1", currStream,dec_params);    
    if (val) {
      i=0;
      do {
        val = currSlice->remapping_of_pic_nums_idc_l1[i] = ue_v("SH: remapping_of_pic_nums_idc_l1", currStream,dec_params);
        if (val==0 || val==1) {
          currSlice->abs_diff_pic_num_minus1_l1[i] = ue_v("SH: abs_diff_pic_num_minus1_l1", currStream,dec_params);
        } else {
          if (val==2) {
            currSlice->long_term_pic_idx_l1[i] = ue_v("SH: long_term_pic_idx_l1", currStream,dec_params);
          }
        }
        i++;
        // assert (i>dec_params->img->num_ref_idx_l1_active);
      } while (val != 3);
    }
  }   
  
}

/*!
 ************************************************************************
 * \brief
 *    read the memory control operations
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

void dec_ref_pic_marking(Bitstream *currStream,   h264_decoder* dec_params)
{
  int val;

  DecRefPicMarking_t *tmp_drpm,*tmp_drpm2;

  // free old buffer content
  while (dec_params->img->dec_ref_pic_marking_buffer)
  { 
    tmp_drpm=dec_params->img->dec_ref_pic_marking_buffer;

    dec_params->img->dec_ref_pic_marking_buffer=tmp_drpm->Next;
    h264_free (tmp_drpm);
  } 

  if (dec_params->img->idr_flag)
  {
    dec_params->img->no_output_of_prior_pics_flag = u_1("SH: no_output_of_prior_pics_flag", currStream,dec_params);
    dec_params->img->long_term_reference_flag = u_1("SH: long_term_reference_flag", currStream,dec_params);
  }
  else
  {
    dec_params->img->adaptive_ref_pic_buffering_flag = u_1("SH: adaptive_ref_pic_buffering_flag", currStream,dec_params);
    if (dec_params->img->adaptive_ref_pic_buffering_flag)
    {
      // read Memory Management Control Operation 
      do
      {
		tmp_drpm=(DecRefPicMarking_t*) h264_malloc (1*sizeof (DecRefPicMarking_t));
        tmp_drpm->Next=NULL;
        
        val = tmp_drpm->memory_management_control_operation = ue_v("SH: memory_management_control_operation", currStream,dec_params);

        if ((val==1)||(val==3)) 
        {
          tmp_drpm->difference_of_pic_nums_minus1 = ue_v("SH: difference_of_pic_nums_minus1", currStream,dec_params);
        }
        if (val==2)
        {
          tmp_drpm->long_term_pic_num = ue_v("SH: long_term_pic_num", currStream,dec_params);
        }
          
        if ((val==3)||(val==6))
        {
          tmp_drpm->long_term_frame_idx = ue_v("SH: long_term_frame_idx", currStream,dec_params);
        }
        if (val==4)
        {
          tmp_drpm->max_long_term_frame_idx_plus1 = ue_v("SH: max_long_term_pic_idx_plus1", currStream,dec_params);
        }
        
        // add command
        if (dec_params->img->dec_ref_pic_marking_buffer==NULL) 
        {
          dec_params->img->dec_ref_pic_marking_buffer=tmp_drpm;
        }
        else
        {
          tmp_drpm2=dec_params->img->dec_ref_pic_marking_buffer;
          while (tmp_drpm2->Next!=NULL) tmp_drpm2=tmp_drpm2->Next;
          tmp_drpm2->Next=tmp_drpm;
        }
        
      }while (val != 0);
      
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    To calculate the poc values
 *        based upon JVT-F100d2
 *  POC200301: Until Jan 2003, this function will calculate the correct POC
 *    values, but the management of POCs in buffered pictures may need more work.
 * \return
 *    none
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
  *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

void decode_poc( h264_decoder* dec_params )
{
  int i;
  // for POC mode 0:
  unsigned int        MaxPicOrderCntLsb = (1<<(dec_params->active_sps->log2_max_pic_order_cnt_lsb_minus4+4));

  switch ( dec_params->active_sps->pic_order_cnt_type )
  {
  case 0: // POC MODE 0
    // 1st
    if(dec_params->img->idr_flag)
    {
      dec_params->img->PrevPicOrderCntMsb = 0;
      dec_params->img->PrevPicOrderCntLsb = 0;
    }
    else
    {
      if (dec_params->img->last_has_mmco_5) 
      {
          dec_params->img->PrevPicOrderCntMsb = 0;
          dec_params->img->PrevPicOrderCntLsb = dec_params->img->toppoc;
      }
    }
    // Calculate the MSBs of current picture
    if( dec_params->img->pic_order_cnt_lsb  <  dec_params->img->PrevPicOrderCntLsb  &&  
      ( dec_params->img->PrevPicOrderCntLsb - dec_params->img->pic_order_cnt_lsb )  >=  ( MaxPicOrderCntLsb / 2 ) )
      dec_params->img->PicOrderCntMsb = dec_params->img->PrevPicOrderCntMsb + MaxPicOrderCntLsb;
    else if ( dec_params->img->pic_order_cnt_lsb  >  dec_params->img->PrevPicOrderCntLsb  &&
      ( dec_params->img->pic_order_cnt_lsb - dec_params->img->PrevPicOrderCntLsb )  >  ( MaxPicOrderCntLsb / 2 ) )
      dec_params->img->PicOrderCntMsb = dec_params->img->PrevPicOrderCntMsb - MaxPicOrderCntLsb;
    else
      dec_params->img->PicOrderCntMsb = dec_params->img->PrevPicOrderCntMsb;
    
    // 2nd

//    if(dec_params->img->field_pic_flag==0)
    {           //frame pix
      dec_params->img->toppoc = dec_params->img->PicOrderCntMsb + dec_params->img->pic_order_cnt_lsb;
      dec_params->img->bottompoc = dec_params->img->toppoc + dec_params->img->delta_pic_order_cnt_bottom;
      dec_params->img->ThisPOC = dec_params->img->framepoc = (dec_params->img->toppoc < dec_params->img->bottompoc)? dec_params->img->toppoc : dec_params->img->bottompoc; // POC200301
    }
    dec_params->img->framepoc=dec_params->img->ThisPOC;

    if ( dec_params->img->frame_num!=dec_params->img->PreviousFrameNum)
      dec_params->img->PreviousFrameNum=dec_params->img->frame_num;

    if(!dec_params->img->disposable_flag)
    {
      dec_params->img->PrevPicOrderCntLsb = dec_params->img->pic_order_cnt_lsb;
      dec_params->img->PrevPicOrderCntMsb = dec_params->img->PicOrderCntMsb;
    }

    break;

  case 1: // POC MODE 1
    // 1st
    if(dec_params->img->idr_flag)
    {
      dec_params->img->FrameNumOffset=0;     //  first pix of IDRGOP, 
      dec_params->img->delta_pic_order_cnt[0]=0;                        //ignore first delta
      if(dec_params->img->frame_num)  
	  {
		  printf("frame_num != 0 in idr pix", -1020);
		  exit(0);
	  }
    }
    else 
    {
      if (dec_params->img->last_has_mmco_5)
      {
        dec_params->img->PreviousFrameNumOffset = 0;
        dec_params->img->PreviousFrameNum = 0;
      }
      if (dec_params->img->frame_num<dec_params->img->PreviousFrameNum)
      {             //not first pix of IDRGOP
        dec_params->img->FrameNumOffset = dec_params->img->PreviousFrameNumOffset + dec_params->img->MaxFrameNum;
      }
      else 
      {
        dec_params->img->FrameNumOffset = dec_params->img->PreviousFrameNumOffset;
      }
    }

    // 2nd
    if(dec_params->active_sps->num_ref_frames_in_pic_order_cnt_cycle) 
      dec_params->img->AbsFrameNum = dec_params->img->FrameNumOffset+dec_params->img->frame_num;
    else 
      dec_params->img->AbsFrameNum=0;
    if(dec_params->img->disposable_flag && dec_params->img->AbsFrameNum>0)
      dec_params->img->AbsFrameNum--;

    // 3rd
    dec_params->img->ExpectedDeltaPerPicOrderCntCycle=0;

    if(dec_params->active_sps->num_ref_frames_in_pic_order_cnt_cycle)
    for(i=0;i<(int) dec_params->active_sps->num_ref_frames_in_pic_order_cnt_cycle;i++)
      dec_params->img->ExpectedDeltaPerPicOrderCntCycle += dec_params->active_sps->offset_for_ref_frame[i];

    if(dec_params->img->AbsFrameNum)
    {
      dec_params->img->PicOrderCntCycleCnt = (dec_params->img->AbsFrameNum-1)/dec_params->active_sps->num_ref_frames_in_pic_order_cnt_cycle;
      dec_params->img->FrameNumInPicOrderCntCycle = (dec_params->img->AbsFrameNum-1)%dec_params->active_sps->num_ref_frames_in_pic_order_cnt_cycle;
      dec_params->img->ExpectedPicOrderCnt = dec_params->img->PicOrderCntCycleCnt*dec_params->img->ExpectedDeltaPerPicOrderCntCycle;
      for(i=0;i<=(int)dec_params->img->FrameNumInPicOrderCntCycle;i++)
        dec_params->img->ExpectedPicOrderCnt += dec_params->active_sps->offset_for_ref_frame[i];
    }
    else 
      dec_params->img->ExpectedPicOrderCnt=0;

    if(dec_params->img->disposable_flag)
      dec_params->img->ExpectedPicOrderCnt += dec_params->active_sps->offset_for_non_ref_pic;

      //frame pix
      dec_params->img->toppoc = dec_params->img->ExpectedPicOrderCnt + dec_params->img->delta_pic_order_cnt[0];
      dec_params->img->bottompoc = dec_params->img->toppoc + dec_params->active_sps->offset_for_top_to_bottom_field + dec_params->img->delta_pic_order_cnt[1];
      dec_params->img->ThisPOC = dec_params->img->framepoc = (dec_params->img->toppoc < dec_params->img->bottompoc)? dec_params->img->toppoc : dec_params->img->bottompoc; // POC200301
    
    dec_params->img->framepoc=dec_params->img->ThisPOC;

    dec_params->img->PreviousFrameNum=dec_params->img->frame_num;
    dec_params->img->PreviousFrameNumOffset=dec_params->img->FrameNumOffset;
  
    break;


  case 2: // POC MODE 2
    if(dec_params->img->idr_flag) // IDR picture
    {
      dec_params->img->FrameNumOffset=0;     //  first pix of IDRGOP, 
      dec_params->img->ThisPOC = dec_params->img->framepoc = dec_params->img->toppoc = dec_params->img->bottompoc = 0;
      if(dec_params->img->frame_num)  
	  {
		  printf("frame_num != 0 in idr pix", -1020);
		  exit(0);
	  }
    }
    else
    {
      if (dec_params->img->last_has_mmco_5)
      {
        dec_params->img->PreviousFrameNum = 0;
        dec_params->img->PreviousFrameNumOffset = 0;
      }
      if (dec_params->img->frame_num<dec_params->img->PreviousFrameNum)
        dec_params->img->FrameNumOffset = dec_params->img->PreviousFrameNumOffset + dec_params->img->MaxFrameNum;
      else 
        dec_params->img->FrameNumOffset = dec_params->img->PreviousFrameNumOffset;


      dec_params->img->AbsFrameNum = dec_params->img->FrameNumOffset+dec_params->img->frame_num;
      if(dec_params->img->disposable_flag)
        dec_params->img->ThisPOC = (2*dec_params->img->AbsFrameNum - 1);
      else
        dec_params->img->ThisPOC = (2*dec_params->img->AbsFrameNum);

       dec_params->img->toppoc = dec_params->img->bottompoc = dec_params->img->framepoc = dec_params->img->ThisPOC;
    }

    if (!dec_params->img->disposable_flag)
      dec_params->img->PreviousFrameNum=dec_params->img->frame_num;
    dec_params->img->PreviousFrameNumOffset=dec_params->img->FrameNumOffset;
    break;


  default:
    //error must occurs
    assert( 1==0 );
    break;
  }
}

/*!
 ************************************************************************
 * \brief
 *    read the weighted prediction tables
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */

static void pred_weight_table(  h264_decoder* dec_params)
{
	
  Slice *currSlice = dec_params->img->currentSlice;
  DataPartition *partition = &(currSlice->partArr[0]);
  Bitstream *currStream = partition->bitstream;
  int luma_weight_flag_l0, luma_weight_flag_l1, chroma_weight_flag_l0, chroma_weight_flag_l1;
  int i,j;

  dec_params->img->luma_log2_weight_denom = ue_v ("SH: luma_log2_weight_denom", currStream,dec_params);
  dec_params->img->wp_round_luma = dec_params->img->luma_log2_weight_denom ? 1<<(dec_params->img->luma_log2_weight_denom - 1): 0;
  

  
  dec_params->img->chroma_log2_weight_denom = ue_v ("SH: chroma_log2_weight_denom", currStream,dec_params);
  dec_params->img->wp_round_chroma = dec_params->img->chroma_log2_weight_denom ? 1<<(dec_params->img->chroma_log2_weight_denom - 1): 0;

  reset_wp_params(dec_params->img);

  for (i=0; i<dec_params->img->num_ref_idx_l0_active; i++)
  {
    luma_weight_flag_l0 = u_1("SH: luma_weight_flag_l0", currStream,dec_params);
    
    if (luma_weight_flag_l0)
    {
		dec_params->img->apply_weights_luma = 1;
      dec_params->img->wp_weight[0][i][0] = se_v ("SH: luma_weight_l0", currStream,dec_params);
      dec_params->img->wp_offset[0][i][0] = se_v ("SH: luma_offset_l0", currStream,dec_params);
    }
    else
    {
      dec_params->img->wp_weight[0][i][0] = 1<<dec_params->img->luma_log2_weight_denom;
      dec_params->img->wp_offset[0][i][0] = 0;
    }
    
    //if (dec_params->active_sps->chroma_format_idc != 0)
    {
      chroma_weight_flag_l0 = u_1 ("SH: chroma_weight_flag_l0", currStream,dec_params);
      
      for (j=1; j<3; j++)
      {
        if (chroma_weight_flag_l0)
        {
          dec_params->img->wp_weight[0][i][j] = se_v("SH: chroma_weight_l0", currStream,dec_params);
          dec_params->img->wp_offset[0][i][j] = se_v("SH: chroma_offset_l0", currStream,dec_params);
		  dec_params->img->apply_weights_chr = 1;
        }
        else
        {
          dec_params->img->wp_weight[0][i][j] = 1<<dec_params->img->chroma_log2_weight_denom;
          dec_params->img->wp_offset[0][i][j] = 0;
        }
      }
    }
  }
  if ((dec_params->img->type == B_SLICE) && dec_params->active_pps->weighted_bipred_idc == 1)
  {
    for (i=0; i<dec_params->img->num_ref_idx_l1_active; i++)
    {
      luma_weight_flag_l1 = u_1("SH: luma_weight_flag_l1", currStream,dec_params);
      
      if (luma_weight_flag_l1)
      {
        dec_params->img->wp_weight[1][i][0] = se_v ("SH: luma_weight_l1", currStream,dec_params);
        dec_params->img->wp_offset[1][i][0] = se_v ("SH: luma_offset_l1", currStream,dec_params);
		dec_params->img->apply_weights_luma =1;
      }
      else
      {
        dec_params->img->wp_weight[1][i][0] = 1<<dec_params->img->luma_log2_weight_denom;
        dec_params->img->wp_offset[1][i][0] = 0;
      }
      
     //if (dec_params->active_sps->chroma_format_idc != 0)
      {
        chroma_weight_flag_l1 = u_1 ("SH: chroma_weight_flag_l1", currStream,dec_params);
        
        for (j=1; j<3; j++)
        {
          if (chroma_weight_flag_l1)
          {
            dec_params->img->wp_weight[1][i][j] = se_v("SH: chroma_weight_l1", currStream,dec_params);
            dec_params->img->wp_offset[1][i][j] = se_v("SH: chroma_offset_l1", currStream,dec_params);
			dec_params->img->apply_weights_chr = 1;
          }
          else
          {
            dec_params->img->wp_weight[1][i][j] = 1<<dec_params->img->chroma_log2_weight_denom;
            dec_params->img->wp_offset[1][i][j] = 0;
          }
        }
      }
    }  
  }
}
