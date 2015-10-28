       
/*!
 ***********************************************************************
 * \file image.c
 *
 * \brief
 *    Decode a Slice
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langoy               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *    - Jani Lainema                    <jani.lainema@nokia.com>
 *    - Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *    - Byeong-Moon Jeon                <jeonbm@lge.com>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *    - Gabi Blaettermann               <blaetter@hhi.de>
 *    - Ye-Kui Wang                     <wyk@ieee.org>
 *    - Antti Hallapuro                 <antti.hallapuro@nokia.com>
 *    - Alexis Tourapis                 <alexismt@ieee.org>
 ***********************************************************************
 */
/*#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif*/

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "annexb.h"
#include "rtp.h"
#include "image.h"
#include "erc_api.h"
#include "nalu.h"
#include "header.h"
#include "parset.h"
#include "fmo.h"
#include "sei.h"
#include "errorconcealment.h"
#include "loopfilter.h"

#define BLOCK8X8_SIZE 2*BLOCK_SIZE

#define BLOCK16x16_SIZE 4*BLOCK_SIZE

#define PADD_SIZE_LUMA		32
#define PADD_SIZE_CHROMA	16

extern objectBuffer_t *erc_object_list;

/***********************************************************************
*\brief decodes one frame
*	 Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/

int decode_one_frame( h264_decoder* dec_params )
{
	ImageParameters *img = dec_params->img;
	int current_header;
	Slice *currSlice = img->currentSlice;
	img->current_slice_nr = 0;
	img->current_mb_nr = -4711;     // initialized to an impossible value for debugging -- correct value is taken from slice header
	currSlice->next_header = -8888; // initialized to an impossible value for debugging -- correct value is taken from slice header
	img->num_dec_mb = 0;
	
	while ((currSlice->next_header != EOS && currSlice->next_header != SOP))
	{
		current_header = read_new_slice(dec_params);
		//printf("done till here.\n");

		if (current_header == EOS)
		{
			exit_picture(dec_params);
			return EOS;
		}
		if (dec_params->active_pps->entropy_coding_mode_flag)
		{
			init_contexts (dec_params);
			//cabac_new_slice();
		}
		
/*		if ( (dec_params->active_pps->weighted_bipred_idc > 0  && (img->type == B_SLICE)) || (dec_params->active_pps->weighted_pred_flag && img->type !=I_SLICE))
			fill_wp_params(img,dec_params);
*/		
		if (current_header == SOP || current_header == SOS)
		{
			Boolean end_of_slice = FALSE;
			img->cod_counter=-1;
/*			
			if (img->type == B_SLICE) 
			{
				compute_colocated(dec_params);
			}
*/			while (end_of_slice == FALSE) // loop over macroblocks
			{	
				// Initializes the current macroblock
				start_macroblock(dec_params);	

				
				// Get the syntax elements from the NAL
				if (dec_params->active_pps->entropy_coding_mode_flag)
				{
					//read_flag = read_one_macroblock_CABAC(dec_params,dec_outputs);
				}
				else
				{
					read_one_macroblock(dec_params);
				}
				
				decode_one_macroblock(dec_params);
				
				if (dec_params->img->errorConcealmentFlag) 
				{
					ercWriteMBMODEandMV_baseline(dec_params);
				}
				
				end_of_slice = exit_macroblock(1,dec_params);
			}
			exit_slice_baseline(dec_params);
		}
		img->current_slice_nr++;
		if(img->num_dec_mb == img->FrameSizeInMbs)
			exit_picture(dec_params);
	}
	//exit_picture(dec_params);
	
	return (SOP);
}

/***********************************************************************
*	 Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/
void reorder_lists_baseline(h264_decoder* dec_params )
{
  int currSliceType=dec_params->img->type;
  Slice * currSlice=dec_params->img->currentSlice;


  if (currSliceType != I_SLICE)
  {
    if (currSlice->ref_pic_list_reordering_flag_l0)
    {
      reorder_ref_pic_list(dec_params->listX[0], &dec_params->listXsize[0], 
                           dec_params->img->num_ref_idx_l0_active - 1, 
                           currSlice->remapping_of_pic_nums_idc_l0, 
                           currSlice->abs_diff_pic_num_minus1_l0, 
                           currSlice->long_term_pic_idx_l0,dec_params);
	  
//      reorder_ref_pic_list(dec_params->listX, &dec_params->listXsize, 
//                         dec_params->img->num_ref_idx_l0_active - 1, 
//                           currSlice->remapping_of_pic_nums_idc_l0, 
//                          currSlice->abs_diff_pic_num_minus1_l0, 
//                           currSlice->long_term_pic_idx_l0,dec_params);
	  
    }

	if (NULL == dec_params->listX[0][dec_params->img->num_ref_idx_l0_active-1])
    //if (NULL == dec_params->listX[dec_params->img->num_ref_idx_l0_active-1])
    {
      printf("RefPicList0[ num_ref_idx_l0_active_minus1 ] is equal to 'no reference picture', invalid bitstream");
	  exit(0);
    }

    // that's a definition
    dec_params->listXsize[0] = dec_params->img->num_ref_idx_l0_active;
    //dec_params->listXsize = dec_params->img->num_ref_idx_l0_active;

  }

 
  if (currSliceType == B_SLICE)
  {
    if (currSlice->ref_pic_list_reordering_flag_l1)
    {
      reorder_ref_pic_list(dec_params->listX[1], &dec_params->listXsize[1], 
                           dec_params->img->num_ref_idx_l1_active - 1, 
                           currSlice->remapping_of_pic_nums_idc_l1, 
                           currSlice->abs_diff_pic_num_minus1_l1, 
                           currSlice->long_term_pic_idx_l1,dec_params);
    }
    if (NULL == dec_params->listX[1][dec_params->img->num_ref_idx_l1_active-1])
    {
      printf("RefPicList1[ num_ref_idx_l1_active_minus1 ] is equal to 'no reference picture', invalid bitstream");
	  exit(0);
    }
    // that's a definition
    dec_params->listXsize[1] = dec_params->img->num_ref_idx_l1_active;
  }
  

  free_ref_pic_list_reordering_buffer(currSlice);
}


/*!
************************************************************************
* \brief
*    Reads new slice from bit_stream
*-----------------------------------------------------------------------			
*	- Code not related to baseline removed.
*	- While Loop & Switch-Case structure removed from the function 
*	- ProcessSPS & ProcessPPS are not called any more in 
*	  read_new_slice
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/

int read_new_slice(  h264_decoder* dec_params )
{
  NALU_t *nalu = dec_params->nalu;

  int current_header;
  int ret;
  int BitsUsedByHeader;
  Slice *currSlice = dec_params->img->currentSlice;
  Bitstream *currStream;
  long ftell_position, expected_slice_type;
  
  expected_slice_type = NALU_TYPE_DPA;

  while (1)
  {
    ftell_position = ftell(dec_params->bits_Annexb);

	
    if (dec_params->input->FileFormat == PAR_OF_ANNEXB)
      ret=GetAnnexbNALU (nalu,dec_params);
    else
      ret=GetRTPNALU (nalu,dec_params);

    //In some cases, zero_byte shall be present. If current NALU is a VCL NALU, we can't tell
    //whether it is the first VCL NALU at this point, so only non-VCL NAL unit is checked here.
    CheckZeroByteNonVCL(nalu, dec_params);

    NALUtoRBSP(nalu);
    
    if (ret < 0)
      printf ("Error while getting the NALU in file format %s, exit\n", dec_params->input->FileFormat==PAR_OF_ANNEXB?"Annex B":"RTP");
    if (ret == 0)
    {
      if(expected_slice_type != NALU_TYPE_DPA)
      {
        /* oops... we found the next slice, go back! */
        fseek(dec_params->bits_Annexb, ftell_position, SEEK_SET);
        return current_header;
      }
      else
        return EOS;
    }

    // Got a NALU
    if (nalu->forbidden_bit)
    {
      printf ("Found NALU w/ forbidden_bit set, bit error?  Let's try...\n");
    }

    switch (nalu->nal_unit_type)
    {
      case NALU_TYPE_SLICE:
      case NALU_TYPE_IDR:
        dec_params->img->idr_flag = (nalu->nal_unit_type == NALU_TYPE_IDR);
        dec_params->img->nal_reference_idc = nalu->nal_reference_idc;
        dec_params->img->disposable_flag = (nalu->nal_reference_idc == NALU_PRIORITY_DISPOSABLE);
        currSlice->max_part_nr = 1;
        //currSlice->ei_flag = 0;
        currStream = currSlice->partArr[0].bitstream;
        currStream->ei_flag = 0;
        currStream->frame_bitoffset = currStream->read_len = 0;// ?
		// STREAM BUFFER EXTRA COPY REMOVED
        //memcpy_mmx (currStream->streamBuffer, &nalu->buf[1], nalu->len-1);
		currStream->streamBuffer = &nalu->buf[1];

        currStream->code_len = currStream->bitstream_length = RBSPtoSODB(currStream->streamBuffer, nalu->len-1);		

        BitsUsedByHeader = FirstPartOfSliceHeader(dec_params);
		UseParameterSet_baseline ( currSlice->pic_parameter_set_id, dec_params );
		BitsUsedByHeader+= RestOfSliceHeader_baseline( dec_params );		

		// IF FMO FLAG PRESENT IN THE STREAM
		if (dec_params->active_pps->num_slice_groups_minus1)
		{
			FmoInit( dec_params );
		}        
        if(is_new_picture(dec_params))
        {
			init_picture( dec_params );
          
          current_header = SOP;
          //check zero_byte if it is also the first NAL unit in the access unit
          CheckZeroByteVCL(nalu,dec_params);
        }
        else
          current_header = SOS;

		init_lists_baseline(dec_params);
		reorder_lists_baseline(dec_params);

        // From here on, active_sps, active_pps and the slice header are valid
        dec_params->img->current_mb_nr = currSlice->start_mb_nr;

        if (dec_params->active_pps->entropy_coding_mode_flag)
        {
            int ByteStartPosition = currStream->frame_bitoffset/8;
            if (currStream->frame_bitoffset%8 != 0) 
            {
                ByteStartPosition++;
            }
            arideco_start_decoding (&currSlice->partArr[0].de_cabac, currStream->streamBuffer, ByteStartPosition, &currStream->read_len, dec_params->img->type);
        }
        return current_header;
        break;
      case NALU_TYPE_DPA:
        break;
		
      case NALU_TYPE_DPB:

        break;

      case NALU_TYPE_DPC:

        break;

      case NALU_TYPE_SEI:
        printf ("read_new_slice: Found NALU_TYPE_SEI, len %d\n", nalu->len);
        InterpretSEIMessage(nalu->buf,nalu->len,dec_params->img,dec_params);
        break;
      case NALU_TYPE_PPS:
        ProcessPPS_baseline(nalu,dec_params);
        break;

      case NALU_TYPE_SPS:
        ProcessSPS_baseline(nalu,dec_params);
        break;
      case NALU_TYPE_AUD:
        
        break;
      case NALU_TYPE_EOSEQ:
        
        break;
      case NALU_TYPE_EOSTREAM:
        
        break;
      case NALU_TYPE_FILL:
        
        
        break;
      default:
        printf ("Found NALU type %d, len %d undefined, ignore NALU, moving on\n", nalu->nal_unit_type, nalu->len);
    }
  }
  return  current_header;
}

/********************************************************************************\
* FUNCTION     : init_picture         .                                          *
*--------------------------------------------------------------------------------*
* DISCRIPTION  : Initialization of StorablePicture structure.                    *
*--------------------------------------------------------------------------------*
* ARGUMENTS    : Decoder parameters,                                             *
*              : Decoder Outputs.                                                *
*--------------------------------------------------------------------------------*
* RETURN VALUE : NONE.                                                           *
*--------------------------------------------------------------------------------*
* OUTPUT       : NONE.                                                           *
*--------------------------------------------------------------------------------*
* CHANGE LOG   : Below are the changes done to the function                      *
*--------------------------------------------------------------------------------*
* DATE         :                                                                 *
*--------------------------------------------------------------------------------*
*              : Function Argument List Changed [Removing Global Variables]      *
*              : Input parameters added are                                      *
*              : - h264_decoder* dec_params                                    *
*                                                                                *
*              : <saad.shams@inforient.com>                                      *
*--------------------------------------------------------------------------------*
* DATE         : 21-11-2005                                                      *
*--------------------------------------------------------------------------------*
*              : - Code not related to baseline removed.                         *
*                                                                                *
*              : "Muhammad Tahir Awan" <tahir.awan@inforient.com>,               *
*              : "Umair Razzak" <umair.razzak@inforient.com>                     *
\********************************************************************************/
void init_picture( h264_decoder* dec_params )
{
	int i;
	ImageParameters*			img			= dec_params->img;  
	StorablePicture*			dec_picture = dec_params->dec_picture;
	seq_parameter_set_rbsp_t*	active_sps	= dec_params->active_sps;
	
	if (dec_picture)
	{
		// this may only happen on slice loss
		exit_picture(dec_params);
	}
	
	if (img->frame_num != img->pre_frame_num && img->frame_num != (img->pre_frame_num + 1) % img->MaxFrameNum) 
	{
		if (active_sps->gaps_in_frame_num_value_allowed_flag == 0)
		{
			// Advanced Error Concealment would be called here to combat unintentional loss of pictures.
			printf("An unintentional loss of pictures occurs! Exit\n");
			exit(0);
		}
		fill_frame_num_gap(dec_params);
	}
	img->pre_frame_num = img->frame_num;
	img->num_dec_mb = 0;
	
	//calculate POC
	decode_poc(dec_params);
	
#ifdef WIN32
		_ftime (&(img->tstruct_start));             // start time ms
#else
		//ftime (&(img->tstruct_start));              // start time ms
#endif
		time( &(img->ltime_start));                // start time s
	
	dec_params->dec_picture = dec_params->dec_pictures_list[dec_params->picture_offset];
	dec_picture = dec_params->dec_picture;
	
	dec_picture->is_empty = 0;
	
	dec_picture->chroma_qp_offset[0] = dec_params->active_pps->chroma_qp_index_offset;
	dec_picture->chroma_qp_offset[1] = dec_params->active_pps->second_chroma_qp_index_offset;
	
	if (dec_params->img->errorConcealmentFlag) 
	{
		ercReset(dec_params);
		dec_params->erc_mvperMB = 0;
	}
	
	dec_picture->poc=img->framepoc;
    
	img->current_slice_nr=0;
	
	if (img->type > 4)
	{
		set_ec_flag(SE_PTYPE,dec_params);
		img->type = P_SLICE;  // concealed element
	}
	
	memset(img->nz_coeff1,0, (dec_params->img->FrameSizeInMbs*24)*sizeof(char));
	
	memset(img->slice_nr,-1, (img->FrameSizeInMbs)*sizeof(char));
	
	for(i=0; i<(int)img->FrameSizeInMbs; i++)
	{
		img->ei_flag[i] = 1 ;
	}
	
	img->mb_y = img->mb_x = 0;
	
	dec_picture->slice_type							= img->type;
	dec_picture->used_for_reference					= (img->nal_reference_idc != 0);
	dec_picture->idr_flag							= img->idr_flag;
	dec_picture->no_output_of_prior_pics_flag		= img->no_output_of_prior_pics_flag;
	dec_picture->long_term_reference_flag			= img->long_term_reference_flag;
	dec_picture->adaptive_ref_pic_buffering_flag	= img->adaptive_ref_pic_buffering_flag;
	dec_picture->dec_ref_pic_marking_buffer			= img->dec_ref_pic_marking_buffer;
	dec_picture->pic_num							= img->frame_num;
	dec_picture->frame_cropping_flag				= active_sps->frame_cropping_flag;
	img->dec_ref_pic_marking_buffer					= NULL;
	
	if (dec_picture->frame_cropping_flag)
	{
		dec_picture->frame_cropping_rect_left_offset   = active_sps->frame_cropping_rect_left_offset;
		dec_picture->frame_cropping_rect_right_offset  = active_sps->frame_cropping_rect_right_offset;
		dec_picture->frame_cropping_rect_top_offset    = active_sps->frame_cropping_rect_top_offset;
		dec_picture->frame_cropping_rect_bottom_offset = active_sps->frame_cropping_rect_bottom_offset;
	}
	
}


void padd_image(StorablePicture* dec_picture)
{
	int i;
	int width			= dec_picture->width;
	int height			= dec_picture->height;
	int width_cr		= dec_picture->width_cr;
	int height_cr		= dec_picture->height_cr;
	int stride_luma		= dec_picture->stride_luma;
	int stride_chroma	= dec_picture->stride_chroma;
	
	// Padding for plane Y
	for(i = 0; i < height; i++)
	{
		memset(	dec_picture->plane[0]+(i*stride_luma)-PADD_SIZE_LUMA, 
			   (dec_picture->plane[0]+(i*stride_luma))[0], PADD_SIZE_LUMA);
		memset(	dec_picture->plane[0]+width+(i*stride_luma), 
			   (dec_picture->plane[0]+width+(i*stride_luma))[-1],
				PADD_SIZE_LUMA);
	}

	for(i = 0; i < PADD_SIZE_LUMA; i++)
	{
		memcpy( dec_picture->imgY + (i*stride_luma), 
				dec_picture->imgY + (PADD_SIZE_LUMA*stride_luma), 
				stride_luma);
		memcpy( dec_picture->imgY + (height+PADD_SIZE_LUMA+i)*stride_luma,
				dec_picture->imgY + (height+PADD_SIZE_LUMA-1)*stride_luma,
				stride_luma);
	}

	// Padding for plane U
	for(i = 0; i < height_cr; i++)
	{
		memset(	dec_picture->plane[1]+(i*stride_chroma)-PADD_SIZE_CHROMA, 
			   (dec_picture->plane[1]+(i*stride_chroma))[0], 
			    PADD_SIZE_CHROMA);
		memset(	dec_picture->plane[1]+width_cr+(i*stride_chroma), 
			   (dec_picture->plane[1]+width_cr+(i*stride_chroma))[-1],
				PADD_SIZE_CHROMA);
	}

	for(i = 0; i < PADD_SIZE_CHROMA; i++)
	{
		memcpy( dec_picture->imgU + (i*stride_chroma),
				dec_picture->imgU + (PADD_SIZE_CHROMA*stride_chroma),
				stride_chroma);
		memcpy( dec_picture->imgU + (height_cr+PADD_SIZE_CHROMA+i)*stride_chroma,
				dec_picture->imgU + (height_cr+PADD_SIZE_CHROMA-1)*stride_chroma,
				stride_chroma);
	}

	// Padding for plane V
	for(i = 0; i < height_cr; i++)
	{
		memset(	dec_picture->plane[2]+(i*stride_chroma)-PADD_SIZE_CHROMA, 
			   (dec_picture->plane[2]+(i*stride_chroma))[0], 
			    PADD_SIZE_CHROMA);
		memset(	dec_picture->plane[2]+width_cr+(i*stride_chroma), 
			   (dec_picture->plane[2]+width_cr+(i*stride_chroma))[-1],
				PADD_SIZE_CHROMA);
	}

	for(i = 0; i < PADD_SIZE_CHROMA; i++)
	{
		memcpy( dec_picture->imgV + (i*stride_chroma),
				dec_picture->imgV + (PADD_SIZE_CHROMA*stride_chroma),
				stride_chroma);
		memcpy( dec_picture->imgV + (height_cr+PADD_SIZE_CHROMA+i)*stride_chroma,
				dec_picture->imgV + (height_cr+PADD_SIZE_CHROMA-1)*stride_chroma,
				stride_chroma);
	}

}

/********************************************************************************\
* FUNCTION     : exit_picture.                                                   *
*--------------------------------------------------------------------------------*
* DISCRIPTION  : finish decoding of a picture, conceal errors and store it into  *
*              : the DPB.                                                        *
*--------------------------------------------------------------------------------*
* ARGUMENTS    : Decoder parameters,                                             *
*              : Decoder Outputs.                                                *
*--------------------------------------------------------------------------------*
* RETURN VALUE : NONE.                                                           *
*--------------------------------------------------------------------------------*
* OUTPUT       : NONE.                                                           *
*--------------------------------------------------------------------------------*
* CHANGE LOG   : Below are the changes done to the function                      *
*--------------------------------------------------------------------------------*
* DATE         :                                                                 *
*--------------------------------------------------------------------------------*
*              : Function Argument List Changed [Removing Global Variables]      *
*              : Input parameters added are                                      *
*              : - h264_decoder* dec_params                                    *
*                                                                                *
*              : <saad.shams@inforient.com>                                      *
*--------------------------------------------------------------------------------*
* DATE         : 10-05-2006                                                      *
*--------------------------------------------------------------------------------*
*              : - Function modified for filling the padded area after the       *
*              :   deblock filter processing. Padded and filled frame is stored  *
*              :   in the dpb buffer and is used for reference in next frame.    *
*                                                                                *
*              : "Muhammad Tahir Awan" <tahir.awan@inforient.com>,               *
*              : "Umair Razzak" <umair.razzak@inforient.com>                     *
\********************************************************************************/
void exit_picture( h264_decoder* dec_params )
{
	//char yuv_types[4][6]= {"4:0:0","4:2:0","4:2:2","4:4:4"};
//	int ercStartMB;
//	int ercSegment;
//	frame recfr;
//	unsigned int i = 0, j, k, pel_a, pel_b, pel_c, pel_d, pel_e, pel_f, pel_g, pel_h;
	int structure, frame_poc, slice_type, refpic, qp, pic_num;
	
	//////////////////////
	float snr_value_not_calculated = 0;
	///////////////////////
	
	int tmp_time;                   // time used by decoding the last frame
	char yuvFormat[10];
	
//    imgpel * imgY = dec_params->dec_picture->imgY;        //!< Y picture component
//    imgpel * imgU = dec_params->dec_picture->imgU;    //!< U picture components
//    imgpel * imgV = dec_params->dec_picture->imgV;    //!< V picture components
//	unsigned int width     = dec_params->img->width;                 //!< width luma
//	unsigned int width_cr  = dec_params->img->width_cr;              //!< width chroma
//	unsigned int height    = dec_params->img->height;                //!< height luma
	
	// return if the last picture has already been finished
	if (dec_params->dec_picture==NULL)
	{
		return;
	}

   //deblocking for frame
	DeblockPicture( dec_params );
/*	
	if (dec_params->img->errorConcealmentFlag) 
	{
		
		recfr.yptr = imgY;
		recfr.uptr = imgU;
		recfr.vptr = imgV;
		
		//! this is always true at the beginning of a picture
		ercStartMB = 0;
		ercSegment = 0;
		
		//! mark the start of the first segment
		ercStartSegment(0, ercSegment, 0 , dec_params->erc_errorVar);
		//! generate the segments according to the macroblock map
        
		for(i = 1; i<dec_params->img->FrameSizeInMbs; i++)
		{
			// faisal changes er
			if(dec_params->img->ei_flag[i] != dec_params->img->ei_flag[i-1])
			{
				ercStopSegment(i-1, ercSegment, 0, dec_params->erc_errorVar); //! stop current segment
				
				//! mark current segment as lost or OK
				if(dec_params->img->ei_flag[i-1])
					ercMarkCurrSegmentLost(width, dec_params->erc_errorVar);
				else
					ercMarkCurrSegmentOK(width, dec_params->erc_errorVar);
				
				ercSegment++;  //! next segment
				ercStartSegment(i, ercSegment, 0 , dec_params->erc_errorVar); //! start new segment
				ercStartMB = i;//! save start MB for this segment 
			}
		}
		//! mark end of the last segment
		ercStopSegment(dec_params->img->FrameSizeInMbs-1, ercSegment, 0, dec_params->erc_errorVar);
		
		if(dec_params->img->ei_flag[i-1])
			ercMarkCurrSegmentLost(width, dec_params->erc_errorVar);
		else
			ercMarkCurrSegmentOK(width, dec_params->erc_errorVar);
		
		//! call the right error concealment function depending on the frame type.
		dec_params->erc_mvperMB /= dec_params->img->FrameSizeInMbs;
		dec_params->erc_img = dec_params->img;
		if(dec_params->dec_picture->slice_type == I_SLICE) // I-frame
			ercConcealIntraFrame(&recfr, width, height, dec_params->erc_errorVar,dec_params);
		else
			ercConcealInterFrame(&recfr, dec_params->erc_object_list, width, height, dec_params->erc_errorVar,dec_params);
	}
*/    
	structure  = dec_params->dec_picture->structure;
	slice_type = dec_params->dec_picture->slice_type;
	frame_poc  = dec_params->dec_picture->frame_poc;
	refpic     = dec_params->dec_picture->used_for_reference;
	qp         = dec_params->img->qp;
	pic_num    = dec_params->dec_picture->pic_num;

	padd_image(dec_params->dec_picture);

	store_picture_in_dpb_baseline(dec_params);
	
	dec_params->frame_no += 1;
	
	dec_params->dec_picture=NULL;
	
	if (dec_params->img->last_has_mmco_5)
	{
		dec_params->img->pre_frame_num = 0;
	}
	
/*#ifdef WIN32
	_ftime (&(dec_params->img->tstruct_end));             // start time ms
#else
	//ftime (&(dec_params->img->tstruct_end));              // start time ms
#endif*/
	
/*	time( &(dec_params->img->ltime_end));                // start time s
	
	tmp_time=(dec_params->img->ltime_end*1000+dec_params->img->tstruct_end.millitm) - (dec_params->img->ltime_start*1000+dec_params->img->tstruct_start.millitm);
	dec_params->tot_time=dec_params->tot_time + tmp_time;*/
/////*	
	sprintf(yuvFormat,"%s", "4:2:0");
	if(slice_type == I_SLICE){ // I picture
		fprintf(stdout,"Frame No. %03d , Frame Type (I)\n",dec_params->frame_no);
			
	}
	else if(slice_type == P_SLICE){ // P pictures
		fprintf(stdout,"Frame No. %03d , Frame Type (P)\n",dec_params->frame_no);
	}	
	else // B pictures
		fprintf(stdout,"Frame No. %03d , Frame Type (B)\n",dec_params->frame_no);
	
	fflush(stdout);
/////*/	
	if(slice_type == I_SLICE || slice_type == P_SLICE || refpic)   // I or P pictures
	{
		dec_params->img->number++;
	}
	else 
	{
		dec_params->Bframe_ctr++;    // B pictures
		dec_params->img->number++;
	}
	
	dec_params->img->current_mb_nr = -4712;   // impossible value for debugging, StW
	dec_params->img->current_slice_nr = 0;
	
	// increase the picture_offset so the pointer points to next dpb buffer 
	// check the is_empty flag and select the next dpb buffer pointer to be used for the reconstruction of the frame
	dec_params->picture_offset+= 1;
	if(dec_params->picture_offset >= (dec_params->dpb.size + 1) )
	{
		dec_params->picture_offset = 0;
	}
	
	while (dec_params->dec_pictures_list[dec_params->picture_offset]->is_empty != 1)
	{
		dec_params->picture_offset+= 1;
		if(dec_params->picture_offset >= (dec_params->dpb.size + 1) )
		{
			dec_params->picture_offset = 0;
		}
	}
}

/*!
************************************************************************
* \brief
*    write the encoding mode and motion vectors of current 
*    MB to the buffer of the error concealment module.
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*-----------------------------------------------------------------------
*		Changes
*			- replaced operator (*) by shifting operations
*
*								<saad.shams@inforient.com>
/***********************************************************************
*	 Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/
///??? how this function is doing error concealment
void ercWriteMBMODEandMV_baseline( h264_decoder* dec_params )
{
	int i, ii, jj, currMBNum = dec_params->img->current_mb_nr;
	int mbx = xPosMB(currMBNum,dec_params->img->width), mby = yPosMB(currMBNum,dec_params->img->width);
	objectBuffer_t *currRegion, *pRegion;
	Macroblock *currMB = &dec_params->img->mb_data[currMBNum];
	short*  mv;
	unsigned int mvWidth = dec_params->img->width>>1;
	
	// faisal ref_idx
	char *curr_ref_idxl0 = dec_params->dec_picture->ref_idx_l0;
	char *curr_ref_idxl1 = dec_params->dec_picture->ref_idx_l1;
	int stride_add;

	currRegion = dec_params->erc_object_list + (currMBNum<<2);
	
	
	if(dec_params->img->type != B_SLICE) //non-B frame
	{
		for (i=0; i<4; i++)
		{
			pRegion             = currRegion + i;
			pRegion->regionMode = (currMB->mb_type  ==I16MB  ? REGMODE_INTRA      :
			currMB->b8mode[i]==IBLOCK ? REGMODE_INTRA_8x8  :
			currMB->b8mode[i]==0      ? REGMODE_INTER_COPY :
			currMB->b8mode[i]==1      ? REGMODE_INTER_PRED : REGMODE_INTER_PRED_8x8);
			if (currMB->b8mode[i]==0 || currMB->b8mode[i]==IBLOCK)  // INTRA OR COPY
			{
				pRegion->mv[0]    = 0;
				pRegion->mv[1]    = 0;
				pRegion->mv[2]    = 0;
			}
			else
			{
				// replaced operator (*) by shifting operations, saad
				ii              = (mbx<<2) + ((i&1)<<1);// + BLOCK_SIZE;
				jj              = 4*mby+(i/2)*2;//(mby<<2) + ((i>>2)<<1);
				if (currMB->b8mode[i]>=5 && currMB->b8mode[i]<=7)  // SMALL BLOCKS
				{
					//replaced operator (/) by shifting operations, saad	
					pRegion->mv[0]  = (*(dec_params->dec_picture->mvL0+(jj*mvWidth)+(ii<<1)) + *(dec_params->dec_picture->mvL0+(jj*mvWidth)+((ii+1)<<1)) + *(dec_params->dec_picture->mvL0+((jj+1)*mvWidth)+(ii<<1)) + *(dec_params->dec_picture->mvL0+((jj+1)*mvWidth)+((ii+1)<<1)) + 2)>>2;
					pRegion->mv[1]  = (*(dec_params->dec_picture->mvL0+(jj*mvWidth)+(ii<<1)+1) + *(dec_params->dec_picture->mvL0+(jj*mvWidth)+((ii+1)<<1)+1) + *(dec_params->dec_picture->mvL0+((jj+1)*mvWidth)+(ii<<1)+1) + *(dec_params->dec_picture->mvL0+((jj+1)*mvWidth)+((ii+1)<<1)+1) + 2)>>2;
				}
				else // 16x16, 16x8, 8x16, 8x8
				{
					pRegion->mv[0]  = *(dec_params->dec_picture->mvL0+(jj*mvWidth)+(ii<<1));
					pRegion->mv[1]  = *(dec_params->dec_picture->mvL0+(jj*mvWidth)+(ii<<1)+1);
				}
				dec_params->erc_mvperMB      += absz(pRegion->mv[0]) + absz(pRegion->mv[1]);

				stride_add = ((jj%4) * 4) + (ii%4);
				pRegion->mv[2]    = *(curr_ref_idxl0 + (currMBNum*16)+ stride_add);
			}
		}
	}
	else
	{
		for (i=0; i<4; i++)
		{
			// replaced operator (*) by shifting operations, saad	
			ii                  = (mbx<<2) + ((i%2)<<1);// + BLOCK_SIZE;
			jj                  = (mby<<2) + ((i>>1)<<1);
			pRegion             = currRegion + i;
			pRegion->regionMode = (currMB->mb_type  ==I16MB  ? REGMODE_INTRA      :
			currMB->b8mode[i]==IBLOCK ? REGMODE_INTRA_8x8  : REGMODE_INTER_PRED_8x8);
			if (currMB->mb_type==I16MB || currMB->b8mode[i]==IBLOCK)  // INTRA
			{
				pRegion->mv[0]    = 0;
				pRegion->mv[1]    = 0;
				pRegion->mv[2]    = 0;
			}
			else
			{
				int idx;

				stride_add = ((jj%4) * 4) + (ii%4);
				
				idx = (*(curr_ref_idxl0 + (currMBNum*16)+ stride_add)<0)?1:0;

				mv = (*(curr_ref_idxl0 + (currMBNum*16)+ stride_add)<0)?dec_params->dec_picture->mvL1:dec_params->dec_picture->mvL0;
				pRegion->mv[0]    = (*(mv+jj*mvWidth+(ii<1)) + *(mv+jj*mvWidth+((ii+1)<1)) + *(mv+(jj+1)*mvWidth+(ii<1)) + *(mv+(jj+1)*mvWidth+((ii+1)<1)) + 2)/4;
				pRegion->mv[1]    = (*(mv+jj*mvWidth+(ii<1)+1) + *(mv+jj*mvWidth+((ii+1)<1)+1) + *(mv+(jj+1)*mvWidth+(ii<1)+1) + *(mv+(jj+1)*mvWidth+((ii+1)<1)+1) + 2)/4;
//				pRegion->mv[0]    = (mv[jj][ii][0] + mv[jj][ii+1][0] + mv[jj+1][ii][0] + mv[jj+1][ii+1][0] + 2)>>2;
//				pRegion->mv[1]    = (mv[jj][ii][1] + mv[jj][ii+1][1] + mv[jj+1][ii][1] + mv[jj+1][ii+1][1] + 2)>>2;
				dec_params->erc_mvperMB      += mabs(pRegion->mv[0]) + mabs(pRegion->mv[1]);
				
				if(idx)
				{
					pRegion->mv[2] = *(curr_ref_idxl1 + (currMBNum*16)+ stride_add);
				}
				else
				{
					pRegion->mv[2] = *(curr_ref_idxl0 + (currMBNum*16)+ stride_add);

				}
			}
		}
	}
}


/*!
 ************************************************************************
 * \brief
 *    set defaults for old_slice
 *    NAL unit of a picture"
*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
  ************************************************************************
 */
void init_old_slice(  h264_decoder* dec_params)
{
  dec_params->old_slice.pps_id = INT_MAX;

  dec_params->old_slice.frame_num = INT_MAX;

  dec_params->old_slice.nal_ref_idc = INT_MAX;
  
  dec_params->old_slice.idr_flag = 0;

  dec_params->old_slice.pic_oder_cnt_lsb          = UINT_MAX;
  dec_params->old_slice.delta_pic_oder_cnt_bottom = INT_MAX;

  dec_params->old_slice.delta_pic_order_cnt[0] = INT_MAX;
  dec_params->old_slice.delta_pic_order_cnt[1] = INT_MAX;

}

/*!
************************************************************************
* \brief
*    save slice parameters that are needed for checking of "first VCL
*    NAL unit of a picture"
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
/***********************************************************************
*	 Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/
void exit_slice_baseline(  h264_decoder* dec_params)
{

  dec_params->old_slice.pps_id = dec_params->img->currentSlice->pic_parameter_set_id;

  dec_params->old_slice.frame_num = dec_params->img->frame_num;

  dec_params->old_slice.nal_ref_idc   = dec_params->img->nal_reference_idc;
  
  dec_params->old_slice.idr_flag = dec_params->img->idr_flag;
  if (dec_params->img->idr_flag)
  {
    dec_params->old_slice.idr_pic_id = dec_params->img->idr_pic_id;
  }

  if (dec_params->active_sps->pic_order_cnt_type == 0)
  {
    dec_params->old_slice.pic_oder_cnt_lsb          = dec_params->img->pic_order_cnt_lsb;
    dec_params->old_slice.delta_pic_oder_cnt_bottom = dec_params->img->delta_pic_order_cnt_bottom;
  }

  if (dec_params->active_sps->pic_order_cnt_type == 1)
  {
    dec_params->old_slice.delta_pic_order_cnt[0] = dec_params->img->delta_pic_order_cnt[0];
    dec_params->old_slice.delta_pic_order_cnt[1] = dec_params->img->delta_pic_order_cnt[1];
  }
}


/*!
 ************************************************************************
 * \brief
 *    detect if current slice is "first VCL NAL unit of a picture"
 *-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************
 */
int is_new_picture(  h264_decoder* dec_params)
{
  int result=0;
  result |= (dec_params->old_slice.pps_id != dec_params->img->currentSlice->pic_parameter_set_id);
  result |= (dec_params->old_slice.frame_num != dec_params->img->frame_num);
  result |= (dec_params->old_slice.nal_ref_idc   != dec_params->img->nal_reference_idc);
  result |= ( dec_params->old_slice.idr_flag != dec_params->img->idr_flag);
  if (dec_params->img->idr_flag && dec_params->old_slice.idr_flag)
  {
    result |= (dec_params->old_slice.idr_pic_id != dec_params->img->idr_pic_id);
  }
  if (dec_params->active_sps->pic_order_cnt_type == 0)
  {
    result |=  (dec_params->old_slice.pic_oder_cnt_lsb          != dec_params->img->pic_order_cnt_lsb);
    result |=  (dec_params->old_slice.delta_pic_oder_cnt_bottom != dec_params->img->delta_pic_order_cnt_bottom);
  }
  if (dec_params->active_sps->pic_order_cnt_type == 1)
  {
    result |= (dec_params->old_slice.delta_pic_order_cnt[0] != dec_params->img->delta_pic_order_cnt[0]);
    result |= (dec_params->old_slice.delta_pic_order_cnt[1] != dec_params->img->delta_pic_order_cnt[1]);
  }
  return result;
}

void reset_wp_params(ImageParameters *img)
{
  int i,comp;
  int log_weight_denom;

  for (i=0; i<MAX_REFERENCE_PICTURES; i++)
  {
    for (comp=0; comp<3; comp++)
    {
      log_weight_denom = (comp == 0) ? img->luma_log2_weight_denom : img->chroma_log2_weight_denom;
      img->wp_weight[0][i][comp] = 1<<log_weight_denom;
      img->wp_weight[1][i][comp] = 1<<log_weight_denom;
    }
  }
}

 
 /*************************************************************************			
 *	 Function Argument List Changed [Removing Global Variables] 
 *	 Input parameters added are 
 *			- h264_decoder* dec_params
 *
 *		<saad.shams@inforient.com>
 ************************************************************************/
void fill_wp_params(ImageParameters *img,  h264_decoder* dec_params)
{
  int i, j;//, k;
  int comp;
  int log_weight_denom;
  int tb, td;
  int bframe = (img->type==B_SLICE);
  int max_bwd_ref, max_fwd_ref;
  int tx,DistScaleFactor;

  max_fwd_ref = img->num_ref_idx_l0_active;
  max_bwd_ref = img->num_ref_idx_l1_active;

   if (dec_params->active_pps->weighted_bipred_idc == 2 && bframe)
  {
    img->luma_log2_weight_denom = 5;
    img->chroma_log2_weight_denom = 5;
    img->wp_round_luma = 16;
    img->wp_round_chroma = 16;

    for (i=0; i<MAX_REFERENCE_PICTURES; i++)
    {
      for (comp=0; comp<3; comp++)
      {
        log_weight_denom = (comp == 0) ? img->luma_log2_weight_denom : img->chroma_log2_weight_denom;
        img->wp_weight[0][i][comp] = 1<<log_weight_denom;
        img->wp_weight[1][i][comp] = 1<<log_weight_denom;
        img->wp_offset[0][i][comp] = 0;
        img->wp_offset[1][i][comp] = 0;
      }
    }
  }

  if (bframe)
  {
    for (i=0; i<max_fwd_ref; i++)
    {
      for (j=0; j<max_bwd_ref; j++)
      {
        for (comp = 0; comp<3; comp++)
        {
          log_weight_denom = (comp == 0) ? img->luma_log2_weight_denom : img->chroma_log2_weight_denom;
          if (dec_params->active_pps->weighted_bipred_idc == 1)
          {
            img->wbp_weight[0][i][j][comp] =  img->wp_weight[0][i][comp];
           img->wbp_weight[1][i][j][comp] =  img->wp_weight[1][j][comp];
          }
          else if (dec_params->active_pps->weighted_bipred_idc == 2)
          {

            td = Clip3(-128,127,dec_params->listX[LIST_1][j]->poc - dec_params->listX[LIST_0][i]->poc);
            if (td == 0 || dec_params->listX[LIST_1][j]->is_long_term || dec_params->listX[LIST_0][i]->is_long_term)
            {
               img->wbp_weight[0][i][j][comp] =   32;
              img->wbp_weight[1][i][j][comp] =   32;
			  
            }
            else
            {
              tb = Clip3(-128,127,img->ThisPOC - dec_params->listX[LIST_0][i]->poc);

              tx = (16384 + absz(td/2))/td;
              DistScaleFactor = Clip3(-1024, 1023, (tx*tb + 32 )>>6);
              
              img->wbp_weight[1][i][j][comp] = DistScaleFactor >> 2;
              img->wbp_weight[0][i][j][comp] = 64 - img->wbp_weight[1][i][j][comp];
              if (img->wbp_weight[1][i][j][comp] < -64 || img->wbp_weight[1][i][j][comp] > 128)
              {
                img->wbp_weight[0][i][j][comp] = 32;
                img->wbp_weight[1][i][j][comp] = 32;
                img->wp_offset[0][i][comp] = 0;
                img->wp_offset[1][i][comp] = 0;
              }
			  else
			  {
//				  img->apply_weights_bi = 0;
			  }
            }
          }
        }
     }
   }
 }
}


