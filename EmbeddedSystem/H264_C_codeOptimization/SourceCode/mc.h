#include "global.h"

enum
{
    PIXEL_16x16 = 0,
    PIXEL_16x8  = 1,
    PIXEL_8x16  = 2,
    PIXEL_8x8   = 3,
    PIXEL_8x4   = 4,
    PIXEL_4x8   = 5,
    PIXEL_4x4   = 6,
    PIXEL_4x2   = 7,
    PIXEL_2x4   = 8,
    PIXEL_2x2   = 9,
};

void mc_luma( imgpel *src, int i_src_stride,
              imgpel *dst,    int i_dst_stride,
              int mvx,int mvy,
              int i_width, int i_height );

void mc_chroma( imgpel *src, int i_src_stride,
                       imgpel *dst, int i_dst_stride,
                       int mvx, int mvy,
                       int i_width, int i_height );

#define PIXEL_AVG_C_DEC( name) \
void name( imgpel *pix1, int i_stride_pix1, \
           imgpel *pix2, int i_stride_pix2 ); \

PIXEL_AVG_C_DEC( pixel_avg_16x16)
PIXEL_AVG_C_DEC( pixel_avg_16x8)
PIXEL_AVG_C_DEC( pixel_avg_8x16)
PIXEL_AVG_C_DEC( pixel_avg_8x8)
PIXEL_AVG_C_DEC( pixel_avg_8x4)
PIXEL_AVG_C_DEC( pixel_avg_4x8)
PIXEL_AVG_C_DEC( pixel_avg_4x4)
PIXEL_AVG_C_DEC( pixel_avg_4x2)
PIXEL_AVG_C_DEC( pixel_avg_2x4)
PIXEL_AVG_C_DEC( pixel_avg_2x2)

__inline void mc_copy( imgpel *src, int i_src_stride, 
					   imgpel *dst, int i_dst_stride, 
					   int i_width, int i_height )
{
    int y;

    for( y = 0; y < i_height; y++ )
    {
        memcpy( dst, src, i_width );

        src += i_src_stride;
        dst += i_dst_stride;
    }
}