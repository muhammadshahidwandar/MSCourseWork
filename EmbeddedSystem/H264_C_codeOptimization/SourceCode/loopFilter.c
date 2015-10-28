 
/*!
 *************************************************************************************
 * \file loopFilter.c
 *
 * \brief
 *    Filter to reduce blocking artifacts on a macroblock level.
 *    The filter strength is QP dependent.
 *
 *************************************************************************************
 */
#include <stdlib.h>
#include <string.h>
//#include <assert.h>
#include "global.h"
#include "image.h"
//#include "mb_access.h"
#include "loopfilter.h"

extern const byte QP_SCALE_CR[52] ;

/* Deblocking filter */

//byte ALPHA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0, 
//								0,0,0,0,4,4,5,6,  
//								7,8,9,10,12,13,15,17,  
//								20,22,25,28,32,36,40,45,  
//								50,56,63,71,80,90,101,113,  
//								127,144,162,182,203,226,255,255
//} ;

static const int ALPHA_TABLE[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
    255, 255
};

//byte  BETA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0, 
//								0,0,0,0,2,2,2,3,  
//								3,3,3, 4, 4, 4, 6, 6,   
//								7, 7, 8, 8, 9, 9,10,10,  
//								11,11,12,12,13,13, 14, 14,   
//								15, 15, 16, 16, 17, 17, 18, 18} ;

static const int BETA_TABLE[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18
};

// coulmn one for all the elements is zero
// coulmn 4 and 5 for individual elements are same

//byte CLIP_TAB[52][5]  =
//{
//  { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
//  { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
//  { 0, 0, 0, 0, 0},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 1, 1, 1, 1},
//  { 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 2, 3, 3},
//  { 0, 1, 2, 3, 3},{ 0, 2, 2, 3, 3},{ 0, 2, 2, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 3, 3, 5, 5},{ 0, 3, 4, 6, 6},{ 0, 3, 4, 6, 6},
//  { 0, 4, 5, 7, 7},{ 0, 4, 5, 8, 8},{ 0, 4, 6, 9, 9},{ 0, 5, 7,10,10},{ 0, 6, 8,11,11},{ 0, 6, 8,13,13},{ 0, 7,10,14,14},{ 0, 8,11,16,16},
//  { 0, 9,12,18,18},{ 0,10,13,20,20},{ 0,11,15,23,23},{ 0,13,17,25,25}
//} ;

static const int CLIP_TAB[52][3] =
{
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 }
};



static __inline int IClip_h264( int v, int i_min, int i_max )
{
    return ( (v < i_min) ? i_min : (v > i_max) ? i_max : v );
}


static __inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}




static __inline void deblock_luma_c(byte *pix, int xstride, int ystride, int alpha, int beta, char *tc0 )
{
    int i, d;
    for( i = 0; i < 4; i++ ) {
        if( tc0[i] < 0 ) {
            pix += 4*ystride;
            continue;
        }
        for( d = 0; d < 4; d++ ) {
            const int p2 = pix[-3*xstride];
            const int p1 = pix[-2*xstride];
            const int p0 = pix[-1*xstride];
            const int q0 = pix[ 0*xstride];
            const int q1 = pix[ 1*xstride];
            const int q2 = pix[ 2*xstride];
   
            if( absz( p0 - q0 ) < alpha &&
                absz( p1 - p0 ) < beta &&
                absz( q1 - q0 ) < beta ) {
   
                int tc = tc0[i];
                int delta;
   
                if( absz( p2 - p0 ) < beta ) {
                    pix[-2*xstride] = p1 + IClip_h264( (( p2 + ((p0 + q0 + 1) >> 1)) >> 1) - p1, -tc0[i], tc0[i] );
                    tc++; 
                }
                if( absz( q2 - q0 ) < beta ) {
                    pix[ 1*xstride] = q1 + IClip_h264( (( q2 + ((p0 + q0 + 1) >> 1)) >> 1) - q1, -tc0[i], tc0[i] );
                    tc++;
                }
    
                delta = IClip_h264( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                pix[-1*xstride] = clip_uint8( p0 + delta );    /* p0' */
                pix[ 0*xstride] = clip_uint8( q0 - delta );    /* q0' */
            }
            pix += ystride;
        }
    }
}



static void deblock_v_luma_c( byte *pix, int stride, int alpha, int beta, char *tc0 )
{
    deblock_luma_c( pix, stride, 1, alpha, beta, tc0 ); 
}
static void deblock_h_luma_c( byte *pix, int stride, int alpha, int beta, char *tc0 )
{
    deblock_luma_c( pix, 1, stride, alpha, beta, tc0 );
}



static __inline void deblock_chroma_c( byte *pix, int xstride, int ystride, int alpha, int beta, char *tc0 )
{
    int i, d;
    for( i = 0; i < 4; i++ ) {
        const int tc = tc0[i];
        if( tc <= 0 ) {
            pix += 2*ystride;
            continue;
        }
        for( d = 0; d < 2; d++ ) {
            const int p1 = pix[-2*xstride];
            const int p0 = pix[-1*xstride];
            const int q0 = pix[ 0*xstride];
            const int q1 = pix[ 1*xstride];

            if( abs( p0 - q0 ) < alpha &&
                abs( p1 - p0 ) < beta &&
                abs( q1 - q0 ) < beta ) {

                int delta = IClip_h264( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                pix[-1*xstride] = clip_uint8( p0 + delta );    /* p0' */
                pix[ 0*xstride] = clip_uint8( q0 - delta );    /* q0' */
            }
            pix += ystride;
        }
    }
}
static void deblock_v_chroma_c( byte *pix, int stride, int alpha, int beta, char *tc0 )
{   
    deblock_chroma_c( pix, stride, 1, alpha, beta, tc0 );
}
static void deblock_h_chroma_c( byte *pix, int stride, int alpha, int beta, char *tc0 )
{   
    deblock_chroma_c( pix, 1, stride, alpha, beta, tc0 );
}



static __inline void deblock_luma_intra_c( byte *pix, int xstride, int ystride, int alpha, int beta )
{
    int d;
    for( d = 0; d < 16; d++ ) {
        const int p2 = pix[-3*xstride];
        const int p1 = pix[-2*xstride];
        const int p0 = pix[-1*xstride];
        const int q0 = pix[ 0*xstride];
        const int q1 = pix[ 1*xstride];
        const int q2 = pix[ 2*xstride];

        if( abs( p0 - q0 ) < alpha &&
            abs( p1 - p0 ) < beta &&
            abs( q1 - q0 ) < beta ) {

            if(abs( p0 - q0 ) < ((alpha >> 2) + 2) ){
                if( abs( p2 - p0 ) < beta)
                {
                    const int p3 = pix[-4*xstride];
                    /* p0', p1', p2' */
                    pix[-1*xstride] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                    pix[-2*xstride] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                    pix[-3*xstride] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                } else {
                    /* p0' */
                    pix[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                }
                if( abs( q2 - q0 ) < beta)
                {
                    const int q3 = pix[3*xstride];
                    /* q0', q1', q2' */
                    pix[0*xstride] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                    pix[1*xstride] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                    pix[2*xstride] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                } else {
                    /* q0' */
                    pix[0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                }
            }else{
                /* p0', q0' */
                pix[-1*xstride] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                pix[ 0*xstride] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
            }
        }
        pix += ystride;
    }
}
static void deblock_v_luma_intra_c( byte *pix, int stride, int alpha, int beta )
{   
    deblock_luma_intra_c( pix, stride, 1, alpha, beta );
}
static void deblock_h_luma_intra_c( byte *pix, int stride, int alpha, int beta )
{   
    deblock_luma_intra_c( pix, 1, stride, alpha, beta );
}



static __inline void deblock_chroma_intra_c( byte *pix, int xstride, int ystride, int alpha, int beta )
{   
    int d; 
    for( d = 0; d < 8; d++ ) {
        const int p1 = pix[-2*xstride];
        const int p0 = pix[-1*xstride];
        const int q0 = pix[ 0*xstride];
        const int q1 = pix[ 1*xstride];

        if( abs( p0 - q0 ) < alpha &&
            abs( p1 - p0 ) < beta &&
            abs( q1 - q0 ) < beta ) {

            pix[-1*xstride] = (2*p1 + p0 + q1 + 2) >> 2;   /* p0' */
            pix[ 0*xstride] = (2*q1 + q0 + p1 + 2) >> 2;   /* q0' */
        }

        pix += ystride;
    }
}
static void deblock_v_chroma_intra_c( byte *pix, int stride, int alpha, int beta )
{   
    deblock_chroma_intra_c( pix, stride, 1, alpha, beta );
}
static void deblock_h_chroma_intra_c( byte *pix, int stride, int alpha, int beta )
{   
    deblock_chroma_intra_c( pix, 1, stride, alpha, beta );
}

					 

static __inline void deblock_edge( LfilterCoefs * fCoefs, byte *pix, int i_stride, int bS[4], int i_qp, int b_chroma,
                                 x264_deblock_inter_t pf_inter, x264_deblock_intra_t pf_intra )
								 
{
    int i;
    const int index_a = IClip_h264( i_qp + fCoefs->AlphaC0_Offset , 0, 51 );
    const int alpha = ALPHA_TABLE[index_a];
    const int beta  = BETA_TABLE[IClip_h264( i_qp + fCoefs->Beta_Offset, 0, 51 )];

    if( bS[0] < 4 ) 
	{
        char tc[4]; 
        for(i=0; i<4; i++)
		{
            tc[i] = (bS[i] ? CLIP_TAB[index_a][bS[i] - 1] : -1) + b_chroma;
		}
        pf_inter( pix, i_stride, alpha, beta, tc );
    } 
	else 
	{
        pf_intra( pix, i_stride, alpha, beta );
    }
}

void DeblockPicture( h264_decoder* dec_params )
{
	ImageParameters* img = dec_params->img;
	const int fwidth_mb = img->FrameWidthInMbs; 
    const int s8x8 = 2 * fwidth_mb; // image width in multiples of 8
    const int s4x4 = 4 * fwidth_mb; // image width in multiples of 4
	const int cbpVmask = 0x3333;	// mask for vertical cbp
	const int cbpHmask = 0x00ff;	// mask for horizontal cbp

    int mb_y, mb_x;
	int filterLeftMbEdgeFlag;		
	int	filterTopMbEdgeFlag;
	unsigned int curr_mb_is_intra;  
	int	         curr_mb_cbp_blk;
	unsigned int NoMbPartLessThan8x8Flag;
	unsigned int curr_mb_type;
	unsigned int mv_both_mbs_zero;
	
	unsigned int mvWidth = img->width>>1;

	//short* ref_pic_idL0 = dec_outputs->dec_picture->ref_pic_idL0;
	//short* ref_pic_idL1 = dec_outputs->dec_picture->ref_pic_idL1;

	char* ref_idx_l0 = dec_params->dec_picture->ref_idx_l0;
	char* ref_idx_l1 = dec_params->dec_picture->ref_idx_l1;

	// Loop over number of macroblocks in a column
    for( mb_y = 0, mb_x = 0; mb_y < img->FrameHeightInMbs; )
    {

        const int mb_xy  = mb_y * fwidth_mb + mb_x;		// current macroblock
        const int mb_8x8 = 2 * s8x8 * mb_y + 2 * mb_x;			// current 8x8 block
        const int mb_4x4 = 4 * s4x4 * mb_y + 4 * mb_x;			// current 4x4 block
		//const int i_edge_end = (h->mb.type[mb_xy] == P_SKIP) ? 1 : 4;
		//const int i_edge_end = (dec_params->img->mb_data[mb_xy].mb_type == 0) ? 1 : 4;	// if MB is P_SKIP , only filter MB-edge
		//const int i_edge_end = ((dec_params->img->mb_data[mb_xy].mb_type == 0) || ((dec_params->img->mb_data[mb_xy].mb_type == 1) && ( (dec_params->img->mb_data[mb_xy ].cbp_blk & 0xffff) == 0))) ? 1 : 4;	// if MB is P_SKIP , only filter MB-edge
		const int i_edge_end = ( img->mb_data1.partition[mb_xy] == D_16x16  &&  (img->mb_data1.cbp_blk[mb_xy ] & 0xffff) == 0) ? 1 : 4;	// if MB is P_SKIP , only filter MB-edge
		int i_edge, i_dir;	

		
		//if(mb_xy == 21)//if(mb_y == 1 && mb_x == 21)
		//	mb_y = mb_y;
		
		// Check if Loop Filter is disabled
		if (img->mb_data1.LFDisableIdc[mb_xy] != 1)
		{
			int bS[4];  /* filtering strength */
			LfilterCoefs LfCoefs;

			curr_mb_is_intra        = (img->mb_data1.mb_type[mb_xy] == I_4x4) ||
									  (img->mb_data1.mb_type[mb_xy] == I_16x16) ||
									  (img->mb_data1.mb_type[mb_xy] == I_PCM);
			NoMbPartLessThan8x8Flag = img->mb_data1.NoMbPartLessThan8x8Flag[mb_xy];
			curr_mb_cbp_blk		    = img->mb_data1.cbp_blk[mb_xy];
			curr_mb_type			= img->mb_data1.mb_type[mb_xy];
			filterLeftMbEdgeFlag    = img->mb_data1.mbAvailA[mb_xy];
			filterTopMbEdgeFlag     = img->mb_data1.mbAvailB[mb_xy];
			LfCoefs.AlphaC0_Offset  = img->mb_data1.LFAlphaC0Offset[mb_xy]; 
			LfCoefs.Beta_Offset     = img->mb_data1.LFBetaOffset[mb_xy];
			

			// If P_SKIP MB , do not run loop Filter
			//if (! (img->mb_data[mb_xy].a_b_mv_zero == 1 && img->mb_data[mb_xy].mb_type == 0)) 
			{
	       /* i_dir == 0 -> vertical edge
			* i_dir == 1 -> horizontal edge */
			for( i_dir = 0; i_dir < 2; i_dir++ )
			{
				//int i_start = (i_dir ? mb_y : mb_x) ? 0 : 1;
				int i_start = (img->mb_data1.LFDisableIdc[mb_xy] ? (i_dir ? filterTopMbEdgeFlag : filterLeftMbEdgeFlag ) : (i_dir ? mb_y : mb_x) ) ? 0 : 1;
				//int i_start = (i_dir ? mb_y : mb_x) ? 0 : 1;
				int i_qp, i_qpn;
				
				int neighbor_mb = i_dir ? (mb_y ? mb_xy-fwidth_mb : mb_xy ) : (mb_x ? mb_xy-1 : mb_xy) ;
				mv_both_mbs_zero = (img->mb_data1.mb_type[mb_xy] == P_SKIP_MV_0) &  (img->mb_data1.mb_type[neighbor_mb] == P_SKIP_MV_0);
				i_start = mv_both_mbs_zero ? 1 : i_start ;			// if P_SKIP macroblock edge , skip it 
				
				for( i_edge = i_start; i_edge < i_edge_end; i_edge++ )
				{
					int mbn_xy, mbn_8x8, mbn_4x4;
					
					// FILTER STRENGTH COMPUTATIONS
					
					mbn_xy  = i_edge > 0 ? mb_xy  : ( i_dir == 0 ? mb_xy  - 1 : mb_xy  - fwidth_mb ); // neighbor macroblock number
					mbn_8x8 = i_edge > 0 ? mb_8x8 : ( i_dir == 0 ? mb_8x8 - 2 : mb_8x8 - 2 * s8x8  );		 // neighbor 8x8 block 
					mbn_4x4 = i_edge > 0 ? mb_4x4 : ( i_dir == 0 ? mb_4x4 - 4 : mb_4x4 - 4 * s4x4  );		 // neighbor 4x4 block
					
					/* *** Get bS for each 4px for the current edge *** */

					if(  curr_mb_is_intra || IS_INTRA_2( img->mb_data1.mb_type[mbn_xy] )  )  
					{
						bS[0] = bS[1] = bS[2] = bS[3] = ( i_edge == 0 ? 4 : 3 );
					}
					else
					{
						int i;
						for( i = 0; i < 4; i++ )
						{
							int x  = i_dir == 0 ? i_edge : i;
							int y  = i_dir == 0 ? i      : i_edge;
							int xn = (x - (i_dir == 0 ? 1 : 0 ))&0x03;		// neighbor x
							int yn = (y - (i_dir == 0 ? 0 : 1 ))&0x03;		// neighbor y
							int maskQ = (1<<((y  << 2) + x ));
							int maskP = (1<<((yn << 2) + xn));
							
							if( ( curr_mb_cbp_blk & maskQ) != 0  || 
								(img->mb_data1.cbp_blk[mbn_xy] & maskP) != 0 )
							{
								bS[i] = 2;
							}
							else
							{
                            /* FIXME: A given frame may occupy more than one position in
							* the reference list. So we should compare the frame numbers,
							* not the indices in the ref list.
								* No harm yet, as we don't generate that case.*/
								
								if ( ( NoMbPartLessThan8x8Flag == 1) && ( (i_edge == 1) || (i_edge == 3) ) )
								{
									bS[i] = 0;
								}
								else
								{
								int  blk_x  = (mb_x << 2) + x;
								int  blk_y  = (mb_y << 2) + y;
								int  blk_x2 = ((i_edge > 0 ? mb_x : ( i_dir == 0 ? mb_x - 1 : mb_x )) << 2)+ xn;
								int  blk_y2 = ((i_edge > 0 ? mb_y : ( i_dir == 0 ? mb_y : mb_y - 1 )) << 2)+ yn;
								
								int  mb_nr  = (mb_y * img->FrameWidthInMbs) + mb_x;
								int  mb_nr2 = ((blk_y2>>2) * img->FrameWidthInMbs) + (blk_x2>>2);


								int  x2 = blk_x2 % 4;
								int  y2 = blk_y2 % 4;
								
								int l;
								
								bS[i] = 0;
								
								/////////////////
								//img->currentSlice->picture_type
								for( l = 0; l < 1 + (img->currentSlice->picture_type==B_SLICE); l++ )
								{
									int ref_frame_0;
									int ref_frame_1;
									short  *tmp_mv;
									int add_1, add_2;

									add_1 = (y <<2) + x;
									add_2 = (y2<<2) + x2;

									if(l)
									{
										//ref_frame_0 = *(ref_pic_idL1 + (mb_nr <<4) + add_1 );
										//ref_frame_1 = *(ref_pic_idL1 + (mb_nr2<<4) + add_2 );
										ref_frame_0 = *(ref_idx_l1 + (mb_nr <<4) + add_1 );
										ref_frame_1 = *(ref_idx_l1 + (mb_nr2<<4) + add_2);
										tmp_mv = dec_params->dec_picture->mvL1;
									}
									else
									{
										//ref_frame_0 = *(ref_pic_idL0 + (mb_nr <<4) + add_1 );
										//ref_frame_1 = *(ref_pic_idL0 + (mb_nr2<<4) + add_2);
										ref_frame_0 = *(ref_idx_l0 + (mb_nr <<4) + add_1 );
										ref_frame_1 = *(ref_idx_l0 + (mb_nr2<<4) + add_2);
										tmp_mv = dec_params->dec_picture->mvL0;
									}

									//ref_frame_0 = (ref_frame_0 < 0) ? -32768 : ref_frame_0;
									//ref_frame_1 = (ref_frame_1 < 0) ? -32768 : ref_frame_1;
									
									add_1 = ((blk_y%4) << 3) + ((blk_x%4)<<1);
									add_2 = ((blk_y2%4) << 3) + ((blk_x2%4)<<1);
		
									if( ref_frame_0 != ref_frame_1 ||
										absz( *(tmp_mv + (mb_nr<<5) + add_1) - *(tmp_mv + (mb_nr2<<5) + add_2)) >= 4 ||
										absz( *(tmp_mv + (mb_nr<<5) + add_1+1) - *(tmp_mv + (mb_nr2<<5) + add_2+1)) >= 4 )
									{
										bS[i] = 1;
										break;
									}
								}

								/////////////////
								
								// for B-Frames run the loop twice 
								//for( l = 0; l < 1 + (i_slice_type == SLICE_TYPE_B); l++ )
								//{
								//    if( h->mb.ref[l][i8p] != h->mb.ref[l][i8q] ||
								//        abs( h->mb.mv[l][i4p][0] - h->mb.mv[l][i4q][0] ) >= 4 ||
								//        abs( h->mb.mv[l][i4p][1] - h->mb.mv[l][i4q][1] ) >= 4 )
								//    {
								//        bS[i] = 1;
								//        break;
								//    }
								//}
							}

							}
						}
					}
					if(bS[0]+bS[1]+bS[2]+bS[3] == 0)
					{
						continue;
					}
					
					/* *** filter *** */
					/* Y plane */
					i_qp = img->mb_data1.qp[mb_xy];   
					i_qpn= img->mb_data1.qp[mbn_xy];
					
					if( i_dir == 0 )
					{
						unsigned int l_stride = img->width + 64;
						unsigned int c_stride = img->width_cr + 32;


						/* vertical edge */
						deblock_edge( &LfCoefs, &dec_params->dec_picture->imgY[(16*mb_y+32) * l_stride + 16*mb_x + 4*i_edge + 32],
							l_stride, bS, (i_qp+i_qpn+1) >> 1, 0,
							deblock_h_luma_c, deblock_h_luma_intra_c );					


						
						if( !(i_edge & 1) )
						{
							/* U/V planes */
							int i_qpc = ( QP_SCALE_CR[ IClip_h264( i_qp + dec_params->dec_picture->chroma_qp_offset[0], 0, 51 )  ] +
								QP_SCALE_CR[ IClip_h264( i_qpn + dec_params->dec_picture->chroma_qp_offset[0], 0, 51 ) ] + 1 ) >> 1;

							
							deblock_edge( &LfCoefs, &dec_params->dec_picture->imgU[(8*mb_y+16) * c_stride + 8*mb_x + 2*i_edge+16],
								c_stride, bS, i_qpc, 1,
								deblock_h_chroma_c, deblock_h_chroma_intra_c );
							deblock_edge( &LfCoefs, &dec_params->dec_picture->imgV[(8*mb_y+16) * c_stride + 8*mb_x + 2*i_edge+16],
								c_stride, bS, i_qpc, 1,
								deblock_h_chroma_c, deblock_h_chroma_intra_c );

						}
						
					}
					
					else
					{
						unsigned int l_stride = img->width + 64;
						unsigned int c_stride = img->width_cr + 32;

								
						/* horizontal edge */
						deblock_edge( &LfCoefs, &dec_params->dec_picture->imgY[ (16*mb_y+32) * l_stride + 4*i_edge * l_stride + 16*mb_x + 32],
							l_stride, bS, (i_qp+i_qpn+1) >> 1, 0,
							deblock_v_luma_c, deblock_v_luma_intra_c );




						/* U/V planes */
						if( !(i_edge & 1) )
						{
							int i_qpc = ( QP_SCALE_CR[ IClip_h264( i_qp + dec_params->dec_picture->chroma_qp_offset[0], 0, 51 )  ] +
								QP_SCALE_CR[ IClip_h264( i_qpn + dec_params->dec_picture->chroma_qp_offset[0], 0, 51 ) ] + 1 ) >> 1;	// ????????????

							
							deblock_edge( &LfCoefs, &dec_params->dec_picture->imgU[(8*mb_y+16)*c_stride + 8*mb_x + 2*i_edge*c_stride+16],
								c_stride, bS, i_qpc, 1,
								deblock_v_chroma_c, deblock_v_chroma_intra_c );
							deblock_edge( &LfCoefs, &dec_params->dec_picture->imgV[(8*mb_y+16)*c_stride + 8*mb_x + 2*i_edge*c_stride+16],
								c_stride, bS, i_qpc, 1,
								deblock_v_chroma_c, deblock_v_chroma_intra_c );

						}
					}
					
				}
			}		

			}
		}

        /* newt mb */
        mb_x++;
        if( mb_x >= fwidth_mb )
        {
            mb_x = 0;
            mb_y++;
        }
    }	


}

//////////////////////