#include "mc.h"

static const int qpel_ref[16] =  {-1, 0,-1, 0, 1, 2, 3, 2,-1, 4,-1, 5, 1, 2, 3, 2};

static const imgpel mc_clip1_table[80+1+335] =
{
    /* -80 -> -1 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,
    /* 0 -> 255 */
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
    90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,101,102,103,104,105,106,107,
    108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,
    126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,
    162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,
    180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,
    198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
    216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,
    234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,
    252,253,254,255,
    /* 256 -> 340 */
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,
};

static __inline imgpel mc_clip1( int x )
{
    return mc_clip1_table[x+80];
}

static __inline int tapfilter( imgpel *pix, int i_pix_next )
{
    return pix[-2*i_pix_next] - 5*pix[-1*i_pix_next] + 20*(pix[0] + pix[1*i_pix_next]) - 5*pix[ 2*i_pix_next] + pix[ 3*i_pix_next];
}

static __inline int tapfilter1( imgpel *pix )
{
    return pix[-2] - 5*pix[-1] + 20*(pix[0] + pix[1]) - 5*pix[ 2] + pix[ 3];
}

static __inline void pixel_avg_wxh( imgpel *dst, int i_dst, imgpel *src, int i_src, int width, int height )
{
    int x, y;
    for( y = 0; y < height; y++ )
    {
        for( x = 0; x < width; x++ )
        {
            dst[x] = ( dst[x] + src[x] + 1 ) >> 1;
        }
        dst += i_dst;
        src += i_src;
    }
}

#define PIXEL_AVG_C( name, width, height ) \
void name( imgpel *pix1, int i_stride_pix1, \
                  imgpel *pix2, int i_stride_pix2 ) \
{ \
    pixel_avg_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height ); \
}
PIXEL_AVG_C( pixel_avg_16x16, 16, 16 )
PIXEL_AVG_C( pixel_avg_16x8,  16, 8 )
PIXEL_AVG_C( pixel_avg_8x16,  8, 16 )
PIXEL_AVG_C( pixel_avg_8x8,   8, 8 )
PIXEL_AVG_C( pixel_avg_8x4,   8, 4 )
PIXEL_AVG_C( pixel_avg_4x8,   4, 8 )
PIXEL_AVG_C( pixel_avg_4x4,   4, 4 )
PIXEL_AVG_C( pixel_avg_4x2,   4, 2 )
PIXEL_AVG_C( pixel_avg_2x4,   2, 4 )
PIXEL_AVG_C( pixel_avg_2x2,   2, 2 )

typedef void (*pixel_avg_h264)( imgpel *dst,  int i_dst_stride,
								imgpel *src1, imgpel *src2, int i_src_stride,
								int i_width, int i_height );

static void pixel_avg_h264_h_O( imgpel *dst,  int i_dst_stride,
								imgpel *src1, imgpel *src2, int i_src_stride,
								int i_width, int i_height )
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( mc_clip1( ( tapfilter1( &src1[x] ) + 16 ) >> 5 ) + src2[x] + 1 ) >> 1;
        }
        src1 += i_src_stride;
        src2 += i_src_stride;
        dst += i_dst_stride;
    }
}

static void pixel_avg_h264_O_v( imgpel *dst,  int i_dst_stride,
								imgpel *src1, imgpel *src2, int i_src_stride,
								int i_width, int i_height )
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( src1[x] + mc_clip1( ( tapfilter( &src2[x], i_src_stride ) + 16 ) >> 5 ) + 1 ) >> 1;
        }
        src1 += i_src_stride;
        src2 += i_src_stride;
        dst += i_dst_stride;
    }
}

static void pixel_avg_h264_h_v( imgpel *dst,  int i_dst_stride,
								imgpel *src1, imgpel *src2, int i_src_stride,
								int i_width, int i_height )
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( mc_clip1( ( tapfilter1( &src1[x] ) + 16 ) >> 5 ) 
                     + mc_clip1( ( tapfilter ( &src2[x], i_src_stride ) + 16 ) >> 5 )
                     + 1) >> 1;
        }
        src1 += i_src_stride;
        src2 += i_src_stride;
        dst += i_dst_stride;
    }
}

static void pixel_avg_h264_h_hv( imgpel *dst,  int i_dst_stride,
	 							 imgpel *src1, imgpel *src2, int i_src_stride,
								 int i_width, int i_height )
{
    imgpel *out;
    imgpel *pix2;
    int x, y;

    for( x = 0; x < i_width; x++ )
    {
        int tap[6];

        pix2 = &src2[x];
        out = &dst[x];

        tap[0] = tapfilter1( &pix2[-2*i_src_stride] );
        tap[1] = tapfilter1( &pix2[-1*i_src_stride] );
        tap[2] = tapfilter1( &pix2[ 0*i_src_stride] );
        tap[3] = tapfilter1( &pix2[ 1*i_src_stride] );
        tap[4] = tapfilter1( &pix2[ 2*i_src_stride] );

        for( y = 0; y < i_height; y++ )
        {
            tap[5] = tapfilter1( &pix2[ 3*i_src_stride] );

            *out = mc_clip1( ( tap[0] - 5*tap[1] + 20 * tap[2] + 20 * tap[3] -5*tap[4] + tap[5] + 512 ) >> 10 );

            /* Next line */
            pix2 += i_src_stride;
            out += i_dst_stride;
            tap[0] = tap[1];
            tap[1] = tap[2];
            tap[2] = tap[3];
            tap[3] = tap[4];
            tap[4] = tap[5];
        }
    }
    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( dst[x] + mc_clip1( ( tapfilter1( &src1[x] ) + 16 ) >> 5 ) + 1 ) >> 1;
        }
        dst  += i_dst_stride;
        src1 += i_src_stride;
    }
}

static void pixel_avg_h264_hv_v( imgpel *dst,  int i_dst_stride,
	 							 imgpel *src1, imgpel *src2, int i_src_stride,
								 int i_width, int i_height )
{
    imgpel *out;
    imgpel *pix1;
    int x, y;

    for( x = 0; x < i_width; x++ )
    {
        int tap[6];

        pix1 = &src1[x];
        out = &dst[x];

        tap[0] = tapfilter1( &pix1[-2*i_src_stride] );
        tap[1] = tapfilter1( &pix1[-1*i_src_stride] );
        tap[2] = tapfilter1( &pix1[ 0*i_src_stride] );
        tap[3] = tapfilter1( &pix1[ 1*i_src_stride] );
        tap[4] = tapfilter1( &pix1[ 2*i_src_stride] );

        for( y = 0; y < i_height; y++ )
        {
            tap[5] = tapfilter1( &pix1[ 3*i_src_stride] );

            *out = mc_clip1( ( tap[0] - 5*tap[1] + 20 * tap[2] + 20 * tap[3] -5*tap[4] + tap[5] + 512 ) >> 10 );

            /* Next line */
            pix1 += i_src_stride;
            out += i_dst_stride;
            tap[0] = tap[1];
            tap[1] = tap[2];
            tap[2] = tap[3];
            tap[3] = tap[4];
            tap[4] = tap[5];
        }
    }
    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( dst[x] + mc_clip1( ( tapfilter( &src2[x], i_src_stride ) + 16 ) >> 5 ) + 1 ) >> 1;
        }
        dst  += i_dst_stride;
        src2 += i_src_stride;
    }
}

pixel_avg_h264 pixel_average_h264[] = {
    pixel_avg_h264_h_O,
    pixel_avg_h264_O_v,
    pixel_avg_h264_h_v,
    pixel_avg_h264_h_hv,
    pixel_avg_h264_hv_v,
    pixel_avg_h264_hv_v
};
#define HPEL_H 0
#define HPEL_V 1
#define HPEL_C 2

#define WIDTH_4  4
#define WIDTH_8  8
#define WIDTH_16 16

static __inline void h264_mc_hpel_w4_h(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < WIDTH_4; x++ )
        {
            dst[x] = mc_clip1( ( tapfilter1( &src[x] ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}

static __inline void h264_mc_hpel_w4_v(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < WIDTH_4; x++ )
        {
            dst[x] = mc_clip1( ( tapfilter( &src[x], i_src_stride ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}

static __inline void h264_mc_hpel_w4_c(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    imgpel *out;
    imgpel *pix;
    int x, y;

    for( x = 0; x < WIDTH_4; x++ )
    {
        int tap[6];

        pix = &src[x];
        out = &dst[x];

        tap[0] = tapfilter1( &pix[-2*i_src_stride] );
        tap[1] = tapfilter1( &pix[-1*i_src_stride] );
        tap[2] = tapfilter1( &pix[ 0*i_src_stride] );
        tap[3] = tapfilter1( &pix[ 1*i_src_stride] );
        tap[4] = tapfilter1( &pix[ 2*i_src_stride] );

        for( y = 0; y < i_height; y++ )
        {
            tap[5] = tapfilter1( &pix[ 3*i_src_stride] );

            *out = mc_clip1( ( tap[0] - 5*tap[1] + 20 * tap[2] + 20 * tap[3] -5*tap[4] + tap[5] + 512 ) >> 10 );

            /* Next line */
            pix += i_src_stride;
            out += i_dst_stride;
            tap[0] = tap[1];
            tap[1] = tap[2];
            tap[2] = tap[3];
            tap[3] = tap[4];
            tap[4] = tap[5];
        }
    }
}

static __inline void h264_mc_hpel_w8_h(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < WIDTH_8; x++ )
        {
            dst[x] = mc_clip1( ( tapfilter1( &src[x] ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}

static __inline void h264_mc_hpel_w8_v(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < WIDTH_8; x++ )
        {
            dst[x] = mc_clip1( ( tapfilter( &src[x], i_src_stride ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}

static __inline void h264_mc_hpel_w8_c(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    imgpel *out;
    imgpel *pix;
    int x, y;

    for( x = 0; x < WIDTH_8; x++ )
    {
        int tap[6];

        pix = &src[x];
        out = &dst[x];

        tap[0] = tapfilter1( &pix[-2*i_src_stride] );
        tap[1] = tapfilter1( &pix[-1*i_src_stride] );
        tap[2] = tapfilter1( &pix[ 0*i_src_stride] );
        tap[3] = tapfilter1( &pix[ 1*i_src_stride] );
        tap[4] = tapfilter1( &pix[ 2*i_src_stride] );

        for( y = 0; y < i_height; y++ )
        {
            tap[5] = tapfilter1( &pix[ 3*i_src_stride] );

            *out = mc_clip1( ( tap[0] - 5*tap[1] + 20 * tap[2] + 20 * tap[3] -5*tap[4] + tap[5] + 512 ) >> 10 );

            /* Next line */
            pix += i_src_stride;
            out += i_dst_stride;
            tap[0] = tap[1];
            tap[1] = tap[2];
            tap[2] = tap[3];
            tap[3] = tap[4];
            tap[4] = tap[5];
        }
    }
}

static __inline void h264_mc_hpel_w16_h(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < WIDTH_16; x++ )
        {
            dst[x] = mc_clip1( ( tapfilter1( &src[x] ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}

static __inline void h264_mc_hpel_w16_v(imgpel *dst, int i_dst_stride,
                                imgpel *src, int i_src_stride,
								int i_height)
{
    int x, y;

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < WIDTH_16; x++ )
        {
            dst[x] = mc_clip1( ( tapfilter( &src[x], i_src_stride ) + 16 ) >> 5 );
        }
        src += i_src_stride;
        dst += i_dst_stride;
    }
}

static __inline void h264_mc_hpel_w16_c(imgpel *dst, int i_dst_stride,
                                        imgpel *src, int i_src_stride,
                                        int i_height)
{
    imgpel *out;
    imgpel *pix;
    int x, y;

    for( x = 0; x < WIDTH_16; x++ )
    {
        int tap[6];

        pix = &src[x];
        out = &dst[x];

        tap[0] = tapfilter1( &pix[-2*i_src_stride] );
        tap[1] = tapfilter1( &pix[-1*i_src_stride] );
        tap[2] = tapfilter1( &pix[ 0*i_src_stride] );
        tap[3] = tapfilter1( &pix[ 1*i_src_stride] );
        tap[4] = tapfilter1( &pix[ 2*i_src_stride] );

        for( y = 0; y < i_height; y++ )
        {
            tap[5] = tapfilter1( &pix[ 3*i_src_stride] );

            *out = mc_clip1( ( tap[0] - 5*tap[1] + 20 * tap[2] + 20 * tap[3] -5*tap[4] + tap[5] + 512 ) >> 10 );

            /* Next line */
            pix += i_src_stride;
            out += i_dst_stride;
            tap[0] = tap[1];
            tap[1] = tap[2];
            tap[2] = tap[3];
            tap[3] = tap[4];
            tap[4] = tap[5];
        }
    }
}

static void (* const hpel_compensation[3][3])(imgpel *dst, int i_dst_stride,
                                              imgpel *src, int i_src_stride,
                                              int i_height) =
{
    { h264_mc_hpel_w4_h, h264_mc_hpel_w8_h, h264_mc_hpel_w16_h },
    { h264_mc_hpel_w4_v, h264_mc_hpel_w8_v, h264_mc_hpel_w16_v },
    { h264_mc_hpel_w4_c, h264_mc_hpel_w8_c, h264_mc_hpel_w16_c },
};

void mc_luma( imgpel *src, int i_src_stride,
              imgpel *dst,    int i_dst_stride,
              int mvx,int mvy,
              int i_width, int i_height )
{
    int qpel_idx = ((mvy&3)<<2) + (mvx&3);
    int offset = (mvy>>2)*i_src_stride + (mvx>>2);

	if( qpel_idx == 0)//0000
    {
        imgpel *src1 = src + offset;
        mc_copy( src1, i_src_stride, dst, i_dst_stride, i_width, i_height );
    }
    else if( qpel_idx ==  2)//0010
    {
        imgpel *src1 = src + offset;
        hpel_compensation[HPEL_H][i_width>>3]( dst, i_dst_stride, src1, i_src_stride, i_height );
    }
    else if( qpel_idx ==  8)//1000
    {
        imgpel *src1 = src + offset;
        hpel_compensation[HPEL_V][i_width>>3]( dst, i_dst_stride, src1, i_src_stride, i_height );
    }
    else if( qpel_idx == 10)//1010
    {
        imgpel *src1 = src + offset;
        hpel_compensation[HPEL_C][i_width>>3]( dst, i_dst_stride, src1, i_src_stride, i_height );
    }
    else //if( qpel_idx & 5 ) /* qpel interpolation needed */
    {
        imgpel *src1 = src + offset + ((mvy&3) == 3) * i_src_stride;
        imgpel *src2 = src + offset + ((mvx&3) == 3);
        pixel_average_h264[qpel_ref[qpel_idx]]( dst, i_dst_stride, src1, src2,
            i_src_stride, i_width, i_height );
    }

}

void mc_chroma( imgpel *src, int i_src_stride,
                imgpel *dst, int i_dst_stride,
                int mvx, int mvy,
                int i_width, int i_height )
{
	
    imgpel *srcp;
    int x, y;

    const int d8x = mvx&0x07;
    const int d8y = mvy&0x07;

    const int cA = (8-d8x)*(8-d8y);
    const int cB = d8x    *(8-d8y);
    const int cC = (8-d8x)*d8y;
    const int cD = d8x    *d8y;

    src  += (mvy >> 3) * i_src_stride + (mvx >> 3);
    srcp = &src[i_src_stride];

    for( y = 0; y < i_height; y++ )
    {
        for( x = 0; x < i_width; x++ )
        {
            dst[x] = ( cA*src[x]  + cB*src[x+1] +
                       cC*srcp[x] + cD*srcp[x+1] + 32 ) >> 6;
        }
        dst  += i_dst_stride;

        src   = srcp;
        srcp += i_src_stride;
    }
}

