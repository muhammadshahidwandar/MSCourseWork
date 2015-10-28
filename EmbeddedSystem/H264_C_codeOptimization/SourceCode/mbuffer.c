
/*!
***********************************************************************
*  \file
*      mbuffer.c
*
*  \brief
*      Frame buffer functions
*
*  \author
*      Main contributors (see contributors.h for copyright, address and affiliation details)
*      - Karsten Sühring                 <suehring@hhi.de>
*      - Alexis Tourapis                 <alexismt@ieee.org>
***********************************************************************
*/

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include "global.h"
#include "mbuffer.h"
#include "memalloc.h"
#include "output.h"
#include "image.h"
#include "header.h"

static void insert_picture_in_dpb_baseline(FrameStore* fs, h264_decoder* dec_params);

static void output_one_frame_from_dpb(  h264_decoder* dec_params);
static int  is_used_for_reference(FrameStore* fs);
static void get_smallest_poc(int *poc,int * pos,  h264_decoder* dec_params);
static int  remove_unused_frame_from_dpb(   h264_decoder* dec_params);
static int  is_short_term_reference(FrameStore* fs);
static int  is_long_term_reference(FrameStore* fs);
void gen_field_ref_ids(StorablePicture *p);


#define MAX_LIST_SIZE 33

/*!
************************************************************************
* \brief
*    Returns the size of the dec_params->dpb depending on level and picture size
*
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
int getDpbSize( h264_decoder* dec_params )
{
	int pic_size = (dec_params->active_sps->pic_width_in_mbs_minus1 + 1) * 
	             (dec_params->active_sps->pic_height_in_map_units_minus1 + 1) * 
				 (dec_params->active_sps->frame_mbs_only_flag?1:2) * 384;
	
	int size = 0;
	
	switch (dec_params->active_sps->level_idc)
	{
	case 10:
		size = 152064;
		break;
	case 11:
		size = 345600;
		break;
	case 12:
		size = 912384;
		break;
	case 13:
		size = 912384;
		break;
	case 20:
		size = 912384;
		break;
	case 21:
		size = 1824768;
		break;
	case 22:
		size = 3110400;
		break;
	case 30:
		size = 3110400;
		break;
	case 31:
		size = 6912000;
		break;
	case 32:
		size = 7864320;
		break;
	case 40:
		size = 12582912;
		break;
	case 41:
		size = 12582912;
		break;
	case 42:
		if(  (dec_params->active_sps->profile_idc==FREXT_HP   ) || (dec_params->active_sps->profile_idc==FREXT_Hi10P)
			|| (dec_params->active_sps->profile_idc==FREXT_Hi422) || (dec_params->active_sps->profile_idc==FREXT_Hi444))
			size = 13369344;
		else
			size = 12582912;
		break; 
	case 50:
		size = 42393600;
		break;
	case 51:
		size = 70778880;
		break;
	default:
		printf("undefined level");
		exit(0);
		break;
	}
	
	size /= pic_size;
	size = min( size, 16);
	
	if (dec_params->active_sps->vui_parameters_present_flag && dec_params->active_sps->vui_seq_parameters.bitstream_restriction_flag)
	{
		if ((int)dec_params->active_sps->vui_seq_parameters.max_dec_frame_buffering > size)
		{
			printf("max_dec_frame_buffering larger than MaxDpbSize");
			exit(0);
		}
		size = max (1, dec_params->active_sps->vui_seq_parameters.max_dec_frame_buffering);
	}
	
	return size;
}

/*!
************************************************************************
* \brief
*    Check then number of frames marked "used for reference" and break 
*    if maximum is exceeded
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void check_num_ref(  h264_decoder* dec_params )
{
	if ((int)(dec_params->dpb.ltref_frames_in_buffer +  dec_params->dpb.ref_frames_in_buffer ) > (max(1,dec_params->dpb.num_ref_frames)))
	{
		printf("Max. number of reference frames exceeded. Invalid stream.");
		exit(0);
	}
}


/*!
************************************************************************
* \brief
*    Allocate memory for decoded picture buffer and initialize with sane values.
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void init_dpb(  h264_decoder* dec_params )
{
	unsigned i,j;
	ImageParameters *img=dec_params->img; 
	
	if (dec_params->dpb.init_done)
	{
		free_dpb(dec_params);
	}
	
	dec_params->dpb.size = getDpbSize(dec_params);
	
	dec_params->dpb.num_ref_frames = dec_params->active_sps->num_ref_frames;
	
	if (dec_params->dpb.size < dec_params->active_sps->num_ref_frames)
	{
		printf("DPB size at specified level is smaller than the specified number of reference frames. This is not allowed.\n");
		exit(0);
	}
	
	dec_params->dpb.used_size = 0;
	dec_params->dpb.last_picture = NULL;
	
	dec_params->dpb.ref_frames_in_buffer = 0;
	dec_params->dpb.ltref_frames_in_buffer = 0;
	
	dec_params->dpb.fs = (FrameStore **)h264_malloc(dec_params->dpb.size * sizeof (FrameStore*));
	if (NULL==dec_params->dpb.fs) 
	{
		printf("init_dpb: dpb->fs");
		exit(0);
	}
	
	dec_params->dpb.fs_ref = (FrameStore **)h264_malloc(dec_params->dpb.size * sizeof (FrameStore*));
	if (NULL==dec_params->dpb.fs_ref) 
	{
		printf("init_dpb: dpb->fs_ref");
		exit(0);
	}
	
	dec_params->dpb.fs_ltref = (FrameStore **)h264_malloc(dec_params->dpb.size * sizeof (FrameStore*));
	if (NULL==dec_params->dpb.fs_ltref) 
	{
		printf("init_dpb: dpb->fs_ltref");
		exit(0);
	}
	
	for (i=0; i<dec_params->dpb.size; i++)
	{
		dec_params->dpb.fs[i]       = alloc_frame_store( dec_params );
		dec_params->dpb.fs_ref[i]   = NULL;
		dec_params->dpb.fs_ltref[i] = NULL;
	}
	
	//////////////
	for (i=0; i<6; i++)
	{
		dec_params->listX[i] = (StorablePicture **)h264_malloc(MAX_LIST_SIZE * sizeof (StorablePicture*)); // +1 for reordering
		if (NULL==dec_params->listX[i]) 
		{
			printf("init_dpb: listX[i]");
			exit(0);
		}
	}
	
	for (j=0;j<6;j++)
	{
		for (i=0; i<MAX_LIST_SIZE; i++)
		{
			dec_params->listX[j][i] = NULL;
		}
		dec_params->listXsize[j]=0;
	}
	//////////////
	
	// ALLOCAT REFERENCE FRAMES IN DPB BUFFER 
	dec_params->dec_pictures_list = (StorablePicture **)h264_malloc((dec_params->dpb.size+1) * sizeof (StorablePicture*));
	
	for (i=0; i<(dec_params->dpb.size+1); i++)
	{
		dec_params->dec_pictures_list[i] = alloc_storable_picture (FRAME, img->width, img->height, 
			img->width_cr, img->height_cr, dec_params);
		dec_params->dec_pictures_list[i]->is_empty = 1;
	}
	dec_params->picture_offset = 0;
	
	dec_params->dpb.last_output_poc = INT_MIN;
	dec_params->img->last_has_mmco_5 = 0;
	dec_params->dpb.init_done = 1;
}

void free_dpb( h264_decoder* dec_params )
{
	unsigned i;
	if (dec_params->dpb.fs)
	{
		for (i=0; i<dec_params->dpb.size; i++)
		{
			free_frame_store(dec_params->dpb.fs[i]);
		}
		h264_free (dec_params->dpb.fs);
		dec_params->dpb.fs=NULL;
	}
	if (dec_params->dpb.fs_ref)
	{
		h264_free (dec_params->dpb.fs_ref);
	}
	if (dec_params->dpb.fs_ltref)
	{
		h264_free (dec_params->dpb.fs_ltref);
	}
	dec_params->dpb.last_output_poc = INT_MIN;
	
    for (i=0; i<6; i++)
    {
        if (dec_params->listX[i])
        {
            h264_free (dec_params->listX[i]);
            dec_params->listX[i] = NULL;
        }
    }
    for (i=0; i<(dec_params->dpb.size+1); i++)
    {
        if(dec_params->dec_pictures_list[i])
        {
            free_storable_picture (dec_params->dec_pictures_list[i]);
            dec_params->dec_pictures_list[i] = NULL;
        }
    }
    if(dec_params->dec_pictures_list !=  NULL)
    {
        h264_free (dec_params->dec_pictures_list);
        dec_params->dec_pictures_list =  NULL;
    }
		dec_params->dpb.init_done = 0;
}


/*!
************************************************************************
* \brief
*    Allocate memory for decoded picture buffer frame stores an initialize with sane values.
*
* \return
*    the allocated FrameStore structure
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
FrameStore* alloc_frame_store( h264_decoder* dec_params )
{
	FrameStore *f;
	
	f = (FrameStore *)h264_malloc (1 * sizeof(FrameStore));
	
	if (NULL==f) 
	{
		printf("alloc_frame_store: f");
		exit(0);
	}
	
	f->is_used      = 0;
	f->is_reference = 0;
	f->is_long_term = 0;
	f->is_orig_reference = 0;
	
	f->is_output = 0;
	
	f->frame        = NULL;;
	
	return f;
}

/*!
************************************************************************
* \brief
*    Allocate memory for a stored picture. 
*
* \param structure
*    picture structure
* \param size_x
*    horizontal luma size
* \param size_y
*    vertical luma size
* \param size_x_cr
*    horizontal chroma size
* \param size_y_cr
*    vertical chroma size
*
* \return
*    the allocated StorablePicture structure
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
StorablePicture* alloc_storable_picture( PictureStructure structure, int size_x, int size_y, 
										 int size_x_cr, int size_y_cr,  h264_decoder* dec_params )
{
	StorablePicture *s;
	int memSize = 0;
	
	//printf ("Allocating (%s) picture (x=%d, y=%d, x_cr=%d, y_cr=%d)\n", (type == FRAME)?"FRAME":(type == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD", size_x, size_y, size_x_cr, size_y_cr);
	
	s = (StorablePicture *)h264_malloc (1 * sizeof(StorablePicture));
	if (NULL==s) 
	{
		printf("alloc_storable_picture: s");
		exit(0);
	}
	memset (s,0,sizeof(StorablePicture));
	
	dec_params->img->FrameSizeInMbs = (size_x * size_y)>>8;
	s->imgU = NULL;
    s->imgV = NULL;

	// 1-d version
	if (( s->imgY = (imgpel* ) h264_malloc( (size_y+64) * (size_x+64) * sizeof(imgpel ))) == NULL)
    {
        printf("get_mem_Luma: array ");
		exit(0);
    }
    memSize += (size_y+64) * (size_x+64) * sizeof(imgpel);
        
    if (( s->imgU = (imgpel* ) h264_malloc( (size_y_cr+32) * (size_x_cr+32) * sizeof(imgpel ))) == NULL)
    {
        printf("get_mem_chromaU: array ");
		exit(0);
    }
    memSize += (size_y_cr+32) * (size_x_cr+32) * sizeof(imgpel);
    
    if (( s->imgV = (imgpel* ) h264_malloc( (size_y_cr+32) * (size_x_cr+32) * sizeof(imgpel ))) == NULL)
    {
        printf("get_mem_chromaU: array ");
		exit(0);
    }
    memSize += (size_y_cr+32) * (size_x_cr+32) * sizeof(imgpel);
	/*
	s->imgU = s->imgY + (size_y+64) * (size_x+64);
	s->imgV = s->imgU + (size_y_cr+32) * (size_x_cr+32);
	*/
	s->width		= size_x;
	s->height		= size_y;
	s->width_cr		= size_x_cr;
	s->height_cr	= size_y_cr;
	
	s->stride_luma = size_x + 64;
	s->stride_chroma = size_x_cr + 32;
	
	s->plane[0] = s->imgY + (32*s->stride_luma) +32;
	s->plane[1] = s->imgU + (16*s->stride_chroma) +16;
	s->plane[2] = s->imgV + (16*s->stride_chroma) +16;

	s->slice_id = (short*) h264_calloc( ((size_y*size_x)>>8) , sizeof(short));
	
	memSize+= get_mem3D      ((byte****)(&(s->ref_idx))   , 2, size_y / BLOCK_SIZE, size_x / BLOCK_SIZE);
	// changes by faisal for linear data structures
	if( (s->ref_idx_l0 = (signed char*) h264_malloc(size_y / BLOCK_SIZE * size_x / BLOCK_SIZE * sizeof(byte*))) == NULL)
	{
		printf("Colocated ref_idx_l0");
		exit(0);
	}

	if( (s->ref_idx_l1 = (signed char*) h264_malloc(size_y / BLOCK_SIZE * size_x / BLOCK_SIZE * sizeof(byte*))) == NULL)
	{
		printf("Colocated ref_idx_l1");
		exit(0);
	}

	memSize+= get_mem3Dint (&(s->ref_id)    , 6, size_y / BLOCK_SIZE, size_x / BLOCK_SIZE);
	
	if( (s->ref_idL0 = (short*) h264_malloc( ((size_x*size_y) >> 4)*sizeof(short))) == NULL )
	{
		printf("ref_idL0: array ");
		exit(0);
	}
	
	if( (s->ref_idL1 = (short*) h264_malloc( ((size_x*size_y) >> 4)*sizeof(short))) == NULL )
	{
		printf("ref_idL1: array ");
		exit(0);
	}
	
	s->mvL0 = (short *)h264_malloc ((size_y / BLOCK_SIZE)*(size_x / BLOCK_SIZE)*2*sizeof(short));

	memSize += (size_y / BLOCK_SIZE)*(size_x / BLOCK_SIZE)*2*sizeof(short);

	s->mvL1 = (short *)h264_malloc ((size_y / BLOCK_SIZE)*(size_x / BLOCK_SIZE)*2*sizeof(short));

	memSize += (size_y / BLOCK_SIZE)*(size_x / BLOCK_SIZE)*2*sizeof(short);

	s->pic_num=0;
	s->long_term_frame_idx=0;
	s->long_term_pic_num=0;
	s->used_for_reference=0;
	s->is_long_term=0;
	s->non_existing=0;
	s->is_output = 0;
	s->max_slice_id = 0;
	
	s->structure=structure;
	
	//s->size_x = size_x;
	//s->size_y = size_y;
	//s->size_x_cr = size_x_cr;
	//s->size_y_cr = size_y_cr;
	
	//  s->frame        = NULL;
	
	s->dec_ref_pic_marking_buffer = NULL;
	
	//s->coded_frame                   = 0;
	return s;
}

/*!
************************************************************************
* \brief
*    Free frame store memory.
*
* \param f
*    FrameStore to be freed
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void free_frame_store( FrameStore* f )
{
	if (f)
	{
		if (f->frame)
		{
			f->frame=NULL;
		}
		h264_free(f);
	}
}

/*!
************************************************************************
* \brief
*    Free picture memory.
*
* \param p
*    Picture to be freed
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void free_storable_picture( StorablePicture* p )
{
	if (p)
	{
		if(p->ref_idx_l0)
		{
			//faisal
			h264_free(p->ref_idx_l0);
		}
		if(p->ref_idx_l1)
		{
			//faisal
			h264_free(p->ref_idx_l1);
		}

		if (p->ref_idx)
		{
			free_mem3D ((byte***)p->ref_idx, 2 );
			p->ref_idx = NULL;
		}
		
		if (p->ref_id)
		{
			free_mem3Dint (p->ref_id, 6);
			p->ref_id = NULL;
		}

		if(p->ref_idL0)
		{
			h264_free(p->ref_idL0);
			p->ref_idL0 = NULL;
		}

		if(p->ref_idL1)
		{
			h264_free(p->ref_idL1);
			p->ref_idL1 = NULL;
		}

		if (p->mvL0)
		{
			h264_free (p->mvL0);
			p->mvL0 = NULL;
		}

		if (p->mvL1)
		{
			h264_free (p->mvL1);
			p->mvL1 = NULL;
		}

        if (p->imgY)
        {
			h264_free(p->imgY);
            p->imgY=NULL;
        }

        if (p->imgU)
        {
            h264_free(p->imgU);
            p->imgU=NULL;
        }
        if (p->imgV)
        {
            h264_free(p->imgV);
            p->imgV=NULL;
        }
		
		if (p->slice_id)
		{
			h264_free(p->slice_id);
			p->slice_id = NULL;
		}

		h264_free(p);
		p = NULL;
	}
}

/*!
************************************************************************
* \brief
*    mark FrameStore unused for reference
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void unmark_for_reference( FrameStore* fs )
{
	if (fs->is_used == 3)
	{
		fs->frame->used_for_reference = 0;
	}
	fs->is_reference = 0;
}


/*!
************************************************************************
* \brief
*    mark FrameStore unused for reference and reset long term flags
*
************************************************************************
*/
static void unmark_for_long_term_reference(FrameStore* fs)
{
	if (fs->is_used == 3)
	{
		fs->frame->used_for_reference = 0;
		fs->frame->is_long_term = 0;
	}
	fs->is_reference = 0;
	fs->is_long_term = 0;
}


/*!
************************************************************************
* \brief
*    compares two stored pictures by picture number for qsort in descending order
*
************************************************************************
*/
static int compare_pic_by_pic_num_desc( const void *arg1, const void *arg2 )
{
	if ( (*(StorablePicture**)arg1)->pic_num < (*(StorablePicture**)arg2)->pic_num)
		return 1;
	if ( (*(StorablePicture**)arg1)->pic_num > (*(StorablePicture**)arg2)->pic_num)
		return -1;
	else
		return 0;
}

/*!
************************************************************************
* \brief
*    compares two stored pictures by picture number for qsort in descending order
*
************************************************************************
*/
static int compare_pic_by_lt_pic_num_asc( const void *arg1, const void *arg2 )
{
	if ( (*(StorablePicture**)arg1)->long_term_pic_num < (*(StorablePicture**)arg2)->long_term_pic_num)
		return -1;
	if ( (*(StorablePicture**)arg1)->long_term_pic_num > (*(StorablePicture**)arg2)->long_term_pic_num)
		return 1;
	else
		return 0;
}

/*!
************************************************************************
* \brief
*    compares two frame stores by pic_num for qsort in descending order
*
************************************************************************
*/
static int compare_fs_by_frame_num_desc( const void *arg1, const void *arg2 )
{
	if ( (*(FrameStore**)arg1)->frame_num_wrap < (*(FrameStore**)arg2)->frame_num_wrap)
		return 1;
	if ( (*(FrameStore**)arg1)->frame_num_wrap > (*(FrameStore**)arg2)->frame_num_wrap)
		return -1;
	else
		return 0;
}


/*!
************************************************************************
* \brief
*    compares two frame stores by lt_pic_num for qsort in descending order
*
************************************************************************
*/
static int compare_fs_by_lt_pic_idx_asc( const void *arg1, const void *arg2 )
{
	if ( (*(FrameStore**)arg1)->long_term_frame_idx < (*(FrameStore**)arg2)->long_term_frame_idx)
		return -1;
	if ( (*(FrameStore**)arg1)->long_term_frame_idx > (*(FrameStore**)arg2)->long_term_frame_idx)
		return 1;
	else
		return 0;
}


/*!
************************************************************************
* \brief
*    compares two stored pictures by poc for qsort in ascending order
*
************************************************************************
*/
static int compare_pic_by_poc_asc( const void *arg1, const void *arg2 )
{
	if ( (*(StorablePicture**)arg1)->poc < (*(StorablePicture**)arg2)->poc)
		return -1;
	if ( (*(StorablePicture**)arg1)->poc > (*(StorablePicture**)arg2)->poc)
		return 1;
	else
		return 0;
}


/*!
************************************************************************
* \brief
*    compares two stored pictures by poc for qsort in descending order
*
************************************************************************
*/
static int compare_pic_by_poc_desc( const void *arg1, const void *arg2 )
{
	if ( (*(StorablePicture**)arg1)->poc < (*(StorablePicture**)arg2)->poc)
		return 1;
	if ( (*(StorablePicture**)arg1)->poc > (*(StorablePicture**)arg2)->poc)
		return -1;
	else
		return 0;
}


/*!
************************************************************************
* \brief
*    compares two frame stores by poc for qsort in ascending order
*
************************************************************************
*/
static int compare_fs_by_poc_asc( const void *arg1, const void *arg2 )
{
	if ( (*(FrameStore**)arg1)->poc < (*(FrameStore**)arg2)->poc)
		return -1;
	if ( (*(FrameStore**)arg1)->poc > (*(FrameStore**)arg2)->poc)
		return 1;
	else
		return 0;
}


/*!
************************************************************************
* \brief
*    compares two frame stores by poc for qsort in descending order
*
************************************************************************
*/
static int compare_fs_by_poc_desc( const void *arg1, const void *arg2 )
{
	if ( (*(FrameStore**)arg1)->poc < (*(FrameStore**)arg2)->poc)
		return 1;
	if ( (*(FrameStore**)arg1)->poc > (*(FrameStore**)arg2)->poc)
		return -1;
	else
		return 0;
}


/*!
************************************************************************
* \brief
*    returns true, if picture is short term reference picture
*
************************************************************************
*/
int is_short_ref(StorablePicture *s)
{
	return ((s->used_for_reference) && (!(s->is_long_term)));
}


/*!
************************************************************************
* \brief
*    returns true, if picture is long term reference picture
*
************************************************************************
*/
int is_long_ref(StorablePicture *s)
{
	return ((s->used_for_reference) && (s->is_long_term));
}


/*!
************************************************************************
* \brief
*    Generates a alternating field list from a given FrameStore list
*
************************************************************************
*/
static void gen_pic_list_from_frame_list(PictureStructure currStrcture, FrameStore **fs_list, int list_idx, StorablePicture **list, int *list_size, int long_term)
{
	int top_idx = 0;
	int bot_idx = 0;
	
	int (*is_ref)(StorablePicture *s);
	if (long_term)
		is_ref=is_long_ref;
	else
		is_ref=is_short_ref;
}

/***********************************************************************
*	- Code not related to baseline removed.
*
*	 "Muhammad Tahir Awan" <tahir.awan@inforient.com>,
*	 "Umair Razzak" <umair.razzak@inforient.com>
****************************
*	 Changes till 21-11-2005
***********************************************************************
*/
void init_lists_baseline( h264_decoder* dec_params )
{
	int currSliceType = dec_params->img->type;
	int add_top = 0, add_bottom = 0;
	unsigned i;
	int j;
	int MaxFrameNum = 1 << (dec_params->active_sps->log2_max_frame_num_minus4 + 4);
	int diff;
	
	int list0idx = 0;
	int list0idx_1 = 0;
	int listltidx = 0;
	
	//FrameStore **fs_list0;
	//FrameStore **fs_list1;
	//FrameStore **fs_listlt;
	
	StorablePicture *tmp_s;
	
	//if (currPicStructure == FRAME)  
	{
		for (i=0; i < dec_params->dpb.ref_frames_in_buffer; i++)
		{
			if (dec_params->dpb.fs_ref[i]->is_used==3)
			{
				if ((dec_params->dpb.fs_ref[i]->frame->used_for_reference)&&(!dec_params->dpb.fs_ref[i]->frame->is_long_term))
				{
					if( dec_params->dpb.fs_ref[i]->frame_num > dec_params->img->frame_num )
					{
						dec_params->dpb.fs_ref[i]->frame_num_wrap = dec_params->dpb.fs_ref[i]->frame_num - MaxFrameNum;
					}
					else
					{
						dec_params->dpb.fs_ref[i]->frame_num_wrap = dec_params->dpb.fs_ref[i]->frame_num;
					}
					dec_params->dpb.fs_ref[i]->frame->pic_num = dec_params->dpb.fs_ref[i]->frame_num_wrap;
				}
			}
		}
		// update long_term_pic_num
		for (i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
		{
			if (dec_params->dpb.fs_ltref[i]->is_used==3)
			{
				if (dec_params->dpb.fs_ltref[i]->frame->is_long_term)
				{
					dec_params->dpb.fs_ltref[i]->frame->long_term_pic_num = dec_params->dpb.fs_ltref[i]->frame->long_term_frame_idx;
				}
			}
		}
	}
	
	if (currSliceType == I_SLICE)
	{
		///LIST
		dec_params->listXsize[0] = 0;
		dec_params->listXsize[1] = 0;
		//dec_params->listXsize = 0;
		return;
	}
	
	if (currSliceType == P_SLICE)
	{
		// Calculate FrameNumWrap and PicNum
		{
			for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
			{
				if (dec_params->dpb.fs_ref[i]->is_used==3)
				{
					if ((dec_params->dpb.fs_ref[i]->frame->used_for_reference)&&(!dec_params->dpb.fs_ref[i]->frame->is_long_term))
					{
						dec_params->listX[0][list0idx++] = dec_params->dpb.fs_ref[i]->frame;
						//dec_params->listX[list0idx++] = dec_params->dpb.fs_ref[i]->frame;
					}
				}
			}
			// order list 0 by PicNum
			qsort((void *)dec_params->listX[0], list0idx, sizeof(StorablePicture*) , compare_pic_by_pic_num_desc);
			//qsort((void *)dec_params->listX, list0idx, sizeof(StorablePicture*), compare_pic_by_pic_num_desc);
			
			dec_params->listXsize[0] = list0idx;
			//dec_params->listXsize = list0idx;
			//      printf("listX[0] (PicNum): "); for (i=0; i<list0idx; i++){printf ("%d  ", listX[0][i]->pic_num);} printf("\n");
			
			// long term handling
			for (i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
			{
				if (dec_params->dpb.fs_ltref[i]->is_used==3)
				{
					if (dec_params->dpb.fs_ltref[i]->frame->is_long_term)
					{
						dec_params->listX[0][list0idx++]=dec_params->dpb.fs_ltref[i]->frame;
						//dec_params->listX[list0idx++]=dec_params->dpb.fs_ltref[i]->frame;
					}
				}
			}
			
			qsort((void *)&dec_params->listX[0][dec_params->listXsize[0]], list0idx-dec_params->listXsize[0], sizeof(StorablePicture*), compare_pic_by_lt_pic_num_asc);
			//qsort((void *)&dec_params->listX[dec_params->listXsize], list0idx-dec_params->listXsize, sizeof(StorablePicture*), compare_pic_by_lt_pic_num_asc);
			
			
			dec_params->listXsize[0] = list0idx;
			//dec_params->listXsize = list0idx;
			
		}
		
		dec_params->listXsize[1] = 0;
	}
	
	// for B frames, list1 intializations
	else
	{
		// B-Slice
		//if (currPicStructure == FRAME)  
		{
			for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
			{
				if (dec_params->dpb.fs_ref[i]->is_used==3)
				{
					if ((dec_params->dpb.fs_ref[i]->frame->used_for_reference)&&(!dec_params->dpb.fs_ref[i]->frame->is_long_term))
					{
						if (dec_params->img->framepoc > dec_params->dpb.fs_ref[i]->frame->poc)
						{
							dec_params->listX[0][list0idx++] = dec_params->dpb.fs_ref[i]->frame;
						}
					}
				}
			}
			qsort((void *)dec_params->listX[0], list0idx, sizeof(StorablePicture*), compare_pic_by_poc_desc);
			list0idx_1 = list0idx;
			for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
			{
				if (dec_params->dpb.fs_ref[i]->is_used==3)
				{
					if ((dec_params->dpb.fs_ref[i]->frame->used_for_reference)&&(!dec_params->dpb.fs_ref[i]->frame->is_long_term))
					{
						if (dec_params->img->framepoc < dec_params->dpb.fs_ref[i]->frame->poc)
						{
							dec_params->listX[0][list0idx++] = dec_params->dpb.fs_ref[i]->frame;
						}
					}
				}
			}
			qsort((void *)&dec_params->listX[0][list0idx_1], list0idx-list0idx_1, sizeof(StorablePicture*), compare_pic_by_poc_asc);
			
			for (j=0; j<list0idx_1; j++)
			{
				dec_params->listX[1][list0idx-list0idx_1+j]=dec_params->listX[0][j];
			}
			for (j=list0idx_1; j<list0idx; j++)
			{
				dec_params->listX[1][j-list0idx_1]=dec_params->listX[0][j];
			}
			
			dec_params->listXsize[0] = dec_params->listXsize[1] = list0idx;
			
			//      printf("listX[0] currPoc=%d (Poc): ", img->framepoc); for (i=0; i<listXsize[0]; i++){printf ("%d  ", listX[0][i]->poc);} printf("\n");
			//      printf("listX[1] currPoc=%d (Poc): ", img->framepoc); for (i=0; i<listXsize[1]; i++){printf ("%d  ", listX[1][i]->poc);} printf("\n");
			
			// long term handling
			for (i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
			{
				if (dec_params->dpb.fs_ltref[i]->is_used==3)
				{
					if (dec_params->dpb.fs_ltref[i]->frame->is_long_term)
					{
						dec_params->listX[0][list0idx]  =dec_params->dpb.fs_ltref[i]->frame;
						dec_params->listX[1][list0idx++]=dec_params->dpb.fs_ltref[i]->frame;
					}
				}
			}
			qsort((void *)&dec_params->listX[0][dec_params->listXsize[0]], list0idx-dec_params->listXsize[0], sizeof(StorablePicture*), compare_pic_by_lt_pic_num_asc);
			qsort((void *)&dec_params->listX[1][dec_params->listXsize[0]], list0idx-dec_params->listXsize[0], sizeof(StorablePicture*), compare_pic_by_lt_pic_num_asc);
			dec_params->listXsize[0] = dec_params->listXsize[1] = list0idx;
		}
	}	  
	
	
	
	
	
	
	
	
	/*******************8888888888888888888888888888888888888**************************/
	
	
	
	
	
	///LIST
	
	// list 1 initializations
	
	if ((dec_params->listXsize[0] == dec_params->listXsize[1]) && (dec_params->listXsize[0] > 1))
	{
		// check if lists are identical, if yes swap first two elements of listX[1]
		diff=0;
		for (j = 0; j< dec_params->listXsize[0]; j++)
		{
			if (dec_params->listX[0][j]!=dec_params->listX[1][j])
				diff=1;
		}
		if (!diff)
		{
			tmp_s = dec_params->listX[1][0];
			dec_params->listX[1][0]=dec_params->listX[1][1];
			dec_params->listX[1][1]=tmp_s;
		}
	}
	
	// set max size
	dec_params->listXsize[0] = min (dec_params->listXsize[0], dec_params->img->num_ref_idx_l0_active);
	//dec_params->listXsize = min (dec_params->listXsize, dec_params->img->num_ref_idx_l0_active);
	dec_params->listXsize[1] = min (dec_params->listXsize[1], dec_params->img->num_ref_idx_l1_active);
	
	///LIST  
	//dec_params->listXsize[1] = min (dec_params->listXsize[1], 0);
	
	
	// set the unused list entries to NULL
	///LIST
	for (i=dec_params->listXsize[0]; i< (MAX_LIST_SIZE) ; i++)
		//for (i=dec_params->listXsize; i< (MAX_LIST_SIZE) ; i++)
	{
		dec_params->listX[0][i] = NULL;
		//    dec_params->listX[i] = NULL;
	}
	
	
	for (i=dec_params->listXsize[1]; i< (MAX_LIST_SIZE) ; i++)
	{
		dec_params->listX[1][i] = NULL;
	}
	
}

/*!
************************************************************************
* \brief
*    Initialize listX[2..5] from lists 0 and 1
*    listX[2]: list0 for current_field==top
*    listX[3]: list1 for current_field==top
*    listX[4]: list0 for current_field==bottom
*    listX[5]: list1 for current_field==bottom
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void init_mbaff_lists(  h264_decoder* dec_params)
{
	
//	unsigned j;
//	int i;
	return;
	/*
	for (i=2;i<6;i++)
	{
    for (j=0; j<MAX_LIST_SIZE; j++)
    {
	dec_params->listX[i][j] = NULL;
    }
    dec_params->listXsize[i]=0;
	}
	
	 dec_params->listXsize[2]=dec_params->listXsize[4]=dec_params->listXsize[0] * 2;
	 
	  dec_params->listXsize[3]=dec_params->listXsize[5]=dec_params->listXsize[1] * 2;
	*/
}

/*!
************************************************************************
* \brief
*    Returns short term pic with given picNum
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static StorablePicture*  get_short_term_pic(int picNum, h264_decoder* dec_params)
{
	unsigned i;
	
	for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
	{
		//if (dec_params->img->structure==FRAME)
		{
			if (dec_params->dpb.fs_ref[i]->is_reference == 3)
				if ((!dec_params->dpb.fs_ref[i]->frame->is_long_term)&&(dec_params->dpb.fs_ref[i]->frame->pic_num == picNum))
					return dec_params->dpb.fs_ref[i]->frame;
		}
	}
	return NULL;
}

/*!
************************************************************************
* \brief
*    Returns short term pic with given LongtermPicNum
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static StorablePicture*  get_long_term_pic(int LongtermPicNum, h264_decoder* dec_params)
{
	unsigned i;
	
	for (i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
	{
		if (dec_params->dpb.fs_ltref[i]->is_reference == 3)
		{
			if ((dec_params->dpb.fs_ltref[i]->frame->is_long_term)&&(dec_params->dpb.fs_ltref[i]->frame->long_term_pic_num == LongtermPicNum))
			{
				return dec_params->dpb.fs_ltref[i]->frame;
			}
		}
	}
	return NULL;
}

/*!
************************************************************************
* \brief
*    Reordering process for short-term reference pictures
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void reorder_short_term(StorablePicture **RefPicListX, int num_ref_idx_lX_active_minus1, int picNumLX, int *refIdxLX, h264_decoder* dec_params)
{
	int cIdx, nIdx;
	
	StorablePicture *picLX;
	
	picLX = get_short_term_pic(picNumLX, dec_params);
	
	for( cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx-- )
	{
		RefPicListX[ cIdx ] = RefPicListX[ cIdx - 1];
	}
	
	RefPicListX[ (*refIdxLX)++ ] = picLX;
	
	nIdx = *refIdxLX;
	
	for( cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++ )
	{
		if (RefPicListX[ cIdx ])
		{
			if( (RefPicListX[ cIdx ]->is_long_term ) ||  (RefPicListX[ cIdx ]->pic_num != picNumLX ))
			{
				RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];
			}
		}
	}
}


/*!
************************************************************************
* \brief
*    Reordering process for short-term reference pictures
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void reorder_long_term(StorablePicture **RefPicListX, int num_ref_idx_lX_active_minus1, int LongTermPicNum, int *refIdxLX, h264_decoder* dec_params)
{
	int cIdx, nIdx;
	
	StorablePicture *picLX;
	
	picLX = get_long_term_pic(LongTermPicNum, dec_params);
	
	for( cIdx = num_ref_idx_lX_active_minus1+1; cIdx > *refIdxLX; cIdx-- )
	{
		RefPicListX[ cIdx ] = RefPicListX[ cIdx - 1];
	}
	
	RefPicListX[ (*refIdxLX)++ ] = picLX;
	
	nIdx = *refIdxLX;
	
	for( cIdx = *refIdxLX; cIdx <= num_ref_idx_lX_active_minus1+1; cIdx++ )
	{
		if( (!RefPicListX[ cIdx ]->is_long_term ) ||  (RefPicListX[ cIdx ]->long_term_pic_num != LongTermPicNum ))
		{
			RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];
		}
	}
}


/*!
************************************************************************
* \brief
*    Reordering process for reference picture lists
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void reorder_ref_pic_list(StorablePicture **list, 
						  int *list_size, 
						  int num_ref_idx_lX_active_minus1, 
						  int *remapping_of_pic_nums_idc, 
						  int *abs_diff_pic_num_minus1, 
						  int *long_term_pic_idx,
						  h264_decoder* dec_params)
{
	int i;
	
	int maxPicNum, currPicNum, picNumLXNoWrap, picNumLXPred, picNumLX;
	int refIdxLX = 0;
	
	//if (dec_params->img->structure==FRAME)
	//{
    maxPicNum  = dec_params->img->MaxFrameNum;
    currPicNum = dec_params->img->frame_num;
	//}
	//else
	//{
	//  maxPicNum  = 2 * dec_params->img->MaxFrameNum;
	//  currPicNum = 2 * dec_params->img->frame_num + 1;
	//}
	
	picNumLXPred = currPicNum;
	
	for (i=0; remapping_of_pic_nums_idc[i]!=3; i++)
	{
		if (remapping_of_pic_nums_idc[i]>3)
		{
			printf("Invalid remapping_of_pic_nums_idc command");
			exit(0);
		}
		
		if (remapping_of_pic_nums_idc[i] < 2)
		{
			if (remapping_of_pic_nums_idc[i] == 0)
			{
				if( picNumLXPred - ( abs_diff_pic_num_minus1[i] + 1 ) < 0 )
					picNumLXNoWrap = picNumLXPred - ( abs_diff_pic_num_minus1[i] + 1 ) + maxPicNum;
				else
					picNumLXNoWrap = picNumLXPred - ( abs_diff_pic_num_minus1[i] + 1 );
			}
			else // (remapping_of_pic_nums_idc[i] == 1)
			{
				if( picNumLXPred + ( abs_diff_pic_num_minus1[i] + 1 )  >=  maxPicNum )
					picNumLXNoWrap = picNumLXPred + ( abs_diff_pic_num_minus1[i] + 1 ) - maxPicNum;
				else
					picNumLXNoWrap = picNumLXPred + ( abs_diff_pic_num_minus1[i] + 1 );
			}
			picNumLXPred = picNumLXNoWrap;
			
			if( picNumLXNoWrap > currPicNum )
				picNumLX = picNumLXNoWrap - maxPicNum;
			else
				picNumLX = picNumLXNoWrap;
			
			reorder_short_term(list, num_ref_idx_lX_active_minus1, picNumLX, &refIdxLX,dec_params);
		}
		else //(remapping_of_pic_nums_idc[i] == 2)
		{
			reorder_long_term(list, num_ref_idx_lX_active_minus1, long_term_pic_idx[i], &refIdxLX,dec_params);
		}
		
	}
	// that's a definition
	*list_size = num_ref_idx_lX_active_minus1 + 1;
}



/*!
************************************************************************
* \brief
*    Update the list of frame stores that contain reference frames/fields
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void update_ref_list(  h264_decoder* dec_params)
{
	unsigned i, j;
	for (i=0, j=0; i<dec_params->dpb.used_size; i++)
	{
		if (is_short_term_reference(dec_params->dpb.fs[i]))
		{
			dec_params->dpb.fs_ref[j++]=dec_params->dpb.fs[i];
		}
	}
	
	dec_params->dpb.ref_frames_in_buffer = j;
	
	while (j<dec_params->dpb.size)
	{
		dec_params->dpb.fs_ref[j++]=NULL;
	}
}


/*!
************************************************************************
* \brief
*    Update the list of frame stores that contain long-term reference 
*    frames/fields
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void update_ltref_list(  h264_decoder* dec_params)
{
	unsigned i, j;
	for (i=0, j=0; i<dec_params->dpb.used_size; i++)
	{
		if (is_long_term_reference(dec_params->dpb.fs[i]))
		{
			dec_params->dpb.fs_ltref[j++]=dec_params->dpb.fs[i];
		}
	}
	
	dec_params->dpb.ltref_frames_in_buffer=j;
	
	while (j<dec_params->dpb.size)
	{
		dec_params->dpb.fs_ltref[j++]=NULL;
	}
}

/*!
************************************************************************
* \brief
*    Perform Memory management for idr pictures
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void idr_memory_management( StorablePicture* p,  h264_decoder* dec_params )
{
	unsigned i;
	
	assert (p->idr_flag);
	
	if (p->no_output_of_prior_pics_flag)
	{
		// free all stored pictures
		for (i=0; i<dec_params->dpb.used_size; i++)
		{
			// reset all reference settings
			free_frame_store(dec_params->dpb.fs[i]);
			dec_params->dpb.fs[i] = alloc_frame_store(dec_params);
		}
		for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
		{
			dec_params->dpb.fs_ref[i]=NULL;
		}
		for (i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
		{
			dec_params->dpb.fs_ltref[i]=NULL;
		}
		dec_params->dpb.used_size=0;
	}
	else
	{
		flush_dpb(dec_params);
	}
	dec_params->dpb.last_picture = NULL;
	
	update_ref_list(dec_params);
	update_ltref_list(dec_params);
	dec_params->dpb.last_output_poc = INT_MIN;
	
	if (p->long_term_reference_flag)
	{
		dec_params->dpb.max_long_term_pic_idx = 0;
		p->is_long_term           = 1;
		p->long_term_frame_idx    = 0;
	}
	else
	{
		dec_params->dpb.max_long_term_pic_idx = -1;
		p->is_long_term           = 0;
	}
}

/*!
************************************************************************
* \brief
*    Perform Sliding window decoded reference picture marking process
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void sliding_window_memory_management( StorablePicture* p, h264_decoder* dec_params )
{
	unsigned i;
	
	assert (!p->idr_flag);
	// if this is a reference pic with sliding sliding window, unmark first ref frame
	if (dec_params->dpb.ref_frames_in_buffer==dec_params->dpb.num_ref_frames - dec_params->dpb.ltref_frames_in_buffer)
	{
		for (i=0; i<dec_params->dpb.used_size;i++)
		{
			if (dec_params->dpb.fs[i]->is_reference  && (!(dec_params->dpb.fs[i]->is_long_term)))
			{
				unmark_for_reference(dec_params->dpb.fs[i]);
				update_ref_list(dec_params);
				break;
			}
		}
	}
	
	p->is_long_term = 0;
}

/*!
************************************************************************
* \brief
*    Calculate picNumX
************************************************************************
*/
static int get_pic_num_x (StorablePicture *p, int difference_of_pic_nums_minus1)
{
	int currPicNum;
	//  if (p->structure == FRAME)
    currPicNum = p->pic_num;//currPicNum = p->frame_num;
	//  else 
	//    currPicNum = 2 * p->frame_num + 1;
	
	return currPicNum - (difference_of_pic_nums_minus1 + 1);
}


/*!
************************************************************************
* \brief
*    Adaptive Memory Management: Mark short term picture unused
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void mm_unmark_short_term_for_reference( StorablePicture *p, int difference_of_pic_nums_minus1, h264_decoder* dec_params )
{
	int picNumX;
	unsigned i;
	picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);
	for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
	{
		//  if (p->structure == FRAME)
		//  {
		if ((dec_params->dpb.fs_ref[i]->is_reference==3) && (dec_params->dpb.fs_ref[i]->is_long_term==0))
		{
			if (dec_params->dpb.fs_ref[i]->frame->pic_num == picNumX)
			{
				unmark_for_reference(dec_params->dpb.fs_ref[i]);
				return;
			}
		}
		//  }
		//  else
		//  {
		//  }
	}
}


/*!
************************************************************************
* \brief
*    Adaptive Memory Management: Mark long term picture unused
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void mm_unmark_long_term_for_reference(StorablePicture *p, int long_term_pic_num, h264_decoder* dec_params)
{
	unsigned i;
	for (i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
	{
		//    if (p->structure == FRAME)
		//    {
		if ((dec_params->dpb.fs_ltref[i]->is_reference==3) && (dec_params->dpb.fs_ltref[i]->is_long_term==3))
		{
			if (dec_params->dpb.fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num)
			{
				unmark_for_long_term_reference(dec_params->dpb.fs_ltref[i]);
			}
		}
		//    }
		//    else
		//    {
		//	  }
	}
}


/*!
************************************************************************
* \brief
*    Mark a long-term reference frame or complementary field pair unused for referemce
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void unmark_long_term_frame_for_reference_by_frame_idx(int long_term_frame_idx,  h264_decoder* dec_params)
{
	unsigned i;
	for(i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
	{
		if (dec_params->dpb.fs_ltref[i]->long_term_frame_idx == long_term_frame_idx)
			unmark_for_long_term_reference(dec_params->dpb.fs_ltref[i]);
	}
}

/*!
************************************************************************
* \brief
*    Mark a long-term reference field unused for reference only if it's not
*    the complementary field of the picture indicated by picNumX
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
///remove
static void unmark_long_term_field_for_reference_by_frame_idx(PictureStructure structure, int long_term_frame_idx, int mark_current, unsigned curr_frame_num, int curr_pic_num,
															  h264_decoder* dec_params)
{
//	unsigned i;
	assert(structure!=FRAME);
	if (curr_pic_num<0)
		curr_pic_num+=(2*dec_params->img->MaxFrameNum);
}


/*!
************************************************************************
* \brief
*    mark a picture as long-term reference
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void mark_pic_long_term(StorablePicture* p, int long_term_frame_idx, int picNumX,  h264_decoder* dec_params)
{
	unsigned i;
//	int add_top, add_bottom;
	
	//if (p->structure == FRAME)
	//{
    for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
    {
		if (dec_params->dpb.fs_ref[i]->is_reference == 3)
		{
			if ((!dec_params->dpb.fs_ref[i]->frame->is_long_term)&&(dec_params->dpb.fs_ref[i]->frame->pic_num == picNumX))
			{
				dec_params->dpb.fs_ref[i]->long_term_frame_idx = dec_params->dpb.fs_ref[i]->frame->long_term_frame_idx
					= long_term_frame_idx;
				dec_params->dpb.fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
				dec_params->dpb.fs_ref[i]->frame->is_long_term = 1;
				
				dec_params->dpb.fs_ref[i]->is_long_term = 3;
				return;
			}
		}
    }
    printf ("Warning: reference frame for long term marking not found\n");
	//}
	//else
	//{
	//}
}


/*!
************************************************************************
* \brief
*    Assign a long term frame index to a short term picture
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params 
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void mm_assign_long_term_frame_idx(StorablePicture* p, int difference_of_pic_nums_minus1, 
										  int long_term_frame_idx,   h264_decoder* dec_params)
{
	int picNumX;
	
	picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);
	
	// remove frames/fields with same long_term_frame_idx
	//if (p->structure == FRAME)
	//{
    unmark_long_term_frame_for_reference_by_frame_idx(long_term_frame_idx,dec_params);
	//}
	//else
	//{
	//    unsigned i;
	//    PictureStructure structure = FRAME;
	//
	//    if (structure==FRAME)
	//    {
	//      error ("field for long term marking not found",200,dec_params,dec_outputs);
	//    }
	//    unmark_long_term_field_for_reference_by_frame_idx(structure, long_term_frame_idx, 0, 0, picNumX,dec_outputs,dec_params);
	//}
	
	mark_pic_long_term(p, long_term_frame_idx, picNumX,dec_params);
}

/*!
************************************************************************
* \brief
*    Set new max long_term_frame_idx
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void mm_update_max_long_term_frame_idx(int max_long_term_frame_idx_plus1,  h264_decoder* dec_params)
{
	unsigned i;
	
	dec_params->dpb.max_long_term_pic_idx = max_long_term_frame_idx_plus1 - 1;
	
	// check for invalid frames
	for (i=0; i<dec_params->dpb.ltref_frames_in_buffer; i++)
	{
		if (dec_params->dpb.fs_ltref[i]->long_term_frame_idx > dec_params->dpb.max_long_term_pic_idx)
		{
			unmark_for_long_term_reference(dec_params->dpb.fs_ltref[i]);
		}
	}
}


/*!
************************************************************************
* \brief
*    Mark all long term reference pictures unused for reference
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void mm_unmark_all_long_term_for_reference (  h264_decoder* dec_params)
{
	mm_update_max_long_term_frame_idx(0,dec_params);
}

/*!
************************************************************************
* \brief
*    Mark all short term reference pictures unused for reference
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void mm_unmark_all_short_term_for_reference ( h264_decoder* dec_params )
{
	unsigned int i;
	for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
	{
		unmark_for_reference(dec_params->dpb.fs_ref[i]);
	}
	update_ref_list(dec_params);
}


/*!
************************************************************************
* \brief
*    Mark the current picture used for long term reference
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void mm_mark_current_picture_long_term(StorablePicture *p, int long_term_frame_idx, h264_decoder* dec_params)
{
	// remove long term pictures with same long_term_frame_idx
	//  if (p->structure == FRAME)
	//  {
    unmark_long_term_frame_for_reference_by_frame_idx(long_term_frame_idx,dec_params);
	//  }
	//  else
	//  {
	//    unmark_long_term_field_for_reference_by_frame_idx(p->structure, long_term_frame_idx, 1, p->pic_num, 0,dec_outputs,dec_params);
	//  }
	
	p->is_long_term = 1;
	p->long_term_frame_idx = long_term_frame_idx;
}


/*!
************************************************************************
* \brief
*    Perform Adaptive memory control decoded reference picture marking process
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void adaptive_memory_management( StorablePicture* p, h264_decoder* dec_params )
{
	DecRefPicMarking_t *tmp_drpm;
	dec_params->img->last_has_mmco_5 = 0;
	
	assert (!p->idr_flag);
	assert (p->adaptive_ref_pic_buffering_flag);
	
	while (p->dec_ref_pic_marking_buffer)
	{
		tmp_drpm = p->dec_ref_pic_marking_buffer;
		switch (tmp_drpm->memory_management_control_operation)
		{
		case 0:
			if (tmp_drpm->Next != NULL)
			{
				printf("memory_management_control_operation = 0 not last operation in buffer");
				exit(0);
			}
			break;
		case 1:
			mm_unmark_short_term_for_reference(p, tmp_drpm->difference_of_pic_nums_minus1,dec_params);
			update_ref_list(dec_params);
			break;
		case 2:
			mm_unmark_long_term_for_reference(p, tmp_drpm->long_term_pic_num,dec_params);
			update_ltref_list(dec_params);
			break;
		case 3:
			mm_assign_long_term_frame_idx(p, tmp_drpm->difference_of_pic_nums_minus1, tmp_drpm->long_term_frame_idx,dec_params);
			update_ref_list(dec_params);
			update_ltref_list(dec_params);
			break;
		case 4:
			mm_update_max_long_term_frame_idx (tmp_drpm->max_long_term_frame_idx_plus1,dec_params);
			update_ltref_list(dec_params);
			break;
		case 5:
			mm_unmark_all_short_term_for_reference(dec_params);
			mm_unmark_all_long_term_for_reference(dec_params);
			dec_params->img->last_has_mmco_5 = 1;
			break;
		case 6:
			mm_mark_current_picture_long_term(p, tmp_drpm->long_term_frame_idx,dec_params);
			check_num_ref(dec_params);
			break;
		default:
			printf("invalid memory_management_control_operation in buffer");
			exit(0);
		}
		p->dec_ref_pic_marking_buffer = tmp_drpm->Next;
		h264_free (tmp_drpm);
	}
	if ( dec_params->img->last_has_mmco_5 )
	{
		//p->pic_num = p->frame_num = 0;
		p->pic_num = 0;
		
		//    switch (p->structure)
		//    {
		//    case FRAME:
		//      {
        //p->top_poc    -= p->poc;
		dec_params->img->toppoc -= p->poc;
        //p->bottom_poc -= p->poc;
		dec_params->img->bottompoc -= p->poc;
        //dec_params->img->toppoc = p->top_poc;
        //dec_params->img->bottompoc = p->bottom_poc;
		
        p->poc = min (dec_params->img->toppoc, dec_params->img->bottompoc);
        dec_params->img->framepoc = p->poc;
		//       break;
		//      }
		//    }
		dec_params->img->ThisPOC = p->poc;
		flush_dpb(dec_params);
	}
}


/*!
************************************************************************
* \brief
*    Store a picture in DPB. This includes cheking for space in DPB and 
*    flushing frames.
*    If we received a frame, we need to check for a new store, if we
*    got a field, check if it's the second field of an already allocated
*    store.
*
* \param p
*    Picture to be stored
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

void store_picture_in_dpb_baseline(  h264_decoder* dec_params )
{
	StorablePicture* p=dec_params->dec_picture;
	unsigned i;
	int poc, pos;
	// diagnostics
	//printf ("Storing (%s) non-ref pic with frame_num #%d\n", (p->type == FRAME)?"FRAME":(p->type == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD", p->pic_num);
	// if frame, check for new store, 
	assert (p!=NULL);
	
	dec_params->img->last_has_mmco_5=0;
	
	if (p->idr_flag)
		idr_memory_management(p,dec_params);
	else
	{
		// adaptive memory management
		if (p->used_for_reference && (p->adaptive_ref_pic_buffering_flag))
			adaptive_memory_management(p,dec_params);
	}  
	
	// this is a frame or a field which has no stored complementary field
	
	// sliding window, if necessary
	if ((!p->idr_flag)&&(p->used_for_reference && (!p->adaptive_ref_pic_buffering_flag)))
	{
		sliding_window_memory_management(p,dec_params);
	} 
	
	// first try to remove unused frames
	if (dec_params->dpb.used_size==dec_params->dpb.size)
	{
		remove_unused_frame_from_dpb(dec_params);
	}
	
	// then output frames until one can be removed
	while (dec_params->dpb.used_size==dec_params->dpb.size)
	{
		// non-reference frames may be output directly
	/*	if (!p->used_for_reference)
		{
			get_smallest_poc(&poc, &pos,dec_params);
			if ((-1==pos) || (p->poc < poc))
			{
				direct_output(dec_params);
				return;
			}
		}*/
		// flush a frame
		output_one_frame_from_dpb(dec_params);
	}
	
	// check for duplicate frame number in short term reference buffer
	if ((p->used_for_reference)&&(!p->is_long_term))
	{
		for (i=0; i<dec_params->dpb.ref_frames_in_buffer; i++)
		{
			if (dec_params->dpb.fs_ref[i]->frame_num == p->pic_num)//if (dec_params->dpb.fs_ref[i]->frame_num == p->frame_num)
			{
				printf("duplicate frame_num im short-term reference picture buffer");
				exit(0);
			}
		}
		
	}
	// store at end of buffer
	//  printf ("store frame/field at pos %d\n",dpb.used_size);
	insert_picture_in_dpb_baseline(dec_params->dpb.fs[dec_params->dpb.used_size],dec_params);
	
	dec_params->dpb.last_picture = NULL;
	
	dec_params->dpb.used_size++;
	
	update_ref_list(dec_params);
	update_ltref_list(dec_params);
	
	check_num_ref(dec_params);
}

/*!
************************************************************************
* \brief
*    Insert the picture into the DPB. A free DPB position is necessary
*    for frames, .
*
* \param fs
*    FrameStore into which the picture will be inserted
* \param p
*    StorablePicture to be inserted
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void insert_picture_in_dpb_baseline( FrameStore* fs, h264_decoder* dec_params )
{
	StorablePicture* p=dec_params->dec_picture;
	//  printf ("insert (%s) pic with frame_num #%d, poc %d\n", (p->structure == FRAME)?"FRAME":(p->structure == TOP_FIELD)?"TOP_FIELD":"BOTTOM_FIELD", p->pic_num, p->poc);
	assert (p!=NULL);
	assert (fs!=NULL);
	
	//switch (p->structure)
	//{
	//case FRAME: 
    fs->frame = p;
    fs->is_used = 3;
    if (p->used_for_reference)
    {
		fs->is_reference = 3;
		fs->is_orig_reference = 3;
		if (p->is_long_term)
		{
			fs->is_long_term = 3;
			fs->long_term_frame_idx = p->long_term_frame_idx;
		}
    }
    // generate field views
	fs->poc = fs->frame->poc;
	if(dec_params->active_sps->profile_idc > 66)
	    dpb_split_field_baseline(fs, dec_params);
	//  break;
	//}
	fs->frame_num = p->pic_num;
	fs->is_output = p->is_output;
	
	//  OPTIONAL SNR CALCULATION REQUIRED ONLY FOR DEBUGGING 
	//if (fs->is_used==3)
	//{
	//  if (-1!=dec_outputs->p_ref)
	//    find_snr(fs->frame,dec_params,dec_outputs);
	//}
}

/*!
************************************************************************
* \brief
*    Check if one of the frames/fields in frame store is used for reference
************************************************************************
*/
static int is_used_for_reference(FrameStore* fs)
{
	if (fs->is_reference)
	{
		return 1;
	}
	
	if (fs->is_used == 3) // frame
	{
		if (fs->frame->used_for_reference)
		{
			return 1;
		}
	}
	
	return 0;
}


/*!
************************************************************************
* \brief
*    Check if one of the frames/fields in frame store is used for short-term reference
************************************************************************
*/
static int is_short_term_reference(FrameStore* fs)
{
	
	if (fs->is_used==3) // frame
	{
		if ((fs->frame->used_for_reference)&&(!fs->frame->is_long_term))
		{
			return 1;
		}
	}
	
	return 0;
}


/*!
************************************************************************
* \brief
*    Check if one of the frames/fields in frame store is used for short-term reference
************************************************************************
*/
static int is_long_term_reference(FrameStore* fs)
{
	
	if (fs->is_used==3) // frame
	{
		if ((fs->frame->used_for_reference)&&(fs->frame->is_long_term))
		{
			return 1;
		}
	}
	
	return 0;
}


/*!
************************************************************************
* \brief
*    remove one frame from DPB
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void remove_frame_from_dpb( int pos,  h264_decoder* dec_params )
{
	FrameStore* fs = dec_params->dpb.fs[pos];
	FrameStore* tmp;
	unsigned i;
	
	//  printf ("remove frame with frame_num #%d\n", fs->frame_num);
	fs->frame->is_empty = 1;			// buffer is empty
	
	switch (fs->is_used)
	{
	case 3:
		//free_storable_picture(fs->frame,dec_params,dec_outputs);
		//free_storable_picture(fs->top_field,dec_params,dec_outputs);
		//free_storable_picture(fs->bottom_field,dec_params,dec_outputs);
		fs->frame=NULL;
		break;
	case 2:
		//free_storable_picture(fs->bottom_field,dec_params,dec_outputs);
		break;
	case 1:
		//free_storable_picture(fs->top_field,dec_params,dec_outputs);
		break;
	case 0:
		break;
	default:
		printf("invalid frame store type");
		exit(0);
	}
	fs->is_used = 0;
	fs->is_long_term = 0;
	fs->is_reference = 0;
	fs->is_orig_reference = 0;
	
	// move empty framestore to end of buffer
	tmp = dec_params->dpb.fs[pos];
	
	for (i=pos; i<dec_params->dpb.used_size-1;i++)
	{
		dec_params->dpb.fs[i] = dec_params->dpb.fs[i+1];
	}
	dec_params->dpb.fs[dec_params->dpb.used_size-1] = tmp;
	dec_params->dpb.used_size--;
}

/*!
************************************************************************
* \brief
*    find smallest POC in the DPB.
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void get_smallest_poc( int *poc,int * pos, h264_decoder* dec_params )
{
	unsigned i;
	
	if (dec_params->dpb.used_size<1)
	{
		printf("Cannot determine smallest POC, DPB empty.");
		exit(0);
	}
	
	*pos=-1;
	*poc = INT_MAX;
	for (i=0; i<dec_params->dpb.used_size; i++)
	{
		if ((*poc>dec_params->dpb.fs[i]->poc)&&(!dec_params->dpb.fs[i]->is_output))
		{
			*poc = dec_params->dpb.fs[i]->poc;
			*pos=i;
		}
	}
}

/*!
************************************************************************
* \brief
*    Remove a picture from DPB which is no longer needed.
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static int remove_unused_frame_from_dpb( h264_decoder* dec_params )
{
	unsigned i;
	
	// check for frames that were already output and no longer used for reference
	for (i=0; i<dec_params->dpb.used_size; i++)
	{
		if (dec_params->dpb.fs[i]->is_output && (!is_used_for_reference(dec_params->dpb.fs[i])))
		{
			remove_frame_from_dpb(i,dec_params);
			return 1;
		}
	}
	return 0;
}

/*!
************************************************************************
* \brief
*    Output one picture stored in the DPB.
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
static void output_one_frame_from_dpb( h264_decoder* dec_params )
{
	int poc, pos;
	//diagnostics
	if (dec_params->dpb.used_size<1)
	{
		printf("Cannot output frame, DPB empty.");
		exit(0);
	}
	
	// find smallest POC
	get_smallest_poc(&poc, &pos,dec_params);
	
	if(pos==-1)
	{
		printf("no frames for output available");
		exit(0);
	}
	
	// call the output function
	//  printf ("output frame with frame_num #%d, poc %d (dpb. dpb.size=%d, dpb.used_size=%d)\n", dpb.fs[pos]->frame_num, dpb.fs[pos]->frame->poc, dpb.size, dpb.used_size);
	
	//write_stored_frame(dec_params->dpb.fs[pos], dec_params->p_out,dec_params);
	write_stored_frame(dec_params->dpb.fs[pos], dec_params->f_out,dec_params);
	
	if (dec_params->dpb.last_output_poc >= poc)
	{
		printf("output POC must be in ascending order");
		exit(0);
	} 
	dec_params->dpb.last_output_poc = poc;
	// free frame store and move empty store to end of buffer
	if (!is_used_for_reference(dec_params->dpb.fs[pos]))
	{
		remove_frame_from_dpb(pos,dec_params);
	}
}



/*!
************************************************************************
* \brief
*    All stored picture are output. Should be called to empty the buffer
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void flush_dpb( h264_decoder* dec_params )
{
	unsigned i;
	
	//diagnostics
	//  printf("Flush remaining frames from dec_outputs->dpb. dpb.size=%d, dpb.used_size=%d\n",dpb.size,dpb.used_size);
	
	// mark all frames unused
	for (i=0; i<dec_params->dpb.used_size; i++)
	{
		unmark_for_reference (dec_params->dpb.fs[i]);
	}
	
	while (remove_unused_frame_from_dpb(dec_params)) ;
	
	// output frames in POC order
	while (dec_params->dpb.used_size)
	{
		output_one_frame_from_dpb(dec_params);
	}
	
	dec_params->dpb.last_output_poc = INT_MIN;
}

#define RSD(x) ((x&2)?(x|1):(x&(~1)))

/*!
************************************************************************
* \brief
*    Extract top field from a frame
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

void dpb_split_field_baseline(FrameStore *fs , h264_decoder* dec_params)
{

	int i, j;
	int idiv,jdiv;
	
	int dummylist0;
	int dummylist1;
	StorablePicture* frame = fs->frame;
	short* ref_idL0 = frame->ref_idL0;
	short* ref_idL1 = frame->ref_idL1;

//	int*   ref_pic_num = frame->ref_pic_num;

	char*  ref_idx_l0 = frame->ref_idx_l0;
	char*  ref_idx_l1 = frame->ref_idx_l1;
	
	int block_width = dec_params->img->width>>2;
	int block_height = dec_params->img->height>>2;

	short* slice_id = frame->slice_id;

	int num_blocks = dec_params->img->FrameSizeInMbs<<4;

	int mb_nr;
	fs->poc = fs->frame->poc;
	// Code re-added
/*	for (j=0 ; j < block_height ; j++)      
	{           
		jdiv= j>>2;
		for (i=0 ; i < block_width ; i++)          
		{   
			idiv = i >>2 ;
			mb_nr = (j>>2)*dec_params->img->FrameWidthInMbs + (i>>2);
			//			currentmb = twosz16 * (jdiv >> 1) + ((idiv) << 1) + (jdiv%2);      
			dummylist0 = fs->frame->ref_idx[LIST_0][j][i];
			dummylist1 = fs->frame->ref_idx[LIST_1][j][i];    
		//	if(frame->slice_id1[mb_nr] < 0 || frame->slice_id1[mb_nr] >10)
		//		printf("gotcha");
			frame->ref_id[LIST_0][j][i] = (dummylist0>=0)? frame->ref_pic_num[frame->slice_id1[mb_nr]][LIST_0][dummylist0] : -1;
			frame->ref_id[LIST_1][j][i] = (dummylist1>=0)? frame->ref_pic_num[frame->slice_id1[mb_nr]][LIST_1][dummylist1] : -1;

		//	fs->frame->ref_id[LIST_0][j][i] = (dummylist0>=0)? fs->frame->ref_pic_num[fs->frame->slice_id[j>>2][i>>2]][LIST_0][dummylist0] : -1;
		//	fs->frame->ref_id[LIST_1][j][i] = (dummylist1>=0)? fs->frame->ref_pic_num[fs->frame->slice_id[jdiv][idiv]][LIST_1][dummylist1] : -1;
			
		}      
	}
*/
/*	for(i = 0; i < dec_params->img->FrameSizeInMbs; i++)
	{
		int* ref_pic_num_l0 = frame->ref_pic_num[frame->slice_id1[i]][LIST_0];
		int* ref_pic_num_l1 = frame->ref_pic_num[frame->slice_id1[i]][LIST_1];
		for(j = 0; j < 16; j++)
		{
			*(ref_idL0++) = (*(ref_idx_l0) < 0) ? -1 : ref_pic_num_l0[*(ref_idx_l0++)];
			*(ref_idL1++) = (*(ref_idx_l1) < 0) ? -1 : ref_pic_num_l1[*(ref_idx_l1++)];
		}
	}
*/	
	//memset( &(fs->frame->field_frame[0][0]), 0, ((fs->frame->size_y * fs->frame->size_x) >> 4) * sizeof(byte));
}

/*!
************************************************************************
* \brief
*    Generate a frame from top and bottom fields, 
*    YUV components and display information only
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
///remove
void dpb_combine_field_yuv( FrameStore *fs, h264_decoder* dec_params )
{
//	int i;
	fs->frame->used_for_reference = 0;
	fs->frame->is_long_term = 0;
    if (fs->frame->is_long_term)
    {
		fs->frame->long_term_frame_idx = fs->long_term_frame_idx;
    }
	//fs->frame->coded_frame = 0;
}




/*!
************************************************************************
* \brief
*    Allocate memory for buffering of reference picture reordering commands
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void alloc_ref_pic_list_reordering_buffer(Slice *currSlice,  h264_decoder* dec_params )
{
	int size = dec_params->img->num_ref_idx_l0_active+1;
	
	if (dec_params->img->type!=I_SLICE)
	{
		if ((currSlice->remapping_of_pic_nums_idc_l0 = (int *)h264_malloc(size*sizeof(int)))==NULL) 
		{
			printf("alloc_ref_pic_list_reordering_buffer: remapping_of_pic_nums_idc_l0");
			exit(0);
		}
		if ((currSlice->abs_diff_pic_num_minus1_l0 = (int *)h264_malloc(size*sizeof(int)))==NULL) 
		{
			printf("alloc_ref_pic_list_reordering_buffer: abs_diff_pic_num_minus1_l0");
			exit(0);
		}
		if ((currSlice->long_term_pic_idx_l0 = (int *)h264_malloc(size*sizeof(int)))==NULL) 
		{
			printf("alloc_ref_pic_list_reordering_buffer: long_term_pic_idx_l0");
			exit(0);
		}
	}
	else
	{
		currSlice->remapping_of_pic_nums_idc_l0 = NULL;
		currSlice->abs_diff_pic_num_minus1_l0 = NULL;
		currSlice->long_term_pic_idx_l0 = NULL;
	}
	
	
	size = dec_params->img->num_ref_idx_l1_active+1;
	//  size = 1;
	
	
	if (dec_params->img->type==B_SLICE)
	{
		if ((currSlice->remapping_of_pic_nums_idc_l1 =(int *) h264_malloc(size*sizeof(int)))==NULL)
		{
			printf("alloc_ref_pic_list_reordering_buffer: remapping_of_pic_nums_idc_l1");
			exit(0);
		}
		if ((currSlice->abs_diff_pic_num_minus1_l1 = (int *) h264_malloc(size*sizeof(int)))==NULL)
		{
			printf("alloc_ref_pic_list_reordering_buffer: abs_diff_pic_num_minus1_l1");
			exit(0);
		}
		if ((currSlice->long_term_pic_idx_l1 = (int *) h264_malloc(size*sizeof(int)))==NULL)
		{
			printf("alloc_ref_pic_list_reordering_buffer: long_term_pic_idx_l1");
			exit(0);
		}
	}
	else
	{
		currSlice->remapping_of_pic_nums_idc_l1 = NULL;
		currSlice->abs_diff_pic_num_minus1_l1 = NULL;
		currSlice->long_term_pic_idx_l1 = NULL;
	}
}


/*!
************************************************************************
* \brief
*    Free memory for buffering of reference picture reordering commands
************************************************************************
*/
void free_ref_pic_list_reordering_buffer(Slice *currSlice)
{
	
	if (currSlice->remapping_of_pic_nums_idc_l0) 
		h264_free(currSlice->remapping_of_pic_nums_idc_l0);
	if (currSlice->abs_diff_pic_num_minus1_l0)
		h264_free(currSlice->abs_diff_pic_num_minus1_l0);
	if (currSlice->long_term_pic_idx_l0)
		h264_free(currSlice->long_term_pic_idx_l0);
	
	currSlice->remapping_of_pic_nums_idc_l0 = NULL;
	currSlice->abs_diff_pic_num_minus1_l0 = NULL;
	currSlice->long_term_pic_idx_l0 = NULL;
	

	if (currSlice->remapping_of_pic_nums_idc_l1)
		h264_free(currSlice->remapping_of_pic_nums_idc_l1);
	if (currSlice->abs_diff_pic_num_minus1_l1)
		h264_free(currSlice->abs_diff_pic_num_minus1_l1);
	if (currSlice->long_term_pic_idx_l1)
		h264_free(currSlice->long_term_pic_idx_l1);
	
	currSlice->remapping_of_pic_nums_idc_l1 = NULL;
	currSlice->abs_diff_pic_num_minus1_l1 = NULL;
	currSlice->long_term_pic_idx_l1 = NULL;

}

/*!
************************************************************************
* \brief
*      Tian Dong
*          June 13, 2002, Modifed on July 30, 2003
*
*      If a gap in frame_num is found, try to fill the gap
* \param img
*      
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
void fill_frame_num_gap( h264_decoder* dec_params )
{
	ImageParameters *img=dec_params->img;
	int CurrFrameNum;
	int UnusedShortTermFrameNum;
	StorablePicture *picture = NULL;
	int tmp1 = img->delta_pic_order_cnt[0];
	int tmp2 = img->delta_pic_order_cnt[1];
	img->delta_pic_order_cnt[0] = img->delta_pic_order_cnt[1] = 0;
	
	//  printf("A gap in frame number is found, try to fill it.\n");
	
	
	UnusedShortTermFrameNum = (img->pre_frame_num + 1) % img->MaxFrameNum;
	CurrFrameNum = img->frame_num;
	
	while (CurrFrameNum != UnusedShortTermFrameNum)
	{
		picture = 
			alloc_storable_picture (FRAME, img->width, img->height, img->width_cr, img->height_cr,dec_params);
		//picture->coded_frame = 1;
		picture->pic_num = UnusedShortTermFrameNum;
		//picture->frame_num = UnusedShortTermFrameNum;
		picture->non_existing = 1;
		picture->is_output = 1;
		picture->used_for_reference = 1;
		
		picture->adaptive_ref_pic_buffering_flag = 0;
		
		img->frame_num = UnusedShortTermFrameNum;
		if (dec_params->active_sps->pic_order_cnt_type!=0)
		{
			decode_poc(dec_params);
		}
		//picture->top_poc=img->toppoc;
		//picture->bottom_poc=img->bottompoc;
		picture->frame_poc=img->framepoc;
		picture->poc=img->framepoc;
		dec_params->dec_picture = picture;
		store_picture_in_dpb_baseline(dec_params);
		
		picture=NULL;
		img->pre_frame_num = UnusedShortTermFrameNum;
		UnusedShortTermFrameNum = (UnusedShortTermFrameNum + 1) % img->MaxFrameNum;
	}
	img->delta_pic_order_cnt[0] = tmp1;
	img->delta_pic_order_cnt[1] = tmp2;
	img->frame_num = CurrFrameNum;
	
}





/*!
************************************************************************
* \brief
*    Compute co-located motion info
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

void compute_colocated( h264_decoder* dec_params )
{	
	ColocatedParams* p = dec_params->Co_located;
	StorablePicture *fs;
	int i,j;//, ii, jj, jdiv;
	int quarter_height, quarter_width;
	short **p_mv_list0; 
	short **p_mv_list1;
	short *fs_mv_list0; 
	short *fs_mv_list1;	
	signed char *p_ref_idx_list0;
	signed char *p_ref_idx_list1;
	signed char *fs_ref_idx_list0;
	signed char *fs_ref_idx_list1;	
	int *p_ref_pic_id_list0;
	int *p_ref_pic_id_list1;
	int *fs_ref_id_list0;
	int *fs_ref_id_list1;
	int *p2_ref_pic_id_list0;
	int *p2_ref_pic_id_list1;
	ImageParameters *img=dec_params->img;  
	unsigned int mvWidth = img->width>>1;
	
	quarter_height = img->height>>2;
	quarter_width = img->width>>2;	
	fs = dec_params->listX[LIST_1][0];	


	if (/*!dec_params->active_sps->frame_mbs_only_flag ||*/ dec_params->active_sps->direct_8x8_inference_flag)      
	{
		int jj,ii;
		p->is_long_term = fs->is_long_term;
		for (j=0 ; j<quarter_height ; j++)      
		{                
			jj = RSD(j);
			p_mv_list0 = p->mv[LIST_0][j]; 
			p_mv_list1 = p->mv[LIST_1][j];
			fs_mv_list0 = (fs->mvL0 + jj*mvWidth); 
			fs_mv_list1 = (fs->mvL1 + jj*mvWidth);
			
			p_ref_idx_list0 = p->ref_idx[LIST_0][j];
			p_ref_idx_list1 = p->ref_idx[LIST_1][j];
			fs_ref_idx_list0 = fs->ref_idx[LIST_0][jj];
			fs_ref_idx_list1 = fs->ref_idx[LIST_1][jj];
			
			p_ref_pic_id_list0 = p->ref_pic_id[LIST_0][j];
			p_ref_pic_id_list1 = p->ref_pic_id[LIST_1][j];
			fs_ref_id_list0 = fs->ref_id[LIST_0][jj];
			fs_ref_id_list1 = fs->ref_id[LIST_1][jj];

		  
			for (i=0 ; i<quarter_width ; i++)          
			{           
				ii = RSD(i);
				
				p_mv_list0[i][0] = *(fs_mv_list0+(ii<<1));
				p_mv_list0[i][1] = *(fs_mv_list0+(ii<<1)+1);
				p_mv_list1[i][0] = *(fs_mv_list1+(ii<<1));
				p_mv_list1[i][1] = *(fs_mv_list1+(ii<<1)+1);



				p_ref_idx_list0[i] = fs_ref_idx_list0[ii];
				p_ref_idx_list1[i] = fs_ref_idx_list1[ii];

				p_ref_pic_id_list0[i] = fs_ref_id_list0[ii];
				p_ref_pic_id_list1[i] = fs_ref_id_list1[ii];
				

				
				if (dec_params->img->direct_spatial_mv_pred_flag == 1)
				{				
					p->moving_block[j][i]= 
						!(
							(!p->is_long_term 
								&& (
									(p_ref_idx_list0[i] == 0) 
									&&  (absz(p_mv_list0[i][0])>>1 == 0) 
									&&  (absz(p_mv_list0[i][1])>>1 == 0)
							   )
							) 
							|| ((p_ref_idx_list0[i] == -1) 
								&&  (p_ref_idx_list1[i] == 0) 
								&&  (absz(p_mv_list1[i][0])>>1 == 0) 
								&&  (absz(p_mv_list1[i][1])>>1 == 0)
							)
						);
				}
			}
		}
	}
	else
	{

		for (j=0 ; j<quarter_height ; j++)
		{                
	//		jj = RSD(j);
			p_mv_list0 = p->mv[LIST_0][j]; 
			p_mv_list1 = p->mv[LIST_1][j];
			fs_mv_list0 = (fs->mvL0 + j*mvWidth); 
			fs_mv_list1 = (fs->mvL1 + j*mvWidth);
			
			p_ref_idx_list0 = p->ref_idx[LIST_0][j];
			p_ref_idx_list1 = p->ref_idx[LIST_1][j];
			fs_ref_idx_list0 = fs->ref_idx[LIST_0][j];
			fs_ref_idx_list1 = fs->ref_idx[LIST_1][j];
			
			p_ref_pic_id_list0 = p->ref_pic_id[LIST_0][j];
			p_ref_pic_id_list1 = p->ref_pic_id[LIST_1][j];
			fs_ref_id_list0 = fs->ref_id[LIST_0][j];
			fs_ref_id_list1 = fs->ref_id[LIST_1][j];
		   
			for (i=0 ; i<quarter_width ; i++)          
			{           
		//		ii = RSD(i);
				

				p_mv_list0[i][0] = *(fs_mv_list0+(i<<1));;
				p_mv_list0[i][1] = *(fs_mv_list0+(i<<1)+1);
				p_mv_list1[i][0] = *(fs_mv_list1+(i<<1));
				p_mv_list1[i][1] = *(fs_mv_list1+(i<<1)+1);
					
				p_ref_idx_list0[i] = fs_ref_idx_list0[i];
				p_ref_idx_list1[i] = fs_ref_idx_list1[i];
					
				p_ref_pic_id_list0[i] = fs_ref_id_list0[i];
				p_ref_pic_id_list1[i] = fs_ref_id_list1[i];	
				
				if (dec_params->img->direct_spatial_mv_pred_flag == 1)
				{
			

					p->moving_block[j][i]= 
						!((!p->is_long_term 
						&& ((p_ref_idx_list0[i] == 0) 
						&&  (absz(p_mv_list0[i][0])>>1 == 0) 
						&&  (absz(p_mv_list0[i][1])>>1 == 0))) 
						|| ((p_ref_idx_list0[i] == -1) 
						&&  (p_ref_idx_list1[i] == 0) 
						&&  (absz(p_mv_list1[i][0])>>1 == 0) 
						&&  (absz(p_mv_list1[i][1])>>1 == 0)));
				}
			}
		}
	}


	// temporal
	if (dec_params->img->direct_spatial_mv_pred_flag == 0)
	{
		for (i=0; i<dec_params->listXsize[0];i++)
		{
			int prescale, iTRb, iTRp;			
			iTRb = Clip3( -128, 127, dec_params->dec_picture->poc - dec_params->listX[LIST_0 /*+ j*/][i]->poc );
			iTRp = Clip3( -128, 127,  dec_params->listX[LIST_1][0]->poc - dec_params->listX[LIST_0][i]->poc);
        
			if (iTRp!=0)
			{
				prescale = ( 16384 + absz( iTRp / 2 ) ) / iTRp;
				dec_params->img->mvscale[0][i] = Clip3( -1024, 1023, ( iTRb * prescale + 32 ) >> 6 ) ;
			}
			else
			{
				dec_params->img->mvscale[0][i] = 9999;
			}
		}
	}
}



/*!
************************************************************************
* \brief
*    Allocate co-located memory 
*
* \param size_x
*    horizontal luma size
* \param size_y
*    vertical luma size
* \param mb_adaptive_frame_field_flag
*    flag that indicates macroblock adaptive frame/field coding
*
* \return
*    the allocated StorablePicture structure
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/
ColocatedParams* alloc_colocated( h264_decoder* dec_params )
{
	
	int size_x=dec_params->img->width;
	int size_y=dec_params->img->height;
	
	ColocatedParams *s;
	
	s = (ColocatedParams *)h264_calloc(1, sizeof(ColocatedParams)); 
	if (NULL == s)
	{
		printf("alloc_colocated: s");
		exit(0);
	}
	
	s->size_x = size_x;
	s->size_y = size_y;
	
	
	get_mem3D      ((byte****)(&(s->ref_idx))   , 2, size_y / BLOCK_SIZE, size_x / BLOCK_SIZE);
	get_mem3Dint (&(s->ref_pic_id), 2, size_y / BLOCK_SIZE, size_x / BLOCK_SIZE);
	get_mem4Dshort (&(s->mv)        , 2, size_y / BLOCK_SIZE, size_x / BLOCK_SIZE,2 );
	
	get_mem2D      (&(s->moving_block), size_y / BLOCK_SIZE, size_x / BLOCK_SIZE);
	

	// changes by faisal for linear data structures
	if( (s->ref_idx_l0 = (signed char*) h264_malloc(size_y / BLOCK_SIZE * size_x / BLOCK_SIZE * sizeof(byte*))) == NULL)
	{
		printf("Colocated ref_idx_l0");
		exit(0);
	}

	if( (s->ref_idx_l1 = (signed char*) h264_malloc(size_y / BLOCK_SIZE * size_x / BLOCK_SIZE * sizeof(byte*))) == NULL)
	{
		printf("Colocated ref_idx_l1");
		exit(0);
	}

	return s;
}

/*!
************************************************************************
* \brief
*    Free co-located memory.
*
* \param p
*    Picture to be freed
*
*-----------------------------------------------------------------------			
*	 Function Argument List Changed [Removing Global Variables] 
*	 Input parameters added are 
*			- h264_decoder* dec_params
*
*		<saad.shams@inforient.com>
************************************************************************
*/

void free_colocated( h264_decoder* dec_params )
{
	ColocatedParams* p=dec_params->Co_located;
	if (p)
	{
		free_mem3D      ((byte***)p->ref_idx, 2);
		free_mem3Dint (p->ref_pic_id, 2);
		free_mem4Dshort (p->mv, 2, p->size_y / BLOCK_SIZE);
		
		//faisal
		h264_free(p->ref_idx_l0);
		h264_free(p->ref_idx_l1);
		
		if (p->moving_block)
		{
			free_mem2D (p->moving_block);
			p->moving_block=NULL;
		}
		h264_free(p);
		p=NULL;
	}
}
