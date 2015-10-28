
/*!
 ************************************************************************
 *  \file
 *     parset.c
 *  \brief
 *     Parameter Sets
 *  \author
 *     Main contributors (see contributors.h for copyright, address and affiliation details)
 *     - Stephan Wenger          <stewe@cs.tu-berlin.de>
 *
 ***********************************************************************
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "global.h"
#include "parsetcommon.h"
#include "parset.h"
#include "nalu.h"
#include "memalloc.h"
#include "fmo.h"
//#include "cabac.h"
#include "vlc.h"
#include "mbuffer.h"
#include "erc_api.h"

#if TRACE
#define SYMTRACESTRING(s) strncpy(sym->tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // do nothing
#endif

const byte ZZ_SCAN[16]  =
{  0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
};

const byte ZZ_SCAN8[64] =
{  0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};
										
//extern int UsedBits;								/*Changed by Saad Bin Shams [Removing Global Variables]*/	
//extern ColocatedParams *Co_located;				/*Changed by Saad Bin Shams [Removing Global Variables]*/
//seq_parameter_set_rbsp_t SeqParSet[MAXSPS];		/*Changed by Saad Bin Shams [Removing Global Variables]*/
//pic_parameter_set_rbsp_t PicParSet[MAXPPS];		/*Changed by Saad Bin Shams [Removing Global Variables]*/
//extern StorablePicture* dec_outputs->dec_picture; /*Changed by Saad Bin Shams [Removing Global Variables]*/	
extern int quant_intra_default[16];
extern int quant_inter_default[16];
extern int quant8_intra_default[64];
extern int quant8_inter_default[64];




 /***********************************************************************			
 *	 Function Argument List Changed [Removing Global Variables] 
 ************************************************************************/                          
//extern void init_frext(struct img_par *img,  h264_decoder* dec_params);
void init_frext(h264_decoder* dec_params);  //!< image parameters;

 /*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/
// syntax for scaling list matrix values
void Scaling_List(int *scalingList, int sizeOfScalingList, Boolean *UseDefaultScalingMatrix, 
				  Bitstream *s,    h264_decoder* dec_params)
{
  int j, scanj;
  int delta_scale, lastScale, nextScale;

  lastScale      = 8;
  nextScale      = 8;

  for(j=0; j<sizeOfScalingList; j++)
  {
    scanj = (sizeOfScalingList==16) ? ZZ_SCAN[j]:ZZ_SCAN8[j];

    if(nextScale!=0)
    {
      delta_scale = se_v (   "   : delta_sl   "                           , s,dec_params);
      nextScale = (lastScale + delta_scale + 256) % 256;
      *UseDefaultScalingMatrix = (scanj==0 && nextScale==0);
    }

    scalingList[scanj] = (nextScale==0) ? lastScale:nextScale;
    lastScale = scalingList[scanj];
  }
}
// fill sps with content of p
 /*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/
int InterpretSPS (DataPartition *p, seq_parameter_set_rbsp_t *sps,   h264_decoder* dec_params)
{
  unsigned i;
  int reserved_zero;
  Bitstream *s = p->bitstream;

  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (sps != NULL);

  dec_params->UsedBits = 0;

  sps->profile_idc                            = u_v  (8, "SPS: profile_idc"                           , s,dec_params);
  
  if ((sps->profile_idc!=66 ) &&
      (sps->profile_idc!=77 ) &&
      (sps->profile_idc!=88 ) &&
      (sps->profile_idc!=100 ) &&
      (sps->profile_idc!=110 ) && 
      (sps->profile_idc!=122 ) &&  
      (sps->profile_idc!=144 ))
  {
    return dec_params->UsedBits;
  }

  sps->constrained_set0_flag                  = u_1  (   "SPS: constrained_set0_flag"                 , s,dec_params);
  sps->constrained_set1_flag                  = u_1  (   "SPS: constrained_set1_flag"                 , s,dec_params);
  sps->constrained_set2_flag                  = u_1  (   "SPS: constrained_set2_flag"                 , s,dec_params);
  sps->constrained_set3_flag                  = u_1  (   "SPS: constrained_set3_flag"                 , s,dec_params);
  reserved_zero                               = u_v  (4, "SPS: reserved_zero_4bits"                   , s,dec_params);
  assert (reserved_zero==0);

  sps->level_idc                              = u_v  (8, "SPS: level_idc"                             , s,dec_params);
  
  sps->seq_parameter_set_id                   = ue_v ("SPS: seq_parameter_set_id"                     , s,dec_params);

  // Fidelity Range Extensions stuff
//  sps->bit_depth_luma_minus8   = 0;
//  sps->bit_depth_chroma_minus8 = 0;
//  dec_params->img->lossless_qpprime_flag   = 0;

  // Residue Color Transform
  //dec_params->img->residue_transform_flag = 0;

  sps->log2_max_frame_num_minus4              = ue_v ("SPS: log2_max_frame_num_minus4"                , s,dec_params);
  sps->pic_order_cnt_type                     = ue_v ("SPS: pic_order_cnt_type"                       , s,dec_params);

  if (sps->pic_order_cnt_type == 0)
    sps->log2_max_pic_order_cnt_lsb_minus4 = ue_v ("SPS: log2_max_pic_order_cnt_lsb_minus4"           , s,dec_params);
  else if (sps->pic_order_cnt_type == 1)
  {
    sps->delta_pic_order_always_zero_flag      = u_1  ("SPS: delta_pic_order_always_zero_flag"       , s,dec_params);
    sps->offset_for_non_ref_pic                = se_v ("SPS: offset_for_non_ref_pic"                 , s,dec_params);
    sps->offset_for_top_to_bottom_field        = se_v ("SPS: offset_for_top_to_bottom_field"         , s,dec_params);
    sps->num_ref_frames_in_pic_order_cnt_cycle = ue_v ("SPS: num_ref_frames_in_pic_order_cnt_cycle"  , s,dec_params);
    for(i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      sps->offset_for_ref_frame[i]               = se_v ("SPS: offset_for_ref_frame[i]"              , s,dec_params);
  }
  sps->num_ref_frames                        = ue_v ("SPS: num_ref_frames"                         , s,dec_params);
  sps->gaps_in_frame_num_value_allowed_flag  = u_1  ("SPS: gaps_in_frame_num_value_allowed_flag"   , s,dec_params);
  sps->pic_width_in_mbs_minus1               = ue_v ("SPS: pic_width_in_mbs_minus1"                , s,dec_params);
  sps->pic_height_in_map_units_minus1        = ue_v ("SPS: pic_height_in_map_units_minus1"         , s,dec_params);
  sps->frame_mbs_only_flag                   = u_1  ("SPS: frame_mbs_only_flag"                    , s,dec_params);

  sps->direct_8x8_inference_flag             = u_1  ("SPS: direct_8x8_inference_flag"              , s,dec_params);
  sps->frame_cropping_flag                   = u_1  ("SPS: frame_cropping_flag"                , s,dec_params);

  if (sps->frame_cropping_flag)
  {
    sps->frame_cropping_rect_left_offset      = ue_v ("SPS: frame_cropping_rect_left_offset"           , s,dec_params);
    sps->frame_cropping_rect_right_offset     = ue_v ("SPS: frame_cropping_rect_right_offset"          , s,dec_params);
    sps->frame_cropping_rect_top_offset       = ue_v ("SPS: frame_cropping_rect_top_offset"            , s,dec_params);
    sps->frame_cropping_rect_bottom_offset    = ue_v ("SPS: frame_cropping_rect_bottom_offset"         , s,dec_params);
  }
  sps->vui_parameters_present_flag           = u_1  ("SPS: vui_parameters_present_flag"            , s,dec_params);
  
  InitVUI(sps);
  ReadVUI(p, sps,dec_params);
  
  sps->Valid = TRUE;

  return dec_params->UsedBits;
}


void InitVUI(seq_parameter_set_rbsp_t *sps)
{
  sps->vui_seq_parameters.matrix_coefficients = 2;
}

 /*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/

int ReadVUI(DataPartition *p, seq_parameter_set_rbsp_t *sps,   h264_decoder* dec_params)
{
  Bitstream *s = p->bitstream;
  if (sps->vui_parameters_present_flag)
  {
    sps->vui_seq_parameters.aspect_ratio_info_present_flag = u_1  ("VUI: aspect_ratio_info_present_flag"   , s,dec_params);
    if (sps->vui_seq_parameters.aspect_ratio_info_present_flag)
    {
      sps->vui_seq_parameters.aspect_ratio_idc             = u_v  ( 8, "VUI: aspect_ratio_idc"              , s,dec_params);
      if (255==sps->vui_seq_parameters.aspect_ratio_idc)
      {
        sps->vui_seq_parameters.sar_width                  = u_v  (16, "VUI: sar_width"                     , s,dec_params);
        sps->vui_seq_parameters.sar_height                 = u_v  (16, "VUI: sar_height"                    , s,dec_params);
      }
  }

    sps->vui_seq_parameters.overscan_info_present_flag     = u_1  ("VUI: overscan_info_present_flag"        , s,dec_params);
    if (sps->vui_seq_parameters.overscan_info_present_flag)
    {
      sps->vui_seq_parameters.overscan_appropriate_flag    = u_1  ("VUI: overscan_appropriate_flag"         , s,dec_params);
    }

    sps->vui_seq_parameters.video_signal_type_present_flag = u_1  ("VUI: video_signal_type_present_flag"    , s,dec_params);
    if (sps->vui_seq_parameters.video_signal_type_present_flag)
    {
      sps->vui_seq_parameters.video_format                    = u_v  ( 3,"VUI: video_format"                      , s,dec_params);
      sps->vui_seq_parameters.video_full_range_flag           = u_1  (   "VUI: video_full_range_flag"             , s,dec_params);
      sps->vui_seq_parameters.colour_description_present_flag = u_1  (   "VUI: color_description_present_flag"    , s,dec_params);
      if(sps->vui_seq_parameters.colour_description_present_flag)
      {
        sps->vui_seq_parameters.colour_primaries              = u_v  ( 8,"VUI: colour_primaries"                  , s,dec_params);
        sps->vui_seq_parameters.transfer_characteristics      = u_v  ( 8,"VUI: transfer_characteristics"          , s,dec_params);
        sps->vui_seq_parameters.matrix_coefficients           = u_v  ( 8,"VUI: matrix_coefficients"               , s,dec_params);
      }
    }
    sps->vui_seq_parameters.chroma_location_info_present_flag = u_1  (   "VUI: chroma_loc_info_present_flag"      , s,dec_params);
    if(sps->vui_seq_parameters.chroma_location_info_present_flag)
    {
      sps->vui_seq_parameters.chroma_sample_loc_type_top_field     = ue_v  ( "VUI: chroma_sample_loc_type_top_field"    , s,dec_params);
      sps->vui_seq_parameters.chroma_sample_loc_type_bottom_field  = ue_v  ( "VUI: chroma_sample_loc_type_bottom_field" , s,dec_params);
    }
    sps->vui_seq_parameters.timing_info_present_flag          = u_1  ("VUI: timing_info_present_flag"           , s,dec_params);
    if (sps->vui_seq_parameters.timing_info_present_flag)
    {
      sps->vui_seq_parameters.num_units_in_tick               = u_v  (32,"VUI: num_units_in_tick"               , s,dec_params);
      sps->vui_seq_parameters.time_scale                      = u_v  (32,"VUI: time_scale"                      , s,dec_params);
      sps->vui_seq_parameters.fixed_frame_rate_flag           = u_1  (   "VUI: fixed_frame_rate_flag"           , s,dec_params);
    }
    sps->vui_seq_parameters.nal_hrd_parameters_present_flag   = u_1  ("VUI: nal_hrd_parameters_present_flag"    , s,dec_params);
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
    {
      ReadHRDParameters(p, &(sps->vui_seq_parameters.nal_hrd_parameters),dec_params);
    }
    sps->vui_seq_parameters.vcl_hrd_parameters_present_flag   = u_1  ("VUI: vcl_hrd_parameters_present_flag"    , s,dec_params);
    if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
      ReadHRDParameters(p, &(sps->vui_seq_parameters.vcl_hrd_parameters),dec_params);
    }
    if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag || sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
      sps->vui_seq_parameters.low_delay_hrd_flag             =  u_1  ("VUI: low_delay_hrd_flag"                 , s,dec_params);
    }
    sps->vui_seq_parameters.pic_struct_present_flag          =  u_1  ("VUI: pic_struct_present_flag   "         , s,dec_params);
    sps->vui_seq_parameters.bitstream_restriction_flag       =  u_1  ("VUI: bitstream_restriction_flag"         , s,dec_params);
    if (sps->vui_seq_parameters.bitstream_restriction_flag)
    {
      sps->vui_seq_parameters.motion_vectors_over_pic_boundaries_flag =  u_1  ("VUI: motion_vectors_over_pic_boundaries_flag", s,dec_params);
      sps->vui_seq_parameters.max_bytes_per_pic_denom                 =  ue_v ("VUI: max_bytes_per_pic_denom"                , s,dec_params);
      sps->vui_seq_parameters.max_bits_per_mb_denom                   =  ue_v ("VUI: max_bits_per_mb_denom"                  , s,dec_params);
      sps->vui_seq_parameters.log2_max_mv_length_horizontal           =  ue_v ("VUI: log2_max_mv_length_horizontal"          , s,dec_params);
      sps->vui_seq_parameters.log2_max_mv_length_vertical             =  ue_v ("VUI: log2_max_mv_length_vertical"            , s,dec_params);
      sps->vui_seq_parameters.num_reorder_frames                      =  ue_v ("VUI: num_reorder_frames"                     , s,dec_params);
      sps->vui_seq_parameters.max_dec_frame_buffering                 =  ue_v ("VUI: max_dec_frame_buffering"                , s,dec_params);
    }
  }
  
  return 0;
}

 /*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/

int ReadHRDParameters(DataPartition *p, hrd_parameters_t *hrd,   h264_decoder* dec_params)
{
  Bitstream *s = p->bitstream;
  unsigned int SchedSelIdx;

  hrd->cpb_cnt_minus1                                      = ue_v (   "VUI: cpb_cnt_minus1"                       , s,dec_params);
  hrd->bit_rate_scale                                      = u_v  ( 4,"VUI: bit_rate_scale"                       , s,dec_params);
  hrd->cpb_size_scale                                      = u_v  ( 4,"VUI: cpb_size_scale"                       , s,dec_params);

  for( SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++ ) 
  {
	  hrd->bit_rate_value_minus1[ SchedSelIdx ]             = ue_v  ( "VUI: bit_rate_value_minus1"                  , s,dec_params);
	  hrd->cpb_size_value_minus1[ SchedSelIdx ]             = ue_v  ( "VUI: cpb_size_value_minus1"                  , s,dec_params);
	  hrd->cbr_flag[ SchedSelIdx ]                          = u_1   ( "VUI: cbr_flag"                               , s,dec_params);
  }

  hrd->initial_cpb_removal_delay_length_minus1            = u_v  ( 5,"VUI: initial_cpb_removal_delay_length_minus1" , s,dec_params);
  hrd->cpb_removal_delay_length_minus1                    = u_v  ( 5,"VUI: cpb_removal_delay_length_minus1"         , s,dec_params);
  hrd->dpb_output_delay_length_minus1                     = u_v  ( 5,"VUI: dpb_output_delay_length_minus1"          , s,dec_params);
  hrd->time_offset_length                                 = u_v  ( 5,"VUI: time_offset_length"          , s,dec_params);

  return 0;
}

 /*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/

int InterpretPPS (DataPartition *p, pic_parameter_set_rbsp_t *pps,   h264_decoder* dec_params)
{
  unsigned i;
  int NumberBitsPerSliceGroupId;
  Bitstream *s = p->bitstream;
  
  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (pps != NULL);

  dec_params->UsedBits = 0;

  pps->pic_parameter_set_id                  = ue_v ("PPS: pic_parameter_set_id"                   , s,dec_params);
  pps->seq_parameter_set_id                  = ue_v ("PPS: seq_parameter_set_id"                   , s,dec_params);
  pps->entropy_coding_mode_flag              = u_1  ("PPS: entropy_coding_mode_flag"               , s,dec_params);

  //! Note: as per JVT-F078 the following bit is unconditional.  If F078 is not accepted, then
  //! one has to fetch the correct SPS to check whether the bit is present (hopefully there is
  //! no consistency problem :-(
  //! The current encoder code handles this in the same way.  When you change this, don't forget
  //! the encoder!  StW, 12/8/02
  pps->pic_order_present_flag                = u_1  ("PPS: pic_order_present_flag"                 , s,dec_params);

  pps->num_slice_groups_minus1               = ue_v ("PPS: num_slice_groups_minus1"                , s,dec_params);

  // FMO stuff begins here
  if (pps->num_slice_groups_minus1 > 0)
  {
    pps->slice_group_map_type               = ue_v ("PPS: slice_group_map_type"                , s,dec_params);
    if (pps->slice_group_map_type == 0)
    {
      for (i=0; i<=pps->num_slice_groups_minus1; i++)
        pps->run_length_minus1 [i]                  = ue_v ("PPS: run_length_minus1 [i]"              , s,dec_params);
    }
    else if (pps->slice_group_map_type == 2)
    {
      for (i=0; i<pps->num_slice_groups_minus1; i++)
      {
        //! JVT-F078: avoid reference of SPS by using ue(v) instead of u(v)
        pps->top_left [i]                          = ue_v ("PPS: top_left [i]"                        , s,dec_params);
        pps->bottom_right [i]                      = ue_v ("PPS: bottom_right [i]"                    , s,dec_params);
      }
    }
    else if (pps->slice_group_map_type == 3 ||
             pps->slice_group_map_type == 4 ||
             pps->slice_group_map_type == 5)
    {
      pps->slice_group_change_direction_flag     = u_1  ("PPS: slice_group_change_direction_flag"      , s,dec_params);
      pps->slice_group_change_rate_minus1        = ue_v ("PPS: slice_group_change_rate_minus1"         , s,dec_params);
    }
    else if (pps->slice_group_map_type == 6)
    {
      if (pps->num_slice_groups_minus1+1 >4)
        NumberBitsPerSliceGroupId = 3;
      else if (pps->num_slice_groups_minus1+1 > 2)
        NumberBitsPerSliceGroupId = 2;
      else
        NumberBitsPerSliceGroupId = 1;
      //! JVT-F078, exlicitly signal number of MBs in the map
      pps->num_slice_group_map_units_minus1      = ue_v ("PPS: num_slice_group_map_units_minus1"               , s,dec_params);
      for (i=0; i<=pps->num_slice_group_map_units_minus1; i++)
        pps->slice_group_id[i] = u_v (NumberBitsPerSliceGroupId, "slice_group_id[i]", s,dec_params);
    }
  }

  // End of FMO stuff

  pps->num_ref_idx_l0_active_minus1          = ue_v ("PPS: num_ref_idx_l0_active_minus1"           , s,dec_params);
  pps->num_ref_idx_l1_active_minus1          = ue_v ("PPS: num_ref_idx_l1_active_minus1"           , s,dec_params);
  pps->weighted_pred_flag                    = u_1  ("PPS: weighted prediction flag"               , s,dec_params);
  pps->weighted_bipred_idc                   = u_v  ( 2, "PPS: weighted_bipred_idc"                , s,dec_params);
  pps->pic_init_qp_minus26                   = se_v ("PPS: pic_init_qp_minus26"                    , s,dec_params);
  pps->pic_init_qs_minus26                   = se_v ("PPS: pic_init_qs_minus26"                    , s,dec_params);

  pps->chroma_qp_index_offset                = se_v ("PPS: chroma_qp_index_offset"                 , s,dec_params);

  pps->deblocking_filter_control_present_flag = u_1 ("PPS: deblocking_filter_control_present_flag" , s,dec_params);
  pps->constrained_intra_pred_flag           = u_1  ("PPS: constrained_intra_pred_flag"            , s,dec_params);
  pps->redundant_pic_cnt_present_flag        = u_1  ("PPS: redundant_pic_cnt_present_flag"         , s,dec_params);
  /*
  if(more_rbsp_data(s->streamBuffer, s->frame_bitoffset,s->bitstream_length)) // more_data_in_rbsp()
  {
    //Fidelity Range Extensions Stuff
    pps->pic_scaling_matrix_present_flag     = u_1  ("PPS: pic_scaling_matrix_present_flag"        , s,dec_params);

    if(pps->pic_scaling_matrix_present_flag)
    {
      for(i=0; i<6; i++)
      {
        pps->pic_scaling_list_present_flag[i]= u_1  ("PPS: pic_scaling_list_present_flag"          , s,dec_params);

        if(pps->pic_scaling_list_present_flag[i])
        {
          if(i<6)
            Scaling_List(pps->ScalingList4x4[i], 16, &pps->UseDefaultScalingMatrix4x4Flag[i], s,dec_params);
          else
            Scaling_List(pps->ScalingList8x8[i-6], 64, &pps->UseDefaultScalingMatrix8x8Flag[i-6], s,dec_params);
        }
      }
    }
    pps->second_chroma_qp_index_offset      = se_v ("PPS: second_chroma_qp_index_offset"          , s,dec_params);
  }
  else
  {
    pps->second_chroma_qp_index_offset      = pps->chroma_qp_index_offset;
  }
  */

  pps->second_chroma_qp_index_offset      = pps->chroma_qp_index_offset;
  pps->Valid = TRUE;
  return dec_params->UsedBits;
}


void PPSConsistencyCheck (pic_parameter_set_rbsp_t *pps)
{
  printf ("Consistency checking a picture parset, to be implemented\n");
//  if (pps->seq_parameter_set_id invalid then do something)
}

void SPSConsistencyCheck (seq_parameter_set_rbsp_t *sps)
{
  printf ("Consistency checking a sequence parset, to be implemented\n");
}
 /*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/

void MakePPSavailable (int id, pic_parameter_set_rbsp_t *pps,  h264_decoder* dec_params)
{
  assert (pps->Valid == TRUE);

  if (dec_params->PicParSet[id].Valid == TRUE && dec_params->PicParSet[id].slice_group_id != NULL)
  {
    h264_free (dec_params->PicParSet[id].slice_group_id);
  }
    memcpy (&dec_params->PicParSet[id], pps, sizeof (pic_parameter_set_rbsp_t));

  
  if ((dec_params->PicParSet[id].slice_group_id = (unsigned int *)h264_malloc ((dec_params->PicParSet[id].num_slice_group_map_units_minus1+1) * sizeof(int))) == NULL)
  {
	  printf("MakePPSavailable: Cannot calloc slice_group_id");
	  exit(0);
  }
    memcpy (dec_params->PicParSet[id].slice_group_id, pps->slice_group_id, (pps->num_slice_group_map_units_minus1+1)*sizeof(int));
}
/*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/

void MakeSPSavailable (int id, seq_parameter_set_rbsp_t *sps,   h264_decoder* dec_params)
{
  assert (sps->Valid == TRUE);
    memcpy (&dec_params->SeqParSet[id], sps, sizeof (seq_parameter_set_rbsp_t));
}

/*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/

void ProcessSPS_baseline(NALU_t *nalu, h264_decoder* dec_params)
{
  //DataPartition *dp = AllocPartition(1, dec_params, dec_outputs);
  //seq_parameter_set_rbsp_t *sps = AllocSPS(dec_params,dec_outputs);

  int dummy;
  DataPartition *dp = dec_params->dp;
  seq_parameter_set_rbsp_t *sps = dec_params->sps;

  //STREAM BUFFERE COPY REMOVED
  //memcpy_mmx (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->streamBuffer = &nalu->buf[1];

	dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);
  dp->bitstream->ei_flag = 0;
	dp->bitstream->frame_bitoffset = 0;
  dummy = InterpretSPS (dp, sps,dec_params);

  if (sps->Valid)
  {
    if (dec_params->active_sps)
    {
      if (sps->seq_parameter_set_id == dec_params->active_sps->seq_parameter_set_id)
      {
        if (!sps_is_equal(sps, dec_params->active_sps))
        {
          if (dec_params->dec_picture)
          {
            // this may only happen on slice loss
            exit_picture(dec_params);
          }
          dec_params->active_sps=NULL;
        }
      }
    }
    // SPSConsistencyCheck (pps);
    MakeSPSavailable (sps->seq_parameter_set_id, sps,dec_params);
    dec_params->img->profile_idc = sps->profile_idc; //ADD-VG
  }

  //FreePartition (dp, 1);
  //FreeSPS (sps);
}

/*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/

void ProcessPPS_baseline (NALU_t *nalu,  h264_decoder* dec_params)
{
  DataPartition *dp;
  pic_parameter_set_rbsp_t *pps;
  int dummy;

  //dp = AllocPartition(1, dec_params, dec_outputs);
  //pps = AllocPPS(dec_params,dec_outputs);

  dp = dec_params->dp;
  pps = dec_params->pps;

  // STREAM BUFFER COPY REMOVED
  //memcpy_mmx (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->streamBuffer = &nalu->buf[1];

dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);
  dp->bitstream->ei_flag = 0;
dp->bitstream->frame_bitoffset = 0;
  dummy = InterpretPPS (dp, pps, dec_params);
  // PPSConsistencyCheck (pps);
  if (dec_params->active_pps)
  {
    if (pps->pic_parameter_set_id == dec_params->active_pps->pic_parameter_set_id)
    {
      if (!pps_is_equal(pps, dec_params->active_pps))
      {
        if (dec_params->dec_picture)
        {
          // this may only happen on slice loss
          exit_picture(dec_params);
        }
        dec_params->active_pps = NULL;
      }
    }
  }

  MakePPSavailable (pps->pic_parameter_set_id, pps,dec_params);

  //FreePartition (dp, 1);
  //FreePPS (pps);
}

void activate_sps_baseline(seq_parameter_set_rbsp_t *sps,  h264_decoder* dec_params )
{
  if (dec_params->active_sps != sps)
  {
    if (dec_params->dec_picture)
    {
      // this may only happen on slice loss
      exit_picture(dec_params);
    }
    dec_params->active_sps = sps;

    // Fidelity Range Extensions stuff (part 1)
    dec_params->img->MaxFrameNum = 1<<(sps->log2_max_frame_num_minus4+4);
    dec_params->img->FrameWidthInMbs = (sps->pic_width_in_mbs_minus1 +1);
//    dec_params->img->PicHeightInMapUnits = (sps->pic_height_in_map_units_minus1 +1);
    dec_params->img->FrameHeightInMbs = sps->pic_height_in_map_units_minus1 +1;
    dec_params->img->FrameSizeInMbs = dec_params->img->FrameWidthInMbs * dec_params->img->FrameHeightInMbs;
    
    dec_params->img->width = dec_params->img->FrameWidthInMbs * MB_BLOCK_SIZE;
    dec_params->img->height = dec_params->img->FrameHeightInMbs * MB_BLOCK_SIZE;

    dec_params->img->width_cr = dec_params->img->width /2;
    dec_params->img->height_cr = dec_params->img->height / 2;


    //init_frext(dec_params->img,dec_params);                                               
	//init_frext(dec_params);                                               
//    dec_params->img->num_cdc_coeff = 4;

    init_global_buffers_baseline(dec_params);
    if (!dec_params->img->no_output_of_prior_pics_flag)
    {
      flush_dpb(dec_params);
    }
    init_dpb(dec_params);

	
    if (NULL!=dec_params->Co_located)
    {
      //free_colocated(dec_params->Co_located,dec_params,dec_outputs);
		free_colocated(dec_params);
    }
	
    //dec_params->Co_located = alloc_colocated (dec_params->img->width, dec_params->img->height,sps->mb_adaptive_frame_field_flag,dec_params,dec_outputs);
    //ercInit(dec_params->img->width, dec_params->img->height, 1,dec_params,dec_outputs);
	//dec_params->Co_located = alloc_colocated (dec_params->img->width, dec_params->img->height,sps->mb_adaptive_frame_field_flag,dec_params,dec_outputs);
	dec_params->Co_located = alloc_colocated (dec_params);

	

    //ercInit(dec_params->img->width, dec_params->img->height, 1,dec_params,dec_outputs);

	// IF FLAG IS ENABLED INITIALIZE ERROR CONCEALMENT
	if (dec_params->img->errorConcealmentFlag) 
	{
	ercInit(1,dec_params);
  }
	
  }
}
/*************************************************************************
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 **************************************************************************/
  

void activate_pps_baseline(pic_parameter_set_rbsp_t *pps, h264_decoder* dec_params)
{
  if (dec_params->active_pps != pps)
  {
    if (dec_params->dec_picture)
    {
      // this may only happen on slice loss
      exit_picture(dec_params);
    }
    dec_params->active_pps = pps;
    //Fidelity Range Extensions stuff (part 2)
	//dec_params->img->Transform8x8Mode = pps->transform_8x8_mode_flag;
  }
}  
