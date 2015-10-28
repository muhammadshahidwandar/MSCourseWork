#include "global.h"

typedef void (*predict_intra_16x16)( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour );

typedef void (*predict_intra_4x4)  ( imgpel *src, unsigned int src_stride, imgpel *dst, unsigned int dst_stride, int neighbour );

typedef void (*predict_intra_8x8)  ( imgpel *src, unsigned int stride_chroma, imgpel *dst, unsigned int dst_stride_chroma, int neighbour );