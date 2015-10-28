#include "predict.h"

extern const imgpel allowed_pixel_value[512];
/****************************************************************************
 * 16x16 prediction for intra luma block
 ****************************************************************************/
static void predict_16x16_h( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    int i;
    for( i = 0; i < 16; i++ )
    {
        const unsigned int v = 0x01010101 * src[-1];
        unsigned int *p = (unsigned int*)dst;
        *p++ = v;
        *p++ = v;
        *p++ = v;
        *p++ = v;

        src += src_stride;
        dst += dst_stride;
    }
}

static void predict_16x16_v( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    unsigned int v0 = *(unsigned int*)&src[ 0-src_stride];
    unsigned int v1 = *(unsigned int*)&src[ 4-src_stride];
    unsigned int v2 = *(unsigned int*)&src[ 8-src_stride];
    unsigned int v3 = *(unsigned int*)&src[12-src_stride];
    int i;
    for( i = 0; i < 16; i++ )
    {
        unsigned int *p = (unsigned int*)dst;
        *p++ = v0;
        *p++ = v1;
        *p++ = v2;
        *p++ = v3;

        dst += dst_stride;
    }
}

#define PREDICT_16x16_DC(v, dst_stride) \
    for( i = 0; i < 16; i++ )\
    {\
        unsigned int *p = (unsigned int*)dst;\
        *p++ = v;\
        *p++ = v;\
        *p++ = v;\
        *p++ = v;\
        dst += dst_stride;\
    }

#define DC_16X16      3
#define DC_LEFT_16X16 2
#define DC_TOP_16X16  1
#define DC_128_16X16  0

static void predict_16x16_dc( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    unsigned int dc = 0;
    int i;
    
    switch(neighbour)
    {
    case DC_16X16:
        for( i = 0; i < 16; i++ )
        {
            dc += src[-1 + i * src_stride];
            dc += src[i - src_stride];
        }
        dc = (( dc + 16 ) >> 5) * 0x01010101;
        break;
    case DC_LEFT_16X16:
        for( i = 0; i < 16; i++ )
        {
            dc += src[-1 + i * src_stride];
        }
        dc = (( dc + 8 ) >> 4) * 0x01010101;
        break;
    case DC_TOP_16X16:
        for( i = 0; i < 16; i++ )
        {
            dc += src[i - src_stride];
        }
        dc = (( dc + 8 ) >> 4) * 0x01010101;
        break;
    default:
        dc = 0x80808080;
        break;
    }
    PREDICT_16x16_DC(dc, dst_stride);
}

static void predict_16x16_p( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    int x, y, i;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    /* calcule H and V */
    for( i = 0; i <= 7; i++ )
    {
        H += ( i + 1 ) * ( src[ 8 + i - src_stride ] - src[6 -i -src_stride] );
        V += ( i + 1 ) * ( src[-1 + (8+i)*src_stride] - src[-1 + (6-i)*src_stride] );
    }

    a = 16 * ( src[-1 + 15*src_stride] + src[15 - src_stride] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;

    i00 = a - b * 7 - c * 7 + 16;

    for( y = 0; y < 16; y++ )
    {
        int pix = i00;
        for( x = 0; x < 16; x++ )
        {
            dst[x] = allowed_pixel_value[( pix>>5 ) + 128];
            pix += b;
        }
        dst += dst_stride;
        i00 += c;
    }
}

predict_intra_16x16 predict_16x16[4] = {predict_16x16_v,
                                        predict_16x16_h,
                                        predict_16x16_dc,
                                        predict_16x16_p};

/****************************************************************************
* 4x4 prediction for intra luma block
****************************************************************************/

#define STRIDE_MPR 16

#define DC_4X4      3
#define DC_LEFT_4X4 2
#define DC_TOP_4X4  1
#define DC_128_4X4  0

static void predict_4x4_dc( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;
    
    unsigned int dc = 0x80808080;
    int left = ( ( (neighbour >> 4) & 3 ) != 0 );// so left is always available
    int top  = ( (neighbour >> 8) != 0 );// so top is always available
    int imode = neighbour & 3;
        
    //imode == 1 top available;
    //imode == 2 left available;
    //imode == 3 left and top available;

    imode |= ( (left << 1) | top );
    switch(imode)
    {
    case DC_4X4:
        dc = (( src[-1+0*src_stride] + src[-1+src_stride] +
            src[-1+2*src_stride] + src[-1+3*src_stride] +
            src[0 - src_stride]  + src[1 - src_stride] +
            src[2 - src_stride]  + src[3 - src_stride] + 4 ) >> 3)*0x01010101;
        break;
    case DC_LEFT_4X4:
        dc = (( src[-1+0*src_stride] + src[-1+src_stride] +
            src[-1+2*src_stride] + src[-1+3*src_stride] + 2 ) >> 2)*0x01010101;
        break;
    case DC_TOP_4X4:
        dc = (( src[0 - src_stride] + src[1 - src_stride] +
            src[2 - src_stride] + src[3 - src_stride] + 2 ) >> 2)*0x01010101;
        break;
    default:
        break;
    }
    *(unsigned int*)&dst[dst_row_0] = 
    *(unsigned int*)&dst[dst_row_1] = 
    *(unsigned int*)&dst[dst_row_2] = 
    *(unsigned int*)&dst[dst_row_3] = dc;
}

static void predict_4x4_h( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;
    
    *(unsigned int*)&dst[dst_row_0] = src[0*src_stride-1] * 0x01010101;
    *(unsigned int*)&dst[dst_row_1] = src[1*src_stride-1] * 0x01010101;
    *(unsigned int*)&dst[dst_row_2] = src[2*src_stride-1] * 0x01010101;
    *(unsigned int*)&dst[dst_row_3] = src[3*src_stride-1] * 0x01010101;
}

static void predict_4x4_v( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;
    
    unsigned int top = *((unsigned int*)&src[-src_stride]);
    *(unsigned int*)&dst[dst_row_0] = 
    *(unsigned int*)&dst[dst_row_1] = 
    *(unsigned int*)&dst[dst_row_2] = 
    *(unsigned int*)&dst[dst_row_3] = top;
}

static void predict_4x4_ddl( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;
    
    const int t0 = src[0-1*src_stride];   
    const int t1 = src[1-1*src_stride];   
    const int t2 = src[2-1*src_stride];   
    const int t3 = src[3-1*src_stride];
    int t4, t5, t6, t7;
    const int emulate = ( ( neighbour & 0x110 ) == 0x110 ) || ( ( neighbour & 0x230 ) == 0x230 )
    || (((neighbour & 4) != 4 ) && ( ( neighbour & 0x030 ) == 0x030 ));
      
    if(!emulate)
    {
        t4 = src[4-1*src_stride];
        t5 = src[5-1*src_stride];
        t6 = src[6-1*src_stride];
        t7 = src[7-1*src_stride];
    }
    else
    {
        t4 = 
        t5 = 
        t6 = 
        t7 = t3;
    }
    dst[dst_row_0+0] = ( t0 + 2*t1 + t2 + 2 ) >> 2;

    dst[dst_row_0+1] =
    dst[dst_row_1+0] = ( t1 + 2*t2 + t3 + 2 ) >> 2;

    dst[dst_row_0+2] =
    dst[dst_row_1+1] =
    dst[dst_row_2+0] = ( t2 + 2*t3 + t4 + 2 ) >> 2;

    dst[dst_row_0+3] =
    dst[dst_row_1+2] =
    dst[dst_row_2+1] =
    dst[dst_row_3+0] = ( t3 + 2*t4 + t5 + 2 ) >> 2;

    dst[dst_row_1+3] =
    dst[dst_row_2+2] =
    dst[dst_row_3+1] = ( t4 + 2*t5 + t6 + 2 ) >> 2;

    dst[dst_row_2+3] =
    dst[dst_row_3+2] = ( t5 + 2*t6 + t7 + 2 ) >> 2;

    dst[dst_row_3+3] = ( t6 + 3*t7 + 2 ) >> 2;
}

static void predict_4x4_ddr( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;
    
    const int lt = src[-1-src_stride];
    const int l0 = src[-1+0*src_stride];
    const int l1 = src[-1+1*src_stride];
    const int l2 = src[-1+2*src_stride];
    const int l3 = src[-1+3*src_stride];
    
    const int t0 = src[0-1*src_stride];
    const int t1 = src[1-1*src_stride];
    const int t2 = src[2-1*src_stride];
    const int t3 = src[3-1*src_stride];
    
    dst[dst_row_0+0] =
        dst[dst_row_1+1] =
        dst[dst_row_2+2] =
        dst[dst_row_3+3] = ( t0 + 2 * lt + l0 + 2 ) >> 2;
    
    dst[dst_row_0+1] =
        dst[dst_row_1+2] =
        dst[dst_row_2+3] = ( lt + 2 * t0 + t1 + 2 ) >> 2;
    
    dst[dst_row_0+2] =
        dst[dst_row_1+3] = ( t0 + 2 * t1 + t2 + 2 ) >> 2;
    
    dst[dst_row_0+3] = ( t1 + 2 * t2 + t3 + 2 ) >> 2;
    
    dst[dst_row_1+0] =
        dst[dst_row_2+1] =
        dst[dst_row_3+2] = ( lt + 2 * l0 + l1 + 2 ) >> 2;
    
    dst[dst_row_2+0] =
        dst[dst_row_3+1] = ( l0 + 2 * l1 + l2 + 2 ) >> 2;
    
    dst[dst_row_3+0] = ( l1 + 2 * l2 + l3 + 2 ) >> 2;
}

static void predict_4x4_vr( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;
    
    const int lt = src[-1-src_stride];
    const int l0 = src[-1+0*src_stride];
    const int l1 = src[-1+1*src_stride];
    const int l2 = src[-1+2*src_stride];
    const int l3 = src[-1+3*src_stride];
    
    const int t0 = src[0-1*src_stride];
    const int t1 = src[1-1*src_stride];
    const int t2 = src[2-1*src_stride];
    const int t3 = src[3-1*src_stride];
    
    dst[dst_row_0+0]=
        dst[dst_row_2+1]= ( lt + t0 + 1 ) >> 1;
    
    dst[dst_row_0+1]=
        dst[dst_row_2+2]= ( t0 + t1 + 1 ) >> 1;
    
    dst[dst_row_0+2]=
        dst[dst_row_2+3]= ( t1 + t2 + 1 ) >> 1;
    
    dst[dst_row_0+3]= ( t2 + t3 + 1 ) >> 1;
    
    dst[dst_row_1+0]=
        dst[dst_row_3+1]= ( l0 + 2 * lt + t0 + 2 ) >> 2;
    
    dst[dst_row_1+1]=
        dst[dst_row_3+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;
    
    dst[dst_row_1+2]=
        dst[dst_row_3+3]= ( t0 + 2 * t1 + t2 + 2) >> 2;
    
    dst[dst_row_1+3]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;
    dst[dst_row_2+0]= ( lt + 2 * l0 + l1 + 2 ) >> 2;
    dst[dst_row_3+0]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;
}

static void predict_4x4_hd( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;

    const int lt= src[-1-1*src_stride];
    const int l0 = src[-1+0*src_stride];
    const int l1 = src[-1+1*src_stride];
    const int l2 = src[-1+2*src_stride];
    const int l3 = src[-1+3*src_stride];
    const int t0 = src[0-1*src_stride];
    const int t1 = src[1-1*src_stride];
    const int t2 = src[2-1*src_stride];
    const int t3 = src[3-1*src_stride];
    
    dst[dst_row_0+0]=
        dst[dst_row_1+2]= ( lt + l0 + 1 ) >> 1;
    dst[dst_row_0+1]=
        dst[dst_row_1+3]= ( l0 + 2 * lt + t0 + 2 ) >> 2;
    dst[dst_row_0+2]= ( lt + 2 * t0 + t1 + 2 ) >> 2;
    dst[dst_row_0+3]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;
    dst[dst_row_1+0]=
        dst[dst_row_2+2]= ( l0 + l1 + 1 ) >> 1;
    dst[dst_row_1+1]=
        dst[dst_row_2+3]= ( lt + 2 * l0 + l1 + 2 ) >> 2;
    dst[dst_row_2+0]=
        dst[dst_row_3+2]= ( l1 + l2+ 1 ) >> 1;
    dst[dst_row_2+1]=
        dst[dst_row_3+3]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;
    dst[dst_row_3+0]= ( l2 + l3 + 1 ) >> 1;
    dst[dst_row_3+1]= ( l1 + 2 * l2 + l3 + 2 ) >> 2;
}

static void predict_4x4_vl( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;

    const int t0 = src[0-1*src_stride];
    const int t1 = src[1-1*src_stride];
    const int t2 = src[2-1*src_stride];
    const int t3 = src[3-1*src_stride];
    //const int emulate = ( ( neighbour & 0x110 ) == 0x110 ) || ( ( neighbour & 0x230 ) == 0x230 );
    int t4, t5, t6, t7;
    const int emulate = ( ( neighbour & 0x110 ) == 0x110 ) || ( ( neighbour & 0x230 ) == 0x230 )
    || (((neighbour & 4) != 4 ) && ( ( neighbour & 0x030 ) == 0x030 ));
    
    if(!emulate)
    {
        t4 = src[4-1*src_stride];
        t5 = src[5-1*src_stride];
        t6 = src[6-1*src_stride];
        t7 = src[7-1*src_stride];
    }
    else
    {
        t4 = 
        t5 = 
        t6 = 
        t7 = t3;
    }
    
    dst[dst_row_0+0]= ( t0 + t1 + 1 ) >> 1;
    dst[dst_row_0+1]=
        dst[dst_row_2+0]= ( t1 + t2 + 1 ) >> 1;
    dst[dst_row_0+2]=
        dst[dst_row_2+1]= ( t2 + t3 + 1 ) >> 1;
    dst[dst_row_0+3]=
        dst[dst_row_2+2]= ( t3 + t4 + 1 ) >> 1;
    dst[dst_row_2+3]= ( t4 + t5 + 1 ) >> 1;
    dst[dst_row_1+0]= ( t0 + 2 * t1 + t2 + 2 ) >> 2;
    dst[dst_row_1+1]=
        dst[dst_row_3+0]= ( t1 + 2 * t2 + t3 + 2 ) >> 2;
    dst[dst_row_1+2]=
        dst[dst_row_3+1]= ( t2 + 2 * t3 + t4 + 2 ) >> 2;
    dst[dst_row_1+3]=
        dst[dst_row_3+2]= ( t3 + 2 * t4 + t5 + 2 ) >> 2;
    dst[dst_row_3+3]= ( t4 + 2 * t5 + t6 + 2 ) >> 2;
}

static void predict_4x4_hu( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour )
{
    const int dst_row_0 = 0;
    const int dst_row_1 = dst_stride;
    const int dst_row_2 = dst_stride * 2;
    const int dst_row_3 = dst_row_1 + dst_row_2;
    
    const int l0 = src[-1+0*src_stride];
    const int l1 = src[-1+1*src_stride];
    const int l2 = src[-1+2*src_stride];
    const int l3 = src[-1+3*src_stride];
    
    
    dst[dst_row_0+0]= ( l0 + l1 + 1 ) >> 1;
    dst[dst_row_0+1]= ( l0 + 2 * l1 + l2 + 2 ) >> 2;
    
    dst[dst_row_0+2]=
        dst[dst_row_1+0]= ( l1 + l2 + 1 ) >> 1;
    
    dst[dst_row_0+3]=
        dst[dst_row_1+1]= ( l1 + 2*l2 + l3 + 2 ) >> 2;
    
    dst[dst_row_1+2]=
        dst[dst_row_2+0]= ( l2 + l3 + 1 ) >> 1;
    
    dst[dst_row_1+3]=
        dst[dst_row_2+1]= ( l2 + 2 * l3 + l3 + 2 ) >> 2;
    
    dst[dst_row_2+3]=
        dst[dst_row_3+1]=
        dst[dst_row_3+0]=
        dst[dst_row_2+2]=
        dst[dst_row_3+2]=
        dst[dst_row_3+3]= l3;
}

predict_intra_4x4   predict_4x4[9] = {predict_4x4_v,
                                      predict_4x4_h,
                                      predict_4x4_dc,
                                      predict_4x4_ddl,
                                      predict_4x4_ddr,
                                      predict_4x4_vr,
                                      predict_4x4_hd,
                                      predict_4x4_vl,
                                      predict_4x4_hu};

/****************************************************************************
 * 8x8 prediction for intra chroma block
 ****************************************************************************/
#define DC_8X8      3
#define DC_LEFT_8X8 2
#define DC_TOP_8X8  1
#define DC_128_8X8  0

static void predict_8x8c_dc( unsigned char *src, unsigned int stride_chroma, unsigned char *dst, unsigned int dst_stride_chroma, int neighbour )
{
    int x, y;
    int s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    unsigned int dc0 = 0;
    unsigned int dc1 = 0;
    unsigned int dc2 = 0;
    unsigned int dc3 = 0;
    int i;

    switch(neighbour)
    {
    case DC_8X8:
        for( i = 0; i < 4; i++ )
        {
            s0 += src[i - stride_chroma];        //  s0 s1
            s1 += src[i + 4 - stride_chroma];    //s2
            s2 += src[-1 + i * stride_chroma];   //s3
            s3 += src[-1 + (i+4)*stride_chroma];
        }
        //dc0 = (( s0 + s2 + 4 ) >> 3)*0x01010101;    //dc0 dc1
        //dc1 = (( s1 + 2 ) >> 2)*0x01010101;         //dc2 dc3
        //dc2 = (( s3 + 2 ) >> 2)*0x01010101;
        //dc3 = (( s1 + s3 + 4 ) >> 3)*0x01010101;
        
        dc0 = (( s0 + s2 + 4 ) >> 3);    //dc0 dc1
        dc1 = (( s1 + 2 ) >> 2);         //dc2 dc3
        dc2 = (( s3 + 2 ) >> 2);
        dc3 = (( s1 + s3 + 4 ) >> 3);
        
        break;
    case DC_LEFT_8X8:
        for( y = 0; y < 4; y++ )
        {
            dc0 += src[y * stride_chroma     - 1];
            dc2 += src[(y+4) * stride_chroma - 1];
        }
        //dc0 = dc1 = (( dc0 + 2 ) >> 2)*0x01010101;
        //dc2 = dc3 = (( dc2 + 2 ) >> 2)*0x01010101;
        
        dc0 = dc1 = (( dc0 + 2 ) >> 2);
        dc2 = dc3 = (( dc2 + 2 ) >> 2);
        
        break;
    case DC_TOP_8X8:
        for( x = 0; x < 4; x++ )
        {
            dc0 += src[x     - stride_chroma];
            dc1 += src[x + 4 - stride_chroma];
        }
        //dc0 = dc2 = (( dc0 + 2 ) >> 2)*0x01010101;
        //dc1 = dc3 = (( dc1 + 2 ) >> 2)*0x01010101;
        dc0 = dc2 = (( dc0 + 2 ) >> 2);
        dc1 = dc3 = (( dc1 + 2 ) >> 2);
        
        break;
    default:
        //dc0 = 
        //dc1 = 
        //dc2 = 
        //dc3 = 0x80808080;
        dc0 = 
        dc1 = 
        dc2 = 
        dc3 = (unsigned char)0x80;
        break;
    }
    for( y = 0; y < 4; y++ )
    {
        //unsigned int *p = (unsigned int*)dst;
        //*p++ = dc0;
        //*p++ = dc1;
        dst[0] = 
        dst[1] = 
        dst[2] = 
        dst[3] = (unsigned char)(dc0);
        dst[4] = 
        dst[5] = 
        dst[6] = 
        dst[7] = (unsigned char)(dc1);
        dst+=dst_stride_chroma;
    }

    for( y = 0; y < 4; y++ )
    {
        //unsigned int *p = (unsigned int*)dst;
        //*p++ = dc2;
        //*p++ = dc3;
        dst[0] = 
        dst[1] = 
        dst[2] = 
        dst[3] = (unsigned char)(dc2);
        dst[4] = 
        dst[5] = 
        dst[6] = 
        dst[7] = (unsigned char)(dc3);
        dst+=dst_stride_chroma;
    }
/*
    {


        unsigned int *p = (unsigned int*)dst;

        dst_stride_chroma = (dst_stride_chroma-8)/4;
        *p++ = dc0;
        *p++ = dc1;
        p+=dst_stride_chroma;
        *p++ = dc0;
        *p++ = dc1;
        p+=dst_stride_chroma;
        *p++ = dc0;
        *p++ = dc1;
        p+=dst_stride_chroma;
        *p++ = dc0;
        *p++ = dc1;
        p+=dst_stride_chroma;
        
        *p++ = dc2;
        *p++ = dc3;
        p+=dst_stride_chroma;
        *p++ = dc2;
        *p++ = dc3;
        p+=dst_stride_chroma;
        *p++ = dc2;
        *p++ = dc3;
        p+=dst_stride_chroma;
        *p++ = dc2;
        *p++ = dc3;
    }
*/
}

static void predict_8x8c_h( unsigned char *src, unsigned int stride_chroma, unsigned char *dst, unsigned int dst_stride_chroma, int neighbour )
{
    int i;
    for( i = 0; i < 8; i++ )
    {
        //unsigned int *p = (unsigned int*)dst;
        //unsigned int v = 0x01010101 * src[-1];
        //*p++ = v;
        //*p++ = v;
        dst[0] = 
        dst[1] = 
        dst[2] = 
        dst[3] = 
        dst[4] = 
        dst[5] = 
        dst[6] = 
        dst[7] = src[-1];
        dst +=dst_stride_chroma;
        src += stride_chroma;
    }
}
static void predict_8x8c_v( unsigned char *src, unsigned int stride_chroma, unsigned char *dst, unsigned int dst_stride_chroma, int neighbour )
{
    //unsigned int v0 = *(unsigned int*)&src[0-stride_chroma];
    //unsigned int v1 = *(unsigned int*)&src[4-stride_chroma];
    int i;
    for( i = 0; i < 8; i++ )
    {
        //unsigned int *p = (unsigned int*)dst;
        //*p++ = v0;
        //*p++ = v1;
        dst[0] = src[0-stride_chroma];
        dst[1] = src[1-stride_chroma];
        dst[2] = src[2-stride_chroma];
        dst[3] = src[3-stride_chroma];
        dst[4] = src[4-stride_chroma];
        dst[5] = src[5-stride_chroma];
        dst[6] = src[6-stride_chroma];
        dst[7] = src[7-stride_chroma];
        dst+=dst_stride_chroma;
    }
}
static void predict_8x8c_p( unsigned char *src, unsigned int stride_chroma, unsigned char *dst, unsigned int dst_stride_chroma, int neighbour )
{
    int i;
    int x,y;
    int a, b, c;
    int H = 0;
    int V = 0;
    int i00;

    for( i = 0; i < 4; i++ )
    {
        H += ( i + 1 ) * ( src[4+i - stride_chroma] - src[2 - i -stride_chroma] );
        V += ( i + 1 ) * ( src[-1 +(i+4)*stride_chroma] - src[-1+(2-i)*stride_chroma] );
    }

    a = 16 * ( src[-1+7*stride_chroma] + src[7 - stride_chroma] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;

    for( y = 0; y < 8; y++ )
    {
        int pix = i00;
        for( x = 0; x < 8; x++ )
        {
            dst[x] = allowed_pixel_value[( pix>>5 ) + 128];
            pix += b;
        }
        dst += dst_stride_chroma;
        i00 += c;
    }
}

predict_intra_8x8 predict_8x8[4] = {predict_8x8c_dc,
                                    predict_8x8c_h,
                                    predict_8x8c_v,
                                    predict_8x8c_p};