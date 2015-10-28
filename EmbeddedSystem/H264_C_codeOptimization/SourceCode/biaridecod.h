/*!
 ***************************************************************************
 * \file
 *    biaridecod.h
 *
 * \brief
 *    Headerfile for binary arithmetic decoder routines
 *
 * \author
 *    Detlev Marpe,
 *    Gabi Blättermann
 *    Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *
 * \date
 *    21. Oct 2000
 **************************************************************************
 */
//#ifdef __CABAC__

#ifndef _BIARIDECOD_H_
#define _BIARIDECOD_H_

#include "global.h"

/************************************************************************
 * D e f i n i t i o n s
 ***********************************************************************
 */

MotionInfoContexts* create_contexts_MotionInfo();
TextureInfoContexts* create_contexts_TextureInfo();
void delete_contexts_MotionInfo(MotionInfoContexts *enco_ctx);
void delete_contexts_TextureInfo(TextureInfoContexts *enco_ctx);

void arideco_start_decoding(DecodingEnvironmentPtr eep, unsigned char *code_buffer, int firstbyte, int *code_len, int slice_type);
int  arideco_bits_read(DecodingEnvironmentPtr dep);
void arideco_done_decoding(DecodingEnvironmentPtr dep);
void biari_init_context (int qp, BiContextTypePtr ctx, const int* ini);
unsigned int biari_decode_symbol(DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct );
unsigned int biari_decode_symbol_eq_prob(DecodingEnvironmentPtr dep);
unsigned int biari_decode_final(DecodingEnvironmentPtr dep);

#endif  // BIARIDECOD_H_

//#endif  // __CABAC__