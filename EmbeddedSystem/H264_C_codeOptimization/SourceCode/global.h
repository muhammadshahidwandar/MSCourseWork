/*Changed by Saad.Shams@inforient.com [Removing Global Variables]*/
/*!
 ************************************************************************
 *  \file
 *     global.h
 *  \brief
 *     global definitions for for H.264 decoder.
 *
 ************************************************************************
 */
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>                              //!< for FILE
#include <time.h>
//#include <sys/timeb.h>
#include "defines.h"
#include "parsetcommon.h"
//#include "biaridecod.h"


struct stat_par;
typedef struct DecRefPicMarking_s  DecRefPicMarking_t;
typedef struct DecRefPicMarking_s  DecRefPicMarking_s;
typedef struct macroblock Macroblock;
typedef struct Bitstream Bitstream;
typedef struct  SyntaxElement  SyntaxElement;
typedef struct datapartition DataPartition;
typedef struct Slice Slice;
typedef struct ImageParameters ImageParameters;
//typedef struct snr_par snr_par;
typedef struct inp_par InputParameters;
typedef struct pix_pos PixelPos;
typedef struct old_slice_par  OldSliceParams;
// Added for main profiole May-08-2006
typedef struct colocated_params  ColocatedParams;
typedef struct frame_store FrameStore;
typedef struct decoded_picture_buffer DecodedPictureBuffer;
typedef struct ercSegment_s ercSegment_t;
typedef struct ercVariables_s ercVariables_t;
typedef struct objectBuffer_t  objectBuffer_t;
typedef struct H264Dec_Inputs H264Dec_Inputs;

typedef struct neighboursInfo NeighboursInfo;


#ifdef WIN32
#define  snprintf _snprintf
#define  open     _open
#define  close    _close
#define  read     _read
#define  write    _write
#define  lseek    _lseeki64
#define  fsync    _commit
#define  OPENFLAGS_WRITE _O_WRONLY|_O_CREAT|_O_BINARY|_O_TRUNC
#define  OPEN_PERMISSIONS _S_IREAD | _S_IWRITE
#define  OPENFLAGS_READ  _O_RDONLY|_O_BINARY
#else
#define  OPENFLAGS_WRITE O_WRONLY|O_CREAT|O_TRUNC
#define  OPENFLAGS_READ  O_RDONLY
#define  OPEN_PERMISSIONS S_IRUSR | S_IWUSR
#endif

#define MAX_LIST_SIZE 33
typedef unsigned char   byte;                   //!<  8 bit unsigned
typedef int             int32;
typedef unsigned int    u_int32;

#define IMGPEL_CHAR
#define LIST_0 0					// added 
#define LIST_1 1					// added by Faisal Abdullah for list1

#ifdef IMGPEL_CHAR
#define imgpel unsigned char
#else
#define imgpel unsigned short
#endif


#if defined(WIN32) && !defined(__GNUC__)
  typedef __int64   int64;
#ifndef INT64_MIN
# define INT64_MIN        (-9223372036854775807i64 - 1i64)
#endif
#else
  typedef long long int64;
#ifndef INT64_MIN
# define INT64_MIN        (-9223372036854775807LL - 1LL)
#endif
#endif

#define MB_CR_SIZE_X 8
#define MB_CR_SIZE_Y 8


#define ET_SIZE 300      //!< size of error text buffer
  //!< buffer for error message for exit with error()
//char errortext[ET_SIZE];				/*Changed by Saad Bin Shams [Removing Global Variables]*/



#if !defined(WIN32) || defined(__GNUC__)
#define max(a, b)      ((a) > (b) ? (a) : (b))  //!< Macro returning max value
#define min(a, b)      ((a) < (b) ? (a) : (b))  //!< Macro returning min value
#endif

#define normalize_clip(x)                                   \
({                                                          \
    int __arg1 = (x);                                       \
    int __result = (__arg1+512)>>10;                        \
    __result;                                               \
})

#define absz(x) (((x)^((x)>>31))-((x)>>31))


#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))


/***********************************************************************
 * T y p e    d e f i n i t i o n s    f o r    T M L
 ***********************************************************************
 */

//! Data Partitioning Modes
typedef enum
{
  PAR_DP_1,    //!< no data partitioning is supported
  PAR_DP_3,    //!< data partitioning with 3 partitions
} PAR_DP_TYPE;

enum mb_class_e
{
    I_4x4           = 0,
    I_8x8           = 1,
    I_16x16         = 2,
    I_PCM           = 3,

    P_L0            = 4,
	P_8x8_ref0      = 5,
    P_8x8           = 5+1,
    P_SKIP          = 6+1,
	P_SKIP_MV_0     = 7+1,

    B_DIRECT        = 8+1,
    B_L0_L0         = 9+1,
    B_L0_L1         = 10+1,
    B_L0_BI         = 11+1,
    B_L1_L0         = 12+1,
    B_L1_L1         = 13+1,
    B_L1_BI         = 14+1,
    B_BI_L0         = 15+1,
    B_BI_L1         = 16+1,
    B_BI_BI         = 17+1,
    B_8x8           = 18+1,
    B_SKIP          = 19+1,
};

enum mb_partition_e
{
    /* sub partition type for P_8x8 and B_8x8 */
    D_L0_4x4        = 0,
    D_L0_8x4        = 1,
    D_L0_4x8        = 2,
    D_L0_8x8        = 3,

    /* sub partition type for B_8x8 only */
    D_L1_4x4        = 4,
    D_L1_8x4        = 5,
    D_L1_4x8        = 6,
    D_L1_8x8        = 7,

    D_BI_4x4        = 8,
    D_BI_8x4        = 9,
    D_BI_4x8        = 10,
    D_BI_8x8        = 11,
    D_DIRECT_8x8    = 12,

    /* partition */
    D_8x8           = 13,
    D_16x8          = 14,
    D_8x16          = 15,
    D_16x16         = 16,
};

enum pred_dir_e
{
	PRED_DIR_L0,
	PRED_DIR_L1,
	PRED_DIR_BI
};
//! Output File Types
typedef enum
{
  PAR_OF_ANNEXB,   //!< Current TML description
  PAR_OF_RTP,   //!< RTP Packet Output format
//  PAR_OF_IFF    //!< Interim File Format
} PAR_OF_TYPE;

//! Boolean Type
//typedef enum {
//  FALSE,
//  TRUE
//} Boolean;

//! definition of H.264 syntax elements
typedef enum {
  SE_HEADER,
  SE_PTYPE,
  SE_MBTYPE,
  SE_REFFRAME,
  SE_INTRAPREDMODE,
  SE_MVD,
  SE_CBP_INTRA,
  SE_LUM_DC_INTRA,
  SE_CHR_DC_INTRA,
  SE_LUM_AC_INTRA,
  SE_CHR_AC_INTRA,
  SE_CBP_INTER,
  SE_LUM_DC_INTER,
  SE_CHR_DC_INTER,
  SE_LUM_AC_INTER,
  SE_CHR_AC_INTER,
  SE_DELTA_QUANT_INTER,
  SE_DELTA_QUANT_INTRA,
  SE_BFRAME,
  SE_EOS,
  SE_MAX_ELEMENTS //!< number of maximum syntax elements, this MUST be the last one!
} SE_type;        // substituting the definitions in element.h


typedef enum {
  INTER_MB,
  INTRA_MB_4x4,
  INTRA_MB_16x16
} IntraInterDecision;

typedef enum {
  BITS_TOTAL_MB,
  BITS_HEADER_MB,
  BITS_INTER_MB,
  BITS_CBP_MB,
  BITS_COEFF_Y_MB,
  BITS_COEFF_UV_MB,
  MAX_BITCOUNTER_MB
} BitCountType;

typedef enum {
  NO_SLICES,
  FIXED_MB,
  FIXED_RATE,
//  CALLBACK,
  FMO
} SliceMode;


typedef enum {
  UVLC,
  CABAC
} SymbolMode;


//typedef enum {
// LIST_0=0,
// LIST_1=1
//} Lists;

typedef enum {
  FRAME,
  TOP_FIELD,
  BOTTOM_FIELD
} PictureStructure;           //!< New enum for field processing

typedef enum {
  P_SLICE = 0,
  B_SLICE,
  I_SLICE,
  SP_SLICE,
  SI_SLICE
} SliceType;


/**********************************************************************
* NALU DATA STRUCTURES
**********************************************************************/

typedef struct 
{
  int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  unsigned max_size;            //! Nal Unit Buffer size
  int nal_unit_type;            //! NALU_TYPE_xxxx
  int nal_reference_idc;        //! NALU_PRIORITY_xxxx
  int forbidden_bit;            //! should be always FALSE
  byte *buf;        //! contains the first byte followed by the EBSP
} NALU_t;


#define MAXRBSPSIZE 64000

#define NALU_TYPE_SLICE    1
#define NALU_TYPE_DPA      2
#define NALU_TYPE_DPB      3
#define NALU_TYPE_DPC      4
#define NALU_TYPE_IDR      5
#define NALU_TYPE_SEI      6
#define NALU_TYPE_SPS      7
#define NALU_TYPE_PPS      8
#define NALU_TYPE_AUD      9
#define NALU_TYPE_EOSEQ    10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL     12

#define NALU_PRIORITY_HIGHEST     3
#define NALU_PRIORITY_HIGH        2
#define NALU_PRIORITY_LOW         1
#define NALU_PRIORITY_DISPOSABLE  0

/**********************************************************************
* C O N T E X T S   F O R   T M L   S Y N T A X   E L E M E N T S
**********************************************************************/
/*
#define NUM_MB_TYPE_CTX  11
#define NUM_B8_TYPE_CTX  9
#define NUM_MV_RES_CTX   10
#define NUM_REF_NO_CTX   6
#define NUM_DELTA_QP_CTX 4
#define NUM_MB_AFF_CTX 4
#define NUM_TRANSFORM_SIZE_CTX 3


struct MotionInfoContexts
{
  BiContextType mb_type_contexts [4][NUM_MB_TYPE_CTX];
  BiContextType b8_type_contexts [2][NUM_B8_TYPE_CTX];
  BiContextType mv_res_contexts  [2][NUM_MV_RES_CTX]; 
  BiContextType ref_no_contexts  [2][NUM_REF_NO_CTX];
  BiContextType delta_qp_contexts[NUM_DELTA_QP_CTX];
  BiContextType mb_aff_contexts  [NUM_MB_AFF_CTX];
  BiContextType transform_size_contexts [NUM_TRANSFORM_SIZE_CTX];
};


#define NUM_IPR_CTX    2
#define NUM_CIPR_CTX   4
#define NUM_CBP_CTX    4
#define NUM_BCBP_CTX   4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX    5
#define NUM_ABS_CTX    5


struct TextureInfoContexts
{
  BiContextType  ipr_contexts [NUM_IPR_CTX];
  BiContextType  cipr_contexts[NUM_CIPR_CTX]; 
  BiContextType  cbp_contexts [3][NUM_CBP_CTX];
  BiContextType  bcbp_contexts[NUM_BLOCK_TYPES][NUM_BCBP_CTX];
  BiContextType  map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
  BiContextType  last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
  BiContextType  one_contexts [NUM_BLOCK_TYPES][NUM_ONE_CTX];
  BiContextType  abs_contexts [NUM_BLOCK_TYPES][NUM_ABS_CTX];
  BiContextType  fld_map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
  BiContextType  fld_last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
};
*/
//*********************** end of data type definition for CABAC *******************

//struct InputParameters;          /*Changed by Saad Bin Shams [Removing Global Variables]*/

/***********************************************************************
 * N e w   D a t a    t y p e s   f o r    T M L
 ***********************************************************************
 */
/*! Buffer structure for decoded referenc picture marking commands */
struct DecRefPicMarking_s
{
  int memory_management_control_operation;
  int difference_of_pic_nums_minus1;
  int long_term_pic_num;
  int long_term_frame_idx;
  int max_long_term_frame_idx_plus1;
  DecRefPicMarking_s *Next;
};

//extern struct  *input;	//changed by Saad Bin Shams [Removing Global Variables]
struct pix_pos
{
  int available;
  int mb_addr;
  int x;
  int y;
  int pos_x;
  int pos_y;
};

struct neighboursInfo
{
  //int mbAddrA, mbAddrB, mbAddrC, mbAddrD;
  int bAvailA[16], bAvailB[16], bAvailC[16], bAvailD[16]; 
  int posx;
  int posy;
  PixelPos A,B,C,D;
  //char block_A, intrapredB, intrapredC, intrapredD;
};

//! Macroblock
struct macroblock
{
    int           qp;
    //int           slice_nr;
    int           delta_quant;          //!< for rate control
    
    Macroblock   *mb_available_up;   //!< pointer to neighboring MB (CABAC)
    Macroblock   *mb_available_left; //!< pointer to neighboring MB (CABAC)
    
    // some storage of macroblock syntax elements for global access
    int           mb_type;
    int           mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];      //!< indices correspond to [forw,backw][block_y][block_x][x,y]
    
    int           cbp;
    int           cbp_blk ;
	int64         cbp_bits;
    
    int           is_skip;
    
    int           i16mode;
    int           b8mode[4];          //!< inter prediction mode for an 8x8 block
    int           b8pdir[4];
    //int           ei_flag;			//!< error indication flag
    
    int           LFDisableIdc;		//!< loop filter
    int           LFAlphaC0Offset;	//!< loop filter
    int           LFBetaOffset;		//!< loop filter
    
    int           c_ipred_mode;       //!< chroma intra prediction mode
    //int           mb_field;
    
    int           skip_flag;
//	int			  all_mv_zero;
//	int			  a_b_mv_zero;
    
    int           mbAddrA, mbAddrB, mbAddrC, mbAddrD;
    int           mbAvailA, mbAvailB, mbAvailC, mbAvailD;
    
    //int           luma_transform_size_8x8_flag;
    int           NoMbPartLessThan8x8Flag;
	// faisal for read_one_macroblock
	int			  part_pred_dir[2];
	int		   (* mb_pred_dir)[2];
	int			  sub_part_pred_dir[4];
	int			  mb_num_of_part;
	int			  partition;
	int			  sub_partition[4];
	int           mb_type_name;
	int			  mb_num_of_sub_part[4];
};

//! Bitstream
struct Bitstream
{
    // CABAC Decoding
    int           read_len;           //!< actual position in the codebuffer, CABAC only
    int           code_len;           //!< overall codebuffer length, CABAC only
    // UVLC Decoding
    int           frame_bitoffset;    //!< actual position in the codebuffer, bit-oriented, UVLC only
    int           bitstream_length;   //!< over codebuffer length, byte oriented, UVLC only
    // ErrorConcealment
    byte          *streamBuffer;      //!< actual codebuffer for read bytes
    int           ei_flag;            //!< error indication, 0: no error, else unspecified error
};

#define NUM_BLOCK_TYPES 10

/***********************************************************************
 * D a t a    t y p e s   f o r  C A B A C
 ***********************************************************************
 */

//! struct to characterize the state of the arithmetic coding engine
typedef struct
{
  //unsigned int    Dlow, Drange;
    unsigned int    Drange;
  unsigned int    Dvalue;
  unsigned int    Dbuffer;
  int             Dbits_to_go;
  byte            *Dcodestrm;
  int             *Dcodestrm_len;
} DecodingEnvironment;

typedef DecodingEnvironment *DecodingEnvironmentPtr;

//! struct for context management
typedef struct
{
  unsigned short state;         // index into state-table CP  
  unsigned char  MPS;           // Least Probable Symbol 0/1 CP
} BiContextType;

typedef BiContextType *BiContextTypePtr;


/**********************************************************************
 * C O N T E X T S   F O R   T M L   S Y N T A X   E L E M E N T S
 **********************************************************************
 */

#define NUM_MB_TYPE_CTX  11
#define NUM_B8_TYPE_CTX  9
#define NUM_MV_RES_CTX   10
#define NUM_REF_NO_CTX   6
#define NUM_DELTA_QP_CTX 4
#define NUM_MB_AFF_CTX 4
#define NUM_TRANSFORM_SIZE_CTX 3


typedef struct
{
  BiContextType mb_type_contexts [4][NUM_MB_TYPE_CTX];
  BiContextType b8_type_contexts [2][NUM_B8_TYPE_CTX];
  BiContextType mv_res_contexts  [2][NUM_MV_RES_CTX];
  BiContextType ref_no_contexts  [2][NUM_REF_NO_CTX];
  BiContextType delta_qp_contexts[NUM_DELTA_QP_CTX];
  BiContextType mb_aff_contexts  [NUM_MB_AFF_CTX];
  BiContextType transform_size_contexts [NUM_TRANSFORM_SIZE_CTX];

} MotionInfoContexts;

#define NUM_IPR_CTX    2
#define NUM_CIPR_CTX   4
#define NUM_CBP_CTX    4
#define NUM_BCBP_CTX   4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX    5
#define NUM_ABS_CTX    5


typedef struct
{
  BiContextType  ipr_contexts [NUM_IPR_CTX];
  BiContextType  cipr_contexts[NUM_CIPR_CTX]; 
  BiContextType  cbp_contexts [3][NUM_CBP_CTX];
  BiContextType  bcbp_contexts[NUM_BLOCK_TYPES][NUM_BCBP_CTX];
  BiContextType  map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
  BiContextType  last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
  BiContextType  one_contexts [NUM_BLOCK_TYPES][NUM_ONE_CTX];
  BiContextType  abs_contexts [NUM_BLOCK_TYPES][NUM_ABS_CTX];
  BiContextType  fld_map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
  BiContextType  fld_last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
} TextureInfoContexts;

typedef struct StorablePicture_s
{
  int		poc;
  int		frame_poc;
  int		pic_num;
  int		long_term_pic_num;
  int		long_term_frame_idx;
  int		is_long_term;
  int		used_for_reference;
  int		is_output;
  int		non_existing;
  short		max_slice_id;
  int		is_empty;
  int		width;
  int		height;
  int		width_cr;
  int		height_cr;

  imgpel*	imgY;			// Y picture component (padded)
  imgpel*	imgU;			// U picture component (padded)
  imgpel*	imgV;			// V picture component (padded)

  imgpel*	plane[3];		// pointing to the start of planes
  int		stride_luma;
  int		stride_chroma;	

  short*	slice_id;		// reference picture   [mb_nr]
  char***	ref_idx;		// reference picture   [list][subblock_y][subblock_x]
  int***	ref_id;			// reference picture identifier [list][subblock_y][subblock_x]
                            // (not  simply index) 
  short*	ref_idL0;		// macroblock wise reference indexes 
  short*	ref_idL1;

  char*		ref_idx_l0;
  char*		ref_idx_l1;       
  short*	mvL0;			// macroblock wise motion vectors
  short*	mvL1;

  int		slice_type;
  int		idr_flag;
  int		no_output_of_prior_pics_flag;
  int		long_term_reference_flag;
  int		adaptive_ref_pic_buffering_flag;

  int		frame_cropping_flag;
  int		frame_cropping_rect_left_offset;
  int		frame_cropping_rect_right_offset;
  int		frame_cropping_rect_top_offset;
  int		frame_cropping_rect_bottom_offset;

  int		chroma_qp_offset[2];
  
  PictureStructure		structure;
  DecRefPicMarking_t*	dec_ref_pic_marking_buffer;	// stores the memory management control 
													// operations
} StorablePicture;

struct old_slice_par
{
   unsigned frame_num;
   int nal_ref_idc;
   unsigned pic_oder_cnt_lsb;
   int delta_pic_oder_cnt_bottom;
   int delta_pic_order_cnt[2];
   int idr_flag;
   int idr_pic_id;
   int pps_id;
};

//! Frame Stores for Decoded Picture Buffer
struct frame_store
{
  int       is_used;                //!< 0=empty; 1=top; 2=bottom; 3=both fields (or frame)
  int       is_reference;           //!< 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       is_long_term;           //!< 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used
  int       is_orig_reference;      //!< original marking by nal_ref_idc: 0=not used for ref; 1=top used; 2=bottom used; 3=both fields (or frame) used

  int       is_non_existent;

  unsigned  frame_num;
  int       frame_num_wrap;
  int       long_term_frame_idx;
  int       is_output;
  int       poc;

  StorablePicture *frame;
};


//! Decoded Picture Buffer
struct decoded_picture_buffer
{
  FrameStore  **fs;
  FrameStore  **fs_ref;
  FrameStore  **fs_ltref;
  unsigned      size;
  unsigned      used_size;
  unsigned      ref_frames_in_buffer;
  unsigned      ltref_frames_in_buffer;
  int           last_output_poc;
  int           max_long_term_pic_idx;
  int           init_done;
  int           num_ref_frames;
  FrameStore   *last_picture;
};

typedef struct h264_decoder_s
{
	FILE*						bits_Annexb;		// Annexb bitstream file
	FILE*						f_out;				// file pointer to output YUV file
	int							p_out;				// file descriptor to output YUV file
	StorablePicture*			dec_picture;
	StorablePicture**			dec_pictures_list;
	unsigned int				picture_offset;
	DecodedPictureBuffer		dpb;
	
    ColocatedParams				*Co_located;
	pic_parameter_set_rbsp_t	*active_pps;
	pic_parameter_set_rbsp_t	*pps;
	seq_parameter_set_rbsp_t	*active_sps;
	seq_parameter_set_rbsp_t	*sps;
	NALU_t						*nalu;
	DataPartition				*dp;
	int							Bframe_ctr;
	ercVariables_t				*erc_errorVar;
	ImageParameters				*erc_img;
	int							erc_mvperMB;
	objectBuffer_t				*erc_object_list;
	int							frame_no;
	int							global_init_done;
	ImageParameters				*img;
	InputParameters				*input;				//!< input parameters from input configuration file
	int							IsFirstByteStreamNALU;
	int							last_dquant;
	int							LastAccessUnitExists;
	
	StorablePicture				**listX[6];
	
	int							listXsize[6];

	int							*MapUnitToSliceGroupMap; 
	int							*MbToSliceGroupMap;
	int							NALUCount;
	OldSliceParams				old_slice;

#if TRACE
	FILE						*p_trace;
#endif
	
	pic_parameter_set_rbsp_t 	PicParSet[MAXPPS];
	seq_parameter_set_rbsp_t	SeqParSet[MAXSPS];

	int							symbolCount;
	int							tot_time;
	int							UsedBits;      // for internal statistics, is adjusted by se_v, ue_v, u_1

	/*Statics*/	
	int ec_flag[SE_MAX_ELEMENTS];        //!< array to set errorconcealment
	int bitcounter;
	int NumberOfSliceGroups;    // the number of slice groups -1 (0 == scan order, 7 == maximum)
    MotionInfoContexts  *mot_ctx;      //!< pointer to struct of context models for use in CABAC
    TextureInfoContexts *tex_ctx;      //!< pointer to struct of context models for use in CABAC
	char						errortext[ET_SIZE]; //!< buffer for error message for exit with error()
} h264_decoder;

//! DataPartition
struct datapartition 
{
  Bitstream           *bitstream;
  DecodingEnvironment de_cabac;
  int     (*readSyntaxElement)(SyntaxElement *se, DataPartition *this_dataPart);
};

//! Slice
struct Slice
{
  int                 picture_type;  //!< picture type
  int                 start_mb_nr;   //!< MUST be set by NAL even in case of ei_flag == 1
  int                 max_part_nr;
  int                 next_header;
  DataPartition       *partArr;      //!< array of partitions
  
  int                 ref_pic_list_reordering_flag_l0;
  int                 *remapping_of_pic_nums_idc_l0;
  int                 *abs_diff_pic_num_minus1_l0;
  int                 *long_term_pic_idx_l0;
  int                 ref_pic_list_reordering_flag_l1;
  int                 *remapping_of_pic_nums_idc_l1;
  int                 *abs_diff_pic_num_minus1_l1;
  int                 *long_term_pic_idx_l1;
  int                 LFDisableIdc;     //!< Disable loop filter on slice
  int                 LFAlphaC0Offset;  //!< Alpha and C0 offset for filtering slice
  int                 LFBetaOffset;     //!< Beta offset for filtering slice
  int                 pic_parameter_set_id;   //!<the ID of the picture parameter set the slice is reffering to
};

//****************************** ~DM ***********************************

//img->dc_pred_value with DC_PRED_VALUE 
#define DC_PRED_VALUE 128
#define MAX_IMGPEL_VALUE 255
// image parameters
struct ImageParameters
{
	int number;									// frame number
	unsigned current_mb_nr;						// bitstream order
	unsigned num_dec_mb;
	int current_slice_nr;
	int *intra_block;
	int qp;                                     // quant for the current frame
	int direct_spatial_mv_pred_flag;            // 1 for Spatial Direct, 0 for Temporal
	int type;                                   // image type INTER/INTRA
	int width;
	int height;
	int width_cr;								// width chroma
	int height_cr;								// height chroma
	int mb_y;
	int mb_x;
	
//	int allrefzero;
	int mvscale[6][MAX_REFERENCE_PICTURES];
	
	imgpel	mpr1[3][16][16];						// predicted block
	short	cof_s[16+4+4][4][4];					// correction coefficients from predicted   
	short	dc_cof_luma[4][4];					// dc coefficients for luma I_16x16
	short	dc_cof_chroma[2][4];					// dc coefficients for chroma
	char	run_idct[24];
	
	void (*p_add4x4_idct)(imgpel* dst, unsigned int dst_stride, short dct[4][4]);  
	void (*p_idct4x4dc)(short d[4][4]);  
	
	//char **ipredmode;							// prediction type [90][74]
	unsigned char **ipredmode;							// prediction type [90][74]
	
	unsigned char  *nz_coeff;
	char* nz_coeff1;

	int cod_counter;							// current count of number of skipped macroblocks
	
	//B pictures
	Slice		*currentSlice;					// pointer to current Slice data struct
	Macroblock	*mb_data;						// array containing all MBs of a whole frame
	char *slice_nr; 
    char *ei_flag;
	
	// for signalling to the neighbour logic that this is a deblocker call
	int DeblockCall;
	
	DecRefPicMarking_t *dec_ref_pic_marking_buffer;                    //!< stores the memory management control operations
	
	int disposable_flag;                   //!< flag for disposable frame, 1:disposable
	int num_ref_idx_l0_active;             //!< number of forward reference
	
	int num_ref_idx_l1_active;             //!< number of backward reference
	
	int slice_group_change_cycle;
	unsigned int pre_frame_num;           //!< store the frame_num in the last decoded slice. For detecting gap in frame_num.
	
	int toppoc;      //poc for this top field // POC200301
	int bottompoc;   //poc of bottom field of frame
	int framepoc;    //poc of this frame // POC200301
	unsigned int frame_num;   //frame_num for this frame
	unsigned int pic_order_cnt_lsb;
	int delta_pic_order_cnt_bottom;
	
	int delta_pic_order_cnt[3];
	
    signed int PrevPicOrderCntMsb;
	unsigned int PrevPicOrderCntLsb;
    signed int PicOrderCntMsb;
	
	unsigned int AbsFrameNum;
    signed int ExpectedPicOrderCnt, PicOrderCntCycleCnt, FrameNumInPicOrderCntCycle;
	unsigned int PreviousFrameNum, FrameNumOffset;
	int ExpectedDeltaPerPicOrderCntCycle;
	int PreviousPOC, ThisPOC;
	int PreviousFrameNumOffset;

	unsigned int luma_log2_weight_denom;
	unsigned int chroma_log2_weight_denom;
	int ***wp_weight;  // weight in [list][index][component] order
	int ***wp_offset;  // offset in [list][index][component] order
	int ****wbp_weight; //weight in [list][fw_index][bw_index][component] order
	int wp_round_luma;
	int wp_round_chroma;
	unsigned int apply_weights;
	unsigned int apply_weights_luma;
	unsigned int apply_weights_chr;
	unsigned int apply_weights_bi;
	
	int idr_flag;
	int nal_reference_idc;                       //!< nal_reference_idc from NAL unit
	int idr_pic_id;
	int MaxFrameNum;
	unsigned FrameWidthInMbs;
	unsigned FrameHeightInMbs;
	unsigned FrameSizeInMbs;
	
	int no_output_of_prior_pics_flag;
	int long_term_reference_flag;
	int adaptive_ref_pic_buffering_flag;
	int last_has_mmco_5;
	
	int model_number;
	int profile_idc;
	
	time_t ltime_start;               // for time measurement
	time_t ltime_end;                 // for time measurement
	
	int errorConcealmentFlag;
	
/*#ifdef WIN32
	struct _timeb tstruct_start;
	struct _timeb tstruct_end;
#else
	struct timeb tstruct_start;
	struct timeb tstruct_end;
#endif*/

	// data for current mb processing
	struct {
		char		mb_type;
		char		partition;
		char		mb_num_of_part;
		char		part_pred_dir[2];
		char		(* mb_pred_dir)[2];
		char		sub_partition[4];
		char		sub_part_pred_dir[4];
		char		mb_num_of_sub_part[4];
		short		cbp;
		short		cbp_blk ;
		char		qp;
		char		i16mode;
		char		c_ipred_mode;       // chroma intra prediction mode
		char		NoMbPartLessThan8x8Flag;
		short		mbAddrA, mbAddrB, mbAddrC, mbAddrD;
		char		mbAvailA, mbAvailB, mbAvailC, mbAvailD;
		char		LFDisableIdc;		// loop filter
		char		LFAlphaC0Offset;	// loop filter
		char		LFBetaOffset;		// loop filter
	} currMB;

	// data for macroblocks of the whole frame
	struct  {
		char*	mb_type;
		char*	partition;
		char*	qp;
		short*	cbp_blk;
		char*	mbAvailA;
		char*	mbAvailB;
		char*	NoMbPartLessThan8x8Flag;
		char*   LFDisableIdc;		// loop filter
		char*   LFAlphaC0Offset;	// loop filter
		char*   LFBetaOffset;		// loop filter
	} mb_data1;
};


// input parameters from configuration file
struct inp_par
{
  char infile[100];                       //!< H.264 inputfile
  char outfile[100];                      //!< Decoded YUV 4:2:0 output
  int FileFormat;                         //!< File format of the Input file, PAR_OF_ANNEXB or PAR_OF_RTP
  int poc_scale;
};



//! definition a picture (field or frame)

struct colocated_params
{
   int         size_x, size_y;
   int       ref_pic_num[6][MAX_LIST_SIZE];  

/********************** Linear memory Changes by Faisal *************************/
	signed char  *    ref_idx_l0;
	signed char  *    ref_idx_l1;       
  

/********************************************************************************/

  signed char  ***   ref_idx;       //!< reference picture   [list][subblock_y][subblock_x]
  int ***   ref_pic_id;    //!< reference picture identifier [list][subblock_y][subblock_x]

  short *	ref_pic_idL0;
  short *	ref_pic_idL1;

  short ****  mv;            //!< motion vector       [list][subblock_y][subblock_x][component]  
  byte  **    moving_block;

  byte        is_long_term;
};




/* segment data structure */
struct ercSegment_s
{
  int      startMBPos;
  int      endMBPos;
  int      fCorrupted;
};

/* Error detector & concealment instance data structure */
struct ercVariables_s
{
  /*  Number of macroblocks (size or size/4 of the arrays) */
  int   nOfMBs;
  /* Number of segments (slices) in frame */
  int     nOfSegments;
  
  /*  Array for conditions of Y blocks */
  int     *yCondition;
  /*  Array for conditions of U blocks */
  int     *uCondition;
  /*  Array for conditions of V blocks */
  int     *vCondition;
  
  /* Array for Slice level information */
  ercSegment_t *segments;
  int     currSegment;
  
  /* Conditions of the MBs of the previous frame */
  int   *prevFrameYCondition;
  
  /* Flag telling if the current segment was found to be corrupted */
  int   currSegmentCorrupted;
  /* Counter for corrupted segments per picture */
  int   nOfCorruptedSegments;
  
  /* State variables for error detector and concealer */
  int   concealment;
  
};

/* region structure stores information about a region that is needed for concealment */
struct objectBuffer_t
{
  byte regionMode;  /* region mode as above */
  int xMin;         /* X coordinate of the pixel position of the top-left corner of the region */
  int yMin;         /* Y coordinate of the pixel position of the top-left corner of the region */
  int32 mv[3];      /* motion vectors in 1/4 pixel units: mvx = mv[0], mvy = mv[1], 
                              and ref_frame = mv[2] */
};

//! Syntaxelement
struct SyntaxElement
{
  int           type;                  //!< type of syntax element for data part.
  int           value1;                //!< numerical value of syntax element
  int           value2;                //!< for blocked symbols, e.g. run/level
  int           len;                   //!< length of code
  int           inf;                   //!< info part of UVLC code
  unsigned int  bitpattern;            //!< UVLC bitpattern
int           context;               //!< CABAC context
int           k;                     //!< CABAC context for coeff_count,uv



  //! for mapping of UVLC to syntaxElement
 /*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - struct h264_decoder*
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
  ///??? remove the function pointer
  void    (*mapping)(int len, int info, int *value1, int *value2,h264_decoder*);
 //! used for CABAC: refers to actual coding method of each individual syntax element type
 /*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - struct h264_decoder*
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
  void  (*reading)(SyntaxElement *, h264_decoder *, DecodingEnvironmentPtr);
};

// prototypes

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void init_conf( InputParameters *inp, char *config_filename,  h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void report(InputParameters *inp, ImageParameters *img, h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void init(ImageParameters *img,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void malloc_slice(InputParameters *inp, ImageParameters *img,h264_decoder* dec_params );
void free_slice(InputParameters *inp,ImageParameters *img);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int decode_one_frame( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void init_picture( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void exit_picture(  h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int read_new_slice(  h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void start_macroblock( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  read_one_macroblock( h264_decoder *dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void read_ipred_modes(h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  decode_one_macroblock( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  exit_macroblock(int eos_bit,  h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void decode_ipcm_mb(h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void readMotionInfoFromNAL( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void readCBPandCoeffsFromNAL( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void readIPCMcoeffsFromNAL(DataPartition *dP, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void readLumaCoeff8x8_CABAC (ImageParameters *img,InputParameters *inp, int b8,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void copyblock_sp(ImageParameters *img,int block_x,int block_y);
void itrans_sp_chroma(ImageParameters *img,int ll);
// Additional parameters added for formation of image in IDCT functions
void itrans_baseline_t(ImageParameters *img,int ioff,int joff,int i0,int j0,int chroma,int j4,int i4,int uv,StorablePicture* dec_picture);
void itrans_baseline_t_CABAC(ImageParameters *img,int ioff,int joff,int i0,int j0,int chroma,int j4,int i4,int uv,StorablePicture* dec_picture);

int  sign(int a , int b);

// NAL functions TML/CABAC bitstream
int  uvlc_startcode_follows(int dummy, h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  cabac_startcode_follows(int eos_bit, h264_decoder* dec_params);
void free_Partition(Bitstream *currStream);

// ErrorConcealment
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void reset_ec_flags(h264_decoder*dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void error(char *text, int code,h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int  is_new_picture(h264_decoder* dec_params);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void init_old_slice(h264_decoder* dec_params);

// dynamic mem allocation
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int init_global_buffers_baseline( h264_decoder* dec_params );
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void free_global_buffers_baseline( h264_decoder* dec_params );

void frame_postprocessing(ImageParameters *img,InputParameters *inp);
void field_postprocessing(ImageParameters *img,InputParameters *inp);
int  bottom_field_picture(ImageParameters *img,InputParameters *inp);

int RBSPtoSODB(byte *streamBuffer, int last_byte_pos);
int EBSPtoRBSP(byte *streamBuffer, int end_bytepos, int begin_bytepos);

// For MB level frame/field coding
void init_super_macroblock(ImageParameters *img,InputParameters *inp);
void exit_super_macroblock(ImageParameters *img,InputParameters *inp);
int  decode_super_macroblock(ImageParameters *img,InputParameters *inp);
void decode_one_Copy_topMB(ImageParameters *img,InputParameters *inp);

void SetOneRefMV(ImageParameters* img);
/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
int peekSyntaxElement_UVLC(SyntaxElement *sym, ImageParameters *img, InputParameters *inp, DataPartition *dP,h264_decoder *dec_params);

/*-----------------------------------------------------------------------			
 *	 Function Argument List Changed [Removing Global Variables] 
 *   Argument(s) Added: 
 *					   - h264_decoder* dec_params,
 *   <saad.shams@inforient.com> 
 *-----------------------------------------------------------------------*/			
void fill_wp_params(ImageParameters *img, h264_decoder* dec_params);
void reset_wp_params(ImageParameters *img);
void FreePartition (DataPartition *dp, int n);

DataPartition *AllocPartition(int n,  h264_decoder* dec_params,int partitionSize);
void tracebits2(const char *trace_str, int len, int info);
void init_decoding_engine_IPCM(ImageParameters *img);
void readIPCMBytes_CABAC(SyntaxElement *sym, Bitstream *currStream);

//NALU  Functions
NALU_t *AllocNALU(int buffersize,  h264_decoder* dec_params);
void FreeNALU(NALU_t *n);

#endif
